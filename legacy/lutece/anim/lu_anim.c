/* Lutèce Engine — скелетная анимация */
#include "lu_anim.h"
#include <stdio.h>

/* ================= скелет ================= */
int lu_skel_add(lu_skeleton *s, const char *name, int parent, xform bind) {
    if (s->n >= LU_MAX_BONES) return -1;
    lu_joint *j = &s->j[s->n]; memset(j, 0, sizeof *j);
    snprintf(j->name, sizeof j->name, "%s", name); j->parent = parent; j->bind = bind;
    return s->n++;
}
int lu_skel_find(const lu_skeleton *s, const char *name) { for (int i = 0; i < s->n; i++) if (!strcmp(s->j[i].name, name)) return i; return -1; }
void lu_skel_finalize(lu_skeleton *s) {
    xform model[LU_MAX_BONES];
    for (int i = 0; i < s->n; i++) {
        model[i] = s->j[i].parent >= 0 ? xf_mul(model[s->j[i].parent], s->j[i].bind) : s->j[i].bind;
        s->j[i].inv_bind = m4_inverse(m4_from_xform(model[i]));
    }
}

/* ================= позы ================= */
void lu_pose_bind(const lu_skeleton *s, lu_pose *p) { p->n = s->n; for (int i = 0; i < s->n; i++) p->l[i] = s->j[i].bind; p->root_delta = V3(0, 0, 0); p->root_yaw_delta = 0; }
void lu_pose_blend(const lu_pose *a, const lu_pose *b, float w, lu_pose *out) {
    int n = a->n; out->n = n;
    if (w <= 0) { if (out != a) *out = *a; return; }
    if (w >= 1) { if (out != b) *out = *b; return; }
    for (int i = 0; i < n; i++) out->l[i] = xf_lerp(a->l[i], b->l[i], w);
    out->root_delta = v3_lerp(a->root_delta, b->root_delta, w);
}
void lu_pose_blend_masked(const lu_pose *a, const lu_pose *b, const float *mask, float w, lu_pose *out) {
    int n = a->n; out->n = n;
    for (int i = 0; i < n; i++) { float k = w * mask[i]; out->l[i] = k <= 0 ? a->l[i] : k >= 1 ? b->l[i] : xf_lerp(a->l[i], b->l[i], k); }
}
void lu_pose_additive(const lu_pose *base, const lu_pose *add, const lu_pose *ref, float w, lu_pose *out) {
    int n = base->n; out->n = n;
    for (int i = 0; i < n; i++) {
        quat dq = quat_mul(add->l[i].r, quat_conj(ref->l[i].r)); if (w != 1) dq = quat_scale_angle(dq, w);
        xform x = base->l[i]; x.r = quat_norm(quat_mul(dq, x.r)); x.t = v3_mad(x.t, v3_sub(add->l[i].t, ref->l[i].t), w);
        out->l[i] = x;
    }
}
void lu_pose_model(const lu_skeleton *s, const lu_pose *p, xform *model) {
    for (int i = 0; i < s->n; i++) model[i] = s->j[i].parent >= 0 ? xf_mul(model[s->j[i].parent], p->l[i]) : p->l[i];
}
void lu_pose_palette(const lu_skeleton *s, const xform *model, m4 *pal) {
    for (int i = 0; i < s->n; i++) pal[i] = m4_mul(m4_from_xform(model[i]), s->j[i].inv_bind);
}

