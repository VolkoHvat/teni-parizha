/* Отрисовка на видеокарте (OpenGL 3.3 core).
   Кадр: каскадные тени → предпроход глубины → SSAO → освещение (PBR, точечные огни, мокрые поверхности)
   → небо с облаками → вода с отражениями → прозрачное → лучи от солнца → блум → тональная компрессия ACES → FXAA → интерфейс. */
#include "r3d.h"
#include "gl33.h"
#include "gl_alias.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GLX_DEF(r,n,a) PFN_##n p_##n;
GL_FUNCS(GLX_DEF)
#undef GLX_DEF
const char* gl_load(void*(*get)(const char*)){
#define GLX_LD(r,n,a) p_##n=(PFN_##n)get(#n);if(!p_##n)return #n;
 GL_FUNCS(GLX_LD)
#undef GLX_LD
 return 0;}
#define GL_SRGB8_ALPHA8 0x8C43
static char r3err[2048];const char* r3_error(void){return r3err;}
/* ---------- шейдеры ---------- */
static const char*SH_COMMON=
"#version 330 core\n"
"uniform vec3 uZen,uHor,uGnd,uSunDir,uSunCol,uEye;uniform float uNight,uCloud,uTime,uFogD,uFogH,uRain;\n"
"float hash12(vec2 p){vec3 p3=fract(vec3(p.xyx)*.1031);p3+=dot(p3,p3.yzx+33.33);return fract((p3.x+p3.y)*p3.z);}\n"
"float vnoise(vec2 p){vec2 i=floor(p),f=fract(p);vec2 u=f*f*(3.-2.*f);return mix(mix(hash12(i),hash12(i+vec2(1,0)),u.x),mix(hash12(i+vec2(0,1)),hash12(i+vec2(1,1)),u.x),u.y);}\n"
"float fbm(vec2 p){float s=0.,a=.5;for(int i=0;i<5;i++){s+=a*vnoise(p);p=p*2.03+vec2(1.7,9.2);a*=.5;}return s;}\n"
"float hash13(vec3 p3){p3=fract(p3*.1031);p3+=dot(p3,p3.zyx+31.32);return fract((p3.x+p3.y)*p3.z);}\n"
"float vnoise3(vec3 p){vec3 i=floor(p),f=fract(p);vec3 u=f*f*(3.-2.*f);\n"
" return mix(mix(mix(hash13(i),hash13(i+vec3(1,0,0)),u.x),mix(hash13(i+vec3(0,1,0)),hash13(i+vec3(1,1,0)),u.x),u.y),mix(mix(hash13(i+vec3(0,0,1)),hash13(i+vec3(1,0,1)),u.x),mix(hash13(i+vec3(0,1,1)),hash13(i+vec3(1,1,1)),u.x),u.y),u.z);}\n"
/* небо без диска и облаков — для тумана и отражений */
"vec3 skyCol(vec3 d){float h=clamp(d.y,0.,1.);vec3 base=mix(uHor,uZen,pow(h,.45));\n"
" float mu=max(dot(d,uSunDir),0.);base+=uSunCol*(.10*pow(mu,6.)+.35*pow(mu,48.))*(1.-.6*uCloud);\n"
" vec3 gray=vec3(dot(base,vec3(.3,.5,.2)));base=mix(base,gray*.9,uCloud*.55);\n"
" if(d.y<0.){base=mix(base,uGnd*.6+uHor*.25,clamp(-d.y*3.,0.,1.));}return base;}\n"
"vec3 applyFog(vec3 c,vec3 w){vec3 v=w-uEye;float dist=length(v);vec3 d=v/max(dist,.001);\n"
" float hf=exp(-max(w.y,-2.)*uFogH);float f=1.-exp(-dist*uFogD*(.35+.65*hf));\n"
" vec3 fc=skyCol(vec3(d.x,max(d.y,.02),d.z));float mu=max(dot(d,uSunDir),0.);fc+=uSunCol*pow(mu,10.)*.25;return mix(c,fc,clamp(f,0.,1.));}\n"
"vec3 aces(vec3 x){return clamp((x*(2.51*x+.03))/(x*(2.43*x+.59)+.14),0.,1.);}\n";
static const char*VS_MAIN=
"layout(location=0) in vec3 aP;layout(location=1) in vec3 aN;layout(location=2) in vec2 aT;layout(location=3) in vec4 aC;\n"
"uniform mat4 uVP,uM;uniform int uFlags;uniform float uWind;\n"
"out vec3 vW;out vec3 vN;out vec2 vT;out vec3 vC;out vec3 vL;invariant gl_Position;\n"
"void main(){vec4 w=uM*vec4(aP,1.);\n"
" if((uFlags&1024)!=0){float h=max(aP.y-1.,0.);float s=sin(uTime*1.6+w.x*.31+w.z*.23)*.05+sin(uTime*3.3+w.x*.9+w.z*.7)*.02;w.xz+=vec2(s,s*.6)*h*(.4+uWind);}\n"
" vW=w.xyz;vN=mat3(uM)*aN;vT=aT;vC=pow(aC.zyx,vec3(2.2));vL=aP;gl_Position=uVP*w;}\n";
static const char*FS_MAIN=
"in vec3 vW;in vec3 vN;in vec2 vT;in vec3 vC;in vec3 vL;\n"
"uniform sampler2D tAlb,tNrm,tAO,tSCol,tSDep;uniform sampler2DArrayShadow tSh;\n"
"uniform int uFlags,uHasTex,uTexMode,uHasNrm,uShadowOn,uAOOn,uDirect,uNumL,uNCasc;uniform vec3 uTint;uniform float uAlpha,uEmis,uWet,uExposure,uBump;\n"
"uniform mat4 uShVP[3];uniform vec4 uSplits;uniform vec3 uCamFwd;uniform vec2 uInvRes;uniform vec4 uVPOff;uniform mat4 uVP;uniform vec4 uLights[64];uniform float uShTexel[3];\n"
"out vec4 oC;\n"
"float shadowAt(vec3 w,vec3 n,float vd){if(uShadowOn==0)return 1.;int ci=-1;for(int i=0;i<3;i++){if(i<uNCasc&&vd<uSplits[i]){ci=i;break;}}if(ci<0)return 1.;\n"
" float tx=uShTexel[ci];vec4 p=uShVP[ci]*vec4(w+n*tx*1.6,1.);vec3 q=p.xyz/p.w*.5+.5;if(q.x<0.||q.y<0.||q.x>1.||q.y>1.||q.z>1.)return 1.;\n"
" float s=0.;float o=1./textureSize(tSh,0).x;q.z-=.0007*(1.+float(ci));\n"
" for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++)s+=texture(tSh,vec4(q.xy+vec2(x,y)*o*1.25,float(ci),q.z));s/=9.;\n"
" float edge=uSplits[ci];float fade=smoothstep(edge*.85,edge,vd);if(ci==uNCasc-1)return mix(s,1.,fade);return s;}\n"
"float D_GGX(float nh,float a){float a2=a*a;float d=nh*nh*(a2-1.)+1.;return a2/(3.14159*d*d);}\n"
"float G_S(float nv,float nl,float r){float k=(r+1.)*(r+1.)/8.;return nv/(nv*(1.-k)+k)*nl/(nl*(1.-k)+k);}\n"
"vec3 F_S(float vh,vec3 f0){return f0+(1.-f0)*pow(1.-vh,5.);}\n"
"vec3 waterN(vec3 w){vec2 p=w.xz;float t=uTime;vec2 g=vec2(0.);\n"
" vec2 d1=normalize(vec2(1.,.3)),d2=normalize(vec2(-.4,1.)),d3=normalize(vec2(.7,-.8)),d4=normalize(vec2(-1.,-.2));\n"
" g+=d1*cos(dot(d1,p)*.9+t*1.3)*.06;g+=d2*cos(dot(d2,p)*1.7+t*1.9)*.04;g+=d3*cos(dot(d3,p)*3.1+t*2.7)*.025;g+=d4*cos(dot(d4,p)*5.3+t*3.6)*.015;\n"
" float e=.15;float n0=fbm(p*.8+vec2(t*.15,t*.07));float nx=fbm((p+vec2(e,0.))*.8+vec2(t*.15,t*.07)),nz=fbm((p+vec2(0.,e))*.8+vec2(t*.15,t*.07));\n"
" g+=vec2(n0-nx,n0-nz)/e*.05;if(uRain>0.){vec2 c=floor(p*2.);float r=hash12(c+floor(t*2.));vec2 f=fract(p*2.)-.5;float dd=length(f);g+=f*sin(dd*40.-t*20.)*exp(-dd*8.)*uRain*.3*step(.6,r);}\n"
" return normalize(vec3(-g.x,1.,-g.y));}\n"
"vec3 ssr(vec3 w,vec3 r,out float hit){hit=0.;float t=.6;for(int i=0;i<28;i++){vec3 p=w+r*t;vec4 c=uVP*vec4(p,1.);if(c.w<.1)break;vec3 q=c.xyz/c.w;vec2 uv=q.xy*.5+.5;\n"
" uv=uVPOff.xy+uv*uVPOff.zw;if(uv.x<0.||uv.y<0.||uv.x>1.||uv.y>1.)break;float sd=texture(tSDep,uv).r;float pd=q.z*.5+.5;\n"
" if(pd>sd&&pd-sd<.004*(1.+t*.05)&&sd<.99999){hit=smoothstep(1.,.85,max(abs(q.x),abs(q.y)))*smoothstep(0.,.1,uv.y);return texture(tSCol,uv).rgb;}t*=1.17;}return vec3(0.);}\n"
"void main(){vec4 tx=vec4(1.);if(uHasTex!=0)tx=texture(tAlb,vT);\n"
" if((uFlags&512)!=0&&tx.a<.45)discard;\n"
" vec3 alb=tx.rgb*vC*uTint;\n"
" vec3 V=normalize(uEye-vW);float vd=length(uEye-vW);\n"
" if((uFlags&2)!=0){vec3 c=alb*(1.4+uEmis*1.2);if((uFlags&2048)==0)c=applyFog(c,vW);if(uDirect!=0)c=pow(aces(c*uExposure),vec3(1./2.2));oC=vec4(c,uAlpha);return;}\n"
" vec3 N=normalize(vN);if(!gl_FrontFacing)N=-N;\n"
" float rough=.82;float em=0.;\n"
" if(uHasTex!=0&&uTexMode==0)em=tx.a;\n"
" if(uHasNrm!=0&&uHasTex!=0){vec4 nm=texture(tNrm,vT);vec3 tn=vec3(nm.xy*2.-1.,0.);tn.xy*=uBump;tn.z=sqrt(max(1.-dot(tn.xy,tn.xy),.05));rough=nm.a;\n"
"  vec3 dp1=dFdx(vW),dp2=dFdy(vW);vec2 du1=dFdx(vT),du2=dFdy(vT);vec3 dp2p=cross(dp2,N),dp1p=cross(N,dp1);vec3 T=dp2p*du1.x+dp1p*du2.x;vec3 B=dp2p*du1.y+dp1p*du2.y;\n"
"  float im=inversesqrt(max(dot(T,T),dot(B,B))+1e-12);vec3 Np=normalize(mat3(T*im,-B*im,N)*tn);if(dot(T,T)>1e-14)N=Np;}\n"
" if((uFlags&64)!=0){float n1=vnoise3(vL*90.),n2=vnoise3(vL*90.+17.);N=normalize(N+(vec3(n1,n2,n1-n2)-.5)*.12);rough=.9;}\n"
" if((uFlags&4096)!=0)rough=.35;\n"
" vec3 f0=vec3(.04);\n"
/* вода */
" float waterHit=0.;vec3 waterRefl=vec3(0.);\n"
" if((uFlags&32)!=0){N=waterN(vW);rough=.05;alb=vec3(.02,.05,.06);}\n"
/* мокрые поверхности и лужи */
" if(uWet>0.&&(uFlags&32)==0){float up=smoothstep(.55,.95,N.y);float por=up*.8+.2;alb*=1.-.35*uWet*por;rough=mix(rough,.12,uWet*por*.85);\n"
"  if(up>.5&&vW.y<.3){float pm=smoothstep(.55,.7,fbm(vW.xz*.35))*uWet;rough=mix(rough,.03,pm);N=normalize(mix(N,vec3(0,1,0),pm));alb*=1.-.25*pm;}}\n"
" float nv=max(dot(N,V),.001);\n"
" vec3 L=uSunDir;float nl=dot(N,L);\n"
" float sh=shadowAt(vW,normalize(vN),dot(vW-uEye,uCamFwd));\n"
" vec3 diff;if((uFlags&256)!=0){float w=max((nl+.45)/1.45,0.);diff=uSunCol*w*mix(vec3(1.),vec3(1.,.55,.45),.35*(1.-max(nl,0.)));}else diff=uSunCol*max(nl,0.);\n"
" if((uFlags&1024)!=0)diff=uSunCol*(max(nl,0.)*.7+max(-nl,0.)*.35);\n"
" vec3 H=normalize(L+V);float nh=max(dot(N,H),0.),vh=max(dot(V,H),0.);float nlc=max(nl,0.);float a=max(rough*rough,.002);\n"
" vec3 F=F_S(vh,f0);vec3 spec=D_GGX(nh,a)*G_S(nv,nlc,rough)*F/(4.*nv*max(nlc,.001)+.0001)*nlc*uSunCol;\n"
" if((uFlags&64)!=0)spec+=uSunCol*pow(1.-nv,4.)*nlc*.15*alb;\n"
" float ao=1.;if(uAOOn!=0){vec2 suv=gl_FragCoord.xy*uInvRes;ao=texture(tAO,suv).r;}\n"
" vec3 amb=mix(uGnd,mix(uHor,uZen,.5)*1.05,N.y*.5+.5);\n"
" vec3 col=alb*(diff*sh)+spec*sh;\n"
" vec3 kd=1.-F_S(nv,f0);col+=alb*amb*ao*kd;\n"
" vec3 R=reflect(-V,N);vec3 env=skyCol(normalize(vec3(R.x,max(R.y,.02),R.z)));float horiz=clamp(1.+dot(R,normalize(vN)),0.,1.);\n"
" vec3 Fr=F_S(nv,f0);float gloss=pow(1.-rough,2.5);\n"
" if((uFlags&32)!=0){float hit;vec3 sc=ssr(vW,R,hit);env=mix(env,sc,hit);vec3 Fw=f0+(1.-f0)*pow(1.-nv,5.);col=alb*amb*.6+env*mix(Fw,vec3(1.),.0)*1.;col=mix(alb*(amb*.5+diff*.15),env,clamp(Fw.r*1.4+.08,0.,1.));col+=uSunCol*pow(max(dot(R,L),0.),400.)*4.*sh;}\n"
" else{float wetr=uWet*smoothstep(.6,.95,N.y);col+=env*Fr*gloss*horiz*ao*(.6+.4*sh)*(1.+wetr);}\n"
/* точечные огни: фонари */
" for(int i=0;i<uNumL;i++){vec4 lp=uLights[i*2],lc=uLights[i*2+1];vec3 d=lp.xyz-vW;float dl=length(d);if(dl>lp.w)continue;vec3 ld=d/dl;float att=pow(clamp(1.-dl/lp.w,0.,1.),2.)/(1.+dl*dl*.25);\n"
"  float ln=max(dot(N,ld),0.);if((uFlags&1024)!=0)ln=abs(dot(N,ld))*.8+.2;vec3 hh=normalize(ld+V);float sp=D_GGX(max(dot(N,hh),0.),max(a,.05))*.25;col+=lc.rgb*lc.w*att*(alb*ln+sp*ln);}\n"
" col+=alb*em*uEmis*5.;\n"
" if((uFlags&2048)==0)col=applyFog(col,vW);\n"
" if(uDirect!=0)col=pow(aces(col*uExposure),vec3(1./2.2));\n"
" oC=vec4(col,uAlpha);}\n";
static const char*VS_DEPTH=
"#version 330 core\nlayout(location=0) in vec3 aP;layout(location=2) in vec2 aT;uniform mat4 uVP,uM;uniform int uFlags;uniform float uTime,uWind;out vec2 vT;invariant gl_Position;\n"
"void main(){vec4 w=uM*vec4(aP,1.);if((uFlags&1024)!=0){float h=max(aP.y-1.,0.);float s=sin(uTime*1.6+w.x*.31+w.z*.23)*.05+sin(uTime*3.3+w.x*.9+w.z*.7)*.02;w.xz+=vec2(s,s*.6)*h*(.4+uWind);}vT=aT;gl_Position=uVP*w;}\n";
static const char*FS_DEPTH="#version 330 core\nin vec2 vT;uniform sampler2D tAlb;uniform int uCut;void main(){if(uCut!=0&&texture(tAlb,vT).a<.45)discard;}\n";
static const char*VS_FULL="#version 330 core\nout vec2 vUV;void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);vUV=p;gl_Position=vec4(p*2.-1.,1.,1.);}\n";
static const char*VS_RECT="#version 330 core\nuniform vec4 uRect;out vec2 vUV;void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2)*.5;p=vec2(gl_VertexID==1||gl_VertexID==2||gl_VertexID==4?1.:0.,gl_VertexID>=2&&gl_VertexID<=4?1.:0.);\n"
" vUV=uRect.xy+p*uRect.zw;gl_Position=vec4((uRect.xy+p*uRect.zw)*2.-1.,0.,1.);}\n";
static const char*FS_SKY=
"in vec2 vUV;uniform mat4 uInvVP;uniform vec3 uMoon;uniform float uExposure;uniform int uDirect;out vec4 oC;\n"
"void main(){vec4 a=uInvVP*vec4(vUV*2.-1.,-1.,1.),b=uInvVP*vec4(vUV*2.-1.,1.,1.);vec3 d=normalize(b.xyz/b.w-a.xyz/a.w);\n"
" vec3 c=skyCol(d);float mu=dot(d,uSunDir);\n"
" c+=uSunCol*smoothstep(.99955,.9998,mu)*30.*(1.-uCloud*.8);\n"
" if(uNight>.05&&d.y>0.){vec3 sd=floor(d*260.);float s=hash13(sd);float tw=.6+.4*sin(uTime*3.+s*40.);c+=vec3(.9,.95,1.)*step(.9975,s)*tw*uNight*2.5*(1.-uCloud);\n"
"  float mm=dot(d,normalize(uMoon));c+=vec3(.85,.9,1.)*smoothstep(.9993,.9996,mm)*3.*uNight;c+=vec3(.2,.25,.4)*pow(max(mm,0.),60.)*uNight;}\n"
" if(d.y>0.){vec2 p=d.xz/(d.y+.08)*1.6+vec2(uTime*.012,uTime*.004);float cv=fbm(p)*.6+fbm(p*2.7+3.)*.4;float cov=mix(.38,.95,uCloud);\n"
"  float den=smoothstep(1.-cov,1.-cov+.32,cv)*smoothstep(0.,.15,d.y);\n"
"  float lit=clamp(.55+.6*(fbm(p+uSunDir.xz*.06)-cv)*3.,0.,1.);vec3 cc=mix(uHor*.75+uZen*.15,uSunCol*.55+uHor*.45,lit)*mix(1.,.55,uCloud);cc+=uSunCol*pow(max(mu,0.),12.)*.45*(1.-den*.5);\n"
"  cc=mix(cc,cc*vec3(.35,.38,.48),uNight);c=mix(c,cc,den*(.85-.25*uNight));}\n"
" if(uDirect!=0)c=pow(aces(c*uExposure),vec3(1./2.2));oC=vec4(c,1.);}\n";
static const char*FS_SSAO=
"#version 330 core\nin vec2 vUV;uniform sampler2D tDep;uniform mat4 uInvVP,uVP;uniform vec2 uTex;uniform vec4 uVPOff;out vec4 oC;\n"
"vec3 wpos(vec2 uv){float z=texture(tDep,uv).r;vec2 n=(uv-uVPOff.xy)/uVPOff.zw;vec4 p=uInvVP*vec4(n*2.-1.,z*2.-1.,1.);return p.xyz/p.w;}\n"
"float h(vec2 p){return fract(sin(dot(p,vec2(12.9898,78.233)))*43758.5453);}\n"
"void main(){float z=texture(tDep,vUV).r;if(z>=.99999){oC=vec4(1.);return;}vec3 P=wpos(vUV);\n"
" vec3 px1=wpos(vUV+vec2(uTex.x,0.)),px2=wpos(vUV-vec2(uTex.x,0.)),py1=wpos(vUV+vec2(0.,uTex.y)),py2=wpos(vUV-vec2(0.,uTex.y));\n"
" vec3 dx=length(px1-P)<length(P-px2)?px1-P:P-px2;vec3 dy=length(py1-P)<length(P-py2)?py1-P:P-py2;vec3 N=normalize(cross(dx,dy));\n"
" vec3 cam=(uInvVP*vec4(0,0,-1,1)).xyz;\n"
" float occ=0.;float a0=h(gl_FragCoord.xy)*6.283;vec3 T=normalize(abs(N.y)<.9?cross(N,vec3(0,1,0)):cross(N,vec3(1,0,0)));vec3 B=cross(N,T);\n"
" for(int i=0;i<12;i++){float fi=float(i)+h(gl_FragCoord.xy+float(i));float ang=a0+fi*2.399;float r=sqrt((fi+.5)/12.);float zz=sqrt(1.-r*r);\n"
"  float rad=.55*(.3+.7*fract(fi*.618));vec3 s=P+(T*cos(ang)*r+B*sin(ang)*r+N*zz)*rad;vec4 c=uVP*vec4(s,1.);vec2 uv=c.xy/c.w*.5+.5;uv=uVPOff.xy+uv*uVPOff.zw;\n"
"  float sz=texture(tDep,uv).r;vec3 sp=wpos(uv);float dd=length(sp-P);float sd=c.z/c.w*.5+.5;if(sz<sd-.00002)occ+=smoothstep(1.2,0.,dd);}\n"
" oC=vec4(clamp(1.-occ/12.*1.25,0.,1.));}\n";
static const char*FS_BLUR=
"#version 330 core\nin vec2 vUV;uniform sampler2D tSrc,tDep;uniform vec2 uDir;out vec4 oC;\n"
"void main(){float z0=texture(tDep,vUV).r;float s=0.,w=0.;for(int i=-3;i<=3;i++){vec2 uv=vUV+uDir*float(i);float z=texture(tDep,uv).r;float wt=exp(-float(i*i)*.18)*(1./(1.+abs(z-z0)*4000.));s+=texture(tSrc,uv).r*wt;w+=wt;}oC=vec4(s/w);}\n";
static const char*FS_DOWN=
"#version 330 core\nin vec2 vUV;uniform sampler2D tSrc;uniform vec2 uTex;uniform int uFirst;uniform float uThresh;out vec4 oC;\n"
"vec3 s(vec2 o){return texture(tSrc,vUV+o*uTex).rgb;}\n"
"void main(){vec3 a=s(vec2(-2,-2)),b=s(vec2(0,-2)),c=s(vec2(2,-2)),d=s(vec2(-1,-1)),e=s(vec2(1,-1)),f=s(vec2(-2,0)),g=s(vec2(0,0)),h=s(vec2(2,0)),i=s(vec2(-1,1)),j=s(vec2(1,1)),k=s(vec2(-2,2)),l=s(vec2(0,2)),m=s(vec2(2,2));\n"
" vec3 r=(d+e+i+j)*.125+(a+b+f+g)*.03125+(b+c+g+h)*.03125+(f+g+k+l)*.03125+(g+h+l+m)*.03125;\n"
" if(uFirst!=0){r=min(r,vec3(60.));float br=max(r.r,max(r.g,r.b));float sk=clamp(br-uThresh+.5,0.,1.);sk=sk*sk*.5;float ct=max(sk,br-uThresh)/max(br,1e-4);r*=ct;}\n"
" oC=vec4(r,1.);}\n";
static const char*FS_UP=
"#version 330 core\nin vec2 vUV;uniform sampler2D tSrc;uniform vec2 uTex;out vec4 oC;\n"
"void main(){vec3 r=vec3(0.);for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++){float w=(x==0?2.:1.)*(y==0?2.:1.);r+=texture(tSrc,vUV+vec2(x,y)*uTex).rgb*w;}oC=vec4(r/16.,1.);}\n";
static const char*FS_GRMASK=
"#version 330 core\nin vec2 vUV;uniform sampler2D tDep,tCol;uniform vec2 uSun;uniform float uAsp;out vec4 oC;\n"
"void main(){float z=texture(tDep,vUV).r;float sky=step(.99999,z);vec2 d=(vUV-uSun);d.x*=uAsp;float fall=exp(-dot(d,d)*7.);vec3 c=texture(tCol,vUV).rgb;float l=min(dot(c,vec3(.3,.5,.2)),4.);oC=vec4(sky*fall*(.4+l*.6));}\n";
static const char*FS_GRBLUR=
"#version 330 core\nin vec2 vUV;uniform sampler2D tSrc;uniform vec2 uSun;out vec4 oC;\n"
"void main(){vec2 d=(uSun-vUV)/48.;vec2 uv=vUV;float s=0.,w=1.;for(int i=0;i<48;i++){s+=texture(tSrc,uv).r*w;w*=.965;uv+=d;}oC=vec4(s/18.);}\n";
static const char*FS_COMP=
"#version 330 core\nin vec2 vUV;uniform sampler2D tHdr,tBloom,tGR;uniform float uExposure,uBloomK,uGRK,uTime,uSat,uVig;uniform vec3 uGRCol,uLift,uGain;uniform int uGR;out vec4 oC;\n"
"vec3 aces(vec3 x){return clamp((x*(2.51*x+.03))/(x*(2.43*x+.59)+.14),0.,1.);}\n"
"void main(){vec3 c=texture(tHdr,vUV).rgb;c+=texture(tBloom,vUV).rgb*uBloomK;if(uGR!=0)c+=texture(tGR,vUV).r*uGRCol*uGRK;\n"
" c*=uExposure;c=aces(c);c=c*uGain+uLift*(1.-c);float l=dot(c,vec3(.2126,.7152,.0722));c=mix(vec3(l),c,uSat);\n"
" vec2 q=vUV-.5;c*=1.-uVig*dot(q,q)*1.6;c=pow(clamp(c,0.,1.),vec3(1./2.2));\n"
" float n=fract(sin(dot(vUV*1000.+uTime,vec2(12.9898,78.233)))*43758.5453);c+=(n-.5)/255.*1.5;\n"
" oC=vec4(c,dot(c,vec3(.299,.587,.114)));}\n";
static const char*FS_FXAA=
"#version 330 core\nin vec2 vUV;uniform sampler2D tSrc;uniform vec2 uTex;out vec4 oC;\n"
"void main(){vec3 rgbNW=texture(tSrc,vUV+vec2(-1,-1)*uTex).rgb,rgbNE=texture(tSrc,vUV+vec2(1,-1)*uTex).rgb,rgbSW=texture(tSrc,vUV+vec2(-1,1)*uTex).rgb,rgbSE=texture(tSrc,vUV+vec2(1,1)*uTex).rgb;vec4 M=texture(tSrc,vUV);\n"
" vec3 lu=vec3(.299,.587,.114);float lNW=dot(rgbNW,lu),lNE=dot(rgbNE,lu),lSW=dot(rgbSW,lu),lSE=dot(rgbSE,lu),lM=M.a;\n"
" float mn=min(lM,min(min(lNW,lNE),min(lSW,lSE))),mx=max(lM,max(max(lNW,lNE),max(lSW,lSE)));\n"
" vec2 dir=vec2(-((lNW+lNE)-(lSW+lSE)),((lNW+lSW)-(lNE+lSE)));float red=max((lNW+lNE+lSW+lSE)*.03125,1./128.);float rc=1./(min(abs(dir.x),abs(dir.y))+red);\n"
" dir=clamp(dir*rc,vec2(-8.),vec2(8.))*uTex;vec3 A=.5*(texture(tSrc,vUV+dir*(1./3.-.5)).rgb+texture(tSrc,vUV+dir*(2./3.-.5)).rgb);\n"
" vec3 B=A*.5+.25*(texture(tSrc,vUV+dir*-.5).rgb+texture(tSrc,vUV+dir*.5).rgb);float lB=dot(B,lu);oC=vec4((lB<mn||lB>mx)?A:B,1.);}\n";
static const char*FS_HUD="#version 330 core\nin vec2 vUV;uniform sampler2D tSrc;uniform vec4 uSrc;out vec4 oC;void main(){vec2 uv=uSrc.xy+(vUV-uSrc.xy);oC=texture(tSrc,vec2(vUV.x,1.-vUV.y));}\n";
static const char*FS_PAINT=
"#version 330 core\nin vec2 vUV;uniform sampler2D tSrc;uniform vec2 uTex;uniform vec4 uR;out vec4 oC;\n"
"float h(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}\n"
"void main(){vec2 uv=vUV;const int R=4;vec3 m[4];vec3 s[4];for(int k=0;k<4;k++){m[k]=vec3(0.);s[k]=vec3(0.);}\n"
" for(int j=0;j<=R;j++)for(int i=0;i<=R;i++){vec3 c0=texture(tSrc,uv+vec2(-i,-j)*uTex).rgb,c1=texture(tSrc,uv+vec2(i,-j)*uTex).rgb,c2=texture(tSrc,uv+vec2(-i,j)*uTex).rgb,c3=texture(tSrc,uv+vec2(i,j)*uTex).rgb;\n"
"  m[0]+=c0;s[0]+=c0*c0;m[1]+=c1;s[1]+=c1*c1;m[2]+=c2;s[2]+=c2*c2;m[3]+=c3;s[3]+=c3*c3;}\n"
" float n=float((R+1)*(R+1));float best=1e9;vec3 col=vec3(0.);for(int k=0;k<4;k++){vec3 mm=m[k]/n;vec3 v=abs(s[k]/n-mm*mm);float sg=v.r+v.g+v.b;if(sg<best){best=sg;col=mm;}}\n"
" vec2 rp=uv;float canvas=(sin(rp.x*700.)*.5+.5)*(sin(rp.y*700.)*.5+.5);col*=.95+.05*canvas;col+=(h(floor(uv/uTex/2.))-.5)*.025;\n"
" col=mix(col,col*vec3(1.05,.98,.86),.5);vec2 q=rp-.5;col*=1.-dot(q,q)*1.2;oC=vec4(col,1.);}\n";
/* ---------- состояние ---------- */
enum{P_MAIN,P_DEPTH,P_SKY,P_SSAO,P_BLUR,P_DOWN,P_UP,P_GRM,P_GRB,P_COMP,P_FXAA,P_HUD,P_PAINT,P_N};
static GLuint prog[P_N];
static GLuint vao_full,vao_stream,vbo_stream,ibo_stream;
static int win_w=1280,win_h=720,vx0,vy0,vw=1280,vh=720,rw,rh,cur_scale_q=-1;
typedef struct{GLuint fbo,tex,dep;int w,h;}Target;
static Target t_hdr,t_copy,t_ao,t_ao2,t_ldr,t_gr1,t_gr2,t_bloom[7],t_paint;static int nbloom;
static GLuint sh_tex,sh_fbo;static int sh_size=0,sh_casc=0;
static GLuint hud_tex;static int hud_w,hud_h;
static GLuint white_tex,flatn_tex;static GLuint win_fbo;void r3_set_target_fbo(unsigned f){win_fbo=f;}
static int has_aniso;static float max_aniso=1;
/* очередь отрисовки */
typedef struct{const Mesh*m;Mat M;const Tex*tex;int flags;float alpha,tint[3];int bv,bi,ni,temp;float dist,c[3],r;}DC;
static DC*dc;static int ndc,cdc;static Vtx*sv;static uint32_t*si;static int nsv,nsi,csv,csi;
static float cam_fovy=1.f,cam_asp=16.f/9,cam_n=.3f,cam_f=300.f;static V3 cam_eye,cam_at;static int cam_set;
static int stat_draws,stat_tris;
void r3_stats(int*d,int*t){*d=stat_draws;*t=stat_tris;}
/* ---------- утилиты ---------- */
static GLuint compile(GLenum type,const char*a,const char*b,const char*name){GLuint s=glCreateShader(type);const char*src[2]={a,b?b:""};glShaderSource(s,2,src,NULL);glCompileShader(s);GLint ok;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
 if(!ok){char log[1500];glGetShaderInfoLog(s,sizeof log,NULL,log);snprintf(r3err,sizeof r3err,"шейдер %s: %s",name,log);return 0;}return s;}
