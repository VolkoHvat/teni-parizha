/* Lutèce Engine — процедурные PBR-текстуры */
#include "lu_texgen.h"

void lu_texset_alloc(lu_texset *t, int n) {
    t->n = n; size_t c = (size_t)n * n;
    t->alb = lu_alloc(c * 4); t->nrm = lu_alloc(c * 4); t->orm = lu_alloc(c * 4); t->hgt = lu_alloc(c * sizeof(float));
}
void lu_texset_free(lu_texset *t) { lu_free(t->alb); lu_free(t->nrm); lu_free(t->orm); lu_free(t->hgt); memset(t, 0, sizeof *t); }
void lu_texset_upload(const lu_texset *t, lu_texture *a, lu_texture *n, lu_texture *o) {
    if (a) *a = lu_texture_create(t->n, t->n, t->alb, LU_TEX_SRGB | LU_TEX_MIPS);
    if (n) *n = lu_texture_create(t->n, t->n, t->nrm, LU_TEX_MIPS);
    if (o) *o = lu_texture_create(t->n, t->n, t->orm, LU_TEX_MIPS);
}
static uint32_t h3(int x, int y, uint32_t s) { return lu_hash((uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u ^ s * 83492791u); }
float lu_tnoise(float x, float y, int p, uint32_t seed) {
    int xi = (int)floorf(x), yi = (int)floorf(y); float fx = x - xi, fy = y - yi;
    float ux = fx * fx * fx * (fx * (fx * 6 - 15) + 10), uy = fy * fy * fy * (fy * (fy * 6 - 15) + 10);
    float r = 0;
    for (int j = 0; j < 2; j++) for (int i = 0; i < 2; i++) {
        int cx = ((xi + i) % p + p) % p, cy = ((yi + j) % p + p) % p; uint32_t h = h3(cx, cy, seed);
        float a = (h & 0xFFFF) / 65536.f * LU_TAU, gx = cosf(a), gy = sinf(a);
        float d = gx * (fx - i) + gy * (fy - j), w = (i ? ux : 1 - ux) * (j ? uy : 1 - uy);
        r += d * w;
    }
    return r * 1.41f;  /* примерно -1..1 */
}
float lu_tfbm(float x, float y, int p, int oct, uint32_t seed) {
    float s = 0, a = .5f, n = 0;
    for (int i = 0; i < oct; i++) { s += lu_tnoise(x, y, p, seed + (uint32_t)i * 101u) * a; n += a; x *= 2; y *= 2; p *= 2; a *= .5f; }
    return s / n;
}
void lu_tvoronoi(float x, float y, int p, uint32_t seed, float *f1, float *f2, uint32_t *id) {
    int xi = (int)floorf(x), yi = (int)floorf(y); float b1 = 9, b2 = 9; uint32_t bid = 0;
    for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
        int cx = xi + i, cy = yi + j, wx = ((cx % p) + p) % p, wy = ((cy % p) + p) % p; uint32_t h = h3(wx, wy, seed);
        float px = cx + .15f + .7f * lu_h01(h), py = cy + .15f + .7f * lu_h01(h >> 7 ^ h * 31u);
        float d = sqrtf((px - x) * (px - x) + (py - y) * (py - y));
        if (d < b1) { b2 = b1; b1 = d; bid = h; } else if (d < b2) b2 = d;
    }
    *f1 = b1; *f2 = b2; if (id) *id = bid;
}
static uint32_t pack_srgb(v3 c, float a) {
    float r = powf(lu_clampf(c.x, 0, 1), 1 / 2.2f), g = powf(lu_clampf(c.y, 0, 1), 1 / 2.2f), b = powf(lu_clampf(c.z, 0, 1), 1 / 2.2f);
    return ((uint32_t)(lu_clampf(a, 0, 1) * 255) << 24) | ((uint32_t)(b * 255) << 16) | ((uint32_t)(g * 255) << 8) | (uint32_t)(r * 255);
}
void lu_texset_finish(lu_texset *t, float strength, float ao, const float *rough, const float *metal) {
    int n = t->n;
    for (int y = 0; y < n; y++) for (int x = 0; x < n; x++) {
        float hl = t->hgt[y * n + (x + n - 1) % n], hr = t->hgt[y * n + (x + 1) % n], hd = t->hgt[((y + n - 1) % n) * n + x], hu = t->hgt[((y + 1) % n) * n + x];
        v3 nn = v3_norm(V3(-(hr - hl) * strength * n / 256.f, -(hu - hd) * strength * n / 256.f, 1));
        t->nrm[y * n + x] = 0xFF000000u | ((uint32_t)((nn.z * .5f + .5f) * 255) << 16) | ((uint32_t)((nn.y * .5f + .5f) * 255) << 8) | (uint32_t)((nn.x * .5f + .5f) * 255);
        /* затенение: насколько точка ниже соседей */
        float h0 = t->hgt[y * n + x], avg = 0; int cnt = 0;
        for (int k = 1; k <= 4; k++) { int d = k * n / 128 + 1; avg += t->hgt[y * n + (x + d) % n] + t->hgt[y * n + (x + n - d % n) % n] + t->hgt[((y + d) % n) * n + x] + t->hgt[((y + n - d % n) % n) * n + x]; cnt += 4; }
        avg /= cnt; float o = lu_clampf(1 - fmaxf(0.f, avg - h0) * ao, .2f, 1);
        float r = rough ? rough[y * n + x] : .8f, m = metal ? metal[y * n + x] : 0;
        t->orm[y * n + x] = 0xFF000000u | ((uint32_t)(lu_clampf(m, 0, 1) * 255) << 16) | ((uint32_t)(lu_clampf(r, 0, 1) * 255) << 8) | (uint32_t)(o * 255);
    }
}
static v3 vcol(lu_palette p, float t, float jitter) { v3 c = v3_lerp(p.c0, p.c1, lu_clampf(t, 0, 1)); float k = 1 + jitter * p.var; return v3_scale(c, k); }

