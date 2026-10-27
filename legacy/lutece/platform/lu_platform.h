/* Lutèce Engine — платформа: окно и контекст OpenGL 3.3, ввод (клавиатура, мышь, геймпад), звуковой вывод.
   Реализации: lu_platform_win32.c (Windows) и lu_platform_headless.c (Linux, EGL без окна — для тестов). */
#ifndef LU_PLATFORM_H
#define LU_PLATFORM_H
#include "../core/lu_core.h"

typedef struct {
    const char *title;      /* UTF-8 */
    int width, height;
    int fullscreen, vsync;
    int msaa;               /* не используется: сглаживание делает рендер */
} lu_window_desc;

int  lu_platform_init(const lu_window_desc *d);   /* 0 — успех */
void lu_platform_shutdown(void);
int  lu_platform_pump(void);                      /* обработать события; 0 — окно закрыто */
void lu_platform_swap(void);
void lu_platform_size(int *w, int *h);            /* размер области рисования в пикселях */
unsigned lu_platform_fbo(void);                   /* кадровый буфер «экрана» (0 в Windows) */
void lu_platform_fullscreen(int on);
int  lu_platform_is_fullscreen(void);
void lu_platform_title(const char *utf8);
void lu_platform_quit(void);
const char *lu_platform_gl_info(void);

/* ---------- ввод ---------- */
enum {
    LU_KEY_NONE = 0,
    LU_KEY_A = 'A', LU_KEY_Z = 'Z', LU_KEY_0 = '0', LU_KEY_9 = '9',
    LU_KEY_SPACE = 128, LU_KEY_ENTER, LU_KEY_ESC, LU_KEY_TAB, LU_KEY_BACKSPACE,
    LU_KEY_SHIFT, LU_KEY_CTRL, LU_KEY_ALT,
    LU_KEY_UP, LU_KEY_DOWN, LU_KEY_LEFT, LU_KEY_RIGHT,
    LU_KEY_F1, LU_KEY_F2, LU_KEY_F3, LU_KEY_F4, LU_KEY_F5, LU_KEY_F6, LU_KEY_F7, LU_KEY_F8, LU_KEY_F9, LU_KEY_F10, LU_KEY_F11, LU_KEY_F12,
    LU_KEY_GRAVE, LU_KEY_HOME, LU_KEY_END, LU_KEY_PGUP, LU_KEY_PGDN, LU_KEY_DELETE,
    LU_MOUSE_LEFT, LU_MOUSE_RIGHT, LU_MOUSE_MIDDLE,
    LU_KEY_N
};
/* кнопки геймпада (раскладка Xbox) */
enum { LU_PAD_A = 1, LU_PAD_B = 2, LU_PAD_X = 4, LU_PAD_Y = 8, LU_PAD_LB = 16, LU_PAD_RB = 32, LU_PAD_BACK = 64, LU_PAD_START = 128,
       LU_PAD_LS = 256, LU_PAD_RS = 512, LU_PAD_UP = 1024, LU_PAD_DOWN = 2048, LU_PAD_LEFT = 4096, LU_PAD_RIGHT = 8192 };
typedef struct {
    uint8_t down[LU_KEY_N], pressed[LU_KEY_N], released[LU_KEY_N];
    float mouse_dx, mouse_dy, wheel;  /* относительное движение за кадр */
    int mouse_x, mouse_y;             /* в пикселях окна */
    int mouse_captured;
    int pad_connected;
    float pad_lx, pad_ly, pad_rx, pad_ry, pad_lt, pad_rt; /* -1..1 / 0..1, с мёртвой зоной */
    uint32_t pad_down, pad_pressed;
    uint32_t text[32]; int ntext;     /* введённые символы (кодовые точки) */
} lu_input;
const lu_input *lu_input_get(void);
void lu_input_capture_mouse(int on);
void lu_input_inject(int key, int down);          /* для тестов и автопилота */

/* ---------- звук ---------- */
/* обратный вызов заполняет frames стереокадров (float L,R в -1..1); вызывается из звукового потока */
typedef void (*lu_audio_fn)(float *out, int frames, void *user);
int  lu_audio_start(int rate, lu_audio_fn fn, void *user);
void lu_audio_stop(void);
#endif
