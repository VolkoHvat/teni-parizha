/* Проверка: ворота гонок, время, прохождение автопилотом */
#include "../engine/engine.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
void platform_quit(void){}
void platform_parallel(job_fn fn,void*arg,int n){for(int i=0;i<n;i++)fn(arg,i);}
void dbg2(int cmd,float a,float b,float c);int dbg_q(int);float dbg_f(int);const char*dbg_s(int);
extern void dbg_dump_race(int r);
static void steps(int n){for(int i=0;i<n;i++){game_update(1.f/60);memset(key_hit,0,sizeof key_hit);}}
int main(int argc,char**argv){clock_t c0=clock();gfx_set_scale(1);game_init();printf("init %.2f s\n",(double)(clock()-c0)/CLOCKS_PER_SEC);
 int only=argc>1?atoi(argv[1]):-1;int tries=argc>2?atoi(argv[2]):1;
 for(int r=0;r<12;r++){if(only>=0&&r!=only)continue;dbg_dump_race(r);
  for(int t=0;t<tries;t++){srand(100+t*7+r);dbg2(5,0,0,0);dbg2(6,(float)r,0,0);dbg2(7,1,0,0);steps(4*60);
   int n=0;float mh=100;int lastnext=-1;while(dbg_q(0)!=3&&n<60*240){steps(1);n++;if(dbg_f(7)<mh)mh=dbg_f(7);if(dbg_q(2)!=lastnext){lastnext=dbg_q(2);}if(n%(60*8)==0&&argc>3)printf("     t=%.0f pos=(%.1f,%.1f,%.1f) ворота=%d st=%d hp=%.0f\n",dbg_f(0),dbg_f(1),dbg_f(2),dbg_f(3),dbg_q(2),dbg_q(6),dbg_f(7));}
   if(!dbg_q(1))printf("     провал у (%.1f,%.1f,%.1f), ворота %d, мин.hp %.0f, последнее сообщение: %s\n",dbg_f(1),dbg_f(2),dbg_f(3),dbg_q(2),mh,dbg_s(1));
   printf("  попытка %d: итог=%s время=%.1f с (эталон %.1f, золото %.1f)%s%s медаль=%d hp=%.0f\n",t,dbg_q(1)?"ПОБЕДА":"провал",dbg_f(0),dbg_f(4),dbg_f(5),dbg_q(1)?"":" — ",dbg_q(1)?"":dbg_s(0),dbg_q(7),dbg_f(7));
   dbg2(7,0,0,0);key_hit[K_E]=1;steps(2);}}
 return 0;}
