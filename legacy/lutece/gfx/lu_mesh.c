/* Lutèce Engine — построение мешей на процессоре и загрузка в видеопамять */
#include "lu_gfx.h"
#include "lu_gl.h"

void lu_mb_init(lu_mesh_builder *b) { memset(b, 0, sizeof *b); b->color = 0xFFFFFFFFu; b->skin_joint = -1; lu_mb_material(b, 0); }
void lu_mb_free(lu_mesh_builder *b) { lu_vec_free(b->v); lu_vec_free(b->idx); memset(b, 0, sizeof *b); }
void lu_mb_material(lu_mesh_builder *b, int material) {
    if (b->nsub > 0) {
        lu_submesh *s = &b->sub[b->nsub - 1];
        s->count = b->idx.n - s->first;
        if (s->count == 0) { s->material = material; return; }   /* пустую часть переиспользуем */
        if (s->material == material) return;
    }
    if (b->nsub >= 16) { LU_WARN("в меше больше 16 частей"); return; }
    lu_submesh s = {b->idx.n, 0, material}; b->sub[b->nsub++] = s;
}
int lu_mb_vertex(lu_mesh_builder *b, v3 p, v3 n, v2 uv) {
    lu_vertex v; memset(&v, 0, sizeof v);
    v.pos = p; v.nrm = n; v.uv = uv; v.color = b->color; v.tan = V4(1, 0, 0, 1);
    if (b->skin_joint >= 0) { v.joints[0] = (uint8_t)b->skin_joint; v.weights[0] = 255; }
    lu_vec_push(b->v, v); return b->v.n - 1;
}
void lu_mb_tri(lu_mesh_builder *b, int a, int c, int d) { lu_vec_push(b->idx, (uint32_t)a); lu_vec_push(b->idx, (uint32_t)c); lu_vec_push(b->idx, (uint32_t)d); }
void lu_mb_quad(lu_mesh_builder *b, v3 a, v3 c, v3 d, v3 e, v2 uv0, v2 uv1) {
    v3 n = v3_norm(v3_cross(v3_sub(c, a), v3_sub(e, a)));
    int i0 = lu_mb_vertex(b, a, n, V2(uv0.x, uv1.y)), i1 = lu_mb_vertex(b, c, n, V2(uv1.x, uv1.y));
    int i2 = lu_mb_vertex(b, d, n, V2(uv1.x, uv0.y)), i3 = lu_mb_vertex(b, e, n, V2(uv0.x, uv0.y));
    lu_mb_tri(b, i0, i1, i2); lu_mb_tri(b, i0, i2, i3);
}
void lu_mb_box(lu_mesh_builder *b, v3 mn, v3 mx, float s) {
    v3 p[8]; for (int i = 0; i < 8; i++) p[i] = V3(i & 1 ? mx.x : mn.x, i & 2 ? mx.y : mn.y, i & 4 ? mx.z : mn.z);
    float dx = (mx.x - mn.x) * s, dy = (mx.y - mn.y) * s, dz = (mx.z - mn.z) * s;
    float ux0 = mn.x * s, uz0 = mn.z * s, uy0 = mn.y * s;
    lu_mb_quad(b, p[4], p[5], p[7], p[6], V2(ux0, uy0), V2(ux0 + dx, uy0 + dy));          /* +Z */
    lu_mb_quad(b, p[1], p[0], p[2], p[3], V2(-ux0 - dx, uy0), V2(-ux0, uy0 + dy));        /* -Z */
    lu_mb_quad(b, p[5], p[1], p[3], p[7], V2(-uz0 - dz, uy0), V2(-uz0, uy0 + dy));        /* +X */
    lu_mb_quad(b, p[0], p[4], p[6], p[2], V2(uz0, uy0), V2(uz0 + dz, uy0 + dy));          /* -X */
    lu_mb_quad(b, p[2], p[6], p[7], p[3], V2(ux0, uz0), V2(ux0 + dx, uz0 + dz));          /* +Y */
    lu_mb_quad(b, p[0], p[1], p[5], p[4], V2(ux0, uz0), V2(ux0 + dx, uz0 + dz));          /* -Y */
}
void lu_mb_lathe(lu_mesh_builder *b, v3 base, const v2 *pr, int n, int seg) {
    int start = b->v.n; float vlen = 0;
    for (int i = 0; i < n; i++) {
        if (i) vlen += sqrtf((pr[i].x - pr[i - 1].x) * (pr[i].x - pr[i - 1].x) + (pr[i].y - pr[i - 1].y) * (pr[i].y - pr[i - 1].y));
        for (int s = 0; s <= seg; s++) {
            float a = LU_TAU * s / seg, sn = sinf(a), cs = cosf(a);
            lu_mb_vertex(b, V3(base.x + pr[i].x * sn, base.y + pr[i].y, base.z + pr[i].x * cs), V3(sn, 0, cs), V2((float)s / seg, vlen));
        }
    }
    for (int i = 0; i < n - 1; i++)
        for (int s = 0; s < seg; s++) {
            int a = start + i * (seg + 1) + s, c = a + seg + 1;
            lu_mb_tri(b, a, a + 1, c + 1); lu_mb_tri(b, a, c + 1, c);
        }
    lu_mb_compute_normals(b, start);
}
void lu_mb_cylinder(lu_mesh_builder *b, v3 base, float r0, float r1, float h, int seg, int caps) {
    v2 pr[2] = {{r0, 0}, {r1, h}};
    lu_mb_lathe(b, base, pr, 2, seg);
    if (caps) {
        for (int c = 0; c < 2; c++) {
            float r = c ? r1 : r0, y = base.y + (c ? h : 0); v3 n = V3(0, c ? 1.f : -1.f, 0);
            int ctr = lu_mb_vertex(b, V3(base.x, y, base.z), n, V2(.5f, .5f)), first = b->v.n;
            for (int s = 0; s <= seg; s++) { float a = LU_TAU * s / seg; lu_mb_vertex(b, V3(base.x + sinf(a) * r, y, base.z + cosf(a) * r), n, V2(.5f + sinf(a) * .5f, .5f + cosf(a) * .5f)); }
            for (int s = 0; s < seg; s++) { if (c) lu_mb_tri(b, ctr, first + s, first + s + 1); else lu_mb_tri(b, ctr, first + s + 1, first + s); }
        }
    }
}
void lu_mb_sphere(lu_mesh_builder *b, v3 c, v3 r, int seg, int ring) {
    int start = b->v.n;
    for (int j = 0; j <= ring; j++) {
        float t = LU_PI * j / ring, st = sinf(t), ct = cosf(t);
        for (int i = 0; i <= seg; i++) {
            float a = LU_TAU * i / seg; v3 n = V3(st * sinf(a), ct, st * cosf(a));
            lu_mb_vertex(b, V3(c.x + n.x * r.x, c.y + n.y * r.y, c.z + n.z * r.z), v3_norm(V3(n.x / r.x, n.y / r.y, n.z / r.z)), V2((float)i / seg, (float)j / ring));
        }
    }
    for (int j = 0; j < ring; j++)
        for (int i = 0; i < seg; i++) {
            int a = start + j * (seg + 1) + i, d = a + seg + 1;
            lu_mb_tri(b, a, d, a + 1); lu_mb_tri(b, a + 1, d, d + 1);
        }
}
void lu_mb_tube(lu_mesh_builder *b, v3 a, v3 c, float r0, float r1, int seg) {
    v3 ax = v3_sub(c, a); float len = v3_len(ax); if (len < 1e-6f) return;
    quat q = quat_from_to(V3(0, 1, 0), v3_scale(ax, 1.f / len));
    int start = b->v.n; lu_mb_cylinder(b, V3(0, 0, 0), r0, r1, len, seg, 0);
    lu_mb_transform(b, start, m4_trs(a, q, V3(1, 1, 1)));
}
void lu_mb_transform(lu_mesh_builder *b, int from, m4 m) {
    m4 nm = m4_transpose(m4_inverse(m));
    for (int i = from; i < b->v.n; i++) {
        lu_vertex *v = &b->v.v[i];
        v->pos = m4_point(m, v->pos); v->nrm = v3_norm(m4_dir(nm, v->nrm));
        v3 t = v3_norm(m4_dir(m, V3(v->tan.x, v->tan.y, v->tan.z))); v->tan = V4(t.x, t.y, t.z, v->tan.w);
    }
}
void lu_mb_compute_normals(lu_mesh_builder *b, int from) {
    for (int i = from; i < b->v.n; i++) b->v.v[i].nrm = V3(0, 0, 0);
    for (int k = 0; k + 2 < b->idx.n; k += 3) {
        uint32_t i0 = b->idx.v[k], i1 = b->idx.v[k + 1], i2 = b->idx.v[k + 2];
        if ((int)i0 < from || (int)i1 < from || (int)i2 < from) continue;
        v3 n = v3_cross(v3_sub(b->v.v[i1].pos, b->v.v[i0].pos), v3_sub(b->v.v[i2].pos, b->v.v[i0].pos));
        b->v.v[i0].nrm = v3_add(b->v.v[i0].nrm, n); b->v.v[i1].nrm = v3_add(b->v.v[i1].nrm, n); b->v.v[i2].nrm = v3_add(b->v.v[i2].nrm, n);
    }
    /* сварка нормалей совпадающих по позиции вершин (швы токарных тел) */
    for (int i = from; i < b->v.n; i++) {
        lu_vertex *v = &b->v.v[i];
        if (v3_len2(v->nrm) < 1e-20f) v->nrm = V3(0, 1, 0); else v->nrm = v3_norm(v->nrm);
    }
}
void lu_mb_compute_tangents(lu_mesh_builder *b) {
    int n = b->v.n; v3 *t1 = lu_alloc(sizeof(v3) * (size_t)n * 2), *t2 = t1 + n;
    for (int k = 0; k + 2 < b->idx.n; k += 3) {
        uint32_t i[3] = {b->idx.v[k], b->idx.v[k + 1], b->idx.v[k + 2]};
        lu_vertex *a = &b->v.v[i[0]], *c = &b->v.v[i[1]], *d = &b->v.v[i[2]];
        v3 e1 = v3_sub(c->pos, a->pos), e2 = v3_sub(d->pos, a->pos);
        float s1 = c->uv.x - a->uv.x, s2 = d->uv.x - a->uv.x, u1 = c->uv.y - a->uv.y, u2 = d->uv.y - a->uv.y;
        float r = s1 * u2 - s2 * u1; if (fabsf(r) < 1e-12f) continue; r = 1.f / r;
        v3 sd = v3_scale(v3_sub(v3_scale(e1, u2), v3_scale(e2, u1)), r), td = v3_scale(v3_sub(v3_scale(e2, s1), v3_scale(e1, s2)), r);
        for (int j = 0; j < 3; j++) { t1[i[j]] = v3_add(t1[i[j]], sd); t2[i[j]] = v3_add(t2[i[j]], td); }
    }
    for (int i = 0; i < n; i++) {
        lu_vertex *v = &b->v.v[i]; v3 nn = v->nrm, t = v3_reject(t1[i], nn);
        if (v3_len2(t) < 1e-12f) { t = v3_cross(fabsf(nn.y) < .9f ? V3(0, 1, 0) : V3(1, 0, 0), nn); }
        t = v3_norm(t); float w = v3_dot(v3_cross(nn, t), t2[i]) < 0 ? -1.f : 1.f;
        v->tan = V4(t.x, t.y, t.z, w);
    }
    lu_free(t1);
}

