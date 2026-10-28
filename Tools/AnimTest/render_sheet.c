/* Тени Парижа — лист поз в картинку: читает суставы из anim_test --dump и рисует «манекен» из капсул
   рендером Lutèce без окна (EGL/Mesa). Запуск: render_sheet poses.txt out.ppm side|34
   Оси дампа — пространство персонажа UE (см, X вперёд, Y вправо, Z вверх); в Lutèce: x = -Y, y = Z, z = X (метры). */
#include "platform/lu_platform.h"
#include "gfx/lu_gfx.h"
#include <stdio.h>
#include <string.h>
int lu_headless_snapshot(const char *path);

#define NB 23
#define MAXF 64
typedef struct { char name[32]; float u; v3 j[NB]; } frame;
static frame F[MAXF]; static int NF;
static const int PAR[NB] = {-1, 0, 1, 2, 3, 4, 5, 4, 7, 8, 9, 4, 11, 12, 13, 1, 15, 16, 17, 1, 19, 20, 21};

static v3 cv(float x, float y, float z) { return V3(-y / 100.f, z / 100.f, x / 100.f); }
static lu_mesh cyl, sph;
static void limb(v3 a, v3 b, float r, lu_material *m) {
    v3 d = v3_sub(b, a); float l = v3_len(d); if (l < 1e-4f) return;
    quat q = quat_from_to(V3(0, 1, 0), v3_scale(d, 1.f / l));
    lu_draw(&cyl, m, 1, m4_trs(a, q, V3(r, l, r)));
    lu_draw(&sph, m, 1, m4_trs(a, Q_ID(), V3(r, r, r)));
    lu_draw(&sph, m, 1, m4_trs(b, Q_ID(), V3(r, r, r)));
}
static int wall_pose(const char *n) { return !strcmp(n, "hang") || !strcmp(n, "brace") || !strcmp(n, "climb"); }

