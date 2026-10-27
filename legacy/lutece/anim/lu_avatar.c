/* Lutèce Engine — генератор персонажей (тело, лицо, причёска, одежда 1789 года) со сглаженной привязкой к костям */
#include "lu_avatar.h"
#include <stdio.h>

static quat RX(float a) { return quat_axis(V3(1, 0, 0), a); }
static quat RY(float a) { return quat_axis(V3(0, 1, 0), a); }
static quat RZ(float a) { return quat_axis(V3(0, 0, 1), a); }
#define C3(r, g, b) ((uint32_t)(0xFF000000u | ((uint32_t)(b) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(r)))

typedef struct {
    lu_mesh_builder mb[LU_AV_NMAT];
    xform M[LU_MAX_BONES];         /* модельные преобразования позы привязки */
    const lu_skeleton *s; const lu_humanoid *h; const lu_outfit *o;
    int det; float k;
} Av;

static v3 jp(Av *a, int j) { return a->M[j].t; }
/* вершина с весами до трёх костей */
static int vtx(Av *a, int mat, v3 p, v3 n, v2 uv, uint32_t col, int j0, float w0, int j1, float w1, int j2, float w2) {
    lu_mesh_builder *b = &a->mb[mat]; b->color = col;
    int i = lu_mb_vertex(b, p, n, uv); lu_vertex *v = &b->v.v[i];
    float s = w0 + w1 + w2; if (s <= 0) { w0 = 1; s = 1; }
    if (j1 < 0) w1 = 0;
    if (j2 < 0) w2 = 0;
    s = w0 + w1 + w2;
    int js[3] = {j0, j1 < 0 ? 0 : j1, j2 < 0 ? 0 : j2}; float ws[3] = {w0 / s, w1 / s, w2 / s};
    int tot = 0;
    for (int k = 0; k < 3; k++) { v->joints[k] = (uint8_t)js[k]; v->weights[k] = (uint8_t)(ws[k] * 255.f + .5f); tot += v->weights[k]; }
    v->joints[3] = 0; v->weights[3] = 0; v->weights[0] = (uint8_t)(v->weights[0] + (255 - tot)); /* сумма ровно 255 */
    return i;
}
static uint32_t shade(uint32_t c, float k) {
    int r = (int)((c & 255) * k), g = (int)(((c >> 8) & 255) * k), b = (int)(((c >> 16) & 255) * k);
    return C3(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b);
}
/* сетка «кольца × сегменты» по функции точки */
typedef v3 (*ring_fn)(Av *a, float u, float t, const void *par, int *j0, float *w0, int *j1, float *w1, int *j2, float *w2, uint32_t *col);
static void grid(Av *a, int mat, int rings, int seg, ring_fn fn, const void *par, int closed_u) {
    lu_mesh_builder *b = &a->mb[mat]; int base = b->v.n;
    for (int r = 0; r <= rings; r++) for (int s = 0; s <= seg; s++) {
        float u = (float)r / rings, t = (float)s / seg; int j0 = 0, j1 = -1, j2 = -1; float w0 = 1, w1 = 0, w2 = 0; uint32_t col = 0xFFFFFFFFu;
        v3 p = fn(a, u, t, par, &j0, &w0, &j1, &w1, &j2, &w2, &col);
        vtx(a, mat, p, V3(0, 1, 0), V2(t, u), col, j0, w0, j1, w1, j2, w2);
    }
    for (int r = 0; r < rings; r++) for (int s = 0; s < seg; s++) {
        int i0 = base + r * (seg + 1) + s, i1 = i0 + 1, i2 = i0 + seg + 1, i3 = i2 + 1;
        lu_mb_tri(b, i0, i2, i1); lu_mb_tri(b, i1, i2, i3);
    }
    LU_UNUSED(closed_u);
    lu_mb_compute_normals(b, base);
}

