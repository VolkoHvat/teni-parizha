/* ===== Звук и музыка: процедурный синтезатор ===== */
/* барокко и рококо: клавесин (альбертиевы басы, трели, форшлаги), струнные, флейта, орган;
   джазовые обороты: свинг, септаккорды, ходячий бас, блюзовая терция, щётки;
   симфонический рок в погоне и гонках: перегруженная гитара, ударные, медь, струнные остинато. */
#define SR 44100
#define NV 64
typedef struct{float f,ph,ph2,vol,pan,age,dur,del,a,b,lp;int type,on;}Voice;
static Voice vc[NV];
enum{V_HARP,V_STR,V_BASS,V_FLUTE,V_ORGAN,V_GTR,V_KICK,V_SNARE,V_HAT,V_BRASS,V_BELL,V_NOISE,V_SWEEP,V_VOWEL,V_TOM};
enum{MUS_EXPLORE,MUS_STEALTH,MUS_CHASE,MUS_RACE,MUS_TITLE,MUS_QUIET};
enum{S_FOOT=1,S_JUMP,S_LAND,S_HAY,S_CLIMB,S_BUMP,S_HIT,S_ALERT,S_GATE,S_START,S_FANFARE,S_COLLECT,S_CLICK,S_BELL,S_ROLL,S_TURN,S_BRUSH,S_COUNT,S_FAIL,S_GRAB,S_SHOT,S_SWISH,S_WINDUP,S_CLASH,S_THUD,S_HORSE,S_PIGEON,S_CRIER,S_THUNDER};
typedef struct{int type;float a,b;}SfxEv;
static SfxEv sq[64];static volatile int sq_w,sq_r;
static volatile int mus_mode=MUS_TITLE;static volatile float amb_crowd,amb_wind,amb_water,amb_rain,a_master=.85f;
static void sfx(int type,float a,float b){int w=sq_w;if(w-sq_r>=63)return;sq[w&63].type=type;sq[w&63].a=a;sq[w&63].b=b;sq_w=w+1;}
static unsigned arng=12345u;static float nz(void){arng=arng*1664525u+1013904223u;return ((arng>>9)&0xFFFF)/32768.f-1.f;}
static float mtof(float m){return 440.f*powf(2.f,(m-69.f)/12.f);}
/* del — задержка в сэмплах */
static void vo(int type,float f,float vol,float dur,float pan,float del,float a,float b){int k=-1;float oldest=-1;
 for(int i=0;i<NV;i++){if(!vc[i].on){k=i;break;}if(vc[i].age>oldest){oldest=vc[i].age;k=i;}}
 Voice*v=&vc[k];v->type=type;v->f=f;v->ph=0;v->ph2=nz()*3;v->vol=vol;v->pan=pan;v->age=0;v->dur=dur;v->del=del;v->a=a;v->b=b;v->lp=0;v->on=1;}
