/* Lutèce Engine — шейдеры GLSL 3.30 */
#ifndef LU_SHADERS_H
#define LU_SHADERS_H

/* общий блок кадра (std140) */
#define LU_GLSL_FRAME \
"layout(std140) uniform Frame{\n" \
" mat4 uView,uProj,uViewProj,uInvViewProj,uPrevViewProj;\n" \
" mat4 uShadowVP[4];\n" \
" vec4 uCamPos;\n"            /* xyz, w=near */ \
" vec4 uSunDir;\n"            /* xyz к солнцу, w=far */ \
" vec4 uSunColor;\n" \
" vec4 uSkyZen,uSkyHor,uGround;\n" \
" vec4 uFog;\n"               /* rgb, плотность */ \
" vec4 uP1;\n"                /* время, ветер, спад тумана по высоте, экспозиция */ \
" vec4 uP2;\n"                /* ночь, влажность, облачность, сила SSAO */ \
" vec4 uSplits;\n"            /* дальние границы каскадов (в метрах) */ \
" vec4 uShadowP;\n"           /* размер тексела, число каскадов, вкл. тени, вкл. SSAO */ \
" vec4 uScreen;\n"            /* w,h,1/w,1/h */ \
" vec4 uLightN;\n"            /* x = число точечных источников */ \
" vec4 uLights[128];\n"       /* пары: (позиция, радиус), (цвет, -) */ \
"};\n"

static const char *GLSL_HEAD = "#version 330 core\n" LU_GLSL_FRAME;

/* общие функции освещения и тумана */
static const char *GLSL_LIGHTING =
"const float PI=3.14159265;\n"
"vec3 sky_color(vec3 d){float h=d.y;vec3 c=mix(uSkyHor.rgb,uSkyZen.rgb,pow(clamp(h,0.,1.),.55));\n"
" c=mix(c,uGround.rgb,smoothstep(0.,-.18,h));\n"
" float s=max(dot(d,uSunDir.xyz),0.);c+=uSunColor.rgb*(pow(s,8.)*.05+pow(s,64.)*.15)*(1.-uP2.x);\n"
" return c;}\n"
"vec3 apply_fog(vec3 col,vec3 wp){vec3 v=wp-uCamPos.xyz;float d=length(v);float k=uP1.z;\n"
" float h0=uCamPos.y,h1=wp.y;float fh=abs(h1-h0)>.01? (exp(-k*h0)-exp(-k*h1))/(k*(h1-h0)) : exp(-k*h0);\n"
" float f=1.-exp(-uFog.a*d*fh);vec3 vd=v/max(d,1e-3);float sun=pow(max(dot(vd,uSunDir.xyz),0.),6.);\n"
" vec3 fc=uFog.rgb+uSunColor.rgb*sun*.12*(1.-uP2.x);return mix(col,fc,clamp(f,0.,1.));}\n"
"float D_GGX(float nh,float a){float a2=a*a;float d=nh*nh*(a2-1.)+1.;return a2/(PI*d*d+1e-6);}\n"
"float V_SmithGGX(float nv,float nl,float a){float k=a*.5;return .25/((nv*(1.-k)+k)*(nl*(1.-k)+k)+1e-5);}\n"
"vec3 F_Schlick(vec3 f0,float vh){return f0+(1.-f0)*pow(1.-vh,5.);}\n"
"vec3 brdf(vec3 N,vec3 V,vec3 L,vec3 alb,float rough,float metal,out vec3 spec){vec3 H=normalize(V+L);float nl=max(dot(N,L),0.),nv=max(dot(N,V),1e-3),nh=max(dot(N,H),0.),vh=max(dot(V,H),0.);\n"
" float a=max(rough*rough,.003);vec3 f0=mix(vec3(.04),alb,metal);vec3 F=F_Schlick(f0,vh);spec=D_GGX(nh,a)*V_SmithGGX(nv,nl,a)*F*nl;\n"
" return (1.-F)*(1.-metal)*alb/PI*nl;}\n";

