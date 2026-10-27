/* «Тени Парижа: Лига крыш» — открытый мир, паркур-гонки, 128 жителей квартала. Собственный движок. */
#include "../engine/engine.h"
#include "../engine/r3d.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <time.h>
#include "world.c"
#include "chars.c"
#include "ident.c"
#include "audio.c"
static float frnd(void){return rand()/(float)RAND_MAX;}
static float wrapa(float a){while(a>3.14159265f)a-=6.2831853f;while(a<-3.14159265f)a+=6.2831853f;return a;}
static float clampf(float v,float a,float b){return v<a?a:v>b?b:v;}
/* ===== способности (награды за гонки) ===== */
enum{AB_GRAB,AB_ROLL,AB_TURN,AB_TJUMP,AB_STAM1,AB_STAM2,AB_N};
static const char*ABNAME[AB_N]={"Хватка","Перекат","Разворот","Разворот в прыжке","Выносливость I","Выносливость II"};
static const char*ABDESC[AB_N]={"Ловите выступы и карнизы в падении и после прыжка, цепляйтесь за любую стену в полёте.","Приземление с кувырком: падение с высоты до 14 м без урона и без потери скорости.","R на бегу — резкий разворот на 180 градусов.","R, а сразу за ним Пробел — разворот с прыжком. На стене R — оттолкнуться назад.","Спринт расходует на четверть меньше сил.","Спринт расходует ещё на четверть меньше сил, силы восстанавливаются быстрее."};
static int ab[AB_N];
/* ===== игрок ===== */
enum{PS_GND,PS_AIR,PS_CLIMB,PS_HIDE};
typedef struct{float x,y,z,vx,vy,vz,yaw,cyaw,cyaw_t,cpitch,cdist,hp,stam,peak,walk,hold,atk,safex,safey,safez,tsafe,fade;
 int st,cbox,crouch,sprint,moving;float cnx,cnz,roll_t,turn_t,turn_win,stumble_t,land_t,grab_cd,foot_prev,climb_snd,nag_t;
 float combat,combat_t,atk_t,atk_dur,dodge_t,react_t,inv_t,chain_t,cmove,regen_t;int combo,atk_q,atk_tg,atk_hit;Pose ps;}Player;
static Player P;static float camx,camy,camz;static int camsm;
static float tod=17.2f;static int night_b,input_lock;
static char toast[220];static float toast_t;
static void set_toast(const char*f,...){va_list a;va_start(a,f);vsnprintf(toast,sizeof toast,f,a);va_end(a);toast_t=3.6f;}
static void player_reset(float x,float y,float z,float yaw){memset(&P,0,sizeof P);P.x=x;P.y=y;P.z=z;P.hp=100;P.stam=100;P.yaw=yaw;P.cyaw=P.cyaw_t=yaw;P.cpitch=.32f;P.cdist=5.4f;P.st=PS_GND;P.cbox=-1;P.safex=x;P.safey=y;P.safez=z;camsm=0;}
static int move_h(float dx,float dz,int*hb,float*hnx,float*hnz){int hit=-1;float nx=P.x+dx;int b=hit_box(nx,P.y,P.z,.33f,1.75f,.55f);if(b<0)P.x=nx;else{hit=b;*hnx=dx>0?-1.f:1.f;*hnz=0;}
 float nz=P.z+dz;b=hit_box(P.x,P.y,nz,.33f,1.75f,.55f);if(b<0)P.z=nz;else{hit=b;*hnx=0;*hnz=dz>0?-1.f:1.f;}*hb=hit;return hit>=0;}
/* бокс, на котором стоим */
static int box_under(float x,float z,float y){int cx=(int)(x/GC),cz=(int)(z/GC);if(cx<0||cz<0||cx>=GW||cz>=GW)return -1;int c=cz*GW+cx,best=-1;
 for(int i=cstart[c];i<cstart[c+1];i++){const Box*b=&boxes[citems[i]];if(b->kind!=BK_SOLID)continue;if(fabsf(b->y1-y)<.08f&&x>=b->x0-.31f&&x<=b->x1+.31f&&z>=b->z0-.31f&&z<=b->z1+.31f)best=citems[i];}return best;}
/* повиснуть на ребре бокса b с той стороны, что смотрит по нормали (nx,nz) */
static void hang_on(int b,float nx,float nz,float y){Box*bb=&boxes[b];P.st=PS_CLIMB;P.cbox=b;P.cnx=nx;P.cnz=nz;P.vx=P.vz=P.vy=0;P.hold=0;
 if(nx!=0)P.x=nx>0?bb->x1+.36f:bb->x0-.36f;else P.z=nz>0?bb->z1+.36f:bb->z0-.36f;P.y=y;P.yaw=atan2f(-nx,-nz);}
/* хватка: поиск карниза в падении */
static int try_ledge_grab(void){int c0x=(int)floorf((P.x-1.f)/GC),c1x=(int)floorf((P.x+1.f)/GC),c0z=(int)floorf((P.z-1.f)/GC),c1z=(int)floorf((P.z+1.f)/GC);
 if(c0x<0)c0x=0;if(c0z<0)c0z=0;if(c1x>=GW)c1x=GW-1;if(c1z>=GW)c1z=GW-1;int best=-1;float bd=.62f,bnx=0,bnz=0;
 for(int cz=c0z;cz<=c1z;cz++)for(int cx=c0x;cx<=c1x;cx++){int c=cz*GW+cx;for(int i=cstart[c];i<cstart[c+1];i++){const Box*b=&boxes[citems[i]];if(b->kind!=BK_SOLID)continue;
   float top=b->y1;if(top<P.y+1.3f||top>P.y+2.15f||top-b->y0<1.2f)continue;
   float qx=clampf(P.x,b->x0,b->x1),qz=clampf(P.z,b->z0,b->z1),dx=P.x-qx,dz=P.z-qz,d=hypotf(dx,dz);if(d<=.01f||d>=bd)continue;
   if(hit_box(P.x,top+.05f,P.z,.3f,1.6f,.05f)>=0)continue;
   bd=d;best=citems[i];if(fabsf(dx)>fabsf(dz)){bnx=dx>0?1.f:-1.f;bnz=0;}else{bnx=0;bnz=dz>0?1.f:-1.f;}}}
 if(best<0)return 0;hang_on(best,bnx,bnz,boxes[best].y1-1.0f);sfx(S_GRAB,1,0);return 1;}
static void respawn(const char*why);
static void race_fail(const char*why);
static float slowmo,tok_cd;static int tok_owner=-1;static int combat_threat(float r);
static int race_active(void);
static int roof_rule(void);
static int race_stealth(void);
static void player_update(float dt){
 P.fade-=dt*1.5f;if(P.fade<0)P.fade=0;P.nag_t-=dt;
 if(mouse_want){float d=-mouse_dx*.0024f;P.cyaw+=d;P.cyaw_t+=d;P.cpitch+=mouse_dy*.0022f;}
 if(key_down[K_LEFT]){P.cyaw+=1.9f*dt;P.cyaw_t+=1.9f*dt;}if(key_down[K_RIGHT]){P.cyaw-=1.9f*dt;P.cyaw_t-=1.9f*dt;}
 if(key_down[K_UP])P.cpitch-=1.2f*dt;if(key_down[K_DOWN])P.cpitch+=1.2f*dt;P.cpitch=clampf(P.cpitch,-.35f,1.25f);
 P.cyaw+=wrapa(P.cyaw_t-P.cyaw)*fminf(1.f,dt*11.f);
 float fx=sinf(P.cyaw_t),fz=cosf(P.cyaw_t),rx=-fz,rz=fx;float ix=(key_down[K_D]?1.f:0)-(key_down[K_A]?1.f:0),iz=(key_down[K_W]?1.f:0)-(key_down[K_S]?1.f:0);
 if(input_lock){ix=iz=0;}
 float dx=fx*iz+rx*ix,dz=fz*iz+rz*ix,il=hypotf(dx,dz);P.moving=il>.1f;if(P.moving){dx/=il;dz/=il;}
 P.crouch=key_down[K_CTRL]&&P.st==PS_GND&&!input_lock;static int stam_lock;if(P.stam<5)stam_lock=1;if(P.stam>30)stam_lock=0;
 P.sprint=key_down[K_SHIFT]&&P.moving&&!stam_lock&&!P.crouch&&!input_lock;
 float drain=9.f*(ab[AB_STAM1]?.75f:1.f)*(ab[AB_STAM2]?.75f:1.f),regen=ab[AB_STAM2]?20.f:14.f;
 float sp=P.crouch?2.0f:P.sprint?7.6f:4.6f;if(P.atk_t>0||P.react_t>0)sp*=.12f;else if(P.combat>.5f&&!P.sprint)sp*=.8f;if(P.sprint&&P.st==PS_GND)P.stam-=drain*dt;else if(P.stam<100)P.stam+=regen*dt;if(P.stam>100)P.stam=100;
 if(P.stumble_t>0){P.stumble_t-=dt;sp*=.35f;}if(P.land_t>0){P.land_t-=dt;sp=0;}if(P.turn_t>0)P.turn_t-=dt;if(P.turn_win>0)P.turn_win-=dt;if(P.grab_cd>0)P.grab_cd-=dt;
 P.atk-=dt;int hb=-1;float hnx=0,hnz=0;
 /* разворот */
 if(key_hit[K_R]&&!input_lock){if(!ab[AB_TURN]&&P.st!=PS_CLIMB){if(P.nag_t<=0){set_toast("Разворот ещё не открыт: выиграйте гонку «Не касаясь мостовой»");P.nag_t=3;}}
  else if(P.st==PS_GND){P.yaw+=3.14159265f;P.cyaw_t+=3.14159265f;P.vx*=-.55f;P.vz*=-.55f;P.turn_t=.25f;P.turn_win=.32f;sfx(S_TURN,1,0);}
  else if(P.st==PS_CLIMB&&ab[AB_TJUMP]){P.vx=P.cnx*6.6f;P.vz=P.cnz*6.6f;P.vy=7.2f;P.st=PS_AIR;P.peak=P.y;P.yaw=atan2f(P.cnx,P.cnz);P.cyaw_t=P.yaw;P.grab_cd=.4f;P.turn_t=.25f;sfx(S_TURN,1,0);sfx(S_JUMP,1,0);}
  else if(P.st==PS_CLIMB&&P.nag_t<=0){set_toast("Отталкивание от стены откроется после гонки «Флажки Лиги»");P.nag_t=3;}}
 if(P.st==PS_GND||P.st==PS_AIR){
  if(P.st==PS_GND&&P.moving&&P.roll_t<=0){float ty=atan2f(dx,dz);P.yaw+=wrapa(ty-P.yaw)*fminf(1,dt*12);}
  if(P.st==PS_GND){
   if(P.roll_t>0){P.roll_t-=dt;float s=hypotf(P.vx,P.vz);if(s<3.5f&&P.moving){P.vx=sinf(P.yaw)*3.5f;P.vz=cosf(P.yaw)*3.5f;}}
   else{float tvx=dx*sp*(P.moving?1:0),tvz=dz*sp*(P.moving?1:0);P.vx+=(tvx-P.vx)*fminf(1,dt*9);P.vz+=(tvz-P.vz)*fminf(1,dt*9);}}
  else{float hsp=hypotf(P.vx,P.vz);
   if(P.moving){float cur=hsp>.3f?atan2f(P.vx,P.vz):atan2f(dx,dz),d=wrapa(atan2f(dx,dz)-cur),mx=2.6f*dt;if(d>mx)d=mx;if(d<-mx)d=-mx;cur+=d;float ns=hsp<sp?hsp+(sp-hsp)*fminf(1,dt*1.4f):hsp;P.vx=sinf(cur)*ns;P.vz=cosf(cur)*ns;}
   else{P.vx*=1-.5f*dt;P.vz*=1-.5f*dt;}
   if(hsp>.5f)P.yaw+=wrapa(atan2f(P.vx,P.vz)-P.yaw)*fminf(1,dt*8);}
  /* спуск с края: Ctrl + шаг за край */
  if(P.st==PS_GND&&P.crouch&&P.moving){float nx=P.x+dx*.55f,nz=P.z+dz*.55f,fl2=floor_at(nx,nz,P.y+.1f,.25f,NULL);
   if(P.y-fl2>1.8f){int b=box_under(P.x,P.z,P.y);if(b>=0){Box*bb=&boxes[b];float ex=0,ez=0;
     if(nx>bb->x1)ex=1;else if(nx<bb->x0)ex=-1;else if(nz>bb->z1)ez=1;else if(nz<bb->z0)ez=-1;
     if(ex!=0||ez!=0){hang_on(b,ex,ez,bb->y1-1.0f);P.grab_cd=.3f;sfx(S_GRAB,.7f,0);goto after_move;}}}}
  int hit=move_h(P.vx*dt,P.vz*dt,&hb,&hnx,&hnz);
  if(hit&&P.moving&&boxes[hb].kind==BK_SOLID&&P.grab_cd<=0){float into=-(dx*hnx+dz*hnz);if(into>.55f){P.hold+=dt;float top=boxes[hb].y1,rel=top-P.y;
    if(P.hold>.1f||P.st==PS_AIR){
     if(rel<=2.3f){float nx=P.x-hnx*.65f,nz=P.z-hnz*.65f;if(hit_box(nx,top+.02f,nz,.3f,1.75f,.05f)<0){P.x=nx;P.z=nz;P.y=top;P.vx*=.6f;P.vz*=.6f;P.vy=0;P.st=PS_GND;P.hold=0;sfx(S_CLIMB,1,0);}}
     else if(rel<=70.f&&(P.st==PS_GND||ab[AB_GRAB])){hang_on(hb,hnx,hnz,P.y);sfx(S_GRAB,.8f,0);}}}else P.hold=0;}else P.hold=0;
  if(P.st==PS_GND){int fk;float fl=floor_at(P.x,P.z,P.y,.3f,&fk);
   if(P.y>fl+.08f){if(P.y-fl<.45f)P.y=fl;else{P.st=PS_AIR;P.vy=0;P.peak=P.y;}}else P.y=fl;
   if(key_hit[K_SPACE]&&P.st==PS_GND&&!input_lock&&P.combat>.5f&&combat_threat(4.5f)){/* уклонение */float ddx=P.moving?dx:cosf(P.yaw),ddz=P.moving?dz:-sinf(P.yaw);P.vx=ddx*7.5f;P.vz=ddz*7.5f;P.dodge_t=.4f;P.inv_t=.45f;P.roll_t=.42f;P.yaw=atan2f(ddx,ddz);sfx(S_ROLL,1,0);key_hit[K_SPACE]=0;}
   if(key_hit[K_SPACE]&&P.st==PS_GND&&!input_lock){int tj=P.turn_win>0&&ab[AB_TJUMP];float hsp=hypotf(P.vx,P.vz);
    if(P.sprint||tj){float s=fmaxf(hsp,tj?8.8f:9.3f);P.vx=sinf(P.yaw)*s;P.vz=cosf(P.yaw)*s;P.vy=7.9f;}else P.vy=7.2f;
    P.st=PS_AIR;P.peak=P.y;P.roll_t=0;P.turn_win=0;sfx(S_JUMP,1,0);}
   P.tsafe+=dt;if(P.st==PS_GND&&fl>-.5f&&P.tsafe>1.f&&!race_active()){P.safex=P.x;P.safey=P.y;P.safez=P.z;P.tsafe=0;}
   {float s=hypotf(P.vx,P.vz);P.walk+=(s*1.6f+3.f*fminf(1.f,s))*dt;float sn=sinf(P.walk);if(s>.6f&&P.roll_t<=0&&((sn>0)!=(P.foot_prev>0)))sfx(S_FOOT,.35f+s/12.f,0);P.foot_prev=sn;}}
  if(P.st==PS_AIR){P.vy-=22.f*dt;P.y+=P.vy*dt;if(P.y>P.peak)P.peak=P.y;if(P.vy>0&&hit_box(P.x,P.y,P.z,.3f,1.75f,1.7f)>=0)P.vy=0;
   if(P.vy<0&&ab[AB_GRAB]&&P.grab_cd<=0&&P.peak-P.y>.4f)if(try_ledge_grab())goto after_move;
   int fk;float fl=floor_at(P.x,P.z,P.y+.1f,.3f,&fk);
   if(P.vy<=0&&P.y<=fl){P.y=fl;float fall=P.peak-P.y;P.st=PS_GND;P.vy=0;
    if(fl<-2.5f){P.hp-=10;respawn("Вы упали в Сену. Течение вынесло вас на берег.");return;}
    else if(fk==BK_HAY&&fall>2.5f){set_toast("Прыжок веры! Мягкая посадка в сено");P.st=PS_HIDE;P.vx=P.vz=0;sfx(S_HAY,1,0);}
    else if(fall>=3.2f){int canroll=ab[AB_ROLL]&&(P.moving||key_down[K_CTRL]);float dmg;
     if(canroll){P.roll_t=.55f;dmg=fall>14.f?(fall-14.f)*7.f:0;sfx(S_ROLL,1,0);}else{dmg=fall>7.f?(fall-7.f)*7.f:0;P.land_t=fall>5.f?.35f:.15f;P.vx*=.2f;P.vz*=.2f;sfx(S_LAND,1,0);}
     if(dmg>0){P.hp-=dmg;set_toast(canroll?"Жёсткий кувырок":"Тяжёлое падение");}}
    else if(fall>.8f)sfx(S_LAND,.45f,0);
    if(roof_rule()&&fl<.8f&&fk!=BK_HAY){race_fail("Вы коснулись мостовой!");return;}}}
 }else if(P.st==PS_CLIMB){Box*b=&boxes[P.cbox];float up=input_lock?0:(key_down[K_W]?1.f:0)-(key_down[K_S]?1.f:0),side=input_lock?0:(key_down[K_D]?1.f:0)-(key_down[K_A]?1.f:0);P.yaw=atan2f(-P.cnx,-P.cnz);
  P.y+=up*(P.sprint?3.6f:2.4f)*dt;float r2x=P.cnz,r2z=-P.cnx;
  if(side!=0){float lx=P.x+r2x*side*2.4f*dt,lz=P.z+r2z*side*2.4f*dt;if(hit_box(lx-P.cnx*.45f,P.y+.6f,lz-P.cnz*.45f,.06f,.8f,-5.f)>=0){P.x=lx;P.z=lz;}}
  P.cmove+=((up!=0||side!=0)?1.f:0.f-P.cmove)*fminf(1.f,dt*8);
  if(up!=0||side!=0){P.walk+=dt*4.5f;P.climb_snd-=dt;if(P.climb_snd<=0){P.climb_snd=.32f;sfx(S_CLIMB,.7f,0);}}
  float top=b->y1;if(P.y>top-.95f&&up>0){float nx=P.x-P.cnx*.75f,nz=P.z-P.cnz*.75f;if(hit_box(nx,top+.02f,nz,.3f,1.75f,.05f)<0){P.x=nx;P.z=nz;P.y=top;P.st=PS_GND;P.vx=P.vz=0;sfx(S_CLIMB,1,0);}
   else{/* сверху мешает труба или окно: сдвигаемся вдоль стены к свободному месту */float best=0;
    for(float s=.4f;s<=3.61f&&best==0;s+=.4f)for(int sg=-1;sg<=1&&best==0;sg+=2){float ox=P.x+r2x*s*sg,oz=P.z+r2z*s*sg;if(hit_box(ox-P.cnx*.45f,P.y+.6f,oz-P.cnz*.45f,.06f,.8f,-5.f)<0)break;if(hit_box(ox-P.cnx*.75f,top+.02f,oz-P.cnz*.75f,.3f,1.75f,.05f)<0)best=s*sg;}
    if(best!=0){float st2=fminf(fabsf(best),2.6f*dt)*(best>0?1.f:-1.f);P.x+=r2x*st2;P.z+=r2z*st2;P.walk+=dt*4.5f;}P.y=top-.95f;}}
  if(P.y>top-.95f&&P.st==PS_CLIMB)P.y=top-.95f;
  {int fk;float fl=floor_at(P.x,P.z,P.y+.2f,.3f,&fk);if(P.y<=fl+.02f&&up<0){P.y=fl;P.st=PS_GND;if(roof_rule()&&fl<.8f){race_fail("Вы коснулись мостовой!");return;}}}
  if(key_hit[K_SPACE]&&!input_lock){P.vx=P.cnx*4.2f;P.vz=P.cnz*4.2f;P.vy=6.4f;P.st=PS_AIR;P.peak=P.y;P.grab_cd=.35f;sfx(S_JUMP,.8f,0);}
  if(key_hit[K_CTRL]&&!input_lock){P.st=PS_AIR;P.vx=P.cnx*1.2f;P.vz=P.cnz*1.2f;P.vy=0;P.peak=P.y;P.grab_cd=.5f;}
 }else if(P.st==PS_HIDE){P.vx=P.vz=0;if(!input_lock&&(key_hit[K_E]||key_hit[K_SPACE]||(P.moving&&P.hold>.25f))){P.st=PS_GND;P.x+=sinf(P.yaw)*1.8f;P.z+=cosf(P.yaw)*1.8f;P.hold=0;}if(P.moving)P.hold+=dt;else P.hold=0;}
 after_move:;
 if(P.hp<=0){P.hp=100;respawn("Вы без сил. Очнулись у последнего безопасного места.");}
 if(P.y<-12)respawn("Вы упали за пределы мира.");}
