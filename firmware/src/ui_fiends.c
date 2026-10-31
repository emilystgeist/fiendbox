/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * FIENDBOX: the familiars. Adapted from JIANT's beings (ui_being.c and the line drawing of ui_organic.c, JIANT 0.5):
 * a sound page without a graph of its own shows a creature as four nested outlines of one body, drawn in lines (no
 * stored artwork), each as hot as the creature is there: the outer membrane cold (a dim cyan), the core hot (blood,
 * then bone), the nuclei hottest. The sound's level warms every line and swells the body; in silence it cools and
 * rests. Each knob of the page shapes it by what it is (its label), else by its place:
 *   CUT        the horns: a closed cell, opening into spikes as the filter lets the harmonics through
 *   RES        the membrane rings: a fine ripple round the outline
 *   DTN SPRD   two eyes, apart (the beating of two oscillators)
 *   NOIS RAND BRTH CRSH   the outline frays
 *   WAVE       the body's lobes
 *   (else)     KNOB 1 more horns, 2 longer, 3 lobes wobbling, 4 more eyes
 * Lines are antialiased, ~250 a frame, in the main loop (never the audio interrupt). */
typedef struct {
    uint8_t k0, kk;                                     /* spikes: k0 + KNOB 1 x kk */
    uint8_t ss;                                         /* their length at KNOB 2 full (16 = the radius x 1.6) */
    uint8_t lob;                                        /* lobes of the membrane */
    uint8_t ax;                                         /* stretched: x * ax / 16 */
    uint8_t tw;                                         /* the spikes twist */
    uint8_t sh;                                         /* their sharpness (a power of the cosine) */
} cr_genome_t;
static const cr_genome_t CR_GENOME[9] = {
    {6, 18, 16, 3, 22, 0, 6},                           /* 0 the sigil-sun: a sun of thin rays */
    {3, 5, 14, 2, 26, 3, 2},                            /* 1 the writhing shade: its arms swirl */
    {4, 6, 10, 4, 18, 0, 1},                            /* 2 the stone idol: blunt */
    {10, 30, 8, 5, 20, 1, 10},                          /* 3 the bristled one: a fringe of hairs */
    {5, 9, 12, 3, 19, 2, 4},                            /* 4 the swarm: seeds of rot */
    {2, 4, 16, 2, 30, 1, 3},                            /* 5 the horned worm: long, two horns */
    {8, 12, 12, 6, 17, 0, 8},                           /* 6 the many-eyed: a crown of points */
    {3, 7, 15, 3, 21, 4, 5},                            /* 7 the hydra: curled tentacles */
    {14, 8, 7, 6, 26, 1, 9},                            /* 8 the crawler: long, many short legs */
};
/* a creature per engine (engines.c order): ANALOG the sigil-sun, PHASE the shade, LOFI the idol, SAMPLE the worm,
 * VOICE the hydra (a mouth of tentacles), TRIO and WHEEL the bristled (a fringe of pipes), GRAIN the swarm, PHYS and
 * FM6 the many-eyed (its operators), DRUM the sigil-sun, NOISE the swarm, GURDY the crawler (its many strings);
 * the retired slots the worm */
static const uint8_t CR_EGEN[NENGINES] = {0, 5, 1, 2, 5, 7, 3, 3, 4, 6, 0, 4, 6, 5, 8};
typedef struct {
    int32_t spikes, len, wob, lob, nuc, sep, fray, ripple;   /* len wob sep fray ripple 0..256 */
} cr_feat_t;

static inline int32_t cr_cl(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }
static int32_t cr_sin(uint32_t ang) { return sine_i(ang << 16); }   /* Q15 */
static int cr_neutral(uint16_t c) { return (c >> 11) == (c & 31u) && ((c >> 5) & 63u) >> 1 == (c >> 11); }

/* heat 0..256: a dim cyan (cold) .. cyan .. blood .. bone (hot), in the palette's own colours */
static uint16_t cr_heat(int32_t v)
{
    v = cr_cl(v, 0, 256);
    if (v < 86)
        return ux_mix(ux_mix(T_BG, T_THEME, 38), T_THEME, v * 100 / 86);
    if (v < 172)
        return ux_mix(T_THEME, T_ACCENT, (v - 86) * 100 / 86);
    return ux_mix(T_ACCENT, T_TEXT, (v - 172) * 100 / 84);
}
/* a line's colour: heat v; GREY and MONO its grey (their lines blend grey on grey) */
static uint16_t cr_col(int32_t v)
{
    uint16_t c = cr_heat(v);
    if (settings.palette == UI_GREY_INDEX || settings.palette == UI_BW_INDEX) {
        uint32_t r = ux_luma(c) >> 3;
        return (uint16_t)(r << 11 | r << 6 | r);
    }
    return c;
}