/* ---------- основной шейдер: вершины ---------- */
static const char *VS_MAIN =
"layout(location=0)in vec3 aPos;layout(location=1)in vec3 aNrm;layout(location=2)in vec4 aTan;layout(location=3)in vec2 aUV;\n"
"layout(location=4)in vec4 aCol;layout(location=5)in uvec4 aJoint;layout(location=6)in vec4 aWeight;\n"
"layout(location=7)in vec4 iM0;layout(location=8)in vec4 iM1;layout(location=9)in vec4 iM2;layout(location=10)in vec4 iM3;\n"
"layout(location=11)in vec4 iTint;layout(location=12)in float iBone;\n"
"uniform int uSkinned;uniform int uFlags;uniform samplerBuffer tBones;uniform mat4 uShadowM;uniform int uShadowPass;\n"
"out vec3 vWP;out vec3 vN;out vec4 vT;out vec2 vUV;out vec4 vC;out vec4 vTint;\n"
"mat4 bone(int i){int b=int(iBone)+i;return mat4(texelFetch(tBones,b*4),texelFetch(tBones,b*4+1),texelFetch(tBones,b*4+2),texelFetch(tBones,b*4+3));}\n"
"void main(){mat4 M=mat4(iM0,iM1,iM2,iM3);vec4 p=vec4(aPos,1.);vec3 n=aNrm,t=aTan.xyz;\n"
" if(uSkinned!=0){mat4 S=bone(int(aJoint.x))*aWeight.x+bone(int(aJoint.y))*aWeight.y+bone(int(aJoint.z))*aWeight.z+bone(int(aJoint.w))*aWeight.w;\n"
"  p=S*p;n=mat3(S)*n;t=mat3(S)*t;}\n"
" vec4 wp=M*p;\n"
" if((uFlags&8)!=0){float w=uP1.y*aCol.a;float ph=uP1.x*1.7+wp.x*.35+wp.z*.27;wp.xyz+=vec3(sin(ph),0.,cos(ph*1.13))*w*.12+vec3(sin(uP1.x*4.1+wp.y*2.),0.,0.)*w*.03;}\n"
" vWP=wp.xyz;mat3 NM=mat3(M);vN=normalize(NM*n);vT=vec4(normalize(NM*t),aTan.w);vUV=aUV;vC=aCol;vTint=iTint;\n"
" gl_Position=(uShadowPass!=0?uShadowM:uViewProj)*wp;}\n";

