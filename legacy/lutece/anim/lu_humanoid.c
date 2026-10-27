/* Lutèce Engine — гуманоид: скелет, решатель поз, процедурная библиотека движений */
#include "lu_humanoid.h"
#include <stdio.h>

const char *lu_motion_names[LU_MOT_N] = {
    "idle", "walk", "run", "sprint", "crouch_idle", "crouch_walk", "jump_up", "jump_air", "leap_air", "fall", "land_soft", "land_hard", "roll",
    "turn180", "climb", "hang", "shimmy_l", "shimmy_r", "climb_up", "vault", "wallrun", "drop_hang", "brace", "strafe_l", "strafe_r",
    "atk1", "atk2", "atk3", "block", "counter", "hit", "dodge", "guard_idle", "talk", "look_around", "sit", "cheer"};
/* длина цикла (два шага) / длительность = скорость */
const float lu_gait_speed[4] = {0.f, 1.45f, 4.6f, 7.4f};

static xform XT(float x, float y, float z) { xform t = XF_ID(); t.t = V3(x, y, z); return t; }
static quat RX(float a) { return quat_axis(V3(1, 0, 0), a); }
static quat RY(float a) { return quat_axis(V3(0, 1, 0), a); }
static quat RZ(float a) { return quat_axis(V3(0, 0, 1), a); }
static quat Q3(quat a, quat b, quat c) { return quat_norm(quat_mul(a, quat_mul(b, c))); }

void lu_humanoid_skeleton(lu_skeleton *s, lu_humanoid *h, float H, int female) {
    memset(s, 0, sizeof *s); memset(h, 0, sizeof *h);
    float k = H / 1.78f, hw = female ? 1.08f : 1.f, sw = female ? .9f : 1.f;
    h->height = H;
    h->root = lu_skel_add(s, "Root", -1, XF_ID());
    h->hips = lu_skel_add(s, "Hips", h->root, XT(0, .98f * k, 0));
    h->spine = lu_skel_add(s, "Spine", h->hips, XT(0, .10f * k, -.01f * k));
    h->spine1 = lu_skel_add(s, "Spine1", h->spine, XT(0, .13f * k, 0));
    h->spine2 = lu_skel_add(s, "Spine2", h->spine1, XT(0, .13f * k, .01f * k));
    h->neck = lu_skel_add(s, "Neck", h->spine2, XT(0, .16f * k, 0));
    h->head = lu_skel_add(s, "Head", h->neck, XT(0, .09f * k, .01f * k));
    static const char *side[2] = {"Left", "Right"};
    char nm[32];
    for (int i = 0; i < 2; i++) {
        float sg = i == 0 ? 1.f : -1.f;
        snprintf(nm, sizeof nm, "%sShoulder", side[i]); h->clav[i] = lu_skel_add(s, nm, h->spine2, XT(sg * .025f * k, .10f * k, .02f * k));
        snprintf(nm, sizeof nm, "%sArm", side[i]); h->arm[i] = lu_skel_add(s, nm, h->clav[i], XT(sg * .155f * k * sw, -.01f * k, -.02f * k));
        snprintf(nm, sizeof nm, "%sForeArm", side[i]); h->fore[i] = lu_skel_add(s, nm, h->arm[i], XT(sg * .29f * k, 0, 0));
        snprintf(nm, sizeof nm, "%sHand", side[i]); h->hand[i] = lu_skel_add(s, nm, h->fore[i], XT(sg * .255f * k, 0, 0));
        snprintf(nm, sizeof nm, "%sHandFingers", side[i]); h->fingers[i] = lu_skel_add(s, nm, h->hand[i], XT(sg * .09f * k, 0, 0));
        snprintf(nm, sizeof nm, "%sHandThumb", side[i]); h->thumb[i] = lu_skel_add(s, nm, h->hand[i], XT(sg * .025f * k, -.01f * k, .03f * k));
    }
    for (int i = 0; i < 2; i++) {
        float sg = i == 0 ? 1.f : -1.f;
        snprintf(nm, sizeof nm, "%sUpLeg", side[i]); h->thigh[i] = lu_skel_add(s, nm, h->hips, XT(sg * .095f * k * hw, -.06f * k, 0));
        snprintf(nm, sizeof nm, "%sLeg", side[i]); h->shin[i] = lu_skel_add(s, nm, h->thigh[i], XT(0, -.44f * k, 0));
        snprintf(nm, sizeof nm, "%sFoot", side[i]); h->foot[i] = lu_skel_add(s, nm, h->shin[i], XT(0, -.405f * k, -.01f * k));
        snprintf(nm, sizeof nm, "%sToeBase", side[i]); h->toe[i] = lu_skel_add(s, nm, h->foot[i], XT(0, -.055f * k, .135f * k));
    }
    lu_skel_finalize(s); s->tscale = 1.f;
    h->leg_len = (.44f + .405f) * k; h->arm_len = (.29f + .255f) * k; h->hip_h = .92f * k; h->foot_h = .075f * k;
    h->shoulder_h = (.98f + .10f + .13f + .13f + .10f - .01f) * k;
}

void lu_hpose_rest(const lu_humanoid *h, lu_hpose *p) {
    memset(p, 0, sizeof *p);
    float k = h->height / 1.78f;
    for (int i = 0; i < 2; i++) {
        float sg = i == 0 ? 1.f : -1.f;
        p->foot[i] = V3(sg * .1f * k, h->foot_h, 0); p->knee_pole[i] = V3(sg * .12f * k, .5f * k, 1.5f);
        p->arm_out[i] = 1.32f; p->elbow[i] = .18f; p->arm_swing[i] = 0; p->finger_curl[i] = .35f;
        p->elbow_pole[i] = V3(sg * .6f, 1.f * k, -1.f);
    }
}

