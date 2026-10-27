/* Тестовый запуск на Linux: рисует кадры в PPM. */
#include "../engine/engine.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
void platform_quit(void){}
void platform_parallel(job_fn fn,void*arg,int n){for(int i=0;i<n;i++)fn(arg,i);}
void dbg_cmd(int cmd,int a,int b,int c);
static void snap(const char*name){gfx_begin();game_render();
 char fn[128];snprintf(fn,sizeof fn,"/tmp/snap_%s.ppm",name);FILE*f=fopen(fn,"wb");fprintf(f,"P6\n%d %d\n255\n",fbw,fbh);
 for(int i=0;i<fbw*fbh;i++){fputc((fb[i]>>16)&255,f);fputc((fb[i]>>8)&255,f);fputc(fb[i]&255,f);}fclose(f);printf("snap %s texts=%d\n",name,tqn);}
static void steps(int n){for(int i=0;i<n;i++){game_update(1.f/60);memset(key_hit,0,sizeof key_hit);}}
int main(int argc,char**argv){gfx_set_scale(argc>1?atoi(argv[1]):2);game_init();steps(30);snap("title");
 key_hit[K_OK]=1;steps(1);steps(10);key_hit[K_OK]=1;steps(1);steps(5);key_hit[K_OK]=1;steps(1);steps(60);snap("world_morning");
 dbg_cmd(1,1,12,0);steps(30);snap("world_noon");dbg_cmd(1,1,18,30);steps(30);snap("world_sunset");dbg_cmd(1,1,23,0);steps(30);snap("world_night");dbg_cmd(1,60,14,0);steps(30);snap("world_winter");
 dbg_cmd(2,1,1,10);steps(10);snap("lib_lena");dbg_cmd(2,3,1,40);steps(10);snap("pool_marta");dbg_cmd(2,5,2,70);steps(10);snap("cafe_kira");dbg_cmd(2,7,3,90);steps(10);snap("studio_iva");
 dbg_cmd(3,48,31,12);steps(30);snap("square");dbg_cmd(3,6,22,12);steps(30);snap("park");dbg_cmd(3,52,43,14);steps(30);snap("harbor");dbg_cmd(3,34,14,12);steps(30);snap("north");dbg_cmd(3,26,23.5f,19);steps(30);snap("bridge");
 return 0;}
