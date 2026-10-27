#include "../engine/engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void platform_quit(void){}
void platform_parallel(job_fn fn,void*arg,int n){for(int i=0;i<n;i++)fn(arg,i);}
void dbg2(int cmd,float a,float b,float c);
int main(int argc,char**argv){gfx_set_scale(1);game_init();srand(11);key_hit[K_ENTER]=1;game_update(1.f/60);memset(key_hit,0,sizeof key_hit);
 long N=argc>1?atol(argv[1]):150000;
 for(long f=0;f<N;f++){
  for(int k=0;k<K_N;k++){if(rand()%50==0)key_down[k]=!key_down[k];}key_down[K_ESC]=0;
  int k=rand()%K_N;if(rand()%10==0&&k!=K_ESC)key_hit[k]=1;mouse_dx=rand()%21-10;mouse_dy=rand()%11-5;
  if(f%3000==1500)dbg2(1,(float)(rand()%190+5),(float)(rand()%40),(float)(rand()%190+5));
  if(f%7000==3500)dbg2(5,(float)(rand()%11),0,0);if(f%2000==100)dbg2(2,(float)(rand()%24),0,0);
  game_update(1.f/60);memset(key_hit,0,sizeof key_hit);
  if(f%211==0){gfx_begin();game_render();}}
 printf("ok %ld\n",N);return 0;}