static float envr(Voice*v,float at,float rel){float e=v->age<at?v->age/at:1.f;if(v->age>v->dur)e*=expf(-(v->age-v->dur)*rel);return e;}
static float voice_sample(Voice*v){const float dt=1.f/SR;float s=0,e,w=6.2831853f*v->f*dt;
 if(v->del>0){v->del-=1.f;return 0;}
 switch(v->type){
 case V_HARP:{e=expf(-v->age*(2.6f+v->f*.0036f))*fminf(1.f,v->age*600.f);float x=sinf(v->ph),c=cosf(v->ph),x2=2*x*c,x3=3*x-4*x*x*x,x4=2*x2*(1-2*x*x);
  s=(x+.62f*x2+.38f*x3+.22f*x4+.1f*nz()*expf(-v->age*90.f))*e*.3f;break;}
 case V_STR:{e=envr(v,.28f,4.5f);float vb=1+.004f*sinf(v->age*34.f),x=0;for(int h=1;h<=5;h++)x+=sinf(v->ph*h*vb)/h+sinf(v->ph2*h)/h*.7f;s=x*e*.12f;v->ph2+=w*1.004f;break;}
 case V_BASS:{e=expf(-v->age*(v->a>0?v->a:3.0f))*fminf(1.f,v->age*400.f);s=(sinf(v->ph)+.5f*sinf(v->ph*2)*expf(-v->age*9.f)+.2f*sinf(v->ph*3)*expf(-v->age*14.f))*e*.55f;break;}
 case V_FLUTE:{e=envr(v,.07f,12.f);float vb=1+.006f*sinf(v->age*35.f)*fminf(1.f,v->age*3.f);s=(sinf(v->ph*vb)+.22f*sinf(v->ph*2*vb)+.06f*sinf(v->ph*3*vb)+.03f*nz())*e*.32f;break;}
 case V_ORGAN:{e=envr(v,.06f,9.f);s=(sinf(v->ph)+.5f*sinf(v->ph*2)+.34f*sinf(v->ph*3)+.25f*sinf(v->ph*4)+.15f*sinf(v->ph*6))*e*.14f;break;}
 case V_GTR:{e=envr(v,.004f,v->b>0?v->b:14.f)*expf(-v->age*(v->a>0?v->a:1.2f));float x=0;for(int h=1;h<=7;h++)x+=sinf(v->ph*h)/h;s=tanhf(x*3.4f)*e*.34f;break;}
 case V_KICK:{float fr=46+95*expf(-v->age*30.f);v->ph+=6.2831853f*fr*dt;s=sinf(v->ph)*expf(-v->age*8.f)*.9f+nz()*expf(-v->age*120.f)*.2f;break;}
 case V_SNARE:{s=nz()*expf(-v->age*16.f)*.6f+sinf(v->ph)*expf(-v->age*22.f)*.4f;break;}
 case V_HAT:{float n=nz();v->lp+=(n-v->lp)*.8f;s=(n-v->lp)*expf(-v->age*(v->a>0?v->a:50.f))*.5f;break;}
 case V_BRASS:{e=envr(v,.035f,10.f);float br=.5f+.5f*fminf(1.f,v->age*8.f),x=0,pw=1;for(int h=1;h<=8;h++){x+=sinf(v->ph*h)/h*pw;pw*=br;}s=x*e*.2f;break;}
 case V_BELL:{static const float pr[5]={1.f,2.f,2.76f,5.4f,8.93f};for(int h=0;h<5;h++)s+=sinf(v->ph*pr[h])*expf(-v->age*(.9f+h*.7f))/(h+1);s*=.28f;break;}
 case V_NOISE:{float n=nz();v->lp+=(n-v->lp)*(v->a>0?v->a:.3f);s=v->lp*expf(-v->age*(v->b>0?v->b:30.f))*.8f;break;}
 case V_SWEEP:{float T=v->dur>0?v->dur:.2f,t=v->age/T;if(t>1){v->on=0;return 0;}float fr=v->f*(1+(v->a-1)*t);v->ph+=6.2831853f*fr*dt;s=sinf(v->ph)*sinf(3.14159f*t)*.4f;break;}
 case V_VOWEL:{float T=v->dur>0?v->dur:.3f,t=v->age/T;if(t>1){v->on=0;return 0;}float g=1-.28f*t;s=(sinf(v->ph*g)+.5f*sinf(v->ph*g*3.1f)+.25f*sinf(v->ph*g*5.2f))*sinf(3.14159f*t)*.18f;break;}
 case V_TOM:{float fr=v->f*(1+.9f*expf(-v->age*20.f));v->ph+=6.2831853f*fr*dt;s=sinf(v->ph)*expf(-v->age*7.f)*.6f;break;}}
 if(v->type!=V_KICK&&v->type!=V_SWEEP&&v->type!=V_TOM)v->ph+=w;
 if(v->ph>6.2831853f*64)v->ph-=6.2831853f*64;v->age+=dt;
 float tail=(v->type==V_STR||v->type==V_FLUTE||v->type==V_ORGAN||v->type==V_BRASS)?v->dur+1.4f:v->type==V_BELL?5.f:v->type==V_HARP?2.4f:v->type==V_BASS?1.6f:v->type==V_GTR?v->dur+.6f:.7f;
 if(v->age>tail)v->on=0;return s;}
