/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Copyright (C) 2026 bemily (FIENDBOX)
 * FIENDBOX: BYTE and FLOAT, two more waves of the LOFI engine (included by eng_lofi.c), adapted from JIANT's eng_lofi.c
 * (JIANT 0.4 .. 0.9). WAVE = BYTE: bytebeat, an 8-bit formula of the time t played as the wave (after viznut's 2011
 * one-liners: the music is in the formula). WAVE = FLOAT: floatbeat, its sibling in floats: formulas made of sines.
 * The old waves are untouched (they still play bit for bit as before): only a track whose WAVE is BYTE or FLOAT comes
 * through here, and for it the slots of LOFI's eight controls read as follows:
 *   BYTE   CHIP the bits   ALGO (DUTY's slot, 32 formulas)  VAR (CRSH's: a variable of the formulas, and a mutation of t)
 *          MOTION (SWP's: the slow bits' speed, 64 as written)  GRIT (VIB's: a rate crusher)
 *          BEND (ARP's: t folded on itself)  LOOP (TONE's: t held in a window: a tuned note)
 *   FLOAT  CHIP the resolution (4BIT, 4B/2, FULL, 1BIT)  ALGO  VAR  MOTION  FEEDBK (the output back into t)  BEND  LOOP
 * t runs with the note played (a key plays the formula in that key) and restarts at each note-on. JIANT's FILTER page
 * does not exist here: BYTE is rounded by two one-poles, FLOAT by one (-3 dB at ~7.5 kHz), as JIANT's own smoothing,
 * and both play 6 dB lower than the other waves (they are full-scale and dense). */
static const char *const N_BYTE[] = {"B01", "B02", "B03", "B04", "B05", "B06", "B07", "B08", "B09", "B10", "B11",
                                     "B12", "B13", "B14", "B15", "B16", "B17", "B18", "B19", "B20", "B21", "B22",
                                     "B23", "B24", "B25", "B26", "B27", "B28", "B29", "B30", "B31", "B32", 0};
static const char *const N_FLOAT[] = {"F01", "F02", "F03", "F04", "F05", "F06", "F07", "F08", "F09", "F10", "F11",
                                      "F12", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20", "F21", "F22",
                                      "F23", "F24", "F25", "F26", "F27", "F28", "F29", "F30", "F31", "F32", 0};
static const char *const N_FCHIP[] = {"4BIT", "4B/2", "FULL", "1BIT"};
static const param_desc_t LOFI_ALGO = {"ALGO", F_INT, 0, 127, 64, N_BYTE, 0};     /* = edit[2] (BYTE) */
static const param_desc_t LOFI_FALGO = {"ALGO", F_INT, 0, 127, 64, N_FLOAT, 0};   /* = edit[2] (FLOAT) */
static const param_desc_t LOFI_VAR = {"VAR", F_PCT, 0, 127, 0, 0, 0};             /* = edit[3] (BYTE, FLOAT) */
static const param_desc_t LOFI_FCHIP = {"CHIP", F_ENUM, 0, 3, 2, N_FCHIP, 0};     /* = edit[0] (FLOAT) */
static const param_desc_t LOFI_MOTN = {"MOTION", F_INT, 0, 127, 64, 0, 0};        /* = edit[4] (BYTE, FLOAT) */
static const param_desc_t LOFI_GRIT = {"GRIT", F_PCT, 0, 127, 0, 0, 0};           /* = edit[5] (BYTE) */
static const param_desc_t LOFI_FDCY = {"FEEDBK", F_PCT, 0, 127, 0, 0, 0};         /* = edit[5] (FLOAT) */
static const param_desc_t LOFI_BENDD = {"BEND", F_PCT, 0, 127, 0, 0, 0};          /* = edit[6] (BYTE, FLOAT) */
static const param_desc_t LOFI_LOOP = {"LOOP", F_PCT, 0, 127, 0, 0, 0};           /* = edit[7] (BYTE, FLOAT) */
/* the descriptor of control k for a BYTE / FLOAT track (0: the engine's own) */
static const param_desc_t *beat_desc(const track_t *t, uint32_t k)
{
    int fl = t->p[P_E1] == RW_FLOAT;
    switch (k) {
    case 0: return fl ? &LOFI_FCHIP : 0;
    case 2: return fl ? &LOFI_FALGO : &LOFI_ALGO;
    case 3: return &LOFI_VAR;
    case 4: return &LOFI_MOTN;
    case 5: return fl ? &LOFI_FDCY : &LOFI_GRIT;
    case 6: return &LOFI_BENDD;
    case 7: return &LOFI_LOOP;
    default: return 0;
    }
}
/* each formula's start, in 512 steps of t: where it is already moving (some sit silent for up to ~2 s from t = 0); a
 * note starts there, LOOP's window too */
static const uint8_t BYTE_T0[32] = {0, 0, 0, 0, 0, 0, 0, 31, 0, 0, 0, 0, 0, 4, 2, 0, 0, 0, 16, 0, 7, 16, 0, 1, 8, 15, 8, 0, 15, 23, 8, 0};
static __attribute__((noinline)) uint32_t lofi_byte(uint32_t f, uint32_t t, uint32_t a)
{
    switch (f & 31u) {
    case 0: return (~t >> 2) * ((127u & t * (7u & t >> 10)) < (245u & t * (2u + (5u & t >> 14))));   /* (Tejeez) */
    case 1: return (t * ((3u + (1u ^ (t >> 10 & 5u))) * (5u + (3u & t >> 14)))) >> (t >> 8 & 3u);   /* (Tejeez) */
    case 2: return ((-t & 4095u) * (255u & t * (t & t >> 13)) >> 12) + (127u & t * (234u & t >> 8 & t >> 3) >> (3u & t >> 14));   /* (xpansive) */
    case 3: return t * (((t >> 9) ^ ((t >> 9) - 1u) ^ 1u) % 13u);
    case 4: return ((t >> 13 | t % 24u) & (t >> 7 | t % 19u)) << 3;
    case 5: return t * (t ^ (t + (t >> 15 | 1u)) ^ (((t - 1280u) ^ t) >> 10));
    case 6: return (t * (0xCA98u >> (t >> 9 & 14u) & 15u)) | t >> 8;
    case 7: return (t * (t >> 8 | t >> 9) & 46u & t >> 8) ^ ((t & t >> 13) | t >> 6);
    case 8: return (t >> 4) * (13u & 0x8898A989u >> (t >> 11 & 30u));
    case 9: return t * (t >> ((8u * ((t >> 15) | (t >> 8))) & 31u) & (20u | (((t >> 19) * 5u) >> (t & 31u)) | t >> 3));
    case 10: return (t & t % 255u) - (t * 3u & t >> 13 & t >> 6);
    case 11: return (t * (t >> 9 | t >> 13) & (t >> 6 | t >> 11) & 127u) + ((t >> 4) * (3u + (t >> 12 & 3u)) & 64u);
    case 12: return t + (t & (t ^ t >> 6)) - t * ((t >> 9) & (t % 16u ? 2u : 6u) & t >> 9);
    case 13: return t * (t >> 11 & t >> 8 & 123u & t >> 3);
    case 14: return (t * 9u & t >> 4) | (t * 5u & t >> 7) | (t * 3u & t >> 10);
    case 15: return (t * ((t & 4096u ? (t % 65536u < 59392u ? 7u : t & 7u) : 16u) + (1u & t >> 14)) >> (3u & -t >> (t & 2048u ? 2 : 10))) | t >> (t & 16384u ? (t & 4096u ? 10 : 3) : 2);   /* (Tejeez) */
    case 16: return (t * ((3u + (1u ^ (t >> 10 & 5u))) * (a + (3u & t >> 14)))) >> (t >> 8 & 3u);   /* (a: VAR) */
    case 17: return t * ((t >> (a & 15u) | t >> 8) & 63u & t >> 4);
    case 18: return (t * a & t >> 7) | (t * 3u & t >> 10);
    case 19: return (t >> a | t) * (t >> 9 & 7u);
    case 20: return t * (t >> 8 & a) ^ t >> 4;
    case 21: return (t * a & t >> 8) * (t >> 12 & 3u);
    case 22: return t * (0x9AD5u >> (t >> (8u + (a & 3u)) & 14u) & 15u);
    case 23: return (t ^ t >> a) * (t >> 10 & 5u);
    case 24: return t >> 2 & (t * a >> 6 | t >> 9);
    case 25: return (t | t >> a | t >> 7) * (t >> 13 & 7u);
    case 26: return t * ((t >> 11 & 3u) + a) & t >> 6;
    case 27: return (t * (a | 1u) % (257u - (t >> 10 & 127u))) & 255u;
    case 28: return t * (t >> 10 & 7u & a) | t >> 5;
    case 29: return (t >> 5 & t >> 7) * a + (t >> 9 & 32u);
    case 30: return t * (t >> 12 & a ? 3u : 2u) & t >> 6;
    default: return (t * a >> 4 | t >> 3) ^ (t >> 7 & t >> 11);
    }
}
/* MOTION 0..127 -> the time's speed, Q8: x1/16 .. x1 (64) .. x15.5, exponential */
static uint32_t lofi_motion(int32_t m)
{
    uint32_t e = (uint32_t)clamp(m, 0, 127) + 64u;
    return ((16u + (e & 15u)) << (e >> 4)) >> 4;
}
/* VAR as a mutation of the time (BYTE's t, FLOAT's step count): v >> 4 picks the way, v & 15 how much. 0..15 the time
 * as written; then stairs that transpose as it goes, xor and or with itself shifted, a bit dropped (holes in the
 * rhythm), the low bits reversed (the phrase backwards), multiplied (faster, higher), scrambled */
static uint32_t lofi_mut(uint32_t t, uint32_t v)
{
    uint32_t k = v & 15u, m;
    switch ((v >> 4) & 7u) {
    case 0: return t;
    case 1: return t + (t >> (9u - k / 2u)) * (k + 1u);
    case 2: return t ^ (t >> (2u + k / 2u));
    case 3: return t | (t >> (1u + k / 2u));
    case 4: return t & ~(1u << (2u + k / 2u));
    case 5: m = (1u << (3u + k / 2u)) - 1u; return (t & ~m) | (~t & m);
    case 6: return t * (3u + k);
    default: return t ^ ((t >> 3) * ((k + 1u) * 0x9E3779B1u >> 24));
    }
}
/* a cubic soft clip: |v| up to a full scale as it is, beyond it saturated (the "drive" of a formula) */
static int32_t lofi_sclip(int32_t v)
{
    int32_t x = v > 32767 ? 32767 : v < -32767 ? -32767 : v;
    return (3 * x - (int32_t)(((int64_t)x * x >> 15) * x >> 15)) >> 1;
}
#define S(x) sine_i((uint32_t)(x) << 19)                 /* sin of x (t in Q8): a turn every 32 t */
#define PM(y, k) ((uint32_t)(((int32_t)(y) * (int32_t)(k)) >> 6))   /* a sine (Q15) times k: k / 16 of a turn */
/* the floatbeat formula f at t (its integer) and x (t in Q8); aux: a formula's own state (a kick's phase) */
static __attribute__((noinline)) int32_t lofi_float(uint32_t f, uint32_t t, uint32_t x, int32_t *aux)
{
    static const uint8_t R[8] = {16, 18, 20, 21, 24, 27, 30, 32};   /* just ratios, Q4: 1 9/8 5/4 4/3 3/2 5/3 15/8 2 */
    uint32_t st = t >> 11, fr = t & 2047u, h;
    int32_t E = 32767 - (int32_t)(fr << 4), a, b;          /* (a step's envelope: 1 .. 0) */
    switch (f & 31u) {
    case 0: return S(x * (((t >> 11) ^ (t >> 13)) & 7u) + x) * E >> 15;                       /* sin(t (t>>11 ^ t>>13 & 7)) */
    case 1: return S(x + PM(S(x * 3u), (int32_t)((t >> 10) & 15u) * 3));                      /* FM, its index walking */
    case 2: return S((x >> 4) * R[(0x36364689u >> ((st & 7u) * 4u)) & 7u]) * E >> 15;         /* a melody, a nibble table */
    case 3: return S(x * (1u + ((t >> 12) & 3u))) * S(x * (((t >> 9) & 7u) + 1u)) >> 15;      /* a ring, rhythmic */
    case 4: {                                                                                  /* a kick, a bass */
        uint32_t k = fr < 1024u ? (1024u - fr) * (1024u - fr) >> 6 : 0u;
        *aux += (int32_t)k + 40;
        a = (st & 1u) ? 0 : sine_i((uint32_t)*aux << 13) * (int32_t)(1024u - (fr < 1024u ? fr : 1024u)) >> 10;
        return (a + (S(x >> 1) >> 1)) >> 1;
    }
    case 5: return S(x * ((((t >> 9) & (t >> 11)) & 7u) | 1u)) * E >> 15;                     /* Sierpinski partials */
    case 6: return lofi_sclip(S(x * (1u + (st & 1u))) * (1 + (int32_t)((t >> 11) & 7u) * 3) >> 1);   /* drive, rhythmic */
    case 7: return S((x >> 4) * R[(st * 3u + (st >> 2)) & 7u] * (1u + (st & 1u)));            /* fifths and octaves */
    case 8: {                                                                                  /* two arpeggios */
        static const uint8_t H[8] = {2, 3, 4, 5, 6, 8, 5, 3};
        return (S(x * H[(t >> 9) & 7u] >> 1) + S(x * H[(t >> 11) & 7u] >> 2)) >> 1;
    }
    case 9: h = (t * 2654435761u) >> 17;                                                       /* a snare, a tone */
        a = (st & 1u) ? (int32_t)h - 16384 : 0;
        return (mulq15(a, E) + S(x)) >> 1;
    case 10: return S(x + PM(S(x * (((t >> 12) & 7u) + 1u)), E >> 10));                       /* a pluck of FM */
    case 11: return S(x + PM(S(t << 2), 6));                                                  /* a vibrato, deep (4 Hz) */
    case 12: return S((uint32_t)(S(x) * (2 + (int32_t)((t >> 11) & 7u))) >> 2);               /* folded, rhythmic */
    case 13: a = S(x * (1u + (st & 1u))); b = S((x >> 4) * (24u + (st & 6u) * 4u));           /* rectified, leaping */
             return (a < 0 ? -a : a) - (b < 0 ? -b : b);
    case 14: return S(x & ~((1u << (4u + (st & 7u))) - 1u));                                  /* a staircase sine */
    case 15: return S(x * (((t >> 8) % 7u) + 1u)) * E >> 15;                                  /* partials mod 7 */
    case 16: {                                                                                 /* three against four */
        int32_t e3 = 32767 - (int32_t)((t % 3072u) * 10u), e4 = E;
        return (mulq15(S(x * (1u + (st & 1u))), e4) + mulq15(S((x >> 4) * (24u + (st & 2u) * 8u)), e3 > 0 ? e3 : 0)) >> 1;
    }
    case 17: return S((x >> 6) * (64u + ((t >> 6) & 255u))) * E >> 15;                        /* a riser each bar */
    case 18: return mulq15(S(x * (1u + st % 3u)) * ((t >> 10) & 1u ? 1 : 0) + S(x + (t << 2)), 16384);   /* beating, gated */
    case 19: {                                                                                 /* odd partials, built up */
        uint32_t k, n = 1u + ((t >> 11) & 3u);
        for (a = 0, k = 0; k < n; k++)
            a += S(x * (2u * k + 1u)) / (int32_t)(2u * k + 1u);
        return a * 3 >> 2;
    }
    case 20: return S(x * (2u + ((st * 5u) & 7u)) >> 1);                                      /* leaps */
    case 21: {                                                                                 /* a drum loop */
        uint32_t k = fr < 768u ? (768u - fr) * (768u - fr) >> 6 : 0u;
        *aux += (int32_t)k + 30;
        a = (st & 3u) == 0u ? sine_i((uint32_t)*aux << 13) * (int32_t)(768u - (fr < 768u ? fr : 768u)) / 768 : 0;
        b = (t >> (8u + (st & 3u))) & 1u ? (int32_t)(((t * 2654435761u) >> 17) - 16384u) * (int32_t)(1024u - (t & 1023u)) >> 12 : 0;
        return (a + b + (S(x >> 1) >> 2)) >> 1;
    }
    case 22: return S(x * (1u + (((t >> 9) & (t >> 7)) & 7u)) + (((t >> 9) & (t >> 7) & 255u) << 9));   /* bytebeat in it */
    case 23: return S(x * 2u + PM(S(x * 5u), (int32_t)((t >> 11) & 3u) * 8));                 /* FM 5:2, stepped */
    case 24: a = S(x * (1u + (st & 3u))); return (int32_t)((int64_t)a * (a < 0 ? -a : a) >> 15) * E >> 15;   /* squared */
    case 25: return S(x & ~(1u << (8u + (st & 7u)))) * E >> 15;                              /* a bit of t dropped: jumps */
    case 26: return S((x >> 4) * (34u - ((t >> 11) & 15u)) >> 1) * E >> 15;                   /* a descending line */
    case 27: return (S(x) + mulq15(S(x * (((t >> 13) & 3u) + 2u)), E)) >> 1;                   /* a partial, struck */
    case 28: return S(x + PM(S(x * 2u), S(t) >> 10));                                         /* FM, its index a 1 Hz sine */
    case 29: return S((x >> 4) * (16u + ((t >> 7) & 63u)));                                   /* a glide up partials */
    case 30: h = ((st + 1u) * 2654435761u) >> 29; return S((x >> 4) * R[h]) * E >> 15;        /* a random line */
    default: a = ((st * 5u) & 7u) < 5u ? E : 0;                                               /* euclid hats, a chord */
        b = (int32_t)(((t * 2654435761u) >> 17) - 16384u) * (int32_t)(512u - (t & 511u)) >> 11;
        h = (x >> 4) * R[(st >> 1) & 7u] >> 4;                                                 /* (the chord's root walks) */
        return (S(h * 16u) + S(h * 20u) + S(h * 24u)) / 4 + (mulq15(b, a) >> 1);
    }
}
#undef S
#undef PM

/* BYTE's samples: t in v->s[2], its fraction (Q24) in v->ph[0], MOTION's slow t (Q8) in v->ph[1], the smoothing's two
 * poles in v->s[3] and v->s[4], the DC's estimate in v->s[5] (BEAT_NODC from a note-on: the first sample sets it) and its warm-up count in v->s[6]. A
 * bytebeat is unsigned and lopsided (a mean of -12000 is common): the envelope would ramp that offset into a thump at
 * every note, so it is tracked (the first 4096 samples with a growing time constant, then ~0.1 s) and taken out. inc the note's, hold the samples' length, bits the resolution */
#define BEAT_NODC (-2147483647 - 1)
static __attribute__((noinline)) void beat_byte(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m,
                                                uint32_t inc, int32_t hold, int32_t bits)
{
    const int16_t *p = t->p;
    uint32_t vr = (uint32_t)clamp(p[P_E3] + ((m->shape - (64 << 8)) >> 8), 0, 127);   /* (LFO / ENV -> SHAPE move VAR) */
    uint32_t f = (uint32_t)p[P_E2] >> 2, a = 1u + (vr >> 3), tt = (uint32_t)v->s[2], tf = v->ph[0], i;
    uint32_t ti = (inc >> 8) * 31u;                     /* t a sample, Q24: C4 (inc ~2.55e7) ~0.18 = 8 kHz */
    uint32_t bs = p[P_E6] ? 13u - (uint32_t)p[P_E6] / 11u : 0u;   /* BEND: the fold's shift, 13 .. 2 (0: none) */
    int32_t held = v->s[0], cnt = v->s[1], l1 = v->s[3], l2 = v->s[4], dc = v->s[5], nw = v->s[6];   /* (locals: out[] may alias v->s[]) */
    uint32_t lm = p[P_E7] ? (1u << (17u - (uint32_t)p[P_E7] * 12u / 127u)) - 1u : 0xFFFFFFFFu;   /* LOOP: t's window */
    uint32_t t0 = (uint32_t)BYTE_T0[f & 31u] << 9;      /* (the formula's start) */
    uint32_t mul = lofi_motion(p[P_E4]), ts = v->ph[1], var = vr;
    hold *= 1 + (p[P_E5] >> 3);                         /* GRIT: each sample held up to 16 x longer */
    for (i = 0; i < n; i++) {
        if (--cnt <= 0) {
            uint32_t te = mul == 256u ? tt : (tt & 255u) | ((ts >> 8) & ~255u);   /* (MOTION: the slow bits at their own
                                                         * speed; 64: t as written) */
            uint32_t tl = lofi_mut((te & lm) + t0, var), tw = bs ? tl ^ (tl >> bs) : tl;   /* (VAR: mutated) */
            int32_t s = (int32_t)(lofi_byte(f, tw, a) & 255u) * 256 - 32640;
            cnt = hold;
            if (bits < 8)
                s = ((s + (1 << (15 - bits))) >> (16 - bits)) * (1 << (16 - bits));
            held = s;
            if (dc == BEAT_NODC)
                dc = s;
        }
        tf += ti;
        tt += tf >> 24;
        tf &= 0xFFFFFFu;
        ts += ((ti >> 8) * mul) >> 16;
        dc += (held - dc) >> (4 + (nw >> 9));           /* (a time constant of 16 samples at first, 4096 from the 4096th) */
        if (nw < 4096)
            nw++;
        l1 += ((held >> 1) - l1) >> 1;                  /* the 8-bit steps rounded: two one-poles (BYTE is knife-sharp) */
        l2 += (l1 - l2) >> 1;
        out[i] += voice_amp(l2 * 2 - dc, m, i);
    }
    v->s[0] = held;
    v->s[1] = cnt;
    v->s[2] = (int32_t)tt;
    v->s[3] = l1;
    v->s[4] = l2;
    v->s[5] = dc;
    v->s[6] = nw;
    v->ph[0] = tf;
    v->ph[1] = ts;
}
/* FLOAT's samples: t in v->s[2], its fraction (Q24) in v->ph[0], MOTION's slow t (Q8) in v->ph[1], a formula's own
 * state in v->s[3], the last output in v->s[0], the smoothing's pole in v->s[4] */
static __attribute__((noinline)) void beat_float(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m,
                                                uint32_t inc)
{
    const int16_t *p = t->p;
    uint32_t f = (uint32_t)p[P_E2] >> 2, tt = (uint32_t)v->s[2], tf = v->ph[0], ts = v->ph[1], i;
    uint32_t ti = inc >> 3;                             /* (t a sample, Q24: 32 a cycle of the note) */
    uint32_t mul = lofi_motion(p[P_E4]), var = (uint32_t)clamp(p[P_E3] + ((m->shape - (64 << 8)) >> 8), 0, 127);
    uint32_t bs = p[P_E6] ? 13u - (uint32_t)p[P_E6] / 11u : 0u;   /* BEND: t ^ t >> 13 .. 2 */
    uint32_t lm = p[P_E7] ? (1u << (17u - (uint32_t)p[P_E7] * 12u / 127u)) - 1u : 0xFFFFFFFFu;   /* LOOP: t's window */
    int32_t aux = v->s[3], fb = v->s[0], fbk = p[P_E5], sm = v->s[4];
    uint32_t chip = (uint32_t)p[P_E0], qs = chip == CHIP_8BIT ? 0u : chip == CHIP_1BIT ? 15u : 12u;   /* (CHIP: the bits) */
    for (i = 0; i < n; i++) {
        uint32_t te = mul == 256u ? tt : (tt & 255u) | ((ts >> 8) & ~255u), tl, tw;
        int32_t s;
        tl = lofi_mut(te & lm, var);
        tw = bs ? tl ^ (tl >> bs) : tl;
        if (fbk) {                                      /* FEEDBK: the last output moves t, up to +-16 t (half a turn of
                                                         * the sines, Q8): the formula modulates itself, FM-like */
            int32_t d = (fb * fbk) >> 10;
            s = lofi_float(f, tw + (uint32_t)(d >> 8), (tw << 8 | tf >> 16) + (uint32_t)d, &aux);
        } else {
            s = lofi_float(f, tw, tw << 8 | tf >> 16, &aux);
        }
        if (qs)
            s = ((s + (1 << (qs - 1u))) >> qs) * (1 << qs);
        if (chip == CHIP_4B2 && (tt & 1u))              /* (4B/2: held over two t) */
            s = fb;
        fb = s;
        tf += ti;
        tt += tf >> 24;
        tf &= 0xFFFFFFu;
        ts += ((ti >> 8) * mul) >> 16;
        sm += (((s >> 1) - sm) * 5) >> 3;               /* one pole: -3 dB at ~7.5 kHz */
        out[i] += voice_amp(sm * 2, m, i);
    }
    v->s[0] = fb;
    v->s[2] = (int32_t)tt;
    v->s[3] = aux;
    v->s[4] = sm;
    v->ph[0] = tf;
    v->ph[1] = ts;
}
/* lofi_render's branch for the two waves: the note's pitch (TEMP's rest too), the level 6 dB down, then the wave's own */
static void beat_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    uint32_t chip = (uint32_t)p[P_E0], inc;
    int32_t bits = chip == CHIP_8BIT ? 8 : chip == CHIP_1BIT ? 1 : 4;
    vmod_t mg = *m;
    mg.amp0 = mulq15(m->amp0, 16384);
    mg.amp1 = mulq15(m->amp1, 16384);
    inc = pitch_inc(clamp(m->pitch16, 0, 2047));
    inc += (uint32_t)((int32_t)(inc >> 12) * v->tfine);
    if (p[P_E1] == RW_FLOAT)
        beat_float(t, v, out, n, &mg, inc);
    else
        beat_byte(t, v, out, n, &mg, inc, 1 + (chip == CHIP_4B2 ? 1 : 0), bits);
}
