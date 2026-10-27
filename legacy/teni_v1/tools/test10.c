/* Снимки поз и боя на видеокарте (EGL/Mesa): драка со стражей, бег, вис на карнизе */
#include "../game2/game.c"
int host_init(int w,int h);double host_frame(void);void host_snap(const char*path);
static void steps(int n){for(int i=0;i<n;i++){game_update(1.f/60);memset(key_hit,0,sizeof key_hit);}}
static void snap(const char*dir,const char*n){char p[512];snprintf(p,sizeof p,"%s/p_%s.ppm",dir,n);host_frame();host_frame();host_snap(p);printf("снимок %s\n",n);}
static void cam(float yaw,float pitch,float dist){P.cyaw=P.cyaw_t=yaw;P.cpitch=pitch;P.cdist=dist;camsm=0;camera_update(1.f/60);}
int main(int argc,char**argv){const char*dir=argc>1?argv[1]:".";const char*only=argc>2?argv[2]:"";if(host_init(1280,720))return 1;gfx_quality=2;gfx_set_scale(2);game_init();
 key_hit[K_ENTER]=1;steps(2);new_game();gmode=GM_PLAY;chapter=1;tod=11.f;
 if(!*only||strstr(only,"fight")){int gi=-1;for(int i=0;i<NID;i++)if(npc[i].type==NT_GUARD&&!npc[i].fus){gi=i;break;}Npc*g=&npc[gi];
  player_reset(g->x+2.2f,g->y,g->z,0);P.y=floor_at(P.x,P.z,g->y+.5f,.2f,NULL);alert_all(g->x,g->z,40.f);steps(20);
  for(int f=0;f<400;f++){Npc*n=&npc[gi];if(n->wind>.3f&&n->wind<.5f)break;game_update(1.f/60);memset(key_hit,0,sizeof key_hit);}
  P.yaw=atan2f(g->x-P.x,g->z-P.z);cam(P.yaw+1.1f,.18f,4.2f);snap(dir,"fight_windup");
  key_hit[K_Q]=1;steps(1);steps(12);cam(P.yaw+1.3f,.15f,4.f);snap(dir,"fight_counter");
  key_hit[K_F]=1;steps(1);steps(10);cam(P.yaw-1.2f,.2f,4.f);snap(dir,"fight_strike");}
 if(!*only||strstr(only,"run")){player_reset(80,0,62,3.14f);P.y=floor_at(P.x,P.z,.5f,.2f,NULL);for(int f=0;f<50;f++){key_down[K_W]=1;key_down[K_SHIFT]=1;game_update(1.f/60);memset(key_hit,0,sizeof key_hit);}
  cam(P.yaw+1.57f,.12f,3.6f);snap(dir,"run_side");key_down[K_W]=key_down[K_SHIFT]=0;}
 if(!*only||strstr(only,"hang")){player_reset(80,0,62,3.14f);P.y=0;int b=-1;float bd=1e9f;for(int i=0;i<nbox;i++){Box*x=&boxes[i];if(x->kind!=BK_SOLID||x->y1<8||x->y1>20)continue;float cx=(x->x0+x->x1)/2,cz=(x->z0+x->z1)/2,d=hypotf(cx-80,cz-60);if(d<bd){bd=d;b=i;}}
  Box*x=&boxes[b];P.x=(x->x0+x->x1)/2;P.z=x->z1+.5f;hang_on(b,0,1,x->y1-1.f);P.y=x->y1-.95f;steps(10);cam(3.14f+.5f,.05f,3.f);snap(dir,"hang");}
 return 0;}
