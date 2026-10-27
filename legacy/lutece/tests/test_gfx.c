/* Проверка рендера Lutèce без окна: сцена с тенями, инстансами, источниками света, текстом. Сохраняет PPM. */
#include "../platform/lu_platform.h"
#include "../gfx/lu_gfx.h"
#include <stdio.h>
int lu_headless_snapshot(const char *path);
static lu_texture checker(int n, uint32_t a, uint32_t b) {
    uint32_t *px = lu_alloc(sizeof(uint32_t) * 256 * 256);
    for (int y = 0; y < 256; y++) for (int x = 0; x < 256; x++) px[y * 256 + x] = (((x / (256 / n)) + (y / (256 / n))) & 1) ? a : b;
    lu_texture t = lu_texture_create(256, 256, px, LU_TEX_SRGB | LU_TEX_MIPS); lu_free(px); return t;
}
int main(int argc, char **argv) {
    const char *out = argc > 1 ? argv[1] : "gfx.ppm"; float tod = argc > 2 ? (float)atof(argv[2]) : 15.f;
    lu_window_desc wd = {"test", 1280, 720, 0, 0, 0};
    if (lu_platform_init(&wd)) return 1;
    if (lu_gfx_init()) { printf("ошибка: %s\n", lu_gfx_error()); return 2; }
    lu_gfx_set_quality(2);
    lu_mesh_builder b; lu_mb_init(&b);
    lu_mb_box(&b, V3(-60, -1, -60), V3(60, 0, 60), .5f); lu_mb_compute_tangents(&b);
    lu_mesh ground = lu_mesh_create(&b, 0); lu_mb_free(&b);
    lu_mb_init(&b); lu_mb_box(&b, V3(-.5f, 0, -.5f), V3(.5f, 1, .5f), 1); lu_mesh cube = lu_mesh_create(&b, 0); lu_mb_free(&b);
    lu_mb_init(&b); lu_mb_sphere(&b, V3(0, 0, 0), V3(1, 1, 1), 32, 20); lu_mesh sph = lu_mesh_create(&b, 0); lu_mb_free(&b);
    lu_mb_init(&b); lu_mb_cylinder(&b, V3(0, 0, 0), .4f, .35f, 6, 20, 1); lu_mesh col = lu_mesh_create(&b, 0); lu_mb_free(&b);
    lu_texture chk = checker(8, 0xFF8C9AA6u, 0xFF6C7882u), stone = checker(16, 0xFFB8C8D4u, 0xFFA8B8C4u);
    lu_material mground = lu_material_default(); mground.albedo = &chk; mground.roughness = .85f;
    lu_material mstone = lu_material_default(); mstone.albedo = &stone; mstone.uv_scale = .5f;
    lu_material mgold = lu_material_default(); mgold.color = V4(1, .78f, .35f, 1); mgold.metallic = 1; mgold.roughness = .3f;
    lu_material mred = lu_material_default(); mred.color = V4(.7f, .08f, .06f, 1); mred.roughness = .5f;
    lu_material mglow = lu_material_default(); mglow.color = V4(1, .8f, .5f, 1); mglow.emissive = V3(6, 4, 1.5f); mglow.flags = LU_MAT_UNLIT;
    lu_material mglass = lu_material_default(); mglass.color = V4(.5f, .7f, 1, .35f); mglass.flags = LU_MAT_BLEND; mglass.roughness = .05f;
    lu_env e = lu_env_default();
    float ang = (tod - 6) / 12.f * LU_PI; e.sun_dir = v3_norm(V3(-cosf(ang) * .7f, sinf(ang), .45f));
    if (tod > 19 || tod < 5) { e.night = 1; e.sun_color = V3(.05f, .07f, .12f); e.sky_zenith = V3(.005f, .008f, .02f); e.sky_horizon = V3(.02f, .03f, .06f); e.exposure = 2.2f; e.fog_color = V3(.02f, .025f, .04f); e.sun_dir = v3_norm(V3(.3f, .6f, -.5f)); }
    e.time = 3;
    double t0 = lu_time(); int frames = argc > 3 ? atoi(argv[3]) : 2;
    for (int f = 0; f < frames; f++) {
        lu_gfx_begin(1280, 720, lu_platform_fbo());
        lu_camera c = {V3(14, 7, 16), V3(0, 2, 0), V3(0, 1, 0), 1.0f, .1f, 500.f}; lu_gfx_camera(&c); lu_gfx_env(&e);
        lu_draw(&ground, &mground, 1, m4_id());
        for (int i = 0; i < 400; i++) { float x = (i % 20 - 10) * 4.f + 2, z = (i / 20 - 10) * 4.f - 30; lu_draw(&col, &mstone, 1, m4_translate(V3(x, 0, z))); }
        lu_draw(&cube, &mred, 1, m4_trs(V3(0, 0, 0), quat_axis(V3(0, 1, 0), .5f), V3(3, 3, 3)));
        lu_draw(&sph, &mgold, 1, m4_trs(V3(4, 1.5f, 3), Q_ID(), V3(1.5f, 1.5f, 1.5f)));
        lu_draw(&sph, &mglass, 1, m4_trs(V3(-3, 1.2f, 5), Q_ID(), V3(1.2f, 1.2f, 1.2f)));
        lu_draw(&sph, &mglow, 1, m4_trs(V3(-6, 3, 0), Q_ID(), V3(.3f, .3f, .3f)));
        lu_light(V3(-6, 3, 0), 12, V3(8, 5, 2));
        lu_debug_box((aabb){V3(-1.6f, 0, -1.6f), V3(1.6f, 3.2f, 1.6f)}, 0xFF00FFFFu);
        lu_ui_rect(20, 20, 420, 92, LU_RGBA(10, 8, 20, 190));
        lu_ui_text(34, 28, 30, LU_RGBA(244, 214, 130, 255), LU_TEXT_SHADOW | LU_TEXT_BOLD, "Lutèce — движок");
        lu_ui_text(34, 66, 20, LU_RGBA(236, 230, 220, 255), 0, "Тени, PBR, SSAO, блум, инстансы «400 колонн»");
        lu_ui_circle(1200, 640, 40, LU_RGBA(200, 60, 50, 220));
        lu_gfx_end(); lu_platform_swap();
    }
    double dt = (lu_time() - t0) / frames;
    lu_gfx_stats s = lu_gfx_get_stats();
    printf("кадр %.1f мс (llvmpipe), вызовов %d, инстансов %d, треугольников %d, теневых вызовов %d\n", dt * 1000, s.draws, s.instances, s.tris, s.shadow_draws);
    lu_headless_snapshot(out);
    return 0;
}
