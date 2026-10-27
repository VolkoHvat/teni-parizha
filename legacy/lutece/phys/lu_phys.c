/* Lutèce Engine — физика мира и сенсоры паркура */
#include "lu_phys.h"

/* ================= геометрия ================= */
static v3 closest_pt_tri(v3 p, v3 a, v3 b, v3 c) {          /* Эриксон, 5.1.5 */
    v3 ab = v3_sub(b, a), ac = v3_sub(c, a), ap = v3_sub(p, a);
    float d1 = v3_dot(ab, ap), d2 = v3_dot(ac, ap); if (d1 <= 0 && d2 <= 0) return a;
    v3 bp = v3_sub(p, b); float d3 = v3_dot(ab, bp), d4 = v3_dot(ac, bp); if (d3 >= 0 && d4 <= d3) return b;
    float vc = d1 * d4 - d3 * d2; if (vc <= 0 && d1 >= 0 && d3 <= 0) return v3_mad(a, ab, d1 / (d1 - d3));
    v3 cp = v3_sub(p, c); float d5 = v3_dot(ab, cp), d6 = v3_dot(ac, cp); if (d6 >= 0 && d5 <= d6) return c;
    float vb = d5 * d2 - d1 * d6; if (vb <= 0 && d2 >= 0 && d6 <= 0) return v3_mad(a, ac, d2 / (d2 - d6));
    float va = d3 * d6 - d5 * d4; if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) return v3_mad(b, v3_sub(c, b), (d4 - d3) / ((d4 - d3) + (d5 - d6)));
    float den = 1.f / (va + vb + vc), v = vb * den, w = vc * den;
    return v3_add(a, v3_add(v3_scale(ab, v), v3_scale(ac, w)));
}
static void closest_seg_seg(v3 p1, v3 q1, v3 p2, v3 q2, v3 *c1, v3 *c2) {   /* Эриксон, 5.1.9 */
    v3 d1 = v3_sub(q1, p1), d2 = v3_sub(q2, p2), r = v3_sub(p1, p2);
    float a = v3_dot(d1, d1), e = v3_dot(d2, d2), f = v3_dot(d2, r), s, t;
    if (a <= 1e-9f && e <= 1e-9f) { *c1 = p1; *c2 = p2; return; }
    if (a <= 1e-9f) { s = 0; t = lu_clampf(f / e, 0, 1); }
    else {
        float c = v3_dot(d1, r);
        if (e <= 1e-9f) { t = 0; s = lu_clampf(-c / a, 0, 1); }
        else {
            float b = v3_dot(d1, d2), den = a * e - b * b;
            s = den != 0 ? lu_clampf((b * f - c * e) / den, 0, 1) : 0;
            t = (b * s + f) / e;
            if (t < 0) { t = 0; s = lu_clampf(-c / a, 0, 1); } else if (t > 1) { t = 1; s = lu_clampf((b - c) / a, 0, 1); }
        }
    }
    *c1 = v3_mad(p1, d1, s); *c2 = v3_mad(p2, d2, t);
}
static int ray_tri(v3 o, v3 d, const lu_tri *T, float *t) {   /* Мёллер–Трумбор, двусторонний */
    v3 e1 = v3_sub(T->b, T->a), e2 = v3_sub(T->c, T->a), p = v3_cross(d, e2); float det = v3_dot(e1, p);
    if (fabsf(det) < 1e-10f) return 0;
    float inv = 1.f / det; v3 s = v3_sub(o, T->a); float u = v3_dot(s, p) * inv; if (u < 0 || u > 1) return 0;
    v3 q = v3_cross(s, e1); float v = v3_dot(d, q) * inv; if (v < 0 || u + v > 1) return 0;
    *t = v3_dot(e2, q) * inv; return 1;
}
/* расстояние от отрезка до треугольника: точки на отрезке и треугольнике */
static float seg_tri(v3 p0, v3 p1, const lu_tri *T, v3 *ps, v3 *pt) {
    float t; v3 d = v3_sub(p1, p0);
    if (ray_tri(p0, d, T, &t) && t >= 0 && t <= 1) { *ps = *pt = v3_mad(p0, d, t); return 0; }
    float best = 1e30f; v3 a, b;
    a = closest_pt_tri(p0, T->a, T->b, T->c); float q = v3_len2(v3_sub(a, p0)); if (q < best) { best = q; *ps = p0; *pt = a; }
    a = closest_pt_tri(p1, T->a, T->b, T->c); q = v3_len2(v3_sub(a, p1)); if (q < best) { best = q; *ps = p1; *pt = a; }
    const v3 *E[3][2] = {{&T->a, &T->b}, {&T->b, &T->c}, {&T->c, &T->a}};
    for (int i = 0; i < 3; i++) { closest_seg_seg(p0, p1, *E[i][0], *E[i][1], &a, &b); q = v3_len2(v3_sub(a, b)); if (q < best) { best = q; *ps = a; *pt = b; } }
    return sqrtf(best);
}

