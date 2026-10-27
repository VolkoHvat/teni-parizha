/* ===== Персонажи 1789 года: герой в плаще с капюшоном, стража, горожане.
   Части тела — отдельные меши на шарнирах; поза плавно смешивается (бег, прыжок, лазание, вис, бой, падение). ===== */
typedef struct{uint32_t skin,hair,eye,coat,pants,trim,shoe;int type,extra;}Look;
enum{PT_HERO,PT_GUARD,PT_MAN,PT_WOMAN,PT_MERCH,PT_COURIER};
enum{PK_TORSO,PK_THIGHL,PK_THIGHR,PK_SHINL,PK_SHINR,PK_UARML,PK_UARMR,PK_FARML,PK_FARMR,PK_HEAD,PK_TAIL,PK_SKIRT,PK_SWORD,PK_TAILL,PK_TAILR};
typedef struct{Mesh m;int kind;V3 piv;int usetex;}MPart;
typedef struct{MPart p[48];int n;Tex face;Look lk;int det;}Model;
static uint32_t facebuf[256*256];
static float HSC=1.f;static int SEG=14,RING=9;
#define HEADY 1.36f
#define E(x,y,z,rx,ry,rz,c) mb_sphere(v3(x,y,z),v3(rx,ry,rz),SEG,RING,c,0)
#define EQ(x,y,z,rx,ry,rz,c) mb_sphere(v3(x,y,z),v3(rx,ry,rz),8,6,c,0)
#define LA(base,arr,n,seg,c,sx,sz) mb_lathe(base,arr,n,seg,c,sx,sz)
/* лицо: реалистичные пропорции, мягкие тени, подробные глаза */
static void face_paint(Model*m){const Look*lk=&m->lk;int male=lk->type!=PT_WOMAN;gfx_target(facebuf,256,256);
 uint32_t skin=lk->skin,skd=gfx_lerp(skin,RGBX(120,70,60),.32f),skl=gfx_lerp(skin,RGBX(255,240,230),.25f),hd=gfx_lerp(lk->hair,0x000000,.25f);
 gfx_rect(0,0,256,256,skin,255);
 for(int r=0;r<14;r++){gfx_ellipse(128,130,128-r*3,128-r*3,gfx_lerp(skd,skin,r/14.f),40);}gfx_ellipse(128,70,70,40,skl,50);gfx_ellipse(84,168,22,16,skl,40);gfx_ellipse(172,168,22,16,skl,40);
 gfx_ellipse(128,40,110,40,gfx_lerp(skin,lk->hair,.25f),70);
 gfx_ellipse(80,176,24,14,RGBX(220,120,110),male?24:42);gfx_ellipse(176,176,24,14,RGBX(220,120,110),male?24:42);
 for(int s=-1;s<=1;s+=2){float ex=128+s*44,ey=136;
  gfx_ellipse(ex,ey-2,26,15,skd,70);
  gfx_ellipse(ex,ey,17,8.5f,RGBX(240,236,232),255);gfx_circle(ex+s*1,ey,7.6f,gfx_lerp(lk->eye,0x000000,.15f),255);gfx_circle(ex+s*1,ey+1,5.6f,gfx_lerp(lk->eye,0xFFFFFF,.18f),200);
  gfx_circle(ex+s*1,ey,3.2f,RGBX(16,12,14),255);gfx_circle(ex+s*1-2.5f,ey-2.5f,1.6f,RGBX(255,255,255),230);
  gfx_line(ex-18,ey+1,ex-8,ey-8,3.2f,RGBX(60,38,36),255);gfx_line(ex-8,ey-8,ex+9,ey-8,3.2f,RGBX(60,38,36),255);gfx_line(ex+9,ey-8,ex+18,ey,3.2f,RGBX(60,38,36),255);
  gfx_line(ex-15,ey+7,ex+15,ey+7,1.6f,skd,150);gfx_line(ex-16,ey-14,ex+16,ey-14,2.f,skd,90);
  if(!male){gfx_line(ex+s*16,ey-3,ex+s*21,ey-7,2.2f,RGBX(40,26,26),255);}
  gfx_line(ex-20,ey-22,ex,ey-27,male?6.5f:4.f,hd,255);gfx_line(ex,ey-27,ex+20,ey-23+s*1,male?5.f:3.f,hd,255);
  if(lk->extra&8){gfx_ring(ex,ey,22,3,RGBX(110,90,60),255);}}
 if(lk->extra&8)gfx_line(150,136,106,136,3,RGBX(110,90,60),255);
 gfx_line(124,140,121,182,4,skd,90);gfx_ellipse(128,186,13,8,skl,90);gfx_ellipse(119,190,4,2.5f,gfx_lerp(skd,0x000000,.3f),200);gfx_ellipse(137,190,4,2.5f,gfx_lerp(skd,0x000000,.3f),200);gfx_ellipse(128,196,14,4,skd,60);
 {uint32_t lip=male?gfx_lerp(skin,RGBX(160,80,80),.45f):RGBX(190,78,96);gfx_ellipse(128,214,18,5,gfx_lerp(lip,0x000000,.15f),230);gfx_ellipse(128,220,16,5,gfx_lerp(lip,0xFFFFFF,.12f),230);gfx_line(110,216,146,216,2.f,gfx_lerp(lip,0x000000,.5f),220);
  gfx_ellipse(128,232,12,4,skd,70);}
 gfx_ellipse(128,246,40,14,skd,40);
 if(male&&(lk->extra&1)){gfx_ellipse(128,236,40,22,hd,200);gfx_ellipse(128,208,26,6,hd,220);gfx_ellipse(128,218,13,6,gfx_lerp(lk->skin,RGBX(160,80,80),.4f),200);}
 else if(male&&lk->type!=PT_HERO){for(int k=0;k<220;k++){uint32_t h=hs(k,(int)lk->skin);float a=rf(h)*3.14f,r=rf(h>>8);float x=128+cosf(a)*r*44,y=226+sinf(a)*r*24;gfx_px((int)x,(int)y,hd,50);}}
 if(lk->extra&2)for(int k=0;k<22;k++){uint32_t h=hs(k,9);gfx_circle(88+rf(h)*80,160+rf(h>>8)*22,1.5f,RGBX(170,108,84),150);}
 if(lk->extra&16){gfx_line(84,104,66,74,4,RGBX(176,110,100),220);}
 if(lk->extra&128){uint32_t wr=gfx_lerp(skin,RGBX(120,80,70),.4f);for(int k=0;k<3;k++)gfx_line(76+k*4,56+k*9,180-k*4,56+k*9,2.f,wr,140);gfx_line(84,150,68,166,2.2f,wr,150);gfx_line(172,150,188,166,2.2f,wr,150);gfx_line(100,206,110,226,2.2f,wr,130);gfx_line(156,206,146,226,2.2f,wr,130);}
 gfx_target_screen();}