void lu_hpose_solve(const lu_skeleton *s, const lu_humanoid *h, const lu_hpose *hp, lu_pose *out) {
    lu_pose_bind(s, out);
    xform rootx = XF_ID(); rootx.t = hp->root; rootx.r = RY(hp->root_yaw);
    out->l[h->root] = rootx;
    xform *hx = &out->l[h->hips];
    hx->t = v3_add(hx->t, hp->pelvis); hx->r = Q3(RY(hp->pelvis_yaw), RX(hp->pelvis_pitch), RZ(hp->pelvis_roll));
    int sp[3] = {h->spine, h->spine1, h->spine2};
    for (int i = 0; i < 3; i++) out->l[sp[i]].r = Q3(RY(hp->spine_yaw / 3), RX(hp->spine_pitch / 3), RZ(hp->spine_roll / 3));
    out->l[h->neck].r = RX(hp->neck_pitch);
    out->l[h->head].r = Q3(RY(hp->head_yaw), RX(hp->head_pitch), RZ(hp->head_roll));
    for (int i = 0; i < 2; i++) {
        float sg = i == 0 ? 1.f : -1.f;
        out->l[h->clav[i]].r = quat_norm(quat_mul(RZ(sg * hp->clav_up[i]), RY(-sg * hp->clav_fwd[i])));
        /* рука: опустить из Т-позы, качнуть вперёд/назад, скрутить вдоль кости */
        out->l[h->arm[i]].r = Q3(RX(-hp->arm_swing[i]), RZ(-sg * hp->arm_out[i]), RX(sg * hp->arm_twist[i]));
        out->l[h->fore[i]].r = RY(-sg * hp->elbow[i]);
        out->l[h->hand[i]].r = quat_norm(quat_mul(RZ(-sg * hp->hand_pitch[i]), RX(hp->hand_roll[i])));
        out->l[h->fingers[i]].r = RZ(-sg * hp->finger_curl[i]);
        out->l[h->thumb[i]].r = RY(-sg * hp->finger_curl[i] * .6f);
    }
    m4 R = m4_from_xform(rootx);
    /* ноги — IK к цели лодыжки, затем ориентация стопы */
    for (int i = 0; i < 2; i++) {
        v3 tgt = m4_point(R, hp->foot[i]), pole = m4_point(R, hp->knee_pole[i]);
        lu_ik_two_bone(s, out, h->thigh[i], h->shin[i], h->foot[i], tgt, pole, 1);
        quat fr = quat_norm(quat_mul(rootx.r, quat_mul(RY(hp->foot_yaw[i]), RX(hp->foot_pitch[i]))));
        lu_ik_set_model_rot(s, out, h->foot[i], fr, 1);
        out->l[h->toe[i]].r = RX(-hp->toe_bend[i]);
    }
    /* руки по целям */
    for (int i = 0; i < 2; i++) if (hp->hand_ik[i]) {
        v3 tgt = m4_point(R, hp->hand[i]), pole = m4_point(R, hp->elbow_pole[i]);
        lu_ik_two_bone(s, out, h->arm[i], h->fore[i], h->hand[i], tgt, pole, 1);
    }
}

/* ===================== процедурные движения ===================== */
typedef void (*pose_fn)(float u, const lu_humanoid *h, lu_hpose *p, const void *par);
static void clip_from(lu_clip *c, const char *name, const lu_skeleton *s, const lu_humanoid *h, float dur, int loop, int keys, pose_fn fn, const void *par, int root_motion) {
    lu_clip_init(c, name, dur, loop);
    lu_pose pose; lu_hpose hp;
    for (int k = 0; k <= keys; k++) {
        float u = (float)k / keys; if (loop && k == keys) u = 0;  /* бесшовный цикл */
        lu_hpose_rest(h, &hp); fn(loop && k == keys ? 1.f : u, h, &hp, par);
        if (loop && k == keys) { lu_hpose tmp; lu_hpose_rest(h, &tmp); fn(0, h, &tmp, par); hp = tmp; }
        lu_hpose_solve(s, h, &hp, &pose);
        float t = dur * (float)k / keys;
        for (int j = 0; j < s->n; j++) lu_clip_key_r(c, j, t, pose.l[j].r);
        lu_clip_key_t(c, h->hips, t, pose.l[h->hips].t);
        if (root_motion) lu_clip_key_t(c, h->root, t, pose.l[h->root].t);
    }
    if (root_motion) { c->root_joint = h->root; c->root_motion = root_motion; }
}
static float ease(float t) { t = lu_clampf(t, 0, 1); return t * t * (3 - 2 * t); }
static float ease_out(float t) { t = lu_clampf(t, 0, 1); return 1 - (1 - t) * (1 - t); }
static float ease_in(float t) { t = lu_clampf(t, 0, 1); return t * t; }
static float tri(float x) { x = x - floorf(x); return x < .5f ? x * 2 : 2 - x * 2; }
/* кусочно-линейная кривая по ключам (u, v) с плавностью */
static float curve(float u, const float *k, int n) {
    if (u <= k[0]) return k[1];
    for (int i = 0; i + 1 < n; i++) if (u <= k[(i + 1) * 2]) { float a = k[i * 2], b = k[(i + 1) * 2]; return lu_lerpf(k[i * 2 + 1], k[(i + 1) * 2 + 1], ease((u - a) / (b - a))); }
    return k[(n - 1) * 2 + 1];
}

