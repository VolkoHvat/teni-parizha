/* Lutèce Engine — «безоконная» платформа для Linux: настоящий OpenGL через EGL (Mesa), кадр рисуется в свой буфер.
   Используется автотестами и снимками кадров. Ввод — только через lu_input_inject, звук — ручной вызов lu_headless_audio. */
#include "lu_platform.h"
#include "../gfx/lu_gl.h"
#include <dlfcn.h>
#include <stdio.h>

typedef void *EGLDisplay; typedef void *EGLConfig; typedef void *EGLContext; typedef int EGLint; typedef unsigned EGLBoolean; typedef unsigned EGLenum;
static void *(*egp)(const char *);
static void *getp(const char *n) { return egp(n); }
static unsigned fbo, ctex, dtex; static int W = 1280, H = 720, running;
static lu_input in; static char gl_info[256];
static lu_audio_fn a_fn; static void *a_user;

int lu_platform_init(const lu_window_desc *d) {
    W = d->width > 0 ? d->width : 1280; H = d->height > 0 ? d->height : 720;
    void *hd = dlopen("libEGL.so.1", RTLD_NOW); if (!hd) { LU_ERR("нет libEGL"); return 1; }
    egp = (void *(*)(const char *))dlsym(hd, "eglGetProcAddress");
    EGLDisplay (*GetPlat)(EGLenum, void *, const EGLint *) = (EGLDisplay(*)(EGLenum, void *, const EGLint *))dlsym(hd, "eglGetPlatformDisplay");
    EGLBoolean (*Init)(EGLDisplay, EGLint *, EGLint *) = (EGLBoolean(*)(EGLDisplay, EGLint *, EGLint *))dlsym(hd, "eglInitialize");
    EGLBoolean (*Choose)(EGLDisplay, const EGLint *, EGLConfig *, EGLint, EGLint *) = (EGLBoolean(*)(EGLDisplay, const EGLint *, EGLConfig *, EGLint, EGLint *))dlsym(hd, "eglChooseConfig");
    EGLBoolean (*Bind)(EGLenum) = (EGLBoolean(*)(EGLenum))dlsym(hd, "eglBindAPI");
    EGLContext (*Create)(EGLDisplay, EGLConfig, EGLContext, const EGLint *) = (EGLContext(*)(EGLDisplay, EGLConfig, EGLContext, const EGLint *))dlsym(hd, "eglCreateContext");
    EGLBoolean (*Make)(EGLDisplay, void *, void *, EGLContext) = (EGLBoolean(*)(EGLDisplay, void *, void *, EGLContext))dlsym(hd, "eglMakeCurrent");
    EGLDisplay dp = GetPlat(0x31DD /* SURFACELESS_MESA */, 0, 0); EGLint ma, mi;
    if (!Init(dp, &ma, &mi)) return 2;
    EGLint at[] = {0x3033, 0, 0x3040, 8, 0x3038}; EGLConfig c; EGLint n = 0; Choose(dp, at, &c, 1, &n); Bind(0x30A2);
    EGLint ca[] = {0x3098, 3, 0x30FB, 3, 0x30FD, 1, 0x3038};
    EGLContext ctx = Create(dp, n ? c : 0, 0, ca); if (!ctx || !Make(dp, 0, 0, ctx)) return 3;
    const char *miss = lu_gl_load(getp); if (miss) { LU_ERR("нет функции GL %s", miss); return 4; }
    glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenTextures(1, &ctex); glBindTexture(GL_TEXTURE_2D, ctex); glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctex, 0);
    glGenTextures(1, &dtex); glBindTexture(GL_TEXTURE_2D, dtex); glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, W, H, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, dtex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) return 5;
    snprintf(gl_info, sizeof gl_info, "%s | %s", (const char *)glGetString(GL_RENDERER), (const char *)glGetString(GL_VERSION));
    running = 1; LU_INFO("GL: %s", gl_info);
    return 0;
}
void lu_platform_shutdown(void) {}
int lu_platform_pump(void) {
    memset(in.pressed, 0, sizeof in.pressed); memset(in.released, 0, sizeof in.released);
    in.mouse_dx = in.mouse_dy = in.wheel = 0; in.ntext = 0; in.pad_pressed = 0; return running;
}
void lu_platform_swap(void) { glFinish(); }
void lu_platform_size(int *w, int *h) { *w = W; *h = H; }
unsigned lu_platform_fbo(void) { return fbo; }
void lu_platform_fullscreen(int on) { LU_UNUSED(on); }
int lu_platform_is_fullscreen(void) { return 0; }
void lu_platform_title(const char *utf8) { LU_UNUSED(utf8); }
void lu_platform_quit(void) { running = 0; }
const char *lu_platform_gl_info(void) { return gl_info; }
const lu_input *lu_input_get(void) { return &in; }
void lu_input_capture_mouse(int on) { in.mouse_captured = on; }
void lu_input_inject(int key, int down) {
    if (key <= 0 || key >= LU_KEY_N) return;
    if (down && !in.down[key]) in.pressed[key] = 1;
    if (!down && in.down[key]) in.released[key] = 1;
    in.down[key] = (uint8_t)down;
}
void lu_headless_mouse(float dx, float dy) { in.mouse_dx += dx; in.mouse_dy += dy; }
int lu_audio_start(int rate, lu_audio_fn fn, void *user) { LU_UNUSED(rate); a_fn = fn; a_user = user; return 0; }
void lu_audio_stop(void) { a_fn = NULL; }
void lu_headless_audio(float *out, int frames) { memset(out, 0, sizeof(float) * 2 * (size_t)frames); if (a_fn) a_fn(out, frames, a_user); }
/* сохранить текущий кадр в PPM (снизу вверх → сверху вниз) */
int lu_headless_snapshot(const char *path) {
    uint8_t *rgb = lu_alloc((size_t)W * H * 3);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo); glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, rgb);
    FILE *f = fopen(path, "wb"); if (!f) { lu_free(rgb); return 0; }
    fprintf(f, "P6 %d %d 255\n", W, H);
    for (int y = H - 1; y >= 0; y--) fwrite(rgb + (size_t)y * W * 3, 1, (size_t)W * 3, f);
    fclose(f); lu_free(rgb); return 1;
}