static void face_store(Model*m){int s=m->face.w;if(s==256){memcpy(m->face.px,facebuf,sizeof(uint32_t)*256*256);return;}int k=256/s;for(int y=0;y<s;y++)for(int x=0;x<s;x++){int r=0,g=0,b=0;for(int j=0;j<k;j++)for(int i=0;i<k;i++){uint32_t p=facebuf[(y*k+j)*256+x*k+i];r+=(p>>16)&255;g+=(p>>8)&255;b+=p&255;}m->face.px[y*s+x]=(uint32_t)(((r/(k*k))<<16)|((g/(k*k))<<8)|(b/(k*k)));}}
static void addp(Model*m,int kind,V3 piv,int usetex){if(m->n>=48)return;MPart*p=&m->p[m->n++];p->m=mb_end();p->kind=kind;p->piv=piv;p->usetex=usetex;
 if(HSC!=1.f&&piv.y>1.3f)for(int i=0;i<p->m.nv;i++){p->m.v[i].x*=HSC;p->m.v[i].y*=HSC;p->m.v[i].z*=HSC;}}
static float skR(float t){static const float tt[7]={0,.05f,.12f,.28f,.5f,.75f,1.f},rd[7]={1.0f,.93f,.86f,.74f,.56f,.38f,.25f};for(int k=0;k<6;k++)if(t<=tt[k+1])return rd[k]+(rd[k+1]-rd[k])*(t-tt[k])/(tt[k+1]-tt[k]);return rd[6];}
static void hand(float s,uint32_t c){E(0,-.315f,.063f,.036f,.052f,.022f,c);float fx[4]={-.024f,-.008f,.008f,.024f};for(int k=0;k<4;k++){mb_tube(v3(fx[k],-.35f,.063f),v3(fx[k]*1.05f,-.39f,.072f),.0098f,.0088f,6,c);mb_tube(v3(fx[k]*1.05f,-.39f,.072f),v3(fx[k]*1.08f,-.415f,.085f),.0088f,.0075f,6,c);}mb_tube(v3(s*.03f,-.30f,.06f),v3(s*.045f,-.35f,.088f),.011f,.009f,6,c);}
static void tricorn(uint32_t c,uint32_t cock){float cr[]={.12f,.34f,.118f,.44f,.06f,.455f,0,.46f};LA(v3(0,0,0),cr,4,18,c,1,1);
 for(int k=0;k<3;k++){float a=k*2.0944f+3.14159f;int b=mb_count();for(int j=0;j<=6;j++){float t=(j/6.f-.5f)*1.9f,aa=a+t;float rr=.14f+.16f*cosf(t*1.6f);float yy=.345f+.09f*(1-cosf(t*1.6f))+.03f;
   mb_vert(sinf(aa)*.115f,.345f,cosf(aa)*.115f,0,1,0,0,0,c);mb_vert(sinf(aa)*rr,yy,cosf(aa)*rr,0,1,0,0,0,c);}
  for(int j=0;j<6;j++){int q=b+j*2;mb_index(q,q+1,q+2);mb_index(q+1,q+3,q+2);mb_index(q,q+2,q+1);mb_index(q+1,q+2,q+3);}}
 E(0,.40f,.13f,.04f,.04f,.015f,cock);mb_tube(v3(-.2f,.38f,.1f),v3(.2f,.38f,.1f),.006f,.006f,4,RGBX(214,180,90));}
