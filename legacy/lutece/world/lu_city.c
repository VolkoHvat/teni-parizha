/* Lutèce Engine — генератор городской застройки */
#include "lu_city.h"
#include "../gfx/lu_texgen.h"
#include <stdio.h>

#define C3(r, g, b) ((uint32_t)(0xFF000000u | ((uint32_t)(b) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(r)))

void lu_city_init(lu_city *c, lu_phys_world *phys, aabb area, float chunk) {
    memset(c, 0, sizeof *c); c->phys = phys; c->chunk = chunk; c->origin = area.mn;
    c->cw = (int)((area.mx.x - area.mn.x) / chunk) + 1; c->ch = (int)((area.mx.z - area.mn.z) / chunk) + 1;
    if (c->cw * c->ch > LU_CITY_MAXCH) { c->ch = LU_CITY_MAXCH / c->cw; }
    c->b = lu_alloc(sizeof(lu_mesh_builder) * (size_t)(c->cw * c->ch * LU_CM_N));
    for (int i = 0; i < c->cw * c->ch * LU_CM_N; i++) lu_mb_init(&c->b[i]);
}
void lu_city_free(lu_city *c) {
    int n = c->cw * c->ch * LU_CM_N;
    for (int i = 0; i < n; i++) { lu_mb_free(&c->b[i]); if (c->mesh) lu_mesh_destroy(&c->mesh[i]); }
    lu_free(c->b); lu_free(c->mesh); lu_vec_free(c->lamps); lu_vec_free(c->hay); lu_vec_free(c->spawn_pts); memset(c, 0, sizeof *c);
}
lu_mesh_builder *lu_city_mb(lu_city *c, v3 at, int mat) {
    int x = (int)((at.x - c->origin.x) / c->chunk), z = (int)((at.z - c->origin.z) / c->chunk);
    x = x < 0 ? 0 : x >= c->cw ? c->cw - 1 : x; z = z < 0 ? 0 : z >= c->ch ? c->ch - 1 : z;
    return &c->b[(z * c->cw + x) * LU_CM_N + mat];
}
/* ---------- примитивы с UV в метрах ---------- */
static void quad(lu_mesh_builder *b, v3 a, v3 bb, v3 cc, v3 d, v2 ua, v2 ub, v2 uc, v2 ud, uint32_t col) {
    v3 n = v3_norm(v3_cross(v3_sub(bb, a), v3_sub(d, a))); b->color = col;
    int i0 = lu_mb_vertex(b, a, n, ua), i1 = lu_mb_vertex(b, bb, n, ub), i2 = lu_mb_vertex(b, cc, n, uc), i3 = lu_mb_vertex(b, d, n, ud);
    lu_mb_tri(b, i0, i1, i2); lu_mb_tri(b, i0, i2, i3);
}
/* рамка фасада: начало o, вправо r, нормаль n (наружу); локальные (u — вдоль, v — вверх, d — наружу) */
typedef struct { v3 o, r, n; } Frame;
static v3 F(const Frame *f, float u, float v, float d) { return v3_add(v3_add(f->o, v3_scale(f->r, u)), v3_add(V3(0, v, 0), v3_scale(f->n, d))); }
static void frect(lu_city *c, int mat, const Frame *f, float u0, float u1, float v0, float v1, float d, float uvs, uint32_t col) {
    if (u1 - u0 < 1e-3f || v1 - v0 < 1e-3f) return;
    v3 a = F(f, u0, v0, d), b = F(f, u1, v0, d), cc = F(f, u1, v1, d), dd = F(f, u0, v1, d);
    lu_mesh_builder *mb = lu_city_mb(c, a, mat);
    quad(mb, a, b, cc, dd, V2(u0 * uvs, v0 * uvs), V2(u1 * uvs, v0 * uvs), V2(u1 * uvs, v1 * uvs), V2(u0 * uvs, v1 * uvs), col);
}
/* коробка в рамке фасада; collide >= 0 — добавить в физику с этими флагами */
static void fbox(lu_city *c, int mat, const Frame *f, float u0, float u1, float v0, float v1, float d0, float d1, float uvs, uint32_t col, int collide) {
    v3 p[8]; for (int i = 0; i < 8; i++) p[i] = F(f, i & 1 ? u1 : u0, i & 2 ? v1 : v0, i & 4 ? d1 : d0);
    lu_mesh_builder *mb = lu_city_mb(c, p[0], mat);
    float U0 = u0 * uvs, U1 = u1 * uvs, V0 = v0 * uvs, V1 = v1 * uvs, D0 = d0 * uvs, D1 = d1 * uvs;
    quad(mb, p[4], p[5], p[7], p[6], V2(U0, V0), V2(U1, V0), V2(U1, V1), V2(U0, V1), col);   /* перед (наружу) */
    quad(mb, p[2], p[6], p[7], p[3], V2(U0, D0), V2(U0, D1), V2(U1, D1), V2(U1, D0), col);   /* верх */
    quad(mb, p[0], p[1], p[5], p[4], V2(U0, D0), V2(U1, D0), V2(U1, D1), V2(U0, D1), col);   /* низ */
    quad(mb, p[0], p[4], p[6], p[2], V2(D0, V0), V2(D1, V0), V2(D1, V1), V2(D0, V1), col);   /* левый торец */
    quad(mb, p[5], p[1], p[3], p[7], V2(D1, V0), V2(D0, V0), V2(D0, V1), V2(D1, V1), col);   /* правый торец */
    if (collide >= 0 && c->phys) {
        v3 mn = p[0], mx = p[0]; for (int i = 1; i < 8; i++) { mn = v3_min(mn, p[i]); mx = v3_max(mx, p[i]); }
        lu_phys_add_box(c->phys, (aabb){mn, mx}, collide);
    }
}
void lu_city_box(lu_city *c, int mat, aabb b, float uvs, uint32_t col, int collide) {
    lu_mesh_builder *mb = lu_city_mb(c, aabb_center(b), mat); mb->color = col;
    lu_mb_box(mb, b.mn, b.mx, uvs);
    if (collide >= 0 && c->phys) lu_phys_add_box(c->phys, b, collide);
}
void lu_city_ground(lu_city *c, int mat, float x0, float z0, float x1, float z1, float y, float uvs, int collide) {
    for (float z = z0; z < z1 - 1e-3f; z += c->chunk) for (float x = x0; x < x1 - 1e-3f; x += c->chunk) {
        float xa = x, za = z, xb = fminf(x + c->chunk, x1), zb = fminf(z + c->chunk, z1);
        lu_mesh_builder *mb = lu_city_mb(c, V3((xa + xb) * .5f, y, (za + zb) * .5f), mat);
        quad(mb, V3(xa, y, zb), V3(xb, y, zb), V3(xb, y, za), V3(xa, y, za), V2(xa * uvs, zb * uvs), V2(xb * uvs, zb * uvs), V2(xb * uvs, za * uvs), V2(xa * uvs, za * uvs), 0xFFFFFFFFu);
    }
    if (collide >= 0 && c->phys) lu_phys_add_box(c->phys, (aabb){V3(x0, y - .5f, z0), V3(x1, y, z1)}, collide);
}
static void tri_phys(lu_city *c, v3 a, v3 b, v3 cc, int flags) { if (c->phys) lu_phys_add_tri(c->phys, a, b, cc, flags); }
static void roof_quad(lu_city *c, int mat, v3 a, v3 b, v3 cc, v3 d, float uvs, int flags) {
    /* UV: вдоль ската (по горизонтали a→b) и вверх по скату */
    float w = v3_dist(a, b), h = v3_dist(a, d);
    lu_mesh_builder *mb = lu_city_mb(c, a, mat);
    quad(mb, a, b, cc, d, V2(0, 0), V2(w * uvs, 0), V2(w * uvs, h * uvs), V2(0, h * uvs), 0xFFFFFFFFu);
    tri_phys(c, a, b, cc, flags); tri_phys(c, a, cc, d, flags);
}
static void roof_tri(lu_city *c, int mat, v3 a, v3 b, v3 cc, float uvs, int flags) {
    lu_mesh_builder *mb = lu_city_mb(c, a, mat); v3 n = v3_norm(v3_cross(v3_sub(b, a), v3_sub(cc, a))); mb->color = 0xFFFFFFFFu;
    float w = v3_dist(a, b); v3 mid = v3_lerp(a, b, .5f); float h = v3_dist(mid, cc);
    int i0 = lu_mb_vertex(mb, a, n, V2(0, 0)), i1 = lu_mb_vertex(mb, b, n, V2(w * uvs, 0)), i2 = lu_mb_vertex(mb, cc, n, V2(w * .5f * uvs, h * uvs));
    lu_mb_tri(mb, i0, i1, i2); tri_phys(c, a, b, cc, flags);
}

