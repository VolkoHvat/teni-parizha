/* Lutèce Engine — рендер кадра */
#include "lu_gfx.h"
#include "lu_gl.h"
#include "lu_shaders.h"
#include <stdio.h>

/* ================= состояние ================= */
static char gerr[4096];
const char *lu_gfx_error(void) { return gerr; }
lu_gfx_quality lu_gfx_q = {1.f, 2048, 3, 1, 1, 1, 140.f};

typedef struct { unsigned fbo, tex, depth; int w, h; } Target;
enum { P_MAIN, P_DEPTH, P_SKY, P_SSAO, P_BLUR, P_BRIGHT, P_DOWN, P_UP, P_COMP, P_FXAA, P_UI, P_LINE, P_N };
static GLuint prog[P_N];
static GLuint ubo_frame, vao_empty, bone_buf, bone_tex, ui_vao, ui_vbo, line_vao, line_vbo;
unsigned lu__inst_vbo;
static GLuint shadow_tex, shadow_fbo[4]; static int shadow_size_cur, shadow_casc_cur;
static Target t_hdr, t_ao, t_ao2, t_ldr, t_bloom[7]; static int nbloom;
static lu_texture tex_white, tex_flatn, tex_font;
static int out_w = 1280, out_h = 720, rw, rh; static unsigned out_fbo;
static lu_camera cam; static lu_env env; static m4 m_view, m_proj, m_vp, m_prev_vp;
static lu_gfx_stats stats; static double frame_t0;

typedef struct {
    const lu_mesh *m; const lu_material *mat; int first, count, skinned;
    m4 w; v4 tint; float bone; aabb wb; float dist;
} Cmd;
static LU_VEC(Cmd) cmds;
static LU_VEC(float) bones;            /* матрицы костей кадра (по 16 float) */
static LU_VEC(float) inst;             /* данные инстансов кадра */
typedef struct { int start, n; int inst_off; } Run;   /* серия одинаковых команд в списке */
static LU_VEC(int) lst_main, lst_trans, lst_sh[4];
static LU_VEC(Run) runs_main, runs_trans, runs_sh[4];
typedef struct { v3 p; float r; v3 c; float d; } PLight;
static LU_VEC(PLight) plights;
typedef struct { float x, y, z; uint32_t c; } LVert;
static LU_VEC(LVert) lines;
typedef struct { float x, y, u, v; uint32_t c; float m, p; } UIVert;
typedef struct { GLuint tex; int first, count; } UIBatch;
static LU_VEC(UIVert) uiv; static LU_VEC(UIBatch) uib;
#define INST_FLOATS 21

typedef struct {
    float view[16], proj[16], vp[16], ivp[16], pvp[16];
    float shadow_vp[4][16];
    float cam[4], sun_dir[4], sun_col[4], sky_zen[4], sky_hor[4], ground[4], fog[4], p1[4], p2[4], splits[4], shadow_p[4], screen[4], light_n[4];
    float lights[128][4];
} FrameUBO;
static FrameUBO fu;

/* ================= шейдеры ================= */
static GLuint compile(GLenum type, const char *const *src, int n, const char *name) {
    GLuint s = glCreateShader(type); glShaderSource(s, n, src, NULL); glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[2048]; glGetShaderInfoLog(s, sizeof log, NULL, log); snprintf(gerr, sizeof gerr, "шейдер %s (%s):\n%s", name, type == GL_VERTEX_SHADER ? "вершины" : "пиксели", log); LU_ERR("%s", gerr); return 0; }
    return s;
}
static GLuint link(const char *name, const char *const *vs, int nvs, const char *const *fs, int nfs) {
    GLuint v = compile(GL_VERTEX_SHADER, vs, nvs, name), f = compile(GL_FRAGMENT_SHADER, fs, nfs, name);
    if (!v || !f) return 0;
    GLuint p = glCreateProgram(); glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    GLint ok; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { char log[2048]; glGetProgramInfoLog(p, sizeof log, NULL, log); snprintf(gerr, sizeof gerr, "сборка %s:\n%s", name, log); LU_ERR("%s", gerr); return 0; }
    glDeleteShader(v); glDeleteShader(f);
    GLuint bi = glGetUniformBlockIndex(p, "Frame"); if (bi != GL_INVALID_INDEX) glUniformBlockBinding(p, bi, 0);
    return p;
}
static GLint U(GLuint p, const char *n) { return glGetUniformLocation(p, n); }
static void samplers(GLuint p) {
    glUseProgram(p);
    static const char *names[] = {"tAlb", "tNrm", "tORM", "tAO", "tShadow", "tBones", "tSrc", "tDepth", "tHDR", "tBloom", "tTex"};
    static const int units[] = {0, 1, 2, 3, 4, 5, 0, 1, 0, 1, 0};
    for (int i = 0; i < LU_ARRAY_N(names); i++) { GLint l = U(p, names[i]); if (l >= 0) glUniform1i(l, units[i]); }
}
static int build_programs(void) {
    const char *vs_main[] = {GLSL_HEAD, "invariant gl_Position;\n", VS_MAIN};
    const char *fs_main[] = {GLSL_HEAD, GLSL_LIGHTING, FS_MAIN};
    const char *fs_depth[] = {GLSL_HEAD, FS_DEPTH};
    const char *vs_full[] = {GLSL_HEAD, VS_FULL};
    const char *fs_sky[] = {GLSL_HEAD, GLSL_LIGHTING, FS_SKY};
    struct { int id; const char *name; const char *const *vs; int nvs; const char *const *fs; int nfs; } L[] = {
        {P_MAIN, "основной", vs_main, 3, fs_main, 3}, {P_DEPTH, "глубина", vs_main, 3, fs_depth, 2}, {P_SKY, "небо", vs_full, 2, fs_sky, 3}};
    for (int i = 0; i < LU_ARRAY_N(L); i++) if (!(prog[L[i].id] = link(L[i].name, L[i].vs, L[i].nvs, L[i].fs, L[i].nfs))) return 0;
    struct { int id; const char *name; const char *fs; } F[] = {{P_SSAO, "ssao", FS_SSAO}, {P_BLUR, "размытие", FS_BLUR}, {P_BRIGHT, "яркие", FS_BRIGHT},
        {P_DOWN, "уменьшение", FS_DOWN}, {P_UP, "увеличение", FS_UP}, {P_COMP, "итог", FS_COMPOSITE}, {P_FXAA, "fxaa", FS_FXAA}};
    for (int i = 0; i < LU_ARRAY_N(F); i++) { const char *fs[] = {GLSL_HEAD, F[i].fs}; if (!(prog[F[i].id] = link(F[i].name, vs_full, 2, fs, 2))) return 0; }
    { const char *vs[] = {VS_UI}, *fs[] = {FS_UI}; if (!(prog[P_UI] = link("интерфейс", vs, 1, fs, 1))) return 0; }
    { const char *vs[] = {GLSL_HEAD, VS_LINE}, *fs[] = {GLSL_HEAD, FS_LINE}; if (!(prog[P_LINE] = link("линии", vs, 2, fs, 2))) return 0; }
    for (int i = 0; i < P_N; i++) samplers(prog[i]);
    return 1;
}