/* ---------- конечность: эллиптическая труба вдоль кости ---------- */
typedef struct { int bone, parent, child; v3 a, c; float r0, r1, ex, ez, bulge, bl0, bl1; uint32_t col0, col1; float col_split; v3 front; } Limb;
static v3 limb_fn(Av *a, float u, float t, const void *par, int *j0, float *w0, int *j1, float *w1, int *j2, float *w2, uint32_t *col) {
    LU_UNUSED(a); const Limb *L = par;
    v3 ax = v3_sub(L->c, L->a); float len = v3_len(ax); v3 d = v3_scale(ax, 1.f / len);
    v3 f = v3_norm(v3_reject(L->front, d)), sd = v3_cross(d, f);
    float r = lu_lerpf(L->r0, L->r1, u) * (1 + L->bulge * sinf(LU_PI * lu_clampf(u * 1.3f, 0, 1)));
    float an = LU_TAU * t; v3 o = v3_add(v3_scale(sd, cosf(an) * r * L->ex), v3_scale(f, sinf(an) * r * L->ez));
    *j0 = L->bone; *w0 = 1; *j1 = L->parent; *w1 = 0; *j2 = L->child; *w2 = 0;
    if (L->parent >= 0 && u < L->bl0) *w1 = .5f * (1 - u / L->bl0);
    if (L->child >= 0 && u > 1 - L->bl1) *w2 = .5f * (u - (1 - L->bl1)) / L->bl1;
    *col = u < L->col_split ? L->col0 : L->col1;
    return v3_add(v3_mad(L->a, d, u * len), o);
}
static void limb(Av *a, int mat, int bone, int parent, int child, v3 p0, v3 p1, float r0, float r1, float ex, float ez, float bulge, uint32_t c0, uint32_t c1, float split, v3 front) {
    Limb L = {bone, parent, child, p0, p1, r0, r1, ex, ez, bulge, .22f, .22f, c0, c1, split, front};
    grid(a, mat, a->det ? 10 : 6, a->det ? 16 : 10, limb_fn, &L, 0);
}
/* ---------- эллипсоид, привязанный к одной-двум костям ---------- */
static void blob(Av *a, int mat, v3 c, v3 r, quat q, uint32_t col, int j0, int j1, float w1, int seg, int ring) {
    lu_mesh_builder *b = &a->mb[mat]; int base = b->v.n;
    for (int j = 0; j <= ring; j++) {
        float th = LU_PI * j / ring, st = sinf(th), ct = cosf(th);
        for (int i = 0; i <= seg; i++) {
            float ph = LU_TAU * i / seg; v3 n = V3(st * sinf(ph), ct, st * cosf(ph));
            v3 p = v3_add(c, quat_rot(q, V3(n.x * r.x, n.y * r.y, n.z * r.z)));
            vtx(a, mat, p, quat_rot(q, v3_norm(V3(n.x / r.x, n.y / r.y, n.z / r.z))), V2((float)i / seg, (float)j / ring), col, j0, 1 - w1, j1, w1, -1, 0);
        }
    }
    for (int j = 0; j < ring; j++) for (int i = 0; i < seg; i++) { int i0 = base + j * (seg + 1) + i, i2 = i0 + seg + 1; lu_mb_tri(b, i0, i2, i0 + 1); lu_mb_tri(b, i0 + 1, i2, i2 + 1); }
}

/* ---------- туловище: кольца по высоте, веса по позвонкам ---------- */
typedef struct { float y, hw, hd, z; } Ring;
typedef struct { const Ring *r; int n; uint32_t col_lo, col_hi; float split_y; float female; } Torso;
static void spine_weights(Av *a, float y, int *j0, float *w0, int *j1, float *w1) {
    int ch[5] = {a->h->hips, a->h->spine, a->h->spine1, a->h->spine2, a->h->neck}; float ys[5];
    for (int i = 0; i < 5; i++) ys[i] = jp(a, ch[i]).y;
    ys[0] -= .05f * a->k;
    if (y <= ys[0]) { *j0 = ch[0]; *w0 = 1; *j1 = -1; *w1 = 0; return; }
    for (int i = 0; i < 4; i++) if (y <= ys[i + 1]) { float t = (y - ys[i]) / (ys[i + 1] - ys[i]); *j0 = ch[i]; *w0 = 1 - t; *j1 = ch[i + 1]; *w1 = t; return; }
    *j0 = ch[4]; *w0 = 1; *j1 = -1; *w1 = 0;
}
static v3 torso_fn(Av *a, float u, float t, const void *par, int *j0, float *w0, int *j1, float *w1, int *j2, float *w2, uint32_t *col) {
    const Torso *T = par; float f = u * (T->n - 1); int i = (int)f; if (i >= T->n - 1) i = T->n - 2; float s = f - i;
    s = s * s * (3 - 2 * s);
    Ring r0 = T->r[i], r1 = T->r[i + 1];
    float y = lu_lerpf(r0.y, r1.y, s) * a->k, hw = lu_lerpf(r0.hw, r1.hw, s) * a->k, hd = lu_lerpf(r0.hd, r1.hd, s) * a->k, z = lu_lerpf(r0.z, r1.z, s) * a->k;
    float an = LU_TAU * t, sx = sinf(an), cz = cosf(an);
    /* сечение: скруглённый прямоугольник (суперэллипс) */
    float px = copysignf(powf(fabsf(sx), .75f), sx) * hw, pz = copysignf(powf(fabsf(cz), .8f), cz) * hd;
    if (cz < 0) pz *= 1.05f; /* спина чуть площе снизу */
    *j2 = -1; *w2 = 0; spine_weights(a, y, j0, w0, j1, w1);
    *col = y < T->split_y * a->k ? T->col_lo : T->col_hi;
    return V3(px, y, pz + z);
}