/* ================= мир и сетка ================= */
void lu_phys_init(lu_phys_world *w) { memset(w, 0, sizeof *w); w->bounds = aabb_empty(); }
void lu_phys_free(lu_phys_world *w) { lu_vec_free(w->tris); lu_free(w->cstart); lu_free(w->citems); lu_free(w->stamp); memset(w, 0, sizeof *w); }
void lu_phys_add_tri(lu_phys_world *w, v3 a, v3 b, v3 c, int flags) {
    v3 n = v3_cross(v3_sub(b, a), v3_sub(c, a)); if (v3_len2(n) < 1e-14f) return;
    lu_tri t = {a, b, c, v3_norm(n), flags}; lu_vec_push(w->tris, t);
}
void lu_phys_add_box(lu_phys_world *w, aabb bx, int flags) {
    m4 m = m4_translate(aabb_center(bx)); lu_phys_add_obb(w, m, aabb_half(bx), flags);
}
void lu_phys_add_obb(lu_phys_world *w, m4 xf, v3 h, int flags) {
    v3 p[8]; for (int i = 0; i < 8; i++) p[i] = m4_point(xf, V3(i & 1 ? h.x : -h.x, i & 2 ? h.y : -h.y, i & 4 ? h.z : -h.z));
    static const int F[6][4] = {{4, 5, 7, 6}, {1, 0, 2, 3}, {5, 1, 3, 7}, {0, 4, 6, 2}, {2, 6, 7, 3}, {0, 1, 5, 4}};
    for (int f = 0; f < 6; f++) { lu_phys_add_tri(w, p[F[f][0]], p[F[f][1]], p[F[f][2]], flags); lu_phys_add_tri(w, p[F[f][0]], p[F[f][2]], p[F[f][3]], flags); }
}
static aabb tri_box(const lu_tri *t) { aabb b = aabb_empty(); b = aabb_add(b, t->a); b = aabb_add(b, t->b); b = aabb_add(b, t->c); return b; }
void lu_phys_build(lu_phys_world *w, float cell) {
    w->bounds = aabb_empty(); for (int i = 0; i < w->tris.n; i++) w->bounds = aabb_union(w->bounds, tri_box(&w->tris.v[i]));
    if (!w->tris.n) w->bounds = (aabb){V3(0, 0, 0), V3(1, 1, 1)};
    w->cell = cell; w->origin = V3(w->bounds.mn.x - cell, 0, w->bounds.mn.z - cell);
    w->gw = (int)((w->bounds.mx.x - w->origin.x) / cell) + 2; w->gh = (int)((w->bounds.mx.z - w->origin.z) / cell) + 2;
    int nc = w->gw * w->gh; lu_free(w->cstart); lu_free(w->citems); lu_free(w->stamp);
    w->cstart = lu_alloc(sizeof(int) * (size_t)(nc + 1)); int *cnt = lu_alloc(sizeof(int) * (size_t)nc);
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < w->tris.n; i++) {
            aabb b = tri_box(&w->tris.v[i]);
            int x0 = (int)((b.mn.x - w->origin.x) / cell), x1 = (int)((b.mx.x - w->origin.x) / cell), z0 = (int)((b.mn.z - w->origin.z) / cell), z1 = (int)((b.mx.z - w->origin.z) / cell);
            for (int z = z0; z <= z1; z++) for (int x = x0; x <= x1; x++) { int c = z * w->gw + x; if (pass == 0) cnt[c]++; else w->citems[w->cstart[c] + cnt[c]++] = i; }
        }
        if (pass == 0) { int s = 0; for (int c = 0; c < nc; c++) { w->cstart[c] = s; s += cnt[c]; cnt[c] = 0; } w->cstart[nc] = s; w->citems = lu_alloc(sizeof(int) * (size_t)(s > 0 ? s : 1)); }
    }
    lu_free(cnt); w->stamp = lu_alloc(sizeof(int) * (size_t)(w->tris.n > 0 ? w->tris.n : 1)); w->cur_stamp = 0;
    LU_INFO("физика: %d треугольников, сетка %dx%d по %.1f м", w->tris.n, w->gw, w->gh, cell);
}
/* обход треугольников в прямоугольнике XZ (каждый — один раз) */
typedef int (*tri_cb)(const lu_tri *t, int idx, void *u);   /* вернуть 1 — остановиться */
static void for_tris(const lu_phys_world *cw, aabb q, tri_cb cb, void *u) {
    lu_phys_world *w = (lu_phys_world *)cw; if (!w->cstart) return;
    if (++w->cur_stamp == 0x7FFFFFFF) { memset(w->stamp, 0, sizeof(int) * (size_t)w->tris.n); w->cur_stamp = 1; }
    int x0 = (int)floorf((q.mn.x - w->origin.x) / w->cell), x1 = (int)floorf((q.mx.x - w->origin.x) / w->cell);
    int z0 = (int)floorf((q.mn.z - w->origin.z) / w->cell), z1 = (int)floorf((q.mx.z - w->origin.z) / w->cell);
    x0 = x0 < 0 ? 0 : x0; z0 = z0 < 0 ? 0 : z0; x1 = x1 >= w->gw ? w->gw - 1 : x1; z1 = z1 >= w->gh ? w->gh - 1 : z1;
    for (int z = z0; z <= z1; z++) for (int x = x0; x <= x1; x++) {
        int c = z * w->gw + x;
        for (int k = w->cstart[c]; k < w->cstart[c + 1]; k++) {
            int i = w->citems[k]; if (w->stamp[i] == w->cur_stamp) continue; w->stamp[i] = w->cur_stamp;
            const lu_tri *t = &w->tris.v[i];
            if (fminf(t->a.y, fminf(t->b.y, t->c.y)) > q.mx.y || fmaxf(t->a.y, fmaxf(t->b.y, t->c.y)) < q.mn.y) continue;
            if (cb(t, i, u)) return;
        }
    }
}

