/* Lutèce Engine — математика: векторы, кватернионы, матрицы 4x4 (по столбцам, как в OpenGL), AABB, плоскости.
   Всё inline, без зависимостей кроме <math.h>. */
#ifndef LU_MATH_H
#define LU_MATH_H
#include <math.h>
#include <stdint.h>

#define LU_PI   3.14159265358979f
#define LU_TAU  6.28318530717959f
#define LU_INLINE static inline

typedef struct { float x, y; } v2;
typedef struct { float x, y, z; } v3;
typedef struct { float x, y, z, w; } v4;
typedef struct { float x, y, z, w; } quat;
/* m[колонка*4 + строка] — column-major, как ожидает glUniformMatrix4fv(..., GL_FALSE, ...) */
typedef struct { float m[16]; } m4;
typedef struct { v3 mn, mx; } aabb;
/* поза кости / объекта: перенос, поворот, масштаб */
typedef struct { v3 t; quat r; v3 s; } xform;

LU_INLINE float lu_minf(float a, float b) { return a < b ? a : b; }
LU_INLINE float lu_maxf(float a, float b) { return a > b ? a : b; }
LU_INLINE float lu_clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
LU_INLINE float lu_lerpf(float a, float b, float t) { return a + (b - a) * t; }
LU_INLINE float lu_smooth(float t) { t = lu_clampf(t, 0, 1); return t * t * (3 - 2 * t); }
LU_INLINE float lu_wrap_pi(float a) { while (a > LU_PI) a -= LU_TAU; while (a < -LU_PI) a += LU_TAU; return a; }
/* экспоненциальное сглаживание, не зависящее от частоты кадров */
LU_INLINE float lu_damp(float cur, float tgt, float rate, float dt) { return tgt + (cur - tgt) * expf(-rate * dt); }