/* ---------- юбка/полы кафтана: оболочка с вырезом спереди, нижняя часть следует за бёдрами ---------- */
typedef struct { float y0, y1, r_top, r_bot, depth, gap_top, gap_bot, back_slit; uint32_t col, trim; } Skirt;
static v3 skirt_fn(Av *a, float u, float t, const void *par, int *j0, float *w0, int *j1, float *w1, int *j2, float *w2, uint32_t *col) {
    const Skirt *S = par; float y = lu_lerpf(S->y0, S->y1, u) * a->k;
    float gap = lu_lerpf(S->gap_top, S->gap_bot, u);
    float an = gap * .5f + (LU_TAU - gap) * t;          /* угол от переда (+Z), по кругу */
    float r = lu_lerpf(S->r_top, S->r_bot, powf(u, 1.3f)) * a->k;
    float x = sinf(an) * r, z = cosf(an) * r * S->depth;
    *col = u > .96f ? S->trim : S->col;
    float hip_y = jp(a, a->h->hips).y;
    int side = x >= 0 ? 0 : 1; float down = lu_clampf((hip_y - .05f * a->k - y) / (.45f * a->k), 0, 1);
    float sideness = fabsf(sinf(an)) * (cosf(an) > -.3f ? 1.f : .5f);
    *j0 = a->h->hips; *w0 = 1; *j1 = a->h->thigh[side]; *w1 = down * .75f * sideness; *j2 = a->h->spine; *w2 = y > hip_y ? lu_clampf((y - hip_y) / (.2f * a->k), 0, 1) : 0;
    return V3(x, y, z - .01f * a->k);
}

/* ---------- шапочка волос / капюшон: часть эллипсоида с вырезом лица ---------- */
typedef struct { v3 c, r; float front_cut, back_cut, peak; uint32_t col; int hood; } Cap;
static v3 cap_fn(Av *a, float u, float t, const void *par, int *j0, float *w0, int *j1, float *w1, int *j2, float *w2, uint32_t *col) {
    const Cap *C = par; float ph = LU_TAU * t; float front = (cosf(ph) + 1) * .5f; /* 1 — спереди */
    float thmax = lu_lerpf(C->back_cut, C->front_cut, powf(front, 1.5f));
    float th = thmax * u, st = sinf(th), ct = cosf(th);
    v3 n = V3(st * sinf(ph), ct, st * cosf(ph));
    v3 p = v3_add(C->c, V3(n.x * C->r.x, n.y * C->r.y, n.z * C->r.z));
    if (C->hood) p.z += C->peak * front * u * u * .4f, p.y += C->peak * (1 - u) * front * .3f;
    *col = C->col; *j0 = a->h->head; *w0 = 1; *j1 = a->h->neck; *w1 = C->hood ? lu_clampf((jp(a, a->h->head).y - p.y) / (.15f * a->k), 0, .8f) : 0; *j2 = -1; *w2 = 0;
    return p;
}