/* ---- реверберация: 4 гребёнки на канал ---- */
static float cb[8][1900];static int cbp[8];static const int cbl[8]={1307,1471,1601,1787,1361,1523,1693,1873};
static float reverb(float x,int ch){float y=0;for(int i=0;i<4;i++){int k=i+ch*4;float o=cb[k][cbp[k]];cb[k][cbp[k]]=x*.3f+o*.8f;if(++cbp[k]>=cbl[k])cbp[k]=0;y+=o;}return y*.5f;}
/* ---- секвенсор ---- */
typedef struct{int root,type;}Ch;
static const int CHT[8][5]={{0,4,7,12,16},{0,4,7,11,14},{0,3,7,12,15},{0,3,7,10,14},{0,4,7,10,14},{0,5,7,10,12},{0,7,12,19,24},{0,3,6,9,12}};
enum{C_M,C_M7,C_m,C_m7,C_D7,C_SUS,C_PW,C_DIM};
/* прогулка: ре мажор, джазовый ii–V с септаккордами; второй период — соль мажор */
static const Ch PR_EXP[2][8]={{{50,C_M7},{47,C_m7},{52,C_m7},{45,C_D7},{54,C_m7},{47,C_D7},{52,C_m7},{45,C_D7}},{{55,C_M7},{55,C_m},{50,C_M7},{47,C_D7},{52,C_m7},{45,C_D7},{50,C_M7},{50,C_M}}};
static const Ch PR_ST[8]={{50,C_m},{50,C_m},{55,C_m},{50,C_m},{46,C_M},{55,C_m},{45,C_SUS},{45,C_D7}};
static const Ch PR_CH[8]={{50,C_m},{46,C_M},{48,C_M},{45,C_M},{50,C_m},{46,C_M},{55,C_m},{45,C_D7}};
static const Ch PR_RC[8]={{50,C_M},{45,C_M},{47,C_m},{43,C_M},{50,C_M},{45,C_M},{43,C_M},{45,C_D7}};
static const int RHY[6][10]={{0,3,4,6,8,10,12,14,-1,-1},{0,2,4,5,6,8,11,12,14,-1},{0,4,6,8,12,14,15,-1,-1,-1},{0,3,6,8,10,13,-1,-1,-1,-1},{0,2,3,4,8,10,12,15,-1,-1},{0,4,8,10,12,14,-1,-1,-1,-1}};
static const int MAJ[7]={0,2,4,5,7,9,11},MIN[7]={0,2,3,5,7,8,10};
/* слои: 0 клавесин, 1 струнные, 2 бас, 3 мелодия, 4 щётки, 5 рок, 6 орган */
static const float LAYG[6][7]={{1.f,.7f,.8f,.9f,.6f,0,0},{.35f,1.f,.5f,0,0,0,.4f},{.6f,.9f,1.f,.8f,0,1.f,0},{.7f,.8f,1.f,.9f,0,1.f,0},{1.f,.9f,.6f,.9f,.2f,0,.7f},{0,.4f,0,0,0,0,.25f}};
static const float BPMM[6]={112,78,152,158,96,80};
static float lay[7],g_bpm=96,sacc,spstep=SR*60.f/96/4;static int step_idx,mel_m=74,mel_rhy,mel_ci;
static Ch chord_at(int mode,int bar,int sec){switch(mode){case MUS_EXPLORE:case MUS_TITLE:return PR_EXP[sec&1][bar&7];case MUS_STEALTH:return PR_ST[bar&7];case MUS_CHASE:return PR_CH[bar&7];default:return PR_RC[bar&7];}}
static int pick_scale(int midi,const int*sc,int ds){int pc=((midi-50)%12+12)%12,best=0,bd=99;for(int i=0;i<7;i++){int d=abs(sc[i]-pc);if(d<bd){bd=d;best=i;}}int ni=best+ds,oct=0;while(ni<0){ni+=7;oct--;}while(ni>=7){ni-=7;oct++;}return midi-pc+oct*12+sc[ni];}
static unsigned lcg=777u;static int rnd(int n){lcg=lcg*1103515245u+12345u;return (int)((lcg>>16)%(unsigned)n);}
static void seq_step(void){int mode=mus_mode;if(mode<0||mode>5)mode=0;int s16=step_idx%16,bar=(step_idx/16)%8,sec=step_idx/128,rock=(mode==MUS_CHASE||mode==MUS_RACE),minor=(mode==MUS_STEALTH||mode==MUS_CHASE);
 Ch ch=chord_at(mode,bar,sec),nx=chord_at(mode,(bar+1)&7,sec+(bar==7));const int*ct=CHT[ch.type];
 float swing=mode==MUS_EXPLORE?.55f:mode==MUS_TITLE?.3f:rock?.12f:0,sd=((s16%4)==2)?swing*spstep*.66f:0;
 if(s16==0&&bar==0){lcg=(unsigned)(sec*7919+13);mel_rhy=rnd(6);}if(s16==0)mel_ci=rnd(6);
 /* клавесин: альбертиевы басы */
 if(lay[0]>.02f&&(mode!=MUS_STEALTH||s16%4==0)){int rate=rock?1:2;if(s16%rate==0){static const int pat[4]={0,2,1,2};int idx=pat[(s16/rate)%4],m=ch.root+12+ct[idx];if(m>66)m-=12;vo(V_HARP,mtof((float)m),.20f*lay[0],.2f,-.25f,sd,0,0);}
  if(s16==0)vo(V_HARP,mtof((float)ch.root),.30f*lay[0],.4f,-.1f,0,0,0);}
 /* струнные */
 if(lay[1]>.02f){if(!rock){if(s16==0)for(int i=0;i<4;i++)vo(V_STR,mtof((float)(ch.root+12+ct[i])),.5f*lay[1],spstep*15.f/SR,(i-1.5f)*.25f,0,0,0);}
  else{if(s16==0)for(int i=0;i<3;i++)vo(V_STR,mtof((float)(ch.root+12+ct[i])),.5f*lay[1],spstep*13.f/SR,(i-1)*.3f,0,0,0);
   if(s16==6||s16==10)for(int i=0;i<3;i++)vo(V_STR,mtof((float)(ch.root+24+ct[i])),.45f*lay[1],spstep*1.5f/SR,(i-1)*.4f,0,0,0);}}
 /* орган */
 if(lay[6]>.02f&&s16==0)for(int i=0;i<3;i++)vo(V_ORGAN,mtof((float)(ch.root+(i?12:0)+ct[i])),.5f*lay[6],spstep*15.f/SR,0,0,0,0);
 /* бас: в прогулке — джазовый ходячий бас с хроматическим подходом */
 if(lay[2]>.02f){if(rock){if(s16%2==0)vo(V_BASS,mtof((float)(ch.root-12)),.55f*lay[2],.2f,0,0,5.f,0);}
  else if(mode==MUS_STEALTH){if(s16==0)vo(V_BASS,mtof((float)(ch.root-12)),.5f*lay[2],2.f,0,0,.8f,0);}
  else if(s16%4==0){int beat=s16/4,m=ch.root-12;if(beat==1)m=ch.root-12+ct[1];if(beat==2)m=ch.root-12+ct[2];if(beat==3){int d=nx.root-ch.root;m=nx.root-12+((d>=0&&d<=5)?-1:1);}vo(V_BASS,mtof((float)m),.6f*lay[2],.3f,0,0,3.2f,0);}}
 /* мелодия с форшлагами, трелями и редкой блюзовой терцией */
 if(lay[3]>.02f&&mode!=MUS_STEALTH){int onset=0;for(int i=0;i<10;i++)if(RHY[mel_rhy][i]==s16)onset=1;
  if(onset){static const int dir[9]={-2,-1,-1,0,1,1,2,3,-3};int d=dir[rnd(9)],nm=pick_scale(mel_m,minor?MIN:MAJ,d);
   if(s16%4==0){int bd=99,bm=nm;for(int i=0;i<5;i++)for(int o=-2;o<=2;o++){int c=ch.root+ct[i]+12*o,dd=abs(c-nm);if(dd<bd&&c>=62&&c<=88){bd=dd;bm=c;}}if(bd<=2)nm=bm;}
   if(nm<62)nm+=12;if(nm>88)nm-=12;if(!minor&&mode==MUS_EXPLORE&&rnd(9)==0)nm=ch.root+12*((nm-ch.root)/12)+3;
   mel_m=nm;int inst=(mode==MUS_TITLE||(mode==MUS_EXPLORE&&((sec+mel_ci)&1)==0))?V_FLUTE:V_HARP;if(rock)inst=(sec&1)?V_BRASS:V_FLUTE;float len=spstep*(2+rnd(3))/SR;
   if(inst==V_HARP)vo(V_HARP,mtof((float)nm),.33f*lay[3],.3f,.2f,sd,0,0);else vo(inst,mtof((float)nm),.55f*lay[3],len,.2f,sd,0,0);
   if(rnd(5)==0){int up=pick_scale(nm,minor?MIN:MAJ,1);vo(V_HARP,mtof((float)up),.18f*lay[3],.12f,.2f,sd>0?sd-spstep*.08f:0,0,0);}
   if(rnd(6)==0&&s16%2==0)for(int t=1;t<=3;t++){int tn=(t&1)?pick_scale(nm,minor?MIN:MAJ,1):nm;vo(V_HARP,mtof((float)tn),.2f*lay[3],.12f,.2f,spstep*.5f*t+sd,0,0);}}}
 /* джазовые щётки */
 if(lay[4]>.02f){if(s16%4==0){vo(V_HAT,0,.5f*lay[4],.06f,.15f,0,35.f,0);if(s16==0||s16==8)vo(V_KICK,0,.16f*lay[4],.2f,0,0,0,0);}if(s16%4==2)vo(V_HAT,0,.35f*lay[4],.06f,.15f,sd,28.f,0);if(s16==4||s16==12)vo(V_NOISE,0,.28f*lay[4],.2f,0,0,.15f,12.f);}
 /* симфо-рок: ударные, квинтовые аккорды гитары, медь */
 if(lay[5]>.02f){float g=lay[5];if(s16==0||s16==6||s16==8||s16==10)vo(V_KICK,0,.8f*g,.3f,0,0,0,0);if(s16==4||s16==12)vo(V_SNARE,190,.55f*g,.2f,0,0,0,0);if(s16%2==0)vo(V_HAT,0,.4f*g,.05f,.3f,0,s16==14?14.f:55.f,0);
  if(bar==7&&s16>=12)vo(V_TOM,160-(s16-12)*22.f,.6f*g,.3f,(s16-13)*.3f,0,0,0);
  if(s16%2==0){int pw=(s16==0||s16==6||s16==10);float dur=pw?spstep*(s16==0?6.f:3.f)/SR:.1f;for(int i=0;i<3;i++){int m=ch.root+(i==0?0:i==1?7:12);vo(V_GTR,mtof((float)m),(pw?.55f:.38f)*g,dur,(i-1)*.15f,0,pw?1.5f:9.f,pw?3.f:16.f);}}
  if(s16==0||s16==8||(s16==10&&bar%2==1))for(int i=0;i<3;i++)vo(V_BRASS,mtof((float)(ch.root+12+ct[i])),.32f*g,spstep*3.f/SR,(i-1)*.3f,0,0,0);}
 if(s16==0&&mode==MUS_TITLE&&bar==0&&sec%2==0)vo(V_BELL,mtof(50.f),.28f,3.f,0,0,0,0);}
