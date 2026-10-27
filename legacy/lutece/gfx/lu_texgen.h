/* Lutèce Engine — процедурные текстуры: бесшовный шум, ячейки Вороного, кладка, черепица, доски,
   карты нормалей и затенения из высот. Результат — наборы PBR-текстур (альбедо, нормали, ORM). */
#ifndef LU_TEXGEN_H
#define LU_TEXGEN_H
#include "lu_gfx.h"

typedef struct { int n; uint32_t *alb, *nrm, *orm; float *hgt; } lu_texset;  /* квадрат n×n, бесшовный */
void lu_texset_alloc(lu_texset *t, int n);
void lu_texset_free(lu_texset *t);
/* загрузить в видеопамять (альбедо в sRGB, мип-уровни) */
void lu_texset_upload(const lu_texset *t, lu_texture *alb, lu_texture *nrm, lu_texture *orm);
/* нормали и ORM из карты высот: strength — крутизна, ao — сила затенения во впадинах */
void lu_texset_finish(lu_texset *t, float strength, float ao, const float *rough, const float *metal);

/* бесшовный шум с периодом p (в клетках) */
float lu_tnoise(float x, float y, int p, uint32_t seed);
float lu_tfbm(float x, float y, int p, int oct, uint32_t seed);
/* ячейки Вороного: расстояние до ближайшего центра (f1), до второго (f2), номер ячейки */
void lu_tvoronoi(float x, float y, int p, uint32_t seed, float *f1, float *f2, uint32_t *id);

typedef struct { v3 c0, c1; float var; } lu_palette;   /* линейные цвета и разброс */
enum {
    LU_TG_STONE_BLOCKS, LU_TG_PLASTER, LU_TG_COBBLE, LU_TG_SLATE, LU_TG_ROOF_TILES, LU_TG_WOOD, LU_TG_BRICK,
    LU_TG_FLAGSTONE, LU_TG_DIRT, LU_TG_CLOTH, LU_TG_METAL, LU_TG_SHUTTER, LU_TG_N
};
/* сгенерировать набор по типу; size — степень двойки (256..1024) */
void lu_texgen(lu_texset *t, int type, int size, lu_palette pal, uint32_t seed);
#endif