/* ===== жители квартала: ровно 128, у каждого своё место в мире ===== */
enum{NT_CIT,NT_GUARD,NT_MERCH,NT_RUNNER,NT_PAINTER};enum{GS_PATROL,GS_SUSP,GS_CHASE,GS_SEARCH,GS_KO};
typedef struct{float x,y,z,yaw,walk,spd,tx,tz,wait,alert,hp,stun,ko,lsx,lsz,lost,atk,t_un,hx,hz,hr,hyaw,down,getup,bump_t,bark_t,look_t,cd;
 int type,st,route,nroute,vis,roam,racing;float rx[5],rz[5];const char*bark;
 float wind,swing,blk_t,open_t,stag_t,aim_t,reload,strafe,flee_t,sit_t,fx,fz;int fus,shot,hitc;float hitc_t;Pose ps;}Npc;
static Npc npc[NID];static Model mdl_npc[NID],mdl_hero;
static unsigned char alb[NID];/* 0 — незнаком, 1 — знаком, 2 — портрет написан */
static int incrowd,chase_cnt,susp_cnt;
static float hubv[NRUN][3]={{28,26,3.14f},{66,81,3.14f},{90,54.5f,0},{126,140,1.57f},{40,141,3.14f}};
static float painter_v[3]={78.5f,49.5f,3.14f};
static int walkable(float x,float z){if(x<2||z<2||x>WORLD-2||z>WORLDZ-2)return 0;if(in_river(x,z)&&ground_tile(x,z)<0)return 0;if(ground_tile(x,z)<0)return 0;return hit_box(x,floor_at(x,z,.5f,.2f,NULL),z,.4f,1.7f,.55f)<0;}
static void fix_spot(float*x,float*z){if(walkable(*x,*z))return;for(int r=1;r<12;r++)for(int a=0;a<12;a++){float nx=*x+cosf(a*.5236f)*r*.5f,nz=*z+sinf(a*.5236f)*r*.5f;if(walkable(nx,nz)){*x=nx;*z=nz;return;}}}
static void npc_target(Npc*n){
 if(n->roam){for(int t=0;t<8;t++){const Rect*r=&strects[rand()%nstr];float x=r->x0+frnd()*(r->x1-r->x0),z=r->z0+frnd()*(r->z1-r->z0);if(walkable(x,z)){n->tx=x;n->tz=z;return;}}}
 for(int t=0;t<8;t++){float a=frnd()*6.2831853f,r=frnd()*n->hr,x=n->hx+cosf(a)*r,z=n->hz+sinf(a)*r;if(walkable(x,z)){n->tx=x;n->tz=z;return;}}n->tx=n->hx;n->tz=n->hz;}
static int npc_step(Npc*n,float dx,float dz){float nx=n->x+dx,nz=n->z+dz;int ok=1;
 if(hit_box(nx,n->y,n->z,.3f,1.7f,.55f)<0&&!(in_river(nx,n->z)&&ground_tile(nx,n->z)<0))n->x=nx;else ok=0;
 if(hit_box(n->x,n->y,nz,.3f,1.7f,.55f)<0&&!(in_river(n->x,nz)&&ground_tile(n->x,nz)<0))n->z=nz;else ok=0;
 n->y=floor_at(n->x,n->z,n->y+.3f,.2f,NULL);if(n->y<-1)n->y=0;return ok;}
static const float GROUTE[16][9]={ /* число точек, затем x,z */
 {4,66,26,94,26,94,54,66,54},{2,30,20,100,20},{3,20,150,20,185,60,185},{4,150,150,170,150,170,170,150,170},{3,168,170,148,170,148,152},{2,160,156,160,172},{2,152,162,170,162},
 {2,156,148,164,148},{2,150,138,170,138},{1,160,139},{2,140,40,140,70},{2,180,30,180,70},{2,180,120,180,170},{2,140,88,140,108},{2,82,87,118,87},{2,100,125,100,190}};
static void spawn_npcs(void){int gi=0,mi=0;
 static const float MS[14][3]={{68,31.4f,3.14f},{72,31.4f,3.14f},{88,31.4f,3.14f},{92,31.4f,3.14f},{30,151.4f,3.14f},{38,151.4f,3.14f},{46,151.4f,3.14f},{38,171.4f,3.14f},
  {17.3f,30,1.57f},{22.7f,48,-1.57f},{57.3f,150,1.57f},{62.7f,166,-1.57f},{137.3f,30,1.57f},{142.7f,48,-1.57f}};
 static const float CLH[2][3]={{40,30,9},{100,108,10}},NOBH[3][3]={{80,40,13},{40,160,12},{160,166,8}},VAGH[4][3]={{60,82,6},{78,88,8},{30,34,6},{52,148,6}},SAIH[3][3]={{40,81,14},{120,115,14},{60,98,5}};
 for(int i=0;i<NID;i++){Npc*n=&npc[i];memset(n,0,sizeof *n);Ident*d=&ids[i];n->hp=100;n->yaw=frnd()*6.28f;n->hr=6;n->roam=0;
  if(d->role==RO_PAINTER){n->type=NT_PAINTER;n->hx=painter_v[0];n->hz=painter_v[1];n->hyaw=painter_v[2];}
  else if(d->role==RO_RUNNER){int r=0;for(int q=0;q<NRUN;q++)if(runner_id[q]==i)r=q;n->type=NT_RUNNER;n->hx=hubv[r][0];n->hz=hubv[r][1];n->hyaw=hubv[r][2];}
  else if(d->cls==CL_SOLDIER){n->type=NT_GUARD;const float*g=GROUTE[gi%16];gi++;n->nroute=(int)g[0];for(int k=0;k<n->nroute;k++){n->rx[k]=g[1+k*2];n->rz[k]=g[2+k*2];}n->hx=n->rx[0];n->hz=n->rz[0];if(n->nroute<2)n->nroute=1;n->fus=(gi%3==0);if(n->fus)n->hp=75;}
  else if(d->cls==CL_SHOP){n->type=NT_MERCH;const float*m=MS[mi%14];mi++;n->hx=m[0];n->hz=m[1];n->hyaw=m[2];}
  else{n->type=NT_CIT;uint32_t h=hs(i,401);
   if(d->cls==CL_CLERGY){const float*c=CLH[h%2];n->hx=c[0];n->hz=c[1];n->hr=c[2];}
   else if(d->cls==CL_NOBLE){const float*c=NOBH[h%3];n->hx=c[0];n->hz=c[1];n->hr=c[2];}
   else if(d->cls==CL_VAGRANT){const float*c=VAGH[h%4];n->hx=c[0];n->hz=c[1];n->hr=c[2];}
   else if(d->cls==CL_SAILOR){const float*c=SAIH[h%3];n->hx=c[0];n->hz=c[1];n->hr=c[2];}
   else{n->roam=1;const Rect*r=&strects[h%nstr];n->hx=r->x0+rf(h>>8)*(r->x1-r->x0);n->hz=r->z0+rf(h>>3)*(r->z1-r->z0);}}
  fix_spot(&n->hx,&n->hz);n->x=n->hx;n->z=n->hz;n->y=floor_at(n->x,n->z,.5f,.2f,NULL);if(n->type!=NT_CIT)n->yaw=n->hyaw;if(n->type==NT_CIT)npc_target(n);if(n->type==NT_GUARD){n->tx=n->rx[0];n->tz=n->rz[0];}}}
static void bark(Npc*n,const char*s){n->bark=s;n->bark_t=2.4f;}
static const char*B_BRUSH[6]={"Pardon!","Осторожнее, месье!","Простите…","Куда так спешите?","Ох!","Полегче!"};
static const char*B_BUMP[6]={"Эй, смотри, куда бежишь!","Ай! Вот наглец!","Ну и манеры!","Совсем ошалел?!","Под ноги смотри!","Сумасшедший!"};
static const char*B_DOWN[6]={"Ой-ой-ой!","Помогите, сбили с ног!","Негодяй!","Стража! Стража!","Моя спина!","Да что ж такое!"};
static const char*B_GUARD[3]={"Стоять!","Ах ты, мерзавец!","Именем короля!"};
static const char*B_SEE[3]={"Что творится!","Вы видели?!","Безобразие!"};
static int guard_sees(Npc*g,float px,float py,float pz,float R){float dx=px-g->x,dz=pz-g->z,d=hypotf(dx,dz);if(d>R)return 0;float ang=d>.01f?(dx*sinf(g->yaw)+dz*cosf(g->yaw))/d:1;
 if(ang<.35f&&d>3.f)return 0;return !seg_blocked(v3(g->x,g->y+1.65f,g->z),v3(px,py+1.2f,pz));}
static void alert_all(float x,float z,float r){for(int i=0;i<NID;i++){Npc*n=&npc[i];if(n->type!=NT_GUARD||n->st==GS_KO)continue;if(hypotf(n->x-x,n->z-z)<r){if(n->st<GS_CHASE)sfx(S_ALERT,1,0);n->st=GS_CHASE;n->alert=1;n->lost=0;n->lsx=P.x;n->lsz=P.z;}}}
static void witness(float x,float z,float amt){for(int i=0;i<NID;i++){Npc*g=&npc[i];if(g->type!=NT_GUARD||g->st==GS_KO||g->down>0)continue;if(!guard_sees(g,x,P.y,z,24.f))continue;g->alert+=amt;g->lsx=P.x;g->lsz=P.z;
  if(g->alert>=1&&g->st<GS_CHASE){g->st=GS_CHASE;g->alert=1;bark(g,B_GUARD[rand()%3]);sfx(S_ALERT,1,0);}else if(g->st==GS_PATROL)g->st=GS_SUSP;}}
/* паника: горожане разбегаются от драки и выстрелов */
static void panic(float x,float z,float r){static const char*BP[5]={"Драка! Бежим!","Господи, стреляют!","Караул!","Прочь отсюда!","Помогите!"};
 for(int i=0;i<NID;i++){Npc*n=&npc[i];if(n->type!=NT_CIT||n->down>0||!n->vis)continue;if(hypotf(n->x-x,n->z-z)>r)continue;if(n->flee_t<=0&&rand()%4==0)bark(n,BP[rand()%5]);n->flee_t=4.f+frnd()*3.f;n->fx=x;n->fz=z;}}
/* касания, толчки и сбивания с ног */
static void crowd_contact(float dt){if(P.st==PS_HIDE||P.st==PS_CLIMB)return;
 for(int i=0;i<NID;i++){Npc*n=&npc[i];if(n->cd>0)n->cd-=dt;if(n->type==NT_GUARD&&n->st==GS_KO)continue;if(n->racing)continue;
  float dx=n->x-P.x,dz=n->z-P.z,d=hypotf(dx,dz),md=.62f;if(d>=md||fabsf(n->y-P.y)>1.1f)continue;if(d<.01f){dx=.01f;d=.01f;}
  float nx=dx/d,nz=dz/d,app=P.vx*nx+P.vz*nz;int fem=ids[i].female;float pitch=fem?280.f+frnd()*60:160.f+frnd()*50;
  if(n->down<=0&&n->getup<=0&&n->cd<=0){
   if(app>5.6f&&n->type!=NT_MERCH&&n->type!=NT_PAINTER){n->down=2.0f+frnd()*1.2f;n->cd=2.f;n->yaw=atan2f(-nx,-nz);float px=n->x+nx*.6f,pz=n->z+nz*.6f;if(hit_box(px,n->y,pz,.3f,1.7f,.55f)<0){n->x=px;n->z=pz;}
    P.stumble_t=.4f;P.vx*=.45f;P.vz*=.45f;bark(n,n->type==NT_GUARD?B_GUARD[rand()%3]:B_DOWN[rand()%6]);sfx(S_BUMP,1,pitch);sfx(S_LAND,.7f,0);
    for(int k=0;k<NID;k++){Npc*o=&npc[k];if(o==n||o->type==NT_GUARD||o->down>0)continue;if(hypotf(o->x-n->x,o->z-n->z)<6.f){o->look_t=1.6f;if(o->bark_t<=0&&rand()%3==0)bark(o,B_SEE[rand()%3]);}}
    if(n->type==NT_GUARD){n->alert=1;n->st=GS_CHASE;n->lsx=P.x;n->lsz=P.z;alert_all(n->x,n->z,16.f);}else witness(n->x,n->z,.85f);}
   else if(app>2.4f){n->bump_t=.6f;n->cd=1.2f;float px=n->x+nx*.32f,pz=n->z+nz*.32f;if(hit_box(px,n->y,pz,.3f,1.7f,.55f)<0){n->x=px;n->z=pz;}n->yaw=atan2f(-nx,-nz);
    P.stumble_t=.22f;P.vx*=.7f;P.vz*=.7f;bark(n,n->type==NT_GUARD?"Эй! Полегче!":B_BUMP[rand()%6]);sfx(S_BUMP,.8f,pitch);
    if(n->type==NT_GUARD){n->alert+=.45f;n->lsx=P.x;n->lsz=P.z;if(n->st==GS_PATROL)n->st=GS_SUSP;}else witness(n->x,n->z,.25f);}
   else{n->cd=1.0f;n->look_t=.8f;sfx(S_BRUSH,.7f,0);if(rand()%4==0)bark(n,B_BRUSH[rand()%6]);}}
  /* раздвинуть */
  float push=md-d;int heavy=(n->type==NT_MERCH||n->type==NT_PAINTER||n->type==NT_RUNNER);float sn=heavy?0:.7f;
  if(sn>0){float px=n->x+nx*push*sn,pz=n->z+nz*push*sn;if(hit_box(px,n->y,pz,.3f,1.7f,.55f)<0){n->x=px;n->z=pz;}else sn=0;}
  float sp2=(1-sn);float qx=P.x-nx*push*sp2,qz=P.z-nz*push*sp2;if(hit_box(qx,P.y,qz,.33f,1.75f,.55f)<0){P.x=qx;P.z=qz;}
  if(app>0){P.vx-=nx*app*.5f;P.vz-=nz*app*.5f;}}}
