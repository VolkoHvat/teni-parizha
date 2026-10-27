/* Снимки кадров с видеокарты (EGL/Mesa): разные места и время суток */
#include "../engine/engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int host_init(int w,int h);double host_frame(void);void host_snap(const char*path);
void dbg2(int cmd,float a,float b,float c);
static void steps(int n){for(int i=0;i<n;i++){game_update(1.f/60);memset(key_hit,0,sizeof key_hit);}}
int main(int argc,char**argv){const char*dir=argc>1?argv[1]:".";int q=argc>2?atoi(argv[2]):2;int W=argc>3?atoi(argv[3]):1280,H=W*9/16;
 if(host_init(W,H))return 1;gfx_quality=q;gfx_set_scale(H/360>0?H/360:1);game_init();
 key_hit[K_ENTER]=1;steps(2);key_hit[K_E]=1;steps(1);for(int i=0;i<6;i++){key_hit[K_E]=1;steps(1);}steps(30);
 struct{const char*n;float x,y,z,yaw,pitch,tod;}S[]={{"street",80,0,58,3.14f,.25f,10.5f},{"market",80,0,58,0,.2f,15.f},{"river",60,0,78,0,.35f,17.8f},{"roofs",106,25,40,2.4f,.5f,12.f},{"night",80,0,58,3.14f,.25f,22.5f},{"cath",60,0,80,1.2f,.1f,8.f},{"park",260,0,66,3.14f,.3f,11.f},{"palace",260,0,34,3.14f,.15f,16.f},{"column",256,0,140,.3f,.1f,14.f},{"barge",300,0,80,3.f,.5f,9.f},{"facade",40,0,60,3.14f,.05f,13.f}};
 int ns=sizeof S/sizeof S[0];char path[512];
 for(int k=0;k<ns;k++){if(argc>4&&!strstr(argv[4],S[k].n))continue;dbg2(1,S[k].x,S[k].y,S[k].z);dbg2(2,S[k].tod,0,0);dbg2(3,S[k].yaw,S[k].pitch,0);steps(40);
  double t=host_frame();t=host_frame();snprintf(path,sizeof path,"%s/g_%s.ppm",dir,S[k].n);host_snap(path);printf("%s: %.0f мс\n",S[k].n,t*1000);}
 return 0;}