/* ---------- походка ---------- */
typedef struct { float stride, duty, lift, bounce, drop, lean, arm, elbow, yaw, roll, heel, sx, crouch, kick; } Gait;
static void gait(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    const Gait *g = par; float k = h->height / 1.78f;
    for (int i = 0; i < 2; i++) {
        float sg = i == 0 ? 1.f : -1.f, ph = u + (i ? .5f : 0); ph -= floorf(ph);
        float L = g->stride * .5f * k, d = g->duty, z, y = h->foot_h, pitch = 0;
        /* ступня: опора (назад линейно), перенос (дуга вперёд) */
        if (ph < d) { float s = ph / d; z = lu_lerpf(L * d, -L * d, s);
            pitch = s < .15f ? lu_lerpf(-.25f * g->heel, 0, s / .15f) : s > .7f ? lu_lerpf(0, .55f * g->heel + .1f, (s - .7f) / .3f) : 0;
            if (pitch > 0) y += .12f * k * sinf(pitch); }
        else { float s = (ph - d) / (1 - d); z = lu_lerpf(-L * d, L * d, ease(s));
            y += g->lift * k * sinf(LU_PI * powf(s, .8f)) + g->kick * k * sinf(LU_PI * fminf(s * 1.6f, 1.f)) * (1 - s);
            pitch = lu_lerpf(.5f * g->heel + .1f, -.25f * g->heel, ease(s)); }
        p->foot[i] = V3(sg * g->sx * k, y, z); p->foot_pitch[i] = pitch; p->toe_bend[i] = pitch > .1f && ph < d ? pitch * .8f : 0;
        p->knee_pole[i] = V3(sg * .15f, .6f, 2.f);
        /* руки: противофаза с ногами */
        float sw = -g->arm * cosf(LU_TAU * ph);
        p->arm_swing[i] = sw; p->elbow[i] = g->elbow + fmaxf(0, sw) * g->elbow * .6f; p->arm_out[i] = 1.3f - g->elbow * .08f;
        p->finger_curl[i] = .4f + g->elbow * .3f;
    }
    float bob = cosf(LU_TAU * 2 * (u - g->duty * .5f));
    p->pelvis = V3(0, -g->drop * k + g->bounce * k * bob - g->crouch * k, g->lean * .05f);
    p->pelvis_yaw = g->yaw * sinf(LU_TAU * u); p->pelvis_roll = g->roll * cosf(LU_TAU * u);
    p->pelvis_pitch = g->lean * .35f + g->crouch * .4f;
    p->spine_yaw = -p->pelvis_yaw * 1.5f; p->spine_pitch = g->lean * .65f + g->crouch * .5f; p->spine_roll = -p->pelvis_roll * .7f;
    p->head_pitch = -(p->spine_pitch + p->pelvis_pitch) * .7f; p->head_yaw = -p->spine_yaw * .4f - p->pelvis_yaw * .5f;
}
static void idle(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    LU_UNUSED(par); float b = sinf(LU_TAU * u), k = h->height / 1.78f;
    p->pelvis = V3(.012f * sinf(LU_TAU * u * .5f) * k, -.012f * k + .004f * b * k, 0); p->pelvis_roll = .02f * sinf(LU_TAU * u * .5f);
    p->spine_pitch = .02f + .015f * b; p->head_pitch = -.02f - .01f * b; p->head_yaw = .06f * sinf(LU_TAU * u * .5f + 1);
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f; p->foot[i] = V3(sg * .12f * k, h->foot_h, i ? -.03f : .04f); p->foot_yaw[i] = sg * .12f; p->arm_out[i] = 1.33f - .015f * b; p->elbow[i] = .2f + .02f * b; p->clav_up[i] = .02f * b; }
}
static void crouch_idle(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    LU_UNUSED(par); float b = sinf(LU_TAU * u), k = h->height / 1.78f;
    p->pelvis = V3(0, -.38f * k + .006f * b, -.05f); p->pelvis_pitch = .35f; p->spine_pitch = .45f + .02f * b; p->head_pitch = -.55f;
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f; p->foot[i] = V3(sg * .15f * k, h->foot_h, i ? -.12f : .14f); p->foot_yaw[i] = sg * .2f; p->arm_out[i] = 1.2f; p->arm_swing[i] = .5f; p->elbow[i] = .9f; }
}
/* ---------- прыжки, падения, приземления ---------- */
static void jump_up(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    LU_UNUSED(par); float k = h->height / 1.78f; float c = curve(u, (const float[]){0, 0, .35f, 1, 1, -.3f}, 3);
    p->pelvis = V3(0, -.22f * k * c, 0); p->spine_pitch = .25f * c; p->pelvis_pitch = .2f * c; p->head_pitch = -.3f * c;
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f; p->foot[i] = V3(sg * .12f * k, h->foot_h + fmaxf(0, -c) * .25f, 0); p->foot_pitch[i] = fmaxf(0, -c) * 1.2f;
        p->arm_swing[i] = lu_lerpf(-.6f * c, 1.6f, ease((u - .35f) / .65f)); p->elbow[i] = .4f; p->arm_out[i] = 1.2f; }
}
static void jump_air(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    LU_UNUSED(par); float k = h->height / 1.78f, b = sinf(LU_TAU * u);
    p->pelvis = V3(0, .02f, 0); p->spine_pitch = .1f; p->head_pitch = -.1f;
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f; p->foot[i] = V3(sg * .12f * k, .3f * k + (i ? .1f : 0), i ? -.1f : .18f); p->foot_pitch[i] = .5f;
        p->arm_swing[i] = 1.f + .1f * b; p->arm_out[i] = .9f; p->elbow[i] = .6f; }
}
static void leap_air(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    LU_UNUSED(par); float k = h->height / 1.78f, b = sinf(LU_TAU * u);
    p->spine_pitch = .25f; p->pelvis_pitch = .1f; p->head_pitch = -.35f; p->pelvis = V3(0, .05f, 0);
    /* ножницы: одна нога вперёд, другая отведена назад */
    p->foot[0] = V3(.1f * k, .35f * k, .45f * k + .03f * b); p->foot_pitch[0] = -.2f;
    p->foot[1] = V3(-.1f * k, .45f * k, -.5f * k - .03f * b); p->foot_pitch[1] = .9f; p->knee_pole[1] = V3(-.1f, .2f, 2.f);
    p->arm_swing[0] = -.7f; p->arm_swing[1] = 1.4f; for (int i = 0; i < 2; i++) { p->arm_out[i] = .8f; p->elbow[i] = .5f; }
}
static void fall(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    LU_UNUSED(par); float k = h->height / 1.78f, b = sinf(LU_TAU * u), c = cosf(LU_TAU * u);
    p->spine_pitch = -.05f; p->head_pitch = .2f;
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f, w = i ? c : b;
        p->foot[i] = V3(sg * .14f * k, .22f * k + .06f * w, .08f * w); p->foot_pitch[i] = .4f;
        p->arm_out[i] = .35f + .2f * w; p->arm_swing[i] = .6f + .4f * w; p->elbow[i] = .7f; }
}
static void land(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    float hard = *(const float *)par, k = h->height / 1.78f;
    float c = curve(u, (const float[]){0, .2f, .18f, 1, .55f + hard * .3f, .8f, 1, 0}, 4);
    p->pelvis = V3(0, -(.28f + .25f * hard) * k * c, -.05f * c); p->pelvis_pitch = (.3f + .3f * hard) * c; p->spine_pitch = (.35f + .4f * hard) * c; p->head_pitch = -.5f * c;
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f; p->foot[i] = V3(sg * .15f * k, h->foot_h, i ? -.08f : .1f); p->foot_yaw[i] = sg * .15f;
        if (hard > 0) { p->hand_ik[i] = c > .5f; p->hand[i] = V3(sg * .25f * k, .05f + .3f * (1 - c), .35f); p->elbow_pole[i] = V3(sg * 1.f, .8f, -.3f); }
        p->arm_out[i] = 1.f; p->arm_swing[i] = .5f * c; p->elbow[i] = .6f; }
}
static void roll(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    LU_UNUSED(par); float k = h->height / 1.78f;
    float rot = LU_TAU * ease(lu_clampf((u - .12f) / .72f, 0, 1));
    /* таз опускается и катится; корень едет вперёд 1.6 м */
    p->root = V3(0, 0, 1.6f * k * ease(u));
    float low = sinf(LU_PI * lu_clampf((u - .05f) / .9f, 0, 1));
    p->pelvis = V3(0, -.5f * k * low, 0); p->pelvis_pitch = rot; p->spine_pitch = .9f * low; p->head_pitch = -.2f * low + .6f * low;
    /* конечности поджаты: цели относительно таза */
    m4 pel = m4_trs(V3(0, h->hip_h + .06f * k + p->pelvis.y, 0), RX(rot), V3(1, 1, 1));
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f;
        v3 f = m4_point(pel, V3(sg * .12f * k, -.32f * k, .25f * k)); if (f.y < h->foot_h) f.y = h->foot_h;
        v3 rest = V3(sg * .12f * k, h->foot_h, i ? -.1f : .1f);
        float tuck = low; p->foot[i] = v3_lerp(rest, f, tuck); p->foot_pitch[i] = rot * tuck;
        p->knee_pole[i] = m4_point(pel, V3(sg * .15f, .3f, 2.f));
        p->arm_out[i] = 1.f; p->arm_swing[i] = .9f * low; p->elbow[i] = 1.6f * low + .2f; p->clav_fwd[i] = .3f * low; }
}
static void turn180(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    LU_UNUSED(par); float k = h->height / 1.78f; float e = ease(u);
    p->root_yaw = LU_PI * e;
    p->pelvis = V3(0, -.12f * k * sinf(LU_PI * u), 0); p->spine_pitch = .2f * sinf(LU_PI * u); p->spine_yaw = .5f * sinf(LU_PI * u); p->head_yaw = .4f * sinf(LU_PI * u);
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f, step = i ? ease((u - .2f) / .5f) : ease((u - .45f) / .5f);
        p->foot[i] = V3(sg * .13f * k, h->foot_h + .12f * sinf(LU_PI * step), (i ? -.1f : .1f));
        p->foot_yaw[i] = -LU_PI * e * (1 - step) * 0; p->arm_out[i] = 1.1f; p->arm_swing[i] = (i ? .4f : -.4f) * sinf(LU_PI * u); p->elbow[i] = .7f; }
}
/* ---------- лазание (стена в +Z на расстоянии ~0.32 м от корня) ---------- */
#define WALLZ .32f
static void climb(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    LU_UNUSED(par); float k = h->height / 1.78f;
    p->pelvis = V3(0, -.05f * k, .1f); p->pelvis_pitch = -.12f; p->spine_pitch = .05f; p->head_pitch = -.35f;
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f, ph = u + (i ? .5f : 0); ph -= floorf(ph);
        /* кисть: держится (0..0.5) и тянется вверх (0.5..1) */
        float hy, fy;
        if (ph < .5f) { hy = lu_lerpf(2.15f, 1.75f, ph / .5f); fy = lu_lerpf(.55f, .15f, ph / .5f); }
        else { float s = (ph - .5f) / .5f; hy = lu_lerpf(1.75f, 2.15f, ease(s)); fy = lu_lerpf(.15f, .55f, ease(s)); }
        p->hand_ik[i] = 1; p->hand[i] = V3(sg * .2f * k, hy * k, WALLZ - .02f + (ph >= .5f ? .06f * sinf(LU_PI * (ph - .5f) * 2) * -1 : 0));
        p->elbow_pole[i] = V3(sg * .9f, 1.2f * k, -.6f); p->finger_curl[i] = ph < .5f ? 1.1f : .4f; p->hand_pitch[i] = .4f;
        float fz = WALLZ - .1f - (ph >= .5f ? .1f * sinf(LU_PI * (ph - .5f) * 2) : 0);
        p->foot[(i + 1) % 2] = V3(-sg * .14f * k, fy * k + h->foot_h, fz); p->foot_pitch[(i + 1) % 2] = .3f;
        p->knee_pole[(i + 1) % 2] = V3(-sg * .3f, .6f, 2.f); }
    p->pelvis_roll = .06f * sinf(LU_TAU * u); p->spine_roll = -.08f * sinf(LU_TAU * u);
}
/* вис на карнизе: верх уступа на высоте ~2.05 м от корня */
#define LEDGEY 2.05f
static void hang_pose(float u, const lu_humanoid *h, lu_hpose *p, int braced, float shimmy) {
    float k = h->height / 1.78f, sw = sinf(LU_TAU * u);
    p->pelvis = V3(0, -.03f * k, .06f + (braced ? .06f : 0)); p->pelvis_pitch = braced ? -.2f : -.05f + .03f * sw; p->head_pitch = -.45f;
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f;
        float sx = 0, sy = 0;
        if (shimmy != 0) { float ph = u + (i ? .5f : 0); ph -= floorf(ph); float lead = shimmy * sg > 0 ? 1.f : -1.f; LU_UNUSED(lead);
            /* кисть переставляется в сторону движения, затем тело подтягивается */
            sx = .12f * shimmy * (ph < .5f ? ease(ph * 2) : 1 - ease((ph - .5f) * 2)) * (i == (shimmy > 0 ? 0 : 1) ? 1.f : .4f); sy = .04f * sinf(LU_PI * ph * 2) * (ph < .5f); }
        p->hand_ik[i] = 1; p->hand[i] = V3(sg * .24f * k + sx, LEDGEY * k - .02f + sy, WALLZ + .02f);
        p->elbow_pole[i] = V3(sg * 1.f, 1.5f * k, -.5f); p->finger_curl[i] = 1.3f; p->hand_pitch[i] = .9f;
        if (braced) { p->foot[i] = V3(sg * .17f * k, .62f * k, WALLZ - .12f); p->foot_pitch[i] = .2f; p->knee_pole[i] = V3(sg * .4f, .7f, 2.f); }
        else { p->foot[i] = V3(sg * .1f * k, .14f * k + .02f * sw * sg, .08f + .04f * sw); p->foot_pitch[i] = .6f; } }
}
static void hang(float u, const lu_humanoid *h, lu_hpose *p, const void *par) { LU_UNUSED(par); hang_pose(u, h, p, 0, 0); }
static void brace(float u, const lu_humanoid *h, lu_hpose *p, const void *par) { LU_UNUSED(par); hang_pose(u, h, p, 1, 0); }
static void shimmy(float u, const lu_humanoid *h, lu_hpose *p, const void *par) { hang_pose(u, h, p, 1, *(const float *)par); }
/* подтягивание на уступ: корень поднимается на высоту уступа и уходит вперёд */
static void climb_up(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    LU_UNUSED(par); float k = h->height / 1.78f;
    float up = curve(u, (const float[]){0, 0, .45f, .62f, .75f, .95f, 1, 1}, 4), fw = curve(u, (const float[]){0, 0, .4f, .05f, .8f, .8f, 1, 1}, 4);
    p->root = V3(0, LEDGEY * k * up, (WALLZ + .3f) * fw);
    v3 ledge = V3(0, LEDGEY * k, WALLZ + .02f);
    float push = lu_clampf((u - .45f) / .3f, 0, 1);
    p->pelvis_pitch = lu_lerpf(-.05f, .6f, sinf(LU_PI * lu_clampf(u * 1.2f, 0, 1))); p->spine_pitch = .7f * sinf(LU_PI * lu_clampf(u * 1.1f, 0, 1)); p->head_pitch = -.3f;
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f;
        p->hand_ik[i] = u < .82f; p->hand[i] = v3_sub(V3(sg * .24f * k, ledge.y - .02f, ledge.z + .05f * push), p->root);
        p->elbow_pole[i] = V3(sg * 1.f, 1.5f * k, -1.f + 2.f * push); p->finger_curl[i] = 1.2f;
        /* ноги: сначала упираются в стену, затем шаг на уступ */
        float step = ease((u - (i ? .55f : .65f)) / .25f);
        v3 wallf = v3_sub(V3(sg * .15f * k, LEDGEY * k * .5f + .1f, WALLZ - .1f), p->root);
        v3 topf = v3_sub(V3(sg * .12f * k, LEDGEY * k + h->foot_h, WALLZ + .35f + (i ? 0 : .1f)), p->root);
        v3 f = u < .45f ? wallf : v3_lerp(wallf, topf, step);
        if (f.y < h->foot_h - LEDGEY * k * up) f.y = h->foot_h - LEDGEY * k * up;
        p->foot[i] = f; p->foot_pitch[i] = .3f * (1 - step); p->knee_pole[i] = V3(sg * .3f, 1.f, 2.f);
        p->arm_out[i] = 1.3f; p->elbow[i] = .3f; }
    p->pelvis = V3(0, -.1f * k * sinf(LU_PI * u), 0);
}
static void drop_hang(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    /* обратное подтягиванию: стоим у края (над уступом) → вис */
    float v = 1 - u; climb_up(lu_clampf(v * .95f + .02f, 0, 1), h, p, par);
}
static void vault(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    LU_UNUSED(par); float k = h->height / 1.78f;
    p->root = V3(0, .45f * k * sinf(LU_PI * u), 1.8f * k * u);
    float tuck = sinf(LU_PI * lu_clampf((u - .15f) / .7f, 0, 1));
    p->pelvis_roll = .35f * tuck; p->pelvis_yaw = .3f * tuck; p->spine_pitch = .3f * tuck; p->head_pitch = -.2f;
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f;
        p->hand_ik[i] = i == 0 && u > .2f && u < .6f; p->hand[i] = v3_sub(V3(.15f * k, 1.0f * k, .9f * k), p->root); p->elbow_pole[i] = V3(sg, 1.f, -1.f);
        p->foot[i] = V3(sg * .1f * k - .25f * tuck, h->foot_h + .45f * k * tuck, .1f * tuck); p->foot_pitch[i] = .3f * tuck;
        p->arm_out[i] = lu_lerpf(1.3f, .7f, tuck); p->arm_swing[i] = 1.f * tuck; p->elbow[i] = .4f; }
}
static void wallrun(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    LU_UNUSED(par); float k = h->height / 1.78f;
    p->pelvis = V3(0, 0, .02f); p->pelvis_pitch = -.25f; p->spine_pitch = .1f; p->head_pitch = -.5f;
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f, ph = u + (i ? .5f : 0); ph -= floorf(ph);
        float fy = ph < .5f ? lu_lerpf(.9f, .1f, ph * 2) : lu_lerpf(.1f, .9f, ease((ph - .5f) * 2));
        p->foot[i] = V3(sg * .12f * k, fy * k + h->foot_h, WALLZ - .08f - (ph >= .5f ? .15f * sinf(LU_PI * (ph - .5f) * 2) : 0)); p->foot_pitch[i] = -.6f;
        p->knee_pole[i] = V3(sg * .2f, 1.2f, 2.f);
        p->hand_ik[i] = 1; float hy = ph < .5f ? lu_lerpf(2.f, 2.3f, ph * 2) : lu_lerpf(2.3f, 2.f, (ph - .5f) * 2);
        p->hand[(i + 1) % 2] = V3(-sg * .22f * k, hy * k, WALLZ - .05f); p->elbow_pole[(i + 1) % 2] = V3(-sg, 1.2f, -.5f); p->finger_curl[i] = .5f; }
}
static void strafe(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    float dir = *(const float *)par, k = h->height / 1.78f;
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f, ph = u + (i ? .5f : 0); ph -= floorf(ph);
        float x = ph < .5f ? lu_lerpf(.18f, -.18f, ph * 2) : lu_lerpf(-.18f, .18f, ease((ph - .5f) * 2));
        p->foot[i] = V3(sg * .13f * k + x * dir * k, h->foot_h + (ph >= .5f ? .08f * sinf(LU_PI * (ph - .5f) * 2) : 0), 0); p->arm_out[i] = 1.2f; p->elbow[i] = .5f; }
    p->pelvis = V3(0, -.04f * k + .015f * cosf(LU_TAU * 2 * u), 0); p->pelvis_roll = -.05f * dir;
}
/* ---------- бой (оружие в правой руке) ---------- */
static void guard_base(lu_hpose *p, const lu_humanoid *h) {
    float k = h->height / 1.78f;
    p->pelvis = V3(0, -.1f * k, 0); p->pelvis_yaw = -.35f; p->spine_yaw = .25f; p->spine_pitch = .1f; p->head_yaw = .15f;
    p->foot[0] = V3(.14f * k, h->foot_h, .22f); p->foot[1] = V3(-.16f * k, h->foot_h, -.18f); p->foot_yaw[0] = -.2f; p->foot_yaw[1] = -.6f;
    p->arm_out[1] = 1.1f; p->arm_swing[1] = .55f; p->elbow[1] = 1.2f; p->arm_twist[1] = .3f; p->finger_curl[1] = 1.4f;
    p->arm_out[0] = 1.2f; p->arm_swing[0] = .3f; p->elbow[0] = .9f;
}
static void guard_idle(float u, const lu_humanoid *h, lu_hpose *p, const void *par) { LU_UNUSED(par); guard_base(p, h); float b = sinf(LU_TAU * u); p->pelvis.y += .01f * b; p->arm_swing[1] += .04f * b; }
static void attack(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    int kind = *(const int *)par; guard_base(p, h);
    float wind = ease(u / .3f), strike = ease((u - .3f) / .25f), rec = ease((u - .65f) / .35f);
    float a = wind * (1 - strike), s = strike * (1 - rec);
    if (kind == 0) { /* горизонтальный справа налево */
        p->spine_yaw = .25f - .6f * a + .9f * s; p->pelvis_yaw = -.35f - .3f * a + .5f * s;
        p->arm_out[1] = lu_lerpf(1.1f, .2f, a) + .9f * s; p->arm_swing[1] = .55f - .3f * a + 1.1f * s; p->elbow[1] = 1.2f - .4f * a - .9f * s; p->arm_twist[1] = .3f + .8f * a; }
    else if (kind == 1) { /* обратный удар */
        p->spine_yaw = .25f + .7f * a - .9f * s; p->arm_out[1] = 1.1f - .5f * a + .2f * s; p->arm_swing[1] = .55f + .9f * a + .3f * s; p->elbow[1] = 1.2f + .5f * a - 1.f * s; p->arm_twist[1] = .3f - 1.f * a; }
    else { /* рубящий сверху (тяжёлый) */
        p->spine_pitch = .1f - .3f * a + .6f * s; p->arm_out[1] = 1.1f - .9f * a + .2f * s; p->arm_swing[1] = .55f + 1.9f * a - .2f * s; p->elbow[1] = 1.2f + .4f * a - 1.1f * s;
        p->pelvis.y -= .12f * s; p->foot[0].z += .15f * s; }
    p->pelvis.z += .08f * s;
}
static void block(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    LU_UNUSED(par); guard_base(p, h); float b = sinf(LU_TAU * u) * .02f;
    p->arm_out[1] = .5f; p->arm_swing[1] = 1.2f + b; p->elbow[1] = 1.4f; p->arm_twist[1] = 1.2f; p->spine_pitch = -.05f; p->pelvis.y -= .05f;
    p->arm_out[0] = .9f; p->arm_swing[0] = .9f; p->elbow[0] = 1.5f;
}
static void counter(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    LU_UNUSED(par); guard_base(p, h);
    float parry = sinf(LU_PI * lu_clampf(u / .35f, 0, 1)), thrust = sinf(LU_PI * lu_clampf((u - .35f) / .45f, 0, 1));
    p->arm_out[1] = 1.1f - .6f * parry; p->arm_swing[1] = .55f + .8f * parry + .6f * thrust; p->elbow[1] = 1.2f - 1.1f * thrust; p->arm_twist[1] = .3f + 1.f * parry;
    p->pelvis.z += .2f * thrust; p->spine_pitch = .1f + .25f * thrust; p->foot[0].z += .2f * thrust; p->spine_yaw = .25f - .3f * thrust;
}
static void hit(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    LU_UNUSED(par); guard_base(p, h); float r = sinf(LU_PI * lu_clampf(u * 1.4f, 0, 1));
    p->spine_pitch = -.45f * r; p->head_pitch = .3f * r; p->pelvis.z -= .12f * r; p->spine_roll = .2f * r; p->arm_out[0] = .8f; p->arm_swing[0] = .3f + .5f * r;
}
static void dodge(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    LU_UNUSED(par); float k = h->height / 1.78f, s = sinf(LU_PI * u);
    p->root = V3(-1.4f * k * ease(u), 0, -.4f * ease(u)); p->pelvis = V3(0, -.3f * k * s, 0); p->pelvis_roll = .3f * s; p->spine_roll = -.4f * s; p->spine_pitch = .3f * s;
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f; p->foot[i] = V3(sg * (.15f + .2f * s) * k, h->foot_h + (i ? .1f * s : 0), 0); p->arm_out[i] = .9f; p->elbow[i] = .9f; p->arm_swing[i] = .5f * s; }
}
/* ---------- жизнь толпы ---------- */
static void talk(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    idle(u, h, p, par); float a = sinf(LU_TAU * u * 2), b = sinf(LU_TAU * u * 3 + 1);
    p->arm_out[1] = 1.05f + .1f * a; p->arm_swing[1] = .5f + .25f * fmaxf(0, a); p->elbow[1] = 1.3f + .2f * b; p->hand_pitch[1] = -.3f * b; p->arm_twist[1] = -.6f;
    p->arm_out[0] = 1.2f + .08f * b; p->arm_swing[0] = .2f + .15f * fmaxf(0, b); p->elbow[0] = .8f + .3f * fmaxf(0, b);
    p->head_yaw = .15f * a; p->head_pitch = -.05f + .05f * b; p->spine_yaw = .05f * a;
}
static void look_around(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    idle(u, h, p, par); float y = curve(u, (const float[]){0, 0, .2f, .8f, .45f, .8f, .65f, -.8f, .85f, -.8f, 1, 0}, 6);
    p->head_yaw = y * .9f; p->spine_yaw = y * .35f; p->head_pitch = -.05f;
}
static void sit(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    LU_UNUSED(par); float k = h->height / 1.78f, b = sinf(LU_TAU * u);
    p->pelvis = V3(0, -.5f * k, -.15f); p->spine_pitch = .12f + .01f * b; p->pelvis_pitch = -.1f; p->head_pitch = -.05f;
    for (int i = 0; i < 2; i++) { float sg = i ? -1.f : 1.f; p->foot[i] = V3(sg * .14f * k, h->foot_h, .38f * k); p->knee_pole[i] = V3(sg * .2f, .6f, 2.f);
        p->hand_ik[i] = 1; p->hand[i] = V3(sg * .16f * k, .52f * k, .2f * k); p->elbow_pole[i] = V3(sg, .8f, -1.f); }
}
static void cheer(float u, const lu_humanoid *h, lu_hpose *p, const void *par) {
    idle(u, h, p, par); float a = sinf(LU_TAU * u * 2);
    for (int i = 0; i < 2; i++) { p->arm_out[i] = -.6f + .2f * a; p->arm_swing[i] = .2f; p->elbow[i] = .4f + .3f * a; }
    p->pelvis.y += .03f * fmaxf(0, a); p->head_pitch = -.2f;
}