static void npcs_update(float dt){incrowd=0;chase_cnt=0;susp_cnt=0;int cn=0;if(tok_cd>0)tok_cd-=dt;if(tok_owner>=0&&(npc[tok_owner].st!=GS_CHASE||npc[tok_owner].wind<=0))tok_owner=-1;
 for(int i=0;i<NID;i++){Npc*n=&npc[i];float ddx=n->x-P.x,ddz=n->z-P.z,dist=hypotf(ddx,ddz);n->vis=dist<100;if(n->bark_t>0)n->bark_t-=dt;
  if(n->type==NT_GUARD){if(n->st==GS_CHASE)chase_cnt++;if(n->st==GS_SUSP||n->st==GS_SEARCH)susp_cnt++;}
  if(n->racing)continue;
  if(!n->vis&&!(n->type==NT_GUARD&&n->st>=GS_CHASE))continue;
  if(n->type==NT_CIT&&dist<3.6f&&n->down<=0)cn++;
  n->spd=0;
  if(n->down>0){n->down-=dt;if(n->down<=0)n->getup=.7f;continue;}
  if(n->getup>0){n->getup-=dt;continue;}
  if(n->bump_t>0){n->bump_t-=dt;continue;}
  if(n->look_t>0){n->look_t-=dt;n->yaw+=wrapa(atan2f(-ddx,-ddz)-n->yaw)*fminf(1,dt*6);continue;}
  if(n->type==NT_MERCH||n->type==NT_PAINTER||n->type==NT_RUNNER){float want=dist<6.f?atan2f(-ddx,-ddz):n->hyaw;n->yaw+=wrapa(want-n->yaw)*fminf(1,dt*3);
   if(hypotf(n->x-n->hx,n->z-n->hz)>.4f){float tx=n->hx-n->x,tz=n->hz-n->z,l=hypotf(tx,tz);n->yaw=atan2f(tx,tz);npc_step(n,tx/l*1.5f*dt,tz/l*1.5f*dt);n->spd=1.5f;n->walk+=(1.5f*1.6f+3.f)*dt;}continue;}
  if(n->type==NT_CIT&&n->flee_t>0){n->flee_t-=dt;float ax=n->x-n->fx,az=n->z-n->fz,al=hypotf(ax,az)+.01f;float ty=atan2f(ax/al,az/al);n->yaw+=wrapa(ty-n->yaw)*fminf(1,dt*8);n->spd=3.6f;
   if(!npc_step(n,sinf(n->yaw)*3.6f*dt,cosf(n->yaw)*3.6f*dt))n->yaw+=1.2f;n->walk+=(3.6f*1.6f+3.f)*dt;if(n->flee_t<=0)npc_target(n);continue;}
  if(n->type==NT_CIT){if(n->wait>0){n->wait-=dt;continue;}float tx=n->tx-n->x,tz=n->tz-n->z,d=hypotf(tx,tz);if(d<1.2f){n->wait=1+frnd()*4;npc_target(n);continue;}
   n->spd=(ids[i].cls==CL_NOBLE||ids[i].age>58)?1.0f:1.25f+(i%5)*.1f;n->yaw+=wrapa(atan2f(tx,tz)-n->yaw)*fminf(1,dt*6);
   if(!npc_step(n,sinf(n->yaw)*n->spd*dt,cosf(n->yaw)*n->spd*dt)){npc_target(n);n->wait=.3f;}n->walk+=(n->spd*1.6f+3.f*fminf(1.f,n->spd))*dt;continue;}
  /* стражник */
  {float dx=P.x-n->x,dz=P.z-n->z,d=hypotf(dx,dz);float strafe_spd=0;
   if(n->st==GS_KO){n->ko-=dt;n->wind=0;n->aim_t=0;if(tok_owner==i)tok_owner=-1;if(n->ko<=0){n->st=GS_SEARCH;n->wait=5;n->hp=60;n->alert=.5f;n->lsx=n->x;n->lsz=n->z;}continue;}
   if(n->stun>0){n->stun-=dt;continue;}
   float R=26.f;if(P.sprint)R*=1.25f;if(P.crouch)R*=.55f;if(incrowd)R*=.35f;if(P.st==PS_HIDE)R=0;if(night_b)R*=.75f;if(race_active()&&n->st==GS_PATROL)R=race_stealth()?R*.8f:0;/* во время гонок стража не замечает бегуна, пока он никого не сбил (кроме «Сада маркиза») */
   int see=d<R&&fabsf(P.y-n->y)<30&&guard_sees(n,P.x,P.y,P.z,R);
   if(see){n->alert+=(.7f+(1-d/R)*2.4f)*dt*(race_active()?.5f:1.f);n->lsx=P.x;n->lsz=P.z;n->lost=0;
    if(n->alert>=1&&n->st<GS_CHASE){n->st=GS_CHASE;n->alert=1;set_toast("Вас заметили!");bark(n,B_GUARD[rand()%3]);sfx(S_ALERT,1,0);alert_all(n->x,n->z,22);}else if(n->alert>.3f&&n->st==GS_PATROL)n->st=GS_SUSP;}
   else{n->alert-=(n->st==GS_CHASE?.04f:.22f)*dt;if(n->alert<0)n->alert=0;n->lost+=dt;}
   float tx=0,tz=0,spd=0;
   if(n->st==GS_PATROL){if(n->nroute>=2){tx=n->rx[n->route]-n->x;tz=n->rz[n->route]-n->z;if(n->wait>0)n->wait-=dt;else{if(hypotf(tx,tz)<1.3f){n->wait=2+frnd()*2;n->route=(n->route+1)%n->nroute;}else spd=1.7f;}}else{n->yaw+=dt*.35f;}}
   else if(n->st==GS_SUSP){float ty=atan2f(n->lsx-n->x,n->lsz-n->z);n->yaw+=wrapa(ty-n->yaw)*fminf(1,dt*4);if(!see&&n->alert<.1f)n->st=GS_PATROL;}
   else if(n->st==GS_CHASE){float gx=see?P.x:n->lsx,gz=see?P.z:n->lsz;tx=gx-n->x;tz=gz-n->z;spd=5.2f;
    float dy=fabsf(P.y-n->y);if(n->hitc_t>0){n->hitc_t-=dt;if(n->hitc_t<=0)n->hitc=0;}if(n->stag_t>0){n->stag_t-=dt;spd=0;}if(n->open_t>0)n->open_t-=dt;if(n->blk_t>0)n->blk_t-=dt;if(n->reload>0)n->reload-=dt;if(n->swing>0){n->swing-=dt;spd=0;}
    /* стрелок: целится издалека, если видит героя */
    if(n->fus&&see&&d>5.f&&d<30.f&&n->reload<=0&&n->stag_t<=0&&n->wind<=0){spd=0;n->yaw+=wrapa(atan2f(dx,dz)-n->yaw)*fminf(1,dt*8);n->aim_t+=dt;
     if(n->aim_t>1.7f){n->aim_t=0;n->reload=5.f;sfx(S_SHOT,1,0);panic(n->x,n->z,16.f);if(P.inv_t<=0&&guard_sees(n,P.x,P.y,P.z,32.f)){P.hp-=22;P.react_t=.4f;set_toast("Ранение! Пробел — уклониться, укрытие спасёт");}else set_toast("Пуля просвистела мимо");}}
    else n->aim_t=fmaxf(0.f,n->aim_t-dt*2.f);
    /* ближний бой: кружат, нападают по одному с замахом (Q — контратака) */
    if(d<3.4f&&dy<1.5f&&n->aim_t<=0){float a=atan2f(dx,dz);n->yaw+=wrapa(a-n->yaw)*fminf(1,dt*10);spd=0;if(n->strafe==0)n->strafe=(i&1)?1.f:-1.f;
     if(n->stag_t<=0&&n->wind<=0&&n->swing<=0){float rr=d-2.1f;float mvx=sinf(a)*rr*1.5f+cosf(a)*n->strafe*1.1f,mvz=cosf(a)*rr*1.5f-sinf(a)*n->strafe*1.1f;if(!npc_step(n,mvx*dt,mvz*dt))n->strafe=-n->strafe;n->walk+=(1.3f*1.6f+3.f)*dt;strafe_spd=1.3f;if(frnd()<dt*.35f)n->strafe=-n->strafe;}
     if(n->wind>0){n->wind-=dt;if(n->wind<=0){n->swing=.32f;tok_cd=.55f+frnd()*.8f;if(tok_owner==i)tok_owner=-1;
       if(d<2.6f&&dy<1.4f&&P.inv_t<=0){P.hp-=15;P.react_t=.35f;sfx(S_HIT,1,0);set_toast("Удар! Q — контратака, Пробел — уклонение");}else sfx(S_SWISH,1,0);}}
     else if(tok_owner<0&&tok_cd<=0&&n->stag_t<=0&&n->open_t<=0&&n->blk_t<=0&&n->swing<=0&&P.st==PS_GND&&!race_active()){n->wind=.85f;tok_owner=i;sfx(S_WINDUP,1,0);panic(n->x,n->z,12.f);}}
    if(P.y-n->y>4.f&&d<22.f){n->t_un+=dt;if(n->t_un>6.f){n->st=GS_SEARCH;n->wait=6;n->alert=.4f;n->t_un=0;}}else n->t_un=0;
    if(!see&&n->lost>8.f){n->st=GS_SEARCH;n->wait=7;}}
   else if(n->st==GS_SEARCH){tx=n->lsx-n->x;tz=n->lsz-n->z;if(hypotf(tx,tz)<1.5f){n->wait-=dt;if(n->wait<=0){n->st=GS_PATROL;n->alert=0;}}else spd=3.0f;if(see&&n->alert>.5f)n->st=GS_SUSP;}
   if(spd>0&&(tx!=0||tz!=0)){n->yaw+=wrapa(atan2f(tx,tz)-n->yaw)*fminf(1,dt*9);if(!npc_step(n,sinf(n->yaw)*spd*dt,cosf(n->yaw)*spd*dt)&&n->st==GS_CHASE)n->lost+=dt*1.5f;n->walk+=(spd*1.6f+3.f)*dt;}n->spd=spd>0?spd:strafe_spd;}}
 /* жители не проходят друг сквозь друга */
 for(int i=0;i<NID;i++){Npc*a=&npc[i];if(!a->vis||a->racing||a->down>0)continue;for(int j=i+1;j<NID;j++){Npc*b=&npc[j];if(!b->vis||b->racing||b->down>0)continue;float dx=b->x-a->x,dz=b->z-a->z;if(fabsf(dx)>.6f||fabsf(dz)>.6f)continue;float d=hypotf(dx,dz);if(d>=.55f||d<.001f)continue;
  float p=(.55f-d)*.5f,ux=dx/d*p,uz=dz/d*p;int am=a->type==NT_CIT||a->type==NT_GUARD,bm=b->type==NT_CIT||b->type==NT_GUARD;
  if(am&&hit_box(a->x-ux,a->y,a->z-uz,.3f,1.7f,.55f)<0){a->x-=ux;a->z-=uz;}if(bm&&hit_box(b->x+ux,b->y,b->z+uz,.3f,1.7f,.55f)<0){b->x+=ux;b->z+=uz;}}}
 incrowd=cn>=3;crowd_contact(dt);}
/* ===== Лига крыш: гонки ===== */
enum{RT_SPRINT,RT_ROOFS,RT_RIVAL,RT_HUNT,RT_TOWER,RT_THIEF};
static const char*RTNAME[6]={"Спринт","Не касаясь мостовой","Дуэль","Охота за флажками","Восхождение","Погоня за вором"};
typedef struct{char k;float x,z;}GSpec;
typedef struct{const char*title;int type,giver,chapter,reward;float sx,sz;int ng;GSpec g[8];const char*intro;int night;float diff;}RaceDef;
#define NRACE 12
static const RaceDef RD[NRACE]={
 {"Первые шаги",RT_SPRINT,0,1,AB_GRAB,30,24,5,{{'G',40,20},{'G',60,40},{'G',80,47.5f},{'R',106,40},{'R',106,29}},"Жак|Новичок? Беги за мной по флажкам: улица, площадь, а потом наверх, на крышу. Лезть можно по любой стене — просто беги в неё!",0,1},
 {"Колокольня",RT_TOWER,0,1,AB_ROLL,28,24,1,{{'T',0,0}},"Жак|Видишь колокольню Сен-Жак? Кто первым наверху — тот и прав. Время пошло!",0,1},
 {"Не касаясь мостовой",RT_ROOFS,1,1,AB_TURN,66,81,5,{{'R',68,74.5f},{'R',84,74.5f},{'R',90,67},{'R',110,67},{'R',128,74.5f}},"Лиза|Правило одно: как залез на первую крышу — мостовая для тебя лава. Улицу перепрыгивай с разбега: Shift и Пробел у самого края.",0,1},
 {"Флажки Лиги",RT_HUNT,2,2,AB_TJUMP,90,54.5f,6,{{'R',70,12},{'R',88,4},{'R',108,12},{'R',128,4},{'R',108,40},{'R',124,30}},"Эмиль|Шесть флажков Лиги на крышах к северу от площади. Порядок любой, время ограничено. По моим расчётам, успеть можно!",0,1},
 {"Держи вора!",RT_THIEF,4,2,AB_STAM1,40,141,6,{{'G',40,160},{'G',20,165},{'R',12,160},{'R',12,147},{'G',20,128},{'G',40,115}},"Ворон|Видишь того карманника? Он только что срезал кошелёк у торговки. Поймаешь — поговорим всерьёз.",0,1},
 {"Прачка против курьера",RT_RIVAL,1,2,AB_STAM2,66,81,6,{{'G',100,81},{'R',110,74.5f},{'R',126,74.5f},{'R',130,67},{'G',140,60},{'G',140,40}},"Лиза|Теперь со мной наперегонки. Я бегаю с корзинами белья по этим крышам каждый день. Не отставай!",0,1.0f},
 {"Мосты Сены",RT_SPRINT,0,3,-1,60,26,8,{{'G',60,70},{'G',60,98},{'G',60,115},{'G',78,115},{'G',78,108.5f},{'G',100,109.2f},{'G',123,108.5f},{'G',122,115.5f}},"Жак|По большому мосту, по набережной и через остров у собора. Длинный забег — береги силы на спринт.",0,1},
 {"Часовая башня",RT_SPRINT,2,3,-1,90,54.5f,4,{{'G',60,62},{'G',60,100},{'G',60,125},{'T',2,0}},"Эмиль|Через реку к часовой башне у рынка — и на самый верх. Флажки покажут мост.",0,1},
 {"Крыши Сент-Антуана",RT_ROOFS,1,3,-1,148,20,6,{{'R',150,29},{'R',170,29},{'R',172,42},{'R',188,40},{'R',190,28},{'R',190,12}},"Лиза|Длинная цепочка крыш на востоке. Мостовая — лава, ты помнишь.",0,1},
 {"Дуэль с Вороном",RT_RIVAL,4,4,-1,40,141,6,{{'G',60,130},{'G',60,115},{'G',78,115},{'G',78,108.5f},{'G',76,98},{'T',1,0}},"Ворон|До северной башни собора. Ещё никто не обгонял меня на этом пути. Посмотрим, курьер.",0,1.f},
 {"Ночной забег",RT_HUNT,3,5,-1,126,140,6,{{'R',130,132},{'R',112,121},{'R',128,121},{'R',148,121},{'R',166,121},{'R',170,132}},"Тень|Ночью стража дремлет, а флажки светятся. Шесть штук вокруг поместья.",1,1},
 {"Сад маркиза",RT_TOWER,3,5,-1,126,140,1,{{'T',3,0}},"Тень|Башня в саду маркиза. Стража не спит. Наверх — и ни звука.",0,1}};
static float vpv[4][3]={{31.2f,36,31.2f},{82.5f,46,91.5f},{27,34,147},{169.4f,30,147.4f}};static int vpsync[4];
static const char*vpname[4]={"Колокольня Сен-Жак","Северная башня собора","Часовая башня рынка","Башня поместья Монфор"};
static const float TWF[4][5]={{30,30,37,38,36},{80,89,88,97,46},{26,146,32,152,34},{168,146,174,152,30}};
typedef struct{float x0,z0,x1,z1,H,top,inset;}Foot;
typedef struct{float x,y,z;int foot;}Gate;
typedef struct{Gate g[8];int ng;float sx,sz,syaw,ref,gold,silver,bronze,limit;}RaceRT;
static RaceRT rrt[NRACE];static float best_t[NRACE];static int medal[NRACE];
static Foot foot_of(int f){Foot o;if(f>=1000){const float*t=TWF[f-1000];o.x0=t[0];o.z0=t[1];o.x1=t[2];o.z1=t[3];o.H=t[4];o.top=t[4];o.inset=0;return o;}
 Bd*b=&blds[f];o.x0=b->x0;o.z0=b->z0;o.x1=b->x1;o.z1=b->z1;o.H=b->H;o.top=b->Hs;o.inset=1;return o;}
/* точка на краю следа здания, ближайшая к (px,pz); out>0 — снаружи, out<0 — внутри */
static V3 edge_pt(int f,float px,float pz,float out,float y){Foot o=foot_of(f);float cx=clampf(px,o.x0,o.x1),cz=clampf(pz,o.z0,o.z1);float nx=0,nz=0;
 if(px<o.x0||px>o.x1||pz<o.z0||pz>o.z1){if(px<o.x0)nx=-1;else if(px>o.x1)nx=1;if(pz<o.z0)nz=-1;else if(pz>o.z1)nz=1;if(nx!=0&&nz!=0){if(fabsf(px-cx)>fabsf(pz-cz))nz=0;else nx=0;}}
 else{float d[4]={px-o.x0,o.x1-px,pz-o.z0,o.z1-pz};int m=0;for(int k=1;k<4;k++)if(d[k]<d[m])m=k;if(m==0){cx=o.x0;nx=-1;}else if(m==1){cx=o.x1;nx=1;}else if(m==2){cz=o.z0;nz=-1;}else{cz=o.z1;nz=1;}}
 if(nx!=0)cx=nx<0?o.x0:o.x1;if(nz!=0)cz=nz<0?o.z0:o.z1;return v3(cx+nx*out,y,cz+nz*out);}
static V3 roof_spot(int i){Bd*b=&blds[i];float cx=(b->x0+b->x1)*.5f,cz=(b->z0+b->z1)*.5f;float best=1e9f;V3 r=v3(cx,b->Hs,cz);
 for(float dz=-3;dz<=3.01f;dz+=.5f)for(float dx=-3;dx<=3.01f;dx+=.5f){float x=cx+dx,z=cz+dz;if(x<b->x0+1.6f||x>b->x1-1.6f||z<b->z0+1.6f||z>b->z1-1.6f)continue;float y=floor_at(x,z,b->Hs+3.f,.4f,NULL);if(fabsf(y-b->Hs)>.05f)continue;if(hit_box(x,y,z,.4f,1.7f,.05f)>=0)continue;float d=dx*dx+dz*dz;if(d<best){best=d;r=v3(x,y,z);}}
 return r;}
/* путь бегуна-соперника (или вора) */
enum{WM_RUN,WM_CLIMB,WM_ARC};typedef struct{float x,y,z,t;int mode,gate;}WP;
static WP wpath[96];static int nwp;
static void wp_add(V3 p,int mode,int gate,float spd){if(nwp>=96)return;WP*w=&wpath[nwp];w->x=p.x;w->y=p.y;w->z=p.z;w->mode=mode;w->gate=gate;w->t=0;
 if(nwp>0){WP*q=&wpath[nwp-1];float dh=hypotf(p.x-q->x,p.z-q->z),dy=fabsf(p.y-q->y);w->t=mode==WM_CLIMB?dy/(3.1f*spd):mode==WM_ARC?fmaxf(.45f,dh/(8.2f*spd))+dy*.03f:dh/(6.9f*spd);}nwp++;}
static void plan_seg(V3 a,int fa,V3 b,int fb,int gate,float spd){
 if(fb<0){if(fa<0){wp_add(b,WM_RUN,gate,spd);return;}
  V3 e=edge_pt(fa,b.x,b.z,-(foot_of(fa).inset+.2f),a.y);wp_add(e,WM_RUN,-1,spd);V3 o=edge_pt(fa,b.x,b.z,1.6f,0);o.y=floor_at(o.x,o.z,.5f,.2f,NULL);wp_add(o,WM_ARC,-1,spd);wp_add(b,WM_RUN,gate,spd);return;}
 if(fa==fb){wp_add(b,WM_RUN,gate,spd);return;}
 Foot B=foot_of(fb);
 if(fa<0){V3 f=edge_pt(fb,a.x,a.z,.38f,a.y);wp_add(f,WM_RUN,-1,spd);
  if(B.inset>0){f.y=B.H;wp_add(f,WM_CLIMB,-1,spd);V3 i=edge_pt(fb,a.x,a.z,-.62f,B.H);wp_add(i,WM_RUN,-1,spd);i.y=B.top;wp_add(i,WM_CLIMB,-1,spd);}
  else{f.y=B.top;wp_add(f,WM_CLIMB,-1,spd);V3 i=edge_pt(fb,a.x,a.z,-.6f,B.top);wp_add(i,WM_RUN,-1,spd);}
  wp_add(b,WM_RUN,gate,spd);return;}
 Foot A=foot_of(fa);V3 pa=edge_pt(fa,b.x,b.z,-(A.inset+.2f),a.y);wp_add(pa,WM_RUN,-1,spd);
 if(B.top<=a.y+.3f){V3 q=edge_pt(fb,a.x,a.z,-(B.inset+.4f),B.top);wp_add(q,WM_ARC,-1,spd);}
 else if(B.inset>0&&B.H<a.y){V3 q=edge_pt(fb,pa.x,pa.z,-.62f,B.H);wp_add(q,WM_ARC,-1,spd);q.y=B.top;wp_add(q,WM_CLIMB,-1,spd);}
 else{V3 q=edge_pt(fb,pa.x,pa.z,.36f,a.y);wp_add(q,WM_ARC,-1,spd);q.y=B.inset>0?B.H:B.top;wp_add(q,WM_CLIMB,-1,spd);if(B.inset>0){V3 i=edge_pt(fb,pa.x,pa.z,-.62f,B.H);wp_add(i,WM_RUN,-1,spd);i.y=B.top;wp_add(i,WM_CLIMB,-1,spd);}}
 wp_add(b,WM_RUN,gate,spd);}
static float plan_race(int r,float spd){RaceRT*R=&rrt[r];nwp=0;V3 a=v3(R->sx,floor_at(R->sx,R->sz,.5f,.2f,NULL),R->sz);int fa=-1;wp_add(a,WM_RUN,-1,spd);
 for(int k=0;k<R->ng;k++){Gate*g=&R->g[k];V3 b=v3(g->x,g->y,g->z);plan_seg(a,fa,b,g->foot,k,spd);a=b;fa=g->foot;}
 float T=0;for(int k=1;k<nwp;k++)T+=wpath[k].t;return T;}
