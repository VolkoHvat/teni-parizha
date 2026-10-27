/* Lutèce Engine — гуманоид: стандартный скелет (имена как у Mixamo — для переноса анимаций из glTF),
   авторинг поз через цели конечностей (IK) и процедурная библиотека движений: ходьба, бег, спринт, прыжки,
   лазание, вис, перебежка по карнизу, подтягивание, кувырок, разворот, удары и т. д.
   Оси: Y вверх, персонаж смотрит в +Z, его левая сторона — +X. Индекс 0 — левая сторона, 1 — правая. */
#ifndef LU_HUMANOID_H
#define LU_HUMANOID_H
#include "lu_anim.h"

typedef struct {
    int root, hips, spine, spine1, spine2, neck, head;
    int clav[2], arm[2], fore[2], hand[2], fingers[2], thumb[2];
    int thigh[2], shin[2], foot[2], toe[2];
    float height, leg_len, arm_len, hip_h, shoulder_h, foot_h;
} lu_humanoid;
void lu_humanoid_skeleton(lu_skeleton *s, lu_humanoid *h, float height, int female);

/* поза для авторинга: туловище задаётся углами, конечности — целями в пространстве модели */
typedef struct {
    v3 root;                   /* смещение корня (движение корня в клипах подтягивания, кувырка) */
    float root_yaw;            /* поворот корня (разворот) */
    v3 pelvis;                 /* смещение таза от нормального положения */
    float pelvis_yaw, pelvis_pitch, pelvis_roll;
    float spine_pitch, spine_yaw, spine_roll;   /* распределяется по трём позвонкам */
    float neck_pitch, head_pitch, head_yaw, head_roll;
    /* ноги: положение лодыжки (модель) и наклон стопы; колено смотрит по pole */
    v3 foot[2]; float foot_pitch[2], foot_yaw[2], toe_bend[2]; v3 knee_pole[2];
    /* руки: если hand_ik[i] — кисть в точке hand[i], иначе прямое управление углами */
    int hand_ik[2]; v3 hand[2]; v3 elbow_pole[2];
    float arm_swing[2], arm_out[2], arm_twist[2], elbow[2];   /* углы для прямого управления */
    float hand_pitch[2], hand_roll[2], finger_curl[2], clav_up[2], clav_fwd[2];
} lu_hpose;
void lu_hpose_rest(const lu_humanoid *h, lu_hpose *p);         /* стойка: ноги на ширине таза, руки вдоль тела */
void lu_hpose_solve(const lu_skeleton *s, const lu_humanoid *h, const lu_hpose *hp, lu_pose *out);

/* библиотека движений */
enum {
    LU_MOT_IDLE, LU_MOT_WALK, LU_MOT_RUN, LU_MOT_SPRINT, LU_MOT_CROUCH_IDLE, LU_MOT_CROUCH_WALK,
    LU_MOT_JUMP_UP, LU_MOT_JUMP_AIR, LU_MOT_LEAP_AIR, LU_MOT_FALL, LU_MOT_LAND_SOFT, LU_MOT_LAND_HARD, LU_MOT_ROLL,
    LU_MOT_TURN180, LU_MOT_CLIMB, LU_MOT_HANG, LU_MOT_SHIMMY_L, LU_MOT_SHIMMY_R, LU_MOT_CLIMB_UP, LU_MOT_VAULT,
    LU_MOT_WALLRUN, LU_MOT_DROP_HANG, LU_MOT_BRACE, LU_MOT_STRAFE_L, LU_MOT_STRAFE_R,
    LU_MOT_ATK1, LU_MOT_ATK2, LU_MOT_ATK3, LU_MOT_BLOCK, LU_MOT_COUNTER, LU_MOT_HIT, LU_MOT_DODGE, LU_MOT_GUARD_IDLE,
    LU_MOT_TALK, LU_MOT_LOOK_AROUND, LU_MOT_SIT, LU_MOT_CHEER,
    LU_MOT_N
};
/* события клипов */
enum { LU_EV_FOOT_L = 1, LU_EV_FOOT_R, LU_EV_HAND_L, LU_EV_HAND_R, LU_EV_HIT, LU_EV_LAND };
/* библиотеку строят один раз на эталонном скелете (рост 1.78 м, мужской); другим скелетам ставят tscale = рост/1.78 */
typedef struct { lu_clip clip[LU_MOT_N]; } lu_motions;
extern const char *lu_motion_names[LU_MOT_N];
void lu_motions_build(lu_motions *m, const lu_skeleton *s, const lu_humanoid *h);
void lu_motions_free(lu_motions *m);
/* скорость (м/с), под которую сделаны циклы походки — для синхронизации шагов с движением */
extern const float lu_gait_speed[4]; /* стоя, шаг, бег, спринт */
#endif
