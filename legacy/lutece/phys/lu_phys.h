/* Lutèce Engine — физика мира и паркур.
   Статическая геометрия столкновений (треугольники в сетке ячеек), лучи, проверки капсулы,
   контроллер персонажа (ступени, склоны, скольжение вдоль стен) и «сенсоры паркура»:
   поиск стены, уступа, карниза для виса, препятствия для перепрыгивания — на любой геометрии, без ручной разметки. */
#ifndef LU_PHYS_H
#define LU_PHYS_H
#include "../core/lu_core.h"

/* флаги поверхностей */
enum {
    LU_SURF_NOCLIMB = 1,   /* нельзя лазать (стекло, гладкий камень) */
    LU_SURF_SOFT = 2,      /* мягкое (сено): гасит падение */
    LU_SURF_WATER = 4,
    LU_SURF_NOGRAB = 8,    /* край нельзя схватить */
    LU_SURF_WOOD = 16, LU_SURF_STONE = 32, LU_SURF_ROOF = 64, LU_SURF_METAL = 128 /* для звуков шагов */
};
typedef struct { v3 a, b, c, n; int flags; } lu_tri;
typedef struct {
    LU_VEC(lu_tri) tris;
    float cell; v3 origin; int gw, gh;     /* сетка XZ */
    int *cstart, *citems;                  /* ячейка → треугольники */
    int *stamp; int cur_stamp;             /* чтобы не проверять треугольник дважды за запрос */
    aabb bounds;
} lu_phys_world;
void lu_phys_init(lu_phys_world *w);
void lu_phys_free(lu_phys_world *w);
void lu_phys_add_tri(lu_phys_world *w, v3 a, v3 b, v3 c, int flags);
void lu_phys_add_box(lu_phys_world *w, aabb b, int flags);
void lu_phys_add_obb(lu_phys_world *w, m4 xf, v3 half, int flags);
void lu_phys_build(lu_phys_world *w, float cell);   /* построить ускоряющую сетку (после добавления всего) */

typedef struct { int hit; float t; v3 pos, normal; int tri, flags; } lu_hit;
int lu_raycast(const lu_phys_world *w, v3 o, v3 dir, float maxd, lu_hit *out);
/* сфера, движущаяся по отрезку: первое касание */
int lu_spherecast(const lu_phys_world *w, v3 o, v3 dir, float r, float maxd, lu_hit *out);
/* пересекается ли капсула (отрезок p0–p1, радиус r) с геометрией */
int lu_capsule_overlap(const lu_phys_world *w, v3 p0, v3 p1, float r);
/* вытолкнуть капсулу из геометрии: возвращает суммарный сдвиг и нормаль самого сильного контакта */
v3 lu_capsule_resolve(const lu_phys_world *w, v3 p0, v3 p1, float r, v3 *contact_n, int iterations);

/* ---------- контроллер персонажа ---------- */
typedef struct {
    v3 pos;                  /* точка между ступнями (низ капсулы) */
    v3 vel;
    float radius, height, step, max_slope_cos, skin;
    int grounded, was_grounded; v3 ground_n; int ground_flags; float ground_y;
    int hit_wall; v3 wall_n; float air_time, fall_start_y;
} lu_cc;
void lu_cc_init(lu_cc *c, v3 pos);
/* сдвинуть на vel*dt со скольжением, ступенями и прилипанием к земле (snap — разрешить прилипание вниз) */
void lu_cc_move(lu_cc *c, const lu_phys_world *w, float dt, int snap);

/* ---------- сенсоры паркура ---------- */
typedef struct {
    int ok;
    v3 wall_pt, wall_n;      /* точка стены на уровне груди и её нормаль (наружу) */
    v3 edge;                 /* точка на кромке уступа (верх стены) */
    v3 edge_dir;             /* направление кромки (вдоль стены) */
    float top_y, height;     /* высота кромки (абсолютная и над ногами) */
    int can_stand;           /* сверху есть место встать */
    int flags;
} lu_ledge;
/* стена перед персонажем: from — ноги, dir — горизонтальное направление; возвращает расстояние или -1 */
float lu_probe_wall(const lu_phys_world *w, v3 from, v3 dir, float height, float maxd, v3 *n_out, int *flags);
/* найти уступ впереди: кромка в диапазоне высот [hmin, hmax] над ногами, стена не дальше reach */
int lu_find_ledge(const lu_phys_world *w, v3 feet, v3 dir, float hmin, float hmax, float reach, lu_ledge *out);
/* найти край рядом с точкой рук (для хватки в полёте): ищет кромку в радиусе r вокруг hands */
int lu_find_grab(const lu_phys_world *w, v3 hands, v3 facing, float r, lu_ledge *out);
/* препятствие для перемахивания: высота 0.4–1.3 м, за ним свободно */
int lu_find_vault(const lu_phys_world *w, v3 feet, v3 dir, float reach, float *obstacle_h, float *depth);
/* продолжается ли кромка вбок на расстояние d (для перебежки по карнизу) */
int lu_ledge_continues(const lu_phys_world *w, const lu_ledge *l, float side, float d, lu_ledge *out);
/* высота пола под точкой (луч вниз из y+up); -1e9 если нет */
float lu_floor_at(const lu_phys_world *w, v3 p, float up, float down, v3 *n, int *flags);
#endif