/* c over the canvas pixel at (x, y) with coverage a (0..256) */
static inline void cr_plot(int32_t x, int32_t y, uint16_t c, int32_t a)
{
    uint16_t *p, o;
    uint32_t r, g, b;
    y += cv_oy;
    if (a <= 0 || (uint32_t)x >= cv_w || (uint32_t)y >= cv_h)
        return;
    p = &cv_px[(uint32_t)y * cv_w + (uint32_t)x];
    if (a >= 256) {
        *p = swap16(c);
        return;
    }
    o = swap16(*p);
    r = (o >> 11) + ((((int32_t)(c >> 11) - (int32_t)(o >> 11)) * a) >> 8);
    if (cr_neutral(o) && cr_neutral(c)) {                /* grey over grey stays grey */
        *p = swap16((uint16_t)(r << 11 | r << 6 | r));
        return;
    }
    g = ((o >> 5) & 63u) + ((((int32_t)((c >> 5) & 63u) - (int32_t)((o >> 5) & 63u)) * a) >> 8);
    b = (o & 31u) + ((((int32_t)(c & 31u) - (int32_t)(o & 31u)) * a) >> 8);
    *p = swap16((uint16_t)(r << 11 | g << 5 | b));
}
/* an antialiased line between Q4 points (two pixels across) */
static void cr_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)
{
    int32_t dx = x1 - x0, dy = y1 - y0, adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy, t, g, p, e, f;
    if (adx >= ady) {                                    /* along x: y in Q8 */
        if (x0 > x1) { t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; }
        g = adx ? ((y1 - y0) << 8) / (x1 - x0) : 0;
        e = x1 >> 4;
        for (p = x0 >> 4; p <= e && p - (x0 >> 4) < 480; p++) {
            int32_t y = (y0 << 4) + g * (p - (x0 >> 4)) - 128;
            f = y & 255;
            cr_plot(p, y >> 8, c, 256 - f);
            cr_plot(p, (y >> 8) + 1, c, f);
        }
    } else {
        if (y0 > y1) { t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; }
        g = ((x1 - x0) << 8) / (y1 - y0);
        e = y1 >> 4;
        for (p = y0 >> 4; p <= e && p - (y0 >> 4) < 480; p++) {
            int32_t x = (x0 << 4) + g * (p - (y0 >> 4)) - 128;
            f = x & 255;
            cr_plot(x >> 8, p, c, 256 - f);
            cr_plot((x >> 8) + 1, p, c, f);
        }
    }
}

/* the output's level now, 0..256, slewed (fast up, slow down) */
static int32_t cr_energy(void)
{
    static int32_t e;
    static uint32_t fr = 0xFFFFFFFFu;
    uint32_t w = scope_w, i;
    int32_t pk = 0, v;
    if (fr == ui.frame)                                 /* (once a frame: a redraw in the same frame draws the same) */
        return e;
    fr = ui.frame;
    for (i = 1; i <= 256u; i += 2u) {
        v = scope_buf[(w - i) & (SCOPE_N - 1u)];
        if (v < 0) v = -v;
        if (v > pk) pk = v;
    }
    v = pk * 256 / 12000;
    if (v > 256) v = 256;
    e += v > e ? (v - e) * 3 / 4 : (v - e) / 6;
    return e;
}
/* one outline: the body at (cx, cy), radius r (Q4), x stretched by ax / 16; outer: with the spikes, the ripple, the
 * fray. Colour c. 64 vertices outside, 32 inside */