/* ================= лучи ================= */
typedef struct { v3 o, d; float best; int idx; } RayQ;
static int ray_cb(const lu_tri *t, int i, void *u) { RayQ *q = u; float tt; if (ray_tri(q->o, q->d, t, &tt) && tt >= 0 && tt < q->best) { q->best = tt; q->idx = i; } return 0; }
int lu_raycast(const lu_phys_world *w, v3 o, v3 dir, float maxd, lu_hit *out) {
    v3 d = v3_norm(dir); RayQ q = {o, d, maxd, -1};
    v3 e = v3_mad(o, d, maxd); aabb b = {v3_min(o, e), v3_max(o, e)};
    for_tris(w, b, ray_cb, &q);
    if (out) { memset(out, 0, sizeof *out); out->t = maxd; }
    if (q.idx < 0) return 0;
    if (out) { const lu_tri *t = &w->tris.v[q.idx]; out->hit = 1; out->t = q.best; out->pos = v3_mad(o, d, q.best); out->normal = v3_dot(t->n, d) > 0 ? v3_neg(t->n) : t->n; out->tri = q.idx; out->flags = t->flags; }
    return 1;
}

/* ================= капсулы и сферы ================= */
typedef struct { v3 p0, p1; float r; int found; } OvQ;
static int ov_cb(const lu_tri *t, int i, void *u) { LU_UNUSED(i); OvQ *q = u; v3 a, b; if (seg_tri(q->p0, q->p1, t, &a, &b) < q->r) { q->found = 1; return 1; } return 0; }
int lu_capsule_overlap(const lu_phys_world *w, v3 p0, v3 p1, float r) {
    OvQ q = {p0, p1, r, 0}; aabb b = {v3_sub(v3_min(p0, p1), V3(r, r, r)), v3_add(v3_max(p0, p1), V3(r, r, r))};
    for_tris(w, b, ov_cb, &q); return q.found;
}
typedef struct { v3 p0, p1; float r; v3 push; v3 best_n; float best_d; } ResQ;
static int res_cb(const lu_tri *t, int i, void *u) {
    LU_UNUSED(i); ResQ *q = u; v3 a, b;
    v3 p0 = v3_add(q->p0, q->push), p1 = v3_add(q->p1, q->push);
    float d = seg_tri(p0, p1, t, &a, &b); if (d >= q->r) return 0;
    v3 n; if (d > 1e-5f) n = v3_scale(v3_sub(a, b), 1.f / d); else { n = t->n; if (v3_dot(n, v3_sub(v3_scale(v3_add(p0, p1), .5f), t->a)) < 0) n = v3_neg(n); }
    float pen = q->r - d; q->push = v3_mad(q->push, n, pen);
    if (pen > q->best_d) { q->best_d = pen; q->best_n = n; }
    return 0;
}
v3 lu_capsule_resolve(const lu_phys_world *w, v3 p0, v3 p1, float r, v3 *cn, int iters) {
    ResQ q = {p0, p1, r, V3(0, 0, 0), V3(0, 0, 0), 0};
    for (int it = 0; it < iters; it++) {
        v3 before = q.push; float R = r + .1f;
        aabb b = {v3_sub(v3_min(v3_add(p0, q.push), v3_add(p1, q.push)), V3(R, R, R)), v3_add(v3_max(v3_add(p0, q.push), v3_add(p1, q.push)), V3(R, R, R))};
        for_tris(w, b, res_cb, &q);
        if (v3_len2(v3_sub(q.push, before)) < 1e-10f) break;
    }
    if (cn) *cn = q.best_n;
    return q.push;
}
int lu_spherecast(const lu_phys_world *w, v3 o, v3 dir, float r, float maxd, lu_hit *out) {
    v3 d = v3_norm(dir); float step = fmaxf(r * .4f, .02f); float lo = 0, hi = -1;
    if (out) memset(out, 0, sizeof *out);
    if (lu_capsule_overlap(w, o, o, r)) { if (out) { out->hit = 1; out->t = 0; out->pos = o; out->normal = V3(0, 1, 0); } return 1; }
    for (float t = step; ; t += step) { float tt = fminf(t, maxd); if (lu_capsule_overlap(w, v3_mad(o, d, tt), v3_mad(o, d, tt), r)) { hi = tt; break; } lo = tt; if (tt >= maxd) break; }
    if (hi < 0) { if (out) out->t = maxd; return 0; }
    for (int i = 0; i < 10; i++) { float m = (lo + hi) * .5f; if (lu_capsule_overlap(w, v3_mad(o, d, m), v3_mad(o, d, m), r)) hi = m; else lo = m; }
    if (out) {
        v3 c = v3_mad(o, d, hi), n; lu_capsule_resolve(w, c, c, r * 1.001f, &n, 1);
        out->hit = 1; out->t = lo; out->pos = v3_mad(o, d, lo); out->normal = v3_len2(n) > 0 ? n : v3_neg(d);
    }
    return 1;
}