static void races_init(void){
 for(int r=0;r<NRACE;r++){const RaceDef*d=&RD[r];RaceRT*R=&rrt[r];R->ng=d->ng;R->sx=d->sx;R->sz=d->sz;fix_spot(&R->sx,&R->sz);
  for(int k=0;k<d->ng;k++){const GSpec*s=&d->g[k];Gate*g=&R->g[k];
   if(s->k=='T'){int t=(int)s->x;g->x=vpv[t][0];g->y=vpv[t][1];g->z=vpv[t][2];g->foot=1000+t;}
   else if(s->k=='R'){int b=bld_at(s->x,s->z);if(b<0){g->x=s->x;g->z=s->z;g->y=floor_at(s->x,s->z,.5f,.2f,NULL);g->foot=-1;}else{V3 p=roof_spot(b);g->x=p.x;g->y=p.y;g->z=p.z;g->foot=b;}}
   else{float x=s->x,z=s->z;fix_spot(&x,&z);g->x=x;g->z=z;g->y=floor_at(x,z,.5f,.2f,NULL);g->foot=-1;}}
  R->syaw=atan2f(R->g[0].x-R->sx,R->g[0].z-R->sz);
  if(d->type==RT_HUNT){/* порядок «ближайший следующий» — так бегут люди */Gate tmp[8];int used[8]={0};float cx=R->sx,cz=R->sz;for(int k=0;k<R->ng;k++){int bi=0;float bd=1e9f;for(int q=0;q<R->ng;q++)if(!used[q]){float dd=hypotf(R->g[q].x-cx,R->g[q].z-cz);if(dd<bd){bd=dd;bi=q;}}used[bi]=1;tmp[k]=R->g[bi];cx=tmp[k].x;cz=tmp[k].z;}memcpy(R->g,tmp,sizeof(Gate)*R->ng);}
  float T=plan_race(r,1.f);R->ref=T+.6f*R->ng;int lng=(d->type==RT_SPRINT||d->type==RT_ROOFS);R->gold=R->ref*(lng?1.25f:1.18f);R->silver=R->ref*(lng?1.6f:1.5f);R->bronze=R->ref*(d->type==RT_HUNT?2.3f:lng?2.2f:2.0f);R->limit=R->bronze;
  if(d->type==RT_RIVAL)R->limit=1e9f;if(d->type==RT_THIEF)R->limit=1e9f;}}
/* состояние гонки */
enum{RS_NONE,RS_COUNT,RS_RUN,RS_RESULT};
static struct{int id,st,next,got[8],left,ok,new_medal,rival,runner,rwp;float t,count,rt,rpos,rtotal,result_t,prev_tod;int pnew[16],npnew,abnew,fail;char why[120];int night_set;}RC;
static int race_active(void){return RC.st==RS_RUN||RC.st==RS_COUNT;}
static int roof_rule(void){return RC.st==RS_RUN&&RD[RC.id].type==RT_ROOFS&&RC.next>=1;}
static int race_stealth(void){return race_active()&&RC.id==11;}
static int chapter=0,ending_done,pending_chapter;
static int race_open(int r){return RD[r].chapter<=chapter;}
static int npc_of_runner(int k){return runner_id[k];}
static void race_place_runner(void){Npc*n=&npc[RC.runner];WP*w=&wpath[0];n->x=w->x;n->y=w->y;n->z=w->z;n->racing=1;n->down=n->getup=n->bump_t=0;}
static void race_begin(int r){const RaceDef*d=&RD[r];RaceRT*R=&rrt[r];memset(&RC,0,sizeof RC);RC.id=r;RC.st=RS_COUNT;RC.count=3.4f;RC.left=R->ng;RC.runner=-1;
 player_reset(R->sx,floor_at(R->sx,R->sz,.5f,.2f,NULL),R->sz,R->syaw);P.fade=1.f;input_lock=1;
 if(d->night){RC.prev_tod=tod;RC.night_set=1;tod=23.2f;}
 for(int i=0;i<NID;i++){Npc*n=&npc[i];if(n->type==NT_GUARD&&n->st!=GS_KO){n->st=GS_PATROL;n->alert=0;}}
 if(d->type==RT_RIVAL||d->type==RT_THIEF){float spd=d->type==RT_THIEF?.84f:1.f;
  RC.runner=d->type==RT_THIEF?thief_id:npc_of_runner(d->giver);
  float T=plan_race(r,spd);float want=d->type==RT_THIEF?R->ref*.95f:R->ref*1.32f*d->diff;float k=T/want;for(int i=1;i<nwp;i++)wpath[i].t/=k;RC.rtotal=want;
  if(d->type==RT_THIEF){/* вор стартует на 7 м впереди */float ox=wpath[1].x-wpath[0].x,oz=wpath[1].z-wpath[0].z,l=hypotf(ox,oz);if(l>12.5f){float t1=wpath[1].t;wpath[0].x+=ox/l*12.f;wpath[0].z+=oz/l*12.f;wpath[1].t=t1*(l-12.f)/l;}}
  race_place_runner();}
 mus_mode=MUS_RACE;sfx(S_COUNT,1,72);}
static void award_portrait(int id){if(id<0||id>=NID||alb[id]==2)return;alb[id]=2;if(RC.npnew<16)RC.pnew[RC.npnew++]=id;}
static int pool_next(void){/* случайный ещё не написанный портрет из общего пула */int cand[NID],n=0;for(int i=0;i<NID;i++){if(alb[i]==2)continue;if(i==painter_id||i==thief_id)continue;int rr=0;for(int k=0;k<NRUN;k++)if(runner_id[k]==i)rr=1;if(rr)continue;cand[n++]=i;}
 if(!n)return -1;return cand[rand()%n];}
static void save_game(void);
static void chapter_check(void);
static void race_finish(int ok,const char*why){RaceRT*R=&rrt[RC.id];const RaceDef*d=&RD[RC.id];RC.st=RS_RESULT;RC.ok=ok;RC.result_t=0;RC.npnew=0;RC.abnew=-1;input_lock=1;
 if(why)snprintf(RC.why,sizeof RC.why,"%s",why);
 if(RC.night_set){tod=RC.prev_tod;RC.night_set=0;}
 if(RC.runner>=0){npc[RC.runner].racing=0;Npc*n=&npc[RC.runner];n->x=n->hx;n->z=n->hz;n->y=floor_at(n->x,n->z,.5f,.2f,NULL);if(RC.runner==thief_id){n->down=0;}}
 if(!ok){sfx(S_FAIL,1,0);return;}
 int m=0;float t=RC.t;if(d->type==RT_THIEF){float q=t/RC.rtotal;m=q<.5f?3:q<.75f?2:1;}else m=t<=R->gold?3:t<=R->silver?2:1;
 RC.new_medal=m;int old=medal[RC.id];if(!best_t[RC.id]||t<best_t[RC.id])best_t[RC.id]=t;
 static const int PER[4]={0,3,3,4};for(int tier=old+1;tier<=m;tier++)for(int k=0;k<PER[tier];k++)award_portrait(pool_next());
 if(m>old)medal[RC.id]=m;
 if(old==0&&d->reward>=0&&!ab[d->reward]){ab[d->reward]=1;RC.abnew=d->reward;}
 sfx(S_FANFARE,1,0);if(RC.npnew)sfx(S_COLLECT,1,0);pending_chapter=1;save_game();}
static void race_fail(const char*why){if(RC.st==RS_RUN)race_finish(0,why);}
static void race_abort_silent(void){if(RC.st==RS_NONE)return;if(RC.night_set){tod=RC.prev_tod;RC.night_set=0;}if(RC.runner>=0){Npc*n=&npc[RC.runner];n->racing=0;n->x=n->hx;n->z=n->hz;n->y=floor_at(n->x,n->z,.5f,.2f,NULL);}RC.st=RS_NONE;RC.runner=-1;input_lock=0;}
static void race_quit(void){if(RC.st==RS_RUN||RC.st==RS_COUNT){race_finish(0,"Гонка прервана");}}
static void runner_update(float dt){if(RC.runner<0)return;Npc*n=&npc[RC.runner];RC.rt+=dt;
 while(RC.rwp+1<nwp&&RC.rpos>=wpath[RC.rwp+1].t){RC.rpos-=wpath[RC.rwp+1].t;RC.rwp++;}
 if(RC.rwp+1>=nwp){WP*w=&wpath[nwp-1];n->x=w->x;n->y=w->y;n->z=w->z;n->spd=0;return;}
 RC.rpos+=dt;WP*a=&wpath[RC.rwp],*b=&wpath[RC.rwp+1];float u=b->t>0?clampf(RC.rpos/b->t,0,1):1;
 float x=lf(a->x,b->x,u),z=lf(a->z,b->z,u),y=lf(a->y,b->y,u);if(b->mode==WM_ARC){float dh=hypotf(b->x-a->x,b->z-a->z);y+=sinf(u*3.14159f)*fminf(3.f,.8f+.15f*dh);}
 if(b->mode!=WM_CLIMB&&(fabsf(b->x-a->x)+fabsf(b->z-a->z))>.05f)n->yaw+=wrapa(atan2f(b->x-a->x,b->z-a->z)-n->yaw)*fminf(1,dt*12);
 n->x=x;n->y=y;n->z=z;n->st=b->mode;n->spd=b->mode==WM_RUN?6.9f:b->mode==WM_CLIMB?2.f:5.f;n->walk+=(n->spd*1.6f+3.f)*dt;
 if(b->mode==WM_CLIMB){n->yaw=atan2f(-(b->x-a->x)+1e-4f,-(b->z-a->z));}}
static int rival_gate(void){int g=-1;for(int k=0;k<=RC.rwp&&k<nwp;k++)if(wpath[k].gate>g)g=wpath[k].gate;return g;}
static void race_update(float dt){if(RC.st==RS_NONE)return;const RaceDef*d=&RD[RC.id];RaceRT*R=&rrt[RC.id];
 if(RC.st==RS_COUNT){int before=(int)ceilf(RC.count-.4f);RC.count-=dt;int after=(int)ceilf(RC.count-.4f);if(after!=before&&before>=1&&before<=3)sfx(S_COUNT,1,after>=1?72:79);
  if(RC.count<=.4f){RC.st=RS_RUN;input_lock=0;sfx(S_COUNT,1,79);sfx(S_START,1,0);}return;}
 if(RC.st==RS_RESULT){RC.result_t+=dt;return;}
 RC.t+=dt;
 if(d->type==RT_HUNT){for(int k=0;k<R->ng;k++){if(RC.got[k])continue;Gate*g=&R->g[k];if(hypotf(P.x-g->x,P.z-g->z)<2.4f&&fabsf(P.y-g->y)<2.6f){RC.got[k]=1;RC.left--;sfx(S_GATE,1,0);set_toast("Флажок! Осталось %d",RC.left);}}
  if(RC.left<=0){race_finish(1,NULL);return;}}
 else if(d->type==RT_THIEF){Npc*n=&npc[RC.runner];runner_update(dt);float dd=sqrtf((n->x-P.x)*(n->x-P.x)+(n->z-P.z)*(n->z-P.z)+(n->y-P.y)*(n->y-P.y)*.5f);
  if(dd<1.5f){bark(n,"Пусти, пусти! Всё отдам!");race_finish(1,NULL);return;}
  if(RC.rwp+1>=nwp){race_finish(0,"Вор ушёл.");return;}
  Gate*g=&R->g[RC.next<R->ng?RC.next:R->ng-1];(void)g;}
 else{if(RC.runner>=0)runner_update(dt);
  Gate*g=&R->g[RC.next];if(hypotf(P.x-g->x,P.z-g->z)<2.5f&&fabsf(P.y-g->y)<(d->type==RT_TOWER?2.2f:2.8f)){RC.next++;sfx(S_GATE,1,0);
   if(RC.next>=R->ng){if(d->type==RT_RIVAL&&RC.rwp+1>=nwp){race_finish(0,"Соперник пришёл первым.");return;}race_finish(1,NULL);return;}}
  if(d->type==RT_RIVAL&&RC.rwp+1>=nwp){race_finish(0,"Соперник пришёл первым.");return;}}
 if(RC.t>R->limit){race_finish(0,"Время вышло.");return;}}
/* ===== сюжет, диалоги, сохранение ===== */
enum{GM_TITLE,GM_PLAY,GM_PAUSE,GM_DIALOG,GM_HELP,GM_ALBUM,GM_RACEMENU,GM_RESULT};static int gmode=GM_TITLE,menu_sel,help_back,album_back;
#define DLMAX 12
static const char*dl[DLMAX];static int dl_n,dl_i;static void(*dl_done)(void);
static void dialog(const char**lines,int n,void(*done)(void)){if(n>DLMAX)n=DLMAX;for(int i=0;i<n;i++)dl[i]=lines[i];dl_n=n;dl_i=0;dl_done=done;gmode=GM_DIALOG;}
static const char*D_PRO[]={"Мэтр Огюстен|Ах, молодой человек! Вы ведь Адриен, курьер? Я Огюстен Брюн, художник. Я задумал великое: «Лица квартала», портреты всех ста двадцати восьми соседей.",
 "Мэтр Огюстен|Но парижане не желают позировать безвестному мазиле. Им подавай знаменитость! А кто нынче знаменит в квартале? Лига крыш: сорвиголовы, что носятся по черепице наперегонки.",
 "Мэтр Огюстен|Станьте звездой Лиги, и квартал сам выстроится ко мне в очередь. За каждую вашу победу я напишу новые портреты. Альбом всегда при вас: клавиша Tab.",
 "Мэтр Огюстен|Начните с Жака-Трубочиста у церковной площади: он учит новичков. А главный в Лиге — Ворон. Говорят, его ещё никто не обгонял.",
 "Подсказка|Звёзды на карте и над головами — бегуны Лиги. Подойдите и нажмите E. Говорить можно с любым жителем: так вы с ним познакомитесь."};
static const char*D_CH2[]={"Записка на трубе|«Новичок неплох. Хочешь в Лигу по-настоящему — поймай карманника у рынка, а потом обгони Лизу. — В.»","Мэтр Огюстен|Весь рынок только о вас и судачит! Прачка Лиза и студент Эмиль согласились позировать. Новые бегуны ждут вас."};
static const char*D_CH3[]={"Эмиль|Ворон заметил тебя. Он такое редко говорит, но сказал: «Курьер бегает почти как я». Почти!","Мэтр Огюстен|Тень и даже тот карманник пришли ко мне сами. Три последних испытания — и Ворон примет вызов."};
static const char*D_CH4[]={"Ворон|Ладно, курьер. Дуэль: от рынка до северной башни собора. Жду тебя у рынка, если не струсишь."};
static const char*D_END[]={"Ворон|…Обогнал. Меня. На моём собственном пути. Ну что ж, Лига крыш теперь и твоя, Адриен.","Мэтр Огюстен|Браво! Весь квартал хочет попасть на мои холсты. Собирайте портреты дальше: гонки можно проходить снова, ради лучших медалей.",
 "Тень|А для тех, кто не боится темноты и стражи, у меня есть ещё два забега. Найдёшь меня у поместья маркиза.","Конец главной истории|Спасибо за игру! Париж открыт: портреты, медали, башни и сено ждут."};
static void save_game(void){FILE*f=fopen("teni_save.dat","wb");if(!f)return;struct{int magic,ver,chapter,ending,a[AB_N],m[NRACE],v[4];float b[NRACE],tod,x,y,z,yaw;unsigned char al[NID];}s;memset(&s,0,sizeof s);
 s.magic=0x5445;s.ver=2;s.chapter=chapter;s.ending=ending_done;memcpy(s.a,ab,sizeof ab);memcpy(s.m,medal,sizeof medal);memcpy(s.v,vpsync,sizeof vpsync);memcpy(s.b,best_t,sizeof best_t);s.tod=tod;
 if(race_active()){s.x=rrt[RC.id].sx;s.z=rrt[RC.id].sz;s.y=0;}else{s.x=P.safex;s.y=P.safey;s.z=P.safez;}s.yaw=P.yaw;memcpy(s.al,alb,sizeof alb);fwrite(&s,sizeof s,1,f);fclose(f);}
static int load_game(int apply){FILE*f=fopen("teni_save.dat","rb");if(!f)return 0;struct{int magic,ver,chapter,ending,a[AB_N],m[NRACE],v[4];float b[NRACE],tod,x,y,z,yaw;unsigned char al[NID];}s;
 int ok=fread(&s,sizeof s,1,f)==1&&s.magic==0x5445&&s.ver==2;fclose(f);if(!ok||!apply)return ok;
 chapter=s.chapter;ending_done=s.ending;memcpy(ab,s.a,sizeof ab);memcpy(medal,s.m,sizeof medal);memcpy(vpsync,s.v,sizeof vpsync);memcpy(best_t,s.b,sizeof best_t);tod=s.tod;memcpy(alb,s.al,sizeof alb);
 for(int i=0;i<NID;i++)if(alb[i]>2)alb[i]=0;player_reset(s.x,s.y,s.z,s.yaw);return 1;}
static int count_alb(int v){int n=0;for(int i=0;i<NID;i++)if(alb[i]>=v)n++;return n;}
static int chapter_done(int c){for(int r=0;r<NRACE;r++)if(RD[r].chapter==c&&!medal[r])return 0;return 1;}
static void after_story(void){gmode=GM_PLAY;save_game();}
static void story_pro_done(void){chapter=1;alb[painter_id]=2;alb[runner_id[0]]=2;set_toast("В альбоме 2 портрета: Мэтр Огюстен и Жак-Трубочист (Tab)");after_story();}
static void chapter_check(void){
 if(chapter==1&&chapter_done(1)){chapter=2;alb[runner_id[1]]=2;alb[runner_id[2]]=2;set_toast("Глава 2. Новые портреты: Лиза и Эмиль");dialog(D_CH2,2,after_story);return;}
 if(chapter==2&&chapter_done(2)){chapter=3;alb[runner_id[3]]=2;if(thief_id>=0)alb[thief_id]=2;set_toast("Глава 3. Новые портреты: Тень и карманник");dialog(D_CH3,2,after_story);return;}
 if(chapter==3&&chapter_done(3)){chapter=4;int p=pool_next();if(p>=0)alb[p]=2;set_toast("Финал открыт. Новый портрет в альбоме");dialog(D_CH4,1,after_story);return;}
 if(chapter==4&&medal[9]){chapter=5;ending_done=1;alb[runner_id[4]]=2;dialog(D_END,4,after_story);return;}}
/* меню гонок у бегуна */
static int rm_giver,rm_list[NRACE],rm_n;
static void open_race_menu(int giver){rm_giver=giver;rm_n=0;for(int r=0;r<NRACE;r++)if(RD[r].giver==giver)rm_list[rm_n++]=r;menu_sel=0;for(int k=0;k<rm_n;k++)if(race_open(rm_list[k])&&!medal[rm_list[k]]){menu_sel=k;break;}gmode=GM_RACEMENU;}
static int pending_race=-1;static const char*intro_lines[2];
static void start_pending(void){gmode=GM_PLAY;if(pending_race>=0)race_begin(pending_race);pending_race=-1;}
static void npc_talk(int i){Npc*n=&npc[i];Ident*d=&ids[i];int fresh=alb[i]==0;if(alb[i]==0)alb[i]=1;
 if(n->type==NT_RUNNER){int g=0;for(int k=0;k<NRUN;k++)if(runner_id[k]==i)g=k;open_race_menu(g);return;}
 if(n->type==NT_PAINTER){static char l1[300];static const char*L[2];snprintf(l1,sizeof l1,"Мэтр Огюстен|Портретов написано: %d из 128. Знакомых у вас в квартале: %d. Побеждайте в Лиге — и холсты не будут пустовать!",count_alb(2),count_alb(1));L[0]=l1;dialog(L,1,after_story);return;}
 if(n->type==NT_GUARD)bark(n,n->st==GS_CHASE?"Ты арестован!":"Проходи, не задерживайся.");
 else bark(n,RUMOR[d->cls][hs(i,(int)g_time)%3]);
 n->look_t=2.5f;if(fresh)set_toast("Новое знакомство: %s, %s",d->name,d->job);}
static void respawn(const char*why){if(race_active()&&RC.st==RS_RUN){race_finish(0,why);P.hp=100;return;}P.hp=100;P.x=P.safex;P.y=P.safey;P.z=P.safez;P.vx=P.vy=P.vz=0;P.st=PS_GND;P.fade=1.f;set_toast("%s",why);
 for(int i=0;i<NID;i++)if(npc[i].type==NT_GUARD&&npc[i].st!=GS_KO){npc[i].st=GS_PATROL;npc[i].alert=0;}}