static void sfx_play(const SfxEv*e){float a=e->a>0?e->a:1.f;switch(e->type){
 case S_FOOT:vo(V_NOISE,0,.3f*a,.04f,0,0,.5f,80.f);vo(V_NOISE,0,.2f*a,.03f,0,.01f*SR,.15f,90.f);break;
 case S_JUMP:vo(V_SWEEP,260,.5f*a,.18f,0,0,2.2f,0);vo(V_NOISE,0,.18f,.12f,0,0,.35f,25.f);break;
 case S_LAND:vo(V_KICK,0,.5f*a,.15f,0,0,0,0);vo(V_NOISE,0,.4f*a,.1f,0,0,.2f,30.f);break;
 case S_HAY:vo(V_NOISE,0,.5f,.25f,0,0,.25f,10.f);vo(V_NOISE,0,.3f,.3f,0,.05f*SR,.5f,8.f);break;
 case S_CLIMB:vo(V_NOISE,0,.25f*a,.07f,0,0,.6f,45.f);break;
 case S_GRAB:vo(V_NOISE,0,.4f,.06f,0,0,.7f,60.f);vo(V_KICK,0,.2f,.1f,0,0,0,0);break;
 case S_BUMP:vo(V_VOWEL,e->b>0?e->b:200.f,.6f*a,.28f,0,0,0,0);vo(V_NOISE,0,.3f*a,.08f,0,0,.15f,50.f);break;
 case S_HIT:vo(V_SNARE,150,.6f,.2f,0,0,0,0);vo(V_KICK,0,.4f,.2f,0,0,0,0);break;
 case S_ALERT:vo(V_BRASS,mtof(76),.55f,.18f,0,0,0,0);vo(V_BRASS,mtof(83),.55f,.3f,0,.14f*SR,0,0);break;
 case S_GATE:vo(V_BELL,mtof(81),.35f,.3f,.2f,0,0,0);vo(V_HARP,mtof(88),.3f,.2f,.2f,.06f*SR,0,0);break;
 case S_START:vo(V_BRASS,mtof(62),.5f,.5f,0,0,0,0);vo(V_BRASS,mtof(69),.5f,.5f,0,0,0,0);vo(V_BRASS,mtof(74),.5f,.6f,0,0,0,0);break;
 case S_FANFARE:{static const int n[6]={62,66,69,74,78,81};for(int i=0;i<6;i++){vo(V_BRASS,mtof((float)n[i]),.5f,.22f,(i-3)*.1f,i*.13f*SR,0,0);vo(V_HARP,mtof((float)n[i]+12),.3f,.3f,0,i*.13f*SR,0,0);}
  vo(V_STR,mtof(62),.5f,1.8f,0,0,0,0);vo(V_STR,mtof(78),.5f,1.8f,0,0,0,0);vo(V_BELL,mtof(86),.3f,1.f,0,.8f*SR,0,0);break;}
 case S_FAIL:vo(V_BRASS,mtof(62),.45f,.3f,0,0,0,0);vo(V_BRASS,mtof(61),.45f,.3f,0,.25f*SR,0,0);vo(V_BRASS,mtof(56),.5f,.7f,0,.5f*SR,0,0);break;
 case S_COLLECT:vo(V_HARP,mtof(86),.4f,.2f,.2f,0,0,0);vo(V_HARP,mtof(90),.4f,.2f,.2f,.07f*SR,0,0);vo(V_HARP,mtof(93),.4f,.3f,.2f,.14f*SR,0,0);break;
 case S_SHOT:vo(V_NOISE,0,.9f*a,.35f,0,0,.9f,9.f);vo(V_KICK,0,.7f*a,.25f,0,0,0,0);vo(V_NOISE,0,.35f*a,1.2f,0,.05f*SR,.12f,2.5f);break;
 case S_SWISH:vo(V_SWEEP,900,.25f,.12f,0,0,.35f,0);vo(V_NOISE,0,.25f,.14f,0,0,.7f,20.f);break;
 case S_WINDUP:vo(V_VOWEL,120.f+40.f*a,.45f,.3f,0,0,0,0);vo(V_NOISE,0,.12f,.2f,0,0,.5f,12.f);break;
 case S_CLASH:vo(V_BELL,mtof(93),.35f,.5f,.1f,0,0,0);vo(V_BELL,mtof(98.3f),.25f,.4f,-.1f,0,0,0);vo(V_NOISE,0,.4f,.08f,0,0,.9f,60.f);break;
 case S_THUD:vo(V_KICK,0,.7f,.3f,0,0,0,0);vo(V_NOISE,0,.4f,.2f,0,0,.15f,14.f);break;
 case S_HORSE:for(int k=0;k<4;k++){vo(V_KICK,0,.25f*a,.08f,0,(k*.11f+(k>1?.07f:0))*SR,0,0);vo(V_NOISE,0,.15f*a,.04f,0,(k*.11f+(k>1?.07f:0))*SR,.6f,70.f);}break;
 case S_PIGEON:for(int k=0;k<6;k++)vo(V_NOISE,0,.16f*a,.05f,(k&1)?.3f:-.3f,k*.06f*SR,.5f,40.f);break;
 case S_CRIER:vo(V_VOWEL,160.f,.4f*a,.6f,0,0,0,0);vo(V_VOWEL,190.f,.4f*a,.5f,0,.65f*SR,0,0);vo(V_VOWEL,150.f,.4f*a,.8f,0,1.2f*SR,0,0);break;
 case S_THUNDER:vo(V_NOISE,0,.8f*a,3.5f,0,0,.05f,1.2f);vo(V_KICK,0,.5f*a,.8f,0,.1f*SR,0,0);break;
 case S_CLICK:vo(V_NOISE,0,.2f,.02f,0,0,.8f,150.f);break;
 case S_BELL:vo(V_BELL,mtof(55),.45f*a,4.f,0,0,0,0);break;
 case S_ROLL:vo(V_NOISE,0,.4f,.3f,0,0,.2f,6.f);break;
 case S_TURN:vo(V_SWEEP,500,.3f,.15f,0,0,.5f,0);vo(V_NOISE,0,.2f,.1f,0,0,.5f,30.f);break;
 case S_BRUSH:vo(V_NOISE,0,.18f*a,.12f,0,0,.2f,18.f);break;
 case S_COUNT:{float m=e->b>0?e->b:76;vo(V_HARP,mtof(m),.5f,.3f,0,0,0,0);vo(V_BELL,mtof(m+12),.2f,.3f,0,0,0,0);break;}}}