/* ================= клипы ================= */
void lu_clip_init(lu_clip *c, const char *name, float duration, int loop) {
    memset(c, 0, sizeof *c); snprintf(c->name, sizeof c->name, "%s", name); c->duration = duration; c->loop = loop; c->root_joint = -1;
}
void lu_clip_free(lu_clip *c) { for (int i = 0; i < c->ntracks; i++) { lu_free(c->tracks[i].times); lu_free(c->tracks[i].values); } lu_free(c->tracks); memset(c, 0, sizeof *c); }
static lu_track *track_get(lu_clip *c, int joint, int path) {
    for (int i = 0; i < c->ntracks; i++) if (c->tracks[i].joint == joint && c->tracks[i].path == path) return &c->tracks[i];
    if (c->ntracks >= c->cap) {
        c->cap = c->cap ? c->cap * 2 : 16; c->tracks = lu_realloc(c->tracks, sizeof(lu_track) * (size_t)c->cap); }
    lu_track *t = &c->tracks[c->ntracks++]; memset(t, 0, sizeof *t); t->joint = joint; t->path = path; return t;
}
static void track_key(lu_track *t, float time, const float *v, int nc) {
    t->times = lu_realloc(t->times, sizeof(float) * (size_t)(t->nkeys + 1));
    t->values = lu_realloc(t->values, sizeof(float) * (size_t)(t->nkeys + 1) * 4);
    t->times[t->nkeys] = time; memcpy(&t->values[t->nkeys * 4], v, sizeof(float) * (size_t)nc); t->nkeys++;
}
void lu_clip_key_t(lu_clip *c, int joint, float t, v3 v) { float f[4] = {v.x, v.y, v.z, 0}; track_key(track_get(c, joint, LU_TRACK_T), t, f, 3); }
void lu_clip_key_r(lu_clip *c, int joint, float t, quat q) { float f[4] = {q.x, q.y, q.z, q.w}; track_key(track_get(c, joint, LU_TRACK_R), t, f, 4); }
void lu_clip_event(lu_clip *c, float t, int code) { if (c->nev < 16) { c->ev_t[c->nev] = t; c->ev_code[c->nev] = code; c->nev++; } }
static void track_eval(const lu_track *tr, float t, float *out) {
    int n = tr->nkeys; if (n == 1 || t <= tr->times[0]) { memcpy(out, tr->values, 16); return; }
    if (t >= tr->times[n - 1]) { memcpy(out, &tr->values[(n - 1) * 4], 16); return; }
    int lo = 0, hi = n - 1; while (hi - lo > 1) { int m = (lo + hi) / 2; if (tr->times[m] <= t) lo = m; else hi = m; }
    float t0 = tr->times[lo], t1 = tr->times[hi], u = tr->step ? 0.f : (t - t0) / (t1 - t0);
    const float *a = &tr->values[lo * 4], *b = &tr->values[hi * 4];
    if (tr->path == LU_TRACK_R) {
        quat qa = {a[0], a[1], a[2], a[3]}, qb = {b[0], b[1], b[2], b[3]}, q = quat_slerp(qa, qb, u);
        out[0] = q.x; out[1] = q.y; out[2] = q.z; out[3] = q.w;
    } else for (int k = 0; k < 3; k++) out[k] = a[k] + (b[k] - a[k]) * u;
}
static float clip_time(const lu_clip *c, float t) {
    if (c->duration <= 0) return 0;
    if (c->loop) { t = fmodf(t, c->duration); if (t < 0) t += c->duration; return t; }
    return lu_clampf(t, 0, c->duration);
}
void lu_clip_sample(const lu_clip *c, const lu_skeleton *s, float t, lu_pose *out) {
    lu_pose_bind(s, out); t = clip_time(c, t);
    for (int i = 0; i < c->ntracks; i++) {
        const lu_track *tr = &c->tracks[i]; if (tr->joint < 0 || tr->joint >= s->n || !tr->nkeys) continue;
        float v[4]; track_eval(tr, t, v); xform *x = &out->l[tr->joint];
        if (tr->path == LU_TRACK_T) { float k = s->tscale > 0 ? s->tscale : 1.f; x->t = V3(v[0] * k, v[1] * k, v[2] * k); }
        else if (tr->path == LU_TRACK_R) x->r = quat_norm((quat){v[0], v[1], v[2], v[3]});
        else x->s = V3(v[0], v[1], v[2]);
    }
}
static v3 root_pos(const lu_clip *c, float t) {
    for (int i = 0; i < c->ntracks; i++) if (c->tracks[i].joint == c->root_joint && c->tracks[i].path == LU_TRACK_T) {
        float v[4]; track_eval(&c->tracks[i], lu_clampf(t, 0, c->duration), v); return V3(v[0], v[1], v[2]);
    }
    return V3(0, 0, 0);
}
int lu_clip_events(const lu_clip *c, float t0, float t1, int *codes, int max) {
    int n = 0; if (!c->nev || t1 <= t0) return 0;
    float d = c->duration;
    for (int i = 0; i < c->nev && n < max; i++) {
        float e = c->ev_t[i];
        if (c->loop && d > 0) {
            float k0 = floorf((t0 - e) / d), k1 = floorf((t1 - e) / d);
            if (k1 > k0) codes[n++] = c->ev_code[i];
        } else if (e > t0 && e <= t1) codes[n++] = c->ev_code[i];
    }
    return n;
}

