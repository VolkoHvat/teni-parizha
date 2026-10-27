/* Сюжет от начала до конца: пролог, главы, финал, 128 портретов, сохранение и загрузка */
#include "../engine/engine.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
void platform_quit(void){}
void platform_parallel(job_fn fn,void*arg,int n){for(int i=0;i<n;i++)fn(arg,i);}
void dbg2(int cmd,float a,float b,float c);int dbg_q(int);float dbg_f(int);
static void steps(int n){for(int i=0;i<n;i++){game_update(1.f/60);memset(key_hit,0,sizeof key_hit);}}
static void press(int k){key_hit[k]=1;steps(1);steps(2);}
int main(){gfx_set_scale(1);remove("teni_save.dat");game_init();steps(3);
 press(K_ENTER); /* «Новая игра» (сохранения нет) */
 printf("после старта: режим=%d глава=%d портретов=%d\n",dbg_q(5),dbg_q(3),dbg_q(4));
 for(int i=0;i<5;i++)press(K_E);printf("после пролога: режим=%d глава=%d портретов=%d\n",dbg_q(5),dbg_q(3),dbg_q(4));
 int order[12]={0,1,2,3,4,5,6,7,8,9,10,11};
 for(int pass=0;pass<2;pass++)for(int q=0;q<12;q++){int r=order[q];
  for(int att=0;att<4;att++){dbg2(6,(float)r,0,0);dbg2(7,1,0,0);int n=0;while(dbg_q(0)!=3&&n<60*200){steps(1);n++;}dbg2(7,0,0,0);int ok=dbg_q(1),m=dbg_q(7);
   steps(40);press(K_E);/* закрыть итоги */steps(3); for(int k=0;k<6&&dbg_q(5)==3;k++)press(K_E);/* диалоги глав */
   if(ok&&m==3)break;}
  printf("проход %d гонка %2d: глава=%d портретов=%3d режим=%d\n",pass,r,dbg_q(3),dbg_q(4),dbg_q(5));}
 /* сохранение и загрузка */
 int before=dbg_q(4),ch=dbg_q(3);press(K_ESC);for(int i=0;i<3;i++)press(K_DOWN);press(K_E);/* «Сохранить и в главное меню» */
 printf("в меню: режим=%d\n",dbg_q(5));game_init();steps(3);press(K_ENTER);/* «Продолжить» */
 printf("после загрузки: режим=%d глава=%d портретов=%d (было %d, глава %d)\n",dbg_q(5),dbg_q(3),dbg_q(4),before,ch);
 remove("teni_save.dat");return 0;}