/* ===== свет, небо, погода ===== */
static float weather_cloud=.3f,weather_rain=0,weather_fog=0,weather_wet=0;
static void get_light(float*night,float*sun){float h=tod,n;if(h<5)n=1;else if(h<7)n=1-(h-5)/2;else if(h<18.5f)n=0;else if(h<21)n=(h-18.5f)/2.5f;else n=1;if(n>1)n=1;if(n<0)n=0;*night=n;
 float s1=1-fabsf(h-18.4f)/1.7f,s2=1-fabsf(h-6.4f)/1.3f;*sun=fmaxf(0,fmaxf(s1,s2));}
static V3 sunvec(void){float ang=(tod-6)/12.f*3.14159f;return vnrm(v3(-cosf(ang)*.75f,sinf(ang),.42f));}
static float lerp3(float*o,const float*a,const float*b,float t){for(int c=0;c<3;c++)o[c]=a[c]+(b[c]-a[c])*t;return 0;}
static void set_env(float night,float sun){
 static const float sunD[3]={2.7f,2.5f,2.25f},sunS[3]={2.5f,1.25f,.52f},moon[3]={.05f,.068f,.11f};
 static const float zD[3]={.16f,.34f,.78f},zS[3]={.22f,.25f,.48f},zN[3]={.006f,.009f,.026f};
 static const float hD[3]={.62f,.74f,.92f},hS[3]={1.05f,.58f,.36f},hN[3]={.022f,.03f,.06f};
 static const float gD[3]={.26f,.24f,.21f},gN[3]={.012f,.012f,.016f};
 float t[3],u[3];V3 sv=sunvec();
 lerp3(t,sunD,sunS,sun);float dayk=clampf(sv.y*4.f,0,1);for(int c=0;c<3;c++)t[c]*=dayk;lerp3(u,t,moon,night);
 float cl=weather_cloud;for(int c=0;c<3;c++)u[c]*=1.f-.82f*cl*cl;memcpy(r3e.sun,u,sizeof u);
 lerp3(t,zD,zS,sun);lerp3(r3e.sky_zen,t,zN,night);lerp3(t,hD,hS,sun);lerp3(r3e.sky_hor,t,hN,night);lerp3(r3e.ground,gD,gN,night);
 for(int c=0;c<3;c++){float g=(r3e.sky_zen[c]+r3e.sky_hor[c])*.5f;r3e.sky_zen[c]+= (g*.8f-r3e.sky_zen[c])*cl*.7f;r3e.sky_hor[c]+=(g*.9f-r3e.sky_hor[c])*cl*.6f;}
 V3 mv=vnrm(v3(.45f,.62f,-.65f));r3e.L=night>.55f?mv:(sv.y>.04f?sv:vnrm(v3(sv.x,.04f,sv.z)));r3e.moon=mv;
 r3e.night=night;r3e.cloud=cl;r3e.rain=weather_rain;r3e.wet=weather_wet;r3e.time=g_time;r3e.wind=.3f+cl*.6f+weather_rain*.6f;
 r3e.fog_dens=.0028f+weather_fog*.022f+weather_rain*.006f+night*.0015f;r3e.fog_height=.045f;
 r3e.exposure=lf(lf(.66f,.82f,sun),1.7f,night)*(1.f+cl*.3f);r3e.emis=fmaxf(night,cl*.35f+weather_rain*.3f)*1.0f+.06f;r3e.smooth=1;
 /* фонари: ближайшие к камере */
 r3e.nlights=0;if(night>.15f){float lit=clampf((night-.15f)*2.f,0,1);int idx[R3_MAXL];float dd[R3_MAXL];int n=0;
  for(int i=0;i<nlamp;i++){float d=hypotf(lampv[i][0]-r3e.eye.x,lampv[i][1]-r3e.eye.z);if(d>90)continue;if(n<R3_MAXL){idx[n]=i;dd[n]=d;n++;}else{int w=0;for(int k=1;k<n;k++)if(dd[k]>dd[w])w=k;if(d<dd[w]){idx[w]=i;dd[w]=d;}}}
  for(int k=0;k<n;k++){float*L=r3e.light[k];int i=idx[k];float fl=.92f+.08f*sinf(g_time*9.f+i*1.7f)*sinf(g_time*13.3f+i);L[0]=lampv[i][0];L[1]=4.6f;L[2]=lampv[i][1];L[3]=14.f;L[4]=1.f;L[5]=.62f;L[6]=.28f;L[7]=6.f*lit*fl;}r3e.nlights=n;}}
static const float flagv[7][3]={{82.2f,46,91.2f},{82.2f,46,105.4f},{31.2f,36,36},{27,34,147.3f},{169.4f,30,147.4f},{160,17.4f,152},{100,20,98}};
static void draw_flags(float t,V3 eye){mb_begin();float u0,v0,u1,v1;tile_uv(TL_WHITE,&u0,&v0,&u1,&v1);uint32_t cc[3]={RGBX(40,70,170),RGBX(246,246,250),RGBX(200,36,48)};
 for(int f=0;f<7;f++){float x=flagv[f][0],y=flagv[f][1],z=flagv[f][2];if(hypotf(x-eye.x,z-eye.z)>160)continue;mb_tube(v3(x,y,z),v3(x,y+4.4f,z),.05f,.05f,5,RGBX(60,50,40));
  for(int c=0;c<9;c++){float xa=c*.27f,xb=xa+.27f,za=sinf(t*5-xa*3)*.22f*xa/2.4f,zb=sinf(t*5-xb*3)*.22f*xb/2.4f;mb_quad(v3(x+xa+.05f,y+2.9f,z+za),v3(x+xb+.05f,y+2.9f,z+zb),v3(x+xb+.05f,y+4.3f,z+zb),v3(x+xa+.05f,y+4.3f,z+za),v3(0,0,1),u0,v0,u1,v1,cc[c/3]);}}
 Mesh m=mb_temp();r3_draw(&m,m_ident(),&atlas,R3_TWOSIDED,0);}
/* ===== отрисовка мира ===== */
static float amp_of(float spd){return spd<.05f?0:fminf(1.1f,.15f+spd/8.5f);}
static void draw_npc(int i,V3 eye){Npc*n=&npc[i];float d=hypotf(n->x-eye.x,n->z-eye.z);if(d>80||!r3_visible(v3(n->x,n->y+.9f,n->z),1.4f))return;
 Pose q=pose0(n->walk,amp_of(n->spd));float yy=n->y;
 if(n->racing){if(n->st==WM_CLIMB){q.climb=1;q.amp=0;}else if(n->st==WM_ARC){q.air=1;}else q.amp=1.f;}
 if(n->down>0||(n->type==NT_GUARD&&n->st==GS_KO)){DM_lay=-1.5f;yy+=.25f;q.amp=0;}
 else if(n->getup>0){float u=n->getup/.7f;DM_lay=-1.5f*u;yy+=.25f*u;q.crouch=u*.6f;}
 if(n->bump_t>0)q.bump=fminf(1.f,n->bump_t/.4f);
 {float dx=P.x-n->x,dz=P.z-n->z,dp=hypotf(dx,dz);q.tail=fminf(1.f,n->spd/6.f)*.6f;q.sway=sinf(n->walk*.5f)*.15f*fminf(1.f,n->spd/3.f);
  if(dp<6.f&&n->down<=0&&!n->racing){float rl=wrapa(atan2f(dx,dz)-n->yaw);q.look=clampf(rl,-1.1f,1.1f)*(fabsf(rl)<2.2f?1.f:0.f);}
  if(n->type==NT_GUARD&&n->st!=GS_KO){int fight=n->st==GS_CHASE&&dp<5.f;q.sword=fight?1.f:0.f;
   if(n->wind>0)q.atk=.32f*(1.f-n->wind/.85f);else if(n->swing>0)q.atk=.35f+.65f*(1.f-n->swing/.32f);
   if(n->blk_t>0)q.blk=1;if(n->stag_t>0)q.react=fminf(1.f,n->stag_t/.4f);if(n->aim_t>0)q.aim=fminf(1.f,n->aim_t*2.f);}
  if(n->type==NT_CIT&&n->flee_t>0)q.amp=amp_of(3.6f);}
 {static float lt[NID];float k=fminf(1.f,(g_time-lt[i])*10.f);if(g_time-lt[i]>.3f)k=1;lt[i]=g_time;pose_blend(&n->ps,&q,k);n->ps.ph=q.ph;}
 draw_model(&mdl_npc[i],n->x,yy,n->z,n->yaw,&n->ps,g_time+i*.37f,d<26,1.f);
 /* мушкетёр целится: вспышка выстрела */
 if(n->type==NT_GUARD&&n->reload>4.85f){mb_begin();float fx=n->x+sinf(n->yaw)*1.2f,fz=n->z+cosf(n->yaw)*1.2f;mb_sphere(v3(fx,n->y+1.45f,fz),v3(.25f,.25f,.25f),8,6,RGBX(255,220,140),0);Mesh m=mb_temp();r3_draw(&m,m_ident(),NULL,R3_NOLIGHT|R3_ADD,200);}}
static void build_race_marks(V3 eye){if(RC.st!=RS_RUN&&RC.st!=RS_COUNT)return;const RaceDef*d=&RD[RC.id];RaceRT*R=&rrt[RC.id];
 mb_begin();for(int k=0;k<R->ng;k++){int show=0;uint32_t col=RGBX(255,214,90);
  if(d->type==RT_HUNT){show=!RC.got[k];col=RGBX(230,60,60);}else if(d->type==RT_THIEF)show=0;else{show=(k==RC.next||k==RC.next+1);if(k==RC.next+1)col=RGBX(220,220,230);}
  if(!show)continue;Gate*g=&R->g[k];if(hypotf(g->x-eye.x,g->z-eye.z)>200)continue;
  if(d->type==RT_HUNT){mb_tube(v3(g->x,g->y,g->z),v3(g->x,g->y+2.6f,g->z),.04f,.04f,6,RGBX(70,50,40));float w=sinf(g_time*4+k)*.12f;mb_tri(v3(g->x,g->y+2.55f,g->z),v3(g->x+1.1f,g->y+2.25f,g->z+w),v3(g->x,g->y+1.95f,g->z),col);mb_tri(v3(g->x,g->y+2.55f,g->z),v3(g->x,g->y+1.95f,g->z),v3(g->x+1.1f,g->y+2.25f,g->z+w),col);}
  else{float px=k>0?R->g[k-1].x:R->sx,pz=k>0?R->g[k-1].z:R->sz,a=atan2f(g->x-px,g->z-pz),ca=cosf(a),sa=sinf(a),rr=1.35f+.08f*sinf(g_time*3);
   for(int s=0;s<16;s++){float t0=6.2831853f*s/16,t1=6.2831853f*(s+1)/16;V3 p0=v3(g->x+ca*cosf(t0)*rr,g->y+1.4f+sinf(t0)*rr,g->z-sa*cosf(t0)*rr),p1=v3(g->x+ca*cosf(t1)*rr,g->y+1.4f+sinf(t1)*rr,g->z-sa*cosf(t1)*rr);mb_tube(p0,p1,.07f,.07f,5,col);}}}
 {Mesh m=mb_temp();r3_draw(&m,m_ident(),NULL,R3_NOLIGHT,0);}
 mb_begin();for(int k=0;k<R->ng;k++){int show=d->type==RT_HUNT?!RC.got[k]:(d->type!=RT_THIEF&&k==RC.next);if(!show)continue;Gate*g=&R->g[k];mb_cyl(v3(g->x,g->y,g->z),.22f,.22f,45,8,d->type==RT_HUNT?RGBX(255,120,90):RGBX(255,226,120));}
 {Mesh m=mb_temp();r3_draw(&m,m_ident(),NULL,R3_ALPHA|R3_NOLIGHT,70);}}
static void draw_scene(V3 eye,V3 at,int with_player){
 float night,sun;get_light(&night,&sun);night_b=night>.5f;r3e.eye=eye;set_env(night,sun);
 r3_camera(eye,at,1.0f,(float)SW/SH,.25f,420.f);
 {/* кварталы от ближнего к дальнему */for(int c=0;c<NCH*NCH;c++){float cx=(c%NCH)*CHS+CHS*.5f,cz=(c/NCH)*CHS+CHS*.5f;float d=hypotf(cx-eye.x,cz-eye.z);if(d>380)continue;if(!r3_visible(v3(cx,15,cz),CHS*.75f+25))continue;r3_draw(&chunk_m[c],m_ident(),&atlas,0,0);if(chunk_f[c].nv&&d<260)r3_draw(&chunk_f[c],m_ident(),&leaves,R3_CUTOUT|R3_WIND|R3_TWOSIDED,0);}}
 r3_draw(&plank_m,m_ident(),&atlas,0,0);
 {mb_begin();float u0,v0,u1,v1;tile_uv(TL_WHITE,&u0,&v0,&u1,&v1);for(float z=84;z<112;z+=14)for(float x=0;x<WORLD;x+=20){if(!r3_visible(v3(x+10,-3,z+7),12))continue;mb_quad(v3(x,-2.9f,z+14),v3(x+20,-2.9f,z+14),v3(x+20,-2.9f,z),v3(x,-2.9f,z),v3(0,1,0),u0,v0,u1,v1,RGBX(40,70,80));}
  for(int b=0;b<nbasin;b++){Basin*B=&basins[b];if(hypotf(B->x-eye.x,B->z-eye.z)>150)continue;int base=mb_count();float uc=(u0+u1)*.5f,vc=(v0+v1)*.5f;mb_vert(B->x,B->y,B->z,0,1,0,uc,vc,RGBX(40,70,80));for(int k=0;k<=24;k++){float a=k*6.2831853f/24;mb_vert(B->x+sinf(a)*B->r,B->y,B->z+cosf(a)*B->r,0,1,0,uc,vc,RGBX(40,70,80));}for(int k=0;k<24;k++)mb_index(base,base+1+k,base+2+k);}
  Mesh wm=mb_temp();r3_draw(&wm,m_ident(),&atlas,R3_WATER,0);}
 draw_flags(g_time,eye);build_race_marks(eye);
 for(int i=0;i<NID;i++){if(!npc[i].vis)continue;draw_npc(i,eye);}
 if(with_player&&P.st!=PS_HIDE){float s=hypotf(P.vx,P.vz);Pose q=pose0(P.walk,P.st==PS_GND?amp_of(s):0);
  if(P.st==PS_AIR){q.air=fminf(1.f,(P.peak-P.y)*.6f+.6f);q.amp=amp_of(s);}if(P.st==PS_CLIMB){q.climb=1;q.ph=P.walk;}if(P.crouch)q.crouch=1;
  if(P.roll_t>0){q.roll=(1-P.roll_t/.55f)*6.2831853f;q.crouch=1;q.amp=0;}if(P.turn_t>0)q.spin=-(P.turn_t/.25f);if(P.stumble_t>0)q.bump=fminf(1.f,P.stumble_t/.3f);if(P.land_t>0)q.crouch=.7f;
  if(P.st==PS_CLIMB){int ledge=P.cbox>=0&&P.y>=boxes[P.cbox].y1-.97f;if(ledge){q.hang=1;q.climb=0;q.ph=P.walk;}}
  q.sword=P.combat;if(P.atk_t>0)q.atk=1.f-P.atk_t/P.atk_dur;if(P.react_t>0)q.react=fminf(1.f,P.react_t/.3f);
  q.tail=fminf(1.f,s/8.f)*.9f+(P.st==PS_AIR?.5f:0);{static float pyaw;q.sway=clampf(wrapa(P.yaw-pyaw)*6.f,-.5f,.5f);pyaw=P.yaw;}
  if(!P.moving&&P.st==PS_GND)q.look=clampf(wrapa(P.cyaw-P.yaw),-1.f,1.f)*.7f;
  {static float lt;float k=fminf(1.f,(g_time-lt)*12.f);if(g_time-lt>.3f)k=1;lt=g_time;pose_blend(&P.ps,&q,k);P.ps.ph=q.ph;}
  if(P.st==PS_CLIMB)DM_lay=-.12f;draw_model(&mdl_hero,P.x,P.y,P.z,P.yaw,&P.ps,g_time,1,1.f);}
 r3_flush();}
/* ===== интерфейс ===== */
#define MAPW 320
#define MAPH 200
static uint32_t maptex[MAPW*MAPH];
static void build_map(void){for(int z=0;z<MAPH;z++)for(int x=0;x<MAPW;x++){int t=ground_tile(x+.5f,z+.5f);uint32_t c=t<0?RGBX(62,104,150):(t==TL_COBBLE||t==TL_COBBLE2)?RGBX(176,168,154):t==TL_SIDEWALK?RGBX(190,184,172):t==TL_PLAZA?RGBX(204,190,164):t==TL_GRASS?RGBX(96,130,84):t==TL_GRAVEL?RGBX(196,186,160):RGBX(116,104,92);maptex[z*MAPW+x]=c;}
 for(int i=0;i<nbld;i++){Bd*b=&blds[i];for(int z=(int)b->z0;z<(int)b->z1&&z<MAPH;z++)for(int x=(int)b->x0;x<(int)b->x1&&x<MAPW;x++)maptex[z*MAPW+x]=(x==(int)b->x0||z==(int)b->z0||x==(int)b->x1-1||z==(int)b->z1-1)?RGBX(110,90,76):RGBX(158,132,112);}
 static const int big[10][4]={{80,93,120,103},{95,88,105,108},{80,89,88,107},{28,40,50,52},{30,30,37,38},{26,146,32,152},{168,146,174,152},{228,2,292,16},{252,0,268,20},{261,157,267,163}};
 for(int k=0;k<10;k++)for(int z=big[k][1];z<big[k][3];z++)for(int x=big[k][0];x<big[k][2];x++)maptex[z*MAPW+x]=RGBX(138,128,116);
 for(int i=0;i<ntree;i++){int x=(int)treev[i][0],z=(int)treev[i][2];for(int dz=-1;dz<=1;dz++)for(int dx=-1;dx<=1;dx++){int xx=x+dx,zz=z+dz;if(xx>=0&&zz>=0&&xx<MAPW&&zz<MAPH)maptex[zz*MAPW+xx]=RGBX(70,110,60);}}
 for(int q=0;q<nplank;q++){int x=(int)plankv[q][0],z=(int)plankv[q][2];for(int k=-4;k<=4;k++){int xx=plankv[q][3]==0?x+k:x,zz=plankv[q][3]==0?z:z+k;if(xx>=0&&zz>=0&&xx<MAPW&&zz<MAPH)maptex[zz*MAPW+xx]=RGBX(150,110,70);}}}
