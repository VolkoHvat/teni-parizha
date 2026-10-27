/* Lutèce Engine — рендер (OpenGL 3.3 core).
   Схема кадра: каскадные тени → предварительный проход глубины → SSAO → освещение PBR (солнце + точечные источники) →
   небо → прозрачные → блум → тонмаппинг ACES → FXAA → интерфейс.
   Отрисовка декларативная: в течение кадра собираются вызовы, одинаковые меши+материалы автоматически объединяются в инстансы. */
#ifndef LU_GFX_H
#define LU_GFX_H
#include "../core/lu_core.h"

/* ---------- текстуры ---------- */
enum { LU_TEX_SRGB = 1, LU_TEX_MIPS = 2, LU_TEX_CLAMP = 4, LU_TEX_NEAREST = 8, LU_TEX_R8 = 16 };
typedef struct { unsigned id; int w, h, flags; } lu_texture;
lu_texture lu_texture_create(int w, int h, const void *pixels, int flags); /* RGBA8 (или R8 при LU_TEX_R8) */
void lu_texture_update(lu_texture *t, const void *pixels);
void lu_texture_destroy(lu_texture *t);

/* ---------- меши ---------- */
typedef struct {
    v3 pos, nrm;
    v4 tan;          /* касательная + знак бинормали */
    v2 uv;
    uint32_t color;  /* RGBA8 (A = доп. параметр: для листвы — гибкость) */
    uint8_t joints[4];
    uint8_t weights[4]; /* нормированные 0..255 */
} lu_vertex;
typedef struct { int first, count, material; } lu_submesh;
#define LU_MAX_LOD 4
typedef struct {
    unsigned vao, vbo, ibo;
    int nverts, nidx, skinned;
    lu_submesh sub[16]; int nsub;
    aabb bounds;
} lu_mesh;
/* CPU-построитель мешей */
typedef struct {
    LU_VEC(lu_vertex) v;
    LU_VEC(uint32_t) idx;
    lu_submesh sub[16]; int nsub;
    uint32_t color; int skin_joint; /* текущие цвет и кость (для простых скелетных мешей) */
} lu_mesh_builder;
void lu_mb_init(lu_mesh_builder *b);
void lu_mb_free(lu_mesh_builder *b);
void lu_mb_material(lu_mesh_builder *b, int material);   /* начать новую часть */
int  lu_mb_vertex(lu_mesh_builder *b, v3 p, v3 n, v2 uv);
void lu_mb_tri(lu_mesh_builder *b, int a, int c, int d);
void lu_mb_quad(lu_mesh_builder *b, v3 a, v3 c, v3 d, v3 e, v2 uv0, v2 uv1); /* a,c,d,e против часовой при взгляде снаружи */
void lu_mb_box(lu_mesh_builder *b, v3 mn, v3 mx, float uv_scale);           /* UV в метрах * uv_scale */
void lu_mb_cylinder(lu_mesh_builder *b, v3 base, float r0, float r1, float h, int seg, int caps);
void lu_mb_sphere(lu_mesh_builder *b, v3 c, v3 r, int seg, int ring);
void lu_mb_tube(lu_mesh_builder *b, v3 a, v3 c, float r0, float r1, int seg);
void lu_mb_lathe(lu_mesh_builder *b, v3 base, const v2 *profile, int n, int seg); /* профиль (радиус, высота) */
void lu_mb_transform(lu_mesh_builder *b, int from_vertex, m4 m);
void lu_mb_compute_normals(lu_mesh_builder *b, int from_vertex);
void lu_mb_compute_tangents(lu_mesh_builder *b);
lu_mesh lu_mesh_create(lu_mesh_builder *b, int skinned);
/* уровни детализации: набор мешей, выбор по расстоянию до камеры (dist[k] — дальняя граница уровня k) */
typedef struct { const lu_mesh *lv[LU_MAX_LOD]; float dist[LU_MAX_LOD]; int n; } lu_lod;
void lu_mesh_destroy(lu_mesh *m);