void game_audio(int16_t*out,int frames){static float l1,l2,l3;
 for(int i=0;i<frames;i++){
  while(sq_r!=sq_w){SfxEv e=sq[sq_r&63];sq_r=sq_r+1;sfx_play(&e);}
  int m=mus_mode;if(m<0||m>5)m=0;for(int l=0;l<7;l++)lay[l]+=(LAYG[m][l]-lay[l])*.00006f;g_bpm+=(BPMM[m]-g_bpm)*.00003f;spstep=SR*60.f/g_bpm/4.f;
  sacc+=1;if(sacc>=spstep){sacc-=spstep;seq_step();step_idx++;}
  float L=0,R=0;for(int k=0;k<NV;k++){Voice*v=&vc[k];if(!v->on)continue;float s=voice_sample(v)*v->vol;L+=s*(1-v->pan)*.6f;R+=s*(1+v->pan)*.6f;}
  float nn=nz();l1+=(nn-l1)*.04f;l2+=(nn-l2)*.004f;l3+=(nn-l3)*.18f;static float l4;l4+=(nn-l4)*.6f;float amb=l1*amb_crowd*.4f+l2*amb_wind*1.4f+l3*amb_water*.1f+(nn-l4)*amb_rain*.16f*(.8f+.4f*nz());
  float mono=(L+R)*.5f,wl=reverb(mono,0),wr=reverb(mono,1);
  out[2*i]=(int16_t)(tanhf((L+wl*.35f+amb)*a_master)*30000);out[2*i+1]=(int16_t)(tanhf((R+wr*.35f+amb)*a_master)*30000);}}