static GLuint link2(const char*vs1,const char*vs2,const char*fs1,const char*fs2,const char*name){GLuint v=compile(GL_VERTEX_SHADER,vs1,vs2,name),f=compile(GL_FRAGMENT_SHADER,fs1,fs2,name);if(!v||!f)return 0;
 GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){char log[1500];glGetProgramInfoLog(p,sizeof log,NULL,log);snprintf(r3err,sizeof r3err,"связка %s: %s",name,log);return 0;}
 glDeleteShader(v);glDeleteShader(f);return p;}
static GLint U(GLuint p,const char*n){return glGetUniformLocation(p,n);}
static void uM4(GLint l,const Mat*m){glUniformMatrix4fv(l,1,GL_TRUE,m->m);}
static void target_free(Target*t){if(t->fbo)glDeleteFramebuffers(1,&t->fbo);if(t->tex)glDeleteTextures(1,&t->tex);if(t->dep)glDeleteTextures(1,&t->dep);memset(t,0,sizeof *t);}
static void target_make(Target*t,int w,int h,GLenum fmt,GLenum ext,GLenum type,int depth,int linear){target_free(t);t->w=w;t->h=h;glGenFramebuffers(1,&t->fbo);glBindFramebuffer(GL_FRAMEBUFFER,t->fbo);
 glGenTextures(1,&t->tex);glBindTexture(GL_TEXTURE_2D,t->tex);glTexImage2D(GL_TEXTURE_2D,0,(GLint)fmt,w,h,0,ext,type,NULL);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,linear?GL_LINEAR:GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,linear?GL_LINEAR:GL_NEAREST);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
 glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,t->tex,0);
 if(depth){glGenTextures(1,&t->dep);glBindTexture(GL_TEXTURE_2D,t->dep);glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,w,h,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_INT,NULL);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,t->dep,0);}
 glBindFramebuffer(GL_FRAMEBUFFER,0);}