/* ================= текстуры и цели рисования ================= */
lu_texture lu_texture_create(int w, int h, const void *px, int flags) {
    lu_texture t = {0, w, h, flags};
    glGenTextures(1, &t.id); glBindTexture(GL_TEXTURE_2D, t.id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (flags & LU_TEX_R8) glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, px);
    else glTexImage2D(GL_TEXTURE_2D, 0, (flags & LU_TEX_SRGB) ? GL_SRGB8_ALPHA8 : GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    GLint wrap = (flags & LU_TEX_CLAMP) ? GL_CLAMP_TO_EDGE : GL_REPEAT;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    if (flags & LU_TEX_NEAREST) { glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); }
    else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (flags & LU_TEX_MIPS) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        if (flags & LU_TEX_MIPS) { glGenerateMipmap(GL_TEXTURE_2D); glTexParameterf(GL_TEXTURE_2D, 0x84FE /* ANISOTROPY */, 8.f); glGetError(); }
    }
    return t;
}
void lu_texture_update(lu_texture *t, const void *px) {
    glBindTexture(GL_TEXTURE_2D, t->id); glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (t->flags & LU_TEX_R8) glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, t->w, t->h, GL_RED, GL_UNSIGNED_BYTE, px);
    else glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, t->w, t->h, GL_RGBA, GL_UNSIGNED_BYTE, px);
    if (t->flags & LU_TEX_MIPS) glGenerateMipmap(GL_TEXTURE_2D);
}
void lu_texture_destroy(lu_texture *t) { if (t->id) glDeleteTextures(1, &t->id); t->id = 0; }
static void target_free(Target *t) {
    if (t->fbo) glDeleteFramebuffers(1, &t->fbo);
    if (t->tex) glDeleteTextures(1, &t->tex);
    if (t->depth) glDeleteTextures(1, &t->depth);
    memset(t, 0, sizeof *t);
}
static void target_make(Target *t, int w, int h, GLenum ifmt, GLenum fmt, GLenum type, int depth) {
    target_free(t); t->w = w; t->h = h;
    glGenFramebuffers(1, &t->fbo); glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    glGenTextures(1, &t->tex); glBindTexture(GL_TEXTURE_2D, t->tex);
    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)ifmt, w, h, 0, fmt, type, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t->tex, 0);
    if (depth) {
        glGenTextures(1, &t->depth); glBindTexture(GL_TEXTURE_2D, t->depth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, t->depth, 0);
    }
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) LU_ERR("кадровый буфер %dx%d неполон", w, h);
}
static void shadow_make(int size, int casc) {
    if (shadow_tex && shadow_size_cur == size && shadow_casc_cur == casc) return;
    if (shadow_tex) { glDeleteTextures(1, &shadow_tex); glDeleteFramebuffers(4, shadow_fbo); }
    glGenTextures(1, &shadow_tex); glBindTexture(GL_TEXTURE_2D_ARRAY, shadow_tex);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, size, size, casc, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE); glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glGenFramebuffers(4, shadow_fbo);
    for (int i = 0; i < casc; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, shadow_fbo[i]);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadow_tex, 0, i);
        glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);
    }
    shadow_size_cur = size; shadow_casc_cur = casc;
}
static void ensure_targets(void) {
    int w = (int)(out_w * lu_gfx_q.render_scale + .5f), h = (int)(out_h * lu_gfx_q.render_scale + .5f);
    w = w < 16 ? 16 : w; h = h < 16 ? 16 : h;
    if (w == rw && h == rh && t_hdr.fbo) return;
    rw = w; rh = h;
    target_make(&t_hdr, w, h, GL_RGBA16F, GL_RGBA, GL_FLOAT, 1);
    target_make(&t_ao, w / 2, h / 2, GL_R8, GL_RED, GL_UNSIGNED_BYTE, 0);
    target_make(&t_ao2, w / 2, h / 2, GL_R8, GL_RED, GL_UNSIGNED_BYTE, 0);
    target_make(&t_ldr, w, h, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    nbloom = 0; int bw = w / 2, bh = h / 2;
    for (int i = 0; i < 7 && bw >= 8 && bh >= 8; i++) { target_make(&t_bloom[i], bw, bh, GL_R11F_G11F_B10F, GL_RGB, GL_FLOAT, 0); nbloom++; bw /= 2; bh /= 2; }
}
void lu_gfx_set_quality(int q) {
    static const lu_gfx_quality Q[4] = {{.7f, 1024, 2, 0, 0, 1, 80.f}, {.85f, 2048, 3, 1, 1, 1, 120.f}, {1.f, 2048, 3, 1, 1, 1, 150.f}, {1.f, 4096, 4, 1, 1, 1, 220.f}};
    lu_gfx_q = Q[q < 0 ? 0 : q > 3 ? 3 : q]; rw = 0;
}

/* ================= шрифт ================= */
extern const int lu_font_w, lu_font_h, lu_font_px, lu_font_spread, lu_font_ascent, lu_font_descent, lu_font_n;
extern const float lu_font_glyphs[][8];
extern const unsigned char lu_font_atlas[];
static const float *glyph(int cp) {
    int lo = 0, hi = lu_font_n - 1;
    while (lo <= hi) { int mid = (lo + hi) / 2; int c = (int)lu_font_glyphs[mid][0]; if (c == cp) return lu_font_glyphs[mid]; if (c < cp) lo = mid + 1; else hi = mid - 1; }
    return cp == '?' ? NULL : glyph('?');
}

/* ================= инициализация ================= */
lu_material lu_material_default(void) {
    lu_material m; memset(&m, 0, sizeof m); m.color = V4(1, 1, 1, 1); m.roughness = .7f; m.metallic = 0; m.normal_scale = 1; m.uv_scale = 1; return m;
}
lu_env lu_env_default(void) {
    lu_env e; memset(&e, 0, sizeof e);
    e.sun_dir = v3_norm(V3(-.4f, .55f, .35f)); e.sun_color = V3(3.2f, 3.0f, 2.7f);
    e.sky_zenith = V3(.18f, .36f, .8f); e.sky_horizon = V3(.62f, .74f, .92f); e.ground = V3(.25f, .23f, .2f);
    e.fog_color = V3(.55f, .62f, .74f); e.fog_density = .004f; e.fog_height_falloff = .045f;
    e.exposure = .6f; e.bloom = .06f; e.wind = .4f; e.cloud_cover = .35f; e.ssao_strength = 1.f; return e;
}
int lu_gfx_init(void) {
    gerr[0] = 0;
    if (!build_programs()) return 1;
    glGenBuffers(1, &ubo_frame); glBindBuffer(GL_UNIFORM_BUFFER, ubo_frame);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(FrameUBO), NULL, GL_DYNAMIC_DRAW); glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo_frame);
    glGenVertexArrays(1, &vao_empty);
    glGenBuffers(1, &lu__inst_vbo); glBindBuffer(GL_ARRAY_BUFFER, lu__inst_vbo); glBufferData(GL_ARRAY_BUFFER, 1 << 20, NULL, GL_STREAM_DRAW);
    glGenBuffers(1, &bone_buf); glBindBuffer(GL_TEXTURE_BUFFER, bone_buf); glBufferData(GL_TEXTURE_BUFFER, 64 * 16 * 4, NULL, GL_STREAM_DRAW);
    glGenTextures(1, &bone_tex); glBindTexture(GL_TEXTURE_BUFFER, bone_tex); glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, bone_buf);
    uint32_t white = 0xFFFFFFFFu, flat = 0xFFFF8080u;
    tex_white = lu_texture_create(1, 1, &white, 0); tex_flatn = lu_texture_create(1, 1, &flat, 0);
    tex_font = lu_texture_create(lu_font_w, lu_font_h, lu_font_atlas, LU_TEX_R8 | LU_TEX_CLAMP | LU_TEX_MIPS);
    /* интерфейс */
    glGenVertexArrays(1, &ui_vao); glBindVertexArray(ui_vao); glGenBuffers(1, &ui_vbo); glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVert), (void *)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(UIVert), (void *)8);
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(UIVert), (void *)16);
    glEnableVertexAttribArray(3); glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(UIVert), (void *)20);
    glGenVertexArrays(1, &line_vao); glBindVertexArray(line_vao); glGenBuffers(1, &line_vbo); glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LVert), (void *)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(LVert), (void *)12);
    glBindVertexArray(0);
    env = lu_env_default(); m_prev_vp = m4_id();
    LU_INFO("Lutèce: рендер готов");
    return 0;
}
void lu_gfx_shutdown(void) {
    for (int i = 0; i < P_N; i++) if (prog[i]) glDeleteProgram(prog[i]);
    target_free(&t_hdr); target_free(&t_ao); target_free(&t_ao2); target_free(&t_ldr); for (int i = 0; i < nbloom; i++) target_free(&t_bloom[i]);
    lu_vec_free(cmds); lu_vec_free(bones); lu_vec_free(inst); lu_vec_free(uiv); lu_vec_free(uib); lu_vec_free(lines); lu_vec_free(plights);
}

