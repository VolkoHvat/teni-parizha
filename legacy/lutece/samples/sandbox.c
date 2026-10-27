/* Lutèce Sandbox — демонстрация движка: квартал Парижа 1789 года, свободный бег по крышам, толпа, смена дня и ночи.
   Windows: окно, клавиатура/мышь/геймпад. Linux (LU_HEADLESS): автопилот и снимки кадров для проверки. */
#include "../platform/lu_platform.h"
#include "../gfx/lu_gfx.h"
#include "../anim/lu_avatar.h"
#include "../game/lu_traversal.h"
#include "../world/lu_city.h"
#include <stdio.h>

#define NNPC 48
typedef struct { lu_skeleton sk; lu_humanoid h; lu_mesh mesh; lu_animgraph ag; lu_cc cc; float yaw; v3 goal; float wait, speed; int idle_mot; } Npc;
typedef struct {
    lu_phys_world phys; lu_city city; lu_material cmat[LU_CM_N]; lu_texture ctex[LU_CM_N][3];
    lu_skeleton ref_sk; lu_humanoid ref_h; lu_motions mot;
    lu_skeleton hero_sk; lu_humanoid hero_h; lu_mesh hero_mesh; lu_material amat[LU_AV_NMAT];
    lu_traversal tr; lu_tp_camera cam;
    Npc npc[NNPC]; int nnpc;
    LU_VEC(v3) walk_pts;
    float tod, time; int show_help, show_prof, quality; double fps_t; int fps_frames; float fps;
    int last_state; float msg_t; char msg[128];
} App;
static App A;