static void bind_target(Target*t){glBindFramebuffer(GL_FRAMEBUFFER,t?t->fbo:win_fbo);if(t)glViewport(0,0,t->w,t->h);}
static void full(void){glBindVertexArray(vao_full);glDrawArrays(GL_TRIANGLES,0,3);}
static void tex_bind(int unit,GLuint t){glActiveTexture(GL_TEXTURE0+unit);glBindTexture(GL_TEXTURE_2D,t);}
/* ---------- настройки качества ---------- */
typedef struct{float scale;int casc,smap,ssao,gr,bloom;float split[3];}Qual;
static const Qual QL[4]={{.75f,1,1024,0,0,1,{38,0,0}},{1.f,2,2048,0,0,1,{18,70,0}},{1.f,3,2048,1,1,1,{14,45,130}},{1.25f,3,3072,1,1,1,{12,40,130}}};
static void ensure_targets(void){int q=gfx_quality<0?0:gfx_quality>3?3:gfx_quality;const Qual*Q=&QL[q];int w=(int)(vw*Q->scale+.5f),h=(int)(vh*Q->scale+.5f);if(w<16)w=16;if(h<16)h=16;
 if(w!=rw||h!=rh||cur_scale_q!=q){rw=w;rh=h;cur_scale_q=q;
  target_make(&t_hdr,w,h,GL_RGBA16F,GL_RGBA,GL_FLOAT,1,1);target_make(&t_copy,w,h,GL_RGBA16F,GL_RGBA,GL_FLOAT,1,1);
  target_make(&t_ao,w/2,h/2,GL_R8,GL_RED,GL_UNSIGNED_BYTE,0,1);target_make(&t_ao2,w/2,h/2,GL_R8,GL_RED,GL_UNSIGNED_BYTE,0,1);
  target_make(&t_ldr,w,h,GL_RGBA8,GL_RGBA,GL_UNSIGNED_BYTE,0,1);target_make(&t_gr1,w/2,h/2,GL_R16F,GL_RED,GL_FLOAT,0,1);target_make(&t_gr2,w/2,h/2,GL_R16F,GL_RED,GL_FLOAT,0,1);
  nbloom=0;int bw=w/2,bh=h/2;while(nbloom<7&&bw>=8&&bh>=8){target_make(&t_bloom[nbloom],bw,bh,GL_RGBA16F,GL_RGBA,GL_FLOAT,0,1);nbloom++;bw/=2;bh/=2;}}
 if(sh_size!=Q->smap||sh_casc!=Q->casc){if(sh_tex)glDeleteTextures(1,&sh_tex);if(!sh_fbo)glGenFramebuffers(1,&sh_fbo);sh_size=Q->smap;sh_casc=Q->casc;
  glGenTextures(1,&sh_tex);glBindTexture(GL_TEXTURE_2D_ARRAY,sh_tex);glTexImage3D(GL_TEXTURE_2D_ARRAY,0,GL_DEPTH_COMPONENT24,sh_size,sh_size,3,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_INT,NULL);
  glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_COMPARE_MODE,GL_COMPARE_REF_TO_TEXTURE);glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_COMPARE_FUNC,GL_LEQUAL);}}