/* ================= сбор кадра ================= */
void lu_gfx_begin(int w, int h, unsigned fbo) {
    out_w = w < 1 ? 1 : w; out_h = h < 1 ? 1 : h; out_fbo = fbo; frame_t0 = lu_time();
    cmds.n = 0; bones.n = 0; plights.n = 0; lines.n = 0; uiv.n = 0; uib.n = 0;
    memset(&stats, 0, sizeof stats);
}
void lu_gfx_camera(const lu_camera *c) {
    cam = *c; if (cam.fovy <= 0) cam.fovy = 1.f; if (cam.znear <= 0) cam.znear = .1f; if (cam.zfar <= cam.znear) cam.zfar = 1000.f;
    if (v3_len2(cam.up) < 1e-6f) cam.up = V3(0, 1, 0);
    m_view = m4_look(cam.pos, cam.target, cam.up);
    m_proj = m4_persp(cam.fovy, (float)out_w / (float)out_h, cam.znear, cam.zfar);
    m_vp = m4_mul(m_proj, m_view);
}
void lu_gfx_env(const lu_env *e) { env = *e; }
m4 lu_gfx_viewproj(void) { return m_vp; }
int lu_gfx_project(v3 p, float *sx, float *sy) {
    v4 c = m4_mul4(m_vp, V4(p.x, p.y, p.z, 1)); if (c.w < .05f) return 0;
    *sx = (c.x / c.w * .5f + .5f) * out_w; *sy = (1 - (c.y / c.w * .5f + .5f)) * out_h; return 1;
}
static void push_cmds(const lu_mesh *m, const lu_material *mats, int nmats, m4 w, v4 tint, float bone, int skinned) {
    if (!m || !m->vao || !m->nsub) return;
    aabb wb = aabb_transform(m->bounds, w);
    if (skinned) { v3 c = aabb_center(wb), h = v3_scale(aabb_half(wb), 1.6f); wb.mn = v3_sub(c, h); wb.mx = v3_add(c, h); } /* позы выходят за исходные границы */
    float dist = v3_dist(aabb_center(wb), cam.pos);
    for (int i = 0; i < m->nsub; i++) {
        const lu_submesh *s = &m->sub[i]; Cmd c;
        int mi = s->material; if (mi >= nmats) mi = nmats - 1; if (mi < 0) mi = 0;
        static lu_material defmat; static int defmat_ok; if (!defmat_ok) { defmat = lu_material_default(); defmat_ok = 1; }
        c.m = m; c.mat = nmats > 0 ? &mats[mi] : &defmat; c.first = s->first; c.count = s->count; c.skinned = skinned;
        c.w = w; c.tint = tint; c.bone = bone; c.wb = wb; c.dist = dist;
        lu_vec_push(cmds, c);
    }
}
void lu_draw(const lu_mesh *m, const lu_material *mats, int nmats, m4 w) { push_cmds(m, mats, nmats, w, V4(1, 1, 1, 1), 0, 0); }
void lu_draw_tinted(const lu_mesh *m, const lu_material *mats, int nmats, m4 w, v4 tint) { push_cmds(m, mats, nmats, w, tint, 0, 0); }
void lu_draw_lod(const lu_lod *l, const lu_material *mats, int nmats, m4 w) {
    if (!l->n) return;
    v3 c = m4_point(w, aabb_center(l->lv[0]->bounds)); float d = v3_dist(c, cam.pos);
    for (int k = 0; k < l->n; k++) if (d < l->dist[k] || k == l->n - 1) { if (d < l->dist[k] || l->dist[k] <= 0) push_cmds(l->lv[k], mats, nmats, w, V4(1, 1, 1, 1), 0, 0); return; }
}
void lu_draw_skinned(const lu_mesh *m, const lu_material *mats, int nmats, m4 w, const m4 *pal, int nb) {
    if (nb > LU_MAX_BONES) nb = LU_MAX_BONES;
    int base = bones.n / 16;
    lu_vec_reserve(bones, bones.n + nb * 16 + 16);
    for (int i = 0; i < nb; i++) { memcpy(&bones.v[bones.n], pal[i].m, 64); bones.n += 16; }
    push_cmds(m, mats, nmats, w, V4(1, 1, 1, 1), (float)base, 1); stats.skinned++;
}
void lu_light(v3 p, float r, v3 c) { PLight l = {p, r, c, v3_dist(p, cam.pos)}; lu_vec_push(plights, l); }
void lu_debug_line(v3 a, v3 b, uint32_t c) { LVert x = {a.x, a.y, a.z, c}, y = {b.x, b.y, b.z, c}; lu_vec_push(lines, x); lu_vec_push(lines, y); }
void lu_debug_box(aabb b, uint32_t c) {
    v3 p[8]; for (int i = 0; i < 8; i++) p[i] = V3(i & 1 ? b.mx.x : b.mn.x, i & 2 ? b.mx.y : b.mn.y, i & 4 ? b.mx.z : b.mn.z);
    static const int E[12][2] = {{0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3}, {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    for (int i = 0; i < 12; i++) lu_debug_line(p[E[i][0]], p[E[i][1]], c);
}
void lu_debug_sphere(v3 c, float r, uint32_t col) {
    for (int ax = 0; ax < 3; ax++) for (int i = 0; i < 24; i++) {
        float a0 = LU_TAU * i / 24, a1 = LU_TAU * (i + 1) / 24; v3 p0, p1;
        if (ax == 0) { p0 = V3(0, cosf(a0), sinf(a0)); p1 = V3(0, cosf(a1), sinf(a1)); }
        else if (ax == 1) { p0 = V3(cosf(a0), 0, sinf(a0)); p1 = V3(cosf(a1), 0, sinf(a1)); }
        else { p0 = V3(cosf(a0), sinf(a0), 0); p1 = V3(cosf(a1), sinf(a1), 0); }
        lu_debug_line(v3_mad(c, p0, r), v3_mad(c, p1, r), col);
    }
}

/* ================= сортировка и серии инстансов ================= */
static int same_batch(const Cmd *a, const Cmd *b) { return a->m == b->m && a->first == b->first && a->mat == b->mat && a->skinned == b->skinned; }
static const Cmd *sort_base;
static int cmp_opaque(const void *x, const void *y) {
    const Cmd *a = &sort_base[*(const int *)x], *b = &sort_base[*(const int *)y];
    if (a->skinned != b->skinned) return a->skinned - b->skinned;
    if (a->mat != b->mat) return a->mat < b->mat ? -1 : 1;
    if (a->m != b->m) return a->m < b->m ? -1 : 1;
    if (a->first != b->first) return a->first - b->first;
    return a->dist < b->dist ? -1 : a->dist > b->dist;
}
static int cmp_far_first(const void *x, const void *y) {
    const Cmd *a = &sort_base[*(const int *)x], *b = &sort_base[*(const int *)y];
    return a->dist > b->dist ? -1 : a->dist < b->dist;
}
static void make_runs(int *ix, int n, void *runs_vec, int allow_instancing) {
    LU_VEC(Run) *R = runs_vec; R->n = 0;
    for (int k = 0; k < n;) {
        int e = k + 1;
        if (allow_instancing) while (e < n && same_batch(&cmds.v[ix[k]], &cmds.v[ix[e]])) e++;
        Run r = {k, e - k, inst.n / INST_FLOATS};
        lu_vec_reserve(inst, inst.n + (e - k) * INST_FLOATS + INST_FLOATS);
        for (int j = k; j < e; j++) {
            const Cmd *c = &cmds.v[ix[j]]; float *o = &inst.v[inst.n];
            memcpy(o, c->w.m, 64); o[16] = c->tint.x; o[17] = c->tint.y; o[18] = c->tint.z; o[19] = c->tint.w; o[20] = c->bone;
            inst.n += INST_FLOATS;
        }
        lu_vec_push(*R, r); k = e;
    }
}

/* ================= проходы ================= */
static GLint um_flags, um_hasalb, um_hasnrm, um_hasorm, um_color, um_mat, um_emis, um_skinned, um_shpass, um_shm;
static GLint ud_flags, ud_hasalb, ud_color, ud_mat, ud_skinned, ud_shpass, ud_shm;
static const lu_material *bound_mat; static int bound_twosided = -1;
static void bind_material(const lu_material *m, int depth_only) {
    if (m == bound_mat) return;
    bound_mat = m;
    int two = (m->flags & (LU_MAT_TWOSIDED | LU_MAT_CUTOUT)) != 0;
    if (two != bound_twosided) { if (two) glDisable(GL_CULL_FACE); else glEnable(GL_CULL_FACE); bound_twosided = two; }
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m->albedo ? m->albedo->id : tex_white.id);
    if (depth_only) {
        glUniform1i(ud_flags, m->flags); glUniform1i(ud_hasalb, m->albedo != NULL);
        glUniform4f(ud_color, m->color.x, m->color.y, m->color.z, m->color.w);
        glUniform4f(ud_mat, m->roughness, m->metallic, m->normal_scale, m->uv_scale > 0 ? m->uv_scale : 1);
        return;
    }
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m->normal ? m->normal->id : tex_flatn.id);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, m->orm ? m->orm->id : tex_white.id);
    glUniform1i(um_flags, m->flags); glUniform1i(um_hasalb, m->albedo != NULL); glUniform1i(um_hasnrm, m->normal != NULL); glUniform1i(um_hasorm, m->orm != NULL);
    glUniform4f(um_color, m->color.x, m->color.y, m->color.z, m->color.w);
    glUniform4f(um_mat, m->roughness, m->metallic, m->normal_scale, m->uv_scale > 0 ? m->uv_scale : 1);
    glUniform3f(um_emis, m->emissive.x, m->emissive.y, m->emissive.z);
}
static void draw_runs(int *ix, void *runs_vec, int depth_only, GLint uskin) {
    LU_VEC(Run) *R = runs_vec; int cur_sk = -1;
    for (int r = 0; r < R->n; r++) {
        const Run *ru = &R->v[r]; const Cmd *c = &cmds.v[ix[ru->start]];
        if (depth_only && (c->mat->flags & (LU_MAT_BLEND | LU_MAT_UNLIT | LU_MAT_NOSHADOW | LU_MAT_WATER | LU_MAT_ADDITIVE))) continue;
        bind_material(c->mat, depth_only);
        if (c->skinned != cur_sk) { glUniform1i(uskin, c->skinned); cur_sk = c->skinned; }
        glBindVertexArray(c->m->vao);
        glBindBuffer(GL_ARRAY_BUFFER, lu__inst_vbo);
        size_t off = (size_t)ru->inst_off * INST_FLOATS * 4; GLsizei st = INST_FLOATS * 4;
        for (int k = 0; k < 4; k++) glVertexAttribPointer((GLuint)(7 + k), 4, GL_FLOAT, GL_FALSE, st, (void *)(off + (size_t)k * 16));
        glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, st, (void *)(off + 64));
        glVertexAttribPointer(12, 1, GL_FLOAT, GL_FALSE, st, (void *)(off + 80));
        glDrawElementsInstanced(GL_TRIANGLES, c->count, GL_UNSIGNED_INT, (void *)((size_t)c->first * 4), ru->n);
        stats.draws++; stats.instances += ru->n; stats.tris += c->count / 3 * ru->n;
    }
}
static void fullscreen(void) { glBindVertexArray(vao_empty); glDrawArrays(GL_TRIANGLES, 0, 3); }
static void bind_target(Target *t) { glBindFramebuffer(GL_FRAMEBUFFER, t ? t->fbo : out_fbo); if (t) glViewport(0, 0, t->w, t->h); else glViewport(0, 0, out_w, out_h); }

