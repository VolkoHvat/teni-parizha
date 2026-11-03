/* Тени Парижа — превью одежды: читает меши из outfit_test --dump и рисует рендером Lutèce без окна (EGL/Mesa).
   Запуск: render_outfit scene.bin labels.txt out.ppm ex ey ez tx ty tz fovy [ширина высота]
   Координаты Lutèce (метры): x = -Y, y = Z, z = X пространства персонажа UE (персонаж смотрит вдоль +z).
   Как в UE, материал умножается на цвет вершин; секция 0 — ткань (шероховатая), 1 — гладкое. */
#include "platform/lu_platform.h"
#include "gfx/lu_gfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int lu_headless_snapshot(const char *path);

#define MAXM 4096
static lu_mesh MESH[MAXM]; static int SEC[MAXM], NM;

int main(int argc, char **argv) {
    if (argc < 11) { printf("render_outfit scene.bin labels.txt out.ppm ex ey ez tx ty tz fovy [w h]\n"); return 1; }
    const int W = argc > 12 ? atoi(argv[11]) : 2000, H = argc > 12 ? atoi(argv[12]) : 1100;
    lu_window_desc wd = {"outfit", W, H, 0, 0, 0};
    if (lu_platform_init(&wd)) return 3;
    if (lu_gfx_init()) { printf("ошибка: %s\n", lu_gfx_error()); return 4; }
    lu_gfx_set_quality(3);
    FILE *f = fopen(argv[1], "rb"); if (!f) return 2;
    for (;;) {
        int hdr[3]; if (fread(hdr, 4, 3, f) != 3) break;
        lu_mesh_builder b; lu_mb_init(&b);
        for (int i = 0; i < hdr[1]; i++) {
            float d[8]; unsigned char c[4];
            if (fread(d, 4, 8, f) != 8 || fread(c, 1, 4, f) != 4) return 5;
            b.color = LU_RGBA(c[0], c[1], c[2], c[3]);
            lu_mb_vertex(&b, V3(d[0], d[1], d[2]), V3(d[3], d[4], d[5]), V2(d[6], d[7]));
        }
        for (int i = 0; i < hdr[2]; i += 3) {
            int t[3]; if (fread(t, 4, 3, f) != 3) return 6;
            lu_mb_tri(&b, t[0], t[1], t[2]);
        }
        lu_mb_compute_tangents(&b);
        if (NM < MAXM) { MESH[NM] = lu_mesh_create(&b, 0); SEC[NM] = hdr[0]; NM++; }
        lu_mb_free(&b);
    }
    fclose(f);
    lu_material mc = lu_material_default(); mc.color = V4(1, 1, 1, 1); mc.roughness = .85f; mc.flags |= LU_MAT_VCOLOR;
    lu_material ms = mc; ms.roughness = .45f;
    lu_material mg = lu_material_default(); mg.color = V4(.42f, .42f, .40f, 1); mg.roughness = .95f;
    lu_mesh box; { lu_mesh_builder b; lu_mb_init(&b); lu_mb_box(&b, V3(-.5f, -.5f, -.5f), V3(.5f, .5f, .5f), 1); box = lu_mesh_create(&b, 0); lu_mb_free(&b); }
    lu_env e = lu_env_default(); e.sun_dir = v3_norm(V3(-.45f, .75f, .55f)); e.time = 3;
    lu_camera cam = {V3((float)atof(argv[4]), (float)atof(argv[5]), (float)atof(argv[6])),
                     V3((float)atof(argv[7]), (float)atof(argv[8]), (float)atof(argv[9])), V3(0, 1, 0), (float)atof(argv[10]), .05f, 200.f};
    for (int pass = 0; pass < 2; pass++) {
        lu_gfx_begin(W, H, lu_platform_fbo()); lu_gfx_camera(&cam); lu_gfx_env(&e);
        lu_draw(&box, &mg, 1, m4_trs(V3(0, -.05f, 0), Q_ID(), V3(30.f, .1f, 30.f)));
        for (int i = 0; i < NM; i++) lu_draw(&MESH[i], SEC[i] ? &ms : &mc, 1, m4_trs(V3(0, 0, 0), Q_ID(), V3(1, 1, 1)));
        /* подписи: проекция точки камерой */
        FILE *tf = fopen(argv[2], "r");
        if (tf) {
            v3 fz = v3_norm(v3_sub(cam.target, cam.pos)), rx = v3_norm(v3_cross(fz, cam.up)), uy = v3_cross(rx, fz);
            float th = tanf(cam.fovy * .5f); float x, y, z; char t[128];
            while (fscanf(tf, "%f %f %f %127[^\n]", &x, &y, &z, t) == 4) {
                v3 d = v3_sub(V3(x, y, z), cam.pos); float dz = v3_dot(d, fz); if (dz < .1f) continue;
                float sx = W * .5f + v3_dot(d, rx) / (dz * th) * H * .5f, sy = H * .5f - v3_dot(d, uy) / (dz * th) * H * .5f;
                lu_ui_text(sx - 6.f * (float)strlen(t) * .5f, sy, 20, LU_RGBA(20, 20, 20, 255), LU_TEXT_BOLD, t);
            }
            fclose(tf);
        }
        lu_gfx_end(); lu_platform_swap();
    }
    lu_headless_snapshot(argv[3]);
    return 0;
}