LU_INLINE v2 V2(float x, float y) { v2 r = {x, y}; return r; }
LU_INLINE v3 V3(float x, float y, float z) { v3 r = {x, y, z}; return r; }
LU_INLINE v4 V4(float x, float y, float z, float w) { v4 r = {x, y, z, w}; return r; }
LU_INLINE v3 v3_add(v3 a, v3 b) { return V3(a.x + b.x, a.y + b.y, a.z + b.z); }
LU_INLINE v3 v3_sub(v3 a, v3 b) { return V3(a.x - b.x, a.y - b.y, a.z - b.z); }
LU_INLINE v3 v3_mul(v3 a, v3 b) { return V3(a.x * b.x, a.y * b.y, a.z * b.z); }
LU_INLINE v3 v3_scale(v3 a, float s) { return V3(a.x * s, a.y * s, a.z * s); }
LU_INLINE v3 v3_mad(v3 a, v3 b, float s) { return V3(a.x + b.x * s, a.y + b.y * s, a.z + b.z * s); }
LU_INLINE v3 v3_neg(v3 a) { return V3(-a.x, -a.y, -a.z); }
LU_INLINE float v3_dot(v3 a, v3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
LU_INLINE v3 v3_cross(v3 a, v3 b) { return V3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
LU_INLINE float v3_len2(v3 a) { return v3_dot(a, a); }
LU_INLINE float v3_len(v3 a) { return sqrtf(v3_dot(a, a)); }
LU_INLINE float v3_dist(v3 a, v3 b) { return v3_len(v3_sub(a, b)); }
LU_INLINE v3 v3_norm(v3 a) { float l = v3_len(a); return l > 1e-12f ? v3_scale(a, 1.f / l) : V3(0, 0, 0); }
LU_INLINE v3 v3_lerp(v3 a, v3 b, float t) { return V3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t); }
LU_INLINE v3 v3_min(v3 a, v3 b) { return V3(lu_minf(a.x, b.x), lu_minf(a.y, b.y), lu_minf(a.z, b.z)); }
LU_INLINE v3 v3_max(v3 a, v3 b) { return V3(lu_maxf(a.x, b.x), lu_maxf(a.y, b.y), lu_maxf(a.z, b.z)); }
LU_INLINE float v3_hlen(v3 a) { return sqrtf(a.x * a.x + a.z * a.z); }
/* проекция на плоскость с нормалью n */
LU_INLINE v3 v3_reject(v3 a, v3 n) { return v3_sub(a, v3_scale(n, v3_dot(a, n))); }

/* ---------- кватернионы (x,y,z — вектор, w — скаляр) ---------- */
LU_INLINE quat Q_ID(void) { quat q = {0, 0, 0, 1}; return q; }
LU_INLINE quat quat_axis(v3 axis, float ang) {
    v3 a = v3_norm(axis); float s = sinf(ang * .5f);
    quat q = {a.x * s, a.y * s, a.z * s, cosf(ang * .5f)}; return q;
}
LU_INLINE quat quat_mul(quat a, quat b) {
    quat r = { a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
               a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
               a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
               a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z };
    return r;
}
LU_INLINE quat quat_conj(quat q) { quat r = {-q.x, -q.y, -q.z, q.w}; return r; }
LU_INLINE float quat_dot(quat a, quat b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
LU_INLINE quat quat_norm(quat q) {
    float l = sqrtf(quat_dot(q, q)); if (l < 1e-12f) return Q_ID();
    quat r = {q.x / l, q.y / l, q.z / l, q.w / l}; return r;
}
LU_INLINE v3 quat_rot(quat q, v3 v) {
    v3 u = V3(q.x, q.y, q.z);
    v3 t = v3_scale(v3_cross(u, v), 2.f);
    return v3_add(v3_add(v, v3_scale(t, q.w)), v3_cross(u, t));
}
/* нормализованный lerp по кратчайшему пути — для смешивания анимаций */
LU_INLINE quat quat_nlerp(quat a, quat b, float t) {
    if (quat_dot(a, b) < 0) { b.x = -b.x; b.y = -b.y; b.z = -b.z; b.w = -b.w; }
    quat r = {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
    return quat_norm(r);
}
LU_INLINE quat quat_slerp(quat a, quat b, float t) {
    float d = quat_dot(a, b);
    if (d < 0) { b.x = -b.x; b.y = -b.y; b.z = -b.z; b.w = -b.w; d = -d; }
    if (d > .9995f) return quat_nlerp(a, b, t);
    float th = acosf(d), s = sinf(th), wa = sinf((1 - t) * th) / s, wb = sinf(t * th) / s;
    quat r = {a.x * wa + b.x * wb, a.y * wa + b.y * wb, a.z * wa + b.z * wb, a.w * wa + b.w * wb};
    return r;
}
/* поворот, переводящий единичный вектор a в единичный b */
LU_INLINE quat quat_from_to(v3 a, v3 b) {
    float d = v3_dot(a, b);
    if (d < -.99999f) {
        v3 ax = v3_cross(V3(1, 0, 0), a); if (v3_len2(ax) < 1e-6f) ax = v3_cross(V3(0, 1, 0), a);
        return quat_axis(ax, LU_PI);
    }
    v3 c = v3_cross(a, b); quat q = {c.x, c.y, c.z, 1 + d}; return quat_norm(q);
}
LU_INLINE quat quat_euler(float yaw, float pitch, float roll) { /* Y, затем X, затем Z */
    return quat_mul(quat_axis(V3(0, 1, 0), yaw), quat_mul(quat_axis(V3(1, 0, 0), pitch), quat_axis(V3(0, 0, 1), roll)));
}
LU_INLINE quat quat_scale_angle(quat q, float k) { /* q^k — для аддитивных слоёв */
    if (q.w < 0) { q.x = -q.x; q.y = -q.y; q.z = -q.z; q.w = -q.w; }
    float w = lu_clampf(q.w, -1, 1), ang = 2 * acosf(w), s = sqrtf(1 - w * w);
    if (s < 1e-5f) return Q_ID();
    return quat_axis(V3(q.x / s, q.y / s, q.z / s), ang * k);
}

/* ---------- xform ---------- */
LU_INLINE xform XF_ID(void) { xform x = {{0, 0, 0}, {0, 0, 0, 1}, {1, 1, 1}}; return x; }
LU_INLINE v3 xf_point(xform a, v3 p) { return v3_add(a.t, quat_rot(a.r, v3_mul(a.s, p))); }
/* a * b: сначала b (локальное), потом a (родитель) */
LU_INLINE xform xf_mul(xform a, xform b) {
    xform r; r.s = v3_mul(a.s, b.s); r.r = quat_norm(quat_mul(a.r, b.r)); r.t = xf_point(a, b.t); return r;
}
LU_INLINE xform xf_inv(xform a) {
    xform r; r.s = V3(1.f / a.s.x, 1.f / a.s.y, 1.f / a.s.z); r.r = quat_conj(a.r);
    r.t = v3_mul(r.s, quat_rot(r.r, v3_neg(a.t))); return r;
}
LU_INLINE xform xf_lerp(xform a, xform b, float t) {
    xform r; r.t = v3_lerp(a.t, b.t, t); r.r = quat_nlerp(a.r, b.r, t); r.s = v3_lerp(a.s, b.s, t); return r;
}

/* ---------- матрицы ---------- */
LU_INLINE m4 m4_id(void) { m4 r = {{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}}; return r; }
LU_INLINE m4 m4_mul(m4 a, m4 b) {
    m4 r;
    for (int c = 0; c < 4; c++)
        for (int rr = 0; rr < 4; rr++)
            r.m[c * 4 + rr] = a.m[0 * 4 + rr] * b.m[c * 4 + 0] + a.m[1 * 4 + rr] * b.m[c * 4 + 1] +
                              a.m[2 * 4 + rr] * b.m[c * 4 + 2] + a.m[3 * 4 + rr] * b.m[c * 4 + 3];
    return r;
}
LU_INLINE v4 m4_mul4(m4 a, v4 v) {
    return V4(a.m[0] * v.x + a.m[4] * v.y + a.m[8] * v.z + a.m[12] * v.w,
              a.m[1] * v.x + a.m[5] * v.y + a.m[9] * v.z + a.m[13] * v.w,
              a.m[2] * v.x + a.m[6] * v.y + a.m[10] * v.z + a.m[14] * v.w,
              a.m[3] * v.x + a.m[7] * v.y + a.m[11] * v.z + a.m[15] * v.w);
}
LU_INLINE v3 m4_point(m4 a, v3 p) { v4 r = m4_mul4(a, V4(p.x, p.y, p.z, 1)); return V3(r.x, r.y, r.z); }
LU_INLINE v3 m4_dir(m4 a, v3 p) { v4 r = m4_mul4(a, V4(p.x, p.y, p.z, 0)); return V3(r.x, r.y, r.z); }
LU_INLINE m4 m4_translate(v3 t) { m4 r = m4_id(); r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z; return r; }
LU_INLINE m4 m4_scale(v3 s) { m4 r = m4_id(); r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z; return r; }
LU_INLINE m4 m4_from_quat(quat q) {
    float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z, xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z,
          wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
    m4 r = {{1 - 2 * (yy + zz), 2 * (xy + wz), 2 * (xz - wy), 0,
             2 * (xy - wz), 1 - 2 * (xx + zz), 2 * (yz + wx), 0,
             2 * (xz + wy), 2 * (yz - wx), 1 - 2 * (xx + yy), 0,
             0, 0, 0, 1}};
    return r;
}
LU_INLINE m4 m4_from_xform(xform x) {
    m4 r = m4_from_quat(x.r);
    for (int i = 0; i < 3; i++) { r.m[i] *= x.s.x; r.m[4 + i] *= x.s.y; r.m[8 + i] *= x.s.z; }
    r.m[12] = x.t.x; r.m[13] = x.t.y; r.m[14] = x.t.z; return r;
}
LU_INLINE m4 m4_trs(v3 t, quat q, v3 s) { xform x = {t, q, s}; return m4_from_xform(x); }
LU_INLINE m4 m4_rot_y(float a) { return m4_from_quat(quat_axis(V3(0, 1, 0), a)); }
LU_INLINE m4 m4_persp(float fovy, float asp, float n, float f) {
    float t = 1.f / tanf(fovy * .5f); m4 r = {{0}};
    r.m[0] = t / asp; r.m[5] = t; r.m[10] = (f + n) / (n - f); r.m[11] = -1; r.m[14] = 2 * f * n / (n - f); return r;
}
LU_INLINE m4 m4_ortho(float l, float r_, float b, float t, float n, float f) {
    m4 r = m4_id();
    r.m[0] = 2 / (r_ - l); r.m[5] = 2 / (t - b); r.m[10] = -2 / (f - n);
    r.m[12] = -(r_ + l) / (r_ - l); r.m[13] = -(t + b) / (t - b); r.m[14] = -(f + n) / (f - n); return r;
}
LU_INLINE m4 m4_look(v3 eye, v3 at, v3 up) {
    v3 f = v3_norm(v3_sub(at, eye)), s = v3_norm(v3_cross(f, up)), u = v3_cross(s, f);
    m4 r = m4_id();
    r.m[0] = s.x; r.m[4] = s.y; r.m[8] = s.z;
    r.m[1] = u.x; r.m[5] = u.y; r.m[9] = u.z;
    r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
    r.m[12] = -v3_dot(s, eye); r.m[13] = -v3_dot(u, eye); r.m[14] = v3_dot(f, eye); return r;
}
m4 m4_inverse(m4 a); /* lu_core.c */
LU_INLINE m4 m4_transpose(m4 a) { m4 r; for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) r.m[i * 4 + j] = a.m[j * 4 + i]; return r; }

/* ---------- AABB и усечённая пирамида ---------- */
LU_INLINE aabb aabb_empty(void) { aabb b = {{1e30f, 1e30f, 1e30f}, {-1e30f, -1e30f, -1e30f}}; return b; }
LU_INLINE aabb aabb_add(aabb b, v3 p) { b.mn = v3_min(b.mn, p); b.mx = v3_max(b.mx, p); return b; }
LU_INLINE aabb aabb_union(aabb a, aabb b) { a.mn = v3_min(a.mn, b.mn); a.mx = v3_max(a.mx, b.mx); return a; }
LU_INLINE v3 aabb_center(aabb b) { return v3_scale(v3_add(b.mn, b.mx), .5f); }
LU_INLINE v3 aabb_half(aabb b) { return v3_scale(v3_sub(b.mx, b.mn), .5f); }
LU_INLINE int aabb_overlap(aabb a, aabb b) {
    return a.mn.x <= b.mx.x && a.mx.x >= b.mn.x && a.mn.y <= b.mx.y && a.mx.y >= b.mn.y && a.mn.z <= b.mx.z && a.mx.z >= b.mn.z;
}
aabb aabb_transform(aabb b, m4 m); /* lu_core.c */
typedef struct { v4 p[6]; } frustum;
frustum frustum_from(m4 vp);
int frustum_aabb(const frustum *f, aabb b);
int frustum_sphere(const frustum *f, v3 c, float r);

/* ---------- случайные числа (детерминированные) ---------- */
LU_INLINE uint32_t lu_hash(uint32_t x) { x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16; return x; }
LU_INLINE uint32_t lu_hash2(int a, int b) { return lu_hash((uint32_t)a * 73856093u ^ lu_hash((uint32_t)b * 19349663u)); }
LU_INLINE float lu_h01(uint32_t h) { return (h & 0xFFFFFF) / 16777216.f; }
typedef struct { uint64_t s; } lu_rng;
LU_INLINE uint32_t rng_u32(lu_rng *r) { r->s = r->s * 6364136223846793005ULL + 1442695040888963407ULL; return (uint32_t)(r->s >> 33); }
LU_INLINE float rng_f(lu_rng *r) { return (rng_u32(r) & 0xFFFFFF) / 16777216.f; }
LU_INLINE float rng_range(lu_rng *r, float a, float b) { return a + (b - a) * rng_f(r); }
#endif
