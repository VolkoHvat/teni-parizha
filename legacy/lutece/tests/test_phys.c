/* Модульные тесты физики и сенсоров паркура Lutèce */
#include "../phys/lu_phys.h"
#include <stdio.h>
static int fails;
#define CHECK(c) do { if (!(c)) { printf("  ПРОВАЛ: %s (строка %d)\n", #c, __LINE__); fails++; } } while (0)
int main(void) {
    lu_phys_world w; lu_phys_init(&w);
    lu_phys_add_box(&w, (aabb){V3(-50, -1, -50), V3(50, 0, 50)}, LU_SURF_STONE);          /* пол */
    lu_phys_add_box(&w, (aabb){V3(-5, 0, 5), V3(5, 2, 6)}, 0);                            /* стена 2 м, грань z=5 */
    lu_phys_add_box(&w, (aabb){V3(10, 0, 5), V3(20, 8, 15)}, LU_SURF_ROOF);               /* дом 8 м */
    for (int i = 0; i < 6; i++) lu_phys_add_box(&w, (aabb){V3(-20, 0, -10 + i * .3f), V3(-15, .18f * (i + 1), 0)}, 0); /* лестница */
    lu_phys_add_box(&w, (aabb){V3(0, 0, -6), V3(3, .9f, -5.6f)}, LU_SURF_WOOD);          /* ящик для перемахивания */
    lu_phys_build(&w, 4.f);
    lu_hit h;
    CHECK(lu_raycast(&w, V3(1, 5, 1), V3(0, -1, 0), 10, &h) && fabsf(h.pos.y) < 1e-4f && h.normal.y > .99f && (h.flags & LU_SURF_STONE));
    CHECK(lu_raycast(&w, V3(0, 1, 0), V3(0, 0, 1), 10, &h) && fabsf(h.t - 5) < 1e-4f && h.normal.z < -.99f);
    CHECK(!lu_raycast(&w, V3(0, 3, 0), V3(0, 0, 1), 10, &h));
    CHECK(lu_capsule_overlap(&w, V3(0, .5f, 4.8f), V3(0, 1.5f, 4.8f), .3f));
    CHECK(!lu_capsule_overlap(&w, V3(0, .5f, 4.5f), V3(0, 1.5f, 4.5f), .3f));
    printf("контроллер…\n");
    lu_cc c; lu_cc_init(&c, V3(0, 0, 0)); c.grounded = 1;
    for (int i = 0; i < 120; i++) { c.vel = V3(0, c.vel.y - 9.8f / 60, 4); if (c.grounded) c.vel.y = 0; lu_cc_move(&c, &w, 1.f / 60, 1); }
    printf("  упёрся в стену на z=%.3f (ожидается ~4.7)\n", c.pos.z); CHECK(fabsf(c.pos.z - 4.7f) < .05f && c.grounded && c.hit_wall);
    for (int i = 0; i < 60; i++) { c.vel = V3(3, 0, 3); lu_cc_move(&c, &w, 1.f / 60, 1); }
    CHECK(c.pos.x > 2.5f && c.pos.z < 4.75f);                                           /* скольжение вдоль стены */
    lu_cc_init(&c, V3(-17.5f, 0, -12)); c.grounded = 1;
    for (int i = 0; i < 180; i++) { c.vel = V3(0, c.grounded ? 0 : c.vel.y - 9.8f / 60, 2); lu_cc_move(&c, &w, 1.f / 60, 1); }
    printf("  лестница: высота %.2f (ожидается 1.08)\n", c.pos.y); CHECK(fabsf(c.pos.y - 1.08f) < .05f);
    lu_cc_init(&c, V3(15, 8, 10)); c.grounded = 1;
    int landed = 0; for (int i = 0; i < 240 && !landed; i++) { c.vel = V3(4, c.grounded ? 0 : c.vel.y - 22.f / 60, 0); lu_cc_move(&c, &w, 1.f / 60, 1); if (c.pos.x > 21 && c.grounded) landed = 1; }
    printf("  спрыгнул с крыши: приземлился=%d y=%.2f\n", landed, c.pos.y); CHECK(landed && fabsf(c.pos.y) < .02f);
    printf("сенсоры паркура…\n");
    lu_ledge L;
    CHECK(lu_find_ledge(&w, V3(0, 0, 4.4f), V3(0, 0, 1), 1.f, 2.4f, 1.f, &L));
    printf("  уступ: верх %.2f, стена n=(%.2f,%.2f,%.2f), встать можно=%d\n", L.top_y, L.wall_n.x, L.wall_n.y, L.wall_n.z, L.can_stand);
    CHECK(fabsf(L.top_y - 2) < .01f && L.wall_n.z < -.99f && L.can_stand && fabsf(L.edge.z - 5) < .01f);
    CHECK(!lu_find_ledge(&w, V3(15, 0, 4.4f), V3(0, 0, 1), 1.f, 2.4f, 1.f, &L));          /* дом слишком высок */
    CHECK(lu_find_ledge(&w, V3(15, 6, 4.4f), V3(0, 0, 1), 1.f, 2.4f, 1.f, &L) && fabsf(L.top_y - 8) < .01f); /* в лазании у края крыши */
    float oh, dep; CHECK(lu_find_vault(&w, V3(1.5f, 0, -7), V3(0, 0, 1), 1.2f, &oh, &dep));
    printf("  препятствие: высота %.2f, глубина %.2f\n", oh, dep); CHECK(fabsf(oh - .9f) < .02f && dep < .6f);
    CHECK(lu_find_grab(&w, V3(1, 2.0f, 4.6f), V3(0, 0, 1), .8f, &L) && fabsf(L.top_y - 2) < .01f);
    CHECK(!lu_find_grab(&w, V3(1, 3.5f, 4.6f), V3(0, 0, 1), .8f, &L));
    lu_ledge L2; lu_find_ledge(&w, V3(0, 0, 4.4f), V3(0, 0, 1), 1.f, 2.4f, 1.f, &L);
    CHECK(lu_ledge_continues(&w, &L, 1, 1.f, &L2));
    lu_find_ledge(&w, V3(4.8f, 0, 4.4f), V3(0, 0, 1), 1.f, 2.4f, 1.f, &L);
    CHECK(!lu_ledge_continues(&w, &L, L.edge_dir.x > 0 ? 1.f : -1.f, 1.f, &L2));         /* конец стены */
    double t0 = lu_time(); int nh = 0; for (int i = 0; i < 10000; i++) nh += lu_raycast(&w, V3(i % 40 - 20.f, 3, -20), V3(.1f, -.3f, 1), 40, &h);
    printf("  10000 лучей: %.1f мс (попаданий %d)\n", (lu_time() - t0) * 1000, nh);
    lu_phys_free(&w);
    printf(fails ? "ЕСТЬ ОШИБКИ: %d\n" : "все тесты физики пройдены\n", fails);
    return fails != 0;
}