/* каскады теней: стабильные (вписанная сфера + привязка к текселям) */
static void compute_cascades(m4 *shvp, float *splits, int nc) {
    float n = cam.znear, f = fminf(lu_gfx_q.shadow_dist, cam.zfar), lambda = .78f;
    v3 L = v3_norm(env.sun_dir); if (L.y < .05f) { L.y = .05f; L = v3_norm(L); }
    float prev = n;
    m4 inv_view = m4_inverse(m_view); float th = tanf(cam.fovy * .5f), asp = (float)out_w / out_h;
    for (int c = 0; c < nc; c++) {
        float t = (float)(c + 1) / nc, lg = n * powf(f / n, t), un = n + (f - n) * t, sp = lambda * lg + (1 - lambda) * un;
        splits[c] = sp;
        /* углы среза в мире */
        v3 corners[8]; int k = 0;
        for (int zi = 0; zi < 2; zi++) { float z = zi ? sp : prev;
            for (int yi = -1; yi <= 1; yi += 2) for (int xi = -1; xi <= 1; xi += 2) corners[k++] = m4_point(inv_view, V3(xi * z * th * asp, yi * z * th, -z)); }
        v3 ctr = V3(0, 0, 0); for (int i = 0; i < 8; i++) ctr = v3_add(ctr, corners[i]); ctr = v3_scale(ctr, 1.f / 8);
        float rad = 0; for (int i = 0; i < 8; i++) rad = fmaxf(rad, v3_dist(ctr, corners[i]));
        rad = ceilf(rad * 16.f) / 16.f;
        float texel = rad * 2 / lu_gfx_q.shadow_size;
        v3 up = fabsf(L.y) > .99f ? V3(0, 0, 1) : V3(0, 1, 0);
        m4 lv = m4_look(V3(0, 0, 0), v3_neg(L), up);
        v3 lc = m4_point(lv, ctr); lc.x = floorf(lc.x / texel) * texel; lc.y = floorf(lc.y / texel) * texel;
        m4 lv_inv = m4_inverse(lv); ctr = m4_point(lv_inv, lc);
        float back = 160.f;
        m4 view = m4_look(v3_mad(ctr, L, rad + back), ctr, up);
        m4 proj = m4_ortho(-rad, rad, -rad, rad, .1f, 2 * rad + back + 40.f);
        shvp[c] = m4_mul(proj, view);
        prev = sp;
    }
    for (int c = nc; c < 4; c++) { splits[c] = 1e9f; shvp[c] = shvp[nc - 1]; }
}