static void phrygian(uint32_t c){float cr[]={.14f,.33f,.13f,.42f,.09f,.52f,.04f,.58f};LA(v3(0,0,0),cr,4,14,c,1,1);E(.06f,.62f,.0f,.05f,.06f,.05f,c);E(.1f,.58f,.02f,.045f,.05f,.045f,c);E(.06f,.4f,.13f,.035f,.035f,.01f,RGBX(60,90,200));E(.06f,.4f,.14f,.025f,.025f,.01f,RGBX(250,250,250));E(.06f,.4f,.15f,.014f,.014f,.01f,RGBX(220,50,60));}
/* открытая полу-оболочка (полы кафтана, фартук, капюшон): двусторонняя */
static void shell(V3 b,const float*pr,int n,int seg,float a0,float a1,uint32_t c,float sx,float sz){int base=mb_count();
 for(int i=0;i<n;i++){float r=pr[2*i],y=pr[2*i+1];for(int s=0;s<=seg;s++){float th=a0+(a1-a0)*s/seg;float sn=sinf(th),cs=cosf(th);mb_vert(b.x+r*sn*sx,b.y+y,b.z+r*cs*sz,sn,0,cs,0,0,c);}}
 for(int i=0;i<n-1;i++)for(int s=0;s<seg;s++){int a=base+i*(seg+1)+s,b2=a+seg+1;mb_index(a,a+1,b2);mb_index(a+1,b2+1,b2);mb_index(a,b2,a+1);mb_index(a+1,b2,b2+1);}}
static void build_head(Model*m,Look lk,int det){int t=lk.type;uint32_t hr=lk.hair,sk=lk.skin,tr=lk.trim;HSC=.7f;SEG=det?18:12;RING=det?12:8;
 mb_begin();mb_sphere(v3(0,.23f,0),v3(.145f,.17f,.155f),det?32:20,det?22:14,0xFFFFFF,1);addp(m,PK_HEAD,v3(0,HEADY,0),1);
 mb_begin();mb_cyl(v3(0,0,0),.036f,.039f,.12f,12,sk);EQ(0,.205f,.152f,.013f,.02f,.018f,sk);EQ(0,.188f,.158f,.018f,.012f,.014f,sk);EQ(-.147f,.22f,-.005f,.014f,.03f,.02f,sk);EQ(.147f,.22f,-.005f,.014f,.03f,.02f,sk);
 E(0,.09f,.11f,.09f,.06f,.06f,sk);
 if(t==PT_HERO){float hp[]={.16f,.0f,.195f,.12f,.2f,.26f,.18f,.38f,.12f,.45f,.02f,.48f};shell(v3(0,0,-.02f),hp,6,20,.75f,5.53f,lk.coat,1,1.1f);
  E(0,.36f,-.06f,.165f,.12f,.16f,lk.coat);E(0,.28f,-.16f,.13f,.15f,.08f,lk.coat);E(0,.42f,.08f,.12f,.05f,.06f,gfx_lerp(lk.coat,0x000000,.25f));
  E(0,.07f,.1f,.115f,.06f,.07f,RGBX(236,230,214));E(0,.14f,.14f,.1f,.025f,.03f,RGBX(236,230,214));E(0,.22f,.05f,.148f,.06f,.14f,hr);
  mb_tube(v3(-.17f,.05f,.12f),v3(.17f,.05f,.12f),.012f,.012f,6,tr);}
 else if(t==PT_GUARD){E(0,.255f,-.035f,.158f,.172f,.165f,hr);E(0,.12f,-.18f,.04f,.14f,.05f,hr);E(0,-.02f,-.2f,.03f,.05f,.04f,RGBX(20,20,24));tricorn(RGBX(26,26,34),RGBX(60,90,200));E(-.147f,.2f,0,.03f,.05f,.05f,RGBX(236,236,240));E(.147f,.2f,0,.03f,.05f,.05f,RGBX(236,236,240));}
 else if(t==PT_WOMAN){E(0,.255f,-.035f,.158f,.172f,.165f,hr);if(lk.extra&512){E(0,.46f,-.02f,.14f,.1f,.14f,hr);E(0,.54f,-.03f,.1f,.08f,.1f,hr);for(int k=0;k<6;k++){float a=k*1.047f;E(sinf(a)*.12f,.44f,cosf(a)*.12f-.02f,.05f,.05f,.05f,gfx_lerp(hr,0xFFFFFF,.1f));}}
  E(-.06f,.36f,.11f,.09f,.05f,.05f,hr);E(.07f,.355f,.11f,.09f,.05f,.05f,hr);E(0,.42f,-.12f,.09f,.09f,.08f,hr);E(0,.25f,-.2f,.07f,.09f,.06f,hr);
  if(lk.extra&1){float br[]={.2f,.37f,.22f,.4f,.16f,.45f,.0f,.5f};LA(v3(0,0,0),br,4,20,RGBX(244,240,230),1,1);float rb[]={.13f,.37f,.135f,.41f};LA(v3(0,0,0),rb,2,20,tr,1,1);}else{float br[]={.12f,.36f,.125f,.4f,.08f,.43f,0,.44f};LA(v3(0,0,0),br,4,16,tr,1,1);}}
 else{E(0,.26f,-.02f,.155f,.165f,.16f,hr);E(0,.36f,.1f,.12f,.05f,.05f,hr);E(0,.1f,-.18f,.045f,.12f,.05f,hr);if(lk.extra&512){E(-.15f,.2f,.0f,.045f,.045f,.05f,hr);E(.15f,.2f,.0f,.045f,.045f,.05f,hr);E(-.15f,.14f,.0f,.045f,.045f,.05f,hr);E(.15f,.14f,.0f,.045f,.045f,.05f,hr);E(0,.05f,-.19f,.04f,.04f,.04f,RGBX(24,24,30));}
  int v=lk.extra>>5&3;if(v==0)tricorn(RGBX(44,36,36),RGBX(60,90,200));else if(v==1)phrygian(RGBX(190,36,44));else{float cr[]={.14f,.34f,.135f,.5f,.1f,.52f,0,.52f};LA(v3(0,0,0),cr,4,16,RGBX(40,36,40),1,1);float br[]={.22f,.34f,.2f,.35f,.13f,.38f};LA(v3(0,0,0),br,3,16,RGBX(40,36,40),1,1);}}
 addp(m,PK_HEAD,v3(0,HEADY,0),0);HSC=1.f;}