/* ================= граф состояний ================= */
void lu_ag_init(lu_animgraph *g, const lu_skeleton *s) { memset(g, 0, sizeof *g); g->skel = s; g->cur = -1; g->prev = -1; lu_pose_bind(s, &g->pose); }
int lu_ag_state_clip(lu_animgraph *g, const char *name, const lu_clip *c, float speed) {
    if (g->nst >= LU_AG_MAX_STATES) return -1;
    lu_ag_state *st = &g->st[g->nst]; memset(st, 0, sizeof *st);
    snprintf(st->name, sizeof st->name, "%s", name); st->clip[0] = c; st->nclip = 1; st->speed = speed > 0 ? speed : 1; st->param = -1;
    return g->nst++;
}
int lu_ag_state_blend(lu_animgraph *g, const char *name, int param, const lu_clip **clips, const float *at, int n, int sync) {
    if (g->nst >= LU_AG_MAX_STATES || n < 1) return -1;
    lu_ag_state *st = &g->st[g->nst]; memset(st, 0, sizeof *st);
    snprintf(st->name, sizeof st->name, "%s", name); st->param = param; st->speed = 1; st->sync = sync;
    if (n > LU_AG_MAX_BLEND) n = LU_AG_MAX_BLEND;
    for (int i = 0; i < n; i++) { st->clip[i] = clips[i]; st->at[i] = at[i]; } st->nclip = n;
    return g->nst++;
}
int lu_ag_find(const lu_animgraph *g, const char *name) { for (int i = 0; i < g->nst; i++) if (!strcmp(g->st[i].name, name)) return i; return -1; }
void lu_ag_play(lu_animgraph *g, int state, float fade, float start) {
    if (state < 0 || state >= g->nst) return;
    if (g->cur >= 0 && fade > 0) { g->prev = g->cur; g->t_prev = g->t_cur; g->fade = 0; g->fade_len = fade; }
    else { g->prev = -1; g->fade_len = 0; }
    g->cur = state; g->t_cur = start; g->phase = 0;
    const lu_ag_state *st = &g->st[state];
    if (st->sync && st->nclip && st->clip[0]->duration > 0) g->phase = start / st->clip[0]->duration;
}
/* веса пространства смешивания: два соседних клипа */
static void blend_weights(const lu_ag_state *st, float p, int *i0, int *i1, float *w) {
    if (st->nclip == 1 || p <= st->at[0]) { *i0 = *i1 = 0; *w = 0; return; }
    if (p >= st->at[st->nclip - 1]) { *i0 = *i1 = st->nclip - 1; *w = 0; return; }
    for (int i = 0; i + 1 < st->nclip; i++) if (p <= st->at[i + 1]) { *i0 = i; *i1 = i + 1; *w = (p - st->at[i]) / (st->at[i + 1] - st->at[i]); return; }
    *i0 = *i1 = 0; *w = 0;
}
/* выборка состояния; t — локальное время (или фаза при синхронизации); dt — для движения корня и событий */
static void sample_state(lu_animgraph *g, int si, float t_before, float t_after, int sync, float ph0, float ph1, lu_pose *out, v3 *rm, int collect) {
    const lu_ag_state *st = &g->st[si]; int i0, i1; float w;
    blend_weights(st, st->param >= 0 ? g->param[st->param] : 0, &i0, &i1, &w);
    int idx[2] = {i0, i1}; float ws[2] = {1 - w, w}; *rm = V3(0, 0, 0);
    for (int k = 0; k < (i0 == i1 ? 1 : 2); k++) {
        const lu_clip *c = st->clip[idx[k]]; float a, b;
        if (sync) { a = ph0 * c->duration; b = ph1 * c->duration; } else { a = t_before; b = t_after; }
        lu_pose *dst = k == 0 ? out : &g->tmp_b;
        lu_clip_sample(c, g->skel, b, dst);
        if (c->root_motion && c->root_joint >= 0) {
            v3 p0 = root_pos(c, a), p1 = root_pos(c, b), d;
            if (c->loop && b < a) { /* переход через конец цикла */ v3 e = root_pos(c, c->duration), s0 = root_pos(c, 0); d = v3_add(v3_sub(e, p0), v3_sub(p1, s0)); }
            else if (c->loop && b - a > c->duration) d = V3(0, 0, 0);
            else d = v3_sub(p1, p0);
            /* смещение в пространстве модели (родитель корня — начало координат) */
            d = v3_scale(d, g->skel->tscale > 0 ? g->skel->tscale : 1.f);
            if (c->root_motion < 2) d.y = 0;
            *rm = v3_mad(*rm, d, k == 0 && i0 != i1 ? ws[0] : (k == 1 ? ws[1] : 1.f));
            xform *rx = &dst->l[c->root_joint]; rx->t.x = 0; rx->t.z = 0; if (c->root_motion >= 2) rx->t.y = 0;
        }
        if (collect && ws[k] >= .5f) {
            float ta = a, tb = b; if (sync && tb < ta) tb += c->duration; /* фазовый перескок */
            g->nevents += lu_clip_events(c, ta, tb, &g->events[g->nevents], 16 - g->nevents);
        }
    }
    if (i0 != i1) lu_pose_blend(out, &g->tmp_b, w, out);
}
void lu_ag_update(lu_animgraph *g, float dt) {
    g->nevents = 0; g->root_motion = V3(0, 0, 0);
    if (g->cur < 0) { lu_pose_bind(g->skel, &g->pose); return; }
    const lu_ag_state *st = &g->st[g->cur];
    float t0 = g->t_cur; g->t_cur += dt * st->speed;
    float ph0 = g->phase, ph1 = ph0;
    if (st->sync) { /* длительность цикла — смесь длительностей соседних клипов */
        int i0, i1; float w; blend_weights(st, st->param >= 0 ? g->param[st->param] : 0, &i0, &i1, &w);
        float dur = lu_lerpf(st->clip[i0]->duration, st->clip[i1]->duration, w); if (dur < 1e-3f) dur = 1;
        ph1 = ph0 + dt * st->speed / dur; g->phase = ph1 - floorf(ph1); ph1 = g->phase; if (ph1 < ph0) ph0 -= 1; /* для событий */
        ph0 = ph0 < 0 ? ph0 + 1 : ph0;
    }
    v3 rm; sample_state(g, g->cur, t0, g->t_cur, st->sync, ph0, ph1, &g->pose, &rm, 1);
    if (g->prev >= 0 && g->fade_len > 0) {
        g->fade += dt; float w = lu_smooth(g->fade / g->fade_len);
        const lu_ag_state *ps = &g->st[g->prev]; float tp0 = g->t_prev; g->t_prev += dt * ps->speed; v3 rm2;
        float pp = 0; if (ps->sync && ps->clip[0]->duration > 0) pp = fmodf(g->t_prev / ps->clip[0]->duration, 1.f);
        sample_state(g, g->prev, tp0, g->t_prev, ps->sync, pp, pp, &g->tmp_a, &rm2, 0);
        lu_pose_blend(&g->tmp_a, &g->pose, w, &g->pose);
        rm = v3_lerp(rm2, rm, w);
        if (g->fade >= g->fade_len) g->prev = -1;
    }
    g->root_motion = rm;
}
float lu_ag_time(const lu_animgraph *g) { return g->t_cur; }
float lu_ag_norm_time(const lu_animgraph *g) {
    if (g->cur < 0) return 0;
    const lu_ag_state *st = &g->st[g->cur];
    if (st->sync) return g->phase;
    float d = st->clip[0]->duration; if (d <= 0) return 1;
    return st->clip[0]->loop ? fmodf(g->t_cur, d) / d : lu_clampf(g->t_cur / d, 0, 1);
}