/* ================= контроллер персонажа ================= */
void lu_cc_init(lu_cc *c, v3 pos) {
    memset(c, 0, sizeof *c); c->pos = pos; c->radius = .3f; c->height = 1.8f; c->step = .42f; c->max_slope_cos = .7f; c->skin = .01f; c->ground_n = V3(0, 1, 0);
}
static void cc_caps(const lu_cc *c, v3 p, v3 *a, v3 *b) { *a = V3(p.x, p.y + c->radius + c->skin, p.z); *b = V3(p.x, p.y + c->height - c->radius, p.z); }
float lu_floor_at(const lu_phys_world *w, v3 p, float up, float down, v3 *n, int *flags) {
    lu_hit h; if (!lu_raycast(w, V3(p.x, p.y + up, p.z), V3(0, -1, 0), up + down, &h)) return -1e9f;
    if (n) *n = h.normal;
    if (flags) *flags = h.flags;
    return h.pos.y;
}
static void cc_ground_probe(lu_cc *c, const lu_phys_world *w, float dist) {
    lu_hit h; v3 o = V3(c->pos.x, c->pos.y + c->radius + .05f, c->pos.z);
    c->grounded = 0;
    if (lu_spherecast(w, o, V3(0, -1, 0), c->radius * .92f, dist + .05f, &h) && h.normal.y >= c->max_slope_cos) {
        float gy = o.y - h.t - c->radius * .92f;  /* низ сферы в точке касания */
        c->grounded = 1; c->ground_n = h.normal; c->ground_y = gy;
        lu_hit r; c->ground_flags = lu_raycast(w, o, V3(0, -1, 0), dist + c->radius + .3f, &r) ? r.flags : 0;
        if (c->vel.y <= 0) c->pos.y = gy;
    }
}
void lu_cc_move(lu_cc *c, const lu_phys_world *w, float dt, int snap) {
    c->was_grounded = c->grounded; c->hit_wall = 0;
    float sp = v3_len(c->vel) * dt; int n = (int)(sp / (c->radius * .45f)) + 1; if (n > 16) n = 16; float h = dt / n;
    for (int s = 0; s < n; s++) {
        v3 old = c->pos; v3 d = v3_scale(c->vel, h);
        c->pos = v3_add(c->pos, d);
        v3 a, b, cn; cc_caps(c, c->pos, &a, &b);
        v3 push = lu_capsule_resolve(w, a, b, c->radius, &cn, 4);
        if (v3_len2(push) > 0) {
            v3 pn = v3_norm(push);
            if (pn.y < c->max_slope_cos && pn.y > -.5f && c->was_grounded && c->vel.y <= .1f) {
                /* попытка шагнуть на ступень */
                v3 up = v3_add(old, V3(0, c->step, 0)), tgt = v3_add(up, V3(d.x, 0, d.z)), ua, ub; cc_caps(c, tgt, &ua, &ub);
                v3 ua0, ub0; cc_caps(c, up, &ua0, &ub0);
                if (!lu_capsule_overlap(w, ua0, ub0, c->radius) && !lu_capsule_overlap(w, ua, ub, c->radius)) {
                    float fy = lu_floor_at(w, tgt, 0, c->step + .05f, NULL, NULL);
                    if (fy > old.y + .02f && fy <= old.y + c->step + .01f) { c->pos = V3(tgt.x, fy, tgt.z); continue; }
                }
            }
            c->pos = v3_add(c->pos, push);
            if (pn.y >= c->max_slope_cos) { if (c->vel.y < 0) c->vel.y = 0; c->grounded = 1; c->ground_n = pn; }
            else if (pn.y < -.5f) { if (c->vel.y > 0) c->vel.y = 0; }
            else { float into = v3_dot(c->vel, pn); if (into < 0) c->vel = v3_mad(c->vel, pn, -into); c->hit_wall = 1; c->wall_n = V3(pn.x, 0, pn.z); }
        }
    }
    cc_ground_probe(c, w, (snap && c->was_grounded && c->vel.y <= 0) ? c->step + .05f : .03f);
    if (c->grounded) { if (!c->was_grounded) c->air_time = 0; c->fall_start_y = c->pos.y; }
    else { if (c->was_grounded) c->fall_start_y = c->pos.y; c->air_time += dt; if (c->pos.y > c->fall_start_y) c->fall_start_y = c->pos.y; }
}