void lu_outfit_default(lu_outfit *o, int female) {
    memset(o, 0, sizeof *o); o->female = female; o->height = female ? 1.66f : 1.78f; o->build = .4f;
    o->skin = C3(232, 190, 160); o->hair = C3(70, 50, 38); o->eyes = C3(70, 90, 110);
    o->shirt = C3(236, 232, 222); o->vest = C3(120, 40, 40); o->coat_color = C3(40, 50, 90); o->coat_trim = C3(200, 170, 90);
    o->pants = C3(70, 60, 50); o->stockings = C3(230, 228, 220); o->shoes = C3(30, 26, 24); o->belt = C3(90, 60, 40); o->hat = C3(30, 28, 32);
    o->coat = female ? LU_COAT_NONE : LU_COAT_LONG; o->hair_style = female ? LU_HAIR_BUN : LU_HAIR_QUEUE; o->skirt = female;
}
void lu_outfit_random(lu_outfit *o, uint32_t seed) {
    lu_rng r = {seed * 2654435761u + 7};
    int female = rng_u32(&r) % 3 == 0; lu_outfit_default(o, female); o->seed = seed;
    static const uint32_t SK[6] = {C3(240, 205, 180), C3(232, 190, 160), C3(214, 170, 135), C3(190, 145, 110), C3(150, 105, 75), C3(110, 75, 55)};
    static const uint32_t HR[7] = {C3(30, 24, 22), C3(70, 50, 38), C3(110, 75, 45), C3(160, 120, 70), C3(200, 170, 120), C3(150, 60, 30), C3(200, 200, 200)};
    static const uint32_t CL[12] = {C3(40, 50, 90), C3(90, 40, 40), C3(50, 70, 50), C3(110, 90, 60), C3(70, 60, 80), C3(30, 30, 34), C3(150, 120, 80), C3(120, 110, 100), C3(160, 60, 40), C3(60, 90, 110), C3(200, 190, 170), C3(90, 70, 50)};
    o->skin = SK[rng_u32(&r) % 6]; o->hair = HR[rng_u32(&r) % 7]; o->height = female ? rng_range(&r, 1.55f, 1.72f) : rng_range(&r, 1.64f, 1.88f);
    o->build = rng_f(&r); o->coat = female ? LU_COAT_NONE : (int)(rng_u32(&r) % 3);
    o->coat_color = CL[rng_u32(&r) % 12]; o->coat_trim = shade(o->coat_color, 1.35f); o->vest = CL[rng_u32(&r) % 12]; o->pants = CL[rng_u32(&r) % 12];
    o->shirt = shade(C3(236, 230, 215), rng_range(&r, .85f, 1.f));
    o->hat_type = (int)(rng_u32(&r) % 4);
    if (o->hat_type == LU_HAT_BONNET && !female) o->hat_type = LU_HAT_CAP;
    if (female && o->hat_type == LU_HAT_TRICORN) o->hat_type = LU_HAT_BONNET;
    o->hair_style = female ? (rng_u32(&r) % 2 ? LU_HAIR_BUN : LU_HAIR_LONG) : (int)(rng_u32(&r) % 3);
    o->apron = female && rng_u32(&r) % 2; o->beard = !female && rng_u32(&r) % 4 == 0;
    o->hat = CL[rng_u32(&r) % 12]; o->sash = rng_u32(&r) % 5 == 0; o->skirt = female;
}
static uint32_t coat_col(const lu_outfit *o) { return o->coat_color; }

void lu_avatar_materials(lu_material m[LU_AV_NMAT]) {
    for (int i = 0; i < LU_AV_NMAT; i++) { m[i] = lu_material_default(); m[i].flags = LU_MAT_VCOLOR; }
    m[LU_AV_SKIN].flags |= LU_MAT_SKIN; m[LU_AV_SKIN].roughness = .55f;
    m[LU_AV_CLOTH].roughness = .9f; m[LU_AV_CLOTH].flags |= LU_MAT_TWOSIDED;
    m[LU_AV_LEATHER].roughness = .45f;
    m[LU_AV_HAIR].roughness = .6f;
    m[LU_AV_EYE].roughness = .08f;
    m[LU_AV_METAL].metallic = 1; m[LU_AV_METAL].roughness = .3f;
}

