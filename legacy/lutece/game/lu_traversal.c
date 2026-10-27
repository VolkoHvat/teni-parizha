/* Lutèce Engine — свободный бег: автомат состояний, анимация, IK стоп */
#include "lu_traversal.h"

const char *lu_tr_names[LU_TR_N] = {"земля", "толчок", "полёт", "приземление", "кувырок", "лазание", "вис", "подтягивание", "спуск", "перемах", "бег по стене", "разворот", "перескок"};
#define LEDGE_Y 2.05f
#define WALL_Z .32f

void lu_tr_default_params(lu_tr_params *p) {
    memset(p, 0, sizeof *p); p->on = 1;
    p->walk = 1.45f; p->run = 4.6f; p->sprint = 7.4f; p->jump_v = 6.2f; p->leap_v = 6.6f; p->gravity = 20.f;
    p->climb_speed = 1.1f; p->shimmy_speed = 1.0f; p->roll_height = 3.2f; p->hurt_height = 7.f;
    p->auto_leap = p->auto_climb = p->auto_vault = p->ledge_grab = p->roll = 1;
}
static void set_state(lu_traversal *t, int st) { t->state = st; t->st_t = 0; }
static void play(lu_traversal *t, int mot, float fade) { lu_ag_play(&t->ag, t->st_mot[mot], fade, 0); }
static int playing(const lu_traversal *t, int mot) { return t->ag.cur == t->st_mot[mot]; }

void lu_tr_init(lu_traversal *t, const lu_skeleton *s, const lu_humanoid *h, const lu_motions *m, v3 pos, float yaw) {
    memset(t, 0, sizeof *t); lu_tr_default_params(&t->P);
    t->skel = s; t->hum = h; t->mot = m; lu_cc_init(&t->cc, pos); t->yaw = yaw;
    float k = s->tscale > 0 ? s->tscale : 1; t->cc.height = 1.8f * k; t->cc.radius = .3f;
    lu_ag_init(&t->ag, s);
    for (int i = 0; i < LU_MOT_N; i++) t->st_mot[i] = lu_ag_state_clip(&t->ag, lu_motion_names[i], &m->clip[i], 1);
    const lu_clip *loco[4] = {&m->clip[LU_MOT_IDLE], &m->clip[LU_MOT_WALK], &m->clip[LU_MOT_RUN], &m->clip[LU_MOT_SPRINT]};
    float at[4] = {0, lu_gait_speed[1] * k, lu_gait_speed[2] * k, lu_gait_speed[3] * k};
    t->st_move = lu_ag_state_blend(&t->ag, "move", 0, loco, at, 4, 1);
    const lu_clip *cr[2] = {&m->clip[LU_MOT_CROUCH_IDLE], &m->clip[LU_MOT_CROUCH_WALK]}; float cat[2] = {0, 1.6f * k};
    t->st_crouch = lu_ag_state_blend(&t->ag, "crouch", 0, cr, cat, 2, 1);
    lu_ag_play(&t->ag, t->st_move, 0, 0); t->ik_on = 1; t->foot_ik_w = 1; t->ignore_top = -1e9f;
    set_state(t, LU_TR_GROUND); t->cc.grounded = 1;
}
void lu_tr_teleport(lu_traversal *t, v3 pos, float yaw) { t->cc.pos = pos; t->air_top = pos.y; t->cc.vel = V3(0, 0, 0); t->yaw = yaw; set_state(t, LU_TR_GROUND); lu_ag_play(&t->ag, t->st_move, 0, 0); t->cc.grounded = 1; }
m4 lu_tr_world(const lu_traversal *t) { return m4_trs(t->cc.pos, quat_axis(V3(0, 1, 0), t->yaw), V3(1, 1, 1)); }
static v3 fwd(float yaw) { return V3(sinf(yaw), 0, cosf(yaw)); }
static float yaw_of(v3 d) { return atan2f(d.x, d.z); }
static void turn_to(lu_traversal *t, float target, float rate, float dt) { float d = lu_wrap_pi(target - t->yaw); float mx = rate * dt; t->yaw += d > mx ? mx : d < -mx ? -mx : d; }

