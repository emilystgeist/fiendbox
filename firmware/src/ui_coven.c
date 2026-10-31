/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * FIENDBOX: the coven. HOME's stage, adapted from JIANT's ecosystem (ui_lzhome.c, JIANT 0.8 / 0.9.7): the four tracks'
 * familiars (ui_fiends.c) swimming free over the stage, each by itself (wandering, bouncing off the walls, a hit kicking
 * it on, swelling with its sound), its number under it; the selected one larger, its number in the theme's colour. Each
 * in three isotherms coloured by its own level (quiet cyan .. loud blood and bone), with a face: angry brows over two
 * eyes that shut on a hit, a mouth that opens (and shows its fangs) with the level. A muted track sleeps: a dim
 * outline, its eyes shut. Nothing in the audio path: the tracks' peaks the UI already reads. */
#define COV_N NTRK
#define COV_X0 28                                       /* the swim's walls: clear of the torches */
#define COV_X1 212
#define COV_BOT ((int32_t)graph_h() - 20)             /* the floor: the numbers under it */
static const int16_t COV_PX[COV_N] = {58, 98, 142, 184}, COV_PY[COV_N] = {36, 72, 40, 76};
static struct {
    int32_t en[COV_N], pen[COV_N];                      /* each track's level now (0..256, slewed), the frame before */
    int32_t sx[COV_N], sy[COV_N], vx[COV_N], vy[COV_N]; /* the swim: where it is and its speed, Q8 px */
    int32_t r[COV_N];                                   /* its radius now (Q4 px: swollen, pulsing) */
    uint32_t tm[COV_N], hit[COV_N];                     /* each one's time (it runs with its sound), its last hit's frame */
    uint32_t f0;
    uint8_t init;
} cov;
enum { COVF_SHUT = 1, COVF_SLEEP = 2 };
static const cr_genome_t CR_DRUM_G = {12, 0, 10, 3, 18, 0, 6};   /* a DRUM track: spiky, a beat */
static int cov_off(uint32_t c) { return trk[c].p[P_MUTE] != 0; }
static const cr_genome_t *cov_genome(uint32_t c)
{
    uint32_t e = trk[c].eng_req % NENGINES;
    return e == ENGI_DRUM ? &CR_DRUM_G : &CR_GENOME[CR_EGEN[e]];
}
static uint16_t cov_dim(void)
{
    if (settings.palette == UI_GREY_INDEX || settings.palette == UI_BW_INDEX) {
        uint32_t g = ux_luma(T_DIM) >> 3;
        return (uint16_t)(g << 11 | g << 6 | g);
    }
    return T_DIM;
}
/* the creature c's features: its genome, its HOME knobs (the engine's four main ones), excited by its level k-fold */
static void cov_feat(uint32_t c, cr_feat_t *f, int32_t k)
{
    uint8_t ids[4];
    uint32_t v, e = trk[c].eng_req % NENGINES;
    for (v = 0; v < 4u; v++)
        ids[v] = (uint8_t)ENGINES[e]->knob[v];
    cr_feat(&trk[c], cov_genome(c), ids, f);
    cr_excite(f, cov.en[c], k);
}
/* the swim, once a drawn frame (dt frames since the last): each track's level (its peak, slewed: a hit at once, a
 * quick fall), its time, its size (it pulses, swells up to ~2.2 x; the selected one larger), its swim (wandering,
 * bouncing off the walls, a hit kicking it on up to ~7 x). Still with ANIM OFF */
