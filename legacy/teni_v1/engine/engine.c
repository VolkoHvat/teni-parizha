#include "engine.h"
#include <math.h>
#include <string.h>
uint32_t fb[FBW_MAX*FBH_MAX];int fbw=SW,fbh=SH,rs=1;int hud_y0=1<<20,hud_y1=-1;TextCmd tq[TQ_MAX];int tqn;int key_down[K_N],key_hit[K_N];int mouse_dx,mouse_dy,mouse_want;float g_time;
int gfx_quality=2;float g_fps=60;
static uint32_t*tgt=fb;static int tw=SW,th=SH,onscreen=1;static float us=1;
static inline int imin(int a,int b){return a<b?a:b;}static inline int imax(int a,int b){return a>b?a:b;}
static inline float fclampf(float v,float a,float b){return v<a?a:v>b?b:v;}
/* непрозрачное смешивание (текстуры) */
static inline uint32_t mix(uint32_t d,uint32_t s,int a){int dr=(d>>16)&255,dg=(d>>8)&255,db=d&255,sr=(s>>16)&255,sg=(s>>8)&255,sb=s&255;
 dr+=((sr-dr)*a)/255;dg+=((sg-dg)*a)/255;db+=((sb-db)*a)/255;return (d&0xFF000000u)|(uint32_t)((dr<<16)|(dg<<8)|db);}
/* «поверх» с предумноженной альфой (слой интерфейса) */
static inline uint32_t over(uint32_t d,uint32_t s,int a){int ia=255-a;int r=(((s>>16)&255)*a+((d>>16)&255)*ia+127)/255,g=(((s>>8)&255)*a+((d>>8)&255)*ia+127)/255,b=((s&255)*a+(d&255)*ia+127)/255,al=a+((d>>24)*ia+127)/255;
 return ((uint32_t)al<<24)|((uint32_t)r<<16)|((uint32_t)g<<8)|(uint32_t)b;}
uint32_t gfx_lerp(uint32_t a,uint32_t b,float t){t=fclampf(t,0,1);int k=(int)(t*255.f+.5f);int ar=(a>>16)&255,ag=(a>>8)&255,ab=a&255;ar+=((((b>>16)&255)-ar)*k)/255;ag+=((((b>>8)&255)-ag)*k)/255;ab+=(((b&255)-ab)*k)/255;return (uint32_t)((ar<<16)|(ag<<8)|ab);}
void gfx_begin(void){tqn=0;if(hud_y1>=hud_y0){int y0=imax(0,hud_y0),y1=imin(fbh-1,hud_y1);if(y1>=y0)memset(&fb[y0*fbw],0,sizeof(uint32_t)*(size_t)fbw*(y1-y0+1));}hud_y0=1<<20;hud_y1=-1;}
void gfx_target(uint32_t*buf,int w,int h){tgt=buf;tw=w;th=h;us=1;onscreen=0;}
void gfx_target_s(uint32_t*buf,int w,int h,float s){tgt=buf;tw=w;th=h;us=s;onscreen=0;}
void gfx_target_screen(void){tgt=fb;tw=fbw;th=fbh;us=(float)rs;onscreen=1;}
void gfx_set_scale(int s){if(s<1)s=1;if(s>4)s=4;rs=s;fbw=SW*s;fbh=SH*s;memset(fb,0,sizeof(uint32_t)*(size_t)fbw*fbh);hud_y0=1<<20;hud_y1=-1;if(onscreen)gfx_target_screen();}
static inline void dirty(int y0,int y1){if(!onscreen)return;if(y0<hud_y0)hud_y0=y0;if(y1>hud_y1)hud_y1=y1;}
static inline void ppx(int x,int y,uint32_t c,int a){if((unsigned)x>=(unsigned)tw||(unsigned)y>=(unsigned)th||a<=0)return;if(a>255)a=255;
 if(onscreen){tgt[y*tw+x]=a>=255?(0xFF000000u|(c&0xFFFFFF)):over(tgt[y*tw+x],c,a);}else{if(a>=255){tgt[y*tw+x]=(tgt[y*tw+x]&0xFF000000u)|(c&0xFFFFFF);return;}tgt[y*tw+x]=mix(tgt[y*tw+x],c,a);}}