/* повиснуть на уступе: корень так, чтобы руки легли на кромку */
static void attach_ledge(lu_traversal *t, const lu_ledge *l) {
    float k = t->skel->tscale > 0 ? t->skel->tscale : 1;
    t->ledge = *l; t->yaw = yaw_of(v3_neg(l->wall_n));
    v3 p = v3_add(l->edge, v3_scale(l->wall_n, WALL_Z * k)); p.y = l->top_y - LEDGE_Y * k;
    t->cc.pos = p; t->cc.vel = V3(0, 0, 0); t->cc.grounded = 0; t->hang_pos = p;
}
static int wall_below(const lu_traversal *t, const lu_phys_world *w) {
    v3 n; return lu_probe_wall(w, t->cc.pos, fwd(t->yaw), .6f, .8f, &n, NULL) >= 0;
}
static void enter_hang(lu_traversal *t, const lu_phys_world *w, const lu_ledge *l, float fade) {
    attach_ledge(t, l); set_state(t, LU_TR_HANG); play(t, wall_below(t, w) ? LU_MOT_BRACE : LU_MOT_HANG, fade);
}
static void enter_air(lu_traversal *t, int mot, float fade) { set_state(t, LU_TR_AIR); t->cc.grounded = 0; t->air_top = t->cc.pos.y; play(t, mot, fade); }
/* движение корня клипа → мир */
static v3 root_world(const lu_traversal *t) { return quat_rot(quat_axis(V3(0, 1, 0), t->yaw), t->ag.root_motion); }

