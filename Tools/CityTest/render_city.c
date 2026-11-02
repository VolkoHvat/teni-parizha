/* Тени Парижа — превью квартала: читает геометрию из dump_scene и рисует её рендером Lutèce без окна (EGL/Mesa)
   с процедурными материалами города (lu_city_materials). Запуск: render_city scene.bin out.ppm вид [час]
   Виды ряда домов: street (с тротуара), elev (фасады прямо), win (окна вблизи), door (первый этаж), corner (угол снизу),
   roof (сверху на карнизы), balc (балкон), dormer (слуховые окна); квартала: level (сверху), lstreet (улица между
   кварталами с уровня глаз), lsquare (площадь с фонтаном), lroof (с крыши на площадь), lalley (переулок).
   Пятый аргумент — файл текстур из dump_tex (TPTexGen, как в игре, с выветриванием) вместо текстур Lutèce. */
#include "platform/lu_platform.h"
#include "gfx/lu_gfx.h"
#include "world/lu_city.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int lu_headless_snapshot(const char *path);

#define MAXM 4096
static lu_mesh MESH[MAXM]; static int MAT[MAXM], NM;

int main(int argc, char **argv) {
    if (argc < 4) { printf("render_city scene.bin out.ppm вид [час] [текстуры.bin]\n"); return 1; }
    const char *view = argv[3]; float tod = argc > 4 ? (float)atof(argv[4]) : 15.5f;
    const int W = 1600, H = 1000;
    lu_window_desc wd = {"city", W, H, 0, 0, 0};
    if (lu_platform_init(&wd)) return 3;
    if (lu_gfx_init()) { printf("ошибка: %s\n", lu_gfx_error()); return 4; }
    lu_gfx_set_quality(3);
    FILE *f = fopen(argv[1], "rb"); if (!f) return 2;
    long tris = 0;
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
        tris += hdr[2] / 3;
        lu_mb_compute_tangents(&b);
        if (NM < MAXM) { MESH[NM] = lu_mesh_create(&b, 0); MAT[NM] = hdr[0]; NM++; }
        lu_mb_free(&b);
    }
    fclose(f);
    static lu_material mats[LU_CM_N]; static lu_texture tex[LU_CM_N][3];
    lu_city_materials(mats, tex, 512);
    if (argc > 5) {   /* текстуры TPTexGen: [материал, размер] + альбедо, нормали, ORM (RGBA8) */
        FILE *tf = fopen(argv[5], "rb"); if (!tf) return 7;
        static lu_texture ttex[LU_CM_N][3]; int hdr[2];
        while (fread(hdr, 4, 2, tf) == 2) {
            const int n = hdr[1], m = hdr[0]; unsigned char *px = malloc((size_t)n * n * 4);
            for (int k = 0; k < 3; k++) {
                if (fread(px, 4, (size_t)n * n, tf) != (size_t)n * n) return 8;
                ttex[m][k] = lu_texture_create(n, n, px, k == 0 ? (LU_TEX_SRGB | LU_TEX_MIPS) : LU_TEX_MIPS);
            }
            free(px);
            if (m >= 0 && m < LU_CM_N && m != LU_CM_LEAVES) {   /* листва: в Lutèce нет отсечения по альфе — оставляем её материал */
                 mats[m].albedo = &ttex[m][0]; mats[m].normal = &ttex[m][1]; mats[m].orm = &ttex[m][2]; mats[m].roughness = 1; mats[m].metallic = 1; mats[m].flags |= LU_MAT_VCOLOR; }   /* в UE базовый материал всегда умножает на цвет вершин */
        }
        fclose(tf);
    }
    lu_env e = lu_env_default();
    float ang = (tod - 6) / 12.f * LU_PI; e.sun_dir = v3_norm(V3(-cosf(ang) * .7f, sinf(ang), -.55f)); e.time = 3;
    /* камеры (улица — со стороны −Z, дома от x = 0 до 61, глубина 10 м) */
    lu_camera cam = {V3(-6, 1.7f, -7), V3(14, 7, 2), V3(0, 1, 0), 1.0f, .1f, 600.f};
    if (!strcmp(view, "elev")) cam = (lu_camera){V3(30.5f, 9, -95), V3(30.5f, 9, 0), V3(0, 1, 0), .37f, 1.f, 600.f};
    else if (!strcmp(view, "win")) cam = (lu_camera){V3(3.5f, 7.5f, -4.5f), V3(5.5f, 8.f, 0), V3(0, 1, 0), .75f, .1f, 300.f};
    else if (!strcmp(view, "door")) cam = (lu_camera){V3(1.5f, 1.7f, -6.5f), V3(7.f, 2.6f, 0), V3(0, 1, 0), .9f, .1f, 300.f};
    else if (!strcmp(view, "corner")) cam = (lu_camera){V3(66.f, 1.7f, -5.f), V3(60.5f, 9.f, 1.f), V3(0, 1, 0), 1.0f, .1f, 300.f};
    else if (!strcmp(view, "roof")) cam = (lu_camera){V3(68.f, 24.f, -6.f), V3(57.f, 17.f, 3.f), V3(0, 1, 0), .9f, .1f, 300.f};
    else if (!strcmp(view, "shop")) cam = (lu_camera){V3(46.f, 1.7f, -6.f), V3(53.f, 2.5f, 0), V3(0, 1, 0), .9f, .1f, 300.f};
    else if (!strcmp(view, "balc")) cam = (lu_camera){V3(3.f, 6.5f, -4.f), V3(6.5f, 4.6f, 0.5f), V3(0, 1, 0), .8f, .1f, 300.f};
    else if (!strcmp(view, "dormer")) cam = (lu_camera){V3(8.f, 21.f, -5.f), V3(5.f, 17.3f, 0.5f), V3(0, 1, 0), .8f, .1f, 300.f};
    else if (!strcmp(view, "lstreet")) cam = (lu_camera){V3(35.2f, 1.75f, -6.f), V3(33.5f, 4.2f, 14.f), V3(0, 1, 0), 1.0f, .1f, 400.f};
    else if (!strcmp(view, "lsquare")) cam = (lu_camera){V3(52.f, 1.75f, 49.f), V3(67.f, 2.6f, 54.f), V3(0, 1, 0), 1.0f, .1f, 400.f};
    else if (!strcmp(view, "lroof")) cam = (lu_camera){V3(22.f, 21.f, 22.f), V3(60.f, 4.f, 52.f), V3(0, 1, 0), 1.0f, .1f, 600.f};
    else if (!strcmp(view, "lalley")) cam = (lu_camera){V3(15.f, 1.75f, -3.5f), V3(15.f, 3.5f, 13.f), V3(0, 1, 0), 1.0f, .1f, 400.f};
    else if (!strcmp(view, "level")) cam = (lu_camera){V3(-25, 40, -35), V3(55, 0, 50), V3(0, 1, 0), .9f, .5f, 900.f};
    for (int pass = 0; pass < 2; pass++) {
        lu_gfx_begin(W, H, lu_platform_fbo()); lu_gfx_camera(&cam); lu_gfx_env(&e);
        for (int i = 0; i < NM; i++) lu_draw(&MESH[i], &mats[MAT[i]], 1, m4_id());
        char t[96]; snprintf(t, sizeof t, "%s: %ld треугольников", view, tris);
        lu_ui_text(16, H - 36, 22, LU_RGBA(255, 255, 255, 255), LU_TEXT_SHADOW, t);
        lu_gfx_end(); lu_platform_swap();
    }
    lu_headless_snapshot(argv[2]);
    return 0;
}