/* ---------- инициализация ---------- */
static GLint u_main[40];enum{UM_VP,UM_M,UM_FLAGS,UM_HASTEX,UM_TEXMODE,UM_HASNRM,UM_TINT,UM_ALPHA,UM_BUMP,UM_N};
int r3_init(void){r3err[0]=0;
 prog[P_MAIN]=link2(SH_COMMON,VS_MAIN,SH_COMMON,FS_MAIN,"main");prog[P_DEPTH]=link2(VS_DEPTH,NULL,FS_DEPTH,NULL,"depth");prog[P_SKY]=link2(VS_FULL,NULL,SH_COMMON,FS_SKY,"sky");
 prog[P_SSAO]=link2(VS_FULL,NULL,FS_SSAO,NULL,"ssao");prog[P_BLUR]=link2(VS_FULL,NULL,FS_BLUR,NULL,"blur");prog[P_DOWN]=link2(VS_FULL,NULL,FS_DOWN,NULL,"down");prog[P_UP]=link2(VS_FULL,NULL,FS_UP,NULL,"up");
 prog[P_GRM]=link2(VS_FULL,NULL,FS_GRMASK,NULL,"grmask");prog[P_GRB]=link2(VS_FULL,NULL,FS_GRBLUR,NULL,"grblur");prog[P_COMP]=link2(VS_FULL,NULL,FS_COMP,NULL,"comp");prog[P_FXAA]=link2(VS_FULL,NULL,FS_FXAA,NULL,"fxaa");
 prog[P_HUD]=link2(VS_RECT,NULL,FS_HUD,NULL,"hud");prog[P_PAINT]=link2(VS_RECT,NULL,FS_PAINT,NULL,"paint");
 for(int i=0;i<P_N;i++)if(!prog[i])return -1;
 GLuint p=prog[P_MAIN];u_main[UM_VP]=U(p,"uVP");u_main[UM_M]=U(p,"uM");u_main[UM_FLAGS]=U(p,"uFlags");u_main[UM_HASTEX]=U(p,"uHasTex");u_main[UM_TEXMODE]=U(p,"uTexMode");u_main[UM_HASNRM]=U(p,"uHasNrm");u_main[UM_TINT]=U(p,"uTint");u_main[UM_ALPHA]=U(p,"uAlpha");u_main[UM_BUMP]=U(p,"uBump");
 glGenVertexArrays(1,&vao_full);glGenVertexArrays(1,&vao_stream);glGenBuffers(1,&vbo_stream);glGenBuffers(1,&ibo_stream);
 glBindVertexArray(vao_stream);glBindBuffer(GL_ARRAY_BUFFER,vbo_stream);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ibo_stream);
 glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vtx),(void*)0);glEnableVertexAttribArray(1);glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vtx),(void*)12);
 glEnableVertexAttribArray(2);glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vtx),(void*)24);glEnableVertexAttribArray(3);glVertexAttribPointer(3,4,GL_UNSIGNED_BYTE,GL_TRUE,sizeof(Vtx),(void*)32);glBindVertexArray(0);
 {uint32_t w=0xFFFFFFFFu,n=0xC88080FFu;glGenTextures(1,&white_tex);glBindTexture(GL_TEXTURE_2D,white_tex);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_BGRA,GL_UNSIGNED_BYTE,&w);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glGenTextures(1,&flatn_tex);glBindTexture(GL_TEXTURE_2D,flatn_tex);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_BGRA,GL_UNSIGNED_BYTE,&n);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);}
 while(glGetError()!=GL_NO_ERROR){}glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,&max_aniso);has_aniso=glGetError()==GL_NO_ERROR&&max_aniso>1;if(max_aniso>8)max_aniso=8;
 glGenTextures(1,&hud_tex);glGenFramebuffers(1,&sh_fbo);
 r3e.exposure=1;r3e.cloud=.3f;r3e.fog_dens=.004f;r3e.fog_height=.05f;
 return 0;}
