#include "../engine/engine.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
void platform_quit(void){}
void platform_parallel(job_fn fn,void*arg,int n){for(int i=0;i<n;i++)fn(arg,i);}
void dbg2(int cmd,float a,float b,float c);extern double r3p_flush,r3p_draw;extern long r3p_tris,r3p_verts,r3p_test,r3p_shade;
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
static void steps(int n){for(int i=0;i<n;i++){game_update(1.f/60);memset(key_hit,0,sizeof key_hit);}}
int main(int c,char**v){int s=atoi(v[1]);gfx_set_scale(s);double t0=now();game_init();printf("загрузка %.2f с\n",now()-t0);steps(3);key_hit[K_ENTER]=1;steps(1);for(int i=0;i<5;i++){key_hit[K_E]=1;steps(2);}
 float spots[5][5]={{80,0,58,3.14f,.25f},{40,0,141.5f,.2f,.28f},{60,0,98,0,.3f},{109,17.3f,40,2.4f,.5f},{100,0,125,3.14f,.3f}};const char*nm[5]={"площадь","рынок","мост","крыша","набережная"};
 for(int k=0;k<5;k++){dbg2(1,spots[k][0],spots[k][1],spots[k][2]);dbg2(3,spots[k][3],spots[k][4],0);steps(30);
  r3p_flush=r3p_draw=0;r3p_tris=r3p_verts=0;r3p_test=r3p_shade=0;double tu=0,tr=0;for(int f=0;f<20;f++){double a=now();steps(1);double b=now();gfx_begin();game_render();double e=now();tu+=b-a;tr+=e-b;}
  printf("%-11s рендер %.1f мс: вершины %.1f мс (%ld тыс.), растр %.1f мс (%ld тыс. треуг.), прочее %.1f мс | пикселей проверено %.1fx, закрашено %.1fx экрана\n",nm[k],tr/20*1000,r3p_draw/20*1000,r3p_verts/20000,r3p_flush/20*1000,r3p_tris/20000,(tr-r3p_draw-r3p_flush)/20*1000,r3p_test/20.0/(fbw*fbh),r3p_shade/20.0/(fbw*fbh));}
 return 0;}
