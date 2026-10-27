/* ===== Мир: парижский квартал 1789 года. Старый квартал (x<200) — прежняя планировка и коллизии (гонки откалиброваны),
   восток (x 200..320) — Королевский сад с дворцом, мост, баржи, жилой квартал и площадь с колонной. ===== */
#include "atlas.c"
/* ---- коллизии ---- */
typedef struct{float x0,y0,z0,x1,y1,z1;int kind;}Box;
enum{BK_SOLID=0,BK_FIRM=1,BK_HAY=2,BK_GROUND=3};
#define MAXBOX 24000
static Box boxes[MAXBOX];static int nbox;static int box_pass;
#define WORLD 320.f
#define WORLDZ 200.f
#define GC 8.f
#define GW 42
static int cstart[GW*GW+2],*citems;
static int add_box(float x0,float y0,float z0,float x1,float y1,float z1,int kind){if(!box_pass||nbox>=MAXBOX)return -1;Box*b=&boxes[nbox];b->x0=x0;b->y0=y0;b->z0=z0;b->x1=x1;b->y1=y1;b->z1=z1;b->kind=kind;return nbox++;}
static void grid_build(void){memset(cstart,0,sizeof cstart);
 for(int pass=0;pass<2;pass++){static int fill[GW*GW];if(pass)memset(fill,0,sizeof fill);
  for(int i=0;i<nbox;i++){Box*b=&boxes[i];int cx0=(int)floorf(b->x0/GC),cx1=(int)floorf(b->x1/GC),cz0=(int)floorf(b->z0/GC),cz1=(int)floorf(b->z1/GC);
   if(cx0<0)cx0=0;if(cz0<0)cz0=0;if(cx1>=GW)cx1=GW-1;if(cz1>=GW)cz1=GW-1;
   for(int cz=cz0;cz<=cz1;cz++)for(int cx=cx0;cx<=cx1;cx++){int c=cz*GW+cx;if(!pass)cstart[c+1]++;else citems[cstart[c]+fill[c]++]=i;}}
  if(!pass){for(int c=0;c<GW*GW;c++)cstart[c+1]+=cstart[c];citems=realloc(citems,sizeof(int)*(cstart[GW*GW]+1));}}}