static void panel(int x,int y,int w,int h,int a){gfx_rect(x+2,y+2,w,h,RGBX(0,0,0),a/2);gfx_rect(x,y,w,h,RGBX(18,14,26),a);gfx_rect(x,y,w,1,RGBX(214,176,90),230);gfx_rect(x,y+h-1,w,1,RGBX(214,176,90),120);}
static void star(float x,float y,float r,uint32_t c){for(int k=0;k<5;k++){float a=k*1.2566f-1.5708f,b=a+.6283f,cc=a-.6283f;gfx_tri(x,y,x+cosf(a)*r,y+sinf(a)*r,x+cosf(b)*r*.42f,y+sinf(b)*r*.42f,c,255);gfx_tri(x,y,x+cosf(a)*r,y+sinf(a)*r,x+cosf(cc)*r*.42f,y+sinf(cc)*r*.42f,c,255);}}
static int giver_has_new(int g){for(int r=0;r<NRACE;r++)if(RD[r].giver==g&&race_open(r)&&!medal[r])return 1;return 0;}
static void draw_minimap(void){int cx=56,cy=SH-58,R=44;gfx_circle(cx,cy,R+3,RGBX(214,176,90),230);gfx_circle(cx,cy,R+1,RGBX(14,12,20),255);
 for(int dy=-R;dy<=R;dy++)for(int dx=-R;dx<=R;dx++){if(dx*dx+dy*dy>R*R)continue;float mx=P.x+dx*.8f,mz=P.z+dy*.8f;if(mx<0||mz<0||mx>=MAPW||mz>=MAPH){gfx_px(cx+dx,cy+dy,RGBX(20,20,28),255);continue;}gfx_px(cx+dx,cy+dy,maptex[(int)mz*MAPW+(int)mx],255);}
 for(int k=0;k<NRUN;k++){Npc*n=&npc[runner_id[k]];float dx=(n->x-P.x)/.8f,dz=(n->z-P.z)/.8f,l=hypotf(dx,dz);int nw=giver_has_new(k);if(!nw&&l>R)continue;if(l>R-4){dx*=(R-4)/l;dz*=(R-4)/l;}star(cx+dx,cy+dz,nw?5.f:3.5f,nw?RGBX(255,214,70):RGBX(170,170,180));}
 {Npc*n=&npc[painter_id];float dx=(n->x-P.x)/.8f,dz=(n->z-P.z)/.8f;if(dx*dx+dz*dz<R*R){gfx_circle(cx+dx,cy+dz,3,RGBX(214,120,200),255);}}
 if(race_active()){const RaceDef*d=&RD[RC.id];RaceRT*Rr=&rrt[RC.id];for(int k=0;k<Rr->ng;k++){int show=d->type==RT_HUNT?!RC.got[k]:(k==RC.next&&d->type!=RT_THIEF);if(!show)continue;float dx=(Rr->g[k].x-P.x)/.8f,dz=(Rr->g[k].z-P.z)/.8f,l=hypotf(dx,dz);if(l>R-4){dx*=(R-4)/l;dz*=(R-4)/l;}gfx_quad(cx+dx,cy+dz-5,cx+dx+4,cy+dz,cx+dx,cy+dz+5,cx+dx-4,cy+dz,d->type==RT_HUNT?RGBX(255,90,80):RGBX(255,214,70),255);}
  if(RC.runner>=0){Npc*n=&npc[RC.runner];float dx=(n->x-P.x)/.8f,dz=(n->z-P.z)/.8f,l=hypotf(dx,dz);if(l>R-3){dx*=(R-3)/l;dz*=(R-3)/l;}gfx_circle(cx+dx,cy+dz,3,RGBX(255,80,200),255);}}
 for(int i=0;i<NID;i++){Npc*n=&npc[i];if(n->type!=NT_GUARD||n->st==GS_KO)continue;if(n->alert<.15f)continue;float dx=(n->x-P.x)/.8f,dz=(n->z-P.z)/.8f;if(dx*dx+dz*dz<R*R)gfx_circle(cx+dx,cy+dz,2.2f,n->st==GS_CHASE?RGBX(255,50,50):RGBX(255,200,60),255);}
 float a=P.yaw;gfx_tri(cx+sinf(a)*6,cy+cosf(a)*6,cx+sinf(a+2.5f)*5,cy+cosf(a+2.5f)*5,cx+sinf(a-2.5f)*5,cy+cosf(a-2.5f)*5,RGBX(255,255,255),255);}
static void fmt_time(char*b,int n,float t){int m=(int)(t/60);float s=t-m*60;if(m)snprintf(b,n,"%d:%04.1f",m,s);else snprintf(b,n,"%.1f с",s);}
static void medal_icon(float x,float y,int m){static const uint32_t C[4]={RGBX(90,86,96),RGBX(196,130,70),RGBX(206,212,222),RGBX(250,206,70)};gfx_circle(x,y,5,RGBX(20,16,24),255);gfx_circle(x,y,4,C[m],255);gfx_circle(x-1.2f,y-1.2f,1.3f,RGBX(255,255,255),m?150:30);}
static char prompt[160];
static void draw_race_hud(void){const RaceDef*d=&RD[RC.id];RaceRT*R=&rrt[RC.id];char b[200],t1[40];
 panel(SW/2-120,8,240,54,215);snprintf(b,sizeof b,"%s · %s",d->title,RTNAME[d->type]);gfx_text(SW/2-116,10,232,14,10,RGBX(236,196,96),TF_CENTER|TF_BOLD,b);
 fmt_time(t1,sizeof t1,RC.t);gfx_text(SW/2-116,24,232,22,17,RGBX(255,250,236),TF_CENTER|TF_BOLD,t1);
 if(d->type==RT_HUNT)snprintf(b,sizeof b,"Флажки: %d из %d · лимит %.0f с",R->ng-RC.left,R->ng,R->limit);
 else if(d->type==RT_THIEF){Npc*n=&npc[RC.runner];snprintf(b,sizeof b,"До вора %.0f м — догоните его!",hypotf(n->x-P.x,n->z-P.z));}
 else if(d->type==RT_RIVAL){int rg=rival_gate();snprintf(b,sizeof b,"Ворота %d/%d · соперник %s",RC.next,R->ng,rg+1>RC.next?"впереди":rg+1<RC.next?"позади":"рядом");}
 else if(d->type==RT_TOWER)snprintf(b,sizeof b,"Заберитесь на вершину · лимит %.0f с",R->limit);
 else snprintf(b,sizeof b,"Ворота %d из %d%s",RC.next,R->ng,d->type==RT_ROOFS&&RC.next>=1?" · мостовая — лава!":"");
 gfx_text(SW/2-116,46,232,14,9,RGBX(220,214,206),TF_CENTER,b);
 if(d->type!=RT_RIVAL&&d->type!=RT_THIEF){float ys=18;for(int m=3;m>=1;m--){medal_icon(SW/2+136,ys,m);fmt_time(t1,sizeof t1,m==3?R->gold:m==2?R->silver:R->bronze);gfx_text(SW/2+144,ys-7,70,14,9,RC.t<=(m==3?R->gold:m==2?R->silver:R->bronze)?RGBX(255,246,220):RGBX(140,130,130),0,t1);ys+=15;}}
 if(RC.st==RS_COUNT){int c=(int)ceilf(RC.count-.4f);snprintf(b,sizeof b,"%d",c<1?1:c);gfx_text(0,120,SW,60,48,RGBX(255,226,140),TF_CENTER|TF_BOLD,b);}
 else if(RC.t<1.f)gfx_text(0,120,SW,60,40,RGBX(255,226,140),TF_CENTER|TF_BOLD,"Марш!");
 /* стрелка к цели за краем экрана */
 int k=d->type==RT_HUNT?-1:RC.next;float gx=0,gy=0,gz=0;if(d->type==RT_HUNT){float bd=1e9f;for(int q=0;q<R->ng;q++)if(!RC.got[q]){float dd=hypotf(R->g[q].x-P.x,R->g[q].z-P.z);if(dd<bd){bd=dd;k=q;}}}
 if(d->type==RT_THIEF&&RC.runner>=0){Npc*n=&npc[RC.runner];gx=n->x;gy=n->y;gz=n->z;}else if(k>=0&&k<R->ng){gx=R->g[k].x;gy=R->g[k].y;gz=R->g[k].z;}else return;
 float sx,sy;int on=r3_project(v3(gx,gy+1.4f,gz),&sx,&sy);if(on&&sx>20&&sx<SW-20&&sy>70&&sy<SH-30){snprintf(b,sizeof b,"%.0f м",hypotf(gx-P.x,gz-P.z));gfx_text((int)sx-30,(int)sy+16,60,12,9,RGBX(255,236,170),TF_CENTER|TF_BOLD,b);return;}
 float rel=wrapa(atan2f(gx-P.x,gz-P.z)-P.cyaw);float ax=SW/2-sinf(rel)*170,ay=SH/2-cosf(rel)*110;float dx=-sinf(rel),dy=-cosf(rel);gfx_tri(ax+dx*12,ay+dy*12,ax-dy*8,ay+dx*8,ax+dy*8,ay-dx*8,RGBX(255,214,70),235);}
static void draw_markers(void){for(int k=0;k<NRUN;k++){Npc*n=&npc[runner_id[k]];if(n->racing||!n->vis)continue;float sx,sy;if(!r3_project(v3(n->x,n->y+2.25f+sinf(g_time*3+k)*.06f,n->z),&sx,&sy))continue;if(sx<0||sx>SW||sy<0||sy>SH)continue;int nw=giver_has_new(k);star(sx,sy,nw?7.f:5.f,nw?RGBX(255,214,70):RGBX(170,170,180));}
 {Npc*n=&npc[painter_id];float sx,sy;if(n->vis&&r3_project(v3(n->x,n->y+2.2f,n->z),&sx,&sy)&&sx>0&&sx<SW&&sy>0&&sy<SH){gfx_circle(sx,sy,5,RGBX(30,20,30),230);gfx_circle(sx,sy,3.5f,RGBX(214,120,200),255);}}
 for(int i=0;i<NID;i++){Npc*n=&npc[i];if(!n->vis)continue;
  if(n->bark_t>0&&n->bark){float sx,sy;if(r3_project(v3(n->x,n->y+2.05f,n->z),&sx,&sy)&&sx>-60&&sx<SW+60&&sy>10&&sy<SH){float dd=hypotf(n->x-P.x,n->z-P.z);if(dd<24){int w=(int)strlen(n->bark)*3+20;if(w>190)w=190;int a=n->bark_t>.4f?230:(int)(n->bark_t*575);gfx_rect((int)sx-w/2,(int)sy-16,w,16,RGBX(250,246,236),a);gfx_tri(sx-4,sy,sx+4,sy,sx,sy+5,RGBX(250,246,236),a);gfx_text((int)sx-w/2+3,(int)sy-15,w-6,14,9,RGBX(40,30,30),TF_CENTER|TF_NOSHADOW,n->bark);}}}
  if(n->type==NT_GUARD&&n->st!=GS_KO&&(n->alert>.04f||n->st==GS_CHASE)){float sx,sy;if(!r3_project(v3(n->x,n->y+2.2f,n->z),&sx,&sy)||sx<0||sx>SW||sy<0||sy>SH)continue;float a=n->st==GS_CHASE?1.f:n->alert;uint32_t col=gfx_lerp(RGBX(255,220,70),RGBX(255,50,40),a);gfx_circle(sx,sy,7,RGBX(20,16,24),230);gfx_circle(sx,sy,3.5f*a+1,col,255);if(a>=1)gfx_text((int)sx-5,(int)sy-8,10,14,10,RGBX(255,255,255),TF_CENTER|TF_BOLD,"!");
   float dd=hypotf(n->x-P.x,n->z-P.z);
   if(n->st==GS_CHASE&&dd<14.f){int w=26;gfx_rect((int)sx-w/2-1,(int)sy+9,w+2,4,RGBX(0,0,0),190);gfx_rect((int)sx-w/2,(int)sy+10,(int)(w*fmaxf(0.f,n->hp)/100.f),2,n->open_t>0?RGBX(255,230,120):RGBX(220,60,60),255);}
   if(n->wind>0&&dd<9.f){float pu=1.f+.25f*sinf(g_time*30.f);gfx_circle(sx,sy-16,11*pu,RGBX(150,10,10),200);gfx_ring(sx,sy-16,11*pu,1.5f,RGBX(255,230,200),255);gfx_text((int)sx-8,(int)sy-24,16,16,11,RGBX(255,255,255),TF_CENTER|TF_BOLD,"Q");}
   if(n->fus&&n->aim_t>.5f){float k=fminf(1.f,(n->aim_t-.5f)/1.2f);gfx_ring(sx,sy-16,14-8*k,1.5f,RGBX(255,170,40),255);gfx_circle(sx,sy-16,2,RGBX(255,170,40),255);if(k>.5f)gfx_text((int)sx-50,(int)sy-40,100,12,8,RGBX(255,210,120),TF_CENTER|TF_BOLD,"Пробел — уклон");}
   if(n->open_t>0&&dd<9.f)gfx_text((int)sx-30,(int)sy-30,60,12,9,RGBX(255,230,120),TF_CENTER|TF_BOLD,"F — добить");}}}
static void draw_hud(void){char b[160];
 if(race_active()&&RC.st!=RS_RESULT)draw_race_hud();
 else{const char*obj=chapter<=1&&!medal[0]?"Найдите Жака-Трубочиста (звезда на карте) и пройдите первую гонку":chapter==4&&!medal[9]?"Финал: найдите Ворона у рынка":chapter>=5?"Вольная игра: медали, портреты и башни":"Бегуны Лиги отмечены звёздами. Портреты — в альбоме (Tab)";
  static const char*CHN[6]={"Пролог","Глава 1. Новичок","Глава 2. Свой среди крыш","Глава 3. Вызов Ворону","Финал","Лига крыш"};
  panel(10,10,330,40,195);gfx_text(18,13,316,14,10,RGBX(236,196,96),TF_BOLD,CHN[chapter<6?chapter:5]);gfx_text(18,27,316,22,9,RGBX(240,236,228),0,obj);}
 {int hh=(int)tod,mm=(int)((tod-hh)*60);snprintf(b,sizeof b,"%02d:%02d   Портреты %d/128",hh,mm,count_alb(2));panel(SW-190,10,180,22,195);gfx_text(SW-186,13,172,16,10,RGBX(240,236,228),TF_RIGHT|TF_BOLD,b);}
 draw_minimap();
 {int w=150,x=SW/2-w/2,y=SH-26;gfx_rect(x-1,y-1,w+2,9,RGBX(0,0,0),170);gfx_rect(x,y,(int)(w*fmaxf(0,P.hp)/100),7,P.hp>35?RGBX(190,50,56):RGBX(255,60,60),255);gfx_rect(x,y+8,(int)(w*P.stam/100),2,RGBX(240,210,90),255);}
 {float ix=SW-150;for(int k=0;k<4;k++){uint32_t c=ab[k]?RGBX(240,210,120):RGBX(80,76,86);gfx_circle(ix+k*16,SH-36,6,RGBX(20,16,24),220);gfx_circle(ix+k*16,SH-36,4.5f,c,255);}gfx_text(SW-158,SH-28,150,12,8,RGBX(210,206,200),TF_RIGHT,"способности (Tab — альбом)");}
 if(P.combat>.5f)gfx_text(SW/2-150,SH-44,300,14,10,RGBX(255,120,100),TF_CENTER|TF_BOLD,"БОЙ · F — удар, Q — контратака, Пробел — уклон");else if(chase_cnt>0)gfx_text(SW/2-70,SH-44,140,14,10,RGBX(255,90,80),TF_CENTER|TF_BOLD,"ПОГОНЯ");else if(susp_cnt>0)gfx_text(SW/2-70,SH-44,140,14,10,RGBX(255,210,80),TF_CENTER|TF_BOLD,"Стража начеку");
 else if(incrowd)gfx_text(SW/2-70,SH-44,140,14,10,RGBX(160,220,180),TF_CENTER,"Вы в толпе");else if(P.crouch)gfx_text(SW/2-70,SH-44,140,14,10,RGBX(200,200,220),TF_CENTER,"Крадётесь · у края — спуск");
 if(prompt[0]&&!(RC.st==RS_RESULT)){int w=280;panel(SW/2-w/2,SH-66,w,18,220);gfx_text(SW/2-w/2+6,SH-64,w-12,14,10,RGBX(255,246,220),TF_CENTER|TF_BOLD,prompt);}
 if(toast_t>0){int w=380;panel(SW/2-w/2,race_active()?68:58,w,20,220);gfx_text(SW/2-w/2+6,(race_active()?71:61),w-12,14,10,RGBX(255,236,200),TF_CENTER|TF_BOLD,toast);}}
/* ===== альбом «Лица квартала» ===== */
static int alb_sel,port_id=-1;static Model port_m;
static const uint32_t CLCOL[CL_N]={RGBX(150,120,90),RGBX(90,120,160),RGBX(170,90,90),RGBX(70,100,180),RGBX(200,170,80),RGBX(110,100,120),RGBX(120,110,100),RGBX(90,140,110),RGBX(130,110,170),RGBX(80,150,170)};
static void portrait_build(int id){if(port_id==id)return;if(port_id>=0)model_free(&port_m);build_person(&port_m,ids[id].look,1);port_id=id;}
static void gilded_frame(int x,int y,int w,int h){uint32_t g1=RGBX(196,150,60),g2=RGBX(250,216,120),g3=RGBX(120,84,30);gfx_rect(x-8,y-8,w+16,8,g1,255);gfx_rect(x-8,y+h,w+16,8,g1,255);gfx_rect(x-8,y,8,h,g1,255);gfx_rect(x+w,y,8,h,g1,255);
 gfx_rect(x-8,y-8,w+16,2,g2,255);gfx_rect(x-8,y-8,2,h+16,g2,255);gfx_rect(x-2,y-2,w+4,2,g3,255);gfx_rect(x-2,y-2,2,h+4,g3,255);gfx_rect(x+w,y-2,2,h+4,g2,200);gfx_rect(x-2,y+h,w+4,2,g2,200);
 for(int k=0;k<4;k++){float cx=k&1?x+w+4:x-4,cy=k&2?y+h+4:y-4;gfx_circle(cx,cy,6,g2,255);gfx_circle(cx,cy,3,g3,255);}
 for(int k=0;k<6;k++){gfx_circle(x+w*(k+.5f)/6,y-4,1.6f,g2,255);gfx_circle(x+w*(k+.5f)/6,y+h+4,1.6f,g2,255);}}
static void draw_portrait(int id,int x,int y,int w,int h){int st=alb[id];
 gfx_vgrad(x,y,w,h,RGBX(70,62,46),RGBX(30,26,22));gfx_glow(x+w*.5f,y+h*.38f,w*.55f,RGBX(120,96,60),120);
 portrait_build(id);float D=1.75f,vis=1.0f,fov=2*atanf((vis*SH/h*.5f)/D);
 float cx=(x+w*.5f)/SW*2-1,cy=1-(y+h*.5f)/SH*2;Mat T=m_ident();T.m[3]=cx;T.m[7]=cy;
 V3 eye=v3(.35f,1.55f,D),at=v3(0,1.43f,0);r3_vp=m_mul(T,m_mul(m_persp(fov,(float)SW/SH,.2f,20.f),m_look(eye,at)));r3e.eye=eye;
 r3e.L=vnrm(v3(-.55f,.6f,.7f));{static const float sc[3]={2.6f,2.25f,1.8f},z[3]={.30f,.26f,.24f},h[3]={.36f,.30f,.24f},g[3]={.14f,.11f,.08f};memcpy(r3e.sun,sc,12);memcpy(r3e.sky_zen,z,12);memcpy(r3e.sky_hor,h,12);memcpy(r3e.ground,g,12);}
 r3e.fog_dens=0;r3e.emis=0;r3e.exposure=1.1f;r3e.cloud=0;r3e.night=0;
 r3_clip[0]=x*rs;r3_clip[1]=y*rs;r3_clip[2]=(x+w)*rs-1;r3_clip[3]=(y+h)*rs-1;r3_clear();Pose q=pose0(0,0);draw_model(&port_m,0,0,0,.35f,&q,g_time*.3f,st==2,st==2?1.f:.06f);r3_flush();r3_clip[0]=r3_clip[1]=0;r3_clip[2]=r3_clip[3]=1<<20;
 if(st==2)r3_post_paint(x,y,w,h);else{gfx_rect(x,y,w,h,RGBX(20,16,14),110);gfx_text(x,y+h/2-12,w,24,22,RGBX(180,160,120),TF_CENTER|TF_BOLD,"?");}
 gilded_frame(x,y,w,h);}