static void build_person(Model*m,Look lk,int det){memset(m,0,sizeof *m);m->lk=lk;m->det=det;SEG=det?16:11;RING=det?11:7;HSC=1.f;int t=lk.type,woman=(t==PT_WOMAN),male=!woman;
 uint32_t sk=lk.skin,tr=lk.trim,bt=lk.pants,tp=lk.coat,sh=lk.shoe;int tseg=det?30:18,ls=det?14:10;float sw=male?.2f:.165f;
 mb_begin();
 if(woman){float bd[]={.172f,.86f,.176f,.92f,.15f,.985f,.118f,1.05f,.13f,1.115f,.148f,1.19f,.155f,1.25f,.145f,1.3f,.085f,1.335f,.05f,1.355f};LA(v3(0,0,0),bd,10,tseg,tp,1,.74f);E(-.052f,1.2f,.075f,.058f,.055f,.06f,tp);E(.052f,1.2f,.075f,.058f,.055f,.06f,tp);E(-sw+.005f,1.285f,0,.056f,.056f,.06f,tp);E(sw-.005f,1.285f,0,.056f,.056f,.06f,tp);
  float fc[]={.1f,1.28f,.17f,1.24f,.15f,1.18f};LA(v3(0,0,0),fc,3,tseg,RGBX(246,242,232),1,.8f);E(0,.96f,-.2f,.14f,.11f,.1f,bt);
  for(int k=0;k<5;k++)mb_tube(v3(-.03f,1.05f+k*.04f,.12f),v3(.03f,1.07f+k*.04f,.12f),.004f,.004f,3,tr);E(0,1.29f,.04f,.12f,.04f,.1f,RGBX(244,240,232));}
 else{float cf=t==PT_HERO?.2f:.185f;float bd[]={cf+.02f,.5f,cf,.7f,.16f,.93f,.148f,1.04f,.17f,1.12f,.19f,1.22f,.205f,1.29f,.15f,1.335f,.075f,1.358f,.05f,1.372f};LA(v3(0,0,0),bd,10,tseg,tp,1,.76f);E(-sw+.01f,1.285f,0,.06f,.06f,.066f,tp);E(sw-.01f,1.285f,0,.06f,.06f,.066f,tp);
  float cl[]={.058f,1.35f,.063f,1.395f,.056f,1.412f};LA(v3(0,0,0),cl,3,16,RGBX(246,244,238),1,1);
  E(0,1.3f,.115f,.045f,.06f,.03f,RGBX(246,244,238));for(int s=-1;s<=1;s+=2)mb_tube(v3(s*.06f,1.32f,.13f),v3(s*.11f,1.02f,.15f),.025f,.012f,5,gfx_lerp(tp,tr,.5f));
  for(int k=0;k<6;k++)EQ(.03f,1.24f-k*.055f,.155f,.011f,.011f,.006f,tr);
  if(t==PT_GUARD){mb_box(v3(0,1.12f,.145f),v3(.08f,.19f,.02f),RGBX(230,230,236));for(int k=0;k<6;k++)E((k&1?.05f:-.05f),1.27f-k*.05f,.16f,.011f,.011f,.01f,RGBX(236,206,100));mb_tube(v3(-.17f,1.28f,.1f),v3(.17f,.85f,.17f),.032f,.032f,6,RGBX(240,240,244));mb_tube(v3(.17f,1.28f,.1f),v3(-.17f,.85f,.17f),.032f,.032f,6,RGBX(240,240,244));
   mb_tube(v3(0,1.2f,-.2f),v3(.1f,.2f,-.26f),.025f,.025f,6,RGBX(86,60,44));E(.1f,1.2f,-.2f,.03f,.03f,.03f,RGBX(60,60,66));mb_tube(v3(-.18f,.95f,.14f),v3(-.3f,.5f,.12f),.018f,.016f,6,RGBX(200,190,150));E(-.3f,.5f,.12f,.03f,.03f,.02f,RGBX(236,206,100));
   E(-.2f,1.33f,0,.07f,.025f,.07f,RGBX(236,206,100));E(.2f,1.33f,0,.07f,.025f,.07f,RGBX(236,206,100));}
  if(t==PT_HERO){mb_box(v3(0,1.0f,0),v3(.205f,.04f,.125f),RGBX(150,30,36));E(0,1.0f,.128f,.04f,.03f,.012f,RGBX(236,206,100));mb_box(v3(.19f,.88f,.08f),v3(.05f,.06f,.04f),RGBX(110,70,44));
   mb_tube(v3(-.2f,1.3f,.1f),v3(.18f,.98f,.13f),.02f,.02f,5,RGBX(90,60,40));for(int k=0;k<4;k++)E(-.16f+k*.1f,1.26f-k*.08f,.135f,.016f,.03f,.012f,RGBX(80,60,40));}
  if(t==PT_MERCH){float ap[]={.2f,.7f,.19f,.9f,.17f,1.15f};LA(v3(0,0,0),ap,3,tseg,RGBX(236,230,214),1,.8f);}
  if(t==PT_COURIER){E(.22f,.92f,.0f,.1f,.14f,.16f,RGBX(130,90,56));mb_tube(v3(-.1f,1.3f,.05f),v3(.2f,.95f,.06f),.018f,.018f,6,RGBX(130,90,56));}}
 addp(m,PK_TORSO,v3(0,0,0),0);
 /* полы кафтана / плаща: две половины сзади и по бокам, шарнир на поясе */
 if(male){float len=t==PT_HERO?.62f:t==PT_GUARD?.5f:.42f;float tp2[]={.2f,0,.215f,-len*.35f,.24f,-len*.7f,.26f,-len};
  for(int s=0;s<2;s++){mb_begin();if(s==0)shell(v3(0,0,0),tp2,4,10,2.1f,3.12f,tp,1,.8f);else shell(v3(0,0,0),tp2,4,10,3.16f,4.18f,tp,1,.8f);
   float hem[]={.26f,-len,.262f,-len-.02f};if(s==0)shell(v3(0,0,0),hem,2,10,2.1f,3.12f,tr,1,.8f);else shell(v3(0,0,0),hem,2,10,3.16f,4.18f,tr,1,.8f);
   addp(m,s?PK_TAILR:PK_TAILL,v3(0,.98f,0),0);}
  if(t==PT_HERO){mb_begin();float fr[]={.2f,0,.205f,-.25f,.21f,-.5f};shell(v3(0,0,0),fr,3,8,.35f,1.2f,tp,1,.8f);shell(v3(0,0,0),fr,3,8,5.08f,5.93f,tp,1,.8f);addp(m,PK_TAIL,v3(0,.98f,0),0);}}
 if(woman){float hem=.06f,rr=.46f;mb_begin();float tt[7]={0,.05f,.12f,.28f,.5f,.75f,1.f},pr[14];for(int k=0;k<7;k++){pr[2*k]=skR(tt[k])*rr;pr[2*k+1]=hem+tt[k]*(1.02f-hem);}LA(v3(0,0,0),pr,7,tseg,bt,1,1);float fr[]={rr*1.05f,hem-.02f,rr*1.07f,hem+.03f,skR(.1f/(1.02f-hem))*rr+.004f,hem+.1f};LA(v3(0,0,0),fr,3,tseg,tr,1,1);
  float ap[]={.2f,1.0f,.25f,.7f,.3f,.35f};shell(v3(0,0,0),ap,3,8,-.6f,.6f,(lk.extra&1)?RGBX(244,240,230):gfx_lerp(bt,0xFFFFFF,.35f),1,1.05f);addp(m,PK_SKIRT,v3(0,1.0f,0),0);}
 for(int s=-1;s<=1;s+=2){mb_begin();uint32_t lc=woman?RGBX(240,236,238):(t==PT_GUARD?RGBX(236,236,240):bt);
  mb_tube(v3(0,0,0),v3(.005f,-.39f,.01f),male?.092f:.085f,male?.064f:.058f,ls,lc);E(.005f,-.39f,.01f,.062f,.062f,.062f,lc);addp(m,s<0?PK_THIGHL:PK_THIGHR,v3(s*.085f,.88f,0),0);
  mb_begin();mb_tube(v3(0,0,0),v3(-.005f,-.41f,-.01f),male?.06f:.056f,.04f,ls,t==PT_GUARD?RGBX(236,236,240):lc);E(-.005f,-.41f,-.01f,.042f,.042f,.042f,lc);E(0,-.1f,-.035f,.045f,.07f,.04f,lc);
  float bt2[]={.056f,-.49f,.062f,-.43f,.06f,-.31f,.054f,-.21f,.062f,-.15f,.066f,-.1f};LA(v3(0,0,0),bt2,(t==PT_HERO||t==PT_GUARD)?6:4,ls+2,sh,1,1.05f);E(0,-.455f,.042f,.05f,.04f,.11f,sh);E(0,-.47f,.09f,.046f,.03f,.06f,sh);mb_box(v3(0,-.478f,-.05f),v3(.04f,.014f,.03f),gfx_lerp(sh,0x000000,.4f));
  if(t!=PT_HERO&&t!=PT_GUARD&&male)EQ(0,-.46f,.07f,.03f,.012f,.025f,RGBX(214,190,120));
  addp(m,s<0?PK_SHINL:PK_SHINR,v3(s*.085f+.005f,.49f,.01f),0);}
 for(int s=-1;s<=1;s+=2){mb_begin();int puff=woman;uint32_t sl=tp,fa=(t==PT_HERO)?gfx_lerp(tp,0x000000,.2f):tp,hc=(t==PT_GUARD)?RGBX(236,232,226):(t==PT_HERO?RGBX(60,46,38):sk);
  if(puff){E(0,-.02f,0,.082f,.088f,.08f,sl);}mb_tube(v3(0,0,0),v3(0,-.29f,.015f),.056f,.042f,ls,sl);E(0,-.29f,.015f,.043f,.043f,.043f,sl);addp(m,s<0?PK_UARML:PK_UARMR,v3(s*sw,1.29f,0),0);
  mb_begin();mb_tube(v3(0,0,0),v3(0,-.26f,.05f),.042f,.03f,ls,fa);mb_tube(v3(0,-.2f,.04f),v3(0,-.265f,.052f),.045f,.04f,12,(t==PT_HERO||t==PT_GUARD)?tr:RGBX(250,248,250));E(0,-.266f,.052f,.029f,.02f,.029f,hc);hand((float)s,hc);
  if(s>0&&t==PT_GUARD){mb_tube(v3(0,-.31f,.085f),v3(0,.79f,.185f),.016f,.012f,6,RGBX(60,60,66));mb_tube(v3(0,.79f,.185f),v3(0,1.05f,.2f),.006f,.002f,4,RGBX(200,200,210));}
  addp(m,s<0?PK_FARML:PK_FARMR,v3(s*sw,1.0f,.015f),0);}
 if(t==PT_HERO){mb_begin();mb_tube(v3(0,0,0),v3(0,0,.7f),.012f,.004f,6,RGBX(214,218,226));mb_tube(v3(-.06f,0,-.01f),v3(.06f,0,-.01f),.01f,.01f,5,RGBX(200,170,80));mb_tube(v3(0,0,-.02f),v3(0,0,-.13f),.016f,.016f,6,RGBX(60,40,30));E(0,0,-.14f,.02f,.02f,.02f,RGBX(200,170,80));addp(m,PK_SWORD,v3(0,0,0),0);}
 build_head(m,lk,det);m->face=tex_new(det?256:128,det?256:128,0);m->face.mip=1;face_paint(m);face_store(m);}