/* ---------- окно в нише ---------- */
static void window(lu_city *c, const lu_building *B, const Frame *f, int wm, float uc, float v0, float w, float h, int detail, int french, lu_rng *rng) {
    float u0 = uc - w * .5f, u1 = uc + w * .5f, v1 = v0 + h, dep = .24f;
    uint32_t tint = B->tint, trim = B->trim_col;
    /* откосы ниши */
    frect(c, wm, &(Frame){F(f, u0, 0, 0), v3_neg(f->n), f->r}, 0, dep, v0, v1, 0, .5f, tint);   /* левый откос (смотрит вправо) */
    { Frame fr = {F(f, u1, 0, -dep), f->n, v3_neg(f->r)}; frect(c, wm, &fr, 0, dep, v0, v1, 0, .5f, tint); }
    { lu_mesh_builder *mb = lu_city_mb(c, F(f, uc, v1, 0), wm); v3 a = F(f, u0, v1, 0), b = F(f, u1, v1, 0), cc = F(f, u1, v1, -dep), d = F(f, u0, v1, -dep); quad(mb, a, b, cc, d, V2(0, 0), V2(w * .5f, 0), V2(w * .5f, dep * .5f), V2(0, dep * .5f), tint); }
    { lu_mesh_builder *mb = lu_city_mb(c, F(f, uc, v0, 0), wm); v3 a = F(f, u0, v0, -dep), b = F(f, u1, v0, -dep), cc = F(f, u1, v0, 0), d = F(f, u0, v0, 0); quad(mb, a, b, cc, d, V2(0, 0), V2(w * .5f, 0), V2(w * .5f, dep * .5f), V2(0, dep * .5f), tint); }
    /* стекло и переплёт */
    int lit = lu_h01(rng_u32(rng)) < .35f;
    frect(c, LU_CM_GLASS, f, u0, u1, v0, v1, -dep, 1, lit ? C3(255, 200, 120) : C3(40, 46, 52));
    if (detail) {
        float fw = .05f;
        fbox(c, LU_CM_TRIM, f, u0, u1, v0, v0 + fw, -dep, -dep + .06f, 1, trim, -1);
        fbox(c, LU_CM_TRIM, f, u0, u1, v1 - fw, v1, -dep, -dep + .06f, 1, trim, -1);
        fbox(c, LU_CM_TRIM, f, u0, u0 + fw, v0, v1, -dep, -dep + .06f, 1, trim, -1);
        fbox(c, LU_CM_TRIM, f, u1 - fw, u1, v0, v1, -dep, -dep + .06f, 1, trim, -1);
        fbox(c, LU_CM_TRIM, f, uc - fw * .5f, uc + fw * .5f, v0, v1, -dep, -dep + .05f, 1, trim, -1);
        for (int k = 1; k < (french ? 4 : 3); k++) { float vy = v0 + h * k / (french ? 4 : 3); fbox(c, LU_CM_TRIM, f, u0, u1, vy - .02f, vy + .02f, -dep, -dep + .045f, 1, trim, -1); }
        /* подоконник (маленький выступ) и наличник сверху */
        if (!french) fbox(c, LU_CM_STONE, f, u0 - .1f, u1 + .1f, v0 - .1f, v0, 0, .14f, .5f, tint, -1);
        fbox(c, LU_CM_STONE, f, u0 - .12f, u1 + .12f, v1, v1 + .16f, 0, .07f, .5f, tint, -1);
        if (lu_h01(B->seed * 7u + 3u) > .5f) fbox(c, LU_CM_STONE, f, uc - .14f, uc + .14f, v1 - .02f, v1 + .2f, 0, .1f, .5f, tint, -1); /* замковый камень */
        /* ставни, раскрытые к стене */
        if (B->shutter_col) {
            float sw = w * .5f, ang = .25f + .5f * lu_h01(rng_u32(rng));
            for (int s = 0; s < 2; s++) {
                float hinge = s ? u1 + .02f : u0 - .02f, dir = s ? 1.f : -1.f;
                v3 hb = F(f, hinge, v0, .02f); v3 out = v3_norm(v3_add(v3_scale(f->r, dir * cosf(ang)), v3_scale(f->n, sinf(ang))));
                Frame sf = {hb, out, v3_norm(v3_cross(out, V3(0, 1, 0)))}; if (v3_dot(sf.n, f->n) < 0) sf.n = v3_neg(sf.n);
                fbox(c, LU_CM_SHUTTER, &sf, 0, sw, 0, h, -.02f, .02f, 1.f / h, B->shutter_col, -1);
            }
        }
    }
}
/* ---------- фасад одной стороны ---------- */
static void facade(lu_city *c, const lu_building *B, int side, lu_rng *rng) {
    float x0 = B->x0, z0 = B->z0, x1 = B->x1, z1 = B->z1, y0 = B->base_y;
    Frame f; float len;
    switch (side) {
    case 0: f = (Frame){V3(x1, y0, z0), V3(-1, 0, 0), V3(0, 0, -1)}; len = x1 - x0; break;
    case 1: f = (Frame){V3(x1, y0, z1), V3(0, 0, -1), V3(1, 0, 0)}; len = z1 - z0; break;
    case 2: f = (Frame){V3(x0, y0, z1), V3(1, 0, 0), V3(0, 0, 1)}; len = x1 - x0; break;
    default: f = (Frame){V3(x0, y0, z0), V3(0, 0, 1), V3(-1, 0, 0)}; len = z1 - z0; break;
    }
    int street = (B->street_sides >> side) & 1, shop = (B->shops >> side) & 1, wm = B->wall_mat;
    float uvs = wm == LU_CM_BRICK ? .4f : .45f;
    float bay = 2.7f; int nb = (int)((len - .8f) / bay); if (nb < 1) nb = 1; float margin = (len - nb * bay) * .5f;
    float H = B->ground_h + (B->floors - 1) * B->floor_h;
    uint32_t tint = B->tint;
    /* цоколь первого этажа — камень */
    for (int fl = 0; fl < B->floors; fl++) {
        float fv0 = fl == 0 ? 0 : B->ground_h + (fl - 1) * B->floor_h, fh = fl == 0 ? B->ground_h : B->floor_h, fv1 = fv0 + fh;
        int fmat = fl == 0 && wm != LU_CM_STONE ? LU_CM_STONE : wm; float fuvs = fmat == LU_CM_STONE ? .45f : uvs;
        int french = fl == B->balcony_floor && street;
        float cur = 0;   /* стена между проёмами */
        for (int i = 0; i < nb; i++) {
            float uc = margin + (i + .5f) * bay;
            float ww, wh, wv0;
            if (fl == 0) {
                if (shop && street) { ww = 2.1f; wh = fh - .9f; wv0 = .15f; }
                else if (street && i == nb / 2) { ww = 1.3f; wh = 2.5f; wv0 = 0; }  /* дверь */
                else { ww = 1.0f; wh = 1.6f; wv0 = 1.1f; }
            } else if (!street && lu_h01(rng_u32(rng)) < .35f) { continue; }        /* глухие участки на задворках */
            else { ww = 1.08f; wh = french ? fh - .55f : fh * .55f; wv0 = french ? .05f : fh * .26f; }
            float u0 = uc - ww * .5f, u1 = uc + ww * .5f;
            frect(c, fmat, &f, cur, u0, fv0, fv1, 0, fuvs, tint);                    /* простенок слева */
            frect(c, fmat, &f, u0, u1, fv0, fv0 + wv0, 0, fuvs, tint);              /* под окном */
            frect(c, fmat, &f, u0, u1, fv0 + wv0 + wh, fv1, 0, fuvs, tint);         /* над окном */
            cur = u1;
            if (fl == 0 && shop && street) {
                /* лавка: тёмный проём с деревянной рамой, вывеска и навес */
                float d = .3f;
                frect(c, LU_CM_GLASS, &f, u0, u1, fv0 + wv0, fv0 + wv0 + wh, -d, 1, C3(60, 50, 40));
                fbox(c, LU_CM_WOOD, &f, u0 - .08f, u0, fv0 + wv0, fv0 + wv0 + wh, -d, .04f, .5f, B->shutter_col ? B->shutter_col : C3(90, 60, 40), -1);
                fbox(c, LU_CM_WOOD, &f, u1, u1 + .08f, fv0 + wv0, fv0 + wv0 + wh, -d, .04f, .5f, B->shutter_col ? B->shutter_col : C3(90, 60, 40), -1);
                fbox(c, LU_CM_WOOD, &f, u0 - .1f, u1 + .1f, fv0 + wv0 + wh, fv0 + wv0 + wh + .5f, 0, .08f, .5f, B->trim_col, -1);   /* вывеска */
                if (lu_h01(rng_u32(rng)) < .7f) {   /* навес из ткани (полосатый) */
                    uint32_t ac = lu_h01(rng_u32(rng)) < .5f ? C3(170, 40, 40) : C3(40, 80, 140);
                    v3 a = F(&f, u0 - .15f, fv0 + wv0 + wh + .45f, .02f), b = F(&f, u1 + .15f, fv0 + wv0 + wh + .45f, .02f);
                    v3 cc = F(&f, u1 + .15f, fv0 + wv0 + wh - .1f, 1.3f), dd = F(&f, u0 - .15f, fv0 + wv0 + wh - .1f, 1.3f);
                    lu_mesh_builder *mb = lu_city_mb(c, a, LU_CM_CLOTH); quad(mb, a, b, cc, dd, V2(0, 0), V2(ww * 2, 0), V2(ww * 2, 1), V2(0, 1), ac);
                    quad(mb, b, a, dd, cc, V2(0, 0), V2(ww * 2, 0), V2(ww * 2, 1), V2(0, 1), ac);
                }
                /* перекладины и прилавок */
                fbox(c, LU_CM_WOOD, &f, u0, u1, fv0 + .15f, fv0 + .95f, -d, .05f, .5f, C3(110, 80, 55), -1);
            } else window(c, B, &f, fmat, uc, fv0 + wv0, ww, wh, street || fl > 0, french, rng);
        }
        frect(c, fmat, &f, cur, len, fv0, fv1, 0, fuvs, tint);
        /* межэтажный карниз (уступ для паркура) */
        if (fl < B->floors - 1) {
            float big = fl == 0 ? .3f : .2f, hh = fl == 0 ? .3f : .2f;
            fbox(c, LU_CM_STONE, &f, -.05f, len + .05f, fv1 - hh * .5f, fv1 + hh * .5f, 0, big, .5f, tint, LU_SURF_STONE);
        }
        /* балкон */
        if (french && fl > 0) {
            float bu0 = margin + bay * .1f, bu1 = len - margin - bay * .1f, by = fv0;
            fbox(c, LU_CM_STONE, &f, bu0, bu1, by - .18f, by + .02f, 0, .85f, .5f, tint, LU_SURF_STONE);
            for (int k = 0; k < 3; k++) { float cu = bu0 + (bu1 - bu0) * (k + .5f) / 3; fbox(c, LU_CM_STONE, &f, cu - .12f, cu + .12f, by - .5f, by - .18f, 0, .75f, .5f, tint, -1); }
            /* кованая решётка */
            fbox(c, LU_CM_IRON, &f, bu0, bu1, by + .95f, by + 1.0f, .78f, .83f, 1, C3(30, 30, 34), LU_SURF_METAL);
            fbox(c, LU_CM_IRON, &f, bu0, bu1, by + .1f, by + .13f, .78f, .83f, 1, C3(30, 30, 34), -1);
            for (float u = bu0; u <= bu1; u += .13f) fbox(c, LU_CM_IRON, &f, u, u + .02f, by, by + .97f, .79f, .82f, 1, C3(30, 30, 34), -1);
            fbox(c, LU_CM_IRON, &f, bu0, bu0 + .03f, by, by + 1.f, 0, .83f, 1, C3(30, 30, 34), -1);
            fbox(c, LU_CM_IRON, &f, bu1 - .03f, bu1, by, by + 1.f, 0, .83f, 1, C3(30, 30, 34), -1);
        }
    }
    /* руст по углам */
    if (street) for (float v = .2f; v < H - .3f; v += .5f) {
        int odd = ((int)(v / .5f)) & 1; float wq = odd ? .5f : .35f;
        fbox(c, LU_CM_STONE, &f, 0, wq, v, v + .44f, 0, .04f, .5f, tint, -1);
        fbox(c, LU_CM_STONE, &f, len - wq, len, v, v + .44f, 0, .04f, .5f, tint, -1);
    }
    /* венчающий карниз */
    fbox(c, LU_CM_STONE, &f, -.25f, len + .25f, H - .25f, H, 0, .25f, .5f, tint, LU_SURF_STONE);
    fbox(c, LU_CM_STONE, &f, -.4f, len + .4f, H, H + .2f, 0, .45f, .5f, tint, LU_SURF_STONE);
    if (street) for (float u = .2f; u < len - .1f; u += .35f) fbox(c, LU_CM_STONE, &f, u, u + .12f, H - .42f, H - .25f, 0, .16f, .5f, tint, -1); /* сухарики */
}