static void draw_album(void){char b[600];gfx_vgrad(0,0,SW,SH,RGBX(40,30,28),RGBX(16,12,14));
 gfx_text(16,8,330,20,16,RGBX(244,214,130),TF_BOLD,"Лица квартала");snprintf(b,sizeof b,"Портреты: %d из 128 · знакомые: %d",count_alb(2),count_alb(1));gfx_text(16,30,330,14,10,RGBX(220,210,196),0,b);
 int gx=16,gy=52;for(int i=0;i<NID;i++){int cx=gx+(i%16)*20,cy=gy+(i/16)*20,st=alb[i];uint32_t c=CLCOL[ids[i].cls];
  if(i==alb_sel){gfx_rect(cx-2,cy-2,21,21,RGBX(255,226,140),255);}
  if(st==2){gfx_rect(cx,cy,17,17,RGBX(214,176,90),255);gfx_rect(cx+1,cy+1,15,15,gfx_lerp(c,RGBX(40,30,30),.25f),255);gfx_circle(cx+8.5f,cy+8.f,4.f,ids[i].look.skin,255);gfx_ellipse(cx+8.5f,cy+5.f,4.4f,2.4f,ids[i].look.hair,255);gfx_rect(cx+4,cy+12,9,4,ids[i].look.coat,255);}
  else if(st==1){gfx_rect(cx,cy,17,17,gfx_lerp(c,RGBX(30,26,26),.55f),255);gfx_circle(cx+8.5f,cy+8.f,4.f,RGBX(60,52,50),255);}
  else{gfx_rect(cx,cy,17,17,RGBX(36,30,30),255);gfx_rect(cx+7,cy+5,3,6,RGBX(70,62,60),255);gfx_rect(cx+7,cy+12,3,2,RGBX(70,62,60),255);}}
 /* легенда по сословиям */
 {int y=222;for(int c=0;c<CL_N;c++){int lx=16+(c%2)*160,ly=y+(c/2)*14;gfx_rect(lx,ly+2,9,9,CLCOL[c],255);int tot=0,got=0;for(int i=0;i<NID;i++)if(ids[i].cls==c){tot++;got+=alb[i]==2;}snprintf(b,sizeof b,"%s %d/%d",CLNAME[c],got,tot);gfx_text(lx+13,ly,140,13,9,RGBX(220,210,196),0,b);}}
 gfx_text(16,SH-18,330,14,9,RGBX(200,180,140),0,"Стрелки/WASD — выбор · Tab или Esc — закрыть");
 /* карточка */
 int id=alb_sel;Ident*d=&ids[id];int px=372,py=16,pw=150,ph=176;draw_portrait(id,px,py,pw,ph);
 int tx=350,tw=SW-tx-10;if(alb[id]>=1){gfx_text(tx,py+ph+14,tw,18,13,RGBX(250,236,200),TF_BOLD,d->name);
   snprintf(b,sizeof b,"%d лет · %s · %s · %s",d->age,d->female?"женщина":"мужчина",CLNAME[d->cls],d->job);gfx_text(tx,py+ph+32,tw,14,9,RGBX(220,206,180),0,b);
   if(alb[id]==2){snprintf(b,sizeof b,"Черты: %s, %s",TRAIT[d->t1],TRAIT[d->t2]);gfx_text(tx,py+ph+46,tw,14,9,RGBX(236,196,96),0,b);gfx_text(tx,py+ph+60,tw,SH-(py+ph+60)-4,9,RGBX(236,230,220),0,d->bio);}
   else gfx_text(tx,py+ph+48,tw,40,9,RGBX(190,180,170),0,"Знакомы, но портрет ещё не написан. Побеждайте в гонках Лиги: за каждую медаль Мэтр Огюстен пишет новые портреты.");}
 else{gfx_text(tx,py+ph+14,tw,18,13,RGBX(170,160,150),TF_BOLD,"Незнакомец");snprintf(b,sizeof b,"%s. Поговорите с жителями квартала (E), чтобы узнать имена; портреты — награды за гонки Лиги.",CLNAME[d->cls]);gfx_text(tx,py+ph+34,tw,60,9,RGBX(190,180,170),0,b);}}
/* ===== меню гонок, итоги, диалоги, меню ===== */
static void draw_racemenu(void){char b[200],t1[40];gfx_rect(0,0,SW,SH,RGBX(0,0,0),150);int w=470,h=40+rm_n*30+26,x=SW/2-w/2,y=SH/2-h/2;panel(x,y,w,h,240);
 Ident*g=&ids[runner_id[rm_giver]];snprintf(b,sizeof b,"%s «%s» · гонки Лиги крыш",g->name,RUN_NICK[rm_giver]);gfx_text(x+10,y+8,w-20,16,12,RGBX(244,214,130),TF_BOLD,b);
 for(int k=0;k<rm_n;k++){int r=rm_list[k],yy=y+34+k*30;if(k==menu_sel)gfx_rect(x+6,yy-2,w-12,28,RGBX(214,176,90),80);
  if(race_open(r)){snprintf(b,sizeof b,"%s — %s",RD[r].title,RTNAME[RD[r].type]);gfx_text(x+30,yy,300,14,11,RGBX(250,244,230),TF_BOLD,b);
   if(best_t[r]>0){fmt_time(t1,sizeof t1,best_t[r]);snprintf(b,sizeof b,"лучшее время %s",t1);}else snprintf(b,sizeof b,"не пройдена%s",RD[r].reward>=0?", награда: способность «":"");
   if(best_t[r]<=0&&RD[r].reward>=0){strncat(b,ABNAME[RD[r].reward],sizeof b-strlen(b)-3);strcat(b,"»");}gfx_text(x+30,yy+13,330,12,9,RGBX(200,190,176),0,b);
   for(int m=1;m<=3;m++)medal_icon(x+w-60+m*14,yy+8,medal[r]>=m?m:0);}
  else{snprintf(b,sizeof b,"%s — откроется %s",RD[r].title,RD[r].chapter==5?"после финала":RD[r].chapter==4?"в финале":"в следующей главе");gfx_text(x+30,yy+5,w-60,14,10,RGBX(130,124,120),0,b);}}
 gfx_text(x+10,y+h-20,w-20,14,9,RGBX(214,176,90),TF_CENTER,"E — начать · Стрелки — выбор · Esc — назад");}
static void draw_result(void){const RaceDef*d=&RD[RC.id];RaceRT*R=&rrt[RC.id];char b[400],t1[40];int w=430,h=RC.ok?(150+(RC.abnew>=0?34:0)+(RC.npnew?16+((RC.npnew+1)/2)*12:0)):110,x=SW/2-w/2,y=SH/2-h/2;
 float a=fminf(1.f,RC.result_t*3);gfx_rect(0,0,SW,SH,RGBX(0,0,0),(int)(120*a));panel(x,y,w,h,240);
 gfx_text(x,y+8,w,20,15,RC.ok?RGBX(250,214,110):RGBX(240,120,110),TF_CENTER|TF_BOLD,RC.ok?"Победа!":"Не вышло");gfx_text(x,y+28,w,14,10,RGBX(230,220,206),TF_CENTER,d->title);
 if(!RC.ok){gfx_text(x+10,y+48,w-20,30,11,RGBX(240,230,220),TF_CENTER,RC.why);gfx_text(x,y+h-22,w,14,10,RGBX(214,176,90),TF_CENTER,"R — ещё раз · E — выйти");return;}
 fmt_time(t1,sizeof t1,RC.t);snprintf(b,sizeof b,"Время %s",t1);gfx_text(x,y+46,w,18,14,RGBX(255,250,236),TF_CENTER|TF_BOLD,b);
 static const char*MN[4]={"","Бронза","Серебро","Золото"};for(int m=1;m<=3;m++)medal_icon(SW/2-28+m*14,y+76,RC.new_medal>=m?RC.new_medal:0);
 snprintf(b,sizeof b,"Медаль: %s",MN[RC.new_medal]);gfx_text(x,y+86,w,14,11,RGBX(240,220,170),TF_CENTER|TF_BOLD,b);
 if(d->type!=RT_RIVAL&&d->type!=RT_THIEF){char g1[30],s1[30],b1[30];fmt_time(g1,30,R->gold);fmt_time(s1,30,R->silver);fmt_time(b1,30,R->bronze);snprintf(b,sizeof b,"золото ≤ %s · серебро ≤ %s · бронза ≤ %s",g1,s1,b1);gfx_text(x,y+102,w,12,9,RGBX(190,180,170),TF_CENTER,b);}
 int yy=y+120;if(RC.abnew>=0){snprintf(b,sizeof b,"Новая способность: %s",ABNAME[RC.abnew]);gfx_text(x+12,yy,w-24,14,11,RGBX(140,220,255),TF_BOLD,b);gfx_text(x+12,yy+14,w-24,24,9,RGBX(210,224,236),0,ABDESC[RC.abnew]);yy+=34;}
 if(RC.npnew){snprintf(b,sizeof b,"Мэтр Огюстен написал новые портреты (%d):",RC.npnew);gfx_text(x+12,yy,w-24,14,10,RGBX(236,196,96),TF_BOLD,b);yy+=15;for(int k=0;k<RC.npnew;k++){gfx_text(x+12+(k%2)*(w/2-6),yy+(k/2)*12,w/2-16,12,9,RGBX(236,230,220),0,ids[RC.pnew[k]].name);}}
 else if(RC.abnew<0)gfx_text(x+12,yy,w-24,14,9,RGBX(190,180,170),0,"Новых портретов нет: улучшите медаль, чтобы получить больше.");
 gfx_text(x,y+h-20,w,14,10,RGBX(214,176,90),TF_CENTER,"E — продолжить · R — ещё раз");}
static void draw_dialog(void){char sp[80];const char*s=dl[dl_i],*bar=strchr(s,'|');int h=70;panel(20,SH-h-14,SW-40,h,232);
 if(bar){int n=(int)(bar-s);if(n>79)n=79;memcpy(sp,s,n);sp[n]=0;gfx_text(32,SH-h-10,SW-64,16,11,RGBX(236,196,96),TF_BOLD,sp);s=bar+1;}
 gfx_text(32,SH-h+6,SW-64,h-20,11,RGBX(244,238,228),0,s);gfx_text(SW-170,SH-30,140,14,9,RGBX(214,176,90),TF_RIGHT,"E — дальше");}
static void menu_draw(const char**it,int n,int sel,int x,int y,int w){for(int i=0;i<n;i++){int yy=y+i*20;if(i==sel){gfx_rect(x,yy,w,18,RGBX(214,176,90),90);gfx_tri(x+5,yy+4,x+5,yy+14,x+12,yy+9,RGBX(255,226,150),255);}gfx_text(x+18,yy+1,w-24,16,12,i==sel?RGBX(255,250,236):RGBX(214,206,196),i==sel?TF_BOLD:0,it[i]);}}
static int menu_input(int n){int ch=0;if(key_hit[K_UP]||key_hit[K_W]){menu_sel=(menu_sel+n-1)%n;sfx(S_CLICK,1,0);}if(key_hit[K_DOWN]||key_hit[K_S]){menu_sel=(menu_sel+1)%n;sfx(S_CLICK,1,0);}if(key_hit[K_ENTER]||key_hit[K_SPACE]||key_hit[K_E])ch=1;return ch;}
static const char*HELP[]={"WASD — бег, мышь/стрелки — камера, Shift — спринт (и быстрое лазание), Ctrl — красться",
 "Беги в стену — залезешь на любое здание. W/S — вверх/вниз, A/D — вдоль стены, Пробел — отпрыгнуть",
 "Shift + Пробел у края крыши — прыжок через улицу (с «Хваткой» ловите стену и карнизы)",
 "Ctrl у края — повиснуть и спуститься по стене; Ctrl на стене — отпустить руки",
 "R — разворот (способность), R затем Пробел — разворот с прыжком, R на стене — оттолкнуться",
 "Приземление на бегу — перекат (способность «Перекат»), прыжок в сено — без урона",
 "E — поговорить, начать гонку, осмотреть квартал с башни, спрятаться в сено",
 "Бой: F — удар (серия из трёх), Q — контратака по красному знаку, Пробел — уклон от удара и пули. Со спины — тихо",
 "Tab — альбом «Лица квартала»: 128 жителей, портреты — награды за гонки",
 "Esc — пауза · F2 — качество · F11 — во весь экран"};
static void draw_help(void){gfx_rect(0,0,SW,SH,RGBX(0,0,0),190);panel(30,30,SW-60,SH-60,240);gfx_text(30,36,SW-60,20,14,RGBX(244,214,130),TF_CENTER|TF_BOLD,"Управление");for(int i=0;i<10;i++)gfx_text(48,64+i*25,SW-96,24,10,RGBX(236,230,220),0,HELP[i]);gfx_text(30,SH-50,SW-60,14,10,RGBX(214,176,90),TF_CENTER,"Esc — назад");}
/* ===== камера ===== */
static V3 cam_eye,cam_at;static float title_t;
static void camera_update(float dt){V3 tgt=v3(P.x,P.y+1.55f+(P.crouch?-.3f:0),P.z);float d=P.cdist+(P.sprint?.9f:0),cp=cosf(P.cpitch),sp=sinf(P.cpitch);
 V3 want=v3(tgt.x-sinf(P.cyaw)*cp*d,tgt.y+sp*d+.4f,tgt.z-cosf(P.cyaw)*cp*d);
 for(int k=0;k<8&&seg_blocked(tgt,want);k++){want.x=tgt.x+(want.x-tgt.x)*.82f;want.y=tgt.y+(want.y-tgt.y)*.82f;want.z=tgt.z+(want.z-tgt.z)*.82f;}
 if(want.y<.4f)want.y=.4f;if(!camsm){camx=want.x;camy=want.y;camz=want.z;camsm=1;}float k=fminf(1.f,dt*14.f);camx+=(want.x-camx)*k;camy+=(want.y-camy)*k;camz+=(want.z-camz)*k;
 cam_eye=v3(camx,camy,camz);cam_at=tgt;}
/* ===== подсказки и взаимодействие ===== */
static int near_npc;
static void interact_update(void){prompt[0]=0;near_npc=-1;if(race_active()||RC.st==RS_RESULT)return;
 for(int k=0;k<4;k++){if(hypotf(P.x-vpv[k][0],P.z-vpv[k][2])<3.6f&&fabsf(P.y-vpv[k][1])<2.8f&&P.st==PS_GND){if(!vpsync[k]){snprintf(prompt,sizeof prompt,"E — осмотреть квартал: %s",vpname[k]);if(key_hit[K_E]){vpsync[k]=1;set_toast("Точка обзора открыта: %s",vpname[k]);sfx(S_BELL,1,0);save_game();key_hit[K_E]=0;}}return;}}
 float bd=2.0f;for(int i=0;i<NID;i++){Npc*n=&npc[i];if(n->racing||n->down>0)continue;float dx=n->x-P.x,dz=n->z-P.z,d=hypotf(dx,dz);if(d>bd||fabsf(n->y-P.y)>1.2f)continue;float f=(dx*sinf(P.yaw)+dz*cosf(P.yaw))/(d+.01f);if(f<-.2f&&d>1.1f)continue;bd=d;near_npc=i;}
 if(near_npc>=0&&P.st==PS_GND){Npc*n=&npc[near_npc];Ident*d=&ids[near_npc];
  if(n->type==NT_RUNNER){int g=0;for(int k=0;k<NRUN;k++)if(runner_id[k]==near_npc)g=k;snprintf(prompt,sizeof prompt,"E — гонки Лиги: %s «%s»",d->name,RUN_NICK[g]);}
  else if(n->type==NT_PAINTER)snprintf(prompt,sizeof prompt,"E — Мэтр Огюстен Брюн, художник");
  else snprintf(prompt,sizeof prompt,"E — поговорить: %s",alb[near_npc]?d->name:"незнакомец");
  if(key_hit[K_E]){npc_talk(near_npc);key_hit[K_E]=0;}return;}
 if(P.st==PS_GND&&P.y<3){for(int h=0;h<nhay;h++){float dx=P.x-haycarts[h][0],dz=P.z-haycarts[h][2];if(fabsf(dx)<2.9f&&fabsf(dz)<2.2f){snprintf(prompt,sizeof prompt,"E — спрятаться в телеге с сеном");if(key_hit[K_E]){P.st=PS_HIDE;P.x=haycarts[h][0];P.z=haycarts[h][2];P.y=.9f;P.hold=0;key_hit[K_E]=0;sfx(S_HAY,1,0);}return;}}}
 if(P.st==PS_HIDE)snprintf(prompt,sizeof prompt,"E или движение — выбраться из сена");}
/* ===== бой: удары сериями, блоки стражи, контратаки (Q), уклонения (Пробел), добивания ===== */
static int combat_threat(float r){for(int i=0;i<NID;i++){Npc*g=&npc[i];if(g->type!=NT_GUARD||g->st!=GS_CHASE)continue;float d=hypotf(g->x-P.x,g->z-P.z);if((d<r&&fabsf(g->y-P.y)<2.f)||(g->fus&&g->aim_t>.6f&&d<32.f))return 1;}return 0;}
static int combat_target(float R){int bi=-1;float bs=1e9f;for(int i=0;i<NID;i++){Npc*g=&npc[i];if(g->type!=NT_GUARD||g->st==GS_KO||g->down>0)continue;float dx=g->x-P.x,dz=g->z-P.z,d=hypotf(dx,dz);if(d>R||fabsf(g->y-P.y)>1.6f)continue;
  float f=(dx*sinf(P.yaw)+dz*cosf(P.yaw))/(d+.01f);if(P.moving){float ix=sinf(P.cyaw_t),iz=cosf(P.cyaw_t);(void)ix;(void)iz;}float sc=d-f*1.2f-(g->st==GS_CHASE?.5f:0);if(sc<bs){bs=sc;bi=i;}}return bi;}
static void guard_hit(int bi,float dmg,int heavy){Npc*g=&npc[bi];g->hp-=dmg;g->hitc++;g->hitc_t=1.6f;
 if(!heavy&&g->wind>.05f){/* замах не прервать простым ударом — нужна контратака */sfx(S_HIT,.8f,0);if(g->hp<=0){g->st=GS_KO;g->ko=45;g->wind=0;if(tok_owner==bi)tok_owner=-1;sfx(S_THUD,1,0);set_toast("Стражник повержен");}return;}
 g->stag_t=heavy?.9f:.42f;g->wind=0;g->swing=0;g->aim_t=0;if(tok_owner==bi)tok_owner=-1;
 if(g->st!=GS_CHASE){g->st=GS_CHASE;alert_all(g->x,g->z,16);}g->alert=1;g->lsx=P.x;g->lsz=P.z;sfx(S_HIT,1,0);panic(g->x,g->z,11.f);
 {float dx=g->x-P.x,dz=g->z-P.z,d=hypotf(dx,dz)+.01f;npc_step(g,dx/d*(heavy?.5f:.25f),dz/d*(heavy?.5f:.25f));}
 if(g->hp<=0){g->st=GS_KO;g->ko=45;g->open_t=0;sfx(S_THUD,1,0);set_toast(heavy?"Добивание! Стражник повержен":"Стражник повержен");}}