/* река: полоса z 84..112, вода на y=-3 */
static int in_river(float x,float z){(void)x;return z>84.f&&z<112.f;}
/* ---- генерация ---- */
typedef struct{float x0,z0,x1,z1;}Rect;
static Rect strects[96];static int nstr;
static float lampv[420][2];static int nlamp;static float treev[400][3];static int ntree;
static float haycarts[16][3];static int nhay;
typedef struct{float x0,z0,x1,z1,H,Hs;int face,floors;}Bd;static Bd blds[900];static int nbld,nbld_old;
static float chimv[1200][3];static int nchim;
static int cur_chunk;
static float clampf_w(float v){return v<0?0:v>1?1:v;}
typedef struct{float x,z,r,y;}Basin;static Basin basins[32];static int nbasin;
#define NCH 13
#define CHS 25.f
static int chunk_of(float x,float z){int cx=(int)(x/CHS),cz=(int)(z/CHS);if(cx<0)cx=0;if(cz<0)cz=0;if(cx>=NCH)cx=NCH-1;if(cz>=NCH)cz=NCH-1;return cz*NCH+cx;}
static int inchunk(float x,float z){return chunk_of(x,z)==cur_chunk;}
/* листва: кроны записываются при генерации квартала, меш строится отдельно (своя текстура) */
typedef struct{float x,y,z,s,cx,cy,cz;int q;}Fol;static Fol cfol[3000];static int ncfol;
static void fol_add(float x,float y,float z,float s,float cx,float cy,float cz,int q){if(ncfol>=3000)return;Fol*f=&cfol[ncfol++];f->x=x;f->y=y;f->z=z;f->s=s;f->cx=cx;f->cy=cy;f->cz=cz;f->q=q;}
/* ---- примитивы с правильной намоткой и мозаичной разверткой ---- */
static V3 vsub3(V3 a,V3 b){return v3(a.x-b.x,a.y-b.y,a.z-b.z);}
static V3 vcr3(V3 a,V3 b){return v3(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x);}
static float vdt3(V3 a,V3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static V3 vl3(V3 a,V3 b,float t){return v3(a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t);}
static float vlen3(V3 a){return sqrtf(vdt3(a,a));}
/* четырёхугольник a-b-c-d (низ-лево, низ-право, верх-право, верх-лево), лицом к out; режется на клетки ~cell м */
static void tquad(V3 a,V3 b,V3 c,V3 d,V3 out,int tile,uint32_t col,float cell){V3 n=vnrm(vcr3(vsub3(b,a),vsub3(d,a)));if(vdt3(n,out)<0){V3 t=a;a=b;b=t;t=c;c=d;d=t;n=v3(-n.x,-n.y,-n.z);}
 float lu=fmaxf(vlen3(vsub3(b,a)),vlen3(vsub3(c,d))),lv=fmaxf(vlen3(vsub3(d,a)),vlen3(vsub3(c,b)));int nu=cell>0?(int)ceilf(lu/cell-.05f):1,nv=cell>0?(int)ceilf(lv/cell-.05f):1;if(nu<1)nu=1;if(nv<1)nv=1;if(nu>40)nu=40;if(nv>40)nv=40;
 float u0,v0,u1,v1;tile_uv(tile,&u0,&v0,&u1,&v1);
 for(int j=0;j<nv;j++)for(int i=0;i<nu;i++){float s0=(float)i/nu,s1=(float)(i+1)/nu,t0=(float)j/nv,t1=(float)(j+1)/nv;
  V3 p00=vl3(vl3(a,b,s0),vl3(d,c,s0),t0),p10=vl3(vl3(a,b,s1),vl3(d,c,s1),t0),p11=vl3(vl3(a,b,s1),vl3(d,c,s1),t1),p01=vl3(vl3(a,b,s0),vl3(d,c,s0),t1);
  mb_quad(p00,p10,p11,p01,n,u0,v0,u1,v1,col);}}
static void bxt(float x0,float y0,float z0,float x1,float y1,float z1,uint32_t col,int tile,float cell){
 tquad(v3(x0,y0,z1),v3(x1,y0,z1),v3(x1,y1,z1),v3(x0,y1,z1),v3(0,0,1),tile,col,cell);tquad(v3(x1,y0,z0),v3(x0,y0,z0),v3(x0,y1,z0),v3(x1,y1,z0),v3(0,0,-1),tile,col,cell);
 tquad(v3(x0,y0,z0),v3(x0,y0,z1),v3(x0,y1,z1),v3(x0,y1,z0),v3(-1,0,0),tile,col,cell);tquad(v3(x1,y0,z1),v3(x1,y0,z0),v3(x1,y1,z0),v3(x1,y1,z1),v3(1,0,0),tile,col,cell);
 tquad(v3(x0,y1,z1),v3(x1,y1,z1),v3(x1,y1,z0),v3(x0,y1,z0),v3(0,1,0),tile,col,cell);if(y0>-.5f)tquad(v3(x0,y0,z0),v3(x1,y0,z0),v3(x1,y0,z1),v3(x0,y0,z1),v3(0,-1,0),tile,col,cell);}
static void tqd(float x0,float z0,float x1,float z1,float y,int tile,uint32_t col){float u0,v0,u1,v1;tile_uv(tile,&u0,&v0,&u1,&v1);mb_quad(v3(x0,y,z1),v3(x1,y,z1),v3(x1,y,z0),v3(x0,y,z0),v3(0,1,0),u0,v0,u1,v1,col);}
static void flat(float x0,float z0,float x1,float z1,float y,int tile,float cell,uint32_t col){
 for(float z=z0;z<z1-.01f;z+=cell){float zz=fminf(z+cell,z1);for(float x=x0;x<x1-.01f;x+=cell){float xx=fminf(x+cell,x1);if(!inchunk((x+xx)/2,(z+zz)/2))continue;int t=tile;if(tile==TL_COBBLE&&(hs((int)x,(int)z)&1))t=TL_COBBLE2;tqd(x,z,xx,zz,y,t,col);}}}
static void wallq(float x0,float z0,float x1,float z1,float y0,float y1,float nx,float nz,int tile,uint32_t col){float rx=nz,rz=-nx;V3 a=v3(x0,y0,z0),b=v3(x1,y0,z1);if((x1-x0)*rx+(z1-z0)*rz<0){a=v3(x1,y0,z1);b=v3(x0,y0,z0);}
 float u0,v0,u1,v1;tile_uv(tile,&u0,&v0,&u1,&v1);mb_quad(a,b,v3(b.x,y1,b.z),v3(a.x,y1,a.z),v3(nx,0,nz),u0,v0,u1,v1,col);}
static void bx(float x0,float y0,float z0,float x1,float y1,float z1,uint32_t col,int tile){bxt(x0,y0,z0,x1,y1,z1,col,tile,0);}
/* цилиндр с развёрткой (кора, колонны) */
static void cylt(V3 b,float r0,float r1,float h,int seg,int tile,uint32_t col,float vrep){float u0,v0,u1,v1;tile_uv(tile,&u0,&v0,&u1,&v1);int base=mb_count();int nv2=(int)ceilf(vrep);if(nv2<1)nv2=1;
 for(int j=0;j<=nv2;j++){float t=(float)j/nv2,r=lf(r0,r1,t),y=b.y+h*t;for(int s=0;s<=seg;s++){float th=6.2831853f*s/seg;float sl=(r0-r1)/h;float l=sqrtf(1+sl*sl);float uu=lf(u0,u1,fmodf((float)s/seg*2.f,1.0001f));float vv=(j&1)?v0:v1;
   mb_vert(b.x+r*sinf(th),y,b.z+r*cosf(th),sinf(th)/l,sl/l,cosf(th)/l,uu,vv,col);}}
 for(int j=0;j<nv2;j++)for(int s=0;s<seg;s++){int a=base+j*(seg+1)+s,c=a+seg+1;mb_index(a,a+1,c);mb_index(a+1,c+1,c);}}
/* ---- фасадные детали ---- */
/* брусок на фасаде: (p0..p1) вдоль стены, выступ dep наружу по (nx,nz) */
static void fbox(float p0x,float p0z,float p1x,float p1z,float y0,float y1,float nx,float nz,float dep,uint32_t col,int tile,float cell){
 float xa=fminf(p0x,p1x),xb=fmaxf(p0x,p1x),za=fminf(p0z,p1z),zb=fmaxf(p0z,p1z);if(nx>0)xb=xa+dep;else if(nx<0){xa=xb-dep;}if(nz>0)zb=za+dep;else if(nz<0){za=zb-dep;}
 if(nx!=0){xa=nx>0?p0x:p0x-dep;xb=nx>0?p0x+dep:p0x;}if(nz!=0){za=nz>0?p0z:p0z-dep;zb=nz>0?p0z+dep:p0z;}bxt(xa,y0,za,xb,y1,zb,col,tile,cell);}
static const uint32_t SHUTC[5]={RGBX(70,104,80),RGBX(92,104,118),RGBX(120,86,62),RGBX(66,82,104),RGBX(160,150,128)};
static void window_trim(float ax,float az,float bx2,float bz2,float y0,float y1,float nx,float nz,int kk,int lit,uint32_t seed,int street){
 float L=hypotf(bx2-ax,bz2-az);float u0=kk==2?16/64.f:18/64.f,u1=kk==2?48/64.f:46/64.f,vt=kk==2?2/64.f:8/64.f,vb=kk==2?62/64.f:54/64.f;
 float H=y1-y0,ytop=y1-vt*H,ybot=y1-vb*H;float pa_x=lf(ax,bx2,u0),pa_z=lf(az,bz2,u0),pb_x=lf(ax,bx2,u1),pb_z=lf(az,bz2,u1);
 float ex=(bx2-ax)/L,ez=(bz2-az)/L;float m=.06f;uint32_t stc=RGBX(236,230,216);
 if(kk!=2)fbox(pa_x-ex*m,pa_z-ez*m,pb_x+ex*m,pb_z+ez*m,ybot-.1f,ybot,nx,nz,.16f,stc,TL_BASE,0);
 fbox(pa_x-ex*m,pa_z-ez*m,pb_x+ex*m,pb_z+ez*m,ytop+.03f,ytop+.13f,nx,nz,.09f,stc,TL_BASE,0);
 if(street&&kk==0){float w=(u1-u0)*L*.5f;uint32_t sc=SHUTC[seed%5];
  fbox(pa_x-ex*(w+.03f),pa_z-ez*(w+.03f),pa_x-ex*.03f,pa_z-ez*.03f,ybot+.02f,ytop-.02f,nx,nz,.05f,sc,TL_SHUTTER,0);
  fbox(pb_x+ex*.03f,pb_z+ez*.03f,pb_x+ex*(w+.03f),pb_z+ez*(w+.03f),ybot+.02f,ytop-.02f,nx,nz,.05f,sc,TL_SHUTTER,0);}
 if(street&&kk!=2&&(seed>>5)%4==0){/* ящик с цветами */float cxm=(pa_x+pb_x)/2+nx*.22f,czm=(pa_z+pb_z)/2+nz*.22f;float hx=fabsf(ez)*(u1-u0)*L*.45f+fabsf(nx)*.1f,hz=fabsf(ex)*(u1-u0)*L*.45f+fabsf(nz)*.1f;
  bxt(cxm-hx,ybot,czm-hz,cxm+hx,ybot+.18f,czm+hz,RGBX(110,74,52),TL_WOOD,0);
  for(int q=0;q<6;q++){uint32_t h=hs(seed+q,91);float t=(q+.5f)/6.f-.5f;float px=cxm+ex*t*2*(hx>hz?hx:hz),pz=czm+ez*t*2*(hx>hz?hx:hz);uint32_t c=(h&3)==0?RGBX(200,40,60):(h&3)==1?RGBX(240,200,60):(h&3)==2?RGBX(220,110,170):RGBX(60,120,50);
   mb_sphere(v3(px,ybot+.24f,pz),v3(.09f,.08f,.09f),6,4,c,0);mb_sphere(v3(px+ez*.05f,ybot+.2f,pz+ex*.05f),v3(.07f,.06f,.07f),5,3,RGBX(60,110,48),0);}}
 (void)lit;}
static void awning(float x0,float z0,float x1,float z1,float nx,float nz){float rx=nz,rz=-nx,ax=x0,az=z0,bx2=x1,bz2=z1;if((x1-x0)*rx+(z1-z0)*rz<0){ax=x1;az=z1;bx2=x0;bz2=z0;}
 V3 a=v3(ax+nx*1.1f,2.45f,az+nz*1.1f),b=v3(bx2+nx*1.1f,2.45f,bz2+nz*1.1f),c=v3(bx2+nx*.03f,3.35f,bz2+nz*.03f),d=v3(ax+nx*.03f,3.35f,az+nz*.03f);V3 o=vnrm(v3(nx*.6f,.8f,nz*.6f));
 tquad(a,b,c,d,o,TL_AWNING,0xFFFFFF,1.2f);tquad(b,a,d,c,v3(-o.x,-o.y,-o.z),TL_AWNING,RGBX(180,170,160),1.2f);
 /* фестоны */for(float t=0;t<1.f;t+=.125f){V3 p=vl3(a,b,t),q=vl3(a,b,t+.125f);V3 m2=vl3(p,q,.5f);m2.y-=.16f;mb_tri(p,m2,q,RGBX(176,44,52));mb_tri(q,m2,p,RGBX(176,44,52));}
 for(int s=0;s<2;s++){V3 p=s?b:a;mb_tube(p,v3(p.x-nx*1.05f,p.y+.95f,p.z-nz*1.05f),.015f,.015f,4,RGBX(40,40,44));}}
static void balcony(float px,float pz,float nx,float nz,float w,float y){float d=.9f,hx=fabsf(nz)*w/2+fabsf(nx)*d/2,hz=fabsf(nx)*w/2+fabsf(nz)*d/2,cx=px+nx*d/2,cz=pz+nz*d/2;
 add_box(cx-hx,y-.14f,cz-hz,cx+hx,y,cz+hz,BK_SOLID);bxt(cx-hx,y-.14f,cz-hz,cx+hx,y,cz+hz,0xFFFFFF,TL_BASE,1.2f);uint32_t ir=RGBX(30,30,34);
 /* консоли */for(int s=-1;s<=1;s+=2){float qx=px+fabsf(nz)*s*w*.35f,qz=pz+fabsf(nx)*s*w*.35f;mb_tube(v3(qx,y-.5f,qz),v3(qx+nx*.75f,y-.15f,qz+nz*.75f),.04f,.03f,5,RGBX(200,194,180));}
 /* кованая решётка: поручень, прутья, завитки */
 float fx=cx+nx*(d/2-.03f),fz=cz+nz*(d/2-.03f);V3 L0=v3(fx-fabsf(nz)*hx,0,fz-fabsf(nx)*hz),L1=v3(fx+fabsf(nz)*hx,0,fz+fabsf(nx)*hz);
 mb_tube(v3(L0.x,y+.95f,L0.z),v3(L1.x,y+.95f,L1.z),.025f,.025f,5,ir);mb_tube(v3(L0.x,y+.08f,L0.z),v3(L1.x,y+.08f,L1.z),.015f,.015f,4,ir);
 int nb=(int)(w/.12f);for(int j=0;j<=nb;j++){V3 p=vl3(L0,L1,(float)j/nb);mb_tube(v3(p.x,y,p.z),v3(p.x,y+.95f,p.z),.009f,.009f,3,ir);}
 for(int j=0;j<(int)(w/.45f);j++){V3 p=vl3(L0,L1,(j+.5f)/(int)(w/.45f));for(int k=0;k<6;k++){float a0=k*1.05f,a1=a0+1.05f;float r=.11f;mb_tube(v3(p.x+fabsf(nz)*cosf(a0)*r,y+.5f+sinf(a0)*r,p.z+fabsf(nx)*cosf(a0)*r),v3(p.x+fabsf(nz)*cosf(a1)*r,y+.5f+sinf(a1)*r,p.z+fabsf(nx)*cosf(a1)*r),.008f,.008f,3,ir);}}
 for(int s=-1;s<=1;s+=2){V3 e0=s<0?L0:L1;V3 e1=v3(e0.x-nx*d,0,e0.z-nz*d);mb_tube(v3(e0.x,y+.95f,e0.z),v3(e1.x,y+.95f,e1.z),.02f,.02f,4,ir);for(int j=0;j<=5;j++){V3 p=vl3(e0,e1,j/5.f);mb_tube(v3(p.x,y,p.z),v3(p.x,y+.95f,p.z),.009f,.009f,3,ir);}}}
static void sign_bracket(float px,float pz,float nx,float nz,int s){float y=3.5f;V3 a=v3(px,y,pz),b=v3(px+nx*1.1f,y,pz+nz*1.1f);mb_tube(a,b,.025f,.025f,5,RGBX(30,30,34));mb_tube(v3(px,y-.5f,pz),v3(px+nx*.7f,y,pz+nz*.7f),.018f,.018f,4,RGBX(30,30,34));
 float cx=px+nx*.75f,cz=pz+nz*.75f;float ex=fabsf(nx)*.32f,ez=fabsf(nz)*.32f;V3 p0=v3(cx-ex,y-.75f,cz-ez),p1=v3(cx+ex,y-.75f,cz+ez),p2=v3(cx+ex,y-.08f,cz+ez),p3=v3(cx-ex,y-.08f,cz-ez);
 tquad(p0,p1,p2,p3,v3(-nz,0,nx),TL_SIGN0+s,0xFFFFFF,0);tquad(p1,p0,p3,p2,v3(nz,0,-nx),TL_SIGN0+s,0xFFFFFF,0);
 for(int k=0;k<2;k++){V3 q=k?p3:p2;q=v3(k?cx-ex*.8f:cx+ex*.8f,y,k?cz-ez*.8f:cz+ez*.8f);mb_tube(q,v3(q.x,y-.08f,q.z),.006f,.006f,3,RGBX(40,40,44));}}
static void building(float x0,float z0,float x1,float z1,int face,uint32_t seed,int floors,int mat){
 float FH=3.3f,GH=4.4f,H=GH+(floors-1)*FH,Hs=H+3.0f,ins=1.0f,cx=(x0+x1)/2,cz=(z0+z1)/2;
 if(box_pass&&nbld<900){Bd*b=&blds[nbld++];b->x0=x0;b->z0=z0;b->x1=x1;b->z1=z1;b->H=H;b->Hs=Hs;b->face=face;b->floors=floors;}
 add_box(x0,0,z0,x1,H,z1,BK_SOLID);add_box(x0+ins,H,z0+ins,x1-ins,Hs,z1-ins,BK_SOLID);
 float chx[3],chz[3];int nch=2+(seed&1);for(int k=0;k<nch;k++){chx[k]=lf(x0+ins+.9f,x1-ins-.9f,rf(hs(seed,k*3+1)));chz[k]=lf(z0+ins+.9f,z1-ins-.9f,rf(hs(seed,k*3+2)));add_box(chx[k]-.45f,Hs,chz[k]-.45f,chx[k]+.45f,Hs+1.9f,chz[k]+.45f,BK_SOLID);
  if(box_pass&&nchim<1200&&(hs(seed,k)%3==0)){chimv[nchim][0]=chx[k];chimv[nchim][1]=Hs+2.4f;chimv[nchim][2]=chz[k];nchim++;}}
 if(!inchunk(cx,cz))return;uint32_t W=0xFFFFFF;
 for(int side=0;side<4;side++){
  float nx=side==1?1:side==3?-1:0,nz=side==0?-1:side==2?1:0;float ax=side==1?x1:x0,az=side==0?z0:side==2?z1:z0,bx2=side==3?x0:x1,bz2=side==0?z0:side==2?z1:z1;
  if(side==1){ax=x1;az=z0;bx2=x1;bz2=z1;}if(side==3){ax=x0;az=z0;bx2=x0;bz2=z1;}
  int street=(side==face),back=(side==(face+2)%4);float len=(side&1)?(z1-z0):(x1-x0);int n=(int)(len/3.3f);if(n<1)n=1;
  /* ориентируем отрезок стены слева направо, как его видит прохожий */
  float qax=ax,qaz=az,qbx=bx2,qbz=bz2;{float rx=nz,rz=-nx;if((bx2-ax)*rx+(bz2-az)*rz<0){qax=bx2;qaz=bz2;qbx=ax;qbz=az;}}
  for(int f=0;f<floors;f++){float y0=f==0?0:GH+(f-1)*FH,y1=f==0?GH:y0+FH;
   for(int i=0;i<n;i++){float t0=(float)i/n,t1=(float)(i+1)/n;float p0x=lf(ax,bx2,t0),p0z=lf(az,bz2,t0),p1x=lf(ax,bx2,t1),p1z=lf(az,bz2,t1);uint32_t h=hs(seed*7+i+side*131,f);int lit=(hs(seed+i*5+side,f*11+3)%3==0),tile;int kk=-1;
    if(street||back){if(f==0){if(street){int st=(hs(seed,i+side*17)%6);tile=48+st*2+lit;if(st==0||st==3)awning(p0x,p0z,p1x,p1z,nx,nz);
       if(i==0&&(hs(seed,77)&1))sign_bracket(lf(p0x,p1x,.08f)+nx*.02f,lf(p0z,p1z,.08f)+nz*.02f,nx,nz,(int)(hs(seed,78)%6));}else tile=((mat*4+3)*2);}
     else{kk=(f==1&&(h%3)==0)?2:(int)((h>>4)%2);tile=(mat*4+kk)*2+lit;if(kk==2&&street)balcony((p0x+p1x)/2,(p0z+p1z)/2,nx,nz,(len/n)*.78f,y0);}}
    else tile=((mat*4+3)*2);
    wallq(p0x,p0z,p1x,p1z,y0,y1,nx,nz,tile,W);
    if(kk>=0){float tl0=t0,tl1=t1;float sax=lf(qax,qbx,0),saz=lf(qaz,qbz,0);(void)sax;(void)saz;
     /* привязка «слева-направо» как в текстуре */float rx=nz,rz=-nx;float lx0=p0x,lz0=p0z,lx1=p1x,lz1=p1z;if((p1x-p0x)*rx+(p1z-p0z)*rz<0){lx0=p1x;lz0=p1z;lx1=p0x;lz1=p0z;}(void)tl0;(void)tl1;
     window_trim(lx0+nx*.005f,lz0+nz*.005f,lx1+nx*.005f,lz1+nz*.005f,y0,y1,nx,nz,kk,lit,h,street);}}}
  /* цоколь, пояс, угловые русты */
  if(street||back){fbox(qax,qaz,qbx,qbz,0,.55f,nx,nz,.06f,W,TL_BASE,1.6f);
   fbox(qax-(qbx-qax)*0.f,qaz,qbx,qbz,GH-.08f,GH+.18f,nx,nz,.18f,W,TL_BASE,1.6f);}
  if(street){float L=hypotf(qbx-qax,qbz-qaz),ex=(qbx-qax)/L,ez=(qbz-qaz)/L;for(int e=0;e<2;e++){float px=e?qbx-ex*.42f:qax,pz=e?qbz-ez*.42f:qaz;fbox(px,pz,px+ex*.42f,pz+ez*.42f,.55f,H-.35f,nx,nz,.05f,RGBX(236,232,222),TL_BASE,1.f);}
   if(hs(seed,5)%2){float px=qax+ex*.14f,pz=qaz+ez*.14f;mb_tube(v3(px+nx*.1f,0,pz+nz*.1f),v3(px+nx*.1f,H-.3f,pz+nz*.1f),.055f,.055f,6,RGBX(70,74,80));mb_tube(v3(px+nx*.1f,H-.3f,pz+nz*.1f),v3(px+nx*.38f,H-.15f,pz+nz*.38f),.05f,.05f,6,RGBX(70,74,80));}}
  /* карниз под мансардой: профилированный, верх ровно на высоте H (по нему ходят) */
  {float L=hypotf(qbx-qax,qbz-qaz),ex=(qbx-qax)/L,ez=(qbz-qaz)/L;float o=.42f;fbox(qax-ex*o,qaz-ez*o,qbx+ex*o,qbz+ez*o,H-.16f,H,nx,nz,.42f,W,TL_BASE,1.6f);fbox(qax-ex*.25f,qaz-ez*.25f,qbx+ex*.25f,qbz+ez*.25f,H-.42f,H-.16f,nx,nz,.24f,W,TL_BASE,1.6f);}
  /* мансарда: наклонная кровля из шифера */
  {float o=.55f;V3 A,B,C,D;
   if(side==0){A=v3(x0+o,H,z0+o);B=v3(x1-o,H,z0+o);C=v3(x1-ins,Hs,z0+ins);D=v3(x0+ins,Hs,z0+ins);}else if(side==2){A=v3(x1-o,H,z1-o);B=v3(x0+o,H,z1-o);C=v3(x0+ins,Hs,z1-ins);D=v3(x1-ins,Hs,z1-ins);}
   else if(side==1){A=v3(x1-o,H,z0+o);B=v3(x1-o,H,z1-o);C=v3(x1-ins,Hs,z1-ins);D=v3(x1-ins,Hs,z0+ins);}else{A=v3(x0+o,H,z1-o);B=v3(x0+o,H,z0+o);C=v3(x0+ins,Hs,z0+ins);D=v3(x0+ins,Hs,z1-ins);}
   V3 out=vnrm(v3(nx*3.f,(ins-o),nz*3.f));int n2=(int)(((side&1)?(z1-z0-2*ins):(x1-x0-2*ins))/3.3f);if(n2<1)n2=1;
   for(int i=0;i<n2;i++){float t0=(float)i/n2,t1=(float)(i+1)/n2;tquad(vl3(A,B,t0),vl3(A,B,t1),vl3(D,C,t1),vl3(D,C,t0),out,TL_SLATE,W,1.6f);}
   /* люкарны: коллизия как раньше, сверху — двускатная крыша и окно */
   float sax,saz,sbx,sbz;if(side==0||side==2){sax=x0+ins;sbx=x1-ins;saz=sbz=side==0?z0+ins:z1-ins;}else{saz=z0+ins;sbz=z1-ins;sax=sbx=side==1?x1-ins:x0+ins;}
   for(int i=0;i<n2;i++){float t0=(float)i/n2,t1=(float)(i+1)/n2;
    if(street&&(i&1)==0){float dx=lf(sax,sbx,(t0+t1)/2),dz=lf(saz,sbz,(t0+t1)/2);float hw=.8f,dd=.95f;float x0d=dx-fabsf(nz)*hw,x1d=dx+fabsf(nz)*hw,z0d=dz-fabsf(nx)*hw,z1d=dz+fabsf(nx)*hw;
     float fx0=x0d,fx1=x1d,fz0=z0d,fz1=z1d;if(nz!=0){fz0=dz+(nz>0?0:-dd);fz1=dz+(nz>0?dd:0);}else{fx0=dx+(nx>0?0:-dd);fx1=dx+(nx>0?dd:0);}
     add_box(fx0,H,fz0,fx1,H+1.9f,fz1,BK_SOLID);bxt(fx0,H,fz0,fx1,H+1.9f,fz1,RGBX(236,230,216),TL_BASE,1.f);
     /* крыша люкарны */{float ry=H+1.9f,rr=.55f;V3 r0,r1,r2,r3,p0,p1;if(nz!=0){float zf=nz>0?fz1+.08f:fz0-.08f,zb=nz>0?fz0:fz1;r0=v3(fx0-.08f,ry,zf);r1=v3(fx1+.08f,ry,zf);r2=v3(fx1+.08f,ry,zb);r3=v3(fx0-.08f,ry,zb);p0=v3(dx,ry+rr,zf);p1=v3(dx,ry+rr,zb);
       tquad(r0,p0,p1,r3,v3(-1,1,0),TL_SLATE,W,0);tquad(p0,r1,r2,p1,v3(1,1,0),TL_SLATE,W,0);mb_tri(r0,r1,p0,RGBX(236,230,216));mb_tri(r1,r0,p0,RGBX(236,230,216));}
      else{float xf=nx>0?fx1+.08f:fx0-.08f,xb=nx>0?fx0:fx1;r0=v3(xf,ry,fz0-.08f);r1=v3(xf,ry,fz1+.08f);r2=v3(xb,ry,fz1+.08f);r3=v3(xb,ry,fz0-.08f);p0=v3(xf,ry+rr,dz);p1=v3(xb,ry+rr,dz);
       tquad(r0,p0,p1,r3,v3(0,1,-1),TL_SLATE,W,0);tquad(p0,r1,r2,p1,v3(0,1,1),TL_SLATE,W,0);mb_tri(r0,r1,p0,RGBX(236,230,216));mb_tri(r1,r0,p0,RGBX(236,230,216));}}
     float gx=dx+nx*(dd+.012f),gz=dz+nz*(dd+.012f);float lx=fabsf(nz)*.42f,lz=fabsf(nx)*.42f;int wl=(hs(seed,i)&1);
     {V3 a=v3(gx-lx,H+.45f,gz-lz),b=v3(gx+lx,H+.45f,gz+lz),c=v3(gx+lx,H+1.6f,gz+lz),d=v3(gx-lx,H+1.6f,gz-lz);int tl=(mat*4+0)*2+wl;float u0,v0,u1,v1;tile_uv(tl,&u0,&v0,&u1,&v1);
      float uu0=lf(u0,u1,.24f),uu1=lf(u0,u1,.76f),vv0=lf(v0,v1,.1f),vv1=lf(v0,v1,.88f);V3 nn=v3(nx,0,nz);if(vdt3(vnrm(vcr3(vsub3(b,a),vsub3(d,a))),nn)<0){V3 t=a;a=b;b=t;t=c;c=d;d=t;}mb_quad(a,b,c,d,nn,uu0,vv0,uu1,vv1,W);}}}}
 }
 /* крыша и парапет */
 flat(x0+ins,z0+ins,x1-ins,z1-ins,Hs,TL_ZINC,4.f,W);
 bxt(x0+ins,Hs,z0+ins,x1-ins,Hs+.45f,z0+ins+.25f,W,TL_BASE,1.6f);bxt(x0+ins,Hs,z1-ins-.25f,x1-ins,Hs+.45f,z1-ins,W,TL_BASE,1.6f);bxt(x0+ins,Hs,z0+ins,x0+ins+.25f,Hs+.45f,z1-ins,W,TL_BASE,1.6f);bxt(x1-ins-.25f,Hs,z0+ins,x1-ins,Hs+.45f,z1-ins,W,TL_BASE,1.6f);
 for(int k=0;k<nch;k++){bxt(chx[k]-.45f,Hs,chz[k]-.45f,chx[k]+.45f,Hs+1.9f,chz[k]+.45f,W,TL_BRICK,1.f);bxt(chx[k]-.55f,Hs+1.9f,chz[k]-.55f,chx[k]+.55f,Hs+2.05f,chz[k]+.55f,W,TL_BASE,0);
  for(int q=0;q<3;q++){float ox2=(q-1)*.28f;mb_cyl(v3(chx[k]+ox2,Hs+2.05f,chz[k]),.1f,.08f,.38f+(q&1)*.08f,8,RGBX(150,74,52));}}}
static void row(float a0,float a1,float c0,float c1,int face,uint32_t seed,int alongx){float p=a0;int k=0;while(p<a1-.5f){float w=7.f+rf(hs(seed,k++))*4.5f;if(p+w>a1-5.5f)w=a1-p;if(w<4){break;}
  uint32_t s2=hs(seed,k*13+7);int fl=3+(int)(s2%4);int mat=(int)((s2>>5)%6);
  if(alongx)building(p,c0,p+w,c1,face,s2,fl,mat);else building(c0,p,c1,p+w,face,s2,fl,mat);p+=w;}}
static void fill_block(float x0,float z0,float x1,float z1,uint32_t seed){float dW=x1-x0,dH=z1-z0,d=fminf(10.f,fminf(dW,dH)/2);
 row(x0,x1,z0,z0+d,0,seed+1,1);row(x0,x1,z1-d,z1,2,seed+2,1);if(dH>2*d+2){row(z0+d,z1-d,x1-d,x1,1,seed+3,0);row(z0+d,z1-d,x0,x0+d,3,seed+4,0);}}
/* ---- крупные постройки ---- */
static void stone_walls(float x0,float z0,float x1,float z1,float y0,float y1,int twall,int twin,float bayw){
 for(int side=0;side<4;side++){float nx=side==1?1:side==3?-1:0,nz=side==0?-1:side==2?1:0;float ax=side==1?x1:x0,az=side==0?z0:side==2?z1:z0,bx2=side==3?x0:x1,bz2=side==0?z0:side==2?z1:z1;if(side==1){ax=x1;az=z0;bx2=x1;bz2=z1;}if(side==3){ax=x0;az=z0;bx2=x0;bz2=z1;}
  float len=(side&1)?(z1-z0):(x1-x0);int n=(int)(len/bayw);if(n<1)n=1;float hh=y1-y0;int fl=(int)(hh/6.f);if(fl<1)fl=1;
  for(int f=0;f<fl;f++)for(int i=0;i<n;i++){float t0=(float)i/n,t1=(float)(i+1)/n;wallq(lf(ax,bx2,t0),lf(az,bz2,t0),lf(ax,bx2,t1),lf(az,bz2,t1),y0+f*hh/fl,y0+(f+1)*hh/fl,nx,nz,(twin&&((i+f)&1)==0&&f>0)?(hs(i,f)%3==0?TL_GOTHL:TL_GOTH):twall,0xFFFFFF);}
  /* контрфорсы-лопатки по углам и ярусные карнизы */
  for(int f=1;f<fl;f++){float yy=y0+f*hh/fl;float L=len,ex=(bx2-ax)/L,ez=(bz2-az)/L;fbox(ax-ex*.2f,az-ez*.2f,bx2+ex*.2f,bz2+ez*.2f,yy-.12f,yy+.1f,nx,nz,.22f,0xFFFFFF,TL_CATH,2.f);}}}
static void gable(float x0,float z0,float x1,float z1,float y,float rise){float zm=(z0+z1)/2;
 tquad(v3(x0-.3f,y,z1+.3f),v3(x1+.3f,y,z1+.3f),v3(x1+.3f,y+rise,zm),v3(x0-.3f,y+rise,zm),v3(0,1,1),TL_LEAD,RGBX(150,160,170),2.f);
 tquad(v3(x1+.3f,y,z0-.3f),v3(x0-.3f,y,z0-.3f),v3(x0-.3f,y+rise,zm),v3(x1+.3f,y+rise,zm),v3(0,1,-1),TL_LEAD,RGBX(150,160,170),2.f);
 mb_tri(v3(x0,y,z0),v3(x0,y,z1),v3(x0,y+rise,zm),RGBX(200,194,180));mb_tri(v3(x1,y,z1),v3(x1,y,z0),v3(x1,y+rise,zm),RGBX(200,194,180));
 mb_tube(v3(x0-.3f,y+rise+.05f,zm),v3(x1+.3f,y+rise+.05f,zm),.08f,.08f,6,RGBX(110,116,124));}
static void pinnacle(float x,float y,float z,float h){float sp[]={.35f,0,.32f,h*.4f,.12f,h*.85f,.02f,h};mb_lathe(v3(x,y,z),sp,4,8,RGBX(196,190,176),1,1);mb_sphere(v3(x,y+h,z),v3(.08f,.08f,.08f),6,4,RGBX(196,190,176),0);}
static void cathedral(void){/* остров: собор с двумя башнями, вид на запад */
 add_box(72,-3,86,128,0,110,BK_GROUND);add_box(80,0,93,120,20,103,BK_SOLID);add_box(95,0,88,105,20,108,BK_SOLID);add_box(80,0,89,88,46,97,BK_SOLID);add_box(80,0,99,88,46,107,BK_SOLID);
 for(int k=0;k<6;k++){float bx0=84+k*6.5f;add_box(bx0,0,90.f,bx0+1.2f,14,93,BK_SOLID);add_box(bx0,0,103,bx0+1.2f,14,106,BK_SOLID);}
 add_box(120,0,94,125,12,102,BK_SOLID);
 if(!inchunk(100,98))return;
 stone_walls(80,93,120,103,0,20,TL_CATH,1,4.f);stone_walls(95,88,105,108,0,20,TL_CATH,1,4.f);stone_walls(80,89,88,97,0,46,TL_CATH,1,4.f);stone_walls(80,99,88,107,0,46,TL_CATH,1,4.f);stone_walls(120,94,125,102,0,12,TL_CATH,1,4.f);
 wallq(80,95,80,101,8,20,-1,0,TL_ROSE,0xFFFFFF);gable(80,93,120,103,20,6);gable(95,88,105,108,20,6);
 for(int k=0;k<6;k++){float bx0=84+k*6.5f;bxt(bx0,0,90,bx0+1.2f,14,93,0xFFFFFF,TL_CATH,2.f);bxt(bx0,0,103,bx0+1.2f,14,106,0xFFFFFF,TL_CATH,2.f);
  tquad(v3(bx0,14,93),v3(bx0+1.2f,14,93),v3(bx0+1.2f,17,95),v3(bx0,17,95),v3(0,1,-1),TL_CATH,0xFFFFFF,0);tquad(v3(bx0+1.2f,14,103),v3(bx0,14,103),v3(bx0,17,101),v3(bx0+1.2f,17,101),v3(0,1,1),TL_CATH,0xFFFFFF,0);
  pinnacle(bx0+.6f,14,91.5f,3.2f);pinnacle(bx0+.6f,14,104.5f,3.2f);}
 for(int t=0;t<2;t++){float zz=t?99:89;for(int c=0;c<8;c++){bxt(80+c*1.0f,46,zz+(c%2)*7.2f,80.8f+c*1.0f,47.2f,zz+.8f+(c%2)*7.2f,0xFFFFFF,TL_CATH,0);}bxt(80,46,zz,88,46.6f,zz+.5f,0xFFFFFF,TL_CATH,2);bxt(80,46,zz+7.5f,88,46.6f,zz+8,0xFFFFFF,TL_CATH,2);bxt(80,46,zz,80.5f,46.6f,zz+8,0xFFFFFF,TL_CATH,2);bxt(87.5f,46,zz,88,46.6f,zz+8,0xFFFFFF,TL_CATH,2);
  for(int c=0;c<4;c++)pinnacle(c&1?87.6f:80.4f,46.6f,c&2?zz+7.6f:zz+.4f,2.5f);}
 {float sp[]={3.4f,20,3.0f,26,1.6f,34,.05f,44};mb_lathe(v3(100,0,98),sp,4,16,RGBX(110,124,130),1,1);}
 /* портал */for(int k=0;k<3;k++){float zc=96.2f+k*1.8f;for(int a=0;a<8;a++){float a0=a*3.14159f/8,a1=(a+1)*3.14159f/8;mb_tube(v3(79.9f,4+sinf(a0)*.8f,zc+cosf(a0)*.8f),v3(79.9f,4+sinf(a1)*.8f,zc+cosf(a1)*.8f),.12f,.12f,5,RGBX(200,194,180));}
  bxt(79.85f,0,zc-.7f,80,4,zc+.7f,RGBX(60,46,40),TL_DOOR,0);}}
static void church(void){add_box(28,0,40,50,13,52,BK_SOLID);add_box(30,0,30,37,36,38,BK_SOLID);if(!inchunk(40,40))return;
 stone_walls(28,40,50,52,0,13,TL_CATH,1,4.f);stone_walls(30,30,37,38,0,36,TL_CATH,1,3.5f);gable(28,40,50,52,13,5);
 {float sp[]={1.2f,36,1.0f,40,.05f,47};mb_lathe(v3(36,0,37),sp,3,12,RGBX(80,92,104),1,1);}wallq(30,38,37,38,28,34,0,1,TL_ROSE,0xFFFFFF);pinnacle(30.3f,36,30.3f,2.5f);pinnacle(36.7f,36,30.3f,2.5f);pinnacle(30.3f,36,37.7f,2.5f);}
static void clocktower(float x0,float z0,float x1,float z1,float H){add_box(x0,0,z0,x1,H,z1,BK_SOLID);if(!inchunk((x0+x1)/2,(z0+z1)/2))return;stone_walls(x0,z0,x1,z1,0,H,TL_CATH,1,3.4f);
 for(int s=0;s<4;s++){float nx=s==1?1:s==3?-1:0,nz=s==0?-1:s==2?1:0;float cx=(x0+x1)/2+nx*(x1-x0)/2,cz=(z0+z1)/2+nz*(z1-z0)/2;if(s==0||s==2)wallq(cx-1.6f,cz+nz*.02f,cx+1.6f,cz+nz*.02f,H-6,H-2.8f,nx,nz,TL_ROSE,0xFFFFFF);else wallq(cx+nx*.02f,cz-1.6f,cx+nx*.02f,cz+1.6f,H-6,H-2.8f,nx,nz,TL_ROSE,0xFFFFFF);}
 {float sp[]={1.2f,H,1.0f,H+2.5f,.05f,H+8};mb_lathe(v3(x1-1.4f,0,z1-1.4f),sp,3,10,RGBX(80,92,104),1,1);}bxt(x0-.3f,H,z0-.3f,x1+.3f,H+.5f,z1+.3f,0xFFFFFF,TL_BASE,1.6f);}
static const float xs_[5][2]={{16,24},{56,64},{96,104},{136,144},{176,184}},zs_[6][2]={{16,24},{56,64},{78,84},{112,118},{136,144},{176,184}};
static const float xe_[3][2]={{200,208},{240,248},{280,288}};
/* сад: гравийные дорожки и газоны */
static int park_tile(float x,float z){float dx=x-260,dz=z-48,r=sqrtf(dx*dx+dz*dz);if(r<13.f)return TL_GRAVEL;if(fabsf(dz)<4||fabsf(dx)<4)return TL_GRAVEL;if(z<24||z>=72||x<214||x>=306)return TL_GRAVEL;if((z>=28&&z<33)||(z>=63&&z<68))return TL_GRAVEL;return TL_GRASS;}
static int ground_tile(float x,float z){
 if(z>=84&&z<112){if(x>=72&&x<128&&z>=86&&z<110)return TL_PLAZA;if((x>=56&&x<64)||(x>=136&&x<144)||(x>=248&&x<256))return TL_COBBLE;if(((x>=74&&x<82)||(x>=118&&x<126))&&(z<86||z>=110))return TL_COBBLE;return -1;}
 if(x>=200){
  if(z<84){if(z>=78)return z<80?TL_SIDEWALK:TL_COBBLE;if(x<208){float e=fminf(x-200,208-x);return e<2.f?TL_SIDEWALK:TL_COBBLE;}if(z<18)return TL_PLAZA;if(z<20)return TL_GRAVEL;if(x>=316)return TL_GRAVEL;return park_tile(x,z);}
  int inv=-1,inh=-1;for(int k=0;k<3;k++)if(x>=xe_[k][0]&&x<xe_[k][1])inv=k;for(int k=3;k<6;k++)if(z>=zs_[k][0]&&z<zs_[k][1])inh=k;
  if(inv>=0||inh>=0){int sw=0;if(inv>=0&&inh<0){float e=fminf(x-xe_[inv][0],xe_[inv][1]-x);sw=e<2.f;}else if(inh>=0&&inv<0){float e=fminf(z-zs_[inh][0],zs_[inh][1]-z);sw=(e<2.f)||inh==3;}return sw?TL_SIDEWALK:TL_COBBLE;}
  if(x>=248&&x<280&&z>=144&&z<176)return TL_PLAZA;if(x>=316)return TL_COBBLE;return TL_DIRT;}
 int inv=-1,inh=-1;for(int k=0;k<5;k++)if(x>=xs_[k][0]&&x<xs_[k][1])inv=k;for(int k=0;k<6;k++)if(z>=zs_[k][0]&&z<zs_[k][1])inh=k;
 if(inv>=0||inh>=0){int sw=0;if(inv>=0&&inh<0){float e=fminf(x-xs_[inv][0],xs_[inv][1]-x);sw=e<2.f;}else if(inh>=0&&inv<0){float e=fminf(z-zs_[inh][0],zs_[inh][1]-z);sw=(e<2.f)||inh==2||inh==3;}return sw?TL_SIDEWALK:TL_COBBLE;}
 if(x>=64&&x<96&&z>=24&&z<56)return TL_PLAZA;if(x>=24&&x<56&&z>=144&&z<176)return TL_PLAZA;if(x>=144&&x<176&&z>=144&&z<176)return TL_GRASS;return TL_DIRT;}
/* ---- реквизит ---- */
static void tree_visual(float x,float z,float trunk_h,float crown,int climb){uint32_t h=hs((int)(x*7),(int)(z*13));
 cylt(v3(x,0,z),.32f,.2f,trunk_h+.6f,10,TL_BARK,RGBX(255,255,255),3);
 /* корни */for(int k=0;k<4;k++){float a=k*1.57f+rf(h>>k)*.6f;mb_tube(v3(x,.25f,z),v3(x+sinf(a)*.65f,0,z+cosf(a)*.65f),.16f,.06f,6,RGBX(84,64,48));}
 int nb=climb?4:5;for(int k=0;k<nb;k++){float a=climb?k*1.5708f:(k*1.256f+rf(h>>(k+3))*.8f);float y0=trunk_h-.2f+(climb?0:k*.25f);float L=climb?2.5f:1.6f+rf(h>>(k+5))*.8f;
  V3 e=v3(x+sinf(a)*L,y0+(climb?.05f:.9f+rf(h>>k)*.6f),z+cosf(a)*L);mb_tube(v3(x,y0,z),e,climb?.17f:.12f,climb?.09f:.05f,6,RGBX(88,68,50));
  V3 e2=v3(e.x+sinf(a+.6f)*1.1f,e.y+1.2f,e.z+cosf(a+.6f)*1.1f);mb_tube(e,e2,.06f,.03f,5,RGBX(88,68,50));}
 int nc=climb?9:7;float cy=trunk_h+crown*.75f+(climb?1.6f:.4f);
 for(int k=0;k<nc;k++){uint32_t q=hs((int)(x*31)+k,(int)(z*17));float a=rf(q)*6.283f,r=rf(q>>8)*crown*.6f,yy=cy+(rf(q>>16)-.4f)*crown*.8f;fol_add(x+sinf(a)*r,yy,z+cosf(a)*r,crown*(.75f+rf(q>>4)*.4f),x,cy,z,(q>>20)%7==0?2:(int)((q>>21)&1));}
 fol_add(x,cy+crown*.5f,z,crown*.9f,x,cy,z,0);}
static void prop_tree(float x,float z){if(box_pass&&ntree<400){treev[ntree][0]=x;treev[ntree][1]=0;treev[ntree][2]=z;ntree++;}add_box(x-.25f,0,z-.25f,x+.25f,5.2f,z+.25f,BK_FIRM);if(!inchunk(x,z))return;tree_visual(x,z,3.0f,2.4f,0);}
/* дерево в саду: ствол-лазалка, четыре ветви-площадки на высоте 4.5 м — по кронам можно бежать */
static void prop_ptree(float x,float z){if(box_pass&&ntree<400){treev[ntree][0]=x;treev[ntree][1]=1;treev[ntree][2]=z;ntree++;}float th=4.5f;
 add_box(x-.3f,0,z-.3f,x+.3f,th,z+.3f,BK_SOLID);add_box(x-2.6f,th-.3f,z-.22f,x+2.6f,th,z+.22f,BK_SOLID);add_box(x-.22f,th-.3f,z-2.6f,x+.22f,th,z+2.6f,BK_SOLID);
 if(!inchunk(x,z))return;tree_visual(x,z,th,3.0f,1);}
static void prop_lamp(float x,float z){if(box_pass&&nlamp<420){lampv[nlamp][0]=x;lampv[nlamp][1]=z;nlamp++;}add_box(x-.12f,0,z-.12f,x+.12f,4.5f,z+.12f,BK_FIRM);if(!inchunk(x,z))return;
 float pr[]={.2f,0,.2f,.15f,.13f,.25f,.13f,.55f,.07f,.75f,.055f,3.9f,.08f,4.0f,.06f,4.25f,.1f,4.32f};mb_lathe(v3(x,0,z),pr,9,10,RGBX(34,36,42),1,1);
 /* фонарь: стеклянный шестигранник с колпаком */float lp[]={.06f,4.32f,.17f,4.42f,.21f,4.95f,.24f,5.0f,.1f,5.18f,.02f,5.3f};
 int b0=mb_count();mb_lathe(v3(x,0,z),lp,2,6,RGBX(34,36,42),1,1);(void)b0;
 {float u0,v0,u1,v1;tile_uv(TL_LANTERN,&u0,&v0,&u1,&v1);float uc=(u0+u1)*.5f,vc=(v0+v1)*.5f;int base=mb_count();for(int s=0;s<=6;s++){float a=s*1.0472f;for(int k=0;k<2;k++){float r=k?.21f:.17f,y=k?4.95f:4.42f;mb_vert(x+sinf(a)*r,y,z+cosf(a)*r,sinf(a),0,cosf(a),uc,vc,RGBX(255,236,190));}}
  for(int s=0;s<6;s++){int a=base+s*2;mb_index(a,a+2,a+1);mb_index(a+2,a+3,a+1);}}
 {float cp[]={.24f,4.95f,.26f,5.0f,.14f,5.12f,.03f,5.28f};mb_lathe(v3(x,0,z),cp,4,6,RGBX(34,36,42),1,1);}
 mb_tube(v3(x,4.2f,z),v3(x+.35f,4.05f,z),.025f,.02f,4,RGBX(34,36,42));}
static void prop_stall(float x,float z){add_box(x-1.2f,0,z-.7f,x+1.2f,1.0f,z+.7f,BK_SOLID);if(!inchunk(x,z))return;bxt(x-1.2f,0,z-.7f,x+1.2f,1.0f,z+.7f,0xFFFFFF,TL_WOOD,1.2f);bxt(x-1.25f,1.0f,z-.75f,x+1.25f,1.06f,z+.75f,0xFFFFFF,TL_PLANKS,1.2f);
 for(int s=-1;s<=1;s+=2)for(int t=-1;t<=1;t+=2)mb_tube(v3(x+s*1.2f,0,z+t*.7f),v3(x+s*1.2f,2.4f+(t<0?.3f:0),z+t*.7f),.04f,.04f,6,RGBX(90,64,44));
 tquad(v3(x-1.45f,2.1f,z+1.15f),v3(x+1.45f,2.1f,z+1.15f),v3(x+1.45f,2.75f,z-.85f),v3(x-1.45f,2.75f,z-.85f),v3(0,1,.3f),TL_AWNING,0xFFFFFF,1.2f);
 tquad(v3(x+1.45f,2.1f,z+1.15f),v3(x-1.45f,2.1f,z+1.15f),v3(x-1.45f,2.75f,z-.85f),v3(x+1.45f,2.75f,z-.85f),v3(0,-1,-.3f),TL_AWNING,RGBX(170,160,150),1.2f);
 uint32_t h=hs((int)x,(int)z);for(int k=0;k<3;k++){float bxx=x-.8f+k*.8f;bxt(bxx-.33f,1.06f,z-.45f,bxx+.33f,1.2f,z+.45f,RGBX(150,110,80),TL_WOOD,0);
  uint32_t fc[4]={RGBX(200,50,40),RGBX(240,190,60),RGBX(110,150,60),RGBX(230,120,40)};uint32_t c=fc[(h>>(k*3))&3];for(int q=0;q<9;q++){float px=bxx-.22f+(q%3)*.22f,pz=z-.3f+(q/3)*.3f;mb_sphere(v3(px,1.27f,pz),v3(.09f,.075f,.09f),6,4,c,0);}}
 for(int k=0;k<2;k++)bxt(x+1.4f,0,z-.5f+k*.6f,x+1.9f,.45f,z-.05f+k*.6f,RGBX(170,130,90),TL_WOOD,0);}
static void prop_crates(float x,float z,int n){add_box(x-.5f,0,z-.5f,x+.5f,.9f*n,z+.5f,BK_SOLID);if(!inchunk(x,z))return;for(int k=0;k<n;k++){float o=(hs((int)x+k,(int)z)%5)*.03f;bxt(x-.5f+o,k*.9f,z-.5f,x+.5f+o,(k+1)*.9f,z+.5f,0xFFFFFF,TL_PLANKS,0);}}
static void prop_barrel(float x,float z){add_box(x-.4f,0,z-.4f,x+.4f,.95f,z+.4f,BK_SOLID);if(!inchunk(x,z))return;float pr[]={.33f,0,.39f,.25f,.41f,.47f,.39f,.7f,.33f,.95f};mb_lathe(v3(x,0,z),pr,5,14,RGBX(120,84,56),1,1);
 mb_cyl(v3(x,.18f,z),.385f,.395f,.06f,14,RGBX(56,56,62));mb_cyl(v3(x,.72f,z),.395f,.385f,.06f,14,RGBX(56,56,62));mb_cyl(v3(x,.93f,z),.32f,.32f,.025f,14,RGBX(100,72,50));}
static void prop_hay(float x,float z){float sx=1.5f,sz=1.0f;if(box_pass&&nhay<16){haycarts[nhay][0]=x;haycarts[nhay][1]=0;haycarts[nhay][2]=z;nhay++;}add_box(x-sx,0,z-sz,x+sx,1.4f,z+sz,BK_HAY);if(!inchunk(x,z))return;
 bxt(x-sx,.35f,z-sz,x+sx,.85f,z+sz,0xFFFFFF,TL_PLANKS,1.f);bxt(x-sx+.1f,.85f,z-sz+.1f,x+sx-.1f,1.4f,z+sz-.1f,0xFFFFFF,TL_HAY,1.f);
 {int b=mb_count();float u0,v0,u1,v1;tile_uv(TL_HAY,&u0,&v0,&u1,&v1);mb_sphere(v3(x,1.4f,z),v3(sx-.05f,.45f,sz-.05f),14,8,0xFFFFFF,1);mb_remap_uv(b,u0,v0,u1,v1);}
 for(int s=-1;s<=1;s+=2){mb_tube(v3(x+s*(sx-.3f),.45f,z-sz-.05f),v3(x+s*(sx-.3f),.45f,z+sz+.05f),.05f,.05f,6,RGBX(70,52,40));
  for(int e=-1;e<=1;e+=2){float zz=z+e*(sz+.11f);mb_tube(v3(x+s*(sx-.3f),.45f,zz-.04f*e),v3(x+s*(sx-.3f),.45f,zz),.47f,.47f,16,RGBX(98,70,48));for(int k=0;k<6;k++){float a=k*1.047f;mb_tube(v3(x+s*(sx-.3f),.45f,zz),v3(x+s*(sx-.3f)+sinf(a)*.42f,.45f+cosf(a)*.42f,zz),.025f,.025f,4,RGBX(80,58,40));}}}
 mb_tube(v3(x-sx,.6f,z),v3(x-sx-1.4f,.4f,z),.04f,.04f,5,RGBX(90,64,44));}
static void prop_bench(float x,float z,int alongx){float hx=alongx?.8f:.25f,hz=alongx?.25f:.8f;add_box(x-hx,0,z-hz,x+hx,.5f,z+hz,BK_SOLID);if(!inchunk(x,z))return;
 for(int k=0;k<3;k++){float o=-hz+k*hz*.66f+.05f;if(alongx)bxt(x-hx,.42f,z+o-.06f,x+hx,.48f,z+o+.04f,0xFFFFFF,TL_PLANKS,0);else bxt(x+o-.06f,.42f,z-hz,x+o+.04f,.48f,z+hz,0xFFFFFF,TL_PLANKS,0);}
 if(alongx){bxt(x-hx,.55f,z+hz-.06f,x+hx,.85f,z+hz,0xFFFFFF,TL_PLANKS,0);}else bxt(x+hx-.06f,.55f,z-hz,x+hx,.85f,z+hz,0xFFFFFF,TL_PLANKS,0);
 for(int s=-1;s<=1;s+=2){float px=alongx?x+s*(hx-.08f):x,pz=alongx?z:z+s*(hz-.08f);bxt(px-.04f,0,pz-.04f,px+.04f,.85f,pz+.04f,RGBX(36,36,40),TL_IRON,0);}}
static void prop_fountain(float x,float z){add_box(x-2.6f,0,z-2.6f,x+2.6f,1.1f,z+2.6f,BK_SOLID);if(box_pass&&nbasin<32){basins[nbasin++]=(Basin){x,z,2.5f,.96f};}if(!inchunk(x,z))return;float b1[]={2.7f,0,2.8f,.15f,2.75f,.9f,2.85f,1.0f,2.5f,1.1f,2.4f,.95f};mb_lathe(v3(x,0,z),b1,6,32,RGBX(206,200,188),1,1);
 mb_cyl(v3(x,0,z),.5f,.3f,1.8f,14,RGBX(200,194,182));float t1[]={1.2f,1.7f,1.3f,1.8f,1.25f,1.9f,.2f,2.0f};mb_lathe(v3(x,0,z),t1,4,22,RGBX(206,200,188),1,1);mb_cyl(v3(x,2.0f,z),.14f,.08f,1.3f,10,RGBX(200,194,182));
 float st[]={.0f,3.3f,.22f,3.35f,.3f,3.55f,.18f,3.8f,.0f,3.85f};mb_lathe(v3(x,0,z),st,5,12,RGBX(214,200,150),1,1);}
/* статуя на постаменте */
static void statue(float x,float z,float yaw){add_box(x-.8f,0,z-.8f,x+.8f,1.6f,z+.8f,BK_SOLID);if(!inchunk(x,z))return;bxt(x-.8f,0,z-.8f,x+.8f,1.6f,z+.8f,0xFFFFFF,TL_MARBLE,0);bxt(x-.95f,1.5f,z-.95f,x+.95f,1.7f,z+.95f,0xFFFFFF,TL_MARBLE,0);bxt(x-.95f,0,z-.95f,x+.95f,.25f,z+.95f,0xFFFFFF,TL_MARBLE,0);
 int b=mb_count();uint32_t mc=RGBX(232,230,224);float body[]={.24f,0,.27f,.3f,.2f,.75f,.24f,1.05f,.2f,1.2f,.08f,1.28f};mb_lathe(v3(0,0,0),body,6,12,mc,1,.8f);mb_sphere(v3(0,1.42f,.0f),v3(.12f,.14f,.13f),10,8,mc,0);
 mb_tube(v3(.22f,1.15f,0),v3(.42f,1.45f,.15f),.06f,.045f,6,mc);mb_tube(v3(-.22f,1.15f,0),v3(-.3f,.75f,.1f),.06f,.045f,6,mc);mb_tube(v3(.42f,1.45f,.15f),v3(.45f,1.85f,.2f),.04f,.03f,6,mc);
 mb_xform(b,m_mul(m_trans(x,1.7f,z),m_roty(yaw)));}
static void hedge(float x0,float z0,float x1,float z1,float h){add_box(x0,0,z0,x1,h,z1,BK_SOLID);if(!inchunk((x0+x1)/2,(z0+z1)/2))return;bxt(x0,0,z0,x1,h,z1,0xFFFFFF,TL_HEDGE,1.5f);}
static void fence(float x0,float z0,float x1,float z1){/* кованая ограда (ходить можно поверху) */float L=hypotf(x1-x0,z1-z0);int n=(int)(L/.16f);
 add_box(fminf(x0,x1)-.06f,0,fminf(z0,z1)-.06f,fmaxf(x0,x1)+.06f,1.9f,fmaxf(z0,z1)+.06f,BK_SOLID);if(!inchunk((x0+x1)/2,(z0+z1)/2))return;uint32_t ir=RGBX(28,28,32);
 bxt(fminf(x0,x1)-.15f,0,fminf(z0,z1)-.15f,fmaxf(x0,x1)+.15f,.4f,fmaxf(z0,z1)+.15f,0xFFFFFF,TL_BASE,1.6f);
 mb_tube(v3(x0,1.75f,z0),v3(x1,1.75f,z1),.025f,.025f,4,ir);mb_tube(v3(x0,.55f,z0),v3(x1,.55f,z1),.025f,.025f,4,ir);
 for(int i=0;i<=n;i++){float t=(float)i/n;float x=lf(x0,x1,t),z=lf(z0,z1,t);mb_tube(v3(x,.4f,z),v3(x,1.85f,z),.012f,.012f,3,ir);if(i%2==0){mb_tri(v3(x-.03f,1.85f,z),v3(x+.03f,1.85f,z),v3(x,1.98f,z),RGBX(200,170,80));mb_tri(v3(x+.03f,1.85f,z),v3(x-.03f,1.85f,z),v3(x,1.98f,z),RGBX(200,170,80));}}
 int np=(int)(L/3.f);for(int i=0;i<=np;i++){float t=(float)i/np;float x=lf(x0,x1,t),z=lf(z0,z1,t);bxt(x-.15f,0,z-.15f,x+.15f,2.1f,z+.15f,0xFFFFFF,TL_BASE,0);mb_sphere(v3(x,2.2f,z),v3(.13f,.13f,.13f),8,5,RGBX(210,204,190),0);}}
/* баржа у набережной: палуба — твёрдая, на неё можно прыгнуть */
static void barge(float x,float z,float len,float w,float yaw_dummy){(void)yaw_dummy;float y=-1.9f;add_box(x-len/2,-3.2f,z-w/2,x+len/2,y,z+w/2,BK_SOLID);add_box(x-len/2+1,y,z-w/2+.3f,x-len/2+3.5f,y+1.4f,z+w/2-.3f,BK_SOLID);
 if(!inchunk(x,z))return;int b=mb_count();(void)b;
 bxt(x-len/2,-3.2f,z-w/2,x+len/2,y,z+w/2,0xFFFFFF,TL_BOAT,1.5f);bxt(x-len/2+1,y,z-w/2+.3f,x-len/2+3.5f,y+1.4f,z+w/2-.3f,RGBX(200,190,170),TL_PLANKS,1.f);
 bxt(x-len/2+.9f,y+1.4f,z-w/2+.2f,x-len/2+3.6f,y+1.5f,z+w/2-.2f,0xFFFFFF,TL_ROOFRED,1.f);
 mb_tube(v3(x+1,y,z),v3(x+1,y+7.5f,z),.1f,.07f,8,RGBX(110,80,56));mb_tube(v3(x-3,y+1.2f,z),v3(x+5,y+1.6f,z),.05f,.05f,6,RGBX(110,80,56));
 for(int k=0;k<5;k++){float px=x+2.5f+k*1.1f,pz=z+((k&1)?.5f:-.5f);bxt(px-.4f,y,pz-.4f,px+.4f,y+.8f,pz+.4f,0xFFFFFF,TL_PLANKS,0);}
 for(int k=0;k<3;k++)prop_barrel(x-1.5f+k*.9f,z+w/2-.7f);}
static void palace(void){/* дворец: x 228..292, z 2..16, центральный павильон с куполом (точка обзора) */
 float x0=228,x1=292,z0=2,z1=16,H=13.2f,Hs=H+3.f,ins=1.f;add_box(x0,0,z0,x1,H,z1,BK_SOLID);add_box(x0+ins,H,z0+ins,x1-ins,Hs,z1-ins,BK_SOLID);
 add_box(252,0,0,268,H+3,18,BK_SOLID);add_box(254.5f,H+3,2.5f,265.5f,H+7,15.5f,BK_SOLID);add_box(257.5f,H+7,5.5f,262.5f,H+12.5f,12.5f,BK_SOLID);
 if(box_pass&&nbld<900){Bd*b=&blds[nbld++];b->x0=x0;b->z0=z0;b->x1=x1;b->z1=z1;b->H=H;b->Hs=Hs;b->face=2;b->floors=3;}
 if(!inchunk(240,10)&&!inchunk(260,10)&&!inchunk(280,10))return;
 if(inchunk(260,10)){for(int side=0;side<4;side++){float nx=side==1?1:side==3?-1:0,nz=side==0?-1:side==2?1:0;float ax=side==1?x1:x0,az=side==0?z0:z1,bx2=side==3?x0:x1,bz2=side==0?z0:z1;if(side==1){ax=x1;az=z0;bx2=x1;bz2=z1;}if(side==3){ax=x0;az=z0;bx2=x0;bz2=z1;}
   float len=(side&1)?(z1-z0):(x1-x0);int n=(int)(len/4.f);for(int f=0;f<3;f++)for(int i=0;i<n;i++){float t0=(float)i/n,t1=(float)(i+1)/n;wallq(lf(ax,bx2,t0),lf(az,bz2,t0),lf(ax,bx2,t1),lf(az,bz2,t1),f*4.4f,(f+1)*4.4f,nx,nz,f==0?TL_BASE:(hs(i,f+side)%4==0?TL_PALWL:TL_PALW),0xFFFFFF);}
   float L=len,ex=(bx2-ax)/L,ez=(bz2-az)/L;fbox(ax-ex*.4f,az-ez*.4f,bx2+ex*.4f,bz2+ez*.4f,H-.2f,H,nx,nz,.45f,0xFFFFFF,TL_BASE,1.6f);fbox(ax,az,bx2,bz2,4.3f,4.6f,nx,nz,.2f,0xFFFFFF,TL_BASE,1.6f);
   float o=.55f;V3 A,B,C,D;if(side==0){A=v3(x0+o,H,z0+o);B=v3(x1-o,H,z0+o);C=v3(x1-ins,Hs,z0+ins);D=v3(x0+ins,Hs,z0+ins);}else if(side==2){A=v3(x1-o,H,z1-o);B=v3(x0+o,H,z1-o);C=v3(x0+ins,Hs,z1-ins);D=v3(x1-ins,Hs,z1-ins);}
   else if(side==1){A=v3(x1-o,H,z0+o);B=v3(x1-o,H,z1-o);C=v3(x1-ins,Hs,z1-ins);D=v3(x1-ins,Hs,z0+ins);}else{A=v3(x0+o,H,z1-o);B=v3(x0+o,H,z0+o);C=v3(x0+ins,Hs,z0+ins);D=v3(x0+ins,Hs,z1-ins);}
   int n2=(int)(len/3.3f);for(int i=0;i<n2;i++){float t0=(float)i/n2,t1=(float)(i+1)/n2;tquad(vl3(A,B,t0),vl3(A,B,t1),vl3(D,C,t1),vl3(D,C,t0),vnrm(v3(nx*3,.45f,nz*3)),TL_SLATE,RGBX(200,210,220),1.6f);}}
  flat(x0+ins,z0+ins,x1-ins,z1-ins,Hs,TL_LEAD,4.f,0xFFFFFF);
  /* центральный павильон: колоннада, фронтон, купол */
  stone_walls(252,0,268,18,0,H+3,TL_CATH,0,4.f);bxt(251.6f,H+2.6f,-.4f,268.4f,H+3.2f,18.4f,0xFFFFFF,TL_BASE,1.6f);
  for(int k=0;k<6;k++){float cxx=253.5f+k*2.6f;cylt(v3(cxx,0,19.2f),.45f,.4f,H+2.2f,12,TL_COLUMN,0xFFFFFF,4);bxt(cxx-.6f,H+2.2f,18.6f,cxx+.6f,H+2.6f,19.8f,0xFFFFFF,TL_BASE,0);bxt(cxx-.6f,0,18.6f,cxx+.6f,.5f,19.8f,0xFFFFFF,TL_BASE,0);}
  bxt(252,H+2.6f,18,268,H+3.4f,20.2f,0xFFFFFF,TL_BASE,1.6f);mb_tri(v3(252,H+3.4f,20.2f),v3(268,H+3.4f,20.2f),v3(260,H+6.4f,20.2f),RGBX(236,230,216));tquad(v3(252,H+3.4f,20.2f),v3(260,H+6.4f,20.2f),v3(260,H+6.4f,18),v3(252,H+3.4f,18),v3(-1,2,0),TL_LEAD,0xFFFFFF,2);tquad(v3(260,H+6.4f,20.2f),v3(268,H+3.4f,20.2f),v3(268,H+3.4f,18),v3(260,H+6.4f,18),v3(1,2,0),TL_LEAD,0xFFFFFF,2);
  stone_walls(254.5f,2.5f,265.5f,15.5f,H+3,H+7,TL_PALW,0,3.6f);
  {float dm[]={5.6f,H+7,5.5f,H+8.5f,5.0f,H+10.f,4.0f,H+11.5f,2.6f,H+12.5f,1.4f,H+13.0f,.3f,H+13.2f};mb_lathe(v3(260,0,9),dm,7,24,RGBX(120,136,140),1,1);}
  bxt(257.5f,H+12.4f,5.5f,262.5f,H+12.5f,12.5f,0xFFFFFF,TL_LEAD,0);{float ln[]={.9f,H+12.5f,.9f,H+14.f,1.1f,H+14.2f,.1f,H+15.2f};mb_lathe(v3(260,0,9),ln,4,10,RGBX(200,190,150),1,1);}
  for(int k=0;k<2;k++)for(int j=0;j<2;j++)pinnacle(k?291.f:229.f,H+.1f,j?15.f:3.f,1.8f);}}
static void column_monument(float x,float z){/* площадь с колонной: высокая точка обзора */
 add_box(x-3,0,z-3,x+3,3,z+3,BK_SOLID);add_box(x-1.25f,3,z-1.25f,x+1.25f,30.5f,z+1.25f,BK_SOLID);add_box(x-1.8f,30.5f,z-1.8f,x+1.8f,31.1f,z+1.8f,BK_SOLID);
 if(!inchunk(x,z))return;bxt(x-3,0,z-3,x+3,3,z+3,0xFFFFFF,TL_MARBLE,1.5f);bxt(x-3.3f,0,z-3.3f,x+3.3f,.5f,z+3.3f,0xFFFFFF,TL_BASE,1.6f);bxt(x-3.2f,2.8f,z-3.2f,x+3.2f,3.2f,z+3.2f,0xFFFFFF,TL_MARBLE,0);
 cylt(v3(x,3.2f,z),1.3f,1.15f,27.3f,20,TL_COLUMN,RGBX(220,214,200),12);
 /* спиральный барельеф */for(int k=0;k<120;k++){float a0=k*.35f,a1=a0+.35f,y0=4+k*.21f,y1=y0+.21f;float r=1.32f-k*.0012f;mb_tube(v3(x+sinf(a0)*r,y0,z+cosf(a0)*r),v3(x+sinf(a1)*r,y1,z+cosf(a1)*r),.07f,.07f,4,RGBX(200,190,160));}
 bxt(x-1.8f,30.5f,z-1.8f,x+1.8f,31.1f,z+1.8f,0xFFFFFF,TL_MARBLE,0);mb_cyl(v3(x,31.1f,z),.7f,.5f,1.2f,12,RGBX(200,170,90));mb_sphere(v3(x,32.6f,z),v3(.5f,.5f,.5f),12,8,RGBX(220,190,100),0);
 for(int k=0;k<4;k++){float a=k*1.5708f+.785f;mb_tube(v3(x+sinf(a)*1.7f,31.1f,z+cosf(a)*1.7f),v3(x+sinf(a)*1.7f,32.f,z+cosf(a)*1.7f),.03f,.03f,4,RGBX(28,28,32));}
 for(int k=0;k<4;k++){float a=k*1.5708f;float lx=x+sinf(a)*4.6f,lz=z+cosf(a)*4.6f;prop_lamp(lx,lz);}}
/* ---- земля, набережные, мосты ---- */
static void gen_streets_and_ground(void){
 int cx0=(cur_chunk%NCH)*(int)CHS,cz0=(cur_chunk/NCH)*(int)CHS;
 for(int zz=cz0;zz<cz0+(int)CHS&&zz<(int)WORLDZ;zz+=2)for(int xx=cx0;xx<cx0+(int)CHS&&xx<(int)WORLD;xx+=2){int t=ground_tile(xx+1.f,zz+1.f);if(t<0)continue;if(t==TL_COBBLE&&(hs(xx,zz)&1))t=TL_COBBLE2;tqd((float)xx,(float)zz,(float)xx+2,(float)zz+2,0,t,0xFFFFFF);}
 /* набережные: каменная кладка с потёками */
 for(float x=0;x<WORLD;x+=4){if(inchunk(x+2,84)){wallq(x,84,x+4,84,-3,0,0,1,TL_QUAY,0xFFFFFF);bxt(x,-.05f,83.6f,x+4,.12f,84.25f,0xFFFFFF,TL_BASE,0);}if(inchunk(x+2,112)){wallq(x,112,x+4,112,-3,0,0,-1,TL_QUAY,0xFFFFFF);bxt(x,-.05f,111.75f,x+4,.12f,112.4f,0xFFFFFF,TL_BASE,0);}}
 for(float z=86;z<110;z+=4){if(inchunk(72,z+2)){wallq(72,z,72,z+4,-3,0,-1,0,TL_QUAY,0xFFFFFF);}if(inchunk(128,z+2)){wallq(128,z,128,z+4,-3,0,1,0,TL_QUAY,0xFFFFFF);}}
 for(float x=72;x<128;x+=4){if(inchunk(x+2,86))wallq(x,86,x+4,86,-3,0,0,-1,TL_QUAY,0xFFFFFF);if(inchunk(x+2,110))wallq(x,110,x+4,110,-3,0,0,1,TL_QUAY,0xFFFFFF);}
 /* мосты */
 {float bxs[3]={56,136,248};for(int k=0;k<3;k++){float xa=bxs[k],xb=xa+8;add_box(xa,-1,84,xb,0,112,BK_GROUND);add_box(xa,0,84,xa+.7f,1.1f,112,BK_SOLID);add_box(xb-.7f,0,84,xb,1.1f,112,BK_SOLID);
   if(inchunk(xa+4,98)){bxt(xa,0,84,xa+.7f,1.1f,112,0xFFFFFF,TL_BASE,1.6f);bxt(xb-.7f,0,84,xb,1.1f,112,0xFFFFFF,TL_BASE,1.6f);bxt(xa-.1f,1.1f,84,xa+.8f,1.25f,112,0xFFFFFF,TL_BASE,1.6f);bxt(xb-.8f,1.1f,84,xb+.1f,1.25f,112,0xFFFFFF,TL_BASE,1.6f);
    /* арки: свод под мостом и быки */for(int a=0;a<3;a++){float zc=88.7f+a*9.3f;bxt(xa-.4f,-3.2f,zc-1.2f,xb+.4f,-.9f,zc+1.2f,0xFFFFFF,TL_QUAY,1.6f);mb_quad(v3(xa-.6f,-3.2f,zc-1.3f),v3(xa-.6f,-3.2f,zc+1.3f),v3(xa-.6f,-1.6f,zc+.4f),v3(xa-.6f,-1.6f,zc-.4f),v3(-1,0,0),0,0,0,0,RGBX(170,164,150));}
    for(float z=86;z<111;z+=1.2f){float a0=(z-84)/28.f*3.14159f;(void)a0;}
    bxt(xa,-1.2f,84,xb,0,112,0xFFFFFF,TL_QUAY,1.6f);for(int s=0;s<2;s++)for(int j=0;j<3;j++){float zz=91+j*7.f;float xx=s?xb-.35f:xa+.35f;mb_tube(v3(xx,1.25f,zz),v3(xx,3.4f,zz),.07f,.06f,6,RGBX(34,36,42));mb_box(v3(xx,3.55f,zz),v3(.14f,.18f,.14f),RGBX(34,36,42));}}}
  float cxs[2]={74,118};for(int k=0;k<2;k++){add_box(cxs[k],-1,84,cxs[k]+8,0,86,BK_GROUND);add_box(cxs[k],-1,110,cxs[k]+8,0,112,BK_GROUND);if(inchunk(cxs[k]+4,85)){bxt(cxs[k],-1.2f,84,cxs[k]+8,0,86,0xFFFFFF,TL_QUAY,1.6f);}if(inchunk(cxs[k]+4,111))bxt(cxs[k],-1.2f,110,cxs[k]+8,0,112,0xFFFFFF,TL_QUAY,1.6f);}}
 /* восточные набережные — твёрдый край, чтобы с баржи можно было залезть обратно */
 for(float x=200;x<WORLD;x+=8){if(x>=244&&x<260)continue;add_box(x,-3.2f,83.2f,x+8,0,84,BK_SOLID);add_box(x,-3.2f,112,x+8,0,112.8f,BK_SOLID);}
 /* границы мира */
 add_box(-6,-5,-6,0,60,WORLDZ+6,BK_FIRM);add_box(WORLD,-5,-6,WORLD+6,60,WORLDZ+6,BK_FIRM);add_box(-6,-5,-6,WORLD+6,60,0,BK_FIRM);add_box(-6,-5,WORLDZ,WORLD+6,60,WORLDZ+6,BK_FIRM);}
static void gen_east(void){
 /* сад: ограда, аллеи деревьев-лазалок, бассейн, партеры, статуи, павильон */
 fence(208,20,256,20);fence(264,20,316,20);fence(208,76,256,76);fence(264,76,316,76);fence(208,20,208,44);fence(208,52,208,76);fence(316,20,316,76);
 for(float x=218;x<=302;x+=9.f){if(fabsf(x-260)<6)continue;prop_ptree(x,30.5f);prop_ptree(x,65.5f);}
 for(float z=39.5f;z<=56.5f;z+=8.5f){prop_ptree(226,z);prop_ptree(294,z);}
 /* большой бассейн: каменный борт по кругу (8 отрезков), внутри — мелкая вода */
 for(int k=0;k<8;k++){float a=k*.785f+.3927f;float px=260+sinf(a)*9.2f,pz=48+cosf(a)*9.2f;add_box(px-1.9f,0,pz-1.9f,px+1.9f,.55f,pz+1.9f,BK_SOLID);}
 if(box_pass&&nbasin<32)basins[nbasin++]=(Basin){260,48,9.f,.4f};
 if(inchunk(260,48)){float rim[]={9.9f,0,10.f,.45f,9.7f,.6f,9.0f,.6f,8.8f,.45f,8.8f,.05f,.1f,.05f};mb_lathe(v3(260,0,48),rim,7,48,RGBX(214,208,196),1,1);}
 prop_fountain(260,48);
 for(int q=0;q<4;q++){float sx=q&1?1:-1,sz=q&2?1:-1;float hx0=260+sx*14,hz0=48+sz*7;hedge(fminf(hx0,hx0+sx*16),fminf(hz0,hz0+sz*9),fmaxf(hx0,hx0+sx*16),fminf(hz0,hz0+sz*9)+.9f,.95f);
  hedge(fminf(hx0,hx0+sx*16),fmaxf(hz0,hz0+sz*9)-.9f,fmaxf(hx0,hx0+sx*16),fmaxf(hz0,hz0+sz*9),.95f);}
 statue(244,44,.5f);statue(276,52,3.6f);statue(244,52,2.6f);statue(276,44,-.6f);statue(214,48,1.57f);statue(306,48,-1.57f);
 {float bp[6][2]={{222,43.3f},{236,43.3f},{284,43.3f},{298,43.3f},{222,52.7f},{298,52.7f}};for(int k=0;k<6;k++)prop_bench(bp[k][0],bp[k][1],1);}
 palace();
 /* жилые кварталы к востоку и югу */
 static const float C2[3][2]={{208,240},{248,280},{288,318}},R2[3][2]={{118,136},{144,176},{184,200}};
 for(int r=0;r<3;r++)for(int c=0;c<3;c++){if(c==1&&r==1)continue;fill_block(C2[c][0],R2[r][0],C2[c][1],R2[r][1],(uint32_t)(701+c*31+r*97));}
 column_monument(264,160);prop_bench(252,148,1);prop_bench(276,148,1);prop_bench(252,172,1);prop_bench(276,172,1);prop_tree(250,150);prop_tree(278,150);prop_tree(250,170);prop_tree(278,170);
 prop_hay(205.5f,150);prop_hay(244,128);prop_crates(242,190,2);prop_barrel(241.5f,189);
 /* баржи */
 barge(296,99,16,4.2f,0);barge(222,95,13,3.8f,0);
 /* фонари на новых улицах */
 for(int k=0;k<3;k++){float xc=(xe_[k][0]+xe_[k][1])/2;for(float z=9;z<200;z+=18){if(z>78&&z<122)continue;if(ground_tile(xc-3.f,z)<0)continue;if(k>0&&z<84)continue;prop_lamp(xc-3.2f,z);prop_lamp(xc+3.2f,z+9);}}
 for(int h=3;h<6;h++){float zc=(zs_[h][0]+zs_[h][1])/2;for(float x=212;x<WORLD;x+=18){int onv=0;for(int v=0;v<3;v++)if(x>xe_[v][0]-6&&x<xe_[v][1]+6)onv=1;if(onv)continue;prop_lamp(x,zc-3.2f);prop_lamp(x+9,zc+3.2f);}}
 for(float x=214;x<WORLD;x+=14){if(x>244&&x<262)continue;prop_lamp(x,80.6f);prop_lamp(x+5,115.6f);}
 for(float x=222;x<306;x+=18){prop_lamp(x,26.f);prop_lamp(x+9,70.f);}
 if(box_pass){for(int k=0;k<3;k++){if(k==0)strects[nstr++]=(Rect){xe_[k][0]+.5f,2,xe_[k][1]-.5f,83};strects[nstr++]=(Rect){xe_[k][0]+.5f,119,xe_[k][1]-.5f,198};}
  strects[nstr++]=(Rect){210,44.5f,250,51.5f};strects[nstr++]=(Rect){270,44.5f,310,51.5f};strects[nstr++]=(Rect){256.5f,22,263.5f,34};strects[nstr++]=(Rect){256.5f,62,263.5f,74};
  strects[nstr++]=(Rect){212,21,312,23.5f};strects[nstr++]=(Rect){212,72.5f,312,75};strects[nstr++]=(Rect){250,146,278,174};strects[nstr++]=(Rect){248.5f,84,255.5f,112};strects[nstr++]=(Rect){210,6,226,17};}}
static void gen_city(void){
 if(box_pass){nbld=0;nlamp=0;ntree=0;nhay=0;nstr=0;nchim=0;nbasin=0;}
 gen_streets_and_ground();
 static const float C[6][2]={{0,16},{24,56},{64,96},{104,136},{144,176},{184,200}},R[6][2]={{0,16},{24,56},{64,78},{118,136},{144,176},{184,200}};
 for(int r=0;r<6;r++)for(int c=0;c<6;c++){if((c==1&&r==1)||(c==2&&r==1)||(c==1&&r==4)||(c==4&&r==4))continue;fill_block(C[c][0],R[r][0],C[c][1],R[r][1],(uint32_t)(c*31+r*97+5));}
 cathedral();church();clocktower(26,146,32,152,34);
 /* особняк с садом */
 building(150,146,170,158,0,901,4,0);building(150,158,156,168,3,903,3,3);building(164,158,170,168,1,905,3,3);clocktower(168,146,174,152,30);
 {float gx0=144,gx1=176,gz0=144,gz1=176;add_box(gx0,0,gz0,158,3,gz0+.6f,BK_SOLID);add_box(162,0,gz0,gx1,3,gz0+.6f,BK_SOLID);add_box(gx0,0,gz1-.6f,gx1,3,gz1,BK_SOLID);add_box(gx0,0,gz0,gx0+.6f,3,gz1,BK_SOLID);add_box(gx1-.6f,0,gz0,gx1,3,gz1,BK_SOLID);
  if(inchunk(160,160)||inchunk(150,144)||inchunk(170,144)){bxt(gx0,0,gz0,158,3,gz0+.6f,0xFFFFFF,TL_BRICK,1.2f);bxt(162,0,gz0,gx1,3,gz0+.6f,0xFFFFFF,TL_BRICK,1.2f);bxt(gx0,0,gz1-.6f,gx1,3,gz1,0xFFFFFF,TL_BRICK,1.2f);bxt(gx0,0,gz0,gx0+.6f,3,gz1,0xFFFFFF,TL_BRICK,1.2f);bxt(gx1-.6f,0,gz0,gx1,3,gz1,0xFFFFFF,TL_BRICK,1.2f);
   for(int k=0;k<2;k++){float gxx=k?162:157.4f;bxt(gxx,0,gz0-.2f,gxx+.6f,3.6f,gz0+.8f,0xFFFFFF,TL_BASE,0);mb_sphere(v3(gxx+.3f,3.9f,gz0+.3f),v3(.3f,.3f,.3f),8,6,RGBX(214,200,150),0);}
   for(float x=158.1f;x<162;x+=.2f)mb_tube(v3(x,0,gz0+.3f),v3(x,2.6f,gz0+.3f),.012f,.012f,3,RGBX(28,28,32));}
  for(int k=0;k<6;k++){float hx=148+(k%3)*10+ (k/3)*0,hz=170-(k/3)*8;add_box(hx,0,hz,hx+6,1.3f,hz+1.2f,BK_SOLID);if(inchunk(hx+3,hz))bxt(hx,0,hz,hx+6,1.3f,hz+1.2f,0xFFFFFF,TL_HEDGE,1.2f);}
  prop_fountain(160,164);if(inchunk(160,170)){bxt(158.4f,0,168.4f,161.6f,.3f,171.6f,0xFFFFFF,TL_MARBLE,0);for(int s=0;s<4;s++){float px=s&1?161.2f:158.8f,pz=s&2?171.2f:168.8f;cylt(v3(px,.3f,pz),.12f,.1f,2.7f,10,TL_COLUMN,0xFFFFFF,1);}float rf2[]={2.6f,3.0f,1.8f,3.4f,.05f,4.3f};mb_lathe(v3(160,0,170),rf2,3,12,RGBX(80,120,110),1,1);mb_sphere(v3(160,.7f,170),v3(.28f,.22f,.28f),8,6,RGBX(200,50,60),0);}add_box(158.4f,0,168.4f,161.6f,.3f,171.6f,BK_SOLID);}
 /* площадь и рынок */
 prop_fountain(80,40);{float sp[8][2]={{68,30},{72,30},{88,30},{92,30},{68,50},{72,50},{88,50},{92,50}};for(int k=0;k<8;k++)prop_stall(sp[k][0],sp[k][1]);}
 {float tp[4][2]={{67,40},{93,40},{80,28},{80,52}};for(int k=0;k<4;k++)prop_tree(tp[k][0],tp[k][1]);prop_bench(75,46,1);prop_bench(85,46,1);prop_bench(75,34,1);prop_bench(85,34,1);prop_crates(94,54,3);prop_barrel(66,52);prop_barrel(66.8f,52.5f);}
 {float sp[8][2]={{30,150},{38,150},{46,150},{30,170},{38,170},{46,170},{52,160},{28,160}};for(int k=0;k<7;k++)if(k!=0)prop_stall(sp[k][0],sp[k][1]);prop_tree(40,160);prop_tree(34,158);prop_crates(48,156,2);prop_crates(48,157,3);prop_barrel(44,164);prop_barrel(45,164.6f);prop_hay(50,148);}
 prop_hay(26,33);prop_hay(83.5f,87.6f);prop_hay(28.5f,154.5f);prop_hay(171.5f,139);
 for(int k=0;k<8;k++)prop_tree(76+k*7.f,88.2f+(k&1)*20.f);
 /* фонари */
 for(int v=0;v<5;v++){float xc=(xs_[v][0]+xs_[v][1])/2;for(float z=9;z<200;z+=18){if(z>78&&z<122)continue;if(ground_tile(xc-3.f,z)<0)continue;prop_lamp(xc-3.2f,z);prop_lamp(xc+3.2f,z+9);}}
 for(int h=0;h<6;h++){float zc=(zs_[h][0]+zs_[h][1])/2;if(h==2||h==3)continue;for(float x=9;x<200;x+=18){int onv=0;for(int v=0;v<5;v++)if(x>xs_[v][0]-6&&x<xs_[v][1]+6)onv=1;if(onv)continue;prop_lamp(x,zc-3.2f);prop_lamp(x+9,zc+3.2f);}}
 for(float x=9;x<200;x+=14){int ok=1;for(int b2=0;b2<2;b2++){float bxx=b2?136:56;if(x>bxx-2&&x<bxx+10)ok=0;}if(ok&&(x<68||x>124)){prop_lamp(x,80.6f);prop_lamp(x+5,115.6f);}}
 /* улицы для ИИ */
 if(box_pass){for(int v=0;v<5;v++){strects[nstr++]=(Rect){xs_[v][0]+.5f,2,xs_[v][1]-.5f,83};strects[nstr++]=(Rect){xs_[v][0]+.5f,119,xs_[v][1]-.5f,198};}
  for(int h=0;h<6;h++){strects[nstr++]=(Rect){2,zs_[h][0]+.5f,198,zs_[h][1]-.5f};}
  strects[nstr++]=(Rect){64.5f,24.5f,95.5f,55.5f};strects[nstr++]=(Rect){24.5f,144.5f,55.5f,175.5f};strects[nstr++]=(Rect){56.5f,84,63.5f,112};strects[nstr++]=(Rect){136.5f,84,143.5f,112};}
 if(box_pass)nbld_old=nbld;
 gen_east();
 if(box_pass){for(int h=3;h<6;h++)strects[nstr++]=(Rect){202,zs_[h][0]+.5f,318,zs_[h][1]-.5f};strects[nstr++]=(Rect){202,78.5f,318,83.5f};}}
/* ---- меши: квартал (атлас) + его листва (своя текстура) ---- */
static Mesh chunk_m[NCH*NCH],chunk_f[NCH*NCH];static void build_planks(void);
static void build_foliage(void){mb_begin();
 for(int i=0;i<ncfol;i++){Fol*f=&cfol[i];float s=f->s;int q=f->q;float u0=(q&1)*.5f+.004f,v0=(q>>1)*.5f+.004f,u1=u0+.492f,v1=v0+.492f;uint32_t hh=hs(i*13,(int)(f->x*10));float rot=rf(hh)*3.14f;
  for(int k=0;k<3;k++){float a=rot+k*1.0472f;float dx=sinf(a)*s,dz=cosf(a)*s;int b=mb_count();V3 c=v3(f->x,f->y,f->z);
   V3 p[4]={v3(c.x-dx,c.y-s*.8f,c.z-dz),v3(c.x+dx,c.y-s*.8f,c.z+dz),v3(c.x+dx,c.y+s*.8f,c.z+dz),v3(c.x-dx,c.y+s*.8f,c.z-dz)};float uu[4]={u0,u1,u1,u0},vv[4]={v1,v1,v0,v0};
   for(int j=0;j<4;j++){V3 nn=vnrm(v3(p[j].x-f->cx,(p[j].y-f->cy)*1.3f+.4f,p[j].z-f->cz));uint32_t col=gfx_lerp(RGBX(200,210,190),0xFFFFFF,clampf_w((p[j].y-f->cy)*.25f+.5f));mb_vert(p[j].x,p[j].y,p[j].z,nn.x,nn.y,nn.z,uu[j],vv[j],col);}
   mb_index(b,b+1,b+2);mb_index(b,b+2,b+3);}
  {int b=mb_count();V3 c=v3(f->x,f->y+s*.15f,f->z);float r=s*.95f;V3 p[4]={v3(c.x-r,c.y,c.z-r),v3(c.x+r,c.y,c.z-r),v3(c.x+r,c.y,c.z+r),v3(c.x-r,c.y,c.z+r)};float uu[4]={u0,u1,u1,u0},vv[4]={v1,v1,v0,v0};
   for(int j=0;j<4;j++){V3 nn=vnrm(v3(p[j].x-f->cx,1.2f,p[j].z-f->cz));mb_vert(p[j].x,p[j].y,p[j].z,nn.x,nn.y,nn.z,uu[j],vv[j],0xFFFFFF);}mb_index(b,b+2,b+1);mb_index(b,b+3,b+2);}}}
static void build_world(void){build_atlas();nbox=0;
 for(int c=0;c<NCH*NCH;c++){cur_chunk=c;box_pass=(c==0);ncfol=0;mb_begin();gen_city();Mesh m=mb_end();float u0,v0,u1,v1;tile_uv(TL_WHITE,&u0,&v0,&u1,&v1);mb_fixuv(&m,(u0+u1)/2,(v0+v1)/2);chunk_m[c]=m;
  build_foliage();chunk_f[c]=mb_end();}
 box_pass=0;build_planks();grid_build();}
static float terrain_h(float x,float z){return in_river(x,z)?-3.f:0.f;}
static float floor_at(float x,float z,float y,float r,int*kind){float h=terrain_h(x,z);int k=BK_GROUND;int c0x=(int)floorf((x-r)/GC),c1x=(int)floorf((x+r)/GC),c0z=(int)floorf((z-r)/GC),c1z=(int)floorf((z+r)/GC);
 if(c0x<0)c0x=0;if(c0z<0)c0z=0;if(c1x>=GW)c1x=GW-1;if(c1z>=GW)c1z=GW-1;
 for(int cz=c0z;cz<=c1z;cz++)for(int cx=c0x;cx<=c1x;cx++){int c=cz*GW+cx;for(int i=cstart[c];i<cstart[c+1];i++){const Box*b=&boxes[citems[i]];if(x+r>b->x0&&x-r<b->x1&&z+r>b->z0&&z-r<b->z1&&b->y1<=y+.55f&&b->y1>h){h=b->y1;k=b->kind;}}}
 if(kind)*kind=k;return h;}
static int hit_box(float x,float y,float z,float r,float hgt,float stepy){int c0x=(int)floorf((x-r)/GC),c1x=(int)floorf((x+r)/GC),c0z=(int)floorf((z-r)/GC),c1z=(int)floorf((z+r)/GC);
 if(c0x<0)c0x=0;if(c0z<0)c0z=0;if(c1x>=GW)c1x=GW-1;if(c1z>=GW)c1z=GW-1;
 for(int cz=c0z;cz<=c1z;cz++)for(int cx=c0x;cx<=c1x;cx++){int c=cz*GW+cx;for(int i=cstart[c];i<cstart[c+1];i++){const Box*b=&boxes[citems[i]];if(b->kind==BK_GROUND)continue;
  if(x+r>b->x0&&x-r<b->x1&&z+r>b->z0&&z-r<b->z1&&b->y1>y+stepy&&b->y0<y+hgt)return citems[i];}}
 return -1;}
static int seg_blocked(V3 a,V3 b){float dx=b.x-a.x,dy=b.y-a.y,dz=b.z-a.z,len=sqrtf(dx*dx+dy*dy+dz*dz);int n=(int)(len/.7f)+1;
 for(int s=1;s<n;s++){float t=(float)s/n,x=a.x+dx*t,y=a.y+dy*t,z=a.z+dz*t;int cx=(int)(x/GC),cz=(int)(z/GC);if(cx<0||cz<0||cx>=GW||cz>=GW)return 1;int c=cz*GW+cx;
  for(int i=cstart[c];i<cstart[c+1];i++){const Box*bb=&boxes[citems[i]];if(bb->kind==BK_GROUND||bb->kind==BK_HAY)continue;if(x>bb->x0&&x<bb->x1&&z>bb->z0&&z<bb->z1&&y>bb->y0&&y<bb->y1)return 1;}}
 return 0;}
/* ---- мостки между крышами через улицы ---- */
static Mesh plank_m;static float plankv[96][6];static int nplank;
static void plank_pass(int lo,int hi,int cap){
 for(int i=lo;i<hi&&nplank<cap;i++)for(int j=lo;j<hi&&nplank<cap;j++){if(i==j)continue;Bd*a=&blds[i],*b=&blds[j];float c=0,x0=0,x1=0,z0=0,z1=0;int ax=-1;
  if(fabsf(b->x0-a->x1-8.f)<.6f){float lo2=fmaxf(a->z0,b->z0),hi2=fminf(a->z1,b->z1);if(hi2-lo2>=3.f){c=(lo2+hi2)*.5f;x0=a->x1;x1=b->x0;z0=c-.5f;z1=c+.5f;ax=0;}}
  if(ax<0&&fabsf(b->z0-a->z1-8.f)<.6f){float lo2=fmaxf(a->x0,b->x0),hi2=fminf(a->x1,b->x1);if(hi2-lo2>=3.f){c=(lo2+hi2)*.5f;z0=a->z1;z1=b->z0;x0=c-.5f;x1=c+.5f;ax=1;}}
  if(ax<0)continue;if(hs(i*31+j,77)%3)continue;float xc=(x0+x1)*.5f,zc=(z0+z1)*.5f;if(in_river(xc,zc)||(zc>78&&zc<118))continue;
  int near=0;for(int q=0;q<nplank;q++)if(fabsf(plankv[q][0]-xc)<6&&fabsf(plankv[q][2]-zc)<6)near=1;if(near)continue;
  float y=fminf(a->H,b->H);add_box(x0,y-.22f,z0,x1,y,z1,BK_SOLID);plankv[nplank][0]=xc;plankv[nplank][1]=y;plankv[nplank][2]=zc;plankv[nplank][3]=(float)ax;plankv[nplank][4]=(float)i;plankv[nplank][5]=(float)j;nplank++;
  bxt(x0,y-.22f,z0,x1,y,z1,0xFFFFFF,TL_PLANKS,1.f);
  if(ax==0){for(int s=0;s<2;s++){float zz=s?z1+.04f:z0-.04f;mb_tube(v3(x0,y+.9f,zz),v3(x1,y+.85f,zz),.02f,.02f,5,RGBX(150,120,80));for(float t=0;t<=1.01f;t+=.25f)mb_tube(v3(x0+(x1-x0)*t,y,zz),v3(x0+(x1-x0)*t,y+.9f,zz),.03f,.03f,5,RGBX(90,64,44));}}
  else{for(int s=0;s<2;s++){float xx=s?x1+.04f:x0-.04f;mb_tube(v3(xx,y+.9f,z0),v3(xx,y+.85f,z1),.02f,.02f,5,RGBX(150,120,80));for(float t=0;t<=1.01f;t+=.25f)mb_tube(v3(xx,y,z0+(z1-z0)*t),v3(xx,y+.9f,z0+(z1-z0)*t),.03f,.03f,5,RGBX(90,64,44));}}}}
static void build_planks(void){box_pass=1;nplank=0;mb_begin();plank_pass(0,nbld_old,40);plank_pass(nbld_old,nbld,64);
 plank_m=mb_end();float u0,v0,u1,v1;tile_uv(TL_WHITE,&u0,&v0,&u1,&v1);mb_fixuv(&plank_m,(u0+u1)/2,(v0+v1)/2);box_pass=0;}
/* здание, в чьей крыше точка; -1 — нет */
static int bld_at(float x,float z){for(int i=0;i<nbld;i++){Bd*b=&blds[i];if(x>=b->x0&&x<b->x1&&z>=b->z0&&z<b->z1)return i;}return -1;}
