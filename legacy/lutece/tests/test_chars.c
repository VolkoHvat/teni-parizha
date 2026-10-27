/* Снимок персонажей Lutèce: разные наряды и позы из библиотеки движений, скиннинг на видеокарте */
#include "../platform/lu_platform.h"
#include "../gfx/lu_gfx.h"
#include "../anim/lu_avatar.h"
#include <stdio.h>
int lu_headless_snapshot(const char *path);
typedef struct { lu_skeleton sk; lu_humanoid h; lu_mesh mesh; int mot; float t; v3 pos; float yaw; int wall; } Ch;
int main(int argc, char **argv) {
    const char *out = argc > 1 ? argv[1] : "chars.ppm"; int mode = argc > 2 ? atoi(argv[2]) : 0; float tt = argc > 3 ? (float)atof(argv[3]) : .3f;
    lu_window_desc wd = {"t", 1280, 720, 0, 0, 0}; if (lu_platform_init(&wd)) return 1;
    if (lu_gfx_init()) { printf("%s\n", lu_gfx_error()); return 2; }
    lu_gfx_set_quality(2);
    lu_skeleton ref; lu_humanoid rh; lu_humanoid_skeleton(&ref, &rh, 1.78f, 0);
    double t0 = lu_time(); lu_motions mot; lu_motions_build(&mot, &ref, &rh); printf("библиотека движений: %.1f мс\n", (lu_time() - t0) * 1000);
    lu_material am[LU_AV_NMAT]; lu_avatar_materials(am);
    static Ch ch[16]; int n = 0;
    int poses0[10] = {LU_MOT_IDLE, LU_MOT_WALK, LU_MOT_RUN, LU_MOT_SPRINT, LU_MOT_LEAP_AIR, LU_MOT_ROLL, LU_MOT_ATK1, LU_MOT_TALK, LU_MOT_CROUCH_WALK, LU_MOT_SIT};
    int poses1[10] = {LU_MOT_CLIMB, LU_MOT_HANG, LU_MOT_BRACE, LU_MOT_SHIMMY_L, LU_MOT_CLIMB_UP, LU_MOT_WALLRUN, LU_MOT_CLIMB_UP, LU_MOT_VAULT, LU_MOT_FALL, LU_MOT_LAND_HARD};
    float ctimes[10] = {0, 0, 0, 0, .25f, .5f, .3f, .2f, .3f, .4f};
    for (int i = 0; i < 10; i++) {
        lu_outfit o; if (i == 0) { lu_outfit_default(&o, 0); o.coat = LU_COAT_HOODED; o.coat_color = 0xFFE6E6EEu; o.coat_trim = 0xFF2A1EB4u; o.sash = 1; o.sword = 1; o.pants = 0xFF3A3A40u; o.shoes = 0xFF1E1A18u; o.stockings = 0xFF3A3A40u; }
        else lu_outfit_random(&o, 100 + i * 7);
        Ch *c = &ch[n++]; t0 = lu_time(); lu_avatar_build(&o, &c->sk, &c->h, &c->mesh, 1);
        if (i < 2) printf("персонаж %d: %d вершин, %d треугольников, %.1f мс\n", i, c->mesh.nverts, c->mesh.nidx / 3, (lu_time() - t0) * 1000);
        c->mot = mode != 1 ? poses0[i] : poses1[i]; c->t = mode != 1 ? (ctimes[i] + tt) * mot.clip[c->mot].duration : tt * mot.clip[c->mot].duration;
        if (mode == 1 && i == 6) c->t = .85f * mot.clip[c->mot].duration;
        c->pos = V3((i - 4.5f) * 1.25f, 0, 0); c->yaw = mode != 1 ? .5f : 0; c->wall = mode == 1 && i < 7;
    }
    lu_mesh_builder b; lu_mb_init(&b); lu_mb_box(&b, V3(-30, -1, -30), V3(30, 0, 30), .5f); lu_mesh ground = lu_mesh_create(&b, 0); lu_mb_free(&b);
    lu_mb_init(&b); lu_mb_box(&b, V3(-.6f, 0, -.5f), V3(.6f, 2.05f, .5f), .5f); lu_mesh wall = lu_mesh_create(&b, 0); lu_mb_free(&b);
    lu_material mg = lu_material_default(); mg.color = V4(.55f, .52f, .48f, 1); mg.roughness = .9f;
    lu_material mw = lu_material_default(); mw.color = V4(.7f, .66f, .6f, 1);
    lu_env e = lu_env_default(); e.sun_dir = v3_norm(V3(-.5f, .7f, .6f)); e.time = 1;
    for (int f = 0; f < 2; f++) {
        lu_gfx_begin(1280, 720, lu_platform_fbo());
        lu_camera cam = mode == 2 ? (lu_camera){V3(-4.6f, 1.55f, 2.6f), V3(-5.0f, 1.25f, 0), V3(0, 1, 0), .75f, .05f, 200} : mode == 0 ? (lu_camera){V3(0, 1.6f, 7.5f), V3(0, 1.0f, 0), V3(0, 1, 0), .9f, .1f, 200} : (lu_camera){V3(1.5f, 2.2f, -7.5f), V3(0, 1.3f, 0), V3(0, 1, 0), .9f, .1f, 200};
        lu_gfx_camera(&cam); lu_gfx_env(&e);
        lu_draw(&ground, &mg, 1, m4_id());
        for (int i = 0; i < n; i++) {
            Ch *c = &ch[i]; lu_pose p; lu_clip_sample(&mot.clip[c->mot], &c->sk, c->t, &p);
            xform M[LU_MAX_BONES]; m4 pal[LU_MAX_BONES]; lu_pose_model(&c->sk, &p, M); lu_pose_palette(&c->sk, M, pal);
            m4 W = m4_trs(c->pos, quat_axis(V3(0, 1, 0), c->yaw), V3(1, 1, 1));
            lu_draw_skinned(&c->mesh, am, LU_AV_NMAT, W, pal, c->sk.n);
            if (c->wall) { float k = c->sk.tscale; lu_draw(&wall, &mw, 1, m4_mul(W, m4_trs(V3(0, 0, .32f * k + .5f), Q_ID(), V3(1, k, 1)))); }
        }
        lu_ui_text(20, 16, 22, LU_RGBA(255, 240, 210, 255), LU_TEXT_SHADOW, mode == 0 ? "Lutèce: персонажи и движения" : "Lutèce: паркур — лазание, вис, карниз, подтягивание");
        lu_gfx_end(); lu_platform_swap();
    }
    lu_gfx_stats st = lu_gfx_get_stats(); printf("скелетных %d, вызовов %d, треугольников %d\n", st.skinned, st.draws, st.tris);
    lu_headless_snapshot(out); return 0;
}