/* ---------- основной шейдер: пиксели ---------- */
static const char *FS_MAIN =
"in vec3 vWP;in vec3 vN;in vec4 vT;in vec2 vUV;in vec4 vC;in vec4 vTint;\n"
"uniform sampler2D tAlb,tNrm,tORM,tAO;uniform sampler2DArrayShadow tShadow;\n"
"uniform int uFlags,uHasAlb,uHasNrm,uHasORM;uniform vec4 uColor;uniform vec4 uMat;uniform vec3 uEmis;\n" /* uMat: rough, metal, normal scale, uv scale */
"layout(location=0)out vec4 oC;\n"
"float hash12(vec2 p){vec3 p3=fract(vec3(p.xyx)*.1031);p3+=dot(p3,p3.yzx+33.33);return fract((p3.x+p3.y)*p3.z);}\n"
"float shadow_at(vec3 wp,vec3 N,float vd){if(uShadowP.z<.5)return 1.;int c=0;int nc=int(uShadowP.y);\n"
" for(int i=0;i<3;i++){if(i<nc-1&&vd>uSplits[i])c=i+1;}\n"
" float tex=uShadowP.x*(1.+float(c)*1.5);vec3 off=N*tex*2.5+uSunDir.xyz*tex*1.0;vec4 sp=uShadowVP[c]*vec4(wp+off,1.);vec3 s=sp.xyz/sp.w*.5+.5;\n"
" if(s.x<0.||s.x>1.||s.y<0.||s.y>1.||s.z>1.)return 1.;\n"
" float a=hash12(gl_FragCoord.xy)*6.2831;mat2 R=mat2(cos(a),sin(a),-sin(a),cos(a));float sum=0.;float ts=1./float(textureSize(tShadow,0).x);\n"
" const vec2 P[8]=vec2[](vec2(-.94,-.4),vec2(.95,-.77),vec2(-.09,-.93),vec2(.34,.29),vec2(-.92,.46),vec2(.1,.96),vec2(.7,-.2),vec2(-.4,-.1));\n"
" for(int k=0;k<8;k++){vec2 o=R*P[k]*ts*1.6;sum+=texture(tShadow,vec4(s.xy+o,float(c),s.z-.0006*(1.+float(c))));}\n"
" float sh=sum/8.;\n"
" if(c<nc-1){float fade=smoothstep(uSplits[c]*.85,uSplits[c],vd);if(fade>0.){vec4 sp2=uShadowVP[c+1]*vec4(wp+off*2.5,1.);vec3 s2=sp2.xyz/sp2.w*.5+.5;float sh2=texture(tShadow,vec4(s2.xy,float(c+1),s2.z-.0012));sh=mix(sh,sh2,fade);}}\n"
" return sh;}\n"
"void main(){vec2 uv=vUV*uMat.w;\n"
" vec3 N=normalize(vN);if(!gl_FrontFacing)N=-N;\n"
" vec4 alb=uColor*vTint;if((uFlags&1024)!=0)alb*=vec4(pow(vC.rgb,vec3(2.2)),1.);\n"
" if((uFlags&512)!=0){vec3 an=abs(N);vec2 tuv=an.y>.6?vWP.xz:(an.x>an.z?vWP.zy:vWP.xy);uv=tuv*uMat.w;}\n"
" if(uHasAlb!=0){vec4 t=texture(tAlb,uv);alb*=t;}\n"
" if((uFlags&1)!=0&&alb.a<.45)discard;\n"
" if(uHasNrm!=0){vec3 tn=texture(tNrm,uv).xyz*2.-1.;tn.xy*=uMat.z;vec3 T=normalize(vT.xyz-N*dot(N,vT.xyz));vec3 B=cross(N,T)*vT.w;N=normalize(T*tn.x+B*tn.y+N*tn.z);}\n"
" float ao=1.,rough=uMat.x,metal=uMat.y;if(uHasORM!=0){vec3 o=texture(tORM,uv).rgb;ao=o.r;rough*=o.g;metal*=o.b;}\n"
" float wet=uP2.y*smoothstep(.5,.95,N.y);rough=mix(rough,rough*.35,wet);alb.rgb*=mix(1.,.65,wet);\n"
" vec3 V=normalize(uCamPos.xyz-vWP);float vd=dot(vWP-uCamPos.xyz,-vec3(uView[0][2],uView[1][2],uView[2][2]));\n"
" if((uFlags&32)!=0){vec3 c=alb.rgb+uEmis;oC=vec4(apply_fog(c,vWP),alb.a);return;}\n"
" vec2 suv=gl_FragCoord.xy*uScreen.zw;float sao=uShadowP.w>.5?texture(tAO,suv).r:1.;sao=mix(1.,sao,uP2.w);\n"
" vec3 L=uSunDir.xyz;float sh=shadow_at(vWP,normalize(vN)*(gl_FrontFacing?1.:-1.),vd);vec3 spec;vec3 dif;\n"
" if((uFlags&16)!=0){float w=.45;float nl=dot(N,L);float wr=max((nl+w)/(1.+w),0.);dif=alb.rgb/PI*wr*mix(vec3(1.),vec3(1.,.6,.5),.35*(1.-max(nl,0.)));vec3 dd;brdf(N,V,L,alb.rgb,max(rough,.45),0.,spec);spec*=.5;}\n"
" else dif=brdf(N,V,L,alb.rgb,rough,metal,spec);\n"
" if((uFlags&4)!=0&&dot(N,L)<0.){dif+=alb.rgb/PI*max(-dot(N,L),0.)*.35;}\n"
" vec3 col=(dif+spec)*uSunColor.rgb*sh;\n"
/* окружение: полусферное небо и отражение неба */
" vec3 amb=mix(uGround.rgb,mix(uSkyHor.rgb,uSkyZen.rgb,.5),N.y*.5+.5);vec3 f0=mix(vec3(.04),alb.rgb,metal);float nv=max(dot(N,V),0.);\n"
" vec3 Fr=f0+(max(vec3(1.-rough),f0)-f0)*pow(1.-nv,5.);vec3 R=reflect(-V,N);vec3 env=sky_color(normalize(vec3(R.x,max(R.y,-.2),R.z)));\n"
" float occ=ao*sao;col+=(1.-Fr)*(1.-metal)*alb.rgb*amb*occ+Fr*env*occ*(1.-rough*.85)*mix(.35,1.,sh*.5+.5);\n"
/* точечные источники */
" int nl=int(uLightN.x);for(int i=0;i<nl;i++){vec4 lp=uLights[i*2];vec3 d=lp.xyz-vWP;float dist2=dot(d,d);if(dist2>lp.w*lp.w)continue;float dist=sqrt(dist2);vec3 Ld=d/dist;\n"
"  float att=pow(clamp(1.-dist2/(lp.w*lp.w),0.,1.),2.)/(1.+dist2*.25);vec3 s2;vec3 d2=brdf(N,V,Ld,alb.rgb,rough,metal,s2);col+=(d2+s2)*uLights[i*2+1].rgb*att;}\n"
" col+=uEmis*(uHasAlb!=0?alb.rgb:vec3(1.));\n"
" if((uFlags&128)!=0){col=mix(col,env*.9+spec*uSunColor.rgb*sh,.6);}\n"
" oC=vec4(apply_fog(col,vWP),alb.a);}\n";