static int light_cmp(const void *a, const void *b) { float x = ((const PLight *)a)->d, y = ((const PLight *)b)->d; return x < y ? -1 : x > y; }

static void ui_flush_draw(void);
static void lines_draw(void);

void lu_gfx_end(void) {
    ensure_targets();
    int nc = lu_gfx_q.cascades < 1 ? 1 : lu_gfx_q.cascades > 4 ? 4 : lu_gfx_q.cascades;
    int shadows_on = lu_gfx_q.shadow_size > 0 && env.sun_dir.y > -.05f;
    if (shadows_on) shadow_make(lu_gfx_q.shadow_size, nc);
    m4 shvp[4]; float splits[4]; compute_cascades(shvp, splits, nc);

    /* ---- данные кадра ---- */
    memcpy(fu.view, m_view.m, 64); memcpy(fu.proj, m_proj.m, 64); memcpy(fu.vp, m_vp.m, 64);
    m4 ivp = m4_inverse(m_vp); memcpy(fu.ivp, ivp.m, 64); memcpy(fu.pvp, m_prev_vp.m, 64); m_prev_vp = m_vp;
    for (int c = 0; c < 4; c++) memcpy(fu.shadow_vp[c], shvp[c].m, 64);
    v3 sd = v3_norm(env.sun_dir);
#define SET4(a, x, y, z, w) do { a[0] = (x); a[1] = (y); a[2] = (z); a[3] = (w); } while (0)
    SET4(fu.cam, cam.pos.x, cam.pos.y, cam.pos.z, cam.znear); SET4(fu.sun_dir, sd.x, sd.y, sd.z, cam.zfar);
    SET4(fu.sun_col, env.sun_color.x, env.sun_color.y, env.sun_color.z, 0);
    SET4(fu.sky_zen, env.sky_zenith.x, env.sky_zenith.y, env.sky_zenith.z, 0); SET4(fu.sky_hor, env.sky_horizon.x, env.sky_horizon.y, env.sky_horizon.z, 0);
    SET4(fu.ground, env.ground.x, env.ground.y, env.ground.z, 0); SET4(fu.fog, env.fog_color.x, env.fog_color.y, env.fog_color.z, env.fog_density);
    SET4(fu.p1, env.time, env.wind, env.fog_height_falloff > 0 ? env.fog_height_falloff : .04f, env.exposure);
    SET4(fu.p2, env.night, env.wetness, env.cloud_cover, lu_gfx_q.ssao ? env.ssao_strength : 0);
    SET4(fu.splits, splits[0], splits[1], splits[2], splits[3]);
    SET4(fu.shadow_p, shadows_on ? 1.f / lu_gfx_q.shadow_size : 0, (float)nc, shadows_on ? 1.f : 0.f, lu_gfx_q.ssao ? 1.f : 0.f);
    SET4(fu.screen, (float)rw, (float)rh, 1.f / rw, 1.f / rh);
    if (plights.n > 1) qsort(plights.v, (size_t)plights.n, sizeof(PLight), light_cmp);
    int nl = plights.n > 64 ? 64 : plights.n; frustum vf = frustum_from(m_vp); int kept = 0;
    for (int i = 0; i < nl; i++) { PLight *l = &plights.v[i]; if (!frustum_sphere(&vf, l->p, l->r)) continue;
        SET4(fu.lights[kept * 2], l->p.x, l->p.y, l->p.z, l->r); SET4(fu.lights[kept * 2 + 1], l->c.x, l->c.y, l->c.z, 0); kept++; }
    SET4(fu.light_n, (float)kept, 0, 0, 0);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_frame); glBufferData(GL_UNIFORM_BUFFER, sizeof fu, &fu, GL_DYNAMIC_DRAW); glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo_frame);

    /* ---- отсечение и списки ---- */
    lst_main.n = lst_trans.n = 0; for (int c = 0; c < 4; c++) lst_sh[c].n = 0;
    frustum sf[4]; for (int c = 0; c < nc; c++) sf[c] = frustum_from(shvp[c]);
    for (int i = 0; i < cmds.n; i++) {
        const Cmd *c = &cmds.v[i]; int fl = c->mat->flags;
        if (frustum_aabb(&vf, c->wb) && c->dist < cam.zfar) { if (fl & (LU_MAT_BLEND | LU_MAT_ADDITIVE | LU_MAT_WATER)) lu_vec_push(lst_trans, i); else lu_vec_push(lst_main, i); }
        if (shadows_on && !(fl & (LU_MAT_BLEND | LU_MAT_NOSHADOW | LU_MAT_UNLIT | LU_MAT_ADDITIVE | LU_MAT_WATER)))
            for (int k = 0; k < nc; k++) if (c->dist < splits[k] * 1.6f + 30 && frustum_aabb(&sf[k], c->wb)) lu_vec_push(lst_sh[k], i);
    }
    sort_base = cmds.v;
    if (lst_main.n > 1) qsort(lst_main.v, (size_t)lst_main.n, sizeof(int), cmp_opaque);
    if (lst_trans.n > 1) qsort(lst_trans.v, (size_t)lst_trans.n, sizeof(int), cmp_far_first);
    for (int k = 0; k < nc; k++) if (lst_sh[k].n > 1) qsort(lst_sh[k].v, (size_t)lst_sh[k].n, sizeof(int), cmp_opaque);
    inst.n = 0;
    make_runs(lst_main.v, lst_main.n, &runs_main, 1);
    make_runs(lst_trans.v, lst_trans.n, &runs_trans, 0);
    for (int k = 0; k < nc; k++) make_runs(lst_sh[k].v, lst_sh[k].n, &runs_sh[k], 1);
    glBindBuffer(GL_ARRAY_BUFFER, lu__inst_vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)((size_t)(inst.n > 0 ? inst.n : 1) * 4), NULL, GL_STREAM_DRAW);
    if (inst.n) glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)((size_t)inst.n * 4), inst.v);
    glBindBuffer(GL_TEXTURE_BUFFER, bone_buf);
    glBufferData(GL_TEXTURE_BUFFER, (GLsizeiptr)((size_t)(bones.n > 16 ? bones.n : 16) * 4), NULL, GL_STREAM_DRAW);
    if (bones.n) glBufferSubData(GL_TEXTURE_BUFFER, 0, (GLsizeiptr)((size_t)bones.n * 4), bones.v);
    glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_BUFFER, bone_tex);

    if (!um_flags) {
        GLuint p = prog[P_MAIN];
        um_flags = U(p, "uFlags"); um_hasalb = U(p, "uHasAlb"); um_hasnrm = U(p, "uHasNrm"); um_hasorm = U(p, "uHasORM"); um_color = U(p, "uColor");
        um_mat = U(p, "uMat"); um_emis = U(p, "uEmis"); um_skinned = U(p, "uSkinned"); um_shpass = U(p, "uShadowPass"); um_shm = U(p, "uShadowM");
        p = prog[P_DEPTH]; ud_flags = U(p, "uFlags"); ud_hasalb = U(p, "uHasAlb"); ud_color = U(p, "uColor"); ud_mat = U(p, "uMat");
        ud_skinned = U(p, "uSkinned"); ud_shpass = U(p, "uShadowPass"); ud_shm = U(p, "uShadowM");
    }
    glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glDepthMask(GL_TRUE); glDisable(GL_BLEND); glEnable(GL_CULL_FACE); glCullFace(GL_BACK); glFrontFace(GL_CCW);

    /* ---- тени ---- */
    if (shadows_on) {
        glUseProgram(prog[P_DEPTH]); glUniform1i(ud_shpass, 1);
        glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(1.6f, 2.f);
        for (int k = 0; k < nc; k++) {
            glBindFramebuffer(GL_FRAMEBUFFER, shadow_fbo[k]); glViewport(0, 0, lu_gfx_q.shadow_size, lu_gfx_q.shadow_size);
            glClear(GL_DEPTH_BUFFER_BIT); glUniformMatrix4fv(ud_shm, 1, GL_FALSE, shvp[k].m);
            bound_mat = NULL; bound_twosided = -1; int d0 = stats.draws;
            draw_runs(lst_sh[k].v, &runs_sh[k], 1, ud_skinned);
            stats.shadow_draws += stats.draws - d0; stats.draws = d0;
        }
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
    /* ---- предварительный проход глубины ---- */
    bind_target(&t_hdr); glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(prog[P_DEPTH]); glUniform1i(ud_shpass, 0); bound_mat = NULL; bound_twosided = -1;
    { int d0 = stats.draws, i0 = stats.instances, t0 = stats.tris; draw_runs(lst_main.v, &runs_main, 1, ud_skinned); stats.draws = d0; stats.instances = i0; stats.tris = t0; }
    /* ---- SSAO ---- */
    if (lu_gfx_q.ssao) {
        glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); bound_twosided = -1;
        bind_target(&t_ao); glUseProgram(prog[P_SSAO]); m4 ip = m4_inverse(m_proj); glUniformMatrix4fv(U(prog[P_SSAO], "uInvProj"), 1, GL_FALSE, ip.m);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, t_hdr.depth); fullscreen();
        glUseProgram(prog[P_BLUR]); bind_target(&t_ao2); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, t_ao.tex);
        glUniform2f(U(prog[P_BLUR], "uDir"), 1.f / t_ao.w, 0); fullscreen();
        bind_target(&t_ao); glBindTexture(GL_TEXTURE_2D, t_ao2.tex); glUniform2f(U(prog[P_BLUR], "uDir"), 0, 1.f / t_ao.h); fullscreen();
        glEnable(GL_DEPTH_TEST);
    }
    /* ---- освещение ---- */
    bind_target(&t_hdr); glDepthMask(GL_FALSE); glDepthFunc(GL_LEQUAL);
    glUseProgram(prog[P_MAIN]); glUniform1i(um_shpass, 0); bound_mat = NULL; bound_twosided = -1;
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, lu_gfx_q.ssao ? t_ao.tex : tex_white.id);
    glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D_ARRAY, shadow_tex);
    draw_runs(lst_main.v, &runs_main, 0, um_skinned);
    /* ---- небо (на дальней плоскости) ---- */
    glDisable(GL_CULL_FACE); bound_twosided = -1; glDepthFunc(GL_LEQUAL);
    glUseProgram(prog[P_SKY]);
    {   /* полноэкранный треугольник в z=1: рисуем с проверкой глубины, где ничего нет */
        glDepthFunc(GL_EQUAL); glDepthRange(1, 1); fullscreen(); glDepthRange(0, 1); glDepthFunc(GL_LEQUAL);
    }
    /* ---- прозрачные ---- */
    if (lst_trans.n) {
        glEnable(GL_BLEND); glUseProgram(prog[P_MAIN]); bound_mat = NULL; bound_twosided = -1;
        for (int r = 0; r < runs_trans.n; r++) {
            const Cmd *c = &cmds.v[lst_trans.v[runs_trans.v[r].start]];
            if (c->mat->flags & LU_MAT_ADDITIVE) glBlendFunc(GL_SRC_ALPHA, GL_ONE); else glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            LU_VEC(Run) one = {&runs_trans.v[r], 1, 1};
            draw_runs(lst_trans.v, &one, 0, um_skinned);
        }
        glDisable(GL_BLEND);
    }
    lines_draw();
    glDepthMask(GL_TRUE); glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE);

    /* ---- блум ---- */
    int bloom = lu_gfx_q.bloom && nbloom > 1;
    if (bloom) {
        bind_target(&t_bloom[0]); glUseProgram(prog[P_BRIGHT]); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, t_hdr.tex);
        glUniform2f(U(prog[P_BRIGHT], "uTexel"), .5f / t_hdr.w, .5f / t_hdr.h); fullscreen();
        glUseProgram(prog[P_DOWN]);
        for (int i = 1; i < nbloom; i++) { bind_target(&t_bloom[i]); glBindTexture(GL_TEXTURE_2D, t_bloom[i - 1].tex); glUniform2f(U(prog[P_DOWN], "uTexel"), 1.f / t_bloom[i - 1].w, 1.f / t_bloom[i - 1].h); fullscreen(); }
        glUseProgram(prog[P_UP]); glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);
        for (int i = nbloom - 1; i > 0; i--) { bind_target(&t_bloom[i - 1]); glBindTexture(GL_TEXTURE_2D, t_bloom[i].tex); glUniform2f(U(prog[P_UP], "uTexel"), 1.f / t_bloom[i].w, 1.f / t_bloom[i].h); fullscreen(); }
        glDisable(GL_BLEND);
    }
    /* ---- итог и FXAA ---- */
    bind_target(&t_ldr); glUseProgram(prog[P_COMP]);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, t_hdr.tex); glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, bloom ? t_bloom[0].tex : tex_white.id);
    glUniform1f(U(prog[P_COMP], "uBloomK"), bloom ? env.bloom : 0.f); fullscreen();
    bind_target(NULL); glUseProgram(prog[P_FXAA]); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, t_ldr.tex);
    glUniform2f(U(prog[P_FXAA], "uTexel"), 1.f / t_ldr.w, 1.f / t_ldr.h); glUniform1i(U(prog[P_FXAA], "uOn"), lu_gfx_q.fxaa); fullscreen();
    /* ---- интерфейс ---- */
    ui_flush_draw();
    glBindVertexArray(0);
    stats.cpu_ms = (lu_time() - frame_t0) * 1000.0;
}
lu_gfx_stats lu_gfx_get_stats(void) { return stats; }