static uint32_t cov_step(void)
{
    uint32_t c, k, dt = ui.frame - cov.f0;
    if (!cov.init) {
        cov.init = 1;
        for (c = 0; c < COV_N; c++) {
            cov.sx[c] = COV_PX[c] << 8; cov.sy[c] = COV_PY[c] << 8;
            cov.vx[c] = (c & 1u) ? 90 : -110; cov.vy[c] = (c & 2u) ? 70 : -60;
        }
    }
    if (dt > 8u) dt = 8u;
    if (ui_prefs & PREF_ANIM_OFF) dt = 0;
    cov.f0 = ui.frame;
    for (c = 0; c < COV_N; c++) {
        track_t *t = &trk[c];
        int32_t v = t->peak * 256 / 7000, r = (c == song.sel ? 17 : 12) * 16, rad, sp;
        if (v > 256) v = 256;
        cov.pen[c] = cov.en[c];
        if (dt) {                                       /* (once a frame: a redraw in the same frame draws the same) */
            t->peak = 0;
            cov.en[c] = v > cov.en[c] ? v : cov.en[c] + (v - cov.en[c]) / 4;
            if (cov.en[c] - cov.pen[c] > 48)            /* a hit: its eyes shut for ~0.1 s */
                cov.hit[c] = ui.frame;
        }
        if (cov.en[c] > 2) cov.tm[c] += dt * (uint32_t)(200 + cov.en[c] * 5);
        else cov.tm[c] += dt * 60u;                     /* (silent: it still breathes, slowly) */
        r = r * (256 + cov.en[c] * 5 / 4 + (cr_sin(cov.tm[c] / 3u) >> 11)) / 256;
        cov.r[c] = r;
        rad = r >> 4;
        sp = 256 + cov.en[c] * 6;
        for (k = 0; k < dt; k++) {                      /* swimming, bouncing off the walls, wandering */
            cov.vx[c] += cr_sin(ui.frame * 211u + c * 16000u + k * 977u) >> 12;
            cov.vy[c] += cr_sin(ui.frame * 157u + c * 23000u + k * 613u) >> 12;
            cov.vx[c] = cr_cl(cov.vx[c], -140, 140);
            cov.vy[c] = cr_cl(cov.vy[c], -110, 110);
            cov.sx[c] += cov.vx[c] * sp >> 8;
            cov.sy[c] += cov.vy[c] * sp >> 8;
            if (cov.sx[c] < (COV_X0 + rad) << 8) { cov.sx[c] = (COV_X0 + rad) << 8; cov.vx[c] = -cov.vx[c]; }
            if (cov.sx[c] > (COV_X1 - rad) << 8) { cov.sx[c] = (COV_X1 - rad) << 8; cov.vx[c] = -cov.vx[c]; }
            if (cov.sy[c] < (rad + 6) << 8) { cov.sy[c] = (rad + 6) << 8; cov.vy[c] = -cov.vy[c]; }
            if (cov.sy[c] > (COV_BOT - rad) << 8) { cov.sy[c] = (COV_BOT - rad) << 8; cov.vy[c] = -cov.vy[c]; }
        }
    }
    return dt;
}
/* the face at (cx, cy) Q4 of a creature of radius r Q4, scaled k (Q8): angry brows over two eyes (shut on a hit,
 * or asleep), a mouth that opens with e (0..256) and shows its fangs */
static void cov_face(int32_t cx, int32_t cy, int32_t r, int32_t k, int32_t e, uint32_t fl)
{
    int32_t rk = r * k >> 8, s = rk >= 19 * 16 ? 3 : 2, ex = rk * 77 >> 8, d;
    int32_t ey = cy - (rk * 31 >> 8), my, mo;
    uint16_t ec = (fl & COVF_SLEEP) ? cov_dim() : cr_col(248), mc = cr_col(200);
    if (ex < 40) ex = 40;                               /* (2.5 px apart at least) */
    for (d = -1; d <= 1; d += 2) {
        int32_t X = (cx + d * ex + 8) >> 4, Y = (ey + 8) >> 4;
        if (fl & (COVF_SHUT | COVF_SLEEP))
            cv_rect(X - (s + 1) / 2, Y, s + 1, 1, ec);
        else {
            cv_rect(X - s / 2, Y - s / 2, s, s, ec);
            cr_line((X + d * (s + 2)) * 16, (Y - s - 3) * 16, (X - d * (s + 2)) * 16, (Y - s) * 16, ec);   /* the brow: angry */
        }
    }
    my = ey + (rk * 92 >> 8 > 56 ? rk * 92 >> 8 : 56);
    mo = (e * 333 >> 8 > 256 ? 256 : e * 333 >> 8) * (rk * 97 >> 8) >> 8;   /* (its level x 1.3, at most the whole) */
    if (fl & COVF_SLEEP)
        cv_rect(((cx + 8) >> 4) - 1, (my + 8) >> 4, 3, 1, ec);
    else if (mo > 21) {
        int32_t rx = (rk * 26 >> 8) + (mo * 72 >> 8), ry = mo / 2, my2 = my + mo / 2 - 8, i;
        uint32_t a;
        int32_t px = 0, py = 0;
        rx = rx < 24 ? 24 : rx;
        for (a = 0; a <= 10u; a++) {                    /* the open mouth: an ellipse in ten lines */
            uint32_t ang = a * 65536u / 10u;
            int32_t x = cx + ((rx * cr_sin(ang + 0x4000u)) >> 15), y = my2 + ((ry * cr_sin(ang)) >> 15);
            if (a)
                cr_line(px, py, x, y, mc);
            px = x;
            py = y;
        }
        for (i = -1; i <= 1; i += 2)                    /* its fangs, from the top edge */
            cv_rect(((cx + i * rx / 2 + 8) >> 4), ((my2 - ry + 8) >> 4) + 1, 1, 2 + (ry > 40), ec);
    } else {
        int32_t w = ((rk * 31 >> 8) + 8) >> 4;
        if (w < 1) w = 1;
        cv_rect(((cx + 8) >> 4) - w, (my + 8) >> 4, 2 * w + 1, 1, mc);
    }
}
/* creature c at (cx, cy) Q4, radius rq Q4: three isotherms by its own level (the outer one with its species' spikes),
 * its face; muted: asleep, a dim outline without spikes, its eyes shut */
