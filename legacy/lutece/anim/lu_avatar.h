/* Lutèce Engine — генератор персонажей: сглаженный скелетный меш тела и одежды по описанию внешности.
   Цвета хранятся в вершинах, поэтому весь персонаж — несколько частей с общими материалами (кожа, ткань, кожа-изделия, волосы, глаза, металл). */
#ifndef LU_AVATAR_H
#define LU_AVATAR_H
#include "lu_humanoid.h"

enum { LU_AV_SKIN, LU_AV_CLOTH, LU_AV_LEATHER, LU_AV_HAIR, LU_AV_EYE, LU_AV_METAL, LU_AV_NMAT };
enum { LU_COAT_NONE, LU_COAT_JACKET, LU_COAT_LONG, LU_COAT_HOODED };
enum { LU_HAT_NONE, LU_HAT_TRICORN, LU_HAT_BONNET, LU_HAT_CAP };
enum { LU_HAIR_SHORT, LU_HAIR_QUEUE, LU_HAIR_BUN, LU_HAIR_LONG, LU_HAIR_BALD };
typedef struct {
    int female; float height, build;      /* build: 0 — худой, 1 — плотный */
    uint32_t skin, hair, eyes, shirt, vest, coat_color, coat_trim, pants, stockings, shoes, belt, hat;
    int coat, hat_type, hair_style, skirt, apron, sash, sword, beard;
    uint32_t seed;
} lu_outfit;
void lu_outfit_default(lu_outfit *o, int female);
void lu_outfit_random(lu_outfit *o, uint32_t seed);   /* горожанин 1789 года */
/* построить скелет и меш (материалы — индексы LU_AV_*) */
void lu_avatar_build(const lu_outfit *o, lu_skeleton *s, lu_humanoid *h, lu_mesh *out, int detail);
/* стандартные материалы для частей (общие для всех персонажей) */
void lu_avatar_materials(lu_material mats[LU_AV_NMAT]);
#endif