static void lines_draw(void) {
    if (!lines.n) return;
    glUseProgram(prog[P_LINE]); glBindVertexArray(line_vao); glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(LVert) * (size_t)lines.n), lines.v, GL_STREAM_DRAW);
    glDepthMask(GL_FALSE); glDrawArrays(GL_LINES, 0, lines.n);
}

/* ================= интерфейс ================= */
static void ui_push(GLuint tex, const UIVert *v, int n) {
    if (!uib.n || uib.v[uib.n - 1].tex != tex) { UIBatch b = {tex, uiv.n, 0}; lu_vec_push(uib, b); }
    lu_vec_reserve(uiv, uiv.n + n + 8);
    memcpy(&uiv.v[uiv.n], v, sizeof(UIVert) * (size_t)n); uiv.n += n; uib.v[uib.n - 1].count += n;
}
static void ui_quad(GLuint tex, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, uint32_t c0, uint32_t c1, float mode, float param) {
    UIVert q[6] = {{x0, y0, u0, v0, c0, mode, param}, {x1, y0, u1, v0, c0, mode, param}, {x1, y1, u1, v1, c1, mode, param},
                   {x0, y0, u0, v0, c0, mode, param}, {x1, y1, u1, v1, c1, mode, param}, {x0, y1, u0, v1, c1, mode, param}};
    ui_push(tex, q, 6);
}
void lu_ui_rect(float x, float y, float w, float h, uint32_t c) { ui_quad(tex_white.id, x, y, x + w, y + h, 0, 0, 1, 1, c, c, 0, 0); }
void lu_ui_rect_grad(float x, float y, float w, float h, uint32_t t, uint32_t b) { ui_quad(tex_white.id, x, y, x + w, y + h, 0, 0, 1, 1, t, b, 0, 0); }
void lu_ui_image(lu_texture *t, float x, float y, float w, float h, v4 uv, uint32_t tint) { ui_quad(t->id, x, y, x + w, y + h, uv.x, uv.y, uv.z, uv.w, tint, tint, 1, 0); }
void lu_ui_circle(float cx, float cy, float r, uint32_t c) { ui_quad(tex_white.id, cx - r, cy - r, cx + r, cy + r, 0, 0, 1, 1, c, c, 3, 0); }
void lu_ui_line(float x0, float y0, float x1, float y1, float th, uint32_t c) {
    float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy); if (l < 1e-4f) return;
    float nx = -dy / l * th * .5f, ny = dx / l * th * .5f;
    UIVert q[6] = {{x0 + nx, y0 + ny, 0, 0, c, 0, 0}, {x1 + nx, y1 + ny, 1, 0, c, 0, 0}, {x1 - nx, y1 - ny, 1, 1, c, 0, 0},
                   {x0 + nx, y0 + ny, 0, 0, c, 0, 0}, {x1 - nx, y1 - ny, 1, 1, c, 0, 0}, {x0 - nx, y0 - ny, 0, 1, c, 0, 0}};
    ui_push(tex_white.id, q, 6);
}
float lu_ui_text_width(float size, const char *s) {
    float sc = size / lu_font_px, w = 0; int cp;
    while ((cp = lu_utf8_next(&s))) { const float *g = glyph(cp); if (g) w += g[7] * sc; }
    return w;
}
static float text_raw(float x, float y, float size, uint32_t c, float bold, const char *s, int n_bytes) {
    float sc = size / lu_font_px, pen = x; const char *end = n_bytes >= 0 ? s + n_bytes : NULL; int cp;
    while ((!end || s < end) && (cp = lu_utf8_next(&s))) {
        const float *g = glyph(cp); if (!g) continue;
        if (g[3] > 0) {
            float gx = pen + g[5] * sc, gy = y + g[6] * sc, gw = g[3] * sc, gh = g[4] * sc;
            ui_quad(tex_font.id, gx, gy, gx + gw, gy + gh, g[1] / lu_font_w, g[2] / lu_font_h, (g[1] + g[3]) / lu_font_w, (g[2] + g[4]) / lu_font_h, c, c, 2, bold);
        }
        pen += g[7] * sc;
    }
    return pen - x;
}
float lu_ui_text(float x, float y, float size, uint32_t c, int flags, const char *s) {
    float w = lu_ui_text_width(size, s);
    if (flags & LU_ALIGN_CENTER) x -= w * .5f; else if (flags & LU_ALIGN_RIGHT) x -= w;
    float bold = (flags & LU_TEXT_BOLD) ? .07f : 0;
    if (flags & LU_TEXT_SHADOW) { float o = fmaxf(1.f, size * .06f); uint32_t sh = (c & 0xFF000000u) / 255u * 170u & 0xFF000000u; text_raw(x + o, y + o, size, sh | 0x00101010u, bold + .08f, s, -1); }
    text_raw(x, y, size, c, bold, s, -1);
    return w;
}
float lu_ui_text_box(float x, float y, float w, float size, uint32_t c, int flags, const char *s) {
    float lh = size * 1.25f, yy = y; const char *line = s;
    while (*line) {
        const char *p = line, *brk = NULL; float acc = 0;
        while (*p && *p != '\n') {
            const char *q = p; int cp = lu_utf8_next(&q); const float *g = glyph(cp); float adv = g ? g[7] * size / lu_font_px : 0;
            if (cp == ' ') brk = p;
            if (acc + adv > w && brk) break;
            acc += adv; p = q;
        }
        const char *stop = (*p && *p != '\n' && brk) ? brk : p;
        char buf[1024]; int n = (int)(stop - line); if (n > 1023) n = 1023; memcpy(buf, line, (size_t)n); buf[n] = 0;
        float lx = x; if (flags & LU_ALIGN_CENTER) lx = x + w * .5f; else if (flags & LU_ALIGN_RIGHT) lx = x + w;
        lu_ui_text(lx, yy, size, c, flags, buf);
        yy += lh; line = stop; while (*line == ' ') line++; if (*line == '\n') line++;
    }
    return yy - y;
}
static void ui_flush_draw(void) {
    if (!uiv.n) return;
    glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE);
    glUseProgram(prog[P_UI]); glUniform2f(U(prog[P_UI], "uInvScreen"), 1.f / out_w, 1.f / out_h);
    glBindVertexArray(ui_vao); glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(UIVert) * (size_t)uiv.n), uiv.v, GL_STREAM_DRAW);
    glActiveTexture(GL_TEXTURE0);
    for (int i = 0; i < uib.n; i++) { glBindTexture(GL_TEXTURE_2D, uib.v[i].tex); glDrawArrays(GL_TRIANGLES, uib.v[i].first, uib.v[i].count); }
    glDisable(GL_BLEND);
}