static void combat_update(float dt){static float lasthp=100;
 if(P.inv_t>0)P.inv_t-=dt;if(P.dodge_t>0)P.dodge_t-=dt;if(P.react_t>0)P.react_t-=dt;if(P.chain_t>0)P.chain_t-=dt;else P.combo=0;if(P.regen_t>0)P.regen_t-=dt;
 int near=0;for(int i=0;i<NID;i++){Npc*g=&npc[i];if(g->type==NT_GUARD&&g->st==GS_CHASE&&hypotf(g->x-P.x,g->z-P.z)<9.f&&fabsf(g->y-P.y)<3.f){near=1;break;}}
 if(near&&P.st==PS_GND&&!race_active()){P.combat=fminf(1.f,P.combat+dt*4.f);P.combat_t=2.5f;}else{P.combat_t-=dt;if(P.combat_t<=0||P.st!=PS_GND)P.combat=fmaxf(0.f,P.combat-dt*2.5f);}
 if(P.hp<lasthp-.01f)P.regen_t=5.f;if(P.regen_t<=0&&P.combat<.1f&&P.hp<100&&P.hp>0){P.hp+=7.f*dt;if(P.hp>100)P.hp=100;}lasthp=P.hp;
 /* текущий удар: выпад к цели и попадание в середине движения */
 if(P.atk_t>0){P.atk_t-=dt;float prog=1-P.atk_t/P.atk_dur;
  if(P.atk_tg>=0){Npc*g=&npc[P.atk_tg];float dx=g->x-P.x,dz=g->z-P.z,d=hypotf(dx,dz);if(d>.01f){P.yaw+=wrapa(atan2f(dx,dz)-P.yaw)*fminf(1.f,dt*18.f);if(d>1.35f&&prog<.5f){int hb;float hx,hz;float st=fminf(d-1.35f,6.f*dt);move_h(dx/d*st,dz/d*st,&hb,&hx,&hz);}}
   if(!P.atk_hit&&prog>.42f){P.atk_hit=1;if(g->st!=GS_KO&&d<2.7f&&fabsf(g->y-P.y)<1.6f){
     float rel=((P.x-g->x)*sinf(g->yaw)+(P.z-g->z)*cosf(g->yaw))/(d+.01f);
     if(g->st!=GS_CHASE&&rel<-.15f){g->st=GS_KO;g->ko=20;sfx(S_THUD,.8f,0);set_toast("Стражник обезврежен без шума");}
     else if(g->open_t>0){guard_hit(P.atk_tg,70,1);slowmo=.25f;}
     else if(g->blk_t>0||(g->st==GS_CHASE&&g->wind<=0&&(g->stag_t<=0||g->hitc>=2)&&frnd()<.3f+.22f*g->hitc)){g->blk_t=.45f;sfx(S_CLASH,1,0);P.react_t=.18f;
      if(P.combo==2&&frnd()<.6f){g->open_t=1.f;g->stag_t=.6f;g->hitc=0;set_toast("Защита пробита! F — добить");}else if(tok_owner<0||tok_owner==P.atk_tg){g->wind=.5f;g->stag_t=0;tok_owner=P.atk_tg;sfx(S_WINDUP,1.4f,0);}}
     else guard_hit(P.atk_tg,P.combo==2?32.f:20.f,P.combo==2);}
    else sfx(S_SWISH,.8f,0);}}}
 if(input_lock||(P.st!=PS_GND&&P.st!=PS_AIR))return;
 /* контратака */
 if(key_hit[K_Q]&&P.st==PS_GND&&P.react_t<=0){int bi=-1;float bd=3.8f;for(int i=0;i<NID;i++){Npc*g=&npc[i];if(g->type!=NT_GUARD||g->wind<=0||g->st==GS_KO)continue;float d=hypotf(g->x-P.x,g->z-P.z);if(d<bd&&fabsf(g->y-P.y)<1.6f){bd=d;bi=i;}}
  if(bi>=0){Npc*g=&npc[bi];g->wind=0;if(tok_owner==bi)tok_owner=-1;tok_cd=.9f;P.yaw=atan2f(g->x-P.x,g->z-P.z);P.atk_dur=P.atk_t=.5f;P.atk_tg=bi;P.atk_hit=1;P.combo=2;P.chain_t=1.f;P.inv_t=.35f;
   sfx(S_CLASH,1,0);guard_hit(bi,38,1);if(g->st!=GS_KO){g->open_t=1.8f;set_toast("Контратака! F — добить");}slowmo=.45f;}
  else{P.react_t=.25f;sfx(S_SWISH,.6f,0);}key_hit[K_Q]=0;}
 if(key_hit[K_F]&&P.atk_t<=0&&P.react_t<=0){int tg=combat_target(P.combat>.5f?3.6f:2.6f);P.combo=P.chain_t>0?(P.combo+1)%3:0;P.atk_dur=P.combo==2?.55f:.38f;P.atk_t=P.atk_dur;P.atk_hit=0;P.atk_tg=tg;P.chain_t=P.atk_dur+.5f;P.atk=.55f;sfx(S_SWISH,1,0);key_hit[K_F]=0;}}
/* ===== автопилот для проверки маршрутов (используется только тестами) ===== */
static int bot_on,bot_wp;static float bot_px,bot_pz,bot_stuck,bot_side,bot_jcd;
static void bot_drive(float dt){if(!bot_on||RC.st!=RS_RUN)return;const RaceDef*d=&RD[RC.id];RaceRT*R=&rrt[RC.id];float tx,ty,tz;
 if(d->type==RT_THIEF){Npc*n=&npc[RC.runner];/* бежим по следу вора, а не напрямик */if(RC.t<.1f)bot_wp=1;while(bot_wp<nwp&&bot_wp<=RC.rwp&&hypotf(wpath[bot_wp].x-P.x,wpath[bot_wp].z-P.z)<1.6f&&fabsf(wpath[bot_wp].y-P.y)<2.5f)bot_wp++;
  if(bot_wp>RC.rwp||bot_wp>=nwp){tx=n->x;ty=n->y;tz=n->z;}else{tx=wpath[bot_wp].x;ty=wpath[bot_wp].y;tz=wpath[bot_wp].z;}}
 else{int k=RC.next;if(d->type==RT_HUNT){float bd=1e9f;for(int q=0;q<R->ng;q++)if(!RC.got[q]){float dd=hypotf(R->g[q].x-P.x,R->g[q].z-P.z)+fabsf(R->g[q].y-P.y)*.5f;if(dd<bd){bd=dd;k=q;}}}if(k>=R->ng)return;tx=R->g[k].x;ty=R->g[k].y;tz=R->g[k].z;}
 for(int q=0;q<K_N;q++)key_down[q]=0;
 if(d->type==RT_THIEF&&bot_wp<nwp&&wpath[bot_wp].mode==WM_CLIMB&&bot_wp+1<nwp&&hypotf(tx-P.x,tz-P.z)<1.3f){tx=wpath[bot_wp+1].x+(wpath[bot_wp+1].x-wpath[bot_wp].x)*3;tz=wpath[bot_wp+1].z+(wpath[bot_wp+1].z-wpath[bot_wp].z)*3;}
 float a=atan2f(tx-P.x,tz-P.z);P.cyaw_t=P.cyaw=a+bot_side*1.3f;key_down[K_W]=1;float hd=hypotf(tx-P.x,tz-P.z);
 if(P.st==PS_CLIMB){P.cyaw_t=P.cyaw=P.yaw;key_down[K_W]=ty>P.y-.5f||hd>3.f;key_down[K_S]=!key_down[K_W];key_down[K_SHIFT]=1;return;}
 key_down[K_SHIFT]=P.stam>20&&hd>4;bot_jcd-=dt;
 if(P.st==PS_GND){float ax=P.x+sinf(a)*1.3f,az=P.z+cosf(a)*1.3f,fl=floor_at(ax,az,P.y+.1f,.25f,NULL),drop=P.y-fl;
  if(drop>1.5f&&hd>3.5f&&(ty>P.y-5.f||RD[RC.id].type==RT_ROOFS)&&bot_jcd<=0){key_hit[K_SPACE]=1;key_down[K_SHIFT]=1;P.sprint=1;bot_jcd=.6f;}
  else if(drop>5.f&&ty<P.y-4.f){key_down[K_CTRL]=1;key_down[K_SHIFT]=0;}}
 bot_stuck+=dt;if(bot_stuck>1.2f){if(hypotf(P.x-bot_px,P.z-bot_pz)<.6f&&P.st!=PS_CLIMB){if(bot_side==0)bot_side=(rand()&1)?1.f:-1.f;else bot_side=-bot_side;if(P.st==PS_GND)key_hit[K_SPACE]=1;}else bot_side=0;bot_stuck=0;bot_px=P.x;bot_pz=P.z;}}
/* ===== главный цикл ===== */
static int last_hour=-1,has_save=-1;
static void new_game(void){memset(ab,0,sizeof ab);memset(medal,0,sizeof medal);memset(best_t,0,sizeof best_t);memset(vpsync,0,sizeof vpsync);memset(alb,0,sizeof alb);chapter=0;ending_done=0;pending_chapter=0;tod=9.5f;memset(&RC,0,sizeof RC);RC.runner=-1;
 spawn_npcs();player_reset(painter_v[0]+.5f,0,painter_v[1]-2.4f,0);P.y=floor_at(P.x,P.z,.5f,.2f,NULL);alb[painter_id]=1;dialog(D_PRO,5,story_pro_done);}
void game_init(void){srand((unsigned)time(NULL));has_save=-1;gen_ids();assign_roles();finish_ids();build_world();build_map();
 Look h={RGBX(236,200,170),RGBX(50,40,40),RGBX(60,50,60),RGBX(34,44,84),RGBX(60,60,76),RGBX(214,176,70),RGBX(30,26,32),PT_HERO,0};build_person(&mdl_hero,h,1);
 for(int i=0;i<NID;i++)build_person(&mdl_npc[i],ids[i].look,0);
 spawn_npcs();races_init();player_reset(80,0,58,3.14f);memset(&RC,0,sizeof RC);RC.runner=-1;gmode=GM_TITLE;menu_sel=0;mus_mode=MUS_TITLE;}
static void result_close(void){RC.st=RS_NONE;input_lock=0;P.fade=.6f;}
void game_update(float dt){g_time+=dt;if(toast_t>0)toast_t-=dt;mouse_want=(gmode==GM_PLAY&&RC.st!=RS_RESULT);
 switch(gmode){
 case GM_TITLE:{title_t+=dt;tod=18.3f;mus_mode=MUS_TITLE;if(has_save<0)has_save=load_game(0);int hs2=has_save;int n=hs2?4:3;if(menu_input(n)){int s=hs2?menu_sel:menu_sel+1;if(s==0){memset(&RC,0,sizeof RC);RC.runner=-1;input_lock=0;spawn_npcs();load_game(1);gmode=GM_PLAY;if(chapter==0)new_game();}else if(s==1)new_game();else if(s==2){help_back=GM_TITLE;gmode=GM_HELP;}else platform_quit();}break;}
 case GM_HELP:if(key_hit[K_ESC]||key_hit[K_ENTER]||key_hit[K_E]||key_hit[K_SPACE])gmode=help_back;break;
 case GM_DIALOG:if(key_hit[K_E]||key_hit[K_ENTER]||key_hit[K_SPACE]){dl_i++;sfx(S_CLICK,1,0);if(dl_i>=dl_n){gmode=GM_PLAY;if(dl_done)dl_done();}}break;
 case GM_ALBUM:{if(key_hit[K_LEFT]||key_hit[K_A])alb_sel=(alb_sel+NID-1)%NID;if(key_hit[K_RIGHT]||key_hit[K_D])alb_sel=(alb_sel+1)%NID;if(key_hit[K_UP]||key_hit[K_W])alb_sel=(alb_sel+NID-16)%NID;if(key_hit[K_DOWN]||key_hit[K_S])alb_sel=(alb_sel+16)%NID;
  if(key_hit[K_TAB]||key_hit[K_ESC])gmode=album_back;mus_mode=MUS_TITLE;break;}
 case GM_RACEMENU:{if(menu_input(rm_n)){int r=rm_list[menu_sel];if(race_open(r)){pending_race=r;intro_lines[0]=RD[r].intro;dialog(intro_lines,1,start_pending);}else sfx(S_FAIL,.5f,0);}if(key_hit[K_ESC])gmode=GM_PLAY;break;}
 case GM_PAUSE:{int n=race_active()?5:4;if(menu_input(n)){int s=menu_sel;if(s==0)gmode=GM_PLAY;else if(s==1){album_back=GM_PAUSE;gmode=GM_ALBUM;}else if(s==2){help_back=GM_PAUSE;gmode=GM_HELP;}else if(race_active()&&s==3){race_quit();gmode=GM_PLAY;}else{race_abort_silent();save_game();gmode=GM_TITLE;menu_sel=0;has_save=-1;}}if(key_hit[K_ESC])gmode=GM_PLAY;break;}
 case GM_PLAY:{
  if(RC.st==RS_RESULT){if(RC.result_t>.5f){if(key_hit[K_E]||key_hit[K_ENTER])result_close();else if(key_hit[K_R]){int r=RC.id;RC.st=RS_NONE;race_begin(r);}}}
  else{if(key_hit[K_ESC]){gmode=GM_PAUSE;menu_sel=0;break;}if(key_hit[K_TAB]&&!race_active()){album_back=GM_PLAY;gmode=GM_ALBUM;break;}}
  if(!race_active())tod+=dt*.008f;if(tod>=24)tod-=24;{int hr=(int)tod;if(last_hour>=0&&hr!=last_hour)sfx(S_BELL,.8f,0);last_hour=hr;}
  if(pending_chapter&&RC.st==RS_NONE){pending_chapter=0;chapter_check();if(gmode!=GM_PLAY)break;save_game();}
  if(slowmo>0){slowmo-=dt;dt*=.35f;}
  bot_drive(dt);player_update(dt);npcs_update(dt);race_update(dt);interact_update();combat_update(dt);camera_update(dt);
  if(race_active())mus_mode=MUS_RACE;else if(chase_cnt>0)mus_mode=MUS_CHASE;else if(susp_cnt>0)mus_mode=MUS_STEALTH;else mus_mode=MUS_EXPLORE;
  {int cn=0;for(int i=0;i<NID;i++)if(npc[i].vis&&hypotf(npc[i].x-P.x,npc[i].z-P.z)<12)cn++;amb_crowd=fminf(1.f,cn/9.f)*.5f;amb_wind=clampf((P.y-8.f)/25.f,0,1)*.4f;float rz=fminf(fabsf(P.z-84),fabsf(P.z-112));amb_water=(in_river(P.x,P.z)||rz<14)?.5f:0;}
  break;}}}
void game_render(void){
 if(gmode==GM_TITLE||(gmode==GM_HELP&&help_back==GM_TITLE)){float a=title_t*.05f;V3 eye=v3(100+sinf(a)*78,52,98+cosf(a)*78),at=v3(100,18,98);draw_scene(eye,at,0);
  for(int i=0;i<120;i++)gfx_rect(0,i,SW,1,RGBX(6,4,14),(int)((120-i)*1.5f));gfx_rect(0,0,SW,SH,RGBX(0,0,0),40);gfx_text(0,26,SW,50,38,RGBX(244,214,130),TF_CENTER|TF_BOLD,"ТЕНИ ПАРИЖА");gfx_text(0,74,SW,20,15,RGBX(236,220,196),TF_CENTER|TF_BOLD,"Лига крыш · 1789");
  gfx_text(0,96,SW,18,10,RGBX(220,206,186),TF_CENTER,"паркур-гонки, 128 жителей квартала и альбом их портретов");
  if(gmode==GM_TITLE){int hs2=has_save>0;static const char*it4[4]={"Продолжить","Новая игра","Управление","Выход"};menu_draw(hs2?it4:it4+1,hs2?4:3,menu_sel,SW/2-90,170,180);}else draw_help();return;}
 if(gmode==GM_ALBUM){draw_album();return;}
 draw_scene(cam_eye,cam_at,1);draw_markers();draw_hud();
 if(RC.st==RS_RESULT)draw_result();
 if(P.fade>0)gfx_rect(0,0,SW,SH,RGBX(0,0,0),(int)(fminf(1.f,P.fade)*255));
 if(gmode==GM_DIALOG)draw_dialog();
 if(gmode==GM_RACEMENU)draw_racemenu();
 if(gmode==GM_PAUSE){gfx_rect(0,0,SW,SH,RGBX(0,0,0),150);int n=race_active()?5:4;panel(SW/2-110,96,220,48+n*20,235);gfx_text(SW/2-110,102,220,18,14,RGBX(244,214,130),TF_CENTER|TF_BOLD,"Пауза");
  static const char*itr[5]={"Продолжить","Альбом портретов","Управление","Сдаться и выйти из гонки","Сохранить и в главное меню"};static const char*itn[4]={"Продолжить","Альбом портретов","Управление","Сохранить и в главное меню"};menu_draw(race_active()?itr:itn,n,menu_sel,SW/2-100,132,200);}
 if(gmode==GM_HELP)draw_help();}
/* ===== отладочные входы для тестов ===== */
void dbg2(int cmd,float a,float b,float c){if(cmd==1){P.x=a;P.y=b;P.z=c;P.vx=P.vz=P.vy=0;P.st=PS_GND;gmode=GM_PLAY;camsm=0;}if(cmd==2)tod=a;if(cmd==3){P.cyaw=P.cyaw_t=a;P.cpitch=b;}if(cmd==4)gmode=GM_PLAY;
 if(cmd==5){for(int k=0;k<AB_N;k++)ab[k]=1;chapter=5;}if(cmd==6){race_begin((int)a);gmode=GM_PLAY;}if(cmd==7)bot_on=(int)a;if(cmd==8){alb_sel=(int)a;album_back=GM_PLAY;gmode=GM_ALBUM;if(b>0)alb[(int)a]=(unsigned char)b;}if(cmd==9){for(int i=0;i<NID;i++)alb[i]=(unsigned char)a;}}
int dbg_q(int what){switch(what){case 0:return RC.st;case 1:return RC.ok;case 2:return RC.next;case 3:return chapter;case 4:return count_alb(2);case 5:return gmode;case 6:return P.st;case 7:return RC.new_medal;}return 0;}
float dbg_f(int what){switch(what){case 0:return RC.t;case 1:return P.x;case 2:return P.y;case 3:return P.z;case 4:return rrt[RC.id].ref;case 5:return rrt[RC.id].gold;case 6:return RC.rtotal;case 7:return P.hp;}return 0;}
const char*dbg_s(int what){return what==0?RC.why:what==1?toast:"";}
void dbg_dump_race(int r){RaceRT*R=&rrt[r];const RaceDef*d=&RD[r];printf("Гонка %d «%s» (%s), старт (%.1f, %.1f)\n",r,d->title,RTNAME[d->type],R->sx,R->sz);
 for(int k=0;k<R->ng;k++){Gate*g=&R->g[k];printf("   ворота %d: %c (%.1f, %.1f, %.1f) здание=%d\n",k,d->g[k].k,g->x,g->y,g->z,g->foot);}
 printf("   эталон %.1f с: золото %.1f / серебро %.1f / бронза %.1f\n",R->ref,R->gold,R->silver,R->bronze);}
void dbg_thief(void){printf("путь вора (%d точек):",nwp);for(int k=0;k<nwp;k++)printf(" [%d %.0f,%.1f,%.0f m%d t%.1f]",k,wpath[k].x,wpath[k].y,wpath[k].z,wpath[k].mode,wpath[k].t);printf("\n");}
void dbg_tick(void){Npc*n=&npc[RC.runner>=0?RC.runner:0];printf("  t=%4.1f игрок(%.1f,%.1f,%.1f st%d hp%.0f) вор(%.1f,%.1f,%.1f) wp=%d bot_wp=%d д=%.1f\n",RC.t,P.x,P.y,P.z,P.st,P.hp,n->x,n->y,n->z,RC.rwp,bot_wp,hypotf(n->x-P.x,n->z-P.z));}
void dbg_bump_test(void){/* поставить прохожего прямо на пути и разогнать героя */int bi=-1;float bd=1e9f;for(int i=0;i<NID;i++){if(npc[i].type!=NT_CIT)continue;float d=hypotf(npc[i].x-P.x,npc[i].z-P.z);if(d<bd){bd=d;bi=i;}}
 if(bi<0)return;Npc*n=&npc[bi];n->x=P.x+sinf(P.yaw)*1.2f;n->z=P.z+cosf(P.yaw)*1.2f;n->y=P.y;n->wait=5;P.vx=sinf(P.yaw)*7.6f;P.vz=cosf(P.yaw)*7.6f;}