/* ---------- квартал ---------- */
#define C3(r, g, b) ((uint32_t)(0xFF000000u | ((uint32_t)(b) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(r)))
static void build_level(App *a) {
    lu_phys_init(&a->phys);
    aabb area = {V3(-20, -2, -20), V3(160, 40, 140)};
    lu_city_init(&a->city, &a->phys, area, 24.f);
    lu_city *c = &a->city;
    /* земля: булыжник улиц и плиты тротуаров/площади */
    lu_city_ground(c, LU_CM_COBBLE, -20, -20, 160, 140, 0, 1, LU_SURF_STONE);
    lu_rng r = {20240789};
    static const uint32_t TINTS[6] = {C3(255, 250, 240), C3(250, 238, 215), C3(238, 228, 210), C3(255, 242, 225), C3(232, 225, 214), C3(245, 232, 205)};
    static const uint32_t SHUT[5] = {C3(60, 80, 70), C3(90, 50, 40), C3(70, 90, 110), C3(200, 195, 180), C3(50, 60, 80)};
    /* кварталы 3×2, между ними улицы 8 м; в каждом квартале переулок 3 м */
    float bxs[3][2] = {{0, 30}, {38, 68}, {104, 134}}, bzs[2][2] = {{0, 26}, {82, 108}};
    for (int bi = 0; bi < 3; bi++) for (int bj = 0; bj < 2; bj++) {
        float x0 = bxs[bi][0], x1 = bxs[bi][1], z0 = bzs[bj][0], z1 = bzs[bj][1];
        lu_city_ground(c, LU_CM_FLAG, x0 - 2, z0 - 2, x1 + 2, z1 + 2, .12f, 1, LU_SURF_STONE);
        float xm = (x0 + x1) * .5f, alley = 3.2f;
        float lots[4][4] = {{x0, z0, xm - alley * .5f, (z0 + z1) * .5f}, {xm + alley * .5f, z0, x1, (z0 + z1) * .5f}, {x0, (z0 + z1) * .5f, xm - alley * .5f, z1}, {xm + alley * .5f, (z0 + z1) * .5f, x1, z1}};
        for (int l = 0; l < 4; l++) {
            /* делим участок на 1–2 дома разной высоты */
            float lx0 = lots[l][0], lz0 = lots[l][1], lx1 = lots[l][2], lz1 = lots[l][3];
            int split = rng_f(&r) < .6f; int nb = split ? 2 : 1;
            for (int s = 0; s < nb; s++) {
                lu_building B; memset(&B, 0, sizeof B);
                float sx0 = lx0, sx1 = lx1; if (split) { float m = (lx0 + lx1) * .5f; if (s == 0) sx1 = m; else sx0 = m; }
                B.x0 = sx0; B.z0 = lz0; B.x1 = sx1; B.z1 = lz1; B.base_y = .12f;
                B.floors = 3 + (int)(rng_f(&r) * 3); B.floor_h = 3.1f; B.ground_h = 3.6f;
                float wm = rng_f(&r); B.wall_mat = wm < .55f ? LU_CM_STONE : wm < .9f ? LU_CM_PLASTER : LU_CM_BRICK;
                B.tint = TINTS[rng_u32(&r) % 6]; B.shutter_col = rng_f(&r) < .75f ? SHUT[rng_u32(&r) % 5] : 0; B.trim_col = C3(235, 230, 220);
                float rf = rng_f(&r); B.roof = rf < .6f ? LU_ROOF_MANSARD : rf < .85f ? LU_ROOF_GABLE : LU_ROOF_FLAT;
                /* стороны на улицу: внешние стороны квартала и переулок */
                int sides = 0;
                if (fabsf(B.z0 - z0) < .1f) sides |= 1;
                if (fabsf(B.z1 - z1) < .1f) sides |= 4;
                if (fabsf(B.x1 - x1) < .1f || fabsf(B.x1 - (xm - alley * .5f)) < .1f) sides |= 2;
                if (fabsf(B.x0 - x0) < .1f || fabsf(B.x0 - (xm + alley * .5f)) < .1f) sides |= 8;
                B.street_sides = sides; B.shops = sides & (rng_f(&r) < .6f ? 5 : 0); B.balcony_floor = rng_f(&r) < .6f ? 1 : 0;
                B.seed = rng_u32(&r);
                lu_city_building(c, &B);
            }
        }
    }
    /* площадь (между кварталами по центру) */
    float px0 = 30, px1 = 104, pz0 = 34, pz1 = 74;
    lu_city_ground(c, LU_CM_FLAG, px0, pz0, px1, pz1, .06f, .7f, LU_SURF_STONE);
    lu_city_fountain(c, V3(67, .06f, 54), 3.4f);
    for (int i = 0; i < 6; i++) lu_city_stall(c, V3(44 + i * 4.2f, .06f, 40), 0, i & 1 ? C3(170, 40, 40) : C3(230, 220, 200));
    for (int i = 0; i < 6; i++) lu_city_stall(c, V3(70 + i * 4.2f, .06f, 68), LU_PI, i & 1 ? C3(40, 80, 140) : C3(200, 170, 60));
    for (int i = 0; i < 8; i++) { lu_city_tree(c, V3(34 + i * 9.f, .06f, 47), 7.5f, (uint32_t)i); lu_city_tree(c, V3(34 + i * 9.f, .06f, 61), 7.f, (uint32_t)i + 50); }
    lu_city_haycart(c, V3(20, .12f, 31), .3f); lu_city_haycart(c, V3(88, .06f, 76.5f), 1.4f); lu_city_haycart(c, V3(52, .12f, 79), 0);
    lu_city_crates(c, V3(58, .06f, 36), 3, 5); lu_city_crates(c, V3(96, .06f, 60), 4, 9); lu_city_crates(c, V3(14, .12f, 30), 2, 3);
    for (float x = -10; x < 150; x += 16) { lu_city_lamp(c, V3(x, .12f, -3)); lu_city_lamp(c, V3(x, .12f, 29)); lu_city_lamp(c, V3(x, .12f, 79)); lu_city_lamp(c, V3(x, .12f, 111)); }
    lu_city_finish(c);
    /* точки для прогулок горожан: тротуары и площадь */
    for (int i = 0; i < 200; i++) {
        v3 p; float u = rng_f(&r);
        if (u < .4f) p = V3(rng_range(&r, px0 + 2, px1 - 2), .06f, rng_range(&r, pz0 + 2, pz1 - 2));
        else if (u < .7f) p = V3(rng_range(&r, -8, 140), .12f, rng_f(&r) < .5f ? -1.5f : 28.f);
        else p = V3(rng_range(&r, -8, 140), .12f, rng_f(&r) < .5f ? 80.5f : 110.f);
        if (lu_capsule_overlap(&a->phys, V3(p.x, p.y + .4f, p.z), V3(p.x, p.y + 1.6f, p.z), .35f)) continue;
        lu_vec_push(a->walk_pts, p);
    }
}
/* ---------- окружение по времени суток ---------- */
static void env_for(float tod, lu_env *e) {
    *e = lu_env_default();
    float ang = (tod - 6.f) / 12.f * LU_PI; v3 sun = v3_norm(V3(-cosf(ang) * .8f, sinf(ang), .45f));
    float day = lu_smooth((sun.y + .05f) / .3f), dusk = lu_smooth(1 - fabsf(sun.y - .08f) / .2f);
    e->night = 1 - day;
    v3 sunc = v3_lerp(V3(3.2f, 2.9f, 2.5f), V3(3.f, 1.5f, .6f), dusk);
    e->sun_color = v3_scale(sunc, day); e->sun_dir = day > .02f ? sun : v3_norm(V3(.3f, .6f, -.5f));
    if (day < .05f) e->sun_color = V3(.05f, .07f, .12f);
    e->sky_zenith = v3_lerp(V3(.004f, .006f, .018f), v3_lerp(V3(.16f, .32f, .78f), V3(.2f, .22f, .45f), dusk), day);
    e->sky_horizon = v3_lerp(V3(.015f, .02f, .045f), v3_lerp(V3(.6f, .72f, .9f), V3(1.05f, .55f, .3f), dusk), day);
    e->ground = v3_lerp(V3(.01f, .01f, .014f), V3(.24f, .22f, .19f), day);
    e->fog_color = v3_lerp(V3(.02f, .025f, .04f), v3_lerp(V3(.55f, .62f, .74f), V3(.85f, .55f, .4f), dusk), day);
    e->fog_density = .006f + .004f * e->night; e->exposure = lu_lerpf(2.4f, .6f, day); e->bloom = lu_lerpf(.12f, .05f, day);
}
/* ---------- горожане ---------- */
static void npc_new_goal(App *a, Npc *n, lu_rng *r) { if (a->walk_pts.n) n->goal = a->walk_pts.v[rng_u32(r) % (uint32_t)a->walk_pts.n]; }
static void spawn_npcs(App *a) {
    lu_rng r = {99};
    for (int i = 0; i < NNPC && a->walk_pts.n; i++) {
        Npc *n = &a->npc[a->nnpc++]; lu_outfit o; lu_outfit_random(&o, 1000 + (uint32_t)i * 13u);
        lu_avatar_build(&o, &n->sk, &n->h, &n->mesh, 0);
        v3 p = a->walk_pts.v[rng_u32(&r) % (uint32_t)a->walk_pts.n]; lu_cc_init(&n->cc, p); n->cc.radius = .28f; n->cc.grounded = 1;
        lu_ag_init(&n->ag, &n->sk);
        const lu_clip *lc[3] = {&a->mot.clip[LU_MOT_IDLE], &a->mot.clip[LU_MOT_WALK], &a->mot.clip[LU_MOT_RUN]};
        float k = n->sk.tscale, at[3] = {0, lu_gait_speed[1] * k, lu_gait_speed[2] * k};
        int st = lu_ag_state_blend(&n->ag, "move", 0, lc, at, 3, 1); lu_ag_state_clip(&n->ag, "talk", &a->mot.clip[LU_MOT_TALK], 1); lu_ag_state_clip(&n->ag, "look", &a->mot.clip[LU_MOT_LOOK_AROUND], 1);
        lu_ag_play(&n->ag, st, 0, rng_f(&r)); n->speed = rng_range(&r, 1.1f, 1.5f) * k; n->yaw = rng_f(&r) * LU_TAU;
        npc_new_goal(a, n, &r);
    }
}
static void npcs_update(App *a, float dt) {
    static lu_rng r = {7};
    for (int i = 0; i < a->nnpc; i++) {
        Npc *n = &a->npc[i]; v3 d = v3_sub(n->goal, n->cc.pos); d.y = 0; float dist = v3_len(d);
        v3 want = V3(0, 0, 0);
        if (n->wait > 0) { n->wait -= dt; if (n->wait <= 0) { npc_new_goal(a, n, &r); lu_ag_play(&n->ag, 0, .4f, 0); } }
        else if (dist < 1.f) { n->wait = rng_range(&r, 2, 8); int s = 1 + (int)(rng_u32(&r) % 2); lu_ag_play(&n->ag, s, .4f, 0); }
        else want = v3_scale(d, n->speed / dist);
        /* расталкивание: отходим от соседей и героя */
        for (int j = 0; j < a->nnpc; j++) { if (j == i) continue; v3 o = v3_sub(n->cc.pos, a->npc[j].cc.pos); o.y = 0; float l = v3_len(o); if (l < .8f && l > 1e-3f) want = v3_mad(want, o, (.8f - l) / l * 2.5f); }
        { v3 o = v3_sub(n->cc.pos, a->tr.cc.pos); o.y = 0; float l = v3_len(o); if (l < 1.f && l > 1e-3f && fabsf(a->tr.cc.pos.y - n->cc.pos.y) < 1.5f) want = v3_mad(want, o, (1.f - l) / l * 4.f); }
        n->cc.vel.x = lu_damp(n->cc.vel.x, want.x, 6, dt); n->cc.vel.z = lu_damp(n->cc.vel.z, want.z, 6, dt); n->cc.vel.y = n->cc.grounded ? -1 : n->cc.vel.y - 20 * dt;
        lu_cc_move(&n->cc, &a->phys, dt, 1);
        if (n->cc.hit_wall && n->wait <= 0) npc_new_goal(a, n, &r);
        float hs = v3_hlen(n->cc.vel); if (hs > .2f) { float ty = atan2f(n->cc.vel.x, n->cc.vel.z); n->yaw += lu_wrap_pi(ty - n->yaw) * fminf(1.f, dt * 6); }
        n->ag.param[0] = hs; lu_ag_update(&n->ag, dt);
    }
}
/* ---------- инициализация ---------- */
static int app_init(App *a, int w, int h) {
    LU_UNUSED(w); LU_UNUSED(h);
    if (lu_gfx_init()) return 1;
    lu_jobs_init(0);
    double t0 = lu_time();
    lu_city_materials(a->cmat, a->ctex, 512);
    LU_INFO("текстуры: %.0f мс", (lu_time() - t0) * 1000); t0 = lu_time();
    build_level(a);
    LU_INFO("квартал: %.0f мс", (lu_time() - t0) * 1000); t0 = lu_time();
    lu_humanoid_skeleton(&a->ref_sk, &a->ref_h, 1.78f, 0); lu_motions_build(&a->mot, &a->ref_sk, &a->ref_h);
    lu_outfit o; lu_outfit_default(&o, 0); o.coat = LU_COAT_HOODED; o.coat_color = C3(236, 234, 228); o.coat_trim = C3(170, 30, 36); o.sash = 1; o.sword = 1;
    o.pants = C3(60, 58, 64); o.stockings = C3(60, 58, 64); o.shoes = C3(40, 32, 28); o.hair = C3(50, 36, 28);
    lu_avatar_build(&o, &a->hero_sk, &a->hero_h, &a->hero_mesh, 1); lu_avatar_materials(a->amat);
    lu_tr_init(&a->tr, &a->hero_sk, &a->hero_h, &a->mot, V3(60, .06f, 50), LU_PI * .5f);
    lu_tpcam_init(&a->cam, LU_PI * .5f);
    spawn_npcs(a);
    LU_INFO("персонажи: %.0f мс", (lu_time() - t0) * 1000);
    a->tod = 15.5f; a->show_help = 1; a->quality = 2; lu_gfx_set_quality(a->quality);
    return 0;
}
/* ---------- кадр ---------- */
static void app_update(App *a, const lu_input *in, float dt) {
    a->time += dt;
    lu_tr_input ti; memset(&ti, 0, sizeof ti);
    float mx = (in->down['D'] ? 1.f : 0) - (in->down['A'] ? 1.f : 0) + in->pad_lx, mz = (in->down['W'] ? 1.f : 0) - (in->down['S'] ? 1.f : 0) + in->pad_ly;
    float cy = a->cam.yaw; v3 fwdv = V3(sinf(cy), 0, cosf(cy)), right = V3(-cosf(cy), 0, sinf(cy));
    v3 mv = v3_add(v3_scale(fwdv, mz), v3_scale(right, mx)); if (v3_len(mv) > 1) mv = v3_norm(mv);
    ti.move = mv; ti.sprint = in->down[LU_KEY_SHIFT] || in->pad_rt > .4f || in->down[LU_MOUSE_RIGHT];
    ti.jump = in->pressed[LU_KEY_SPACE] || (in->pad_pressed & LU_PAD_A); ti.crouch = in->down[LU_KEY_CTRL] || in->down['C'] || (in->pad_down & LU_PAD_B);
    ti.drop = in->pressed['Q'] || (in->pad_pressed & LU_PAD_B); ti.turn = in->pressed['R'] || (in->pad_pressed & LU_PAD_X);
    lu_tr_update(&a->tr, &a->phys, &ti, dt);
    if (a->tr.state != a->last_state) { a->last_state = a->tr.state; }
    npcs_update(a, dt);
    float lx = -in->mouse_dx * .0025f - in->pad_rx * 2.6f * dt, ly = in->mouse_dy * .0022f - in->pad_ry * 1.8f * dt;
    v3 focus = v3_add(a->tr.cc.pos, V3(0, 1.55f, 0));
    if (a->tr.state == LU_TR_HANG || a->tr.state == LU_TR_CLIMB) focus.y += .4f;
    lu_tpcam_update(&a->cam, &a->phys, focus, lx, ly, dt);
    if (in->pressed['T']) a->tod = fmodf(a->tod + 1.f, 24.f);
    if (in->pressed[LU_KEY_F1]) a->show_help = !a->show_help;
    if (in->pressed[LU_KEY_F3]) a->show_prof = !a->show_prof;
    if (in->pressed[LU_KEY_F2]) { a->quality = (a->quality + 1) % 4; lu_gfx_set_quality(a->quality); }
    a->tod += dt / 60.f * .5f; if (a->tod >= 24) a->tod -= 24;   /* сутки за 48 минут */
}
static void draw_char(const lu_skeleton *s, const lu_mesh *m, const lu_material *mats, const xform *model, m4 W) {
    m4 pal[LU_MAX_BONES]; lu_pose_palette(s, model, pal); lu_draw_skinned(m, mats, LU_AV_NMAT, W, pal, s->n);
}
static void app_render(App *a, int w, int h, unsigned fbo) {
    lu_gfx_begin(w, h, fbo);
    lu_camera cam = {a->cam.eye, a->cam.target, V3(0, 1, 0), a->cam.fov, .1f, 600.f}; lu_gfx_camera(&cam);
    lu_env e; env_for(a->tod, &e); e.time = a->time; e.wind = .5f; lu_gfx_env(&e);
    lu_city_draw(&a->city, a->cmat);
    /* фонари ночью */
    if (e.night > .2f) { float k = lu_smooth((e.night - .2f) / .4f);
        for (int i = 0; i < a->city.lamps.n; i++) { v3 p = a->city.lamps.v[i]; if (v3_dist(p, a->cam.eye) < 70) lu_light(p, 13.f, V3(7.f * k, 4.2f * k, 1.8f * k)); }
        a->cmat[LU_CM_LAMP].emissive = V3(5 * k, 3.2f * k, 1.4f * k); } else a->cmat[LU_CM_LAMP].emissive = V3(0, 0, 0);
    draw_char(&a->hero_sk, &a->hero_mesh, a->amat, a->tr.model, lu_tr_world(&a->tr));
    for (int i = 0; i < a->nnpc; i++) {
        Npc *n = &a->npc[i]; if (v3_dist(n->cc.pos, a->cam.eye) > 90) continue;
        xform M[LU_MAX_BONES]; lu_pose_model(&n->sk, &n->ag.pose, M);
        draw_char(&n->sk, &n->mesh, a->amat, M, m4_trs(n->cc.pos, quat_axis(V3(0, 1, 0), n->yaw), V3(1, 1, 1)));
    }
    /* интерфейс */
    float s = h / 720.f; char b[256];
    a->fps_frames++; if (lu_time() - a->fps_t > .5) { a->fps = (float)(a->fps_frames / (lu_time() - a->fps_t)); a->fps_t = lu_time(); a->fps_frames = 0; }
    lu_ui_rect(14 * s, 14 * s, 330 * s, 64 * s, LU_RGBA(12, 10, 20, 170));
    lu_ui_text(26 * s, 20 * s, 24 * s, LU_RGBA(244, 214, 130, 255), LU_TEXT_SHADOW | LU_TEXT_BOLD, "Lutèce · Paris 1789");
    int hh = (int)a->tod, mm = (int)((a->tod - hh) * 60);
    snprintf(b, sizeof b, "%02d:%02d   %s   %.0f кадр/с", hh, mm, lu_tr_names[a->tr.state], a->fps);
    lu_ui_text(26 * s, 50 * s, 17 * s, LU_RGBA(236, 230, 220, 255), LU_TEXT_SHADOW, b);
    if (a->show_help) {
        static const char *H[] = {"WASD — движение, мышь — камера", "Shift (или ПКМ) — свободный бег: забегать на стены, прыгать с крыш",
            "Пробел — прыжок / перемахнуть / ухватиться", "Ctrl — красться; у края крыши — спуститься в вис", "Q — отпустить руки, R — разворот на бегу",
            "T — +1 час, F2 — качество, F3 — профайлер, F11 — полный экран", "F1 — скрыть подсказки, Esc — освободить мышь"};
        float y = h - (24 * LU_ARRAY_N(H) + 20) * s;
        lu_ui_rect(14 * s, y - 8 * s, 520 * s, (24 * LU_ARRAY_N(H) + 14) * s, LU_RGBA(12, 10, 20, 150));
        for (int i = 0; i < LU_ARRAY_N(H); i++) lu_ui_text(26 * s, y + i * 24 * s, 17 * s, LU_RGBA(236, 230, 220, 255), LU_TEXT_SHADOW, H[i]);
    }
    if (a->show_prof) {
        lu_gfx_stats st = lu_gfx_get_stats();
        snprintf(b, sizeof b, "вызовов %d · инстансов %d · треугольников %.1fM · теней %d · скелетных %d", st.draws, st.instances, st.tris / 1e6f, st.shadow_draws, st.skinned);
        lu_ui_text(w - 20 * s, 20 * s, 15 * s, LU_RGBA(200, 230, 200, 255), LU_ALIGN_RIGHT | LU_TEXT_SHADOW, b);
        const lu_prof_entry *pe; int np = lu_prof_list(&pe);
        for (int i = 0; i < np; i++) { snprintf(b, sizeof b, "%s: %.2f мс", pe[i].name, pe[i].avg); lu_ui_text(w - 20 * s, (44 + i * 20) * s, 15 * s, LU_RGBA(200, 230, 200, 255), LU_ALIGN_RIGHT | LU_TEXT_SHADOW, b); }
    }
    lu_gfx_end();
}