void lu_texgen(lu_texset *t, int type, int n, lu_palette pal, uint32_t seed) {
    lu_texset_alloc(t, n);
    float *rough = lu_alloc(sizeof(float) * (size_t)n * n), *metal = NULL; float strength = 3.f, ao = 3.f;
    for (int y = 0; y < n; y++) for (int x = 0; x < n; x++) {
        float u = (x + .5f) / n, v = (y + .5f) / n, h = .5f, a = 1, r = .85f; v3 c = pal.c0;
        float grain = lu_tfbm(u * 16, v * 16, 16, 4, seed);
        switch (type) {
        case LU_TG_STONE_BLOCKS: case LU_TG_BRICK: case LU_TG_FLAGSTONE: {
            int rows = type == LU_TG_BRICK ? 8 : type == LU_TG_FLAGSTONE ? 3 : 4, cols = type == LU_TG_BRICK ? 4 : type == LU_TG_FLAGSTONE ? 3 : 2;
            float mort = type == LU_TG_BRICK ? .1f : type == LU_TG_FLAGSTONE ? .025f : .035f;
            float fy = v * rows; int row = (int)fy; float ofs = (row & 1) ? .5f : 0; if (type == LU_TG_FLAGSTONE) ofs = lu_h01(h3(row, 0, seed)) * .6f;
            float fx = u * cols + ofs; int col = (int)floorf(fx); float lx = fx - col, ly = fy - row;
            uint32_t bid = h3(((col % cols) + cols) % cols, row, seed);
            float edge = fminf(fminf(lx, 1 - lx) * (float)rows / cols * (type == LU_TG_BRICK ? 2.f : 1.f), fminf(ly, 1 - ly));
            float wob = lu_tfbm(u * 32, v * 32, 32, 3, seed + 7) * .012f;
            float m = lu_smooth((edge + wob - mort * .5f) / (mort * .8f));
            float chips = lu_tfbm(u * 48, v * 48, 48, 3, seed + 3); m *= 1 - lu_smooth((chips - .35f) * 6) * lu_smooth((.12f - edge) * 20) * .7f;
            h = m * (.85f + .15f * lu_smooth(edge * 6)) + grain * .05f;
            float tint = lu_h01(bid); c = vcol(pal, tint, lu_h01(bid >> 9) * 2 - 1);
            c = v3_scale(c, .92f + .16f * grain);
            v3 mortar = v3_scale(v3_lerp(pal.c0, V3(.5f, .5f, .48f), .5f), .7f);
            c = v3_lerp(mortar, c, m);
            float grime = lu_smooth(lu_tfbm(u * 4, v * 2, 4, 3, seed + 11) * 1.5f + .2f) * .35f; c = v3_scale(c, 1 - grime * (1 - v * .5f));
            r = lu_lerpf(.95f, .75f + .1f * grain, m); break; }
        case LU_TG_PLASTER: {
            float n1 = lu_tfbm(u * 8, v * 8, 8, 5, seed), crack = 0;
            float f1, f2; lu_tvoronoi(u * 6, v * 6, 6, seed + 5, &f1, &f2, NULL); crack = lu_smooth(1 - (f2 - f1) * 25) * lu_smooth(lu_tfbm(u * 4, v * 4, 4, 2, seed + 9) * 3);
            h = .5f + n1 * .25f - crack * .3f;
            c = vcol(pal, .5f + n1, n1); float stain = lu_smooth(lu_tfbm(u * 3, v * 3, 3, 4, seed + 2) * 2) * .3f;
            c = v3_scale(c, (1 - stain) * (1 - crack * .5f)); r = .9f; break; }
        case LU_TG_COBBLE: {
            float f1, f2; uint32_t id; float w = lu_tfbm(u * 10, v * 10, 10, 2, seed + 3) * .15f; lu_tvoronoi(u * 10 + w, v * 10 + w, 10, seed, &f1, &f2, &id);
            float m = lu_smooth((f2 - f1) * 4.5f); h = m * (.75f + .25f * lu_h01(id)) + grain * .04f;
            c = v3_lerp(V3(.12f, .11f, .1f), vcol(pal, lu_h01(id), lu_h01(id >> 8) * 2 - 1), m);
            c = v3_scale(c, .9f + .2f * grain); r = lu_lerpf(.95f, .55f + .2f * lu_h01(id >> 4), m); break; }
        case LU_TG_SLATE: case LU_TG_ROOF_TILES: {
            int rows = type == LU_TG_SLATE ? 10 : 8, cols = type == LU_TG_SLATE ? 6 : 6;
            float fy = v * rows; int row = (int)fy; float ly = fy - row, fx = u * cols + ((row & 1) ? .5f : 0); int col = (int)floorf(fx); float lx = fx - col;
            uint32_t id = h3(((col % cols) + cols) % cols, row, seed);
            if (type == LU_TG_SLATE) { h = ly * .9f + .1f; float side = fminf(lx, 1 - lx); h *= lu_smooth(side * 30); c = vcol(pal, lu_h01(id), lu_h01(id >> 8) * 2 - 1); c = v3_scale(c, .7f + .4f * ly); r = .55f + .2f * grain; }
            else { float arch = sinf(lx * LU_PI); h = arch * .7f + ly * .3f; c = vcol(pal, lu_h01(id), lu_h01(id >> 8) * 2 - 1); c = v3_scale(c, .6f + .4f * arch * (.6f + .4f * ly)); r = .7f; }
            c = v3_scale(c, .92f + .16f * grain);
            float moss = lu_smooth(lu_tfbm(u * 5, v * 5, 5, 3, seed + 4) * 2.5f - .4f) * .4f; c = v3_lerp(c, V3(.16f, .19f, .1f), moss * (1 - ly)); break; }
        case LU_TG_WOOD: case LU_TG_SHUTTER: {
            int planks = type == LU_TG_SHUTTER ? 8 : 5; float fx = u * planks; int pk = (int)fx; float lx = fx - pk;
            float g = lu_tfbm(u * 4 + lu_h01(h3(pk, 0, seed)) * 7, v * 40, 4, 4, seed + (uint32_t)pk);
            if (type == LU_TG_SHUTTER) { float ly = fmodf(v * 16, 1); h = lu_smooth(ly * 4) * lu_smooth((1 - ly) * 1.5f) * .8f; c = vcol(pal, .5f + g, g); c = v3_scale(c, .75f + .35f * h); r = .7f; }
            else { h = .6f + g * .2f; h *= lu_smooth(fminf(lx, 1 - lx) * 40); c = vcol(pal, lu_h01(h3(pk, 1, seed)), g * 2); c = v3_scale(c, .85f + .3f * g); r = .75f; }
            break; }
        case LU_TG_DIRT: { float n1 = lu_tfbm(u * 8, v * 8, 8, 6, seed); h = .5f + n1 * .4f; c = vcol(pal, .5f + n1 * 1.5f, n1); r = .95f; break; }
        case LU_TG_CLOTH: { float wx = sinf(u * n * .5f), wy = sinf(v * n * .5f); h = .5f + .25f * (wx * (wy > 0) + wy * (wx <= 0)); c = vcol(pal, .5f + grain, grain * .5f); c = v3_scale(c, .9f + .1f * h); r = .95f; strength = 1.f; break; }
        case LU_TG_METAL: { float b = lu_tfbm(u * 2, v * 64, 2, 4, seed); h = .5f + b * .1f; c = vcol(pal, .5f + b, 0); r = .35f + .15f * b; break; }
        }
        t->hgt[y * n + x] = h; t->alb[y * n + x] = pack_srgb(c, a); rough[y * n + x] = r;
    }
    if (type == LU_TG_METAL) { metal = lu_alloc(sizeof(float) * (size_t)n * n); for (int i = 0; i < n * n; i++) metal[i] = 1; }
    if (type == LU_TG_COBBLE || type == LU_TG_SLATE || type == LU_TG_ROOF_TILES) strength = 4.f;
    lu_texset_finish(t, strength, ao, rough, metal);
    lu_free(rough); lu_free(metal);
}