void lu_avatar_build(const lu_outfit *o, lu_skeleton *s, lu_humanoid *h, lu_mesh *out, int det) {
    lu_humanoid_skeleton(s, h, o->height, o->female);
    s->tscale = o->height / 1.78f;   /* библиотека движений строится на эталонном скелете 1.78 м */
    Av A; memset(&A, 0, sizeof A); Av *a = &A; a->s = s; a->h = h; a->o = o; a->det = det; a->k = o->height / 1.78f;
    for (int i = 0; i < LU_AV_NMAT; i++) lu_mb_init(&a->mb[i]);
    lu_pose bp; lu_pose_bind(s, &bp); lu_pose_model(s, &bp, a->M);
    float k = a->k, bw = 1 + o->build * .18f, fem = o->female ? 1.f : 0.f;
    int long_coat = o->coat == LU_COAT_LONG || o->coat == LU_COAT_HOODED, jacket = o->coat == LU_COAT_JACKET;
    uint32_t upper = o->coat ? coat_col(o) : (o->female ? o->vest : o->shirt), sleeve = o->coat ? coat_col(o) : (o->female ? o->shirt : o->shirt), lower = o->pants;
    /* ---- туловище ---- */
    Ring R[10] = {{.86f, .165f * bw + .02f * fem, .12f * bw, -.01f}, {.95f, .17f * bw + .03f * fem, .125f * bw, -.01f}, {1.05f, .15f * bw - .02f * fem, .11f * bw, 0},
                  {1.15f, .155f * bw - .01f * fem, .112f * bw, .005f}, {1.25f, .17f * bw, .125f * bw + .015f * fem, .01f}, {1.34f, .188f * bw, .122f * bw, .01f},
                  {1.41f, .19f * bw - .015f * fem, .108f * bw, .0f}, {1.455f, .13f, .085f, -.005f}, {1.49f, .07f, .065f, .0f}, {1.5f, .055f, .055f, .0f}};
    Torso T = {R, 10, lower, upper, o->female ? .99f : 1.0f, fem};
    grid(a, LU_AV_CLOTH, det ? 22 : 14, det ? 28 : 18, torso_fn, &T, 0);
    if (o->female) { for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f; blob(a, LU_AV_CLOTH, V3(sg * .065f * k, 1.27f * k, .1f * k), V3(.07f * k, .065f * k, .06f * k), Q_ID(), upper, h->spine2, h->spine1, .4f, 12, 8); } }
    /* пуговицы, пояс, кушак */
    for (int i = 0; i < 6 && !o->female; i++) blob(a, LU_AV_METAL, V3(.0f, (1.36f - i * .07f) * k, .125f * k * bw + .005f), V3(.008f, .008f, .005f), Q_ID(), C3(220, 190, 110), h->spine1, h->spine2, .5f, 6, 4);
    if (o->sash || !o->female) { Ring BR[3] = {{1.0f, .158f * bw, .118f * bw, 0}, {1.03f, .16f * bw, .12f * bw, 0}, {1.06f, .155f * bw, .114f * bw, 0}};
        Torso B = {BR, 3, o->sash ? C3(170, 30, 40) : o->belt, o->sash ? C3(170, 30, 40) : o->belt, 0, 0}; grid(a, LU_AV_LEATHER, 3, 24, torso_fn, &B, 0); }
    /* ---- шея, голова, лицо ---- */
    v3 neck = jp(a, h->neck), head = jp(a, h->head);
    limb(a, LU_AV_SKIN, h->neck, h->spine2, h->head, v3_add(neck, V3(0, -.04f * k, 0)), v3_add(head, V3(0, .03f * k, 0)), .052f * k * (1 - .1f * fem), .048f * k, 1, 1, 0, o->skin, o->skin, 2, V3(0, 0, 1));
    v3 hc = v3_add(head, V3(0, .09f * k, .015f * k)); float hs = k * (1 - .04f * fem);
    blob(a, LU_AV_SKIN, hc, V3(.082f * hs, .112f * hs, .1f * hs), Q_ID(), o->skin, h->head, -1, 0, det ? 28 : 16, det ? 20 : 12);
    blob(a, LU_AV_SKIN, v3_add(hc, V3(0, -.065f * hs, .035f * hs)), V3(.06f * hs, .045f * hs, .065f * hs), Q_ID(), o->skin, h->head, -1, 0, 16, 10); /* челюсть */
    blob(a, LU_AV_SKIN, v3_add(hc, V3(0, -.01f * hs, .1f * hs)), V3(.013f * hs, .03f * hs, .02f * hs), RX(-.25f), shade(o->skin, .97f), h->head, -1, 0, 10, 8);   /* нос */
    blob(a, LU_AV_SKIN, v3_add(hc, V3(0, -.033f * hs, .1f * hs)), V3(.02f * hs, .01f * hs, .015f * hs), Q_ID(), shade(o->skin, .97f), h->head, -1, 0, 10, 6);
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f;
        blob(a, LU_AV_SKIN, v3_add(hc, V3(sg * .082f * hs, 0, -.005f)), V3(.012f * hs, .026f * hs, .018f * hs), Q_ID(), shade(o->skin, .92f), h->head, -1, 0, 10, 8);   /* уши */
        v3 ec = v3_add(hc, V3(sg * .031f * hs, .018f * hs, .078f * hs));
        blob(a, LU_AV_EYE, ec, V3(.0135f * hs, .0115f * hs, .01f * hs), Q_ID(), C3(240, 238, 232), h->head, -1, 0, 12, 8);
        blob(a, LU_AV_EYE, v3_add(ec, V3(0, 0, .0082f * hs)), V3(.0078f * hs, .0078f * hs, .0032f * hs), Q_ID(), o->eyes, h->head, -1, 0, 10, 6);
        blob(a, LU_AV_EYE, v3_add(ec, V3(0, 0, .0105f * hs)), V3(.0034f * hs, .0034f * hs, .0012f * hs), Q_ID(), C3(12, 10, 12), h->head, -1, 0, 8, 4);
        blob(a, LU_AV_SKIN, v3_add(ec, V3(0, .0125f * hs, -.001f)), V3(.016f * hs, .0045f * hs, .011f * hs), RX(.15f), shade(o->skin, .9f), h->head, -1, 0, 10, 6); /* веко */
        blob(a, LU_AV_HAIR, v3_add(ec, V3(sg * .002f, .022f * hs, .004f * hs)), V3(.017f * hs, .0035f * hs, .006f * hs), RZ(sg * -.12f), shade(o->hair, .8f), h->head, -1, 0, 8, 4); /* бровь */
        blob(a, LU_AV_SKIN, v3_add(hc, V3(sg * .045f * hs, -.02f * hs, .07f * hs)), V3(.025f * hs, .02f * hs, .02f * hs), Q_ID(), shade(o->skin, o->female ? 1.02f : .99f), h->head, -1, 0, 8, 6); }
    blob(a, LU_AV_SKIN, v3_add(hc, V3(0, -.055f * hs, .088f * hs)), V3(.022f * hs, .007f * hs, .01f * hs), Q_ID(), o->female ? C3(190, 90, 100) : shade(o->skin, .8f), h->head, -1, 0, 10, 6); /* губы */
    if (o->beard) blob(a, LU_AV_HAIR, v3_add(hc, V3(0, -.075f * hs, .05f * hs)), V3(.058f * hs, .05f * hs, .06f * hs), Q_ID(), o->hair, h->head, -1, 0, 14, 8);
    /* ---- волосы, головной убор, капюшон ---- */
    if (o->hair_style != LU_HAIR_BALD) {
        Cap hcap = {v3_add(hc, V3(0, .006f * hs, -.006f * hs)), V3(.089f * hs, .12f * hs, .108f * hs), .95f, 2.1f, 0, o->hair, 0};
        if (o->hair_style == LU_HAIR_LONG) hcap.back_cut = 2.5f;
        grid(a, LU_AV_HAIR, det ? 12 : 8, det ? 24 : 16, cap_fn, &hcap, 0);
        if (o->hair_style == LU_HAIR_QUEUE) { v3 q0 = v3_add(hc, V3(0, -.04f * hs, -.1f * hs)); limb(a, LU_AV_HAIR, h->head, h->neck, -1, q0, v3_add(q0, V3(0, -.16f * k, -.02f)), .016f, .01f, 1, 1, 0, o->hair, o->hair, 2, V3(0, 0, 1));
            blob(a, LU_AV_CLOTH, q0, V3(.02f, .015f, .012f), Q_ID(), C3(20, 20, 24), h->head, -1, 0, 8, 6); }
        if (o->hair_style == LU_HAIR_BUN) blob(a, LU_AV_HAIR, v3_add(hc, V3(0, .05f * hs, -.09f * hs)), V3(.045f * hs, .04f * hs, .04f * hs), Q_ID(), o->hair, h->head, -1, 0, 12, 8);
        if (o->hair_style == LU_HAIR_LONG) for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f; v3 q0 = v3_add(hc, V3(sg * .07f * hs, -.02f * hs, -.04f * hs));
            limb(a, LU_AV_HAIR, h->head, h->neck, -1, q0, v3_add(q0, V3(sg * .02f, -.2f * k, -.03f)), .03f, .02f, 1, .6f, 0, o->hair, o->hair, 2, V3(0, 0, 1)); }
    }
    if (o->coat == LU_COAT_HOODED) {
        Cap hood = {v3_add(hc, V3(0, .012f * hs, -.012f * hs)), V3(.108f * hs, .14f * hs, .128f * hs), 1.25f, 2.6f, .06f * k, coat_col(o), 1};
        grid(a, LU_AV_CLOTH, 12, 28, cap_fn, &hood, 0);
    } else if (o->hat_type == LU_HAT_TRICORN) {
        lu_mesh_builder *b = &a->mb[LU_AV_CLOTH]; int base = b->v.n; v3 hb = v3_add(hc, V3(0, .075f * hs, -.005f));
        v2 crown[4] = {{.085f * hs, 0}, {.088f * hs, .05f * hs}, {.07f * hs, .08f * hs}, {0, .085f * hs}};
        lu_mb_lathe(b, hb, crown, 4, 16);
        for (int i = base; i < b->v.n; i++) { lu_vertex *v = &b->v.v[i]; v->color = o->hat; v->joints[0] = (uint8_t)h->head; v->weights[0] = 255; }
        base = b->v.n;
        for (int i = 0; i <= 24; i++) for (int r = 0; r < 2; r++) {   /* поля, загнутые с трёх сторон */
            float an = LU_TAU * i / 24, up = powf(fabsf(cosf(an * 1.5f)), 3.f), rr = (r ? .17f : .08f) * hs;
            v3 p = v3_add(hb, V3(sinf(an) * rr, r ? .02f + .07f * up * hs : .005f, cosf(an) * rr));
            vtx(a, LU_AV_CLOTH, p, V3(0, 1, 0), V2(0, 0), o->hat, h->head, 1, -1, 0, -1, 0);
        }
        for (int i = 0; i < 24; i++) { int q = base + i * 2; lu_mb_tri(b, q, q + 1, q + 3); lu_mb_tri(b, q, q + 3, q + 2); }
        lu_mb_compute_normals(b, base);
    } else if (o->hat_type == LU_HAT_BONNET) {
        Cap bn = {v3_add(hc, V3(0, .02f * hs, -.01f * hs)), V3(.1f * hs, .125f * hs, .12f * hs), 1.05f, 1.9f, 0, C3(244, 240, 230), 0};
        grid(a, LU_AV_CLOTH, 8, 24, cap_fn, &bn, 0);
    } else if (o->hat_type == LU_HAT_CAP) {
        Cap cp = {v3_add(hc, V3(0, .03f * hs, -.02f * hs)), V3(.095f * hs, .11f * hs, .115f * hs), .9f, 1.6f, 0, C3(170, 30, 40), 0};
        grid(a, LU_AV_CLOTH, 8, 24, cap_fn, &cp, 0);
    }
    /* ---- руки ---- */
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f;
        v3 sh = jp(a, h->arm[i]), el = jp(a, h->fore[i]), wr = jp(a, h->hand[i]), fi = jp(a, h->fingers[i]);
        blob(a, LU_AV_CLOTH, v3_add(sh, V3(-sg * .03f * k, -.004f, 0)), V3(.052f * k * bw, .05f * k, .056f * k), Q_ID(), sleeve, h->arm[i], h->clav[i], .5f, 14, 10);
        limb(a, LU_AV_CLOTH, h->arm[i], h->clav[i], h->fore[i], sh, el, .056f * k * bw, .044f * k, 1, 1.05f, .12f, sleeve, sleeve, 2, V3(0, 0, 1));
        limb(a, LU_AV_CLOTH, h->fore[i], h->arm[i], h->hand[i], el, v3_mad(wr, V3(sg, 0, 0), -.01f), .045f * k, .033f * k, 1.1f, 1, .08f, sleeve, sleeve, 2, V3(0, 1, 0));
        if (o->coat) limb(a, LU_AV_CLOTH, h->fore[i], h->arm[i], -1, v3_mad(wr, V3(sg, 0, 0), -.09f * k), v3_mad(wr, V3(sg, 0, 0), -.03f * k), .052f * k, .052f * k, 1, 1, 0, o->coat_trim, o->coat_trim, 2, V3(0, 1, 0)); /* обшлаг */
        limb(a, LU_AV_CLOTH, h->hand[i], h->fore[i], -1, v3_mad(wr, V3(sg, 0, 0), -.025f * k), v3_mad(wr, V3(sg, 0, 0), .01f * k), .036f * k, .042f * k, 1, .8f, 0, C3(244, 242, 236), C3(244, 242, 236), 2, V3(0, 1, 0)); /* манжета */
        blob(a, LU_AV_SKIN, v3_mad(wr, V3(sg, 0, 0), .045f * k), V3(.048f * k, .016f * k, .042f * k), Q_ID(), o->skin, h->hand[i], h->fore[i], .15f, 12, 8);
        blob(a, LU_AV_SKIN, v3_mad(fi, V3(sg, 0, 0), .035f * k), V3(.045f * k, .013f * k, .038f * k), Q_ID(), o->skin, h->fingers[i], h->hand[i], .2f, 12, 8);
        blob(a, LU_AV_SKIN, v3_add(jp(a, h->thumb[i]), V3(sg * .02f * k, -.005f, .025f * k)), V3(.014f * k, .013f * k, .03f * k), RY(sg * .5f), o->skin, h->thumb[i], h->hand[i], .25f, 8, 6);
    }
    /* ---- ноги ---- */
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f;
        v3 hp = jp(a, h->thigh[i]), kn = jp(a, h->shin[i]), an = jp(a, h->foot[i]), to = jp(a, h->toe[i]);
        float tr = (.09f + .02f * fem) * k * bw;
        limb(a, LU_AV_CLOTH, h->thigh[i], h->hips, h->shin[i], v3_add(hp, V3(sg * .005f, .04f * k, 0)), kn, tr, .058f * k, 1, 1.1f, .05f, lower, lower, 2, V3(0, 0, 1));
        uint32_t sock = o->female ? o->stockings : o->stockings;
        limb(a, LU_AV_CLOTH, h->shin[i], h->thigh[i], h->foot[i], kn, v3_add(an, V3(0, .02f, 0)), .057f * k, .04f * k, 1, 1.05f, .18f, lower, sock, .18f, V3(0, 0, 1));
        if (!o->female) blob(a, LU_AV_METAL, v3_add(kn, V3(sg * .055f * k, -.06f * k, .01f)), V3(.006f, .006f, .004f), Q_ID(), C3(220, 190, 110), h->shin[i], -1, 0, 6, 4);
        /* туфли с пряжкой */
        v3 fc = v3_lerp(an, v3_add(to, V3(0, 0, .03f * k)), .5f); fc.y = .045f * k;
        blob(a, LU_AV_LEATHER, fc, V3(.045f * k, .045f * k, .13f * k), Q_ID(), o->shoes, h->foot[i], h->toe[i], .3f, 14, 10);
        blob(a, LU_AV_LEATHER, v3_add(an, V3(0, -.01f, -.02f)), V3(.044f * k, .04f * k, .05f * k), Q_ID(), o->shoes, h->foot[i], h->shin[i], .2f, 10, 8);
        blob(a, LU_AV_METAL, v3_add(fc, V3(0, .04f * k, .03f * k)), V3(.02f * k, .006f, .015f * k), Q_ID(), C3(225, 225, 230), h->foot[i], -1, 0, 8, 4);
    }
    /* ---- полы кафтана, юбки, фартук ---- */
    if (long_coat) { Skirt S = {1.04f, .5f, .18f * bw, .27f * bw, .82f, .5f, 1.3f, 0, coat_col(o), o->coat_trim}; grid(a, LU_AV_CLOTH, 12, 28, skirt_fn, &S, 0); }
    else if (jacket) { Skirt S = {1.04f, .84f, .17f * bw, .2f * bw, .8f, .4f, .9f, 0, coat_col(o), o->coat_trim}; grid(a, LU_AV_CLOTH, 6, 24, skirt_fn, &S, 0); }
    if (o->skirt) { Skirt S = {1.04f, .06f, .16f, .42f, .92f, 0.001f, 0.001f, 0, o->pants, shade(o->pants, .8f)}; grid(a, LU_AV_CLOTH, 16, 32, skirt_fn, &S, 0);
        if (o->apron) { Skirt Ap = {1.0f, .3f, .175f, .28f, .98f, LU_TAU - 1.6f, LU_TAU - 1.8f, 0, C3(240, 236, 226), C3(240, 236, 226)}; grid(a, LU_AV_CLOTH, 8, 12, skirt_fn, &Ap, 0); } }
    /* ---- шпага в ножнах на левом бедре ---- */
    if (o->sword) { v3 hip = jp(a, h->hips); limb(a, LU_AV_LEATHER, h->hips, -1, -1, v3_add(hip, V3(.2f * k, -.02f, .05f)), v3_add(hip, V3(.26f * k, -.7f * k, -.25f)), .018f, .014f, 1, .5f, 0, C3(30, 24, 22), C3(30, 24, 22), 2, V3(1, 0, 0));
        blob(a, LU_AV_METAL, v3_add(hip, V3(.19f * k, .05f, .08f)), V3(.03f, .03f, .012f), Q_ID(), C3(220, 190, 110), h->hips, -1, 0, 10, 6); }
    /* ---- слияние частей в один меш (по материалам) ---- */
    lu_mesh_builder all; lu_mb_init(&all);
    for (int m = 0; m < LU_AV_NMAT; m++) {
        lu_mesh_builder *b = &a->mb[m]; if (!b->v.n) continue;
        lu_mb_material(&all, m); int base = all.v.n;
        for (int i = 0; i < b->v.n; i++) lu_vec_push(all.v, b->v.v[i]);
        for (int i = 0; i < b->idx.n; i++) lu_vec_push(all.idx, b->idx.v[i] + (uint32_t)base);
        lu_mb_free(b);
    }
    lu_mb_compute_tangents(&all);
    *out = lu_mesh_create(&all, 1);
    lu_mb_free(&all);
}