/* ---------- только глубина (тени и предварительный проход) ---------- */
static const char *FS_DEPTH =
"in vec3 vWP;in vec3 vN;in vec4 vT;in vec2 vUV;in vec4 vC;in vec4 vTint;\n"
"uniform sampler2D tAlb;uniform int uFlags,uHasAlb;uniform vec4 uColor;uniform vec4 uMat;\n"
"void main(){if((uFlags&1)!=0&&uHasAlb!=0&&texture(tAlb,vUV*uMat.w).a*uColor.a<.45)discard;}\n";

/* ---------- полноэкранный треугольник ---------- */
static const char *VS_FULL =
"out vec2 vUV;void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);vUV=p;gl_Position=vec4(p*2.-1.,0.,1.);}\n";

/* ---------- небо: градиент, солнце, облака, звёзды ---------- */
static const char *FS_SKY =
"in vec2 vUV;out vec4 oC;\n"
"float h21(vec2 p){p=fract(p*vec2(123.34,456.21));p+=dot(p,p+45.32);return fract(p.x*p.y);}\n"
"float vn(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);return mix(mix(h21(i),h21(i+vec2(1,0)),f.x),mix(h21(i+vec2(0,1)),h21(i+vec2(1,1)),f.x),f.y);}\n"
"float fbm(vec2 p){float s=0.,a=.5;for(int i=0;i<5;i++){s+=vn(p)*a;p=p*2.03+vec2(1.7,9.2);a*=.5;}return s;}\n"
"void main(){vec4 c=uInvViewProj*vec4(vUV*2.-1.,1.,1.);vec3 d=normalize(c.xyz/c.w-uCamPos.xyz);\n"
" vec3 col=sky_color(d);float s=max(dot(d,uSunDir.xyz),0.);col+=uSunColor.rgb*smoothstep(.9994,.9997,s)*8.*(1.-uP2.x);\n"
" if(d.y>0.){vec2 cp=d.xz/(d.y+.12)*1.4+vec2(uP1.x*.006,uP1.x*.002);float cl=fbm(cp);float cov=mix(.62,.32,uP2.z);float m=smoothstep(cov,cov+.25,cl)*smoothstep(0.,.25,d.y);\n"
"  vec3 cc=mix(uSkyHor.rgb*1.2,vec3(1.)*max(uSunColor.r,.05)*.55+uSkyZen.rgb*.4,.5)*(.8+.4*fbm(cp*2.+3.));cc+=uSunColor.rgb*pow(s,6.)*.25;col=mix(col,cc,m*.85);\n"
"  if(uP2.x>.3){vec2 sp=d.xz/(d.y+.3)*90.;float st=step(.996,h21(floor(sp)))*smoothstep(.3,1.,uP2.x)*(1.-m);col+=vec3(st)*.6;}}\n"
" col=apply_fog(col,uCamPos.xyz+d*uSunDir.w*.98);oC=vec4(col,1.);}\n";