static void rect_raw(int x,int y,int w,int h,uint32_t c,int a){int x1=imin(x+w,tw),y1=imin(y+h,th);x=imax(x,0);y=imax(y,0);if(x>=x1||y>=y1)return;dirty(y,y1-1);
 for(int j=y;j<y1;j++)for(int i=x;i<x1;i++)ppx(i,j,c,a);}
void gfx_px(int x,int y,uint32_t c,int a){int s=(int)us;if(s<1)s=1;dirty(y*s,y*s+s-1);for(int j=0;j<s;j++)for(int i=0;i<s;i++)ppx(x*s+i,y*s+j,c,a);}
void gfx_rect(int x,int y,int w,int h,uint32_t c,int a){rect_raw((int)(x*us),(int)(y*us),(int)(w*us),(int)(h*us),c,a);}
void gfx_vgrad(int x,int y,int w,int h,uint32_t t,uint32_t b){int H=(int)(h*us);for(int j=0;j<H;j++)rect_raw((int)(x*us),(int)(y*us)+j,(int)(w*us),1,gfx_lerp(t,b,H>1?(float)j/(H-1):0),255);}
static void circle_raw(float cx,float cy,float r,uint32_t c,int a){
 int x0=imax(0,(int)floorf(cx-r-1)),x1=imin(tw-1,(int)ceilf(cx+r+1)),y0=imax(0,(int)floorf(cy-r-1)),y1=imin(th-1,(int)ceilf(cy+r+1));if(y1>=y0)dirty(y0,y1);
 for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){float d=hypotf(x+.5f-cx,y+.5f-cy),cv=fclampf(r+.5f-d,0,1);if(cv>0)ppx(x,y,c,(int)(a*cv));}}
void gfx_circle(float cx,float cy,float r,uint32_t c,int a){circle_raw(cx*us,cy*us,r*us,c,a);}
void gfx_ring(float cx,float cy,float r,float t,uint32_t c,int a){cx*=us;cy*=us;r*=us;t*=us;float R=r+t;
 int x0=imax(0,(int)floorf(cx-R-1)),x1=imin(tw-1,(int)ceilf(cx+R+1)),y0=imax(0,(int)floorf(cy-R-1)),y1=imin(th-1,(int)ceilf(cy+R+1));if(y1>=y0)dirty(y0,y1);
 for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){float d=hypotf(x+.5f-cx,y+.5f-cy),cv=fclampf(t*.5f+.5f-fabsf(d-r),0,1);if(cv>0)ppx(x,y,c,(int)(a*cv));}}
void gfx_ellipse(float cx,float cy,float rx,float ry,uint32_t c,int a){if(rx<=0||ry<=0)return;cx*=us;cy*=us;rx*=us;ry*=us;float m=fminf(rx,ry);
 int x0=imax(0,(int)floorf(cx-rx-1)),x1=imin(tw-1,(int)ceilf(cx+rx+1)),y0=imax(0,(int)floorf(cy-ry-1)),y1=imin(th-1,(int)ceilf(cy+ry+1));if(y1>=y0)dirty(y0,y1);
 for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){float dx=(x+.5f-cx)/rx,dy=(y+.5f-cy)/ry,d=sqrtf(dx*dx+dy*dy),cv=fclampf((1-d)*m+.5f,0,1);if(cv>0)ppx(x,y,c,(int)(a*cv));}}
void gfx_line(float x0,float y0,float x1,float y1,float t,uint32_t c,int a){x0*=us;y0*=us;x1*=us;y1*=us;t*=us;
 float dx=x1-x0,dy=y1-y0,l2=dx*dx+dy*dy,r=t*.5f;
 int ax=imax(0,(int)floorf(fminf(x0,x1)-r-1)),bx=imin(tw-1,(int)ceilf(fmaxf(x0,x1)+r+1)),ay=imax(0,(int)floorf(fminf(y0,y1)-r-1)),by=imin(th-1,(int)ceilf(fmaxf(y0,y1)+r+1));if(by>=ay)dirty(ay,by);
 for(int y=ay;y<=by;y++)for(int x=ax;x<=bx;x++){float px=x+.5f,py=y+.5f,u=l2>0?((px-x0)*dx+(py-y0)*dy)/l2:0;u=fclampf(u,0,1);
  float d=hypotf(px-(x0+u*dx),py-(y0+u*dy)),cv=fclampf(r+.5f-d,0,1);if(cv>0)ppx(x,y,c,(int)(a*cv));}}
