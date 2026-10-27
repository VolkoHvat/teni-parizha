/* Движок «Тени Парижа»: 3D на видеокарте (OpenGL 3.3), 2D-интерфейс рисуется программно в прозрачный слой.
   Логическое разрешение интерфейса 640x360, внутреннее — в rs раз больше. */
#ifndef ENGINE_H
#define ENGINE_H
#include <stdint.h>
#define SW 640
#define SH 360
#define FBW_MAX 2560
#define FBH_MAX 1440
#define RGBX(r,g,b) ((uint32_t)((((r)&255)<<16)|(((g)&255)<<8)|((b)&255)))
/* слой интерфейса: ARGB с предумноженной альфой; пустые пиксели = 0 */
extern uint32_t fb[FBW_MAX*FBH_MAX];
extern int fbw,fbh,rs;
extern int hud_y0,hud_y1;           /* строки, в которые что-то рисовали в этом кадре */
typedef struct{int x,y,w,h,size,flags;uint32_t col;char s[480];}TextCmd;
#define TQ_MAX 420
extern TextCmd tq[TQ_MAX]; extern int tqn;
enum{TF_CENTER=1,TF_BOLD=2,TF_RIGHT=4,TF_NOSHADOW=8};
enum{K_W,K_A,K_S,K_D,K_UP,K_DOWN,K_LEFT,K_RIGHT,K_SPACE,K_SHIFT,K_CTRL,K_E,K_F,K_Q,K_R,K_TAB,K_ENTER,K_ESC,K_N};
extern int key_down[K_N],key_hit[K_N];
extern int mouse_dx,mouse_dy,mouse_want;
extern float g_time;
void gfx_set_scale(int s);
uint32_t gfx_lerp(uint32_t a,uint32_t b,float t);
void gfx_begin(void);
void gfx_target(uint32_t*buf,int w,int h);
void gfx_target_s(uint32_t*buf,int w,int h,float scale);
void gfx_target_screen(void);
void gfx_px(int x,int y,uint32_t c,int a);
void gfx_rect(int x,int y,int w,int h,uint32_t c,int a);
void gfx_vgrad(int x,int y,int w,int h,uint32_t top,uint32_t bot);
void gfx_circle(float cx,float cy,float r,uint32_t c,int a);
void gfx_ring(float cx,float cy,float r,float th,uint32_t c,int a);
void gfx_ellipse(float cx,float cy,float rx,float ry,uint32_t c,int a);
void gfx_line(float x0,float y0,float x1,float y1,float th,uint32_t c,int a);
void gfx_poly(const float*p,int n,uint32_t c,int a);
void gfx_tri(float x0,float y0,float x1,float y1,float x2,float y2,uint32_t c,int a);
void gfx_quad(float x0,float y0,float x1,float y1,float x2,float y2,float x3,float y3,uint32_t c,int a);
void gfx_glow(float cx,float cy,float r,uint32_t c,int a);
void gfx_text(int x,int y,int w,int h,int size,uint32_t col,int flags,const char*s);
/* текст: платформа растеризует очередь tq в слой интерфейса (Windows — GDI) */
void platform_draw_text(void);
typedef void (*job_fn)(void*,int);
void platform_parallel(job_fn fn,void*arg,int n);
void game_init(void); void game_update(float dt); void game_render(void);
void platform_quit(void);
void game_audio(int16_t*out,int frames);
/* настройки графики (F2): 0 низкие … 3 ультра */
extern int gfx_quality;
extern float g_fps;
#endif