/* ---------- SSAO по глубине ---------- */
static const char *FS_SSAO =
"in vec2 vUV;out vec4 oC;uniform sampler2D tDepth;uniform mat4 uInvProj;\n"
"vec3 vpos(vec2 uv){float z=texture(tDepth,uv).r*2.-1.;vec4 p=uInvProj*vec4(uv*2.-1.,z,1.);return p.xyz/p.w;}\n"
"float h(vec2 p){return fract(sin(dot(p,vec2(12.9898,78.233)))*43758.5453);}\n"
"void main(){float z=texture(tDepth,vUV).r;if(z>=1.){oC=vec4(1.);return;}vec3 P=vpos(vUV);\n"
" vec3 N=normalize(cross(dFdx(P),dFdy(P)));float r=.5;float occ=0.;float a0=h(gl_FragCoord.xy)*6.283;\n"
" for(int i=0;i<12;i++){float a=a0+float(i)*2.39996;float rr=r*(float(i)+.5)/12.;vec3 dir=vec3(cos(a),sin(a),0.)*rr;\n"
"  vec3 S=P+dir+N*rr*.4;vec4 sp=uProj*vec4(S,1.);vec2 suv=sp.xy/sp.w*.5+.5;if(suv.x<0.||suv.y<0.||suv.x>1.||suv.y>1.)continue;\n"
"  float sz=vpos(suv).z;float dz=sz-S.z;occ+=step(.03,dz)*smoothstep(1.,0.,dz/r*.5);}\n"
" oC=vec4(vec3(pow(1.-occ/12.,1.6)),1.);}\n";
static const char *FS_BLUR =
"in vec2 vUV;out vec4 oC;uniform sampler2D tSrc;uniform sampler2D tDepth;uniform vec2 uDir;\n"
"void main(){float d0=texture(tDepth,vUV).r;float s=0.,w=0.;for(int i=-3;i<=3;i++){vec2 uv=vUV+uDir*float(i);float d=texture(tDepth,uv).r;float k=exp(-abs(d-d0)*3000.)*(1.-abs(float(i))/4.);s+=texture(tSrc,uv).r*k;w+=k;}oC=vec4(vec3(s/max(w,1e-4)),1.);}\n";

/* ---------- блум ---------- */
static const char *FS_BRIGHT =
"in vec2 vUV;out vec4 oC;uniform sampler2D tSrc;uniform vec2 uTexel;\n"
"void main(){vec3 c=vec3(0.);for(int y=-1;y<=1;y+=2)for(int x=-1;x<=1;x+=2)c+=texture(tSrc,vUV+vec2(x,y)*uTexel).rgb;c*=.25*uP1.w;\n"
" float l=max(c.r,max(c.g,c.b));c*=max(l-1.,0.)/max(l,1e-4);oC=vec4(min(c,vec3(30.)),1.);}\n";
static const char *FS_DOWN =
"in vec2 vUV;out vec4 oC;uniform sampler2D tSrc;uniform vec2 uTexel;\n"
"void main(){vec3 c=texture(tSrc,vUV).rgb*.25;c+=texture(tSrc,vUV+vec2(-1,-1)*uTexel).rgb*.1875;c+=texture(tSrc,vUV+vec2(1,-1)*uTexel).rgb*.1875;c+=texture(tSrc,vUV+vec2(-1,1)*uTexel).rgb*.1875;c+=texture(tSrc,vUV+vec2(1,1)*uTexel).rgb*.1875;oC=vec4(c,1.);}\n";
static const char *FS_UP =
"in vec2 vUV;out vec4 oC;uniform sampler2D tSrc;uniform vec2 uTexel;\n"
"void main(){vec3 c=vec3(0.);c+=texture(tSrc,vUV+vec2(-1,0)*uTexel).rgb;c+=texture(tSrc,vUV+vec2(1,0)*uTexel).rgb;c+=texture(tSrc,vUV+vec2(0,-1)*uTexel).rgb;c+=texture(tSrc,vUV+vec2(0,1)*uTexel).rgb;oC=vec4(c*.25,1.);}\n";