/* ---------- кадр ---------- */
void r3_frame_begin(int w,int h){win_w=w<1?1:w;win_h=h<1?1:h;float sc=(float)win_w/SW;if((float)win_h/SH<sc)sc=(float)win_h/SH;vw=(int)(SW*sc);vh=(int)(SH*sc);vx0=(win_w-vw)/2;vy0=(win_h-vh)/2;
 glBindFramebuffer(GL_FRAMEBUFFER,win_fbo);glViewport(0,0,win_w,win_h);glDisable(GL_SCISSOR_TEST);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);stat_draws=stat_tris=0;}
static void hud_composite(void){if(hud_y1<hud_y0)return;int y0=hud_y0<0?0:hud_y0,y1=hud_y1>=fbh?fbh-1:hud_y1;if(y1<y0)return;
 glBindTexture(GL_TEXTURE_2D,hud_tex);if(hud_w!=fbw||hud_h!=fbh){hud_w=fbw;hud_h=fbh;glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,fbw,fbh,0,GL_BGRA,GL_UNSIGNED_BYTE,NULL);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);}
 glPixelStorei(GL_UNPACK_ALIGNMENT,4);glTexSubImage2D(GL_TEXTURE_2D,0,0,y0,fbw,y1-y0+1,GL_BGRA,GL_UNSIGNED_BYTE,&fb[(size_t)y0*fbw]);
 glBindFramebuffer(GL_FRAMEBUFFER,win_fbo);glViewport(vx0,vy0,vw,vh);glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
 glUseProgram(prog[P_HUD]);float ry0=1.f-(float)(y1+1)/fbh,ry1=1.f-(float)y0/fbh;glUniform4f(U(prog[P_HUD],"uRect"),0,ry0,1,ry1-ry0);glUniform1i(U(prog[P_HUD],"tSrc"),0);tex_bind(0,hud_tex);
 glBindVertexArray(vao_full);glDrawArrays(GL_TRIANGLES,0,6);glDisable(GL_BLEND);}
void r3_frame_end(void){hud_composite();}
/* ---------- запись команд ---------- */
static void mesh_upload(Mesh*m){if(m->gl_vao||m->nv==0)return;glGenVertexArrays(1,&m->gl_vao);glGenBuffers(1,&m->gl_vbo);glGenBuffers(1,&m->gl_ibo);glBindVertexArray(m->gl_vao);
 glBindBuffer(GL_ARRAY_BUFFER,m->gl_vbo);glBufferData(GL_ARRAY_BUFFER,(GLsizeiptr)sizeof(Vtx)*m->nv,m->v,GL_STATIC_DRAW);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m->gl_ibo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,(GLsizeiptr)sizeof(uint32_t)*m->ni,m->i,GL_STATIC_DRAW);
 glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vtx),(void*)0);glEnableVertexAttribArray(1);glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vtx),(void*)12);
 glEnableVertexAttribArray(2);glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vtx),(void*)24);glEnableVertexAttribArray(3);glVertexAttribPointer(3,4,GL_UNSIGNED_BYTE,GL_TRUE,sizeof(Vtx),(void*)32);glBindVertexArray(0);}
static void mesh_bounds(const Mesh*m,float*c,float*r){float mn[3]={1e30f,1e30f,1e30f},mx[3]={-1e30f,-1e30f,-1e30f};for(int i=0;i<m->nv;i++){const float*p=&m->v[i].x;for(int k=0;k<3;k++){if(p[k]<mn[k])mn[k]=p[k];if(p[k]>mx[k])mx[k]=p[k];}}
 if(m->nv==0){c[0]=c[1]=c[2]=0;*r=0;return;}for(int k=0;k<3;k++)c[k]=(mn[k]+mx[k])*.5f;*r=.5f*sqrtf((mx[0]-mn[0])*(mx[0]-mn[0])+(mx[1]-mn[1])*(mx[1]-mn[1])+(mx[2]-mn[2])*(mx[2]-mn[2]));}
void r3_mesh_free(Mesh*m){if(m->gl_vao){glDeleteVertexArrays(1,&m->gl_vao);glDeleteBuffers(1,&m->gl_vbo);glDeleteBuffers(1,&m->gl_ibo);}m->gl_vao=m->gl_vbo=m->gl_ibo=0;}
void r3_tex_dirty(Tex*t){t->ver++;}
void r3_tex_free(Tex*t){if(t->gl)glDeleteTextures(1,&t->gl);if(t->gl_n)glDeleteTextures(1,&t->gl_n);t->gl=t->gl_n=0;t->glver=-1;}
void r3_draw(const Mesh*m,Mat M,const Tex*tex,int flags,int alpha){if(!m||m->nv==0||m->ni==0)return;if(ndc>=cdc){cdc=cdc?cdc*2:4096;dc=realloc(dc,sizeof(DC)*(size_t)cdc);}
 DC*d=&dc[ndc];memset(d,0,sizeof *d);d->M=M;d->tex=tex;d->flags=flags;d->alpha=(flags&(R3_ALPHA|R3_ADD))?alpha/255.f:1.f;for(int k=0;k<3;k++)d->tint[k]=r3_tint[k];
 float lc[3],lr;
 if(m->temp){if(nsv+m->nv>csv){while(nsv+m->nv>csv)csv=csv?csv*2:262144;sv=realloc(sv,sizeof(Vtx)*(size_t)csv);}if(nsi+m->ni>csi){while(nsi+m->ni>csi)csi=csi?csi*2:524288;si=realloc(si,sizeof(uint32_t)*(size_t)csi);}
  memcpy(&sv[nsv],m->v,sizeof(Vtx)*(size_t)m->nv);memcpy(&si[nsi],m->i,sizeof(uint32_t)*(size_t)m->ni);d->temp=1;d->bv=nsv;d->bi=nsi;d->ni=m->ni;nsv+=m->nv;nsi+=m->ni;d->m=NULL;mesh_bounds(m,lc,&lr);}
 else{Mesh*mm=(Mesh*)m;if(!mm->bounded){mesh_bounds(mm,mm->bc,&mm->br);mm->bounded=1;}d->m=m;d->ni=m->ni;lc[0]=m->bc[0];lc[1]=m->bc[1];lc[2]=m->bc[2];lr=m->br;}
 const float*a=M.m;d->c[0]=a[0]*lc[0]+a[1]*lc[1]+a[2]*lc[2]+a[3];d->c[1]=a[4]*lc[0]+a[5]*lc[1]+a[6]*lc[2]+a[7];d->c[2]=a[8]*lc[0]+a[9]*lc[1]+a[10]*lc[2]+a[11];
 float s0=sqrtf(a[0]*a[0]+a[4]*a[4]+a[8]*a[8]),s1=sqrtf(a[1]*a[1]+a[5]*a[5]+a[9]*a[9]),s2=sqrtf(a[2]*a[2]+a[6]*a[6]+a[10]*a[10]);float sm=s0>s1?s0:s1;if(s2>sm)sm=s2;d->r=lr*sm;
 ndc++;}
