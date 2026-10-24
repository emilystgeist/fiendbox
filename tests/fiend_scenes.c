/* SPDX-License-Identifier: GPL-3.0-only */
/* FIENDBOX audition: a candidate scene (4 parts, HALL, TAPE) rendered to a WAV as the device plays it after
 * power-on + PLAY: four passes, then the tail.   build/host/fiend_scenes OUTDIR
 * Not a test: an ear check while the scenes are written. */
#define main hostsim_main
#include "hostsim.c"
#undef main

typedef struct { uint32_t e; const char *name; uint32_t pat; } part_t;   /* engine id, sound, PATTERNS[pat - 1] */

static uint32_t preset_of(uint32_t e, const char *name)
{
    uint32_t i;
    for (i = 0; i < ENGINES[e]->npresets; i++)
        if (!strcmp(ENGINES[e]->presets[i].name, name))
            return i;
    fprintf(stderr, "fiend scenes: engine %u has no sound %s\n", (unsigned)e, name);
    exit(1);
}

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
            uint8_t n = PATTERNS[p[i].pat - 1u].note[t], fl = PATTERNS[p[i].pat - 1u].flags[t];
            put_step(&trk[i], t, n ? 1u : 0u, &n, (fl & 4u) ? ST_TIE : n ? ST_NOTE : ST_REST, n ? fl & 3u : 0u);
            trk[i].step[t].vel = n ? 96 : 0;
        }
        trk[i].p[P_SLEN] = 16;
        trk[i].p[P_SDIV] = 1;
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

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : "build/fiend_scenes";
    /* engine ids: SAMPLE 4, VOICE 5, WHEEL 7, GRAIN 8, FM6 12; patterns: 14 DIRGE, 17 PEDAL, 19 LAMENT, 20 ARPEGGIO, 21 ANTIPHON */
    static const part_t SHRINE[4] = {{12, "PIPE ORGAN", 17}, {8, "FROZEN", 14}, {4, "BOWED PSALT", 19}, {4, "FLUTE", 21}};
    static const part_t GOTHIC[4] = {{12, "VIRGINAL", 20}, {12, "PIPE ORGAN", 14}, {7, "HARMONIUM", 19}, {5, "LOW DRONE", 17}};
    render_scene(dir, "shrine", 48, 50, SHRINE);
    render_scene(dir, "gothic", 66, 40, GOTHIC);
    return 0;
}
