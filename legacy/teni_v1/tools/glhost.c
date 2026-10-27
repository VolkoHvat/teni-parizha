/* Проверочный хост: настоящий OpenGL без окна (EGL + Mesa), кадры в PNG через PPM. Только для тестов на Linux. */
#include "../engine/engine.h"
#include "../engine/r3d.h"
#include "../engine/gl33.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <time.h>
void platform_parallel(job_fn fn,void*arg,int n){for(int i=0;i<n;i++)fn(arg,i);}
void platform_quit(void){}
void platform_draw_text(void){}
typedef void*EGLDisplay;typedef void*EGLConfig;typedef void*EGLContext;typedef int EGLint;typedef unsigned EGLBoolean;typedef unsigned EGLenum;
static void*(*egp)(const char*);
static void*getp(const char*n){void*p=egp(n);return p;}
static unsigned fbo,ctex,dtex;int host_w=1280,host_h=720;
extern PFN_glGenFramebuffers p_glGenFramebuffers;
int host_init(int w,int h){host_w=w;host_h=h;void*hd=dlopen("libEGL.so.1",RTLD_NOW);if(!hd){printf("нет EGL\n");return -1;}egp=dlsym(hd,"eglGetProcAddress");
 EGLDisplay(*GetPlat)(EGLenum,void*,const EGLint*)=dlsym(hd,"eglGetPlatformDisplay");EGLBoolean(*Init)(EGLDisplay,EGLint*,EGLint*)=dlsym(hd,"eglInitialize");
 EGLBoolean(*Choose)(EGLDisplay,const EGLint*,EGLConfig*,EGLint,EGLint*)=dlsym(hd,"eglChooseConfig");EGLBoolean(*Bind)(EGLenum)=dlsym(hd,"eglBindAPI");
 EGLContext(*Create)(EGLDisplay,EGLConfig,EGLContext,const EGLint*)=dlsym(hd,"eglCreateContext");EGLBoolean(*Make)(EGLDisplay,void*,void*,EGLContext)=dlsym(hd,"eglMakeCurrent");
 EGLDisplay d=GetPlat(0x31DD,0,0);EGLint ma,mi;if(!Init(d,&ma,&mi))return -2;EGLint at[]={0x3033,0,0x3040,8,0x3038};EGLConfig c;EGLint n;Choose(d,at,&c,1,&n);Bind(0x30A2);
 EGLint ca[]={0x3098,3,0x30FB,3,0x30FD,1,0x3038};EGLContext ctx=Create(d,n?c:0,0,ca);if(!ctx||!Make(d,0,0,ctx))return -3;
 const char*miss=gl_load(getp);if(miss){printf("нет функции %s\n",miss);return -4;}
 /* «окно» — своя рамка */
 p_glGenFramebuffers(1,&fbo);p_glBindFramebuffer(GL_FRAMEBUFFER,fbo);p_glGenTextures(1,&ctex);p_glBindTexture(GL_TEXTURE_2D,ctex);p_glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
 p_glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,ctex,0);p_glGenTextures(1,&dtex);p_glBindTexture(GL_TEXTURE_2D,dtex);p_glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,w,h,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_INT,NULL);
 p_glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,dtex,0);if(p_glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)return -5;
 r3_set_target_fbo(fbo);printf("GL: %s | %s\n",(const char*)p_glGetString(GL_RENDERER),(const char*)p_glGetString(GL_VERSION));
 if(r3_init()){printf("r3_init: %s\n",r3_error());return -6;}return 0;}
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
double host_frame(void){double a=now();r3_frame_begin(host_w,host_h);gfx_begin();game_render();platform_draw_text();r3_frame_end();p_glFinish();return now()-a;}
void host_snap(const char*path){uint8_t*rgb=malloc((size_t)host_w*host_h*3);r3_read_pixels(rgb,host_w,host_h);FILE*f=fopen(path,"wb");fprintf(f,"P6 %d %d 255\n",host_w,host_h);
 for(int y=host_h-1;y>=0;y--)fwrite(rgb+(size_t)y*host_w*3,1,(size_t)host_w*3,f);fclose(f);free(rgb);}