void r3_outline(const Mesh*m,Mat model,float thick,uint32_t col){(void)m;(void)model;(void)thick;(void)col;}
void r3_camera(V3 eye,V3 at,float fovy,float asp,float n,float f){cam_eye=eye;cam_at=at;cam_fovy=fovy;cam_asp=asp;cam_n=n;cam_f=f;cam_set=1;r3_vp=m_mul(m_persp(fovy,asp,n,f),m_look(eye,at));r3e.eye=eye;}
static int need_depth_clear;void r3_clear(void){need_depth_clear=1;}
/* ---------- текстуры ---------- */
static void tex_ensure(const Tex*tc){Tex*t=(Tex*)tc;if(t->gl&&t->glver==t->ver)return;size_t n=(size_t)t->w*t->h;uint32_t*buf=malloc(n*4);
 for(size_t i=0;i<n;i++){uint32_t a=t->em?(t->em[i]&255):(t->cutout||t->aem)?(t->px[i]>>24):0;buf[i]=(t->px[i]&0xFFFFFF)|(a<<24);}
 if(!t->gl)glGenTextures(1,&t->gl);glBindTexture(GL_TEXTURE_2D,t->gl);glPixelStorei(GL_UNPACK_ALIGNMENT,4);glTexImage2D(GL_TEXTURE_2D,0,GL_SRGB8_ALPHA8,t->w,t->h,0,GL_BGRA,GL_UNSIGNED_BYTE,buf);
 int mip=t->mip>0;glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,mip?GL_LINEAR_MIPMAP_LINEAR:GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,t->wrap?GL_REPEAT:GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,t->wrap?GL_REPEAT:GL_CLAMP_TO_EDGE);
 if(mip){glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,t->mip>1?t->mip:12);glGenerateMipmap(GL_TEXTURE_2D);if(has_aniso)glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,max_aniso);}
 if(t->nmap){/* карта нормалей из рельефа (или яркости), в альфе — шероховатость */
  float*hg=malloc(n*sizeof(float));for(size_t i=0;i<n;i++){if(t->hgt)hg[i]=t->hgt[i]/255.f;else{uint32_t p=t->px[i];hg[i]=(((p>>16)&255)*.3f+((p>>8)&255)*.55f+(p&255)*.15f)/255.f;}}
  int W=t->w,H=t->h;float bs=t->bump>0?t->bump:2.f;
  for(int y=0;y<H;y++)for(int x=0;x<W;x++){int xm=x>0?x-1:x,xp=x<W-1?x+1:x,ym=y>0?y-1:y,yp=y<H-1?y+1:y;
   float dx=(hg[y*W+xp]-hg[y*W+xm])*bs,dy=(hg[yp*W+x]-hg[ym*W+x])*bs;float l=sqrtf(dx*dx+dy*dy+1);float nx=-dx/l,ny=dy/l,nz=1/l;
   int R=(int)((nx*.5f+.5f)*255),G=(int)((ny*.5f+.5f)*255),B=(int)((nz*.5f+.5f)*255),A=t->rgh?t->rgh[(size_t)y*W+x]:200;buf[(size_t)y*W+x]=((uint32_t)A<<24)|((uint32_t)R<<16)|((uint32_t)G<<8)|(uint32_t)B;}
  free(hg);if(!t->gl_n)glGenTextures(1,&t->gl_n);glBindTexture(GL_TEXTURE_2D,t->gl_n);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,W,H,0,GL_BGRA,GL_UNSIGNED_BYTE,buf);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,t->wrap?GL_REPEAT:GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,t->wrap?GL_REPEAT:GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,t->mip>1?t->mip:12);glGenerateMipmap(GL_TEXTURE_2D);if(has_aniso)glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,max_aniso);}
 free(buf);t->glver=t->ver;}
/* ---------- рисование списка ---------- */
static Mat cur_vp;
static void set_common(GLuint p,int direct){glUseProgram(p);
 glUniform3f(U(p,"uZen"),r3e.sky_zen[0],r3e.sky_zen[1],r3e.sky_zen[2]);glUniform3f(U(p,"uHor"),r3e.sky_hor[0],r3e.sky_hor[1],r3e.sky_hor[2]);glUniform3f(U(p,"uGnd"),r3e.ground[0],r3e.ground[1],r3e.ground[2]);
 glUniform3f(U(p,"uSunDir"),r3e.L.x,r3e.L.y,r3e.L.z);glUniform3f(U(p,"uSunCol"),r3e.sun[0],r3e.sun[1],r3e.sun[2]);glUniform3f(U(p,"uEye"),r3e.eye.x,r3e.eye.y,r3e.eye.z);
 glUniform1f(U(p,"uNight"),r3e.night);glUniform1f(U(p,"uCloud"),r3e.cloud);glUniform1f(U(p,"uTime"),r3e.time);glUniform1f(U(p,"uFogD"),r3e.fog_dens);glUniform1f(U(p,"uFogH"),r3e.fog_height);glUniform1f(U(p,"uRain"),r3e.rain);
 glUniform1f(U(p,"uExposure"),r3e.exposure>0?r3e.exposure:1);glUniform1i(U(p,"uDirect"),direct);}
static int cull_main(const DC*d){const float*m=cur_vp.m;float cx=m[0]*d->c[0]+m[1]*d->c[1]+m[2]*d->c[2]+m[3],cy=m[4]*d->c[0]+m[5]*d->c[1]+m[6]*d->c[2]+m[7],cw=m[12]*d->c[0]+m[13]*d->c[1]+m[14]*d->c[2]+m[15];float r=d->r;
 if(cw+r<cam_n*.5f)return 1;if(cw<r)return 0;float ax=fabsf(m[0])+fabsf(m[1])+fabsf(m[2]),ay=fabsf(m[4])+fabsf(m[5])+fabsf(m[6]);return fabsf(cx)>cw+r*ax*1.05f||fabsf(cy)>cw+r*ay*1.05f;}
static void draw_one(const DC*d){if(d->temp){glBindVertexArray(vao_stream);glDrawElementsBaseVertex(GL_TRIANGLES,d->ni,GL_UNSIGNED_INT,(void*)((size_t)d->bi*4),d->bv);}
 else{Mesh*m=(Mesh*)d->m;if(!m->gl_vao)mesh_upload(m);glBindVertexArray(m->gl_vao);glDrawElements(GL_TRIANGLES,d->ni,GL_UNSIGNED_INT,(void*)0);}stat_draws++;stat_tris+=d->ni/3;}
static const Tex*bound_tex;static int bound_flags=-1;
static void main_state(const DC*d){if(d->flags!=bound_flags){glUniform1i(u_main[UM_FLAGS],d->flags);bound_flags=d->flags;if(d->flags&R3_TWOSIDED||d->flags&R3_CUTOUT)glDisable(GL_CULL_FACE);else glEnable(GL_CULL_FACE);}
 if(d->tex!=bound_tex){if(d->tex){tex_ensure(d->tex);tex_bind(0,d->tex->gl);tex_bind(1,d->tex->gl_n?d->tex->gl_n:flatn_tex);glUniform1i(u_main[UM_HASTEX],1);glUniform1i(u_main[UM_TEXMODE],(d->tex->em||d->tex->aem)?0:d->tex->cutout?1:2);glUniform1i(u_main[UM_HASNRM],d->tex->gl_n!=0);glUniform1f(u_main[UM_BUMP],1.f);}
  else{tex_bind(0,white_tex);tex_bind(1,flatn_tex);glUniform1i(u_main[UM_HASTEX],0);glUniform1i(u_main[UM_HASNRM],0);}bound_tex=d->tex;}
 uM4(u_main[UM_M],&d->M);glUniform3f(u_main[UM_TINT],d->tint[0],d->tint[1],d->tint[2]);glUniform1f(u_main[UM_ALPHA],d->alpha);}
static int*ord;static int cord;
static void sort_by(int*ix,int n,int asc){/* сортировка вставками по дальности: списки почти упорядочены от кадра к кадру */for(int i=1;i<n;i++){int v=ix[i];float dv=dc[v].dist;int j=i-1;while(j>=0&&(asc?dc[ix[j]].dist>dv:dc[ix[j]].dist<dv)){ix[j+1]=ix[j];j--;}ix[j+1]=v;}}
static void upload_stream(void){glBindVertexArray(vao_stream);glBindBuffer(GL_ARRAY_BUFFER,vbo_stream);glBufferData(GL_ARRAY_BUFFER,(GLsizeiptr)sizeof(Vtx)*(nsv?nsv:1),NULL,GL_STREAM_DRAW);if(nsv)glBufferSubData(GL_ARRAY_BUFFER,0,(GLsizeiptr)sizeof(Vtx)*nsv,sv);
 glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ibo_stream);glBufferData(GL_ELEMENT_ARRAY_BUFFER,(GLsizeiptr)sizeof(uint32_t)*(nsi?nsi:1),NULL,GL_STREAM_DRAW);if(nsi)glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,0,(GLsizeiptr)sizeof(uint32_t)*nsi,si);glBindVertexArray(0);}
