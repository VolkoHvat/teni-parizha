#include "../engine/engine.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
void platform_quit(void){}
void platform_parallel(job_fn fn,void*arg,int n){for(int i=0;i<n;i++)fn(arg,i);}
void dbg2(int cmd,float a,float b,float c);
static void snap(const char*name){gfx_begin();game_render();char fn[128];snprintf(fn,sizeof fn,"/tmp/s2_%s.ppm",name);FILE*f=fopen(fn,"wb");fprintf(f,"P6\n%d %d\n255\n",fbw,fbh);for(int i=0;i<fbw*fbh;i++){fputc((fb[i]>>16)&255,f);fputc((fb[i]>>8)&255,f);fputc(fb[i]&255,f);}fclose(f);printf("snap %s\n",name);}
static void steps(int n){for(int i=0;i<n;i++){game_update(1.f/60);memset(key_hit,0,sizeof key_hit);}}
int main(int argc,char**argv){gfx_set_scale(argc>1?atoi(argv[1]):2);game_init();steps(10);snap("title");
 key_hit[K_ENTER]=1;steps(2);steps(5);snap("intro");
 key_hit[K_E]=1;steps(1);key_hit[K_E]=1;steps(1);key_hit[K_E]=1;steps(1);key_hit[K_E]=1;steps(1);steps(30);snap("street");
 dbg2(1,36,0,25);dbg2(3,3.14f,.3f,0);steps(30);snap("church_view");
 key_down[K_W]=1;steps(240);key_down[K_W]=0;snap("climbing");
 dbg2(1,60,0,40);dbg2(3,1.0f,.45f,0);steps(30);snap("plaza");
 dbg2(1,100,0,125);dbg2(3,3.14f,.25f,0);steps(30);snap("cathedral");
 dbg2(2,12.f,0,0);dbg2(1,31.5f,36,31.5f);dbg2(3,.8f,.5f,0);steps(30);snap("tower_top");
 return 0;}