/* ================= IK ================= */
static quat parent_model_rot(const lu_skeleton *s, const xform *model, int j) { int p = s->j[j].parent; return p >= 0 ? model[p].r : Q_ID(); }
int lu_ik_two_bone(const lu_skeleton *s, lu_pose *p, int a, int b, int c, v3 target, v3 pole, float weight) {
    if (weight <= 0) return 1;
    xform model[LU_MAX_BONES]; lu_pose_model(s, p, model);
    v3 A = model[a].t, B = model[b].t, C = model[c].t;
    float l1 = v3_dist(A, B), l2 = v3_dist(B, C); v3 d = v3_sub(target, A); float dl = v3_len(d); int reach = 1;
    float mx = (l1 + l2) * .9995f, mn = fabsf(l1 - l2) * 1.001f + 1e-4f;
    if (dl > mx) { d = v3_scale(d, mx / dl); dl = mx; reach = 0; }
    if (dl < mn) { d = v3_scale(v3_norm(d), mn); dl = mn; reach = 0; }
    v3 dir = v3_scale(d, 1.f / dl);
    float ax = (l1 * l1 - l2 * l2 + dl * dl) / (2 * dl), h = sqrtf(fmaxf(l1 * l1 - ax * ax, 0));
    v3 pv = v3_sub(pole, A); v3 bend = v3_reject(pv, dir);
    if (v3_len2(bend) < 1e-8f) bend = v3_reject(v3_sub(B, A), dir);
    bend = v3_norm(bend);
    v3 Bn = v3_add(A, v3_add(v3_scale(dir, ax), v3_scale(bend, h))), Tn = v3_add(A, d);
    /* верхнее звено */
    quat q1 = quat_from_to(v3_norm(v3_sub(B, A)), v3_norm(v3_sub(Bn, A)));
    quat ma = quat_norm(quat_mul(q1, model[a].r));
    quat la = quat_norm(quat_mul(quat_conj(parent_model_rot(s, model, a)), ma));
    p->l[a].r = quat_nlerp(p->l[a].r, la, weight);
    /* нижнее звено: пересчитать после поворота верхнего */
    lu_pose_model(s, p, model);
    v3 B2 = model[b].t, C2 = model[c].t;
    quat q2 = quat_from_to(v3_norm(v3_sub(C2, B2)), v3_norm(v3_sub(Tn, B2)));
    quat mb = quat_norm(quat_mul(q2, model[b].r));
    quat lb = quat_norm(quat_mul(quat_conj(model[a].r), mb));
    p->l[b].r = quat_nlerp(p->l[b].r, lb, weight);
    return reach;
}
void lu_ik_look(const lu_skeleton *s, lu_pose *p, int j, v3 local_fwd, v3 target, float weight, float max_angle) {
    if (weight <= 0) return;
    xform model[LU_MAX_BONES]; lu_pose_model(s, p, model);
    v3 fwd = v3_norm(quat_rot(model[j].r, local_fwd)), want = v3_norm(v3_sub(target, model[j].t));
    if (v3_len2(want) < 1e-8f) return;
    float ang = acosf(lu_clampf(v3_dot(fwd, want), -1, 1)); if (ang < 1e-4f) return;
    quat dq = quat_from_to(fwd, want); float k = weight; if (ang > max_angle) k *= max_angle / ang;
    dq = quat_scale_angle(dq, k);
    quat m = quat_norm(quat_mul(dq, model[j].r));
    p->l[j].r = quat_norm(quat_mul(quat_conj(parent_model_rot(s, model, j)), m));
}
void lu_ik_set_model_rot(const lu_skeleton *s, lu_pose *p, int j, quat mr, float weight) {
    xform model[LU_MAX_BONES]; lu_pose_model(s, p, model);
    quat l = quat_norm(quat_mul(quat_conj(parent_model_rot(s, model, j)), mr));
    p->l[j].r = quat_nlerp(p->l[j].r, l, weight);
}
