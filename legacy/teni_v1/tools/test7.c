#include "../engine/engine.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
void platform_quit(void){}
void platform_parallel(job_fn fn,void*arg,int n){for(int i=0;i<n;i++)fn(arg,i);}
void dbg2(int cmd,float a,float b,float c);int dbg_q(int);void dbg_bump_test(void);
static const char*OUT;
static void snap(const char*name){gfx_begin();game_render();char fn[512];snprintf(fn,sizeof fn,"%s/v_%s.ppm",OUT,name);FILE*f=fopen(fn,"wb");fprintf(f,"P6\n%d %d\n255\n",fbw,fbh);for(int i=0;i<fbw*fbh;i++){fputc((fb[i]>>16)&255,f);fputc((fb[i]>>8)&255,f);fputc(fb[i]&255,f);}fclose(f);printf("snap %s (тексты %d)\n",name,tqn);}
static void steps(int n){for(int i=0;i<n;i++){game_update(1.f/60);memset(key_hit,0,sizeof key_hit);}}
static void press(int k){key_hit[k]=1;steps(1);steps(2);}
int main(int c,char**v){OUT=v[1];gfx_set_scale(2);remove("teni_save.dat");game_init();steps(20);snap("title");
 press(K_ENTER);steps(5);snap("prologue");for(int i=0;i<5;i++)press(K_E);steps(30);
 dbg2(2,11.f,0,0);dbg2(1,40,0,141.5f);dbg2(3,0.2f,.28f,0);steps(60);snap("market");
 dbg2(1,80,0,58);dbg2(3,3.14f,.25f,0);steps(30);key_down[K_W]=1;key_down[K_SHIFT]=1;steps(10);dbg_bump_test();steps(14);key_down[K_W]=0;key_down[K_SHIFT]=0;snap("knockdown");steps(40);
 dbg2(5,0,0,0);dbg2(6,0,0,0);steps(60*3+40);key_down[K_W]=1;key_down[K_SHIFT]=1;steps(50);snap("race");key_down[K_W]=0;key_down[K_SHIFT]=0;
 dbg2(7,1,0,0);{int n=0;while(dbg_q(0)!=3&&n<60*60){steps(1);n++;}}dbg2(7,0,0,0);steps(40);snap("result");press(K_E);steps(5);
 dbg2(6,10,0,0);steps(60*3+60);snap("nighthunt");dbg2(7,1,0,0);steps(60*9);snap("nighthunt_roof");dbg2(7,0,0,0);
 dbg2(2,15.f,0,0);dbg2(9,1,0,0);dbg2(8,23,2,0);steps(3);snap("album_painter");dbg2(8,93,2,0);steps(3);snap("album_raven");dbg2(8,50,1,0);steps(3);snap("album_met");dbg2(8,77,2,0);steps(3);snap("album_noble");
 return 0;}