static int start_vault(lu_traversal *t, const lu_phys_world *w, float reach) {
    lu_cc *c = &t->cc; float oh, dep; v3 f = fwd(t->yaw);
    if (!lu_find_vault(w, c->pos, f, reach, &oh, &dep)) return 0;
    lu_ledge L; if (!lu_find_ledge(w, c->pos, f, .35f, 1.35f, reach, &L)) return 0;
    v3 into = v3_neg(L.wall_n); t->yaw = yaw_of(into);
    t->act_from = c->pos; t->act_to = v3_add(V3(L.edge.x, c->pos.y, L.edge.z), v3_scale(into, dep + .75f));
    float fy = lu_floor_at(w, t->act_to, 1.f, 2.f, NULL, NULL); if (fy > -1e8f) t->act_to.y = fy;
    t->act_h = L.top_y + .25f;
    set_state(t, LU_TR_VAULT); play(t, LU_MOT_VAULT, .08f); return 1;
}
static void ground_update(lu_traversal *t, const lu_phys_world *w, const lu_tr_input *in, float dt) {
    lu_cc *c = &t->cc; float k = t->skel->tscale > 0 ? t->skel->tscale : 1;
    float mag = fminf(1.f, v3_len(in->move)); v3 dir = mag > .05f ? v3_scale(in->move, 1.f / v3_len(in->move)) : fwd(t->yaw);
    float target = in->crouch ? 1.6f * k * mag : in->sprint ? t->P.sprint * k * mag : (mag < .55f ? t->P.walk * k * mag / .55f : lu_lerpf(t->P.walk, t->P.run, (mag - .55f) / .45f) * k);
    v3 want = v3_scale(dir, mag > .05f ? target : 0);
    float acc = in->sprint ? 7.f : 11.f;
    c->vel.x = lu_damp(c->vel.x, want.x, acc, dt); c->vel.z = lu_damp(c->vel.z, want.z, acc, dt);
    if (mag > .05f) turn_to(t, yaw_of(dir), in->sprint ? 7.f : 11.f, dt);
    float hs = v3_hlen(c->vel);
    /* резкий разворот */
    if (in->turn && hs > 2.f) { set_state(t, LU_TR_TURN); play(t, LU_MOT_TURN180, .08f); c->vel = v3_scale(c->vel, .3f); return; }
    /* прыжок / паркур по кнопке */
    if (in->jump) {
        lu_ledge L;
        if (t->P.auto_vault && start_vault(t, w, .9f + hs * .12f)) return;
        if (lu_find_ledge(w, c->pos, fwd(t->yaw), 1.3f * k, 2.45f * k, .9f, &L)) { enter_hang(t, w, &L, .2f); return; }
        if (lu_probe_wall(w, c->pos, fwd(t->yaw), 1.2f, .8f, NULL, NULL) >= 0) { set_state(t, LU_TR_CLIMB); play(t, LU_MOT_CLIMB, .2f); t->cc.vel = V3(0, 0, 0); return; }
        c->vel.y = hs > t->P.run * k * .9f ? t->P.leap_v : t->P.jump_v; set_state(t, LU_TR_JUMP); t->air_top = c->pos.y; play(t, hs > 5.5f ? LU_MOT_LEAP_AIR : LU_MOT_JUMP_UP, .08f); c->grounded = 0; return;
    }
    /* свободный бег: автоматические действия */
    if (in->sprint && mag > .3f) {
        v3 f = fwd(t->yaw);
        float wd = lu_probe_wall(w, c->pos, f, .5f, .55f + hs * .1f, NULL, NULL);
        if (wd >= 0) {
            lu_ledge L;
            if (t->P.auto_vault && start_vault(t, w, 1.2f)) return;
            if (t->P.auto_climb && lu_find_ledge(w, c->pos, f, 1.3f * k, 2.45f * k, 1.f, &L)) { enter_hang(t, w, &L, .15f); return; }
            if (t->P.auto_climb && wd < .45f) { set_state(t, LU_TR_WALLRUN); play(t, LU_MOT_WALLRUN, .12f); c->vel = V3(0, 5.2f, 0); return; }
        }
        /* край крыши впереди → дальний прыжок */
        if (t->P.auto_leap && hs > 3.f) {
            v3 ahead = v3_add(c->pos, v3_scale(f, .55f)); float fy = lu_floor_at(w, ahead, .5f, 3.f, NULL, NULL);
            if (fy < c->pos.y - 1.2f) { c->vel.y = t->P.leap_v; c->vel.x = f.x * fmaxf(hs, t->P.sprint * k * .95f); c->vel.z = f.z * fmaxf(hs, t->P.sprint * k * .95f);
                set_state(t, LU_TR_JUMP); t->air_top = c->pos.y; play(t, LU_MOT_LEAP_AIR, .1f); c->grounded = 0; return; }
        }
    }
    /* спуститься с края в вис (присед + движение к краю) */
    if (in->crouch && mag > .3f) {
        v3 f = dir, ahead = v3_add(c->pos, v3_scale(f, .5f)); float fy = lu_floor_at(w, ahead, .5f, 3.f, NULL, NULL);
        if (fy < c->pos.y - 1.6f) {
            lu_ledge L; v3 feet = v3_add(c->pos, v3_scale(f, 1.1f)); feet.y -= LEDGE_Y * k;
            if (lu_find_ledge(w, feet, v3_neg(f), 1.6f * k, 2.5f * k, 1.4f, &L)) {
                t->ledge = L; t->yaw = yaw_of(v3_neg(L.wall_n)); set_state(t, LU_TR_DROP); play(t, LU_MOT_DROP_HANG, .12f);
                t->anim_root_start = c->pos; return;
            }
        }
    }
    c->vel.y -= t->P.gravity * dt;
    if (c->grounded && c->vel.y < 0) c->vel.y = -1.f;
    lu_cc_move(c, w, dt, 1);
    if (!c->grounded) { enter_air(t, LU_MOT_FALL, .25f); return; }
    /* анимация */
    t->ag.param[0] = hs;
    int want_st = in->crouch ? t->st_crouch : t->st_move;
    if (t->ag.cur != want_st) lu_ag_play(&t->ag, want_st, .22f, 0);
    float side = lu_wrap_pi(yaw_of(dir) - t->yaw) * (mag > .05f ? 1.f : 0.f); t->lean = lu_damp(t->lean, lu_clampf(side * hs * .05f, -.35f, .35f), 6, dt);
}
static void air_update(lu_traversal *t, const lu_phys_world *w, const lu_tr_input *in, float dt) {
    lu_cc *c = &t->cc; float k = t->skel->tscale > 0 ? t->skel->tscale : 1;
    float mag = fminf(1.f, v3_len(in->move));
    if (mag > .05f) { v3 d = v3_scale(in->move, 1.f / v3_len(in->move)); float hs = v3_hlen(c->vel);
        c->vel.x += d.x * 3.f * dt; c->vel.z += d.z * 3.f * dt; float nh = v3_hlen(c->vel); if (nh > fmaxf(hs, 2.f)) { c->vel.x *= fmaxf(hs, 2.f) / nh; c->vel.z *= fmaxf(hs, 2.f) / nh; } }
    if (v3_hlen(c->vel) > .5f) turn_to(t, yaw_of(c->vel), 4.f, dt);
    c->vel.y -= t->P.gravity * dt;
    lu_cc_move(c, w, dt, 0);
    if (c->pos.y > t->air_top) t->air_top = c->pos.y;
    /* хватка за уступ */
    if (t->P.ledge_grab && c->vel.y < 1.5f && t->st_t > .12f && !in->drop) {
        lu_ledge L; v3 hands = v3_add(c->pos, V3(0, LEDGE_Y * k, 0));
        if (lu_find_grab(w, hands, fwd(t->yaw), .75f, &L)) { enter_hang(t, w, &L, .12f); return; }
        if (c->hit_wall && in->sprint && c->vel.y < 0 && v3_dot(c->wall_n, fwd(t->yaw)) < -.6f) { t->yaw = yaw_of(v3_neg(c->wall_n)); set_state(t, LU_TR_CLIMB); play(t, LU_MOT_CLIMB, .15f); c->vel = V3(0, 0, 0); return; }
    }
    if (t->state == LU_TR_JUMP && t->st_t > .25f) { set_state(t, LU_TR_AIR); if (!playing(t, LU_MOT_LEAP_AIR)) play(t, LU_MOT_JUMP_AIR, .15f); }
    if (c->vel.y < -6.f && !playing(t, LU_MOT_FALL)) play(t, LU_MOT_FALL, .35f);
    if (c->grounded) {
        float fall = t->air_top - c->pos.y; t->fall_h = fall; int soft = (c->ground_flags & LU_SURF_SOFT) != 0;
        float hs = v3_hlen(c->vel);
        if (!soft && fall > t->P.hurt_height && !(t->P.roll && hs > 1.5f)) t->damage += (fall - t->P.hurt_height) * 12.f;
        if (soft) { set_state(t, LU_TR_LAND); play(t, LU_MOT_LAND_SOFT, .08f); c->vel = V3(0, 0, 0); }
        else if (fall > t->P.roll_height && t->P.roll && hs > 1.5f) { set_state(t, LU_TR_ROLL); play(t, LU_MOT_ROLL, .06f); if (fall > t->P.hurt_height * 1.6f) t->damage += (fall - t->P.hurt_height * 1.6f) * 10.f; }
        else if (fall > 2.2f) { set_state(t, LU_TR_LAND); play(t, LU_MOT_LAND_HARD, .06f); c->vel = v3_scale(c->vel, .2f); }
        else if (fall > .6f) { set_state(t, LU_TR_LAND); play(t, LU_MOT_LAND_SOFT, .08f); }
        else { set_state(t, LU_TR_GROUND); lu_ag_play(&t->ag, t->st_move, .2f, 0); }
    }
}
static void climb_update(lu_traversal *t, const lu_phys_world *w, const lu_tr_input *in, float dt) {
    lu_cc *c = &t->cc; float k = t->skel->tscale > 0 ? t->skel->tscale : 1;
    v3 f = fwd(t->yaw), n; int fl = 0;
    float wd = lu_probe_wall(w, c->pos, f, 1.2f * k, 1.2f, &n, &fl);
    if (wd < 0 || (fl & LU_SURF_NOCLIMB)) { enter_air(t, LU_MOT_FALL, .2f); return; }
    t->yaw = yaw_of(v3_neg(n)); f = fwd(t->yaw);
    v3 side_dir = v3_cross(V3(0, 1, 0), n);  /* вдоль стены */
    float up = v3_dot(in->move, f), sd = v3_dot(in->move, side_dir);
    /* прижаться к стене */
    v3 at = v3_add(c->pos, v3_scale(f, wd - WALL_Z * k)); c->pos.x = at.x; c->pos.z = at.z;
    float vy = up * t->P.climb_speed * k * (in->sprint ? 1.6f : 1.f), vs = sd * t->P.shimmy_speed * k;
    v3 np = v3_add(c->pos, V3(side_dir.x * vs * dt, vy * dt, side_dir.z * vs * dt));
    /* тело отодвинуто от стены: выступы до ~0.35 м (карнизы, подоконники) не мешают лезть */
    v3 bc = v3_sub(np, v3_scale(f, .2f));
    if (lu_probe_wall(w, np, f, 1.2f * k, 1.2f, NULL, NULL) >= 0 && !lu_capsule_overlap(w, V3(bc.x, bc.y + .4f, bc.z), V3(bc.x, bc.y + 1.45f * k, bc.z), .17f)) c->pos = np;
    else if (vy > 0) { /* наверху стена кончилась без уступа — пробуем подтянуться на крышу */ }
    float moving = fminf(1.f, fabsf(up) + fabsf(sd)); t->ag.st[t->st_mot[LU_MOT_CLIMB]].speed = moving > .1f ? moving * (in->sprint ? 1.6f : 1.f) : 0.0f;
    /* кромка над головой → вис */
    lu_ledge L;
    if (t->ignore_top > -1e8f && c->pos.y + 2.05f * k > t->ignore_top + 1.2f) t->ignore_top = -1e9f;
    if (up > .1f && lu_find_ledge(w, c->pos, f, 1.7f * k, 2.4f * k, 1.f, &L) && L.top_y > t->ignore_top + .3f) { enter_hang(t, w, &L, .2f); return; }
    if (up < -.1f) { float fy = lu_floor_at(w, c->pos, .3f, .5f, NULL, NULL); if (c->pos.y - fy < .25f) { c->pos.y = fy; set_state(t, LU_TR_GROUND); lu_ag_play(&t->ag, t->st_move, .25f, 0); c->grounded = 1; return; } }
    if (in->drop) { c->vel = v3_scale(n, 1.f); enter_air(t, LU_MOT_FALL, .2f); return; }
    if (in->jump) { /* отскок от стены назад */ t->yaw += LU_PI; c->vel = v3_add(v3_scale(n, 4.5f), V3(0, 5.f, 0)); set_state(t, LU_TR_JUMP); t->air_top = c->pos.y; play(t, LU_MOT_JUMP_AIR, .1f); return; }
}
static void hang_update(lu_traversal *t, const lu_phys_world *w, const lu_tr_input *in, float dt) {
    lu_cc *c = &t->cc; float k = t->skel->tscale > 0 ? t->skel->tscale : 1;
    v3 f = fwd(t->yaw), side_dir = t->ledge.edge_dir;
    float up = v3_dot(in->move, f), sd = v3_dot(in->move, side_dir);
    int braced = wall_below(t, w);
    if (t->st_t < .15f) return;
    if ((up > .5f || (in->sprint && up > .1f) || in->jump) && t->ledge.can_stand) { set_state(t, LU_TR_CLIMB_UP); play(t, LU_MOT_CLIMB_UP, .1f); t->anim_root_start = c->pos; return; }
    if ((in->jump || up > .5f) && !t->ledge.can_stand) {
        /* выше — уступ (скат мансарды, следующий карниз)? перескочить к нему */
        lu_ledge L2; v3 into = v3_neg(t->ledge.wall_n);
        v3 feet_top = v3_add(V3(t->ledge.edge.x, t->ledge.top_y, t->ledge.edge.z), v3_scale(into, -.05f));
        v3 feet_hang = c->pos;
        if (lu_find_ledge(w, feet_top, into, .5f * k, 2.6f * k, 1.3f, &L2) || (in->jump && lu_find_ledge(w, feet_hang, f, 2.6f * k, 4.2f * k, 1.f, &L2))) {
            t->next_ledge = L2; t->act_from = c->pos; t->act_dur = .75f;
            set_state(t, LU_TR_HOP); play(t, LU_MOT_CLIMB, .15f); return;
        }
        v3 wn2; if (lu_probe_wall(w, c->pos, f, 2.6f * k, 1.f, &wn2, NULL) >= 0) { t->ignore_top = t->ledge.top_y; set_state(t, LU_TR_CLIMB); play(t, LU_MOT_CLIMB, .2f); return; }
    }
    if (in->drop || (up < -.6f && !braced)) { c->vel = v3_scale(t->ledge.wall_n, .8f); enter_air(t, LU_MOT_FALL, .2f); t->st_t = -.3f; return; }
    if (up < -.6f && braced) { set_state(t, LU_TR_CLIMB); play(t, LU_MOT_CLIMB, .25f); return; }
    if (fabsf(sd) > .3f) {
        float s = sd > 0 ? 1.f : -1.f; lu_ledge L2;
        if (lu_ledge_continues(w, &t->ledge, s, .35f, &L2)) {
            v3 p = v3_add(c->pos, v3_scale(side_dir, s * t->P.shimmy_speed * k * dt));
            lu_ledge L3; v3 feet = v3_add(V3(p.x, t->ledge.top_y - t->ledge.height, p.z), V3(0, 0, 0)); LU_UNUSED(feet);
            if (lu_find_ledge(w, V3(p.x, p.y, p.z), f, 1.8f * k, 2.35f * k, 1.f, &L3)) attach_ledge(t, &L3); else c->pos = p;
            int m = s > 0 ? LU_MOT_SHIMMY_R : LU_MOT_SHIMMY_L;
            /* правая сторона персонажа — -X модели; edge_dir смотрит вправо от взгляда на стену */
            if (!playing(t, m)) play(t, m, .15f);
            return;
        }
    }
    int idle = braced ? LU_MOT_BRACE : LU_MOT_HANG; if (!playing(t, idle)) play(t, idle, .2f);
}
static void rootmotion_update(lu_traversal *t, const lu_phys_world *w, int done_state, float dt) {
    lu_cc *c = &t->cc; LU_UNUSED(dt);
    c->pos = v3_add(c->pos, root_world(t));
    if (lu_ag_norm_time(&t->ag) >= .999f) {
        if (done_state == LU_TR_GROUND) {
            v3 a = V3(c->pos.x, c->pos.y + c->radius + .02f, c->pos.z), b = V3(c->pos.x, c->pos.y + c->height - c->radius, c->pos.z);
            c->pos = v3_add(c->pos, lu_capsule_resolve(w, a, b, c->radius, NULL, 4));
            float fy = lu_floor_at(w, c->pos, .6f, 1.f, NULL, NULL); if (fy > -1e8f && c->pos.y - fy < .6f) c->pos.y = fy;
            set_state(t, LU_TR_GROUND); lu_ag_play(&t->ag, t->st_move, .2f, 0); c->grounded = 1; c->vel = V3(0, 0, 0);
            if (t->state == LU_TR_GROUND && t->ag.param[0] > 0) {}
        } else if (done_state == LU_TR_HANG) { enter_hang(t, w, &t->ledge, 0); }
    }
}
void lu_tr_update(lu_traversal *t, const lu_phys_world *w, const lu_tr_input *in, float dt) {
    t->damage = 0; t->st_t += dt;
    lu_cc *c = &t->cc; float k = t->skel->tscale > 0 ? t->skel->tscale : 1;
    switch (t->state) {
    case LU_TR_GROUND: ground_update(t, w, in, dt); break;
    case LU_TR_JUMP: case LU_TR_AIR: air_update(t, w, in, dt); break;
    case LU_TR_LAND: {
        c->vel.x = lu_damp(c->vel.x, 0, 6, dt); c->vel.z = lu_damp(c->vel.z, 0, 6, dt); c->vel.y -= t->P.gravity * dt; lu_cc_move(c, w, dt, 1);
        float d = t->mot->clip[playing(t, LU_MOT_LAND_HARD) ? LU_MOT_LAND_HARD : LU_MOT_LAND_SOFT].duration;
        if (t->st_t > d * (v3_len(in->move) > .2f ? .55f : .95f)) { set_state(t, LU_TR_GROUND); lu_ag_play(&t->ag, t->st_move, .2f, 0); }
        break; }
    case LU_TR_ROLL: {
        v3 rm = root_world(t); c->vel.x = rm.x / fmaxf(dt, 1e-4f); c->vel.z = rm.z / fmaxf(dt, 1e-4f); c->vel.y -= t->P.gravity * dt; lu_cc_move(c, w, dt, 1);
        if (lu_ag_norm_time(&t->ag) > .97f) { set_state(t, LU_TR_GROUND); lu_ag_play(&t->ag, t->st_move, .15f, 0); t->ag.param[0] = t->P.run * k; c->vel = v3_scale(fwd(t->yaw), t->P.run * k * .8f); }
        break; }
    case LU_TR_CLIMB: climb_update(t, w, in, dt); break;
    case LU_TR_HANG: hang_update(t, w, in, dt); break;
    case LU_TR_CLIMB_UP: rootmotion_update(t, w, LU_TR_GROUND, dt); break;
    case LU_TR_VAULT: {
        float u = lu_clampf(t->st_t / t->mot->clip[LU_MOT_VAULT].duration, 0, 1), e = u * u * (3 - 2 * u);
        v3 p = v3_lerp(t->act_from, t->act_to, e); float arc = sinf(LU_PI * u); p.y = lu_lerpf(t->act_from.y, t->act_to.y, e) + fmaxf(0.f, (t->act_h - fmaxf(t->act_from.y, t->act_to.y))) * arc;
        c->pos = p;
        if (u >= 1) { set_state(t, LU_TR_GROUND); lu_ag_play(&t->ag, t->st_move, .15f, 0); c->grounded = 1; c->vel = v3_scale(fwd(t->yaw), (in->sprint ? t->P.sprint : t->P.run) * k * .8f); t->ag.param[0] = v3_hlen(c->vel); }
        break; }
    case LU_TR_DROP: rootmotion_update(t, w, LU_TR_HANG, dt); break;
    case LU_TR_WALLRUN: {
        c->vel.y -= 9.f * dt; c->pos.y += c->vel.y * dt;
        lu_ledge L; if (lu_find_ledge(w, c->pos, fwd(t->yaw), 1.6f * k, 2.4f * k, .9f, &L)) { enter_hang(t, w, &L, .12f); break; }
        if (c->vel.y < 1.f || t->st_t > .75f) { if (lu_probe_wall(w, c->pos, fwd(t->yaw), 1.2f * k, .9f, NULL, NULL) >= 0) { set_state(t, LU_TR_CLIMB); play(t, LU_MOT_CLIMB, .2f); c->vel = V3(0, 0, 0); } else enter_air(t, LU_MOT_FALL, .2f); }
        break; }
    case LU_TR_HOP: {
        /* кинематический перескок к следующему уступу; конечная точка — вис на нём */
        float u = lu_clampf(t->st_t / t->act_dur, 0, 1), e = u * u * (3 - 2 * u);
        lu_traversal tmp = *t; attach_ledge(&tmp, &t->next_ledge); v3 to = tmp.cc.pos;
        v3 p = v3_lerp(t->act_from, to, e); p.y += .25f * sinf(LU_PI * u); c->pos = p; t->yaw = tmp.yaw;
        t->ag.st[t->st_mot[LU_MOT_CLIMB]].speed = 1.4f;
        if (u >= 1) enter_hang(t, w, &t->next_ledge, .15f);
        break; }
    case LU_TR_TURN:
        c->vel.x = lu_damp(c->vel.x, 0, 8, dt); c->vel.z = lu_damp(c->vel.z, 0, 8, dt); lu_cc_move(c, w, dt, 1);
        if (lu_ag_norm_time(&t->ag) > .98f) { t->yaw += LU_PI; set_state(t, LU_TR_GROUND); lu_ag_play(&t->ag, t->st_move, 0, 0); }
        break;
    }
    /* анимация и IK */
    lu_ag_update(&t->ag, dt);
    t->nevents = t->ag.nevents; memcpy(t->events, t->ag.events, sizeof(int) * (size_t)t->nevents);
    t->pose = t->ag.pose;
    const lu_humanoid *h = t->hum;
    /* наклон в повороте на бегу */
    if (t->state == LU_TR_GROUND && fabsf(t->lean) > .01f) { xform *hx = &t->pose.l[h->hips]; hx->r = quat_norm(quat_mul(quat_axis(V3(0, 0, 1), -t->lean), hx->r)); }
    /* IK стоп на неровной земле */
    int on_ground = t->state == LU_TR_GROUND || t->state == LU_TR_LAND;
    t->foot_ik_w = lu_damp(t->foot_ik_w, on_ground ? 1.f : 0.f, 12, dt);
    if (t->ik_on && t->foot_ik_w > .01f) {
        lu_pose_model(t->skel, &t->pose, t->model);
        m4 W = lu_tr_world(t); float off[2]; v3 fm[2];
        for (int i = 0; i < 2; i++) {
            fm[i] = t->model[h->foot[i]].t; v3 wp = m4_point(W, fm[i]);
            float gy = lu_floor_at(w, wp, .5f, 1.f, NULL, NULL);
            off[i] = gy > -1e8f ? lu_clampf(gy - c->pos.y, -.45f, .45f) : 0;
        }
        float pel = fminf(0.f, fminf(off[0], off[1]));
        t->pelvis_off = lu_damp(t->pelvis_off, pel, 14, dt);
        t->pose.l[h->hips].t.y += t->pelvis_off * t->foot_ik_w;
        lu_pose_model(t->skel, &t->pose, t->model);
        for (int i = 0; i < 2; i++) {
            float lift = fm[i].y - h->foot_h * k; /* насколько стопа поднята в анимации */
            if (fabsf(off[i]) < .02f) continue;
            v3 tgt = fm[i]; tgt.y = h->foot_h * k + off[i] + fmaxf(0.f, lift);
            v3 knee = t->model[h->shin[i]].t; v3 pole = v3_add(knee, v3_scale(fwd(0), .5f));
            lu_ik_two_bone(t->skel, &t->pose, h->thigh[i], h->shin[i], h->foot[i], tgt, pole, t->foot_ik_w);
        }
    }
    if (t->look_w > .01f) {
        m4 Wi = m4_inverse(lu_tr_world(t)); v3 tl = m4_point(Wi, t->look_target);
        lu_ik_look(t->skel, &t->pose, h->head, V3(0, 0, 1), tl, t->look_w, 1.2f);
    }
    lu_pose_model(t->skel, &t->pose, t->model);
    t->surface = c->ground_flags;
}

