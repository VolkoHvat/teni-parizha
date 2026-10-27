/* Модульные тесты анимации Lutèce: скелет, клипы, смешивание, граф состояний, IK */
#include "../anim/lu_anim.h"
#include <stdio.h>
static int fails;
#define CHECK(c) do { if (!(c)) { printf("  ПРОВАЛ: %s (строка %d)\n", #c, __LINE__); fails++; } } while (0)
static int v3near(v3 a, v3 b, float e) { return fabsf(a.x - b.x) <= e && fabsf(a.y - b.y) <= e && fabsf(a.z - b.z) <= e; }
static xform XT(float x, float y, float z) { xform t = XF_ID(); t.t = V3(x, y, z); return t; }
int main(void) {
    lu_skeleton sk; memset(&sk, 0, sizeof sk);
    int root = lu_skel_add(&sk, "root", -1, XT(0, 0, 0));
    int hip = lu_skel_add(&sk, "hip", root, XT(0, 1, 0));
    int knee = lu_skel_add(&sk, "knee", hip, XT(0, -.5f, 0));
    int foot = lu_skel_add(&sk, "foot", knee, XT(0, -.5f, 0));
    lu_skel_finalize(&sk);
    CHECK(lu_skel_find(&sk, "knee") == knee);
    lu_pose p; lu_pose_bind(&sk, &p); xform M[LU_MAX_BONES]; lu_pose_model(&sk, &p, M);
    CHECK(v3near(M[foot].t, V3(0, 0, 0), 1e-5f));
    m4 pal[LU_MAX_BONES]; lu_pose_palette(&sk, M, pal);
    CHECK(v3near(m4_point(pal[foot], V3(.3f, .2f, .1f)), V3(.3f, .2f, .1f), 1e-5f)); /* в позе привязки палитра — единица */
    printf("клипы…\n");
    lu_clip c; lu_clip_init(&c, "мах", 1.f, 1);
    lu_clip_key_r(&c, hip, 0, Q_ID()); lu_clip_key_r(&c, hip, .5f, quat_axis(V3(1, 0, 0), LU_PI / 2)); lu_clip_key_r(&c, hip, 1, Q_ID());
    lu_clip_event(&c, .25f, 7);
    lu_clip_sample(&c, &sk, .5f, &p); lu_pose_model(&sk, &p, M);
    CHECK(v3near(M[foot].t, V3(0, 1, -1), 1e-4f));        /* поворот вокруг X на +90°: -Y переходит в -Z */
    lu_clip_sample(&c, &sk, 1.5f, &p); lu_pose_model(&sk, &p, M);
    CHECK(v3near(M[foot].t, V3(0, 1, -1), 1e-4f));        /* зацикливание */
    int ev[4]; CHECK(lu_clip_events(&c, .2f, .3f, ev, 4) == 1 && ev[0] == 7);
    CHECK(lu_clip_events(&c, .9f, 1.3f, ev, 4) == 1);     /* событие после перехода цикла */
    CHECK(lu_clip_events(&c, .3f, .9f, ev, 4) == 0);
    printf("смешивание…\n");
    lu_pose a, b; lu_pose_bind(&sk, &a); lu_clip_sample(&c, &sk, .5f, &b); lu_pose_blend(&a, &b, .5f, &p); lu_pose_model(&sk, &p, M);
    CHECK(v3near(M[foot].t, V3(0, 1 - cosf(LU_PI / 4), -sinf(LU_PI / 4)), 1e-3f));
    printf("граф…\n");
    lu_clip walk, run; lu_clip_init(&walk, "шаг", 1.f, 1); lu_clip_init(&run, "бег", .5f, 1);
    walk.root_joint = root; walk.root_motion = 1; run.root_joint = root; run.root_motion = 1;
    lu_clip_key_t(&walk, root, 0, V3(0, 0, 0)); lu_clip_key_t(&walk, root, 1, V3(0, 0, 1.5f));   /* 1.5 м/с */
    lu_clip_key_t(&run, root, 0, V3(0, 0, 0)); lu_clip_key_t(&run, root, .5f, V3(0, 0, 2.5f));  /* 5 м/с */
    lu_animgraph g; lu_ag_init(&g, &sk);
    const lu_clip *cl[2] = {&walk, &run}; float at[2] = {1.5f, 5.f};
    int loco = lu_ag_state_blend(&g, "движение", 0, cl, at, 2, 1);
    g.param[0] = 1.5f; lu_ag_play(&g, loco, 0, 0);
    v3 sum = V3(0, 0, 0); for (int i = 0; i < 120; i++) { lu_ag_update(&g, 1.f / 60); sum = v3_add(sum, g.root_motion); }
    printf("  шаг: за 2 с смещение %.2f м (ожидается 3.0)\n", sum.z); CHECK(fabsf(sum.z - 3.f) < .1f);
    g.param[0] = 5.f; sum = V3(0, 0, 0); for (int i = 0; i < 120; i++) { lu_ag_update(&g, 1.f / 60); sum = v3_add(sum, g.root_motion); }
    printf("  бег: за 2 с смещение %.2f м (ожидается 10.0)\n", sum.z); CHECK(fabsf(sum.z - 10.f) < .3f);
    g.param[0] = 3.25f; sum = V3(0, 0, 0); for (int i = 0; i < 120; i++) { lu_ag_update(&g, 1.f / 60); sum = v3_add(sum, g.root_motion); }
    printf("  середина: %.2f м\n", sum.z); CHECK(sum.z > 4.f && sum.z < 9.f);
    int sw = lu_ag_state_clip(&g, "мах", &c, 1); lu_ag_play(&g, sw, .2f, 0);
    lu_ag_update(&g, .1f); CHECK(g.prev == loco); lu_ag_update(&g, .2f); CHECK(g.prev == -1);
    printf("IK…\n");
    lu_pose_bind(&sk, &p); v3 tgt = V3(0, .3f, .4f);
    int ok = lu_ik_two_bone(&sk, &p, hip, knee, foot, tgt, V3(0, .5f, 2), 1); lu_pose_model(&sk, &p, M);
    CHECK(ok); CHECK(v3near(M[foot].t, tgt, 1e-3f)); CHECK(M[knee].t.z > 0); /* колено в сторону полюса */
    CHECK(fabsf(v3_dist(M[hip].t, M[knee].t) - .5f) < 1e-4f);
    ok = lu_ik_two_bone(&sk, &p, hip, knee, foot, V3(0, -2, 0), V3(0, .5f, 2), 1); CHECK(!ok);
    lu_pose_bind(&sk, &p); lu_ik_look(&sk, &p, knee, V3(0, -1, 0), V3(0, .5f, 5), 1, 3.f); lu_pose_model(&sk, &p, M);
    CHECK(v3near(v3_norm(v3_sub(M[foot].t, M[knee].t)), v3_norm(V3(0, 0, 4.5f)), 2e-3f));
    lu_clip_free(&c); lu_clip_free(&walk); lu_clip_free(&run);
    printf(fails ? "ЕСТЬ ОШИБКИ: %d\n" : "все тесты анимации пройдены\n", fails);
    return fails != 0;
}
