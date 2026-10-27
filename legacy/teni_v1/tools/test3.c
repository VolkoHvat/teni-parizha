#include "../engine/engine.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
void platform_quit(void){}
void platform_parallel(job_fn fn,void*arg,int n){for(int i=0;i<n;i++)fn(arg,i);}
void dbg2(int cmd,float a,float b,float c);int dbg_mstep(void);void dbg_pos(float*,float*,float*);
static void steps(int n){for(int i=0;i<n;i++){game_update(1.f/60);memset(key_hit,0,sizeof key_hit);}}
static void press(int k){key_hit[k]=1;steps(1);}
int main(){gfx_set_scale(1);game_init();steps(5);press(K_ENTER);steps(2);for(int i=0;i<5;i++)press(K_E);steps(10);printf("after intro: mstep=%d (1=CLIMB)\n",dbg_mstep());
 dbg2(1,31.2f,36,31.2f);steps(5);press(K_E);steps(2);for(int i=0;i<3;i++)press(K_E);steps(5);printf("after sync: mstep=%d (2=LEAP)\n",dbg_mstep());
 dbg2(1,26,14,33);steps(120);printf("after hay leap: mstep=%d (3=GOPOST)\n",dbg_mstep());
 dbg2(1,80,0,58);steps(5);for(int i=0;i<4;i++)press(K_E);steps(5);printf("after post: mstep=%d (4=TAIL)\n",dbg_mstep());
 dbg2(5,5,0,0);float rx,ry,rz;dbg_pos(&rx,&ry,&rz);dbg2(1,rx,ry,rz);steps(10);press(K_E);steps(3);printf("after letter: mstep=%d (6=ESCAPE1)\n",dbg_mstep());
 dbg2(5,7,0,0);dbg2(1,160,0,170);steps(5);press(K_E);steps(3);printf("after seal: mstep=%d (8=ESCAPE2)\n",dbg_mstep());
 dbg2(5,9,0,0);dbg2(1,82.5f,46,91.5f);steps(5);press(K_E);steps(2);for(int i=0;i<3;i++)press(K_E);steps(5);printf("end: mstep=%d (9 final then end screen)\n",dbg_mstep());
 return 0;}