static int is_opaque(const DC*d){return !(d->flags&(R3_ALPHA|R3_ADD|R3_WATER));}
static int casts(const DC*d){return is_opaque(d)&&!(d->flags&(R3_NOSHADOW|R3_NOLIGHT));}
static Mat ortho(float l,float r,float b,float t,float n,float f){Mat m=m_ident();m.m[0]=2/(r-l);m.m[5]=2/(t-b);m.m[10]=-2/(f-n);m.m[3]=-(r+l)/(r-l);m.m[7]=-(t+b)/(t-b);m.m[11]=-(f+n)/(f-n);return m;}
static Mat shvp[3];static float shtexel[3];
static void shadow_pass(const Qual*Q){V3 L=r3e.L;float ll=sqrtf(L.x*L.x+L.y*L.y+L.z*L.z);if(ll<1e-4f)return;L.x/=ll;L.y/=ll;L.z/=ll;
 V3 f={cam_at.x-cam_eye.x,cam_at.y-cam_eye.y,cam_at.z-cam_eye.z};float fl=sqrtf(f.x*f.x+f.y*f.y+f.z*f.z);f.x/=fl;f.y/=fl;f.z/=fl;float th=tanf(cam_fovy*.5f),tw=th*cam_asp;float diag=sqrtf(1+th*th+tw*tw);
 GLuint p=prog[P_DEPTH];glUseProgram(p);GLint uvp=U(p,"uVP"),um=U(p,"uM"),ucut=U(p,"uCut"),ufl=U(p,"uFlags");glUniform1f(U(p,"uTime"),r3e.time);glUniform1f(U(p,"uWind"),r3e.wind);glUniform1i(U(p,"tAlb"),0);
 glBindFramebuffer(GL_FRAMEBUFFER,sh_fbo);glDrawBuffer(0);glReadBuffer(0);glViewport(0,0,sh_size,sh_size);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LESS);glDepthMask(GL_TRUE);glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(1.6f,2.f);glDisable(GL_CULL_FACE);
 float prev=cam_n;
 for(int c=0;c<Q->casc;c++){float d0=prev,d1=Q->split[c];prev=d1;float mid=(d0+d1)*.5f;V3 cen={cam_eye.x+f.x*mid,cam_eye.y+f.y*mid,cam_eye.z+f.z*mid};float rad=sqrtf(((d1-d0)*.5f)*((d1-d0)*.5f)+(d1*th*diag)*(d1*th*diag))*1.02f;
  rad=ceilf(rad);float texel=2*rad/sh_size;shtexel[c]=texel;
  V3 leye={L.x*400,L.y*400,L.z*400},org={0,0,0};Mat LV=m_look(leye,org);
  float lx=LV.m[0]*cen.x+LV.m[1]*cen.y+LV.m[2]*cen.z+LV.m[3],ly=LV.m[4]*cen.x+LV.m[5]*cen.y+LV.m[6]*cen.z+LV.m[7],lz=LV.m[8]*cen.x+LV.m[9]*cen.y+LV.m[10]*cen.z+LV.m[11];
  lx=floorf(lx/texel)*texel;ly=floorf(ly/texel)*texel;
  /* фиксированная ориентация света + привязка к текселям — тени не «плывут» при движении камеры */
  Mat LP=ortho(lx-rad,lx+rad,ly-rad,ly+rad,-lz-320.f,-lz+320.f);
  shvp[c]=m_mul(LP,LV);
  glFramebufferTextureLayer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,sh_tex,0,c);glClear(GL_DEPTH_BUFFER_BIT);uM4(uvp,&shvp[c]);
  int lastcut=-1,lastfl=-1;const Tex*lt=NULL;
  for(int i=0;i<ndc;i++){DC*d=&dc[i];if(!casts(d))continue;
   float vx=LV.m[0]*d->c[0]+LV.m[1]*d->c[1]+LV.m[2]*d->c[2]+LV.m[3],vy=LV.m[4]*d->c[0]+LV.m[5]*d->c[1]+LV.m[6]*d->c[2]+LV.m[7];if(fabsf(vx-lx)>rad+d->r||fabsf(vy-ly)>rad+d->r)continue;
   if(c==2&&d->r<.8f)continue;
   int cut=(d->flags&R3_CUTOUT)&&d->tex;if(cut!=lastcut){glUniform1i(ucut,cut);lastcut=cut;}if(cut&&d->tex!=lt){tex_ensure(d->tex);tex_bind(0,d->tex->gl);lt=d->tex;}
   if(d->flags!=lastfl){glUniform1i(ufl,d->flags);lastfl=d->flags;}uM4(um,&d->M);draw_one(d);}}
 glDisable(GL_POLYGON_OFFSET_FILL);glBindFramebuffer(GL_FRAMEBUFFER,win_fbo);}
static void set_lights(GLuint p){float buf[R3_MAXL*8];int n=r3e.nlights>R3_MAXL?R3_MAXL:r3e.nlights;for(int i=0;i<n;i++){memcpy(&buf[i*8],r3e.light[i],sizeof(float)*8);}
 glUniform1i(U(p,"uNumL"),n);if(n)glUniform4fv(U(p,"uLights"),n*2,buf);}
static void draw_list(int*ix,int n,int depth_only){for(int k=0;k<n;k++){const DC*d=&dc[ix[k]];if(!depth_only)main_state(d);else{uM4(U(prog[P_DEPTH],"uM"),&d->M);}draw_one(d);}}
static void flush_scene(void){const Qual*Q=&QL[cur_scale_q<0?2:cur_scale_q];
 upload_stream();cur_vp=r3_vp;
 if(ndc>cord){cord=ndc*2;ord=realloc(ord,sizeof(int)*(size_t)cord);}
 int nop=0,nal=0,nwa=0;static int*al=NULL,*wa=NULL;static int cal=0;if(ndc>cal){cal=ndc*2;al=realloc(al,sizeof(int)*(size_t)cal);wa=realloc(wa,sizeof(int)*(size_t)cal);}
 for(int i=0;i<ndc;i++){DC*d=&dc[i];float dx=d->c[0]-cam_eye.x,dy=d->c[1]-cam_eye.y,dz=d->c[2]-cam_eye.z;d->dist=sqrtf(dx*dx+dy*dy+dz*dz);if(cull_main(d))continue;
  if(d->flags&R3_WATER)wa[nwa++]=i;else if(is_opaque(d))ord[nop++]=i;else al[nal++]=i;}
 sort_by(ord,nop,1);sort_by(al,nal,0);
 int shadows=r3e.L.y>.02f&&Q->casc>0;if(shadows)shadow_pass(Q);
 bind_target(&t_hdr);glClearColor(0,0,0,1);glDepthMask(GL_TRUE);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);glDisable(GL_BLEND);glEnable(GL_CULL_FACE);glCullFace(GL_BACK);glFrontFace(GL_CCW);
 Mat ivp=m_inv(r3_vp);
 /* предпроход глубины + SSAO */
 int ssao=Q->ssao;
 if(ssao){GLuint p=prog[P_DEPTH];glUseProgram(p);uM4(U(p,"uVP"),&r3_vp);glUniform1f(U(p,"uTime"),r3e.time);glUniform1f(U(p,"uWind"),r3e.wind);glUniform1i(U(p,"tAlb"),0);glColorMask(0,0,0,0);glDepthFunc(GL_LESS);
  GLint um=U(p,"uM"),ucut=U(p,"uCut"),ufl=U(p,"uFlags");int lc=-1,lf=-1;for(int k=0;k<nop;k++){DC*d=&dc[ord[k]];int cut=(d->flags&R3_CUTOUT)&&d->tex;if(cut!=lc){glUniform1i(ucut,cut);lc=cut;}if(cut){tex_ensure(d->tex);tex_bind(0,d->tex->gl);}
   if(d->flags!=lf){glUniform1i(ufl,d->flags);lf=d->flags;if(d->flags&(R3_TWOSIDED|R3_CUTOUT))glDisable(GL_CULL_FACE);else glEnable(GL_CULL_FACE);}uM4(um,&d->M);draw_one(d);}
  glColorMask(1,1,1,1);
  glDisable(GL_DEPTH_TEST);bind_target(&t_ao);p=prog[P_SSAO];glUseProgram(p);uM4(U(p,"uInvVP"),&ivp);uM4(U(p,"uVP"),&r3_vp);glUniform2f(U(p,"uTex"),1.f/rw,1.f/rh);glUniform4f(U(p,"uVPOff"),0,0,1,1);glUniform1i(U(p,"tDep"),0);tex_bind(0,t_hdr.dep);full();
  p=prog[P_BLUR];glUseProgram(p);glUniform1i(U(p,"tSrc"),0);glUniform1i(U(p,"tDep"),1);tex_bind(1,t_hdr.dep);
  bind_target(&t_ao2);glUniform2f(U(p,"uDir"),2.f/rw,0);tex_bind(0,t_ao.tex);full();bind_target(&t_ao);glUniform2f(U(p,"uDir"),0,2.f/rh);tex_bind(0,t_ao2.tex);full();
  bind_target(&t_hdr);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);}
 else glDepthFunc(GL_LESS);
 /* основной проход */
 GLuint p=prog[P_MAIN];set_common(p,0);uM4(u_main[UM_VP],&r3_vp);glUniform1i(U(p,"tAlb"),0);glUniform1i(U(p,"tNrm"),1);glUniform1i(U(p,"tAO"),2);glUniform1i(U(p,"tSh"),3);glUniform1i(U(p,"tSCol"),4);glUniform1i(U(p,"tSDep"),5);
 glUniform1f(U(p,"uEmis"),r3e.emis);glUniform1f(U(p,"uWet"),r3e.wet);glUniform1f(U(p,"uWind"),r3e.wind);glUniform1i(U(p,"uShadowOn"),shadows);glUniform1i(U(p,"uAOOn"),ssao);glUniform1i(U(p,"uNCasc"),Q->casc);
 {Mat sh[3];for(int c=0;c<3;c++){sh[c]=shvp[c];}float tmp[48];for(int c=0;c<3;c++)for(int k=0;k<16;k++)tmp[c*16+k]=sh[c].m[k];glUniformMatrix4fv(U(p,"uShVP"),3,GL_TRUE,tmp);glUniform1fv(U(p,"uShTexel"),3,shtexel);}
 glUniform4f(U(p,"uSplits"),Q->split[0],Q->casc>1?Q->split[1]:0,Q->casc>2?Q->split[2]:0,0);glUniform2f(U(p,"uInvRes"),1.f/rw,1.f/rh);glUniform4f(U(p,"uVPOff"),0,0,1,1);set_lights(p);{float fx=cam_at.x-cam_eye.x,fy=cam_at.y-cam_eye.y,fz=cam_at.z-cam_eye.z,fl=sqrtf(fx*fx+fy*fy+fz*fz)+1e-6f;glUniform3f(U(p,"uCamFwd"),fx/fl,fy/fl,fz/fl);}
 tex_bind(2,ssao?t_ao.tex:white_tex);glActiveTexture(GL_TEXTURE0+3);glBindTexture(GL_TEXTURE_2D_ARRAY,sh_tex);tex_bind(4,t_copy.tex);tex_bind(5,t_copy.dep);
 bound_tex=(const Tex*)1;bound_flags=-1;draw_list(ord,nop,0);
 /* небо там, где ничего нет */
 glDepthFunc(GL_LEQUAL);glDepthMask(GL_FALSE);glDisable(GL_CULL_FACE);{GLuint ps=prog[P_SKY];set_common(ps,0);uM4(U(ps,"uInvVP"),&ivp);glUniform3f(U(ps,"uMoon"),r3e.moon.x,r3e.moon.y,r3e.moon.z);full();}
 glDepthMask(GL_TRUE);glDepthFunc(GL_LESS);
 /* вода: копия кадра для отражений */
 if(nwa){glBindFramebuffer(GL_READ_FRAMEBUFFER,t_hdr.fbo);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,t_copy.fbo);glBlitFramebuffer(0,0,rw,rh,0,0,rw,rh,GL_COLOR_BUFFER_BIT,GL_NEAREST);glBlitFramebuffer(0,0,rw,rh,0,0,rw,rh,GL_DEPTH_BUFFER_BIT,GL_NEAREST);
  bind_target(&t_hdr);glUseProgram(p);tex_bind(4,t_copy.tex);tex_bind(5,t_copy.dep);bound_tex=(const Tex*)1;bound_flags=-1;glEnable(GL_CULL_FACE);draw_list(wa,nwa,0);}
 /* прозрачное */
 if(nal){glUseProgram(p);glEnable(GL_BLEND);glDepthMask(GL_FALSE);bound_tex=(const Tex*)1;bound_flags=-1;
  for(int k=0;k<nal;k++){const DC*d=&dc[al[k]];if(d->flags&R3_ADD)glBlendFunc(GL_SRC_ALPHA,GL_ONE);else glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);main_state(d);draw_one(d);}
  glDisable(GL_BLEND);glDepthMask(GL_TRUE);}
 glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
 /* лучи */
 int gr=0;float sunx=.5f,suny=.5f;
 if(Q->gr&&r3e.L.y>0.f&&r3e.night<.7f){V3 sp={cam_eye.x+r3e.L.x*500,cam_eye.y+r3e.L.y*500,cam_eye.z+r3e.L.z*500};const float*m=r3_vp.m;float cx=m[0]*sp.x+m[1]*sp.y+m[2]*sp.z+m[3],cy=m[4]*sp.x+m[5]*sp.y+m[6]*sp.z+m[7],cw=m[12]*sp.x+m[13]*sp.y+m[14]*sp.z+m[15];
  if(cw>0){sunx=cx/cw*.5f+.5f;suny=cy/cw*.5f+.5f;if(sunx>-.4f&&sunx<1.4f&&suny>-.4f&&suny<1.4f)gr=1;}}
 if(gr){bind_target(&t_gr1);GLuint q=prog[P_GRM];glUseProgram(q);glUniform1i(U(q,"tDep"),0);glUniform1i(U(q,"tCol"),1);tex_bind(0,t_hdr.dep);tex_bind(1,t_hdr.tex);glUniform2f(U(q,"uSun"),sunx,suny);glUniform1f(U(q,"uAsp"),(float)rw/rh);full();
  bind_target(&t_gr2);q=prog[P_GRB];glUseProgram(q);glUniform1i(U(q,"tSrc"),0);tex_bind(0,t_gr1.tex);glUniform2f(U(q,"uSun"),sunx,suny);full();}
 /* блум */
 if(Q->bloom&&nbloom>0){GLuint q=prog[P_DOWN];glUseProgram(q);glUniform1i(U(q,"tSrc"),0);glUniform1f(U(q,"uThresh"),1.1f);
  for(int i=0;i<nbloom;i++){bind_target(&t_bloom[i]);const Target*src=i==0?&t_hdr:&t_bloom[i-1];glUniform2f(U(q,"uTex"),1.f/src->w,1.f/src->h);glUniform1i(U(q,"uFirst"),i==0);tex_bind(0,src->tex);full();}
  q=prog[P_UP];glUseProgram(q);glUniform1i(U(q,"tSrc"),0);glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE);
  for(int i=nbloom-1;i>0;i--){bind_target(&t_bloom[i-1]);glUniform2f(U(q,"uTex"),1.f/t_bloom[i].w,1.f/t_bloom[i].h);tex_bind(0,t_bloom[i].tex);full();}glDisable(GL_BLEND);}
 /* сведение */
 bind_target(&t_ldr);{GLuint q=prog[P_COMP];glUseProgram(q);glUniform1i(U(q,"tHdr"),0);glUniform1i(U(q,"tBloom"),1);glUniform1i(U(q,"tGR"),2);tex_bind(0,t_hdr.tex);tex_bind(1,(Q->bloom&&nbloom)?t_bloom[0].tex:white_tex);tex_bind(2,t_gr2.tex);
  glUniform1f(U(q,"uExposure"),r3e.exposure>0?r3e.exposure:1);glUniform1f(U(q,"uBloomK"),(Q->bloom&&nbloom)?.075f:0);glUniform1f(U(q,"uGRK"),.42f*(1-r3e.cloud*.6f));glUniform1i(U(q,"uGR"),gr);glUniform3f(U(q,"uGRCol"),r3e.sun[0],r3e.sun[1],r3e.sun[2]);
  glUniform1f(U(q,"uTime"),r3e.time);glUniform1f(U(q,"uSat"),1.08f-.25f*r3e.cloud*(1-r3e.night));glUniform1f(U(q,"uVig"),.55f);
  float warm=1-r3e.night;glUniform3f(U(q,"uLift"),.012f*r3e.night,.014f*r3e.night,.03f*r3e.night+.004f);glUniform3f(U(q,"uGain"),1.f+.03f*warm,1.f+.005f*warm,1.f-.04f*warm+.04f*r3e.night);full();}
 /* FXAA в окно */
 glBindFramebuffer(GL_FRAMEBUFFER,win_fbo);glViewport(vx0,vy0,vw,vh);{GLuint q=prog[P_FXAA];glUseProgram(q);glUniform1i(U(q,"tSrc"),0);tex_bind(0,t_ldr.tex);glUniform2f(U(q,"uTex"),1.f/rw,1.f/rh);full();}
 glEnable(GL_DEPTH_TEST);}