/* ================= камера ================= */
void lu_tpcam_init(lu_tp_camera *c, float yaw) { memset(c, 0, sizeof *c); c->yaw = yaw; c->pitch = .25f; c->dist = 4.2f; c->cur_dist = 4.2f; c->fov = 1.0f; c->min_pitch = -.6f; c->max_pitch = 1.3f; }
void lu_tpcam_update(lu_tp_camera *c, const lu_phys_world *w, v3 focus, float dx, float dy, float dt) {
    c->yaw += dx; c->pitch = lu_clampf(c->pitch + dy, c->min_pitch, c->max_pitch);
    if (v3_len2(c->smooth_target) == 0) c->smooth_target = focus;
    c->smooth_target.x = lu_damp(c->smooth_target.x, focus.x, 14, dt); c->smooth_target.z = lu_damp(c->smooth_target.z, focus.z, 14, dt);
    c->smooth_target.y = lu_damp(c->smooth_target.y, focus.y, 8, dt);
    c->target = c->smooth_target;
    v3 back = V3(-sinf(c->yaw) * cosf(c->pitch), sinf(c->pitch), -cosf(c->yaw) * cosf(c->pitch));
    float want = c->dist; lu_hit h;
    if (lu_spherecast(w, c->target, back, .22f, c->dist, &h)) want = fmaxf(.4f, h.t - .05f);
    c->cur_dist = want < c->cur_dist ? want : lu_damp(c->cur_dist, want, 4, dt);
    c->eye = v3_mad(c->target, back, c->cur_dist);
}
