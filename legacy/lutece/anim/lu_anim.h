/* Lutèce Engine — скелетная анимация: скелеты, позы, клипы с ключами, смешивание, аддитивные слои,
   маски по костям, граф состояний с плавными переходами, двухзвенная IK и поворот головы к цели. */
#ifndef LU_ANIM_H
#define LU_ANIM_H
#include "../core/lu_core.h"
#include "../gfx/lu_gfx.h"

#define LU_JOINT_NAME 32
typedef struct {
    char name[LU_JOINT_NAME];
    int parent;            /* -1 у корня; родитель всегда раньше потомка */
    xform bind;            /* локальная поза привязки */
    m4 inv_bind;           /* обратная матрица привязки в пространстве модели */
} lu_joint;
/* tscale — масштаб дорожек переноса клипов (перенос анимаций, сделанных для другого роста) */
typedef struct { lu_joint j[LU_MAX_BONES]; int n; float tscale; } lu_skeleton;
int  lu_skel_add(lu_skeleton *s, const char *name, int parent, xform bind);
int  lu_skel_find(const lu_skeleton *s, const char *name);
void lu_skel_finalize(lu_skeleton *s);   /* вычислить inv_bind из поз привязки */

typedef struct { xform l[LU_MAX_BONES]; int n; v3 root_delta; float root_yaw_delta; } lu_pose;
void lu_pose_bind(const lu_skeleton *s, lu_pose *p);
void lu_pose_blend(const lu_pose *a, const lu_pose *b, float w, lu_pose *out);           /* out может совпадать с a */
void lu_pose_blend_masked(const lu_pose *a, const lu_pose *b, const float *mask, float w, lu_pose *out);
/* аддитивный слой: out = base * (add - ref) * w */
void lu_pose_additive(const lu_pose *base, const lu_pose *add, const lu_pose *ref, float w, lu_pose *out);
void lu_pose_model(const lu_skeleton *s, const lu_pose *p, xform *model);                /* локальные → модельные */
void lu_pose_palette(const lu_skeleton *s, const xform *model, m4 *palette);             /* для видеокарты */

/* ---------- клипы ---------- */
enum { LU_TRACK_T, LU_TRACK_R, LU_TRACK_S };
typedef struct { int joint, path, nkeys; float *times; float *values; /* 3 или 4 float на ключ */ int step; } lu_track;
typedef struct {
    char name[32];
    float duration;
    int loop;
    lu_track *tracks; int ntracks, cap;
    int root_joint;        /* корень для извлечения движения (-1 — не извлекать) */
    int root_motion;       /* 1 — горизонтальное смещение корня уходит в root_motion графа, 2 — и вертикальное тоже */
    /* события (шаги, удары): времена и коды */
    float ev_t[16]; int ev_code[16]; int nev;
} lu_clip;
void lu_clip_init(lu_clip *c, const char *name, float duration, int loop);
void lu_clip_free(lu_clip *c);
/* добавить ключ (дорожки создаются автоматически, ключи должны идти по возрастанию времени) */
void lu_clip_key_t(lu_clip *c, int joint, float t, v3 v);
void lu_clip_key_r(lu_clip *c, int joint, float t, quat q);
void lu_clip_event(lu_clip *c, float t, int code);
/* выборка: начинает с позы привязки и накладывает дорожки */
void lu_clip_sample(const lu_clip *c, const lu_skeleton *s, float t, lu_pose *out);
/* события, пройденные на интервале (t0, t1] с учётом зацикливания; возвращает число */
int  lu_clip_events(const lu_clip *c, float t0, float t1, int *codes, int max);

/* ---------- граф состояний ---------- */
#define LU_AG_MAX_STATES 64
#define LU_AG_MAX_BLEND 6
typedef struct {
    char name[32];
    const lu_clip *clip[LU_AG_MAX_BLEND];   /* одномерное пространство смешивания по параметру */
    float at[LU_AG_MAX_BLEND]; int nclip;
    int param;                               /* индекс параметра для пространства смешивания */
    float speed;                             /* множитель скорости воспроизведения */
    int sync;                                /* синхронизировать фазы клипов (локомоция) */
} lu_ag_state;
typedef struct {
    const lu_skeleton *skel;
    lu_ag_state st[LU_AG_MAX_STATES]; int nst;
    float param[16];
    /* текущее и предыдущее состояние для перехода */
    int cur, prev; float t_cur, t_prev, fade, fade_len, prev_weight_at_start;
    float phase;                             /* нормализованная фаза для синхронизации */
    lu_pose pose, tmp_a, tmp_b;
    int events[16]; int nevents;             /* события текущего кадра */
    v3 root_motion;                          /* смещение корня за кадр (в пространстве модели) */
} lu_animgraph;
void lu_ag_init(lu_animgraph *g, const lu_skeleton *s);
int  lu_ag_state_clip(lu_animgraph *g, const char *name, const lu_clip *c, float speed);
int  lu_ag_state_blend(lu_animgraph *g, const char *name, int param, const lu_clip **clips, const float *at, int n, int sync);
int  lu_ag_find(const lu_animgraph *g, const char *name);
void lu_ag_play(lu_animgraph *g, int state, float fade, float start_time);   /* fade — длительность перехода */
void lu_ag_update(lu_animgraph *g, float dt);
float lu_ag_time(const lu_animgraph *g);       /* время в текущем состоянии */
float lu_ag_norm_time(const lu_animgraph *g);  /* 0..1 для текущего клипа */

/* ---------- инверсная кинематика (на модельных преобразованиях) ---------- */
/* двухзвенная цепочка a→b→c (бедро–колено–стопа / плечо–локоть–кисть); target и pole — в пространстве модели.
   Меняет локальные повороты a и b в позе; возвращает 1, если цель достижима */
int  lu_ik_two_bone(const lu_skeleton *s, lu_pose *p, int a, int b, int c, v3 target, v3 pole, float weight);
/* повернуть кость так, чтобы её ось fwd (локальная) смотрела на цель; ограничение угла max_angle */
void lu_ik_look(const lu_skeleton *s, lu_pose *p, int joint, v3 local_fwd, v3 target, float weight, float max_angle);
/* выставить мировой поворот кости (для стоп/кистей после IK) */
void lu_ik_set_model_rot(const lu_skeleton *s, lu_pose *p, int joint, quat model_rot, float weight);
#endif
