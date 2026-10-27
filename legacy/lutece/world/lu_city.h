/* Lutèce Engine — генератор городской застройки XVIII века: дома с фасадами (окна в нишах, ставни, подоконники,
   карнизы по этажам, балконы, лавки с навесами, руст на углах), мансардные и черепичные крыши, трубы,
   мостовые, тротуары и уличные предметы. Одновременно строит геометрию столкновений для паркура.
   Геометрия группируется по ячейкам мира и материалам для отсечения и экономии вызовов. */
#ifndef LU_CITY_H
#define LU_CITY_H
#include "../gfx/lu_gfx.h"
#include "../phys/lu_phys.h"

enum {
    LU_CM_STONE, LU_CM_PLASTER, LU_CM_BRICK, LU_CM_SLATE, LU_CM_TILES, LU_CM_WOOD, LU_CM_SHUTTER, LU_CM_IRON,
    LU_CM_GLASS, LU_CM_COBBLE, LU_CM_FLAG, LU_CM_CLOTH, LU_CM_TRIM, LU_CM_DIRT, LU_CM_LEAVES, LU_CM_BARK, LU_CM_LAMP, LU_CM_WATER, LU_CM_N
};
enum { LU_ROOF_MANSARD, LU_ROOF_GABLE, LU_ROOF_FLAT };
typedef struct {
    float x0, z0, x1, z1;      /* след на земле */
    float base_y;              /* уровень земли */
    int floors; float floor_h, ground_h;
    int wall_mat;              /* LU_CM_STONE / PLASTER / BRICK */
    uint32_t tint, shutter_col, trim_col;
    int roof; int shops;       /* битовая маска сторон с лавками: 1 -Z, 2 +X, 4 +Z, 8 -X */
    int balcony_floor;         /* этаж с балконом (0 — нет) */
    int street_sides;          /* какие стороны выходят на улицу (детальные фасады) */
    uint32_t seed;
} lu_building;
#define LU_CITY_MAXCH 1024
typedef struct {
    lu_phys_world *phys;
    float chunk; int cw, ch; v3 origin;
    lu_mesh_builder *b;        /* [ячейка * LU_CM_N + материал] */
    lu_mesh *mesh; int built;
    int nbuildings; long tris;
    /* точки для игры */
    LU_VEC(v3) lamps; LU_VEC(v4) hay; LU_VEC(v3) spawn_pts;
} lu_city;
void lu_city_init(lu_city *c, lu_phys_world *phys, aabb area, float chunk);
void lu_city_free(lu_city *c);
lu_mesh_builder *lu_city_mb(lu_city *c, v3 at, int mat);
void lu_city_building(lu_city *c, const lu_building *b);
void lu_city_ground(lu_city *c, int mat, float x0, float z0, float x1, float z1, float y, float uv, int collide);
void lu_city_box(lu_city *c, int mat, aabb b, float uv, uint32_t col, int collide_flags); /* collide_flags < 0 — без столкновений */
void lu_city_lamp(lu_city *c, v3 base);
void lu_city_tree(lu_city *c, v3 base, float h, uint32_t seed);
void lu_city_haycart(lu_city *c, v3 pos, float yaw);
void lu_city_stall(lu_city *c, v3 pos, float yaw, uint32_t awning);
void lu_city_fountain(lu_city *c, v3 pos, float r);
void lu_city_crates(lu_city *c, v3 pos, int n, uint32_t seed);
void lu_city_finish(lu_city *c);          /* загрузить меши и построить сетку столкновений */
void lu_city_draw(const lu_city *c, const lu_material *mats);
/* материалы по умолчанию (текстуры генерируются процедурно) */
void lu_city_materials(lu_material mats[LU_CM_N], lu_texture tex[LU_CM_N][3], int tex_size);
#endif
