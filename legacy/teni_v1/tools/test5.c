#include "../engine/engine.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
void platform_quit(void){}
void platform_parallel(job_fn fn,void*arg,int n){for(int i=0;i<n;i++)fn(arg,i);}
void dbg2(int cmd,float a,float b,float c);int dbg_q(int);float dbg_f(int);void dbg_thief(void);void dbg_tick(void);
static void steps(int n){for(int i=0;i<n;i++){game_update(1.f/60);memset(key_hit,0,sizeof key_hit);}}
int main(int c,char**v){gfx_set_scale(1);game_init();srand(5);dbg2(5,0,0,0);dbg2(6,(float)atoi(v[1]),0,0);dbg2(7,1,0,0);dbg_thief();steps(3*60+30);
 for(int s=0;s<40&&dbg_q(0)!=3;s++){dbg_tick();steps(60);}printf("итог %d\n",dbg_q(1));return 0;}
