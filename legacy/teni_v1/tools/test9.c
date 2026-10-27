/* Проверка боя: герой против патруля. Бот бьёт сериями, контратакует по сигналу (Q), уклоняется от выстрелов. */
#include "../game2/game.c"
void platform_parallel(job_fn fn,void*arg,int n){for(int i=0;i<n;i++)fn(arg,i);}
void platform_quit(void){}
void platform_draw_text(void){}
static void steps(int n){for(int i=0;i<n;i++){game_update(1.f/60);memset(key_hit,0,sizeof key_hit);}}
int main(int argc,char**argv){int skill=argc>1?atoi(argv[1]):2;srand(argc>2?atoi(argv[2]):5);
 game_init();key_hit[K_ENTER]=1;steps(2);new_game();gmode=GM_PLAY;chapter=1;
 /* ближайший стражник */
 int gi=-1;for(int i=0;i<NID;i++)if(npc[i].type==NT_GUARD&&!npc[i].fus){gi=i;break;}
 Npc*g=&npc[gi];player_reset(g->x+1.6f,g->y,g->z,0);P.y=floor_at(P.x,P.z,g->y+.5f,.2f,NULL);P.yaw=atan2f(g->x-P.x,g->z-P.z);
 alert_all(g->x,g->z,40.f);steps(30);
 int counters=0,kos=0,deaths=0,hits_taken=0,dodges=0;float lasthp=P.hp;int frames=60*90;
 for(int f=0;f<frames;f++){
  /* реакция бота */
  int wind=-1,aim=-1;for(int i=0;i<NID;i++){Npc*n=&npc[i];if(n->type!=NT_GUARD||n->st==GS_KO)continue;float d=hypotf(n->x-P.x,n->z-P.z);if(n->wind>0&&n->wind<.5f&&d<3.6f)wind=i;if(n->fus&&n->aim_t>1.3f&&d<30)aim=i;}
  if(wind>=0&&(skill>=2||(skill==1&&rand()%3==0))){key_hit[K_Q]=1;counters++;}
  else if(aim>=0&&skill>=2&&P.dodge_t<=0){key_hit[K_SPACE]=1;key_down[K_D]=1;dodges++;}
  else{int t=combat_target(4.f);if(t>=0){P.yaw=atan2f(npc[t].x-P.x,npc[t].z-P.z);if(f%9==0)key_hit[K_F]=1;}
   else{/* подойти к ближайшему преследователю */int b=-1;float bd=30;for(int i=0;i<NID;i++){Npc*n=&npc[i];if(n->type!=NT_GUARD||n->st!=GS_CHASE)continue;float d=hypotf(n->x-P.x,n->z-P.z);if(d<bd){bd=d;b=i;}}
    if(b>=0){P.cyaw_t=P.cyaw=atan2f(npc[b].x-P.x,npc[b].z-P.z);key_down[K_W]=1;}}}
  game_update(1.f/60);memset(key_hit,0,sizeof key_hit);key_down[K_W]=key_down[K_D]=0;
  if(P.hp<lasthp-.5f)hits_taken++;if(P.hp>lasthp+50)deaths++;lasthp=P.hp;
  if(getenv("V")&&f%20==0&&f<60*25){printf("%5.2f hp%3.0f atk%.2f |",f/60.f,P.hp,P.atk_t);for(int i=0;i<NID;i++){Npc*n=&npc[i];if(n->type==NT_GUARD&&n->st==GS_CHASE)printf(" g%d d%.1f hp%.0f w%.2f s%.2f st%.2f b%.2f tok%d",i,hypotf(n->x-P.x,n->z-P.z),n->hp,n->wind,n->swing,n->stag_t,n->blk_t,tok_owner);}printf("\n");}
  if(f%600==0){int ch=0,ko=0;for(int i=0;i<NID;i++)if(npc[i].type==NT_GUARD){if(npc[i].st==GS_CHASE)ch++;if(npc[i].st==GS_KO)ko++;}printf("t=%3ds hp=%5.1f бой=%.1f погоня=%d повержено=%d\n",f/60,P.hp,P.combat,ch,ko);}}
 for(int i=0;i<NID;i++)if(npc[i].type==NT_GUARD&&npc[i].st==GS_KO)kos++;
 printf("итог: навык %d — контратак %d, уклонов %d, получено ударов %d, поражений %d, стражников без сознания %d\n",skill,counters,dodges,hits_taken,deaths,kos);return 0;}
