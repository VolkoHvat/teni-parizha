#include "../engine/engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void platform_quit(void){}
void platform_parallel(job_fn fn,void*arg,int n){for(int i=0;i<n;i++)fn(arg,i);}
int main(void){gfx_set_scale(2);game_init();srand(7);long frames=0;
 for(long f=0;f<150000;f++){
  for(int k=0;k<K_N;k++){if(rand()%40==0)key_down[k]=!key_down[k];}
  int k=rand()%K_N;if(rand()%12==0)key_hit[k]=1;
  if(rand()%4000==0)key_hit[K_ESC]=0; /* реже закрывать мир */
  game_update(1.f/30);memset(key_hit,0,sizeof key_hit);
  if(f%97==0){gfx_begin();game_render();}frames++;}
 printf("ok %ld frames\n",frames);return 0;}