/* ================= сенсоры паркура ================= */
float lu_probe_wall(const lu_phys_world *w, v3 from, v3 dir, float height, float maxd, v3 *n_out, int *flags) {
    v3 d = v3_norm(V3(dir.x, 0, dir.z)); lu_hit h;
    if (!lu_raycast(w, V3(from.x, from.y + height, from.z), d, maxd, &h)) return -1;
    if (fabsf(h.normal.y) > .5f) return -1;
    if (n_out) *n_out = v3_norm(V3(h.normal.x, 0, h.normal.z));
    if (flags) *flags = h.flags;
    return h.t;
}
int lu_find_ledge(const lu_phys_world *w, v3 feet, v3 dir, float hmin, float hmax, float reach, lu_ledge *out) {
    memset(out, 0, sizeof *out);
    v3 d = v3_norm(V3(dir.x, 0, dir.z));
    /* сверху вниз: идём вперёд и ищем первую горизонтальную площадку нужной высоты (так находятся и выступающие карнизы) */
    for (float s = .12f; s <= reach + .35f; s += .08f) {
        v3 p = v3_add(feet, v3_scale(d, s)), o = V3(p.x, feet.y + hmax + .3f, p.z);
        if (lu_capsule_overlap(w, o, o, .04f)) continue;          /* начало луча внутри стены */
        lu_hit h; if (!lu_raycast(w, o, V3(0, -1, 0), hmax - hmin + .5f, &h)) continue;
        if (h.normal.y < .65f) continue;
        float top = h.pos.y; if (top < feet.y + hmin || top > feet.y + hmax) continue;
        /* передняя грань: горизонтальный луч чуть ниже кромки */
        v3 st = V3(feet.x, top - .07f, feet.z); st = v3_sub(st, v3_scale(d, .3f));
        lu_hit hf; if (!lu_raycast(w, st, d, s + .6f, &hf) || fabsf(hf.normal.y) > .5f) continue;
        if (hf.t - .3f > reach) continue;
        if (hf.flags & LU_SURF_NOGRAB) return 0;
        /* над кромкой свободно */
        lu_hit ha; v3 sa = V3(st.x, top + .22f, st.z);
        if (lu_raycast(w, sa, d, hf.t + .25f, &ha)) continue;
        v3 wn = v3_norm(V3(hf.normal.x, 0, hf.normal.z));
        out->ok = 1; out->wall_n = wn; out->wall_pt = hf.pos; out->top_y = top; out->height = top - feet.y; out->flags = h.flags | hf.flags;
        out->edge = V3(hf.pos.x, top, hf.pos.z); out->edge_dir = v3_norm(v3_cross(V3(0, 1, 0), wn));
        v3 sp = v3_add(out->edge, v3_scale(wn, -.5f));
        float fy = lu_floor_at(w, V3(sp.x, top + .8f, sp.z), 0, 1.4f, NULL, NULL); if (fy < top - .6f || fy > top + .7f) fy = top;
        out->can_stand = !lu_capsule_overlap(w, V3(sp.x, fy + .45f, sp.z), V3(sp.x, fy + 1.5f, sp.z), .26f);
        return 1;
    }
    return 0;
}
int lu_find_grab(const lu_phys_world *w, v3 hands, v3 facing, float r, lu_ledge *out) {
    v3 f = v3_norm(V3(facing.x, 0, facing.z)); if (v3_len2(f) < .5f) f = V3(0, 0, 1);
    /* сначала прямо вперёд, затем немного по сторонам */
    for (int k = 0; k < 3; k++) {
        float a = k == 0 ? 0 : (k == 1 ? .5f : -.5f); v3 d = quat_rot(quat_axis(V3(0, 1, 0), a), f);
        v3 feet = V3(hands.x, hands.y - 2.05f, hands.z);
        if (lu_find_ledge(w, feet, d, 1.75f, 2.35f, r, out)) return 1;
    }
    return 0;
}
int lu_find_vault(const lu_phys_world *w, v3 feet, v3 dir, float reach, float *oh, float *depth) {
    lu_ledge l; if (!lu_find_ledge(w, feet, dir, .4f, 1.3f, reach, &l)) return 0;
    v3 into = v3_neg(l.wall_n);
    /* где кончается верх препятствия */
    float dd = 0;
    for (float t = .2f; t <= 1.6f; t += .15f) { v3 p = v3_add(l.edge, v3_scale(into, t)); float fy = lu_floor_at(w, V3(p.x, l.top_y + .3f, p.z), 0, .5f, NULL, NULL); if (fy < l.top_y - .2f) { dd = t; break; } }
    if (dd <= 0) return 0;
    v3 land = v3_add(l.edge, v3_scale(into, dd + .6f));
    if (lu_capsule_overlap(w, V3(land.x, feet.y + .5f, land.z), V3(land.x, feet.y + 1.6f, land.z), .28f)) return 0;
    if (oh) *oh = l.height;
    if (depth) *depth = dd;
    return 1;
}
int lu_ledge_continues(const lu_phys_world *w, const lu_ledge *l, float side, float d, lu_ledge *out) {
    v3 p = v3_add(l->edge, v3_scale(l->edge_dir, side * d));
    v3 feet = v3_add(V3(p.x, l->top_y - l->height, p.z), v3_scale(l->wall_n, .5f));
    if (!lu_find_ledge(w, feet, v3_neg(l->wall_n), l->height - .35f, l->height + .35f, .9f, out)) return 0;
    return fabsf(out->top_y - l->top_y) < .3f;
}
