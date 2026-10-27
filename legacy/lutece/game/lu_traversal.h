/* Lutèce Engine — система свободного бега (паркур) в духе AC2:
   единый автомат состояний персонажа поверх контроллера капсулы, сенсоров уступов, графа анимаций и IK.
   Режим «свободного бега» (sprint): персонаж сам запрыгивает на стены, перепрыгивает препятствия,
   прыгает с края крыши в сторону движения и хватается за карнизы. */
#ifndef LU_TRAVERSAL_H
#define LU_TRAVERSAL_H
#include "../phys/lu_phys.h"
#include "../anim/lu_humanoid.h"

enum {
    LU_TR_GROUND, LU_TR_JUMP, LU_TR_AIR, LU_TR_LAND, LU_TR_ROLL, LU_TR_CLIMB, LU_TR_HANG, LU_TR_CLIMB_UP,
    LU_TR_DROP, LU_TR_VAULT, LU_TR_WALLRUN, LU_TR_TURN, LU_TR_HOP, LU_TR_N
};
extern const char *lu_tr_names[LU_TR_N];
typedef struct {
    v3 move;          /* желаемое направление в мире (XZ), длина 0..1 */
    int sprint;       /* свободный бег / спринт */
    int jump;         /* нажатие прыжка (однократное) */
    int drop;         /* отпустить / спуститься (однократное) */
    int crouch;       /* красться */
    int turn;         /* резкий разворот (однократное) */
} lu_tr_input;
typedef struct {
    int on;           /* использовать */
    float walk, run, sprint;            /* скорости */
    float jump_v, leap_v, gravity;      /* вертикальная скорость прыжка/дальнего прыжка, тяжесть */
    float climb_speed, shimmy_speed;
    float roll_height, hurt_height;     /* высоты падения: кувырок, урон */
    int auto_leap, auto_climb, auto_vault, ledge_grab, roll;  /* способности (могут открываться в игре) */
} lu_tr_params;
typedef struct {
    lu_tr_params P;
    lu_cc cc; float yaw, yaw_vel;
    int state; float st_t;              /* время в состоянии */
    lu_ledge ledge; v3 hang_pos;        /* текущий уступ и точка «корня» в висе */
    v3 anim_root_start;                 /* для клипов с движением корня */
    float fall_h, air_top;              /* высота последнего падения, верхняя точка полёта */
    v3 act_from, act_to; float act_h, act_dur; lu_ledge next_ledge;  /* кинематические действия (перемах, перескок между уступами) */
    float ignore_top;                   /* не хвататься снова за только что пройденный карниз */
    float damage;                       /* урон за кадр (для игры) */
    int events[16], nevents;            /* события анимации (шаги и т. п.) */
    int surface;                        /* поверхность под ногами (флаги) */
    /* анимация */
    const lu_skeleton *skel; const lu_humanoid *hum; const lu_motions *mot;
    lu_animgraph ag; int st_move, st_crouch, st_mot[LU_MOT_N];
    lu_pose pose;                       /* итоговая поза после IK */
    xform model[LU_MAX_BONES];
    float speed_smooth, lean, foot_ik_w, pelvis_off;
    float hand_ik_w;
    v3 look_target; float look_w;
    int ik_on;
} lu_traversal;
void lu_tr_default_params(lu_tr_params *p);
void lu_tr_init(lu_traversal *t, const lu_skeleton *s, const lu_humanoid *h, const lu_motions *m, v3 pos, float yaw);
void lu_tr_update(lu_traversal *t, const lu_phys_world *w, const lu_tr_input *in, float dt);
/* мировая матрица персонажа (для отрисовки) */
m4 lu_tr_world(const lu_traversal *t);
void lu_tr_teleport(lu_traversal *t, v3 pos, float yaw);

/* ---------- камера от третьего лица ---------- */
typedef struct {
    float yaw, pitch, dist, cur_dist; v3 target, eye, smooth_target;
    float fov, min_pitch, max_pitch, shake;
} lu_tp_camera;
void lu_tpcam_init(lu_tp_camera *c, float yaw);
void lu_tpcam_update(lu_tp_camera *c, const lu_phys_world *w, v3 focus, float look_dx, float look_dy, float dt);
#endif
