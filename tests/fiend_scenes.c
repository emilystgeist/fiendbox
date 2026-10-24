/* SPDX-License-Identifier: GPL-3.0-only */
/* FIENDBOX audition: a candidate scene (4 parts, HALL, TAPE) rendered to a WAV as the device plays it after
 * power-on + PLAY: four passes, then the tail.   build/host/fiend_scenes OUTDIR
 * Not a test: an ear check while the scenes are written. */
#define main hostsim_main
#include "hostsim.c"
#undef main

typedef struct { uint32_t e; const char *name; uint32_t pat; int16_t lvl, rev; } part_t;   /* engine id, sound, PATTERNS[pat - 1] */

static uint32_t preset_of(uint32_t e, const char *name)
{
    uint32_t i;
    for (i = 0; i < ENGINES[e]->npresets; i++)
        if (!strcmp(ENGINES[e]->presets[i].name, name))
            return i;
    fprintf(stderr, "fiend scenes: engine %u has no sound %s\n", (unsigned)e, name);
    exit(1);
}

/* LABYRINTH's candidate patterns, A minor, 1/8 steps (100 + index); 0 = rest, flags 1 accent, 2 slide, 4 tie */
#define T_ 4
static const struct { const char *name; uint8_t note[16], flags[16]; } EXTRA[] = {
    {"CRAWL", {33, 0, 33, 45, 33, 0, 36, 0, 33, 0, 33, 45, 31, 0, 29, 0},          /* 100 bass: A, octave jumps, G F */
     {1, 0, 0, 2, 0, 0, 1, 0, 1, 0, 0, 2, 0, 0, 1, 0}},
    {"DREAD", {57, 0, 0, 0, 53, 0, 0, 0, 55, 0, 0, 0, 52, 0, 0, 0},                /* 101 pad roots: Am F G E */
     {0, T_, T_, T_, 0, T_, T_, T_, 0, T_, T_, T_, 0, T_, T_, T_}},
    {"SIREN", {76, 0, 0, 75, 0, 0, 72, 0, 0, 0, 69, 0, 0, 0, 0, 0},                /* 102 lead: E, Eb (the tritone's cousin), C, A */
     {1, T_, 0, 2, T_, 0, 1, T_, 0, 0, 1, T_, T_, 0, 0, 0}},
    {"RUSTBEAT", {36, 0, 42, 0, 38, 0, 42, 36, 36, 0, 42, 0, 38, 0, 36, 0},     /* 103 drums: GM kick, hat, snare */
     {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0}},
};
static uint8_t pat_note(uint32_t n, uint32_t t) { return !n ? 0u : n >= 100u ? EXTRA[n - 100u].note[t] : PATTERNS[n - 1u].note[t]; }
static uint8_t pat_flags(uint32_t n, uint32_t t) { return !n ? 0u : n >= 100u ? EXTRA[n - 100u].flags[t] : PATTERNS[n - 1u].flags[t]; }

static void render_scene(const char *dir, const char *file, uint32_t bpm, uint32_t tape, const part_t *p)
{
    char path[512];
    FILE *f;
    uint32_t t, i, stopped = 0, pass = 480u * FS / bpm, frames = 4u * pass + 8u * FS;   /* 16 eighth notes a pass */
    int32_t o[2 * CTL];
    memset(trk, 0, sizeof trk);
    host_tracks_init();
    for (i = 0; i < G_COUNT; i++)
        song.g[i] = GP[i].def;
    song.g[G_BPM] = (int16_t)bpm;
    song.g[G_RTYPE] = 2;
    song.g[G_TAPE] = (int16_t)tape;
    rev_clear();
    hall_clear();
    fx.rtype = 2;
    for (i = 0; i < NPART; i++) {
        host_preset(&trk[i], p[i].e, preset_of(p[i].e, p[i].name));
        for (t = 0; t < 16u; t++) {                    /* as ui.c load_pat16 (not in the host build) */
            uint8_t n = pat_note(p[i].pat, t), fl = pat_flags(p[i].pat, t);
            put_step(&trk[i], t, n ? 1u : 0u, &n, (fl & 4u) ? ST_TIE : n ? ST_NOTE : ST_REST, n ? fl & 3u : 0u);
            trk[i].step[t].vel = n ? 96 : 0;
        }
        trk[i].p[P_SLEN] = 16;
        trk[i].p[P_SDIV] = 1;
        if (p[i].lvl) trk[i].p[P_LEVEL] = p[i].lvl;
        if (p[i].rev) trk[i].p[P_REV] = p[i].rev;
    }
    snprintf(path, sizeof path, "%s/%s.wav", dir, file);
    if (!(f = fopen(path, "wb"))) {
        perror(path);
        return;
    }
    wav_hdr(f, frames);
    transport_req = 1;
    for (t = 0; t < frames; t += CTL) {
        if (!stopped && t >= 4u * pass) {
            transport_req = 2;                         /* four passes, then the tail alone */
            stopped = 1;
        }
        mix_block(o, CTL);
        for (i = 0; i < CTL; i++)
            wav_put(f, o[2 * i], o[2 * i + 1]);
    }
    fclose(f);
    printf("fiend scenes: %s\n", path);
}