/* наложение (портрет в альбоме): прямой вывод в окно в пределах r3_clip */
static void flush_overlay(void){hud_composite();if(hud_y1>=hud_y0){int y0=hud_y0<0?0:hud_y0,y1=hud_y1>=fbh?fbh-1:hud_y1;if(y1>=y0)memset(&fb[(size_t)y0*fbw],0,sizeof(uint32_t)*(size_t)fbw*(y1-y0+1));hud_y0=1<<20;hud_y1=-1;}
 upload_stream();cur_vp=r3_vp;
 float sx=(float)vw/fbw,sy=(float)vh/fbh;int cx0=r3_clip[0],cy0=r3_clip[1],cx1=r3_clip[2]+1,cy1=r3_clip[3]+1;if(cx1>fbw)cx1=fbw;if(cy1>fbh)cy1=fbh;
 glBindFramebuffer(GL_FRAMEBUFFER,win_fbo);glViewport(vx0,vy0,vw,vh);glEnable(GL_SCISSOR_TEST);glScissor(vx0+(int)(cx0*sx),vy0+(int)((fbh-cy1)*sy),(int)((cx1-cx0)*sx+.5f),(int)((cy1-cy0)*sy+.5f));
 glDepthMask(GL_TRUE);glClear(GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LESS);glDisable(GL_BLEND);glEnable(GL_CULL_FACE);
 GLuint p=prog[P_MAIN];set_common(p,1);uM4(u_main[UM_VP],&r3_vp);glUniform1i(U(p,"tAlb"),0);glUniform1i(U(p,"tNrm"),1);glUniform1i(U(p,"tAO"),2);glUniform1i(U(p,"tSh"),3);glUniform1i(U(p,"tSCol"),4);glUniform1i(U(p,"tSDep"),5);
 glUniform1f(U(p,"uEmis"),0);glUniform1f(U(p,"uWet"),0);glUniform1i(U(p,"uShadowOn"),0);glUniform1i(U(p,"uAOOn"),0);glUniform1i(U(p,"uNumL"),0);glUniform1f(U(p,"uFogD"),0);
 tex_bind(2,white_tex);glActiveTexture(GL_TEXTURE0+3);glBindTexture(GL_TEXTURE_2D_ARRAY,sh_tex);tex_bind(4,white_tex);tex_bind(5,white_tex);
 bound_tex=(const Tex*)1;bound_flags=-1;for(int i=0;i<ndc;i++){DC*d=&dc[i];if(d->flags&(R3_ALPHA|R3_ADD))continue;main_state(d);draw_one(d);}
 glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);}
void r3_flush(void){if(!prog[P_MAIN]){ndc=nsv=nsi=0;return;}
 int clipped=r3_clip[0]>0||r3_clip[1]>0||r3_clip[2]<fbw-1||r3_clip[3]<fbh-1;
 if(clipped)flush_overlay();else{ensure_targets();flush_scene();}
 ndc=0;nsv=nsi=0;need_depth_clear=0;}
/* живописная обработка области интерфейса (логические координаты) */
void r3_post_paint(int x,int y,int w,int h){float sx=(float)vw/SW,sy=(float)vh/SH;int X=vx0+(int)(x*sx),Y=vy0+(int)((SH-y-h)*sy),W=(int)(w*sx),H=(int)(h*sy);if(W<4||H<4)return;
 if(t_paint.w!=W||t_paint.h!=H)target_make(&t_paint,W,H,GL_RGBA8,GL_RGBA,GL_UNSIGNED_BYTE,0,1);
 glBindFramebuffer(GL_FRAMEBUFFER,win_fbo);glBindTexture(GL_TEXTURE_2D,t_paint.tex);glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,X,Y,W,H);
 glViewport(X,Y,W,H);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);GLuint p=prog[P_PAINT];glUseProgram(p);glUniform1i(U(p,"tSrc"),0);tex_bind(0,t_paint.tex);glUniform2f(U(p,"uTex"),1.f/W,1.f/H);glUniform4f(U(p,"uRect"),0,0,1,1);glUniform4f(U(p,"uR"),0,0,1,1);
 glBindVertexArray(vao_full);glDrawArrays(GL_TRIANGLES,0,6);glViewport(0,0,win_w,win_h);}
int r3_read_pixels(uint8_t*rgb,int w,int h){glBindFramebuffer(GL_FRAMEBUFFER,win_fbo);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,rgb);return glGetError()==GL_NO_ERROR;}
