/* Стресс-тест под ASan/UBSan: случайный ввод, телепорты, гонки, альбом, рендер */
#include "../engine/engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void platform_quit(void){}
void platform_parallel(job_fn fn,void*arg,int n){for(int i=0;i<n;i++)fn(arg,i);}
void dbg2(int cmd,float a,float b,float c);
int main(int argc,char**argv){gfx_set_scale(1);game_init();srand(argc>2?atoi(argv[2]):11);key_hit[K_ENTER]=1;game_update(1.f/60);memset(key_hit,0,sizeof key_hit);
 long N=argc>1?atol(argv[1]):60000;int16_t abuf[512*2];
 for(long f=0;f<N;f++){
  for(int k=0;k<K_N;k++){if(rand()%45==0)key_down[k]=!key_down[k];}key_down[K_ESC]=0;
  int k=rand()%K_N;if(rand()%9==0)key_hit[k]=1;if(k==K_ESC&&rand()%4)key_hit[k]=0;mouse_dx=rand()%31-15;mouse_dy=rand()%11-5;
  if(f%2500==1200)dbg2(1,(float)(rand()%196+2),(float)(rand()%30),(float)(rand()%196+2));
  if(f%4100==2000)dbg2(6,(float)(rand()%12),0,0);if(f%9000==4500)dbg2(5,0,0,0);
  if(f%5300==100)dbg2(8,(float)(rand()%128),(float)(rand()%3),0);if(f%1700==50)dbg2(2,(float)(rand()%24),0,0);
  if(f%3000==2999){dbg2(7,1,0,0);}if(f%3000==1500)dbg2(7,0,0,0);
  game_update(1.f/60);memset(key_hit,0,sizeof key_hit);
  if(f%173==0){gfx_begin();game_render();}
  if(f%40==0)game_audio(abuf,512);
  if(f%20000==0&&f)fprintf(stderr,"  %ld кадров\n",f);}
 printf("ok %ld\n",N);remove("teni_save.dat");return 0;}