/* every scene of the firmware's own table (GB_SCENES), as the loader sets it up */
static void render_table(const char *dir)
{
    uint32_t s, i;
    for (s = 0; s < GB_NSCENE; s++) {
        part_t p[4];
        char file[48];
        if (!GB_SCENES[s].part[0][2])
            continue;                                  /* BLANK has no notes */
        for (i = 0; i < 4; i++) {
            const uint8_t *q = GB_SCENES[s].part[i];
            p[i].e = q[0];
            p[i].name = ENGINES[q[0]]->presets[q[1]].name;
            p[i].pat = q[2];
            p[i].lvl = q[3];
            p[i].rev = q[4];
        }
        snprintf(file, sizeof file, "table_%u_%s", (unsigned)s, GB_SCENES[s].name);
        render_scene(dir, file, GB_SCENES[s].bpm, GB_SCENES[s].tape, p);
    }
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : "build/fiend_scenes";
    /* engine ids: SAMPLE 4, VOICE 5, WHEEL 7, GRAIN 8, FM6 12; patterns: 14 DIRGE, 17 PEDAL, 19 LAMENT, 20 ARPEGGIO, 21 ANTIPHON */
    static const part_t SHRINE[4] = {{12, "PIPE ORGAN", 17}, {8, "FROZEN", 14}, {4, "BOWED PSALT", 19}, {4, "FLUTE", 21}};
    static const part_t GOTHIC[4] = {{12, "VIRGINAL", 20}, {12, "PIPE ORGAN", 14}, {7, "HARMONIUM", 19}, {5, "LOW DRONE", 17}};
    /* engine ids: ANALOG 0, PHASE 2, TRIO 6, PHYS 9, DRUM 10, FM6 12 (PHASE and TRIO are my guesses). Level / reverb: strings and
     * lead down, drums dry, so the kick and snare lead */
    static const part_t LAB_A[4] = {{12, "FM BASS", 100, 0, 0}, {12, "DARK STRINGS", 101, 70, 0}, {12, "PLUCK", 102, 80, 0}, {10, "DRUM KIT", 103, 0, 20}};
    static const part_t LAB_B[4] = {{6, "FAT BASS", 100, 0, 0}, {12, "DARK STRINGS", 101, 70, 0}, {9, "PLUCK", 102, 80, 0}, {10, "DRUM KIT", 103, 0, 20}};
    static const part_t LAB_C[4] = {{0, "SQR BASS", 100, 0, 0}, {12, "DARK STRINGS", 101, 70, 0}, {2, "WIRE", 102, 80, 0}, {10, "DRUM KIT", 103, 0, 20}};
    render_scene(dir, "shrine", 48, 50, SHRINE);
    render_scene(dir, "gothic", 66, 40, GOTHIC);
    /* VOICE 5, WHEEL 7, FM6 12, NOISE 11 */
    static const part_t RUINS[4] = {{5, "LOW DRONE", 17, 0, 0}, {7, "CRYPT ORGAN", 14, 70, 0},
                                    {12, "BELL", 21, 60, 0}, {11, "CAVE WIND", 17, 0, 0}};
    render_scene(dir, "ruins", 44, 55, RUINS);
    render_scene(dir, "labyrinth_a", 90, 30, LAB_A);
    render_scene(dir, "labyrinth_b", 90, 30, LAB_B);
    render_scene(dir, "labyrinth_c", 90, 30, LAB_C);
    render_table(dir);
    return 0;
}