int main(int argc, char **argv) {
    if (argc < 4) { printf("render_sheet poses.txt out.ppm side|34\n"); return 1; }
    FILE *fi = fopen(argv[1], "r"); if (!fi) return 1;
    while (NF < MAXF && fscanf(fi, "%31s %f", F[NF].name, &F[NF].u) == 2) {
        for (int i = 0; i < NB; i++) { float x, y, z; if (fscanf(fi, "%f %f %f", &x, &y, &z) != 3) return 2; F[NF].j[i] = cv(x, y, z); }
        NF++;
    }
    fclose(fi);
    int side = !strcmp(argv[3], "side");
    const int W = 2400, H = 1500, COLS = 13; const float SP = 1.25f, ROWH = 3.1f;
    lu_window_desc wd = {"sheet", W, H, 0, 0, 0};
    if (lu_platform_init(&wd)) return 3;
    if (lu_gfx_init()) { printf("ошибка: %s\n", lu_gfx_error()); return 4; }
    lu_gfx_set_quality(2);
    lu_mesh_builder b;
    lu_mb_init(&b); lu_mb_cylinder(&b, V3(0, 0, 0), 1, 1, 1, 14, 0); cyl = lu_mesh_create(&b, 0); lu_mb_free(&b);
    lu_mb_init(&b); lu_mb_sphere(&b, V3(0, 0, 0), V3(1, 1, 1), 16, 10); sph = lu_mesh_create(&b, 0); lu_mb_free(&b);
    lu_mesh box; lu_mb_init(&b); lu_mb_box(&b, V3(-.5f, -.5f, -.5f), V3(.5f, .5f, .5f), 1); box = lu_mesh_create(&b, 0); lu_mb_free(&b);
    lu_material mL = lu_material_default(); mL.color = V4(.25f, .45f, .85f, 1); mL.roughness = .6f;
    lu_material mR = lu_material_default(); mR.color = V4(.85f, .3f, .25f, 1); mR.roughness = .6f;
    lu_material mT = lu_material_default(); mT.color = V4(.75f, .72f, .66f, 1); mT.roughness = .7f;
    lu_material mG = lu_material_default(); mG.color = V4(.42f, .44f, .40f, 1); mG.roughness = .9f;
    lu_material mW = lu_material_default(); mW.color = V4(.62f, .55f, .45f, 1); mW.roughness = .9f;
    lu_env e = lu_env_default(); e.sun_dir = v3_norm(V3(.4f, .8f, .5f)); e.time = 3;
    /* раскладка: столбцы вдоль экрана, ряды друг над другом */
    v3 off[MAXF];
    for (int f = 0; f < NF; f++) {
        int c = f % COLS, r = f / COLS; float h = (c - (COLS - 1) * .5f) * SP, v = -r * ROWH;
        off[f] = side ? V3(0, v, h) : V3(h, v, 0);
    }
    float cx = 0, cy = -ROWH * ((NF - 1) / COLS) * .5f + 1.f;
    lu_camera cam;
    if (side) { cam = (lu_camera){V3(-60, cy + 2, 0), V3(0, cy, 0), V3(0, 1, 0), .19f, 1, 300}; }
    else { cam = (lu_camera){V3(-40, cy + 14, 42), V3(cx, cy, 0), V3(0, 1, 0), .20f, 1, 300}; }
    for (int pass = 0; pass < 2; pass++) {
        lu_gfx_begin(W, H, lu_platform_fbo()); lu_gfx_camera(&cam); lu_gfx_env(&e);
        for (int f = 0; f < NF; f++) {
            v3 o = off[f]; v3 *J = F[f].j; v3 P[NB];
            for (int i = 0; i < NB; i++) P[i] = v3_add(J[i], o);
            /* пол-плитка; у виса и лазания — стена 0.32 м впереди с уступом на 2.05 м */
            lu_draw(&box, &mG, 1, m4_trs(v3_add(o, V3(0, -.02f, .1f)), Q_ID(), V3(1.f, .04f, 1.1f)));
            if (side && wall_pose(F[f].name)) lu_draw(&box, &mW, 1, m4_trs(v3_add(o, V3(0, 1.0f, .32f + .3f)), Q_ID(), V3(1.f, 2.04f, .6f)));
            for (int i = 1; i < NB; i++) {
                int p = PAR[i]; if (p <= 0) continue;
                int left = (i >= 7 && i <= 10) || (i >= 15 && i <= 18), right = (i >= 11 && i <= 14) || (i >= 19 && i <= 22);
                float r = (i == 15 || i == 19) ? .065f : (i == 16 || i == 20) ? .05f : (i == 8 || i == 12) ? .045f : (i == 9 || i == 13) ? .038f : (i == 10 || i == 14) ? .03f : (i == 17 || i == 21) ? .04f : (i == 18 || i == 22) ? .035f : .09f;
                if (i == 7 || i == 11) r = .05f;
                if (i == 6) r = .06f;
                limb(P[p], P[i], r, left ? &mL : right ? &mR : &mT);
            }
            limb(P[15], P[19], .07f, &mT); /* таз */
            v3 hd = v3_add(P[6], v3_scale(v3_norm(v3_sub(P[6], P[5])), .1f));
            lu_draw(&sph, &mT, 1, m4_trs(hd, Q_ID(), V3(.11f, .13f, .11f)));
            /* нос — видно, куда смотрит голова */
            v3 fw = v3_norm(v3_sub(P[6], P[5])); v3 rx = v3_norm(v3_sub(P[11], P[7])); v3 fwd = v3_norm(v3_cross(rx, fw));
            lu_draw(&sph, &mR, 1, m4_trs(v3_add(hd, v3_scale(fwd, -.11f)), Q_ID(), V3(.03f, .03f, .03f)));
        }
        /* подписи над каждой фигурой: проекция точки над головой камерой (как в lu_camera: fovy по вертикали) */
        {
            v3 fz = v3_norm(v3_sub(cam.target, cam.pos)), rx = v3_norm(v3_cross(fz, cam.up)), uy = v3_cross(rx, fz);
            float th = tanf(cam.fovy * .5f);
            for (int f = 0; f < NF; f++) {
                v3 d = v3_sub(v3_add(off[f], V3(0, 2.35f, 0)), cam.pos);
                float z = v3_dot(d, fz); if (z < .1f) continue;
                float sx = W * .5f + v3_dot(d, rx) / (z * th) * H * .5f, sy = H * .5f - v3_dot(d, uy) / (z * th) * H * .5f;
                char t[48]; snprintf(t, sizeof t, "%s %.2f", F[f].name, F[f].u);
                lu_ui_text(sx - 6.5f * (float)strlen(t), sy, 20, LU_RGBA(20, 20, 20, 255), LU_TEXT_BOLD, t);
            }
        }
        lu_ui_text(20, H - 40, 22, LU_RGBA(20, 20, 20, 255), 0, side ? "вид сбоку: персонаж смотрит вправо; синий — левая сторона, красный — правая"
                                                                    : "вид 3/4 спереди; синий — левая сторона, красный — правая; красная точка — нос; стены у виса и лазания скрыты");
        lu_gfx_end(); lu_platform_swap();
    }
    lu_headless_snapshot(argv[2]);
    return 0;
}
