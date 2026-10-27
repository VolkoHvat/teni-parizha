/* Проверка свободного бега Lutèce автопилотом: ящик → стена → карниз → крыша → прыжок на соседнюю крышу */
#include "../game/lu_traversal.h"
#include <stdio.h>
static int fails;
#define CHECK(c) do { if (!(c)) { printf("  ПРОВАЛ: %s (строка %d)\n", #c, __LINE__); fails++; } } while (0)
int main(void) {
    lu_phys_world w; lu_phys_init(&w);
    lu_phys_add_box(&w, (aabb){V3(-30, -1, -30), V3(30, 0, 60)}, LU_SURF_STONE);
    lu_phys_add_box(&w, (aabb){V3(3.5f, 0, 5), V3(6.5f, .9f, 5.6f)}, LU_SURF_WOOD);      /* ящик */
    lu_phys_add_box(&w, (aabb){V3(0, 0, 10), V3(10, 6.6f, 20)}, LU_SURF_STONE);          /* дом А */
    lu_phys_add_box(&w, (aabb){V3(0, 3.2f, 9.72f), V3(10, 3.42f, 10)}, LU_SURF_STONE);   /* карниз второго этажа */
    lu_phys_add_box(&w, (aabb){V3(0, 0, 23), V3(10, 6.2f, 33)}, LU_SURF_ROOF);           /* дом Б через улицу 3 м */
    lu_phys_add_box(&w, (aabb){V3(-8, 0, 40), V3(-4, 1.2f, 44)}, LU_SURF_SOFT);          /* сено */
    lu_phys_build(&w, 4);
    lu_skeleton sk; lu_humanoid hu; lu_humanoid_skeleton(&sk, &hu, 1.78f, 0);
    lu_motions mot; lu_motions_build(&mot, &sk, &hu);
    static lu_traversal T; lu_tr_init(&T, &sk, &hu, &mot, V3(5, 0, 0), 0);
    int seen[LU_TR_N] = {0}, last = -1; float dt = 1.f / 60;
    for (int f = 0; f < 60 * 14; f++) {
        lu_tr_input in; memset(&in, 0, sizeof in);
        in.move = V3(0, 0, 1); in.sprint = 1;
        if (T.state == LU_TR_HANG || T.state == LU_TR_CLIMB) { in.move = V3(0, 0, 1); }
        lu_tr_update(&T, &w, &in, dt);
        seen[T.state] = 1;
        if (T.state != last) { printf("  %5.2f с: %-14s поз (%.2f, %.2f, %.2f)\n", f * dt, lu_tr_names[T.state], T.cc.pos.x, T.cc.pos.y, T.cc.pos.z); last = T.state; }
        if (T.cc.pos.z > 27 && T.state == LU_TR_GROUND && T.cc.pos.y > 6) break;
    }
    printf("итог: (%.2f, %.2f, %.2f) состояние %s\n", T.cc.pos.x, T.cc.pos.y, T.cc.pos.z, lu_tr_names[T.state]);
    CHECK(seen[LU_TR_VAULT]); CHECK(seen[LU_TR_HANG]); CHECK(seen[LU_TR_CLIMB_UP]);
    CHECK(T.cc.pos.y > 6.1f && T.cc.pos.z > 23);   /* на крыше дома Б */
    /* падение в сено и с высоты без кувырка */
    lu_tr_teleport(&T, V3(-6, 12, 42), 0); T.state = LU_TR_AIR; T.cc.grounded = 0; T.air_top = 12;
    float dmg = 0; for (int f = 0; f < 120; f++) { lu_tr_input in; memset(&in, 0, sizeof in); lu_tr_update(&T, &w, &in, dt); dmg += T.damage; }
    printf("прыжок веры в сено с 10.8 м: урон %.0f\n", dmg); CHECK(dmg == 0);
    lu_tr_teleport(&T, V3(20, 14, 0), 0); T.state = LU_TR_AIR; T.cc.grounded = 0; T.air_top = 14;
    dmg = 0; for (int f = 0; f < 120; f++) { lu_tr_input in; memset(&in, 0, sizeof in); lu_tr_update(&T, &w, &in, dt); dmg += T.damage; }
    printf("падение с 14 м стоя: урон %.0f\n", dmg); CHECK(dmg > 30);
    lu_phys_free(&w); lu_motions_free(&mot);
    printf(fails ? "ЕСТЬ ОШИБКИ: %d\n" : "свободный бег работает\n", fails);
    return fails != 0;
}
