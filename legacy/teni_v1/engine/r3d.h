/* 3D-слой движка: построитель мешей на процессоре + отрисовка на видеокарте (OpenGL 3.3):
   каскадные тени, HDR, PBR-освещение, SSAO, небо с облаками, вода, туман, блум, лучи, FXAA. */
#ifndef R3D_H
#define R3D_H
#include "engine.h"
typedef struct{float m[16];}Mat;
typedef struct{float x,y,z;}V3;
typedef struct{float x,y,z,nx,ny,nz,u,v;uint32_t c;}Vtx;
typedef struct{Vtx*v;int nv;uint32_t*i;int ni;unsigned gl_vao,gl_vbo,gl_ibo;float bc[3],br;int temp,bounded;}Mesh;
/* текстура: px — 0xAARRGGBB; альфа = свечение (em) или вырез (cutout). rgh/hgt — шероховатость и рельеф для карты нормалей */
typedef struct{int w,h;uint32_t*px;uint32_t*em;int wrap,cutout,nmap,mip,aem;uint8_t*rgh,*hgt;float bump;unsigned gl,gl_n;int ver,glver;}Tex;
#define R3_MAXL 32
typedef struct{V3 L;float sun[3];float amb[3];float sky_zen[3],sky_hor[3],ground[3];float fog[3],fogs,foge,fog_dens,fog_height;V3 eye;float emis,smooth;
 float night,cloud,rain,wet,time,exposure,wind;V3 moon;int nlights;float light[R3_MAXL][8];}R3Env;
extern float r3_tint[3];extern int r3_clip[4];extern R3Env r3e;extern Mat r3_vp;
#define R3_ALPHA 1
#define R3_NOLIGHT 2
#define R3_TWOSIDED 8
#define R3_ADD 16
#define R3_WATER 32
#define R3_CLOTH 64
#define R3_NOSHADOW 128
#define R3_SKIN 256
#define R3_CUTOUT 512
#define R3_WIND 1024
#define R3_NOFOG 2048
#define R3_GLOSSY 4096
int  r3_init(void);
const char* r3_error(void);
void r3_frame_begin(int win_w,int win_h);
void r3_frame_end(void);
void r3_camera(V3 eye,V3 at,float fovy,float asp,float n,float f);
void r3_clear(void);
void r3_flush(void);
void r3_post_paint(int x,int y,int w,int h);
int  r3_visible(V3 c,float r);
int  r3_read_pixels(uint8_t*rgb,int w,int h);
void r3_mesh_free(Mesh*m);
void r3_tex_dirty(Tex*t);
void r3_tex_free(Tex*t);
void r3_stats(int*draws,int*tris);
void r3_set_target_fbo(unsigned fbo);
Mat m_ident(void);Mat m_mul(Mat a,Mat b);Mat m_trans(float x,float y,float z);Mat m_roty(float a);Mat m_rotx(float a);Mat m_rotz(float a);Mat m_scale(float x,float y,float z);
Mat m_look(V3 eye,V3 at);Mat m_persp(float fovy,float asp,float n,float f);Mat m_inv(Mat a);
void r3_draw(const Mesh*m,Mat model,const Tex*tex,int flags,int alpha);
void r3_outline(const Mesh*m,Mat model,float thick,uint32_t col);
int r3_project(V3 p,float*sx,float*sy);
Tex tex_new(int w,int h,int emissive);
void mb_begin(void);
void mb_uvrect(float u0,float v0,float u1,float v1);
void mb_fixuv(Mesh*m,float uw,float vw);
void mb_sphere(V3 c,V3 r,int seg,int ring,uint32_t col,int uvfront);
void mb_lathe(V3 base,const float*pr,int n,int seg,uint32_t col,float sx,float sz);
void mb_tube(V3 a,V3 b,float r0,float r1,int seg,uint32_t col);
void mb_cyl(V3 base,float r0,float r1,float h,int seg,uint32_t col);
void mb_box(V3 c,V3 h,uint32_t col);
void mb_quad(V3 a,V3 b,V3 c,V3 d,V3 n,float u0,float v0,float u1,float v1,uint32_t col);
void mb_tri(V3 a,V3 b,V3 c,uint32_t col);
void mb_vert(float x,float y,float z,float nx,float ny,float nz,float u,float v,uint32_t c);
void mb_index(int a,int b,int c);
int  mb_count(void);
void mb_xform(int from,Mat m);
void mb_remap_uv(int from,float u0,float v0,float u1,float v1);
Mesh mb_temp(void);
Mesh mb_end(void);
#endif