/* ---------- материалы ---------- */
enum {
    LU_MAT_CUTOUT = 1,     /* альфа-отсечение (листва, решётки) */
    LU_MAT_BLEND = 2,      /* прозрачный */
    LU_MAT_TWOSIDED = 4,
    LU_MAT_WIND = 8,       /* колышется (листва, ткань) */
    LU_MAT_SKIN = 16,      /* мягкое рассеяние для кожи */
    LU_MAT_UNLIT = 32,
    LU_MAT_NOSHADOW = 64,
    LU_MAT_WATER = 128,
    LU_MAT_ADDITIVE = 256,
    LU_MAT_TRIPLANAR = 512,/* мировые UV (для зданий без развёртки) */
    LU_MAT_VCOLOR = 1024   /* умножать на цвет вершины */
};
typedef struct {
    lu_texture *albedo, *normal, *orm;   /* orm: R — затенение, G — шероховатость, B — металл */
    v4 color;                            /* множитель альбедо (линейный) */
    float roughness, metallic, normal_scale, uv_scale;
    v3 emissive;                         /* линейный HDR */
    int flags;
} lu_material;
lu_material lu_material_default(void);

/* ---------- скелетные меши ---------- */
#define LU_MAX_BONES 128

/* ---------- окружение и камера ---------- */
typedef struct {
    v3 sun_dir;          /* направление К солнцу */
    v3 sun_color;        /* линейная яркость */
    v3 sky_zenith, sky_horizon, ground;
    v3 fog_color; float fog_density, fog_height_falloff;
    float exposure, bloom, time, wind, cloud_cover, wetness, night;
    float ssao_strength;
} lu_env;
lu_env lu_env_default(void);
typedef struct { v3 pos, target, up; float fovy, znear, zfar; } lu_camera;

/* ---------- настройки качества ---------- */
typedef struct { float render_scale; int shadow_size, cascades, ssao, bloom, fxaa; float shadow_dist; } lu_gfx_quality;
extern lu_gfx_quality lu_gfx_q;
void lu_gfx_set_quality(int level); /* 0..3 */

/* ---------- кадр ---------- */
int  lu_gfx_init(void);
const char *lu_gfx_error(void);
void lu_gfx_shutdown(void);
void lu_gfx_begin(int width, int height, unsigned target_fbo);
void lu_gfx_camera(const lu_camera *c);
void lu_gfx_env(const lu_env *e);
void lu_draw(const lu_mesh *m, const lu_material *mats, int nmats, m4 world);
void lu_draw_tinted(const lu_mesh *m, const lu_material *mats, int nmats, m4 world, v4 tint);
void lu_draw_lod(const lu_lod *l, const lu_material *mats, int nmats, m4 world);
void lu_draw_skinned(const lu_mesh *m, const lu_material *mats, int nmats, m4 world, const m4 *palette, int nbones);
void lu_light(v3 pos, float radius, v3 color);
void lu_debug_line(v3 a, v3 b, uint32_t rgba);
void lu_debug_box(aabb b, uint32_t rgba);
void lu_debug_sphere(v3 c, float r, uint32_t rgba);
void lu_gfx_end(void);              /* выполнить кадр (3D + интерфейс) */
int  lu_gfx_project(v3 p, float *sx, float *sy); /* мир → пиксели экрана */
m4   lu_gfx_viewproj(void);
typedef struct { int draws, instances, tris, shadow_draws, skinned; double cpu_ms; } lu_gfx_stats;
lu_gfx_stats lu_gfx_get_stats(void);

/* ---------- 2D-интерфейс (координаты в пикселях, начало слева сверху) ---------- */
void lu_ui_rect(float x, float y, float w, float h, uint32_t rgba);
void lu_ui_rect_grad(float x, float y, float w, float h, uint32_t top, uint32_t bottom);
void lu_ui_image(lu_texture *t, float x, float y, float w, float h, v4 uv, uint32_t tint);
void lu_ui_circle(float cx, float cy, float r, uint32_t rgba);
void lu_ui_line(float x0, float y0, float x1, float y1, float th, uint32_t rgba);
/* текст: встроенный SDF-шрифт (латиница + кириллица), size — высота в пикселях */
enum { LU_ALIGN_LEFT = 0, LU_ALIGN_CENTER = 1, LU_ALIGN_RIGHT = 2, LU_TEXT_SHADOW = 4, LU_TEXT_BOLD = 8 };
float lu_ui_text(float x, float y, float size, uint32_t rgba, int flags, const char *utf8);
float lu_ui_text_width(float size, const char *utf8);
/* перенос по словам в ширину w; возвращает высоту блока */
float lu_ui_text_box(float x, float y, float w, float size, uint32_t rgba, int flags, const char *utf8);
#define LU_RGBA(r, g, b, a) ((uint32_t)(((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(r)))
#endif