static void chimney(lu_city *c, v3 base, float h, uint32_t tint) {
    aabb b = {V3(base.x - .45f, base.y, base.z - .3f), V3(base.x + .45f, base.y + h, base.z + .3f)};
    lu_city_box(c, LU_CM_BRICK, b, .5f, tint, LU_SURF_STONE);
    lu_city_box(c, LU_CM_STONE, (aabb){V3(b.mn.x - .06f, b.mx.y, b.mn.z - .06f), V3(b.mx.x + .06f, b.mx.y + .12f, b.mx.z + .06f)}, .5f, tint, LU_SURF_STONE);
    for (int i = 0; i < 2; i++) { lu_mesh_builder *mb = lu_city_mb(c, base, LU_CM_TILES); mb->color = C3(170, 100, 70); lu_mb_cylinder(mb, V3(base.x - .2f + i * .4f, b.mx.y + .12f, base.z), .09f, .07f, .3f, 8, 1); }
}
static void roof(lu_city *c, const lu_building *B, lu_rng *rng) {
    float H = B->base_y + B->ground_h + (B->floors - 1) * B->floor_h + .2f;
    float x0 = B->x0 - .1f, z0 = B->z0 - .1f, x1 = B->x1 + .1f, z1 = B->z1 + .1f;
    int roofflags = LU_SURF_ROOF;
    if (B->roof == LU_ROOF_FLAT) {
        lu_city_ground(c, LU_CM_FLAG, x0, z0, x1, z1, H, .5f, -1); lu_phys_add_box(c->phys, (aabb){V3(x0, H - .5f, z0), V3(x1, H, z1)}, LU_SURF_STONE);
        float t = .25f, ph = .7f;   /* парапет */
        lu_city_box(c, LU_CM_STONE, (aabb){V3(x0, H, z0), V3(x1, H + ph, z0 + t)}, .5f, B->tint, LU_SURF_STONE);
        lu_city_box(c, LU_CM_STONE, (aabb){V3(x0, H, z1 - t), V3(x1, H + ph, z1)}, .5f, B->tint, LU_SURF_STONE);
        lu_city_box(c, LU_CM_STONE, (aabb){V3(x0, H, z0 + t), V3(x0 + t, H + ph, z1 - t)}, .5f, B->tint, LU_SURF_STONE);
        lu_city_box(c, LU_CM_STONE, (aabb){V3(x1 - t, H, z0 + t), V3(x1, H + ph, z1 - t)}, .5f, B->tint, LU_SURF_STONE);
        chimney(c, V3(lu_lerpf(x0, x1, .3f), H, lu_lerpf(z0, z1, .6f)), 1.4f, B->tint);
        return;
    }
    if (B->roof == LU_ROOF_MANSARD) {
        float ins = .85f, h1 = 2.5f, ins2 = fminf(fminf((x1 - x0), (z1 - z0)) * .5f - ins - .3f, 2.2f), h2 = ins2 * .6f;
        v3 a0 = V3(x0, H, z0), a1 = V3(x1, H, z0), a2 = V3(x1, H, z1), a3 = V3(x0, H, z1);
        v3 b0 = V3(x0 + ins, H + h1, z0 + ins), b1 = V3(x1 - ins, H + h1, z0 + ins), b2 = V3(x1 - ins, H + h1, z1 - ins), b3 = V3(x0 + ins, H + h1, z1 - ins);
        /* нижние крутые скаты */
        roof_quad(c, LU_CM_SLATE, a1, a0, b0, b1, .5f, roofflags); roof_quad(c, LU_CM_SLATE, a2, a1, b1, b2, .5f, roofflags);
        roof_quad(c, LU_CM_SLATE, a3, a2, b2, b3, .5f, roofflags); roof_quad(c, LU_CM_SLATE, a0, a3, b3, b0, .5f, roofflags);
        /* верхняя пологая часть: вальма с коньком вдоль длинной стороны */
        int alongx = (x1 - x0) >= (z1 - z0);
        float cxm = (x0 + x1) * .5f, czm = (z0 + z1) * .5f, half = alongx ? (x1 - x0) * .5f - ins - ins2 : (z1 - z0) * .5f - ins - ins2; if (half < 0) half = 0;
        v3 r0 = alongx ? V3(cxm - half, H + h1 + h2, czm) : V3(cxm, H + h1 + h2, czm - half), r1 = alongx ? V3(cxm + half, H + h1 + h2, czm) : V3(cxm, H + h1 + h2, czm + half);
        if (alongx) { roof_quad(c, LU_CM_SLATE, b1, b0, r0, r1, .5f, roofflags); roof_quad(c, LU_CM_SLATE, b3, b2, r1, r0, .5f, roofflags); roof_tri(c, LU_CM_SLATE, b2, b1, r1, .5f, roofflags); roof_tri(c, LU_CM_SLATE, b0, b3, r0, .5f, roofflags); }
        else { roof_quad(c, LU_CM_SLATE, b2, b1, r0, r1, .5f, roofflags); roof_tri(c, LU_CM_SLATE, b1, b0, r0, .5f, roofflags); roof_quad(c, LU_CM_SLATE, b0, b3, r1, r0, .5f, roofflags); roof_tri(c, LU_CM_SLATE, b3, b2, r1, .5f, roofflags); }
        /* слуховые окна на нижнем скате улицы */
        for (int side = 0; side < 4; side++) {
            if (!((B->street_sides >> side) & 1)) continue;
            float len = side & 1 ? z1 - z0 : x1 - x0; int nd = (int)(len / 3.f);
            for (int i = 0; i < nd; i++) {
                float u = (i + .5f) * len / nd; v3 p, n, r;
                switch (side) { case 0: p = V3(x1 - u, H, z0); n = V3(0, 0, -1); break; case 1: p = V3(x1, H, z1 - u); n = V3(1, 0, 0); break; case 2: p = V3(x0 + u, H, z1); n = V3(0, 0, 1); break; default: p = V3(x0, H, z0 + u); n = V3(-1, 0, 0); }
                r = v3_cross(V3(0, 1, 0), n);
                Frame df = {v3_add(p, v3_scale(n, -.25f)), r, n};
                fbox(c, LU_CM_PLASTER, &df, -.55f, .55f, .2f, 1.7f, -ins - .2f, 0, .5f, B->tint, LU_SURF_STONE);
                frect(c, LU_CM_GLASS, &df, -.35f, .35f, .4f, 1.45f, .01f, 1, lu_h01(rng_u32(rng)) < .3f ? C3(255, 200, 120) : C3(40, 46, 52));
                fbox(c, LU_CM_TRIM, &df, -.4f, .4f, 1.45f, 1.52f, -.02f, .04f, 1, B->trim_col, -1);
                /* двускатная крышка */
                v3 t0 = F(&df, -.65f, 1.7f, .1f), t1 = F(&df, .65f, 1.7f, .1f), t2 = F(&df, 0, 2.15f, .1f), k0 = F(&df, -.65f, 1.7f, -ins - .3f), k1 = F(&df, .65f, 1.7f, -ins - .3f), k2 = F(&df, 0, 2.15f, -ins - .3f);
                roof_quad(c, LU_CM_SLATE, t0, k0, k2, t2, .5f, roofflags); roof_quad(c, LU_CM_SLATE, k1, t1, t2, k2, .5f, roofflags);
                lu_mesh_builder *mb = lu_city_mb(c, t0, LU_CM_PLASTER); mb->color = B->tint; v3 nn = n; int q0 = lu_mb_vertex(mb, t0, nn, V2(0, 0)), q1 = lu_mb_vertex(mb, t1, nn, V2(.6f, 0)), q2 = lu_mb_vertex(mb, t2, nn, V2(.3f, .25f)); lu_mb_tri(mb, q0, q1, q2);
            }
        }
        int nch = 1 + (int)(lu_h01(rng_u32(rng)) * 3);
        for (int i = 0; i < nch; i++) { float u = lu_h01(rng_u32(rng)); v3 p = alongx ? V3(lu_lerpf(r0.x, r1.x, u), 0, czm + (i & 1 ? .6f : -.6f)) : V3(cxm + (i & 1 ? .6f : -.6f), 0, lu_lerpf(r0.z, r1.z, u)); p.y = H + h1 + h2 * .5f; chimney(c, p, 1.6f, C3(150, 90, 70)); }
        return;
    }
    /* двускатная черепичная */
    int alongx = (x1 - x0) >= (z1 - z0); float rise = (alongx ? (z1 - z0) : (x1 - x0)) * .5f * .75f;
    float cxm = (x0 + x1) * .5f, czm = (z0 + z1) * .5f, ov = .35f;
    if (alongx) {
        v3 e0 = V3(x0 - ov, H - .25f, z0 - ov), e1 = V3(x1 + ov, H - .25f, z0 - ov), e2 = V3(x1 + ov, H - .25f, z1 + ov), e3 = V3(x0 - ov, H - .25f, z1 + ov), r0 = V3(x0 - ov, H + rise, czm), r1 = V3(x1 + ov, H + rise, czm);
        roof_quad(c, LU_CM_TILES, e1, e0, r0, r1, .6f, roofflags); roof_quad(c, LU_CM_TILES, e3, e2, r1, r0, .6f, roofflags);
        lu_mesh_builder *mb = lu_city_mb(c, r0, B->wall_mat); mb->color = B->tint;
        for (int s = 0; s < 2; s++) { float x = s ? x1 : x0; v3 n = V3(s ? 1.f : -1.f, 0, 0); int q0 = lu_mb_vertex(mb, V3(x, H, z0), n, V2(z0 * .45f, H * .45f)), q1 = lu_mb_vertex(mb, V3(x, H, z1), n, V2(z1 * .45f, H * .45f)), q2 = lu_mb_vertex(mb, V3(x, H + rise, czm), n, V2(czm * .45f, (H + rise) * .45f)); if (s) lu_mb_tri(mb, q0, q2, q1); else lu_mb_tri(mb, q0, q1, q2);
            tri_phys(c, V3(x, H, z0), V3(x, H, z1), V3(x, H + rise, czm), LU_SURF_STONE); }
    } else {
        v3 e0 = V3(x0 - ov, H - .25f, z0 - ov), e1 = V3(x1 + ov, H - .25f, z0 - ov), e2 = V3(x1 + ov, H - .25f, z1 + ov), e3 = V3(x0 - ov, H - .25f, z1 + ov), r0 = V3(cxm, H + rise, z0 - ov), r1 = V3(cxm, H + rise, z1 + ov);
        roof_quad(c, LU_CM_TILES, e0, e3, r1, r0, .6f, roofflags); roof_quad(c, LU_CM_TILES, e2, e1, r0, r1, .6f, roofflags);
        lu_mesh_builder *mb = lu_city_mb(c, r0, B->wall_mat); mb->color = B->tint;
        for (int s = 0; s < 2; s++) { float z = s ? z1 : z0; v3 n = V3(0, 0, s ? 1.f : -1.f); int q0 = lu_mb_vertex(mb, V3(x0, H, z), n, V2(x0 * .45f, H * .45f)), q1 = lu_mb_vertex(mb, V3(x1, H, z), n, V2(x1 * .45f, H * .45f)), q2 = lu_mb_vertex(mb, V3(cxm, H + rise, z), n, V2(cxm * .45f, (H + rise) * .45f)); if (s) lu_mb_tri(mb, q0, q1, q2); else lu_mb_tri(mb, q0, q2, q1);
            tri_phys(c, V3(x0, H, z), V3(x1, H, z), V3(cxm, H + rise, z), LU_SURF_STONE); }
    }
    chimney(c, V3(lu_lerpf(x0, x1, .7f), H + rise * .4f, lu_lerpf(z0, z1, .4f)), 1.8f, C3(150, 90, 70));
}
void lu_city_building(lu_city *c, const lu_building *B) {
    lu_rng rng = {B->seed * 977u + 13u};
    float H = B->ground_h + (B->floors - 1) * B->floor_h;
    /* объём для столкновений: стены — стена (лазать можно везде, кроме стекла витрин) */
    lu_phys_add_box(c->phys, (aabb){V3(B->x0, B->base_y, B->z0), V3(B->x1, B->base_y + H + .2f, B->z1)}, B->wall_mat == LU_CM_BRICK ? 0 : LU_SURF_STONE);
    for (int s = 0; s < 4; s++) facade(c, B, s, &rng);
    roof(c, B, &rng);
    c->nbuildings++;
}
void lu_city_lamp(lu_city *c, v3 p) {
    lu_mesh_builder *mb = lu_city_mb(c, p, LU_CM_IRON); mb->color = C3(28, 28, 32);
    lu_mb_cylinder(mb, p, .09f, .06f, .5f, 10, 1); lu_mb_cylinder(mb, V3(p.x, p.y + .5f, p.z), .05f, .04f, 3.2f, 8, 0);
    lu_mb_cylinder(mb, V3(p.x, p.y + 3.7f, p.z), .2f, .12f, .12f, 8, 1);
    lu_mb_cylinder(mb, V3(p.x, p.y + 4.25f, p.z), .02f, .22f, .25f, 8, 1);
    lu_mesh_builder *gl = lu_city_mb(c, p, LU_CM_LAMP); gl->color = 0xFFFFFFFFu; lu_mb_cylinder(gl, V3(p.x, p.y + 3.82f, p.z), .16f, .2f, .43f, 8, 0);
    lu_phys_add_box(c->phys, (aabb){V3(p.x - .08f, p.y, p.z - .08f), V3(p.x + .08f, p.y + 4.3f, p.z + .08f)}, LU_SURF_METAL);
    v3 lp = V3(p.x, p.y + 4.f, p.z); lu_vec_push(c->lamps, lp);
}
void lu_city_tree(lu_city *c, v3 p, float h, uint32_t seed) {
    lu_rng r = {seed * 31u + 5u};
    lu_mesh_builder *mb = lu_city_mb(c, p, LU_CM_BARK); mb->color = C3(110, 90, 75);
    lu_mb_cylinder(mb, p, .22f, .13f, h * .55f, 10, 0);
    for (int i = 0; i < 3; i++) { float a = i * 2.1f + rng_f(&r); v3 b0 = V3(p.x, p.y + h * (.35f + .1f * i), p.z); lu_mb_tube(mb, b0, V3(b0.x + sinf(a) * 1.2f, b0.y + 1.4f, b0.z + cosf(a) * 1.2f), .09f, .04f, 6); }
    lu_mesh_builder *lv = lu_city_mb(c, p, LU_CM_LEAVES);
    for (int i = 0; i < 9; i++) {
        float a = rng_f(&r) * LU_TAU, rr = rng_range(&r, .2f, 1.6f), y = p.y + h * rng_range(&r, .55f, .95f), s = rng_range(&r, 1.0f, 1.7f);
        int start = lv->v.n; lv->color = C3(70 + (int)(rng_f(&r) * 40), 105 + (int)(rng_f(&r) * 40), 45);
        lu_mb_sphere(lv, V3(p.x + sinf(a) * rr, y, p.z + cosf(a) * rr), V3(s, s * .8f, s), 9, 6);
        for (int k = start; k < lv->v.n; k++) { lv->v.v[k].color = (lv->v.v[k].color & 0x00FFFFFFu) | 0xB0000000u; }  /* альфа вершины = гибкость для ветра */
    }
    lu_phys_add_box(c->phys, (aabb){V3(p.x - .2f, p.y, p.z - .2f), V3(p.x + .2f, p.y + h * .55f, p.z + .2f)}, LU_SURF_WOOD);
}
void lu_city_haycart(lu_city *c, v3 p, float yaw) {
    m4 M = m4_trs(p, quat_axis(V3(0, 1, 0), yaw), V3(1, 1, 1));
    lu_mesh_builder *mb = lu_city_mb(c, p, LU_CM_WOOD); int s = mb->v.n; mb->color = C3(130, 95, 65);
    lu_mb_box(mb, V3(-1.1f, .55f, -.75f), V3(1.1f, .7f, .75f), .5f);
    lu_mb_box(mb, V3(-1.1f, .7f, -.8f), V3(1.1f, 1.0f, -.72f), .5f); lu_mb_box(mb, V3(-1.1f, .7f, .72f), V3(1.1f, 1.0f, .8f), .5f);
    for (int i = 0; i < 2; i++) { int w0 = mb->v.n; lu_mb_cylinder(mb, V3(0, 0, 0), .5f, .5f, .1f, 16, 1); lu_mb_transform(mb, w0, m4_trs(V3(0, .5f, i ? .85f : -.95f), quat_axis(V3(1, 0, 0), LU_PI / 2), V3(1, 1, 1))); }
    lu_mb_tube(mb, V3(1.1f, .65f, -.4f), V3(2.6f, .45f, -.3f), .05f, .05f, 6); lu_mb_tube(mb, V3(1.1f, .65f, .4f), V3(2.6f, .45f, .3f), .05f, .05f, 6);
    lu_mb_transform(mb, s, M);
    lu_mesh_builder *hy = lu_city_mb(c, p, LU_CM_DIRT); s = hy->v.n; hy->color = C3(220, 185, 95);
    lu_mb_sphere(hy, V3(0, .95f, 0), V3(1.2f, .55f, .8f), 14, 8); lu_mb_transform(hy, s, M);
    v3 a = m4_point(M, V3(-1.1f, .5f, -.8f)), b = m4_point(M, V3(1.1f, 1.45f, .8f));
    lu_phys_add_box(c->phys, (aabb){v3_min(a, b), v3_max(a, b)}, LU_SURF_SOFT);
    lu_vec_push(c->hay, V4(p.x, p.y + 1.45f, p.z, 1.2f));
}
void lu_city_stall(lu_city *c, v3 p, float yaw, uint32_t aw) {
    m4 M = m4_trs(p, quat_axis(V3(0, 1, 0), yaw), V3(1, 1, 1));
    lu_mesh_builder *mb = lu_city_mb(c, p, LU_CM_WOOD); int s = mb->v.n; mb->color = C3(140, 105, 70);
    lu_mb_box(mb, V3(-1.2f, 0, -.6f), V3(1.2f, .9f, .6f), .5f);
    for (int i = 0; i < 4; i++) lu_mb_box(mb, V3((i & 1) ? 1.1f : -1.2f, 0, (i & 2) ? .5f : -.7f), V3((i & 1) ? 1.2f : -1.1f, 2.3f, (i & 2) ? .6f : -.6f), .5f);
    lu_mb_transform(mb, s, M);
    lu_mesh_builder *cl = lu_city_mb(c, p, LU_CM_CLOTH); s = cl->v.n;
    v3 q[4] = {V3(-1.35f, 2.35f, -.85f), V3(1.35f, 2.35f, -.85f), V3(1.35f, 2.1f, .85f), V3(-1.35f, 2.1f, .85f)};
    for (int k = 0; k < 4; k++) q[k] = m4_point(M, q[k]);
    quad(cl, q[0], q[1], q[2], q[3], V2(0, 0), V2(3, 0), V2(3, 1), V2(0, 1), aw); quad(cl, q[1], q[0], q[3], q[2], V2(0, 0), V2(3, 0), V2(3, 1), V2(0, 1), aw);
    LU_UNUSED(s);
    lu_mesh_builder *fr = lu_city_mb(c, p, LU_CM_TRIM); s = fr->v.n;
    lu_rng r = {(uint32_t)(p.x * 13 + p.z * 7)};
    for (int i = 0; i < 18; i++) { fr->color = rng_f(&r) < .5f ? C3(200, 60, 40) : rng_f(&r) < .5f ? C3(230, 180, 50) : C3(90, 150, 60); lu_mb_sphere(fr, V3(rng_range(&r, -1.f, 1.f), .98f, rng_range(&r, -.45f, .45f)), V3(.08f, .08f, .08f), 6, 4); }
    lu_mb_transform(fr, s, M);
    v3 a = m4_point(M, V3(-1.2f, 0, -.6f)), b = m4_point(M, V3(1.2f, .9f, .6f));
    lu_phys_add_box(c->phys, (aabb){v3_min(a, b), v3_max(a, b)}, LU_SURF_WOOD);
}
void lu_city_fountain(lu_city *c, v3 p, float r) {
    lu_mesh_builder *mb = lu_city_mb(c, p, LU_CM_STONE); mb->color = C3(225, 215, 195);
    v2 rim[6] = {{r, 0}, {r + .1f, .1f}, {r + .1f, .55f}, {r - .05f, .65f}, {r - .25f, .65f}, {r - .3f, .2f}};
    lu_mb_lathe(mb, p, rim, 6, 40);
    v2 col[7] = {{.6f, .2f}, {.45f, .5f}, {.3f, 1.4f}, {.9f, 1.6f}, {.95f, 1.75f}, {.25f, 1.9f}, {.15f, 2.8f}};
    lu_mb_lathe(mb, p, col, 7, 24);
    lu_mesh_builder *w = lu_city_mb(c, p, LU_CM_WATER); w->color = 0xFFFFFFFFu;
    int base = lu_mb_vertex(w, V3(p.x, p.y + .5f, p.z), V3(0, 1, 0), V2(0, 0));
    for (int i = 0; i <= 32; i++) { float a = LU_TAU * i / 32; lu_mb_vertex(w, V3(p.x + sinf(a) * (r - .25f), p.y + .5f, p.z + cosf(a) * (r - .25f)), V3(0, 1, 0), V2(sinf(a), cosf(a))); }
    for (int i = 0; i < 32; i++) lu_mb_tri(w, base, base + 1 + i, base + 2 + i);
    /* столкновения: кольцо из коробок */
    for (int i = 0; i < 12; i++) { float a = LU_TAU * i / 12; v3 q = V3(p.x + sinf(a) * r, p.y, p.z + cosf(a) * r); lu_phys_add_obb(c->phys, m4_trs(V3(q.x, p.y + .32f, q.z), quat_axis(V3(0, 1, 0), a), V3(1, 1, 1)), V3(r * .3f, .33f, .2f), LU_SURF_STONE); }
    lu_phys_add_box(c->phys, (aabb){V3(p.x - .5f, p.y, p.z - .5f), V3(p.x + .5f, p.y + 2.8f, p.z + .5f)}, LU_SURF_STONE);
}
void lu_city_crates(lu_city *c, v3 p, int n, uint32_t seed) {
    lu_rng r = {seed};
    for (int i = 0; i < n; i++) {
        float s = rng_range(&r, .7f, 1.f); v3 q = V3(p.x + rng_range(&r, -.4f, .4f) + (i % 2) * 1.0f, p.y + (i / 2) * .95f, p.z + rng_range(&r, -.3f, .3f));
        if (i >= 2) { q.x = p.x + .5f; q.y = p.y + .95f; }
        lu_city_box(c, LU_CM_WOOD, (aabb){V3(q.x - s * .5f, q.y, q.z - s * .5f), V3(q.x + s * .5f, q.y + s * .95f, q.z + s * .5f)}, 1.f / s, C3(170, 130, 90), LU_SURF_WOOD);
    }
}
void lu_city_finish(lu_city *c) {
    int n = c->cw * c->ch * LU_CM_N; c->mesh = lu_alloc(sizeof(lu_mesh) * (size_t)n);
    for (int i = 0; i < n; i++) {
        lu_mesh_builder *b = &c->b[i]; if (!b->v.n) continue;
        lu_mb_compute_tangents(b); c->mesh[i] = lu_mesh_create(b, 0); c->tris += b->idx.n / 3; lu_mb_free(b); lu_mb_init(b);
    }
    lu_phys_build(c->phys, 4.f); c->built = 1;
    LU_INFO("город: %d домов, %ld треугольников, %d фонарей", c->nbuildings, c->tris, c->lamps.n);
}
void lu_city_draw(const lu_city *c, const lu_material *mats) {
    int n = c->cw * c->ch * LU_CM_N;
    for (int i = 0; i < n; i++) if (c->mesh[i].vao) lu_draw(&c->mesh[i], &mats[i % LU_CM_N], 1, m4_id());
}
void lu_city_materials(lu_material m[LU_CM_N], lu_texture tex[LU_CM_N][3], int ts) {
    static const struct { int tg; float c0[3], c1[3], var, rough, uv; int flags; } D[LU_CM_N] = {
        {LU_TG_STONE_BLOCKS, {.62f, .56f, .45f}, {.74f, .68f, .56f}, .12f, .8f, 1, LU_MAT_VCOLOR},       /* камень (известняк) */
        {LU_TG_PLASTER, {.7f, .66f, .58f}, {.8f, .76f, .68f}, .1f, .9f, 1, LU_MAT_VCOLOR},               /* штукатурка */
        {LU_TG_BRICK, {.42f, .17f, .1f}, {.55f, .25f, .15f}, .15f, .85f, 1, LU_MAT_VCOLOR},              /* кирпич */
        {LU_TG_SLATE, {.13f, .15f, .19f}, {.2f, .22f, .27f}, .2f, .5f, 1, 0},                           /* шифер */
        {LU_TG_ROOF_TILES, {.45f, .17f, .08f}, {.6f, .27f, .13f}, .2f, .7f, 1, LU_MAT_VCOLOR},           /* черепица */
        {LU_TG_WOOD, {.35f, .23f, .14f}, {.5f, .34f, .2f}, .15f, .75f, 1, LU_MAT_VCOLOR},                /* дерево */
        {LU_TG_SHUTTER, {.8f, .8f, .8f}, {.9f, .9f, .9f}, .05f, .7f, 1, LU_MAT_VCOLOR},                  /* ставни (цвет в вершинах) */
        {LU_TG_METAL, {.05f, .05f, .06f}, {.08f, .08f, .09f}, .1f, .45f, 1, LU_MAT_VCOLOR},              /* железо */
        {-1, {0}, {0}, 0, .08f, 1, LU_MAT_VCOLOR},                                                       /* стекло */
        {LU_TG_COBBLE, {.3f, .29f, .27f}, {.45f, .43f, .4f}, .2f, .8f, .5f, 0},                          /* булыжник */
        {LU_TG_FLAGSTONE, {.5f, .47f, .42f}, {.62f, .58f, .52f}, .1f, .8f, .35f, 0},                     /* плиты */
        {LU_TG_CLOTH, {.9f, .9f, .9f}, {1, 1, 1}, .05f, .95f, 2, LU_MAT_VCOLOR | LU_MAT_TWOSIDED},       /* ткань */
        {-1, {0}, {0}, 0, .6f, 1, LU_MAT_VCOLOR},                                                        /* крашеное дерево */
        {LU_TG_DIRT, {.6f, .55f, .45f}, {.75f, .7f, .55f}, .2f, .95f, 1, LU_MAT_VCOLOR},                 /* земля / сено */
        {-1, {0}, {0}, 0, .7f, 1, LU_MAT_VCOLOR | LU_MAT_WIND | LU_MAT_TWOSIDED},                        /* листва */
        {LU_TG_WOOD, {.25f, .2f, .16f}, {.35f, .28f, .22f}, .1f, .9f, 2, LU_MAT_VCOLOR},                 /* кора */
        {-1, {0}, {0}, 0, .3f, 1, LU_MAT_UNLIT},                                                         /* стекло фонаря */
        {-1, {0}, {0}, 0, .05f, 1, LU_MAT_WATER | LU_MAT_BLEND},                                         /* вода */
    };
    for (int i = 0; i < LU_CM_N; i++) {
        m[i] = lu_material_default(); m[i].flags = D[i].flags; m[i].roughness = D[i].rough; m[i].uv_scale = D[i].uv;
        if (D[i].tg >= 0) {
            lu_texset t; lu_palette p = {V3(D[i].c0[0], D[i].c0[1], D[i].c0[2]), V3(D[i].c1[0], D[i].c1[1], D[i].c1[2]), D[i].var};
            lu_texgen(&t, D[i].tg, ts, p, 1000u + (uint32_t)i * 17u);
            lu_texset_upload(&t, &tex[i][0], &tex[i][1], &tex[i][2]); lu_texset_free(&t);
            m[i].albedo = &tex[i][0]; m[i].normal = &tex[i][1]; m[i].orm = &tex[i][2]; m[i].roughness = 1;
        }
    }
    m[LU_CM_LAMP].color = V4(1, .8f, .5f, 1); m[LU_CM_LAMP].emissive = V3(4, 2.6f, 1.2f);
    m[LU_CM_WATER].color = V4(.12f, .2f, .22f, .75f);
    m[LU_CM_GLASS].metallic = .2f;
    m[LU_CM_SHUTTER].color = V4(1, 1, 1, 1);
}
