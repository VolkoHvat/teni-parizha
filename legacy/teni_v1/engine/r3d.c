/* Общая часть 3D: матрицы, проекция, построитель мешей (процессор). Отрисовка — в r3d_gl.c */
#include "r3d.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
R3Env r3e;Mat r3_vp;float r3_tint[3]={1,1,1};int r3_clip[4]={0,0,1<<20,1<<20};
Mat m_ident(void){Mat r;memset(&r,0,sizeof r);r.m[0]=r.m[5]=r.m[10]=r.m[15]=1;return r;}
Mat m_mul(Mat a,Mat b){Mat r;for(int i=0;i<4;i++)for(int j=0;j<4;j++){float s=0;for(int k=0;k<4;k++)s+=a.m[i*4+k]*b.m[k*4+j];r.m[i*4+j]=s;}return r;}
Mat m_trans(float x,float y,float z){Mat r=m_ident();r.m[3]=x;r.m[7]=y;r.m[11]=z;return r;}
Mat m_scale(float x,float y,float z){Mat r=m_ident();r.m[0]=x;r.m[5]=y;r.m[10]=z;return r;}
Mat m_roty(float a){Mat r=m_ident();float c=cosf(a),s=sinf(a);r.m[0]=c;r.m[2]=s;r.m[8]=-s;r.m[10]=c;return r;}
Mat m_rotx(float a){Mat r=m_ident();float c=cosf(a),s=sinf(a);r.m[5]=c;r.m[6]=-s;r.m[9]=s;r.m[10]=c;return r;}
Mat m_rotz(float a){Mat r=m_ident();float c=cosf(a),s=sinf(a);r.m[0]=c;r.m[1]=-s;r.m[4]=s;r.m[5]=c;return r;}
static V3 vsub(V3 a,V3 b){V3 r={a.x-b.x,a.y-b.y,a.z-b.z};return r;}
static V3 vcross(V3 a,V3 b){V3 r={a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};return r;}
static float vdot(V3 a,V3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static V3 vnorm(V3 a){float l=sqrtf(vdot(a,a));if(l<1e-8f)l=1;V3 r={a.x/l,a.y/l,a.z/l};return r;}
Mat m_look(V3 e,V3 at){V3 up={0,1,0},f=vnorm(vsub(at,e)),r=vnorm(vcross(f,up)),u=vcross(r,f);Mat m=m_ident();
 m.m[0]=r.x;m.m[1]=r.y;m.m[2]=r.z;m.m[3]=-vdot(r,e);m.m[4]=u.x;m.m[5]=u.y;m.m[6]=u.z;m.m[7]=-vdot(u,e);
 m.m[8]=-f.x;m.m[9]=-f.y;m.m[10]=-f.z;m.m[11]=vdot(f,e);return m;}
Mat m_persp(float fy,float asp,float n,float f){Mat m;memset(&m,0,sizeof m);float t=1.f/tanf(fy*.5f);m.m[0]=t/asp;m.m[5]=t;m.m[10]=(f+n)/(n-f);m.m[11]=2*f*n/(n-f);m.m[14]=-1;return m;}
Mat m_inv(Mat A){const float*m=A.m;float inv[16];
 inv[0]=m[5]*m[10]*m[15]-m[5]*m[11]*m[14]-m[9]*m[6]*m[15]+m[9]*m[7]*m[14]+m[13]*m[6]*m[11]-m[13]*m[7]*m[10];
 inv[4]=-m[4]*m[10]*m[15]+m[4]*m[11]*m[14]+m[8]*m[6]*m[15]-m[8]*m[7]*m[14]-m[12]*m[6]*m[11]+m[12]*m[7]*m[10];
 inv[8]=m[4]*m[9]*m[15]-m[4]*m[11]*m[13]-m[8]*m[5]*m[15]+m[8]*m[7]*m[13]+m[12]*m[5]*m[11]-m[12]*m[7]*m[9];
 inv[12]=-m[4]*m[9]*m[14]+m[4]*m[10]*m[13]+m[8]*m[5]*m[14]-m[8]*m[6]*m[13]-m[12]*m[5]*m[10]+m[12]*m[6]*m[9];
 inv[1]=-m[1]*m[10]*m[15]+m[1]*m[11]*m[14]+m[9]*m[2]*m[15]-m[9]*m[3]*m[14]-m[13]*m[2]*m[11]+m[13]*m[3]*m[10];
 inv[5]=m[0]*m[10]*m[15]-m[0]*m[11]*m[14]-m[8]*m[2]*m[15]+m[8]*m[3]*m[14]+m[12]*m[2]*m[11]-m[12]*m[3]*m[10];
 inv[9]=-m[0]*m[9]*m[15]+m[0]*m[11]*m[13]+m[8]*m[1]*m[15]-m[8]*m[3]*m[13]-m[12]*m[1]*m[11]+m[12]*m[3]*m[9];
 inv[13]=m[0]*m[9]*m[14]-m[0]*m[10]*m[13]-m[8]*m[1]*m[14]+m[8]*m[2]*m[13]+m[12]*m[1]*m[10]-m[12]*m[2]*m[9];
 inv[2]=m[1]*m[6]*m[15]-m[1]*m[7]*m[14]-m[5]*m[2]*m[15]+m[5]*m[3]*m[14]+m[13]*m[2]*m[7]-m[13]*m[3]*m[6];
 inv[6]=-m[0]*m[6]*m[15]+m[0]*m[7]*m[14]+m[4]*m[2]*m[15]-m[4]*m[3]*m[14]-m[12]*m[2]*m[7]+m[12]*m[3]*m[6];
 inv[10]=m[0]*m[5]*m[15]-m[0]*m[7]*m[13]-m[4]*m[1]*m[15]+m[4]*m[3]*m[13]+m[12]*m[1]*m[7]-m[12]*m[3]*m[5];
 inv[14]=-m[0]*m[5]*m[14]+m[0]*m[6]*m[13]+m[4]*m[1]*m[14]-m[4]*m[2]*m[13]-m[12]*m[1]*m[6]+m[12]*m[2]*m[5];
 inv[3]=-m[1]*m[6]*m[11]+m[1]*m[7]*m[10]+m[5]*m[2]*m[11]-m[5]*m[3]*m[10]-m[9]*m[2]*m[7]+m[9]*m[3]*m[6];
 inv[7]=m[0]*m[6]*m[11]-m[0]*m[7]*m[10]-m[4]*m[2]*m[11]+m[4]*m[3]*m[10]+m[8]*m[2]*m[7]-m[8]*m[3]*m[6];
 inv[11]=-m[0]*m[5]*m[11]+m[0]*m[7]*m[9]+m[4]*m[1]*m[11]-m[4]*m[3]*m[9]-m[8]*m[1]*m[7]+m[8]*m[3]*m[5];
 inv[15]=m[0]*m[5]*m[10]-m[0]*m[6]*m[9]-m[4]*m[1]*m[10]+m[4]*m[2]*m[9]+m[8]*m[1]*m[6]-m[8]*m[2]*m[5];
 float det=m[0]*inv[0]+m[1]*inv[4]+m[2]*inv[8]+m[3]*inv[12];Mat r;if(fabsf(det)<1e-20f)return m_ident();det=1.f/det;for(int i=0;i<16;i++)r.m[i]=inv[i]*det;return r;}
int r3_project(V3 p,float*sx,float*sy){const float*m=r3_vp.m;float x=m[0]*p.x+m[1]*p.y+m[2]*p.z+m[3],y=m[4]*p.x+m[5]*p.y+m[6]*p.z+m[7],w=m[12]*p.x+m[13]*p.y+m[14]*p.z+m[15];
 if(w<.2f)return 0;*sx=(x/w*.5f+.5f)*SW;*sy=(1-(y/w*.5f+.5f))*SH;return 1;}
int r3_visible(V3 c,float r){const float*m=r3_vp.m;float cx=m[0]*c.x+m[1]*c.y+m[2]*c.z+m[3],cy=m[4]*c.x+m[5]*c.y+m[6]*c.z+m[7],cw=m[12]*c.x+m[13]*c.y+m[14]*c.z+m[15];
 if(cw+r<.3f)return 0;if(cw<.3f)return 1;return fabsf(cx/cw)<1+r*fabsf(m[0])/cw*1.2f+r/cw&&fabsf(cy/cw)<1+r*fabsf(m[5])/cw*1.2f+r/cw;}
Tex tex_new(int w,int h,int em){Tex t;memset(&t,0,sizeof t);t.w=w;t.h=h;t.px=calloc((size_t)w*h,4);t.em=em?calloc((size_t)w*h,4):NULL;t.mip=1;return t;}
/* ---- построитель мешей (растущие буферы, 32-битные индексы) ---- */
static float muv[4];static int muvon;void mb_uvrect(float u0,float v0,float u1,float v1){muv[0]=u0;muv[1]=v0;muv[2]=u1;muv[3]=v1;muvon=(u1>u0);}
static Vtx*bv;static uint32_t*bi;static int bnv,bni,bcv,bci;
void mb_begin(void){bnv=bni=0;}
int mb_count(void){return bnv;}
static int av(float x,float y,float z,float nx,float ny,float nz,float u,float v,uint32_t c){if(bnv>=bcv){bcv=bcv?bcv*2:65536;bv=realloc(bv,sizeof(Vtx)*(size_t)bcv);}
 Vtx*t=&bv[bnv];t->x=x;t->y=y;t->z=z;t->nx=nx;t->ny=ny;t->nz=nz;t->u=u;t->v=v;t->c=c;return bnv++;}
static void ai(int a,int b,int c){if(bni+3>bci){bci=bci?bci*2:196608;bi=realloc(bi,sizeof(uint32_t)*(size_t)bci);}bi[bni++]=(uint32_t)a;bi[bni++]=(uint32_t)b;bi[bni++]=(uint32_t)c;}
void mb_vert(float x,float y,float z,float nx,float ny,float nz,float u,float v,uint32_t c){av(x,y,z,nx,ny,nz,u,v,c);}
void mb_index(int a,int b,int c){ai(a,b,c);}
void mb_xform(int from,Mat M){const float*m=M.m;for(int i=from;i<bnv;i++){Vtx*t=&bv[i];float x=t->x,y=t->y,z=t->z,nx=t->nx,ny=t->ny,nz=t->nz;
 t->x=m[0]*x+m[1]*y+m[2]*z+m[3];t->y=m[4]*x+m[5]*y+m[6]*z+m[7];t->z=m[8]*x+m[9]*y+m[10]*z+m[11];
 float ax=m[0]*nx+m[1]*ny+m[2]*nz,ay=m[4]*nx+m[5]*ny+m[6]*nz,az=m[8]*nx+m[9]*ny+m[10]*nz,l=sqrtf(ax*ax+ay*ay+az*az);if(l<1e-8f)l=1;t->nx=ax/l;t->ny=ay/l;t->nz=az/l;}}
void mb_sphere(V3 c,V3 r,int seg,int ring,uint32_t col,int uvf){int base=bnv;
 for(int j=0;j<=ring;j++){float ph=3.14159265f*j/ring,sy=cosf(ph),sr=sinf(ph);
  for(int s=0;s<=seg;s++){float th=6.2831853f*s/seg,ux=sr*sinf(th),uz=sr*cosf(th);
   float nx=ux/r.x,ny=sy/r.y,nz=uz/r.z,l=sqrtf(nx*nx+ny*ny+nz*nz);if(l<1e-6f)l=1;
   av(c.x+r.x*ux,c.y+r.y*sy,c.z+r.z*uz,nx/l,ny/l,nz/l,uvf?.5f+.5f*ux:0,uvf?.5f-.5f*sy:0,col);}}
 for(int j=0;j<ring;j++)for(int s=0;s<seg;s++){int a=base+j*(seg+1)+s,b=a+seg+1;ai(a,b,a+1);ai(a+1,b,b+1);}}
void mb_cyl(V3 bs,float r0,float r1,float h,int seg,uint32_t col){int base=bnv;float sl=(r0-r1)/(h!=0?h:1);
 for(int s=0;s<=seg;s++){float th=6.2831853f*s/seg,sx=sinf(th),cz=cosf(th),l=sqrtf(1+sl*sl);
  av(bs.x+r0*sx,bs.y,bs.z+r0*cz,sx/l,sl/l,cz/l,0,0,col);av(bs.x+r1*sx,bs.y+h,bs.z+r1*cz,sx/l,sl/l,cz/l,0,0,col);}
 for(int s=0;s<seg;s++){int b0=base+s*2,t0=b0+1,b1=b0+2,t1=b0+3;ai(b0,b1,t0);ai(b1,t1,t0);}
 int ct=av(bs.x,bs.y+h,bs.z,0,1,0,0,0,col),cb=av(bs.x,bs.y,bs.z,0,-1,0,0,0,col);int rt=bnv,rb;
 for(int s=0;s<=seg;s++){float th=6.2831853f*s/seg;av(bs.x+r1*sinf(th),bs.y+h,bs.z+r1*cosf(th),0,1,0,0,0,col);}rb=bnv;
 for(int s=0;s<=seg;s++){float th=6.2831853f*s/seg;av(bs.x+r0*sinf(th),bs.y,bs.z+r0*cosf(th),0,-1,0,0,0,col);}
 for(int s=0;s<seg;s++){ai(ct,rt+s,rt+s+1);ai(cb,rb+s+1,rb+s);}}
void mb_quad(V3 a,V3 b,V3 c,V3 d,V3 n,float u0,float v0,float u1,float v1,uint32_t col){
 if(muvon){float A=muv[0],B=muv[1],C=muv[2],D=muv[3];u0=A+u0*(C-A);u1=A+u1*(C-A);v0=B+v0*(D-B);v1=B+v1*(D-B);}
 int i0=av(a.x,a.y,a.z,n.x,n.y,n.z,u0,v1,col),i1=av(b.x,b.y,b.z,n.x,n.y,n.z,u1,v1,col),i2=av(c.x,c.y,c.z,n.x,n.y,n.z,u1,v0,col),i3=av(d.x,d.y,d.z,n.x,n.y,n.z,u0,v0,col);ai(i0,i1,i2);ai(i0,i2,i3);}
void mb_tri(V3 a,V3 b,V3 c,uint32_t col){V3 n=vnorm(vcross(vsub(b,a),vsub(c,a)));int i0=av(a.x,a.y,a.z,n.x,n.y,n.z,0,0,col),i1=av(b.x,b.y,b.z,n.x,n.y,n.z,0,0,col),i2=av(c.x,c.y,c.z,n.x,n.y,n.z,0,0,col);ai(i0,i1,i2);}
static V3 P3(float x,float y,float z){V3 r={x,y,z};return r;}
/* коробка: UV по реальному размеру граней, чтобы текстура не растягивалась (в пределах прямоугольника атласа) */
void mb_box(V3 c,V3 h,uint32_t col){float x0=c.x-h.x,x1=c.x+h.x,y0=c.y-h.y,y1=c.y+h.y,z0=c.z-h.z,z1=c.z+h.z;
 mb_quad(P3(x0,y0,z1),P3(x1,y0,z1),P3(x1,y1,z1),P3(x0,y1,z1),P3(0,0,1),0,0,1,1,col);
 mb_quad(P3(x1,y0,z0),P3(x0,y0,z0),P3(x0,y1,z0),P3(x1,y1,z0),P3(0,0,-1),0,0,1,1,col);
 mb_quad(P3(x0,y0,z0),P3(x0,y0,z1),P3(x0,y1,z1),P3(x0,y1,z0),P3(-1,0,0),0,0,1,1,col);
 mb_quad(P3(x1,y0,z1),P3(x1,y0,z0),P3(x1,y1,z0),P3(x1,y1,z1),P3(1,0,0),0,0,1,1,col);
 mb_quad(P3(x0,y1,z1),P3(x1,y1,z1),P3(x1,y1,z0),P3(x0,y1,z0),P3(0,1,0),0,0,1,1,col);
 mb_quad(P3(x0,y0,z0),P3(x1,y0,z0),P3(x1,y0,z1),P3(x0,y0,z1),P3(0,-1,0),0,0,1,1,col);}
Mesh mb_temp(void){Mesh m;memset(&m,0,sizeof m);m.v=bv;m.nv=bnv;m.i=bi;m.ni=bni;m.temp=1;return m;}
Mesh mb_end(void){Mesh m;memset(&m,0,sizeof m);m.nv=bnv;m.ni=bni;m.v=malloc(sizeof(Vtx)*(size_t)(bnv?bnv:1));m.i=malloc(sizeof(uint32_t)*(size_t)(bni?bni:1));
 if(bnv)memcpy(m.v,bv,sizeof(Vtx)*(size_t)bnv);if(bni)memcpy(m.i,bi,sizeof(uint32_t)*(size_t)bni);return m;}
void mb_lathe(V3 b,const float*pr,int n,int seg,uint32_t col,float sx,float sz){int base=bnv;
 for(int i=0;i<n;i++){float r=pr[2*i],y=pr[2*i+1];int ip=i>0?i-1:0,in=i<n-1?i+1:n-1;float dr=pr[2*in]-pr[2*ip],dy=pr[2*in+1]-pr[2*ip+1],nl=sqrtf(dr*dr+dy*dy)+1e-6f,nr=dy/nl,ny=-dr/nl;
  for(int s=0;s<=seg;s++){float th=6.2831853f*s/seg,sn=sinf(th),cs=cosf(th),nx=nr*sn/sx,nz=nr*cs/sz,l=sqrtf(nx*nx+ny*ny+nz*nz)+1e-6f;av(b.x+r*sn*sx,b.y+y,b.z+r*cs*sz,nx/l,ny/l,nz/l,0,0,col);}}
 for(int i=0;i<n-1;i++)for(int s=0;s<seg;s++){int a=base+i*(seg+1)+s,b2=a+seg+1;ai(a,a+1,b2);ai(a+1,b2+1,b2);}}
void mb_tube(V3 a,V3 b,float r0,float r1,int seg,uint32_t col){V3 d=vnorm(vsub(b,a)),up=fabsf(d.y)<.9f?P3(0,1,0):P3(1,0,0),u=vnorm(vcross(d,up)),v=vcross(d,u);int base=bnv;
 for(int s=0;s<=seg;s++){float th=6.2831853f*s/seg,c=cosf(th),sn=sinf(th);V3 n=P3(c*u.x+sn*v.x,c*u.y+sn*v.y,c*u.z+sn*v.z);
  av(a.x+n.x*r0,a.y+n.y*r0,a.z+n.z*r0,n.x,n.y,n.z,0,0,col);av(b.x+n.x*r1,b.y+n.y*r1,b.z+n.z*r1,n.x,n.y,n.z,0,0,col);}
 for(int s=0;s<seg;s++){int b0=base+s*2,t0=b0+1,b1=b0+2,t1=b0+3;ai(b0,b1,t0);ai(b1,t1,t0);}
 int ct=av(b.x,b.y,b.z,d.x,d.y,d.z,0,0,col),cb=av(a.x,a.y,a.z,-d.x,-d.y,-d.z,0,0,col),rt=bnv;
 for(int s=0;s<=seg;s++){float th=6.2831853f*s/seg,c=cosf(th),sn=sinf(th);av(b.x+(c*u.x+sn*v.x)*r1,b.y+(c*u.y+sn*v.y)*r1,b.z+(c*u.z+sn*v.z)*r1,d.x,d.y,d.z,0,0,col);}
 int rb=bnv;for(int s=0;s<=seg;s++){float th=6.2831853f*s/seg,c=cosf(th),sn=sinf(th);av(a.x+(c*u.x+sn*v.x)*r0,a.y+(c*u.y+sn*v.y)*r0,a.z+(c*u.z+sn*v.z)*r0,-d.x,-d.y,-d.z,0,0,col);}
 for(int s=0;s<seg;s++){ai(ct,rt+s,rt+s+1);ai(cb,rb+s+1,rb+s);}}
void mb_remap_uv(int from,float u0,float v0,float u1,float v1){for(int i=from;i<bnv;i++){bv[i].u=u0+bv[i].u*(u1-u0);bv[i].v=v0+bv[i].v*(v1-v0);}}
void mb_fixuv(Mesh*m,float uw,float vw){for(int i=0;i<m->nv;i++)if(m->v[i].u==0&&m->v[i].v==0){m->v[i].u=uw;m->v[i].v=vw;}}