static void cov_being(uint32_t c, int32_t cx, int32_t cy, int32_t rq)
{
    static const int16_t SC[3] = {256, 179, 113};
    const cr_genome_t *g = cov_genome(c);
    cr_feat_t f;
    int32_t en = cov.en[c], rr = rq + rq * en * 30 / 65536, i, h = 23 + en * 118 / 256, st = 38 + en * 13 / 256;
    uint32_t t = cov.tm[c] + c * 20000u, fl;
    if (cov_off(c)) {
        cr_feat_t a = {0, 0, 40, 3, 1, 0, 0, 0};
        cr_outline(g, &a, cx, cy, rq, 0, cov.tm[c], cov_dim());
        cov_face(cx, cy, rq, 230, 0, COVF_SLEEP);
        return;
    }
    cov_feat(c, &f, 1);                                 /* (JIANT excites 4-fold on a taller stage) */
    for (i = 0; i < 3; i++)                             /* (their colours by its level) */
        cr_outline(g, &f, cx, cy, rr * SC[i] >> 8, !i, t, cr_col(h + i * st));
    fl = (ui.frame - cov.hit[c]) < 3u && en > 77 ? COVF_SHUT : 0u;
    cov_face(cx, cy, rr, 159, en, fl);
}
/* HOME's stage: the granules drifting, the creatures swimming, each with its number under it */
static void graph_coven(void)
{
    uint32_t c, k, still = (ui_prefs & PREF_ANIM_OFF) != 0u;
    (void)cov_step();
    cv_oy = 0;
    for (k = 0; k < 22u; k++) {                         /* the dark: granules drifting */
        uint32_t h = k * 2654435761u;
        int32_t gx = COV_X0 + (int32_t)((h >> 8) % 180u + ui.frame * (1u + (h >> 28)) / 6u) % 180;
        int32_t gy = 8 + (int32_t)(((h >> 16) % (uint32_t)(COV_BOT + 6)) + (still ? 0 : (uint32_t)(cr_sin((uint32_t)(ui.frame * 300u + h)) >> 13))) %
                         (uint32_t)(COV_BOT + 6);
        cv_rect(gx, gy, 1 + (int32_t)(h >> 31), 1 + (int32_t)(h >> 31), ux_mix(T_SURF, T_THEME, 22 + (int32_t)(h >> 29) * 4));
    }
    for (c = 0; c < COV_N; c++) {
        int32_t rad = cov.r[c] >> 4, px = cov.sx[c] >> 8, py = cov.sy[c] >> 8, ny = py + rad / 2 + 4;
        char num[2] = {(char)('1' + c), 0};
        cov_being(c, cov.sx[c] >> 4, cov.sy[c] >> 4, cov.r[c]);
        if (ny > COV_BOT + 4) ny = COV_BOT + 4;
        cv_text(px - text_w(&AF_S, num) / 2, ny, &AF_S, num, c == song.sel ? T_THEME : cov_off(c) ? T_DIM : T_MID);
    }
}