/* ---------- итог: экспозиция, ACES, цветокоррекция, виньетка ---------- */
static const char *FS_COMPOSITE =
"in vec2 vUV;out vec4 oC;uniform sampler2D tHDR,tBloom;uniform float uBloomK;\n"
"vec3 aces(vec3 x){const float a=2.51,b=.03,c=2.43,d=.59,e=.14;return clamp((x*(a*x+b))/(x*(c*x+d)+e),0.,1.);}\n"
"void main(){vec3 c=texture(tHDR,vUV).rgb;c+=texture(tBloom,vUV).rgb*uBloomK;c*=uP1.w;\n"
" c=aces(c);float l=dot(c,vec3(.2126,.7152,.0722));c=mix(vec3(l),c,1.08);c=pow(c,vec3(1./2.2));\n"
" vec2 q=vUV-.5;c*=1.-dot(q,q)*.45;c+=(fract(sin(dot(gl_FragCoord.xy,vec2(12.9898,78.233)))*43758.5453)-.5)/255.;\n"
" oC=vec4(c,dot(c,vec3(.299,.587,.114)));}\n";
static const char *FS_FXAA =
"in vec2 vUV;out vec4 oC;uniform sampler2D tSrc;uniform vec2 uTexel;uniform int uOn;\n"
"void main(){vec3 c=texture(tSrc,vUV).rgb;if(uOn==0){oC=vec4(c,1.);return;}\n"
" float lM=texture(tSrc,vUV).a,lNW=texture(tSrc,vUV+vec2(-1,-1)*uTexel).a,lNE=texture(tSrc,vUV+vec2(1,-1)*uTexel).a,lSW=texture(tSrc,vUV+vec2(-1,1)*uTexel).a,lSE=texture(tSrc,vUV+vec2(1,1)*uTexel).a;\n"
" float mn=min(lM,min(min(lNW,lNE),min(lSW,lSE))),mx=max(lM,max(max(lNW,lNE),max(lSW,lSE)));\n"
" vec2 dir=vec2(-((lNW+lNE)-(lSW+lSE)),((lNW+lSW)-(lNE+lSE)));float red=max((lNW+lNE+lSW+lSE)*.03125,1./128.);float rc=1./(min(abs(dir.x),abs(dir.y))+red);\n"
" dir=clamp(dir*rc,-8.,8.)*uTexel;vec3 a=.5*(texture(tSrc,vUV+dir*(1./3.-.5)).rgb+texture(tSrc,vUV+dir*(2./3.-.5)).rgb);\n"
" vec3 b=a*.5+.25*(texture(tSrc,vUV-dir*.5).rgb+texture(tSrc,vUV+dir*.5).rgb);float lb=dot(b,vec3(.299,.587,.114));oC=vec4((lb<mn||lb>mx)?a:b,1.);}\n";

/* ---------- 2D-интерфейс ---------- */
static const char *VS_UI =
"#version 330 core\nlayout(location=0)in vec2 aP;layout(location=1)in vec2 aUV;layout(location=2)in vec4 aC;layout(location=3)in vec2 aM;\n"
"uniform vec2 uInvScreen;out vec2 vUV;out vec4 vC;out vec2 vM;\n"
"void main(){vUV=aUV;vC=aC;vM=aM;gl_Position=vec4(aP.x*uInvScreen.x*2.-1.,1.-aP.y*uInvScreen.y*2.,0.,1.);}\n";
static const char *FS_UI =
"#version 330 core\nin vec2 vUV;in vec4 vC;in vec2 vM;out vec4 oC;uniform sampler2D tTex;\n"
"void main(){int m=int(vM.x+.5);vec4 c=vC;\n"
" if(m==1){c*=texture(tTex,vUV);}\n"
" else if(m==2){float d=texture(tTex,vUV).r;float w=max(fwidth(d),1e-4)*.75;float th=.5-vM.y;c.a*=smoothstep(th-w,th+w,d);}\n"
" else if(m==3){float r=length(vUV*2.-1.);float w=fwidth(r);c.a*=1.-smoothstep(1.-w,1.,r);}\n"
" oC=vec4(c.rgb*c.a,c.a);}\n";

/* ---------- отладочные линии ---------- */
static const char *VS_LINE =
"layout(location=0)in vec3 aP;layout(location=1)in vec4 aC;out vec4 vC;void main(){vC=aC;gl_Position=uViewProj*vec4(aP,1.);}\n";
static const char *FS_LINE = "in vec4 vC;out vec4 oC;void main(){oC=vC;}\n";
#endif