void lu_motions_build(lu_motions *m, const lu_skeleton *s, const lu_humanoid *h) {
    memset(m, 0, sizeof *m);
    /*                stride duty  lift   bounce drop  lean  arm   elbow yaw   roll  heel  sx    crouch kick */
    static const Gait WALK = {1.45f, .62f, .1f, .018f, .02f, .06f, .35f, .25f, .1f, .04f, 1.f, .09f, 0, 0};
    static const Gait RUN = {2.3f, .38f, .2f, -.035f, .07f, .3f, .9f, 1.4f, .14f, .05f, .8f, .07f, 0, .14f};
    static const Gait SPRINT = {3.0f, .34f, .26f, -.04f, .09f, .5f, 1.25f, 1.55f, .16f, .05f, .7f, .06f, 0, .2f};
    static const Gait CROUCH = {1.1f, .6f, .1f, .01f, .05f, .3f, .2f, .8f, .08f, .03f, .6f, .12f, .32f, 0};
    float dur_w = WALK.stride / lu_gait_speed[1], dur_r = RUN.stride / lu_gait_speed[2], dur_s = SPRINT.stride / lu_gait_speed[3];
    clip_from(&m->clip[LU_MOT_IDLE], "idle", s, h, 4.f, 1, 16, idle, NULL, 0);
    clip_from(&m->clip[LU_MOT_WALK], "walk", s, h, dur_w, 1, 24, gait, &WALK, 0);
    clip_from(&m->clip[LU_MOT_RUN], "run", s, h, dur_r, 1, 20, gait, &RUN, 0);
    clip_from(&m->clip[LU_MOT_SPRINT], "sprint", s, h, dur_s, 1, 20, gait, &SPRINT, 0);
    clip_from(&m->clip[LU_MOT_CROUCH_IDLE], "crouch_idle", s, h, 3.f, 1, 12, crouch_idle, NULL, 0);
    clip_from(&m->clip[LU_MOT_CROUCH_WALK], "crouch_walk", s, h, CROUCH.stride / 1.6f, 1, 20, gait, &CROUCH, 0);
    for (int g = 0; g < 4; g++) {  /* шаги: левая нога касается в начале цикла, правая — в середине */
        lu_clip *c = &m->clip[g == 0 ? LU_MOT_WALK : g == 1 ? LU_MOT_RUN : g == 2 ? LU_MOT_SPRINT : LU_MOT_CROUCH_WALK];
        float d = g == 0 ? WALK.duty : g == 1 ? RUN.duty : g == 2 ? SPRINT.duty : CROUCH.duty; LU_UNUSED(d);
        lu_clip_event(c, .0f, LU_EV_FOOT_L); lu_clip_event(c, c->duration * .5f, LU_EV_FOOT_R);
    }
    clip_from(&m->clip[LU_MOT_JUMP_UP], "jump_up", s, h, .28f, 0, 10, jump_up, NULL, 0);
    clip_from(&m->clip[LU_MOT_JUMP_AIR], "jump_air", s, h, 1.2f, 1, 8, jump_air, NULL, 0);
    clip_from(&m->clip[LU_MOT_LEAP_AIR], "leap_air", s, h, 1.f, 1, 8, leap_air, NULL, 0);
    clip_from(&m->clip[LU_MOT_FALL], "fall", s, h, .8f, 1, 10, fall, NULL, 0);
    float soft = 0, hard = 1;
    clip_from(&m->clip[LU_MOT_LAND_SOFT], "land_soft", s, h, .35f, 0, 10, land, &soft, 0);
    clip_from(&m->clip[LU_MOT_LAND_HARD], "land_hard", s, h, .75f, 0, 14, land, &hard, 0);
    clip_from(&m->clip[LU_MOT_ROLL], "roll", s, h, .7f, 0, 24, roll, NULL, 1);
    clip_from(&m->clip[LU_MOT_TURN180], "turn180", s, h, .35f, 0, 10, turn180, NULL, 0);
    m->clip[LU_MOT_TURN180].root_joint = h->root;
    clip_from(&m->clip[LU_MOT_CLIMB], "climb", s, h, 1.1f, 1, 20, climb, NULL, 0);
    lu_clip_event(&m->clip[LU_MOT_CLIMB], .5f * 1.1f, LU_EV_HAND_L); lu_clip_event(&m->clip[LU_MOT_CLIMB], 0, LU_EV_HAND_R);
    clip_from(&m->clip[LU_MOT_HANG], "hang", s, h, 2.4f, 1, 12, hang, NULL, 0);
    clip_from(&m->clip[LU_MOT_BRACE], "brace", s, h, 2.4f, 1, 12, brace, NULL, 0);
    float sl = 1, sr = -1;
    clip_from(&m->clip[LU_MOT_SHIMMY_L], "shimmy_l", s, h, .9f, 1, 16, shimmy, &sl, 0);
    clip_from(&m->clip[LU_MOT_SHIMMY_R], "shimmy_r", s, h, .9f, 1, 16, shimmy, &sr, 0);
    clip_from(&m->clip[LU_MOT_CLIMB_UP], "climb_up", s, h, 1.05f, 0, 30, climb_up, NULL, 2);
    clip_from(&m->clip[LU_MOT_DROP_HANG], "drop_hang", s, h, .8f, 0, 24, drop_hang, NULL, 2);
    clip_from(&m->clip[LU_MOT_VAULT], "vault", s, h, .6f, 0, 20, vault, NULL, 2);
    clip_from(&m->clip[LU_MOT_WALLRUN], "wallrun", s, h, .5f, 1, 16, wallrun, NULL, 0);
    clip_from(&m->clip[LU_MOT_STRAFE_L], "strafe_l", s, h, .7f, 1, 12, strafe, &sl, 0);
    clip_from(&m->clip[LU_MOT_STRAFE_R], "strafe_r", s, h, .7f, 1, 12, strafe, &sr, 0);
    int k0 = 0, k1 = 1, k2 = 2;
    clip_from(&m->clip[LU_MOT_ATK1], "atk1", s, h, .5f, 0, 16, attack, &k0, 0); lu_clip_event(&m->clip[LU_MOT_ATK1], .2f, LU_EV_HIT);
    clip_from(&m->clip[LU_MOT_ATK2], "atk2", s, h, .5f, 0, 16, attack, &k1, 0); lu_clip_event(&m->clip[LU_MOT_ATK2], .2f, LU_EV_HIT);
    clip_from(&m->clip[LU_MOT_ATK3], "atk3", s, h, .7f, 0, 20, attack, &k2, 0); lu_clip_event(&m->clip[LU_MOT_ATK3], .3f, LU_EV_HIT);
    clip_from(&m->clip[LU_MOT_BLOCK], "block", s, h, 1.f, 1, 8, block, NULL, 0);
    clip_from(&m->clip[LU_MOT_COUNTER], "counter", s, h, .7f, 0, 20, counter, NULL, 0); lu_clip_event(&m->clip[LU_MOT_COUNTER], .45f, LU_EV_HIT);
    clip_from(&m->clip[LU_MOT_HIT], "hit", s, h, .45f, 0, 12, hit, NULL, 0);
    clip_from(&m->clip[LU_MOT_DODGE], "dodge", s, h, .45f, 0, 14, dodge, NULL, 1);
    clip_from(&m->clip[LU_MOT_GUARD_IDLE], "guard_idle", s, h, 1.6f, 1, 8, guard_idle, NULL, 0);
    clip_from(&m->clip[LU_MOT_TALK], "talk", s, h, 3.f, 1, 24, talk, NULL, 0);
    clip_from(&m->clip[LU_MOT_LOOK_AROUND], "look_around", s, h, 5.f, 1, 24, look_around, NULL, 0);
    clip_from(&m->clip[LU_MOT_SIT], "sit", s, h, 4.f, 1, 8, sit, NULL, 0);
    clip_from(&m->clip[LU_MOT_CHEER], "cheer", s, h, 1.f, 1, 12, cheer, NULL, 0);
    LU_UNUSED(tri); LU_UNUSED(ease_in); LU_UNUSED(ease_out); LU_UNUSED(dur_r); LU_UNUSED(dur_s);
}
void lu_motions_free(lu_motions *m) { for (int i = 0; i < LU_MOT_N; i++) lu_clip_free(&m->clip[i]); }