static float DM_lay=0;
/* поза: ph — фаза шага, amp — темп; air/climb/crouch/hang — доли поз; roll — угол переката; bump — отшатывание; spin — разворот;
   atk — фаза удара (0..1), blk — блок, react — получен удар, sit — сидит, look — поворот головы, tail — подъём пол, sway — их качание, sword — оружие в руке, aim — прицел */
typedef struct{float ph,amp,lean,air,climb,crouch,roll,bump,spin,atk,blk,react,sit,look,tail,sway,hang,sword,aim;}Pose;
static Pose pose0(float ph,float amp){Pose q;memset(&q,0,sizeof q);q.ph=ph;q.amp=amp;return q;}
/* плавное смешивание поз (фаза — напрямую, кувырок/разворот/удар — без сглаживания) */
static void pose_blend(Pose*cur,const Pose*tgt,float k){cur->ph=tgt->ph;cur->amp+=(tgt->amp-cur->amp)*fminf(1.f,k*1.5f);cur->roll=tgt->roll;cur->spin=tgt->spin;cur->atk=tgt->atk;
#define PB(f) cur->f+=(tgt->f-cur->f)*k
 PB(lean);PB(air);PB(climb);PB(crouch);PB(bump);PB(blk);PB(react);PB(sit);PB(look);PB(tail);PB(sway);PB(hang);PB(sword);PB(aim);
#undef PB
}
static void model_free(Model*m){for(int i=0;i<m->n;i++){r3_mesh_free(&m->p[i].m);free(m->p[i].m.v);free(m->p[i].m.i);}r3_tex_free(&m->face);free(m->face.px);memset(m,0,sizeof *m);}
static float sw_of(const Model*m){return m->lk.type==PT_WOMAN?.165f:.2f;}
static void draw_model(const Model*m,float x,float y,float z,float yaw,const Pose*q,float t,int outline,float tint){
 float amp=q->amp,ph=q->ph,run=fminf(1.f,fmaxf(0.f,(amp-.45f)/.45f)),go=fminf(1.f,amp*3.f),ground=1.f-q->air;if(ground<0)ground=0;go*=fmaxf(0.f,1.f-q->hang)*fmaxf(0.f,1.f-q->sit);
 float bob=go>.01f?-fabsf(cosf(ph))*(.015f+.05f*run)*go*ground:sinf(t*1.8f)*.007f;
 float lean=.04f*go+.26f*run+q->crouch*.45f-q->bump*.45f+q->climb*.1f+q->air*.15f*run+q->atk*.2f-q->react*.3f+q->blk*.1f,twist=sinf(ph)*(.08f+.16f*run)*go*ground+(q->atk>0?sinf(q->atk*3.14159f)*.5f:0)-q->blk*.25f;
 float sity=q->sit*-.42f;
 Mat root=m_mul(m_trans(x,y+bob-q->crouch*.3f+sity,z),m_roty(yaw+q->spin*3.14159f));
 if(q->roll!=0)root=m_mul(root,m_mul(mtv(v3(0,.55f,0)),m_mul(m_rotx(q->roll),mtv(v3(0,-.55f,0)))));
 root=m_mul(root,m_rotx(DM_lay));
 Mat upper=m_mul(root,m_mul(mtv(v3(0,.95f,0)),m_mul(m_rotx(lean),m_mul(m_roty(twist),mtv(v3(0,-.95f,0))))));
 r3_tint[0]=r3_tint[1]=r3_tint[2]=tint;
 for(int i=0;i<m->n;i++){const MPart*p=&m->p[i];Mat M=root;
  int side=(p->kind==PK_THIGHL||p->kind==PK_SHINL||p->kind==PK_UARML||p->kind==PK_FARML)?-1:1;float pp=ph+(side>0?3.14159f:0),s1=sinf(pp),c1=cosf(pp);
  float A=(.32f+.5f*run)*go,th=-A*s1,fl=(.06f+(.38f+1.2f*run)*powf(fmaxf(0.f,c1),1.4f))*go+.05f;
  float ua=(.24f+.62f*run)*go*s1,fa=-(.2f+1.25f*run)*go-.35f*run*fmaxf(0.f,s1)-.1f,uz=-side*.07f;
  if(q->crouch>0){th=lf(th,-1.05f+th*.5f,q->crouch);fl=lf(fl,1.8f,q->crouch);ua=lf(ua,-.35f+ua*.5f,q->crouch);fa=lf(fa,-1.1f,q->crouch);}
  if(q->air>0){float a=fminf(1.f,q->air);th=lf(th,side<0?-1.0f:-.2f,a);fl=lf(fl,side<0?.55f:1.35f,a);ua=lf(ua,side<0?-2.0f:-1.4f,a);fa=lf(fa,-.55f,a);uz=lf(uz,-side*.35f,a);}
  if(q->climb>0){float a=q->climb,cs=sinf(q->ph+(side>0?3.14159f:0));th=lf(th,-.85f+.45f*cs,a);fl=lf(fl,1.15f+.5f*cs,a);ua=lf(ua,-2.75f+.45f*cs,a);fa=lf(fa,-.3f-.2f*cs,a);}
  if(q->hang>0){float a=fminf(1.f,q->hang),sw=sinf(t*2.f+side)*.08f;th=lf(th,-.15f+sw,a);fl=lf(fl,.25f,a);ua=lf(ua,-2.95f,a);fa=lf(fa,-.12f,a);uz=lf(uz,-side*.18f,a);}
  if(q->sit>0){th=lf(th,-1.55f,q->sit);fl=lf(fl,1.55f,q->sit);ua=lf(ua,.15f,q->sit);fa=lf(fa,-.9f,q->sit);}
  if(q->bump>0){ua=lf(ua,-1.2f,q->bump);fa=lf(fa,-1.0f,q->bump);}
  if(q->react>0&&side>0){ua=lf(ua,-.6f,q->react);fa=lf(fa,-.4f,q->react);}
  if(q->sword>0){if(side>0){float w=q->atk;float sa=w<.35f?lf(-.6f,-2.4f,w/.35f):lf(-2.4f,-.9f,fminf(1.f,(w-.35f)/.3f));ua=lf(ua,w>0?sa:-.65f,q->sword);fa=lf(fa,w>0?(w<.35f?-1.4f:-.15f):-.9f,q->sword);uz=lf(uz,w>0&&w<.6f?-.6f:-.25f,q->sword);
    if(q->blk>0){ua=lf(ua,-1.35f,q->blk);fa=lf(fa,-1.2f,q->blk);uz=lf(uz,.3f,q->blk);}}
   else{ua=lf(ua,-.35f,q->sword);fa=lf(fa,-.9f,q->sword);uz=lf(uz,.45f,q->sword);if(q->blk>0){ua=lf(ua,-1.1f,q->blk);fa=lf(fa,-1.6f,q->blk);}}}
  if(q->aim>0&&side>0){ua=lf(ua,-1.4f,q->aim);fa=lf(fa,-.2f,q->aim);}
  switch(p->kind){
  case PK_THIGHL:case PK_THIGHR:M=m_mul(root,m_mul(mtv(p->piv),m_mul(m_rotx(th),m_rotz(q->air*side*.12f))));break;
  case PK_SHINL:case PK_SHINR:{V3 hip=v3(p->piv.x-.005f,.88f,0);M=m_mul(root,m_mul(mtv(hip),m_mul(m_mul(m_rotx(th),m_rotz(q->air*side*.12f)),m_mul(mtv(v3(.005f,-.39f,.01f)),m_rotx(fl)))));break;}
  case PK_UARML:case PK_UARMR:M=m_mul(upper,m_mul(mtv(p->piv),m_mul(m_rotx(ua),m_rotz(uz))));break;
  case PK_FARML:case PK_FARMR:{V3 sh=v3(p->piv.x,1.29f,0);M=m_mul(upper,m_mul(mtv(sh),m_mul(m_rotx(ua),m_mul(m_rotz(uz),m_mul(mtv(v3(0,-.29f,.015f)),m_rotx(fa))))));break;}
  case PK_SWORD:{if(q->sword<.3f)continue;V3 sh=v3(sw_of(m),1.29f,0);M=m_mul(upper,m_mul(mtv(sh),m_mul(m_rotx(ua),m_mul(m_rotz(uz),m_mul(mtv(v3(0,-.29f,.015f)),m_mul(m_rotx(fa),m_mul(mtv(v3(0,-.32f,.07f)),m_rotx(-.25f))))))));break;}
  case PK_TORSO:M=upper;break;
  case PK_HEAD:M=m_mul(upper,m_mul(mtv(p->piv),m_mul(m_roty(q->look),m_mul(m_rotx(-lean*.75f),m_rotz(sinf(t*.7f)*.02f)))));break;
  case PK_TAIL:case PK_TAILL:case PK_TAILR:{float fl2=q->tail*1.15f+.05f*go,sw2=q->sway*(p->kind==PK_TAILL?1.f:p->kind==PK_TAILR?-1.f:0.f)+sinf(ph*2)*.06f*go;M=m_mul(upper,m_mul(mtv(p->piv),m_mul(m_rotx(-fl2-lean*.6f),m_rotz(sw2))));break;}
  case PK_SKIRT:M=m_mul(root,m_mul(mtv(p->piv),m_mul(m_rotx(sinf(ph)*.07f*go-q->tail*.15f),mtv(v3(0,-p->piv.y,0)))));break;default:break;}
  r3_draw(&p->m,M,p->usetex?&m->face:NULL,p->kind==PK_HEAD?R3_SKIN:(p->kind==PK_SWORD?R3_GLOSSY:R3_CLOTH),0);(void)outline;}
 r3_tint[0]=r3_tint[1]=r3_tint[2]=1;DM_lay=0;}