/* общий экземплярный буфер создаётся в lu_gfx.c; здесь — только привязка атрибутов */
extern unsigned lu__inst_vbo;
static void setup_vao(lu_mesh *m, const lu_vertex *v, int nv, const uint32_t *idx, int ni) {
    glGenVertexArrays(1, &m->vao); glBindVertexArray(m->vao);
    glGenBuffers(1, &m->vbo); glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(lu_vertex) * (size_t)nv), v, GL_STATIC_DRAW);
    glGenBuffers(1, &m->ibo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(sizeof(uint32_t) * (size_t)ni), idx, GL_STATIC_DRAW);
    GLsizei st = sizeof(lu_vertex);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, st, (void *)offsetof(lu_vertex, pos));
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, st, (void *)offsetof(lu_vertex, nrm));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, st, (void *)offsetof(lu_vertex, tan));
    glEnableVertexAttribArray(3); glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, st, (void *)offsetof(lu_vertex, uv));
    glEnableVertexAttribArray(4); glVertexAttribPointer(4, 4, GL_UNSIGNED_BYTE, GL_TRUE, st, (void *)offsetof(lu_vertex, color));
    glEnableVertexAttribArray(5); glVertexAttribIPointer(5, 4, GL_UNSIGNED_BYTE, st, (void *)offsetof(lu_vertex, joints));
    glEnableVertexAttribArray(6); glVertexAttribPointer(6, 4, GL_UNSIGNED_BYTE, GL_TRUE, st, (void *)offsetof(lu_vertex, weights));
    glBindBuffer(GL_ARRAY_BUFFER, lu__inst_vbo);
    for (int k = 7; k <= 12; k++) { glEnableVertexAttribArray((GLuint)k); glVertexAttribDivisor((GLuint)k, 1); }
    glBindVertexArray(0);
}
lu_mesh lu_mesh_create(lu_mesh_builder *b, int skinned) {
    lu_mesh m; memset(&m, 0, sizeof m);
    if (b->nsub > 0) b->sub[b->nsub - 1].count = b->idx.n - b->sub[b->nsub - 1].first;
    int k = 0; for (int i = 0; i < b->nsub; i++) if (b->sub[i].count > 0) m.sub[k++] = b->sub[i]; m.nsub = k;
    m.nverts = b->v.n; m.nidx = b->idx.n; m.skinned = skinned;
    m.bounds = aabb_empty(); for (int i = 0; i < b->v.n; i++) m.bounds = aabb_add(m.bounds, b->v.v[i].pos);
    if (!b->v.n || !b->idx.n) return m;
    setup_vao(&m, b->v.v, b->v.n, b->idx.v, b->idx.n);
    return m;
}
void lu_mesh_destroy(lu_mesh *m) {
    if (m->vao) { glDeleteVertexArrays(1, &m->vao); glDeleteBuffers(1, &m->vbo); glDeleteBuffers(1, &m->ibo); }
    memset(m, 0, sizeof *m);
}