static void cr_outline(const cr_genome_t *g, const cr_feat_t *f, int32_t cx, int32_t cy, int32_t r, int outer, uint32_t t,
                       uint16_t c)
{
    int32_t i, px = 0, py = 0, nv = outer ? 64 : 32;
    for (i = 0; i <= nv; i++) {
        uint32_t th = (uint32_t)(i % nv) * 65536u / (uint32_t)nv;
        int32_t rr = r + ((((r * f->wob) >> 8) * ((9175 * cr_sin((uint32_t)f->lob * th + t) +
                                                    3932 * cr_sin((uint32_t)(f->lob + 2) * th - 2u * t)) >> 15)) >> 15);
        int32_t x, y;
        if (outer) {
            int32_t c2 = cr_sin(((uint32_t)f->spikes * th >> 1) + (uint32_t)g->tw * 3000u + 0x4000u), p = c2 > 0 ? c2 : 0, n;
            for (n = 1; n < g->sh; n++)
                p = (p * c2) >> 15;
            if (p < 0) p = 0;
            rr += (((r * f->len) >> 8) * p >> 15) * 26 >> 4;
            rr += (((r * f->ripple) >> 8) * cr_sin(th * 18u + 3u * t) >> 15) / 14;
            if (f->fray)
                rr += ((int32_t)((((uint32_t)i * 2654435761u) + t * 40503u) >> 24) - 128) * ((r * f->fray) >> 8) / 1800;
        }
        x = cx + ((rr * cr_sin(th + 0x4000u) >> 15) * g->ax >> 4);
        y = cy + (rr * cr_sin(th) >> 15);
        if (i)
            cr_line(px, py, x, y, c);
        px = x;
        py = y;
    }
}
/* a small ring (an eye), Q4 */
static void cr_ring(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t i, px = cx + r, py = cy;
    for (i = 1; i <= 12; i++) {
        uint32_t th = (uint32_t)i * 65536u / 12u;
        int32_t x = cx + (r * cr_sin(th + 0x4000u) >> 15), y = cy + (r * cr_sin(th) >> 15);
        cr_line(px, py, x, y, c);
        px = x;
        py = y;
    }
}
/* the creature: its isotherms (outer cold .. core hot, all warmer with en 0..256), then its eyes. cx cy r in Q4 */
static void cr_lines(const cr_genome_t *g, const cr_feat_t *f, int32_t cx, int32_t cy, int32_t r, int32_t en, uint32_t t)
{
    static const int16_t SC[4] = {256, 180, 112, 52}, HT[4] = {28, 90, 150, 206};
    int32_t i, n;
    r += r * en * 30 / 65536;                            /* (it swells with the sound) */
    for (i = 0; i < 4; i++)
        cr_outline(g, f, cx, cy, r * SC[i] >> 8, !i, t, cr_col(HT[i] + en * 40 / 256));
    for (n = 0; n < f->nuc; n++) {                       /* the eyes: orbiting, DTN apart */
        uint32_t a = t + (uint32_t)n * 65536u / (uint32_t)f->nuc;
        int32_t d = f->nuc > 1 ? r * (24 + f->sep * 40 / 256) / 100 : 0;
        int32_t x = cx + ((d * cr_sin(a + 0x4000u) >> 15) * g->ax >> 4), y = cy + (d * cr_sin(a) >> 15) * 7 / 10;
        cr_ring(x, y, r * 15 / 100, cr_col(214 + en * 42 / 256));
        cr_ring(x, y, r * 6 / 100, cr_col(240));
    }
}
/* a creature's features from four of its track's parameters (ids), each by what it is (its label), else by its place */
static void cr_feat(const track_t *t, const cr_genome_t *g, const uint8_t *ids, cr_feat_t *f)
{
    int32_t k[4];
    const char *lab[4];
    uint32_t c;
    for (c = 0; c < 4u; c++) {
        const param_desc_t *d = ids[c] < P_COUNT ? track_desc(t, ids[c]) : 0;
        int32_t r = d && d->label && d->label[0] != '-' ? RATIO(d, enum_rank(d, t->p[ids[c]])) : -1;
        lab[c] = d && d->label ? d->label : "";
        k[c] = r < 0 ? 128 : r * 256 / 1000;
    }
    f->spikes = g->k0 + k[0] * g->kk / 256;
    f->len = 48 + k[1] * 208 / 256;
    f->wob = 40 + k[2] * 216 / 256;
    f->lob = g->lob;
    f->nuc = 1 + k[3] * 3 / 257;
    f->sep = f->fray = f->ripple = 0;
    for (c = 0; c < 4u; c++) {                           /* a knob by what it is (the labels as the engines print them) */
        const char *l = lab[c];
        if (str_eq(l, "CUT")) { f->spikes = 10 + k[c] * 22 / 256; f->len = k[c] * k[c] / 256; }
        else if (str_eq(l, "RES") || str_eq(l, "RESO")) f->ripple = k[c];
        else if (str_eq(l, "DTN") || str_eq(l, "DETUNE") || str_eq(l, "SPRD") || str_eq(l, "DTUNE")) { f->sep = k[c]; if (f->nuc < 2) f->nuc = 2; }
        else if (str_eq(l, "NOIS") || str_eq(l, "NOISE") || str_eq(l, "RAND") || str_eq(l, "BRTH") || str_eq(l, "BREATH") ||
                 str_eq(l, "CRSH") || str_eq(l, "CRUSH")) f->fray = k[c];
        else if (str_eq(l, "WAVE")) f->lob = 2 + k[c] * 5 / 257;
    }
}
/* the sound exaggerated on the creature: at energy en (0..256) it bulges, its spikes shoot out, its membrane shivers
 * and frays; a hit is seen at once */
static void cr_excite(cr_feat_t *f, int32_t en, int32_t k)
{
    f->wob = cr_cl(f->wob + en * k / 4, 0, 256);
    f->len = cr_cl(f->len + en * k / 3, 0, 420);
    f->ripple = cr_cl(f->ripple + en * k / 3, 0, 256);
    f->fray = cr_cl(f->fray + en * k / 6, 0, 256);
}
/* the page's creature at (cx, cy), radius r (Q4): its engine's species, its four knobs by what they are */
static void creature_at(const track_t *t, int32_t cx, int32_t cy, int32_t r)
{
    static uint32_t tm, f0;                              /* its time runs with the sound: in silence it rests */
    const cr_genome_t *g = &CR_GENOME[CR_EGEN[t->eng_req % NENGINES]];
    const page_t *pg = cur_page();
    int32_t en = cr_energy();
    cr_feat_t f;
    cr_feat(t, g, pg->id, &f);
    f.lob += (int32_t)(pg->id[0] != P_E0);               /* (the engine's second page: a lobe more, the same species) */
    cr_excite(&f, en, 2);
    if (en > 2) tm += (ui.frame - f0) * (uint32_t)(160 + en * 3);
    f0 = ui.frame;
    cr_lines(g, &f, cx, cy, r * (256 + en / 3) / 256, en, tm);
}
static void creature(const track_t *t) { creature_at(t, 120 * 16, 61 * 16, 36 * 16); }