void gfx_poly(const float*p0,int n,uint32_t c,int a){if(n<3||n>8)return;float p[16];for(int i=0;i<2*n;i++)p[i]=p0[i]*us;
 float miny=p[1],maxy=p[1];for(int i=1;i<n;i++){miny=fminf(miny,p[2*i+1]);maxy=fmaxf(maxy,p[2*i+1]);}
 int y0=imax(0,(int)floorf(miny)),y1=imin(th-1,(int)ceilf(maxy));if(y1>=y0)dirty(y0,y1);
 for(int y=y0;y<=y1;y++){float sy=y+.5f,xs[16];int k=0;
  for(int i=0;i<n;i++){int j=(i+1)%n;float ax=p[2*i],ay=p[2*i+1],bx=p[2*j],by=p[2*j+1];if((ay<=sy&&by>sy)||(by<=sy&&ay>sy)){if(k<16)xs[k++]=ax+(sy-ay)/(by-ay)*(bx-ax);}}
  for(int i=1;i<k;i++){float v=xs[i];int j=i-1;while(j>=0&&xs[j]>v){xs[j+1]=xs[j];j--;}xs[j+1]=v;}
  for(int i=0;i+1<k;i+=2){int xa=(int)ceilf(xs[i]-.5f),xb=(int)ceilf(xs[i+1]-.5f);for(int x=imax(xa,0);x<imin(xb,tw);x++)ppx(x,y,c,a);}}}
void gfx_tri(float x0,float y0,float x1,float y1,float x2,float y2,uint32_t c,int a){float p[6]={x0,y0,x1,y1,x2,y2};gfx_poly(p,3,c,a);}
void gfx_quad(float x0,float y0,float x1,float y1,float x2,float y2,float x3,float y3,uint32_t c,int a){float p[8]={x0,y0,x1,y1,x2,y2,x3,y3};gfx_poly(p,4,c,a);}
/* мягкое свечение: на экране — аддитивно (альфа не растёт), в текстуре — осветление */
void gfx_glow(float cx,float cy,float r,uint32_t c,int a){cx*=us;cy*=us;r*=us;
 int x0=imax(0,(int)(cx-r)),x1=imin(tw-1,(int)(cx+r)),y0=imax(0,(int)(cy-r)),y1=imin(th-1,(int)(cy+r));int cr=(c>>16)&255,cg=(c>>8)&255,cb=c&255;if(y1>=y0)dirty(y0,y1);
 for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){float d=hypotf(x+.5f-cx,y+.5f-cy)/r;if(d>=1)continue;float f=(1-d);f=f*f*a/255.f;
  uint32_t p=tgt[y*tw+x];int pr=(p>>16)&255,pg=(p>>8)&255,pb=p&255;pr=imin(255,pr+(int)(cr*f));pg=imin(255,pg+(int)(cg*f));pb=imin(255,pb+(int)(cb*f));
  uint32_t al=p>>24;tgt[y*tw+x]=(al<<24)|(uint32_t)((pr<<16)|(pg<<8)|pb);}}
void gfx_text(int x,int y,int w,int h,int size,uint32_t col,int flags,const char*s){if(tqn>=TQ_MAX)return;TextCmd*t=&tq[tqn++];t->x=x;t->y=y;t->w=w;t->h=h;t->size=size;t->col=col;t->flags=flags;
 strncpy(t->s,s,sizeof t->s-1);t->s[sizeof t->s-1]=0;{int a=imax(0,y*rs-6),b=imin(fbh-1,(y+h)*rs+6);if(a<hud_y0)hud_y0=a;if(b>hud_y1)hud_y1=b;}}