#ifdef LU_HEADLESS
int lu_headless_snapshot(const char *path);
/* автопилот: маршрут по площади, на дом и по крышам; снимки ключевых моментов */
int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : "."; float tod = argc > 2 ? (float)atof(argv[2]) : 15.5f;
    lu_window_desc wd = {"sandbox", 1280, 720, 0, 0, 0}; if (lu_platform_init(&wd)) return 1;
    if (app_init(&A, 1280, 720)) { printf("ошибка: %s\n", lu_gfx_error()); return 2; }
    A.tod = tod; A.show_help = 0; A.show_prof = 1;
    lu_input in; memset(&in, 0, sizeof in);
    float dt = 1.f / 30; char path[512]; int shot = 0;
    struct { float t; const char *name; } shots[] = {{.1f, "start"}, {3.f, "plaza_run"}, {6.5f, "wall"}, {9.f, "climb"}, {13.f, "roof"}, {17.f, "roofrun"}, {21.f, "leap"}};
    A.cam.yaw = 0; lu_tr_teleport(&A.tr, V3(60, .06f, 44), 0);
    for (int f = 0; f < 30 * 22; f++) {
        float t = f * dt; memset(in.pressed, 0, sizeof in.pressed);
        memset(in.down, 0, sizeof in.down);
        /* бежим на север (−Z) к кварталу с домами, затем по крышам */
        A.cam.yaw = LU_PI; in.down['W'] = 1; in.down[LU_KEY_SHIFT] = t > 1.f;
        if (A.tr.state == LU_TR_HANG && t > 2) in.down['W'] = 1;
        app_update(&A, &in, dt);
        if (shot < (int)LU_ARRAY_N(shots) && t >= shots[shot].t) {
            double t0 = lu_time(); app_render(&A, 1280, 720, lu_platform_fbo()); lu_platform_swap();
            snprintf(path, sizeof path, "%s/sb_%s.ppm", dir, shots[shot].name); lu_headless_snapshot(path);
            printf("%5.2f с  %-14s (%.1f, %.1f, %.1f)  кадр %.0f мс\n", t, lu_tr_names[A.tr.state], A.tr.cc.pos.x, A.tr.cc.pos.y, A.tr.cc.pos.z, (lu_time() - t0) * 1000);
            shot++;
        }
    }
    return 0;
}
#else
int lu_main(int argc, char **argv) {
    LU_UNUSED(argc); LU_UNUSED(argv);
    lu_log_to_file("lutece_log.txt");
    lu_window_desc wd = {"Lutèce — Paris 1789 (песочница движка)", 1600, 900, 0, 1, 0};
    if (lu_platform_init(&wd)) return 1;
    if (app_init(&A, 1600, 900)) { LU_ERR("рендер: %s", lu_gfx_error()); return 2; }
    lu_input_capture_mouse(1);
    double prev = lu_time(), acc = 0; const float step = 1.f / 60;
    while (lu_platform_pump()) {
        const lu_input *in = lu_input_get();
        if (in->pressed[LU_KEY_ESC]) lu_input_capture_mouse(!in->mouse_captured);
        if (in->pressed[LU_MOUSE_LEFT] && !in->mouse_captured) lu_input_capture_mouse(1);
        if (in->pressed[LU_KEY_F11]) lu_platform_fullscreen(!lu_platform_is_fullscreen());
        double now = lu_time(); double fdt = now - prev; prev = now; if (fdt > .1) fdt = .1; acc += fdt;
        int steps = 0; lu_input stepped = *in;
        LU_PROF("логика") { while (acc >= step && steps < 4) { app_update(&A, &stepped, step); memset(stepped.pressed, 0, sizeof stepped.pressed); stepped.mouse_dx = stepped.mouse_dy = 0; stepped.pad_pressed = 0; acc -= step; steps++; } }
        if (acc > step * 4) acc = 0;
        int w, h; lu_platform_size(&w, &h); if (w < 16 || h < 16) { lu_sleep_ms(10); continue; }
        LU_PROF("рендер") app_render(&A, w, h, lu_platform_fbo());
        lu_platform_swap(); lu_prof_frame();
    }
    lu_platform_shutdown();
    return 0;
}
#endif
