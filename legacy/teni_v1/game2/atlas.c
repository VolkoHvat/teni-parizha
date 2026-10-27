/* ===== Текстурный атлас 4096×4096: 16×16 плиток по 256 px. Материалы считаются попиксельно:
   цвет, рельеф (для карты нормалей) и шероховатость. Окна, лавки, витражи рисуются векторно поверх. ===== */
#define AT 4096
#define TS 256
#define TSC 4.f
static Tex atlas;static uint8_t*at_h,*at_r;
enum{TL_SLATE=60,TL_ZINC,TL_COBBLE,TL_COBBLE2,TL_SIDEWALK,TL_PLAZA,TL_DIRT,TL_GRASS,TL_PLANKS,TL_HAY,TL_WHITE,TL_BASE,TL_BRICK,TL_CATH,TL_ROOFRED,TL_AWNING,TL_GOTH,TL_GOTHL,TL_ROSE,TL_WOOD,
 TL_SHUTTER,TL_GRAVEL,TL_IRON,TL_DOOR,TL_MARBLE,TL_HEDGE,TL_BARK,TL_CLOTH,TL_SIGN0,TL_SIGN1,TL_SIGN2,TL_SIGN3,TL_SIGN4,TL_SIGN5,TL_COLUMN,TL_PALW,TL_PALWL,TL_DOOR2,TL_QUAY,TL_LEAD,TL_BOAT,TL_SAIL,TL_LANTERN,TL_N};
static uint32_t hs(int a,int b){uint32_t h=(uint32_t)a*374761393u+(uint32_t)b*668265263u;h=(h^(h>>13))*1274126177u;return h^(h>>16);}
static float rf(uint32_t h){return (h&0xFFFF)/65535.f;}
static V3 v3(float x,float y,float z){V3 r={x,y,z};return r;}
static V3 vnrm(V3 a){float l=sqrtf(a.x*a.x+a.y*a.y+a.z*a.z);if(l<1e-6f)l=1;return v3(a.x/l,a.y/l,a.z/l);}
static Mat mtv(V3 p){return m_trans(p.x,p.y,p.z);}
static float lf(float a,float b,float t){return a+(b-a)*t;}
static void tile_uv(int id,float*u0,float*v0,float*u1,float*v1){int tx=id%16,ty=id/16;*u0=(tx*TS+3.f)/AT;*u1=((tx+1)*TS-3.f)/AT;*v0=(ty*TS+3.f)/AT;*v1=((ty+1)*TS-3.f)/AT;}
static const uint32_t WALLC[6]={RGBX(222,210,184),RGBX(178,178,182),RGBX(214,184,136),RGBX(168,98,82),RGBX(190,198,204),RGBX(212,172,152)};
/* шум */
static float vn2(float x,float y,int s){int ix=(int)floorf(x),iy=(int)floorf(y);float fx=x-ix,fy=y-iy;fx=fx*fx*(3-2*fx);fy=fy*fy*(3-2*fy);
 float a=rf(hs(ix+s*131,iy)),b=rf(hs(ix+1+s*131,iy)),c=rf(hs(ix+s*131,iy+1)),d=rf(hs(ix+1+s*131,iy+1));return lf(lf(a,b,fx),lf(c,d,fx),fy);}
static float fbm2(float x,float y,int s,int o){float r=0,a=.5f,n=0;for(int i=0;i<o;i++){r+=a*vn2(x,y,s+i*7);n+=a;x*=2.03f;y*=2.03f;a*=.5f;}return r/n;}
static inline uint32_t rgbf(float r,float g,float b){int R=(int)(r*255),G=(int)(g*255),B=(int)(b*255);R=R<0?0:R>255?255:R;G=G<0?0:G>255?255:G;B=B<0?0:B>255?255:B;return (uint32_t)((R<<16)|(G<<8)|B);}
static inline void cf(uint32_t c,float*r,float*g,float*b){*r=((c>>16)&255)/255.f;*g=((c>>8)&255)/255.f;*b=(c&255)/255.f;}
static inline void put(int id,int x,int y,uint32_t c,float h,int r){int ox=(id%16)*TS,oy=(id/16)*TS;size_t i=(size_t)(oy+y)*AT+ox+x;atlas.px[i]=(atlas.px[i]&0xFF000000u)|(c&0xFFFFFF);at_h[i]=(uint8_t)(h<0?0:h>1?255:h*255);at_r[i]=(uint8_t)r;}
/* отметка областей при векторном рисовании (координаты в «единицах» 64 на плитку) */
static void mark(float x,float y,float w,float h,int rough,int hv){int X0=(int)(x*TSC),Y0=(int)(y*TSC),X1=(int)((x+w)*TSC),Y1=(int)((y+h)*TSC);if(X0<0)X0=0;if(Y0<0)Y0=0;if(X1>AT)X1=AT;if(Y1>AT)Y1=AT;
 for(int yy=Y0;yy<Y1;yy++)for(int xx=X0;xx<X1;xx++){size_t i=(size_t)yy*AT+xx;if(rough>=0)at_r[i]=(uint8_t)rough;if(hv>=0)at_h[i]=(uint8_t)hv;}}
static void emrect(float x,float y,float w,float h){int X0=(int)(x*TSC),Y0=(int)(y*TSC),X1=(int)((x+w)*TSC),Y1=(int)((y+h)*TSC);for(int yy=Y0;yy<Y1;yy++)for(int xx=X0;xx<X1;xx++)atlas.px[(size_t)yy*AT+xx]|=0xFF000000u;}
static void emcircle(float cx,float cy,float r){for(int yy=(int)((cy-r)*TSC);yy<=(int)((cy+r)*TSC);yy++)for(int xx=(int)((cx-r)*TSC);xx<=(int)((cx+r)*TSC);xx++){float dx=xx/TSC-cx,dy=yy/TSC-cy;if(dx*dx+dy*dy<r*r)atlas.px[(size_t)yy*AT+xx]|=0xFF000000u;}}
/* ---- стены: тёсаный камень / штукатурка / кирпич ---- */
static void wall_bg(int id,int m){uint32_t wc=WALLC[m];float br,bg,bb;cf(wc,&br,&bg,&bb);
 for(int y=0;y<TS;y++)for(int x=0;x<TS;x++){float u=x*64.f/TS,v=y*64.f/TS;float h,r,g,b;int ro;
  if(m==3){int row=(int)(v/5.f);float off=(row&1)?8:0;float bx=fmodf(u+off,16.f),by=fmodf(v,5.f);int joint=bx<.9f||by<.8f;uint32_t hh=hs((int)((u+off)/16)+id*7,row);float var=rf(hh)*.16f-.08f;
   float n=fbm2(u*.9f,v*.9f,id,3)*.12f;if(joint){r=.62f;g=.58f;b=.52f;h=.35f;ro=235;}else{r=br*(1+var)-n*.3f;g=bg*(1+var*.8f)-n*.3f;b=bb*(1+var*.6f)-n*.3f;h=.6f+n;ro=215;}}
  else{float ch=8.f;int row=(int)(v/ch);float off=(row&1)?11:0;float bw=(m==1)?14.f:22.f;float bx=fmodf(u+off+bw*4,bw),by=fmodf(v,ch);int joint=(bx<.5f||by<.45f);
   uint32_t hh=hs((int)((u+off)/bw)+id*13,row);float var=rf(hh)*.07f-.035f;float n=fbm2(u*.5f,v*.5f,id+3,4),fine=vn2(u*6,v*6,id)*.03f;
   float stain=fbm2(u*.12f+id,v*.06f,id+9,3);stain=stain>.55f?(stain-.55f)*.5f:0;
   r=br*(1+var)-(n-.5f)*.06f-fine-stain;g=bg*(1+var)-(n-.5f)*.06f-fine-stain;b=bb*(1+var)-(n-.5f)*.05f-fine-stain*.9f;
   if(joint){r*=.82f;g*=.8f;b*=.78f;h=.42f;ro=235;}else{float e=fminf(fminf(bx,bw-bx),fminf(by,ch-by));h=.55f+fminf(e,1.2f)*.08f+n*.06f;ro=220;}}
  float grime=v>50?(v-50)/14.f*.08f:0;r-=grime;g-=grime;b-=grime*.8f;put(id,x,y,rgbf(r,g,b),h,ro);}}
static void paint_bay(int id,int m,int k,int lit){float ox=(id%16)*64.f,oy=(id/16)*64.f;uint32_t wc=WALLC[m],st=gfx_lerp(wc,RGBX(250,244,230),.55f),dk=gfx_lerp(wc,RGBX(40,30,30),.45f),fr=RGBX(232,226,214);
 wall_bg(id,m);gfx_target_s(atlas.px,AT,AT,TSC);
 if(k==3){/* глухой простенок с филёнкой */gfx_rect(ox+12,oy+12,40,40,gfx_lerp(wc,st,.3f),255);gfx_rect(ox+14,oy+14,36,36,wc,255);gfx_rect(ox+14,oy+14,36,1,dk,90);mark(ox+12,oy+12,40,40,-1,170);mark(ox+14,oy+14,36,36,-1,140);return;}
 float wx=ox+18,wy=oy+8,ww=28,wh=46;if(k==2){wx=ox+16;wy=oy+2;ww=32;wh=60;}
 /* наличник */gfx_rect(wx-3,wy-3,ww+6,wh+6,st,255);mark(wx-3,wy-3,ww+6,wh+6,200,190);
 if(k==1){gfx_tri(wx-5,wy-3,wx+ww+5,wy-3,wx+ww/2,wy-14,st,255);gfx_tri(wx+2,wy-4,wx+ww-2,wy-4,wx+ww/2,wy-10,gfx_lerp(wc,dk,.2f),255);mark(wx-5,wy-14,ww+10,11,200,200);}
 /* стекло */
 if(lit){gfx_vgrad(wx,wy,ww,wh,RGBX(255,222,150),RGBX(240,160,78));emrect(wx,wy,ww,wh);
  for(int q=0;q<3;q++)gfx_ellipse(wx+ww*(.3f+q*.2f),wy+wh*.55f,3,7,RGBX(150,80,40),70);}
 else{gfx_vgrad(wx,wy,ww,wh,RGBX(70,86,108),RGBX(26,32,44));gfx_line(wx+4,wy+wh-6,wx+ww-6,wy+6,3,RGBX(170,190,214),50);gfx_rect(wx+2,wy+2,ww/2-3,wh-4,RGBX(200,190,170),26);}
 mark(wx,wy,ww,wh,14,150);
 /* переплёт: створки и горбыльки */
 gfx_rect(wx+ww/2-1,wy,2,wh,fr,255);mark(wx+ww/2-1,wy,2,wh,170,200);for(int q=1;q<4;q++){float yy=wy+wh*q/4.f;gfx_rect(wx,yy-.6f,ww,1.2f,fr,255);mark(wx,yy-.6f,ww,1.2f,170,190);}
 gfx_rect(wx,wy,ww,1.2f,fr,255);gfx_rect(wx,wy+wh-1.2f,ww,1.2f,fr,255);gfx_rect(wx,wy,1.2f,wh,fr,255);gfx_rect(wx+ww-1.2f,wy,1.2f,wh,fr,255);
 if(k==2){gfx_rect(wx,oy+52,ww,12,RGBX(36,34,40),255);for(float x=wx+1;x<wx+ww;x+=3)gfx_rect(x,oy+53,.8f,10,RGBX(60,58,64),255);mark(wx,oy+52,ww,12,150,170);}
 /* потёки под подоконником */
 for(int q=0;q<6;q++){uint32_t h=hs(id*17+q,5);float sx=wx+2+rf(h)*(ww-4),len=4+rf(h>>8)*10;gfx_line(sx,wy+wh+4,sx+.3f,wy+wh+4+len,1.2f,dk,30);}}
static void paint_shop(int id,int s,int lit){float ox=(id%16)*64.f,oy=(id/16)*64.f;uint32_t sb[6]={RGBX(126,32,38),RGBX(34,58,96),RGBX(52,52,58),RGBX(176,120,40),RGBX(80,44,34),RGBX(44,90,72)};
 wall_bg(id,0);gfx_target_s(atlas.px,AT,AT,TSC);
 gfx_rect(ox,oy+54,64,10,RGBX(112,106,100),255);mark(ox,oy+54,64,10,225,140);
 /* вывеска */gfx_rect(ox,oy,64,16,sb[s],255);mark(ox,oy,64,16,120,170);gfx_rect(ox,oy+15,64,1.2f,RGBX(214,180,90),255);gfx_rect(ox,oy+.3f,64,1.2f,RGBX(214,180,90),255);
 for(int i=0;i<9;i++){uint32_t h=hs(id+i,3);gfx_rect(ox+6+i*6,oy+5,3+h%3,6,RGBX(232,200,110),255);}mark(ox+5,oy+4,56,8,60,200);
 gfx_rect(ox,oy+16,64,2,RGBX(40,30,30),200);
 uint32_t g1=lit?RGBX(255,222,150):RGBX(72,88,110),g2=lit?RGBX(236,156,76):RGBX(28,34,48);uint32_t wd=gfx_lerp(sb[s],RGBX(30,24,20),.3f);
 if(s==0||s==3||s==5){int n=s==5?2:1;for(int k=0;k<n;k++){float wx=ox+(n==1?8:6+k*30),ww=n==1?48:26;gfx_rect(wx-2,oy+22,ww+4,32,wd,255);mark(wx-2,oy+22,ww+4,32,150,190);gfx_vgrad(wx,oy+24,ww,28,g1,g2);mark(wx,oy+24,ww,28,12,140);if(lit)emrect(wx,oy+24,ww,28);
   for(float x=wx+ww/4;x<wx+ww;x+=ww/4){gfx_rect(x-.6f,oy+24,1.2f,28,wd,255);}gfx_rect(wx,oy+37,ww,1.2f,wd,255);}
  if(s==3)for(int k=0;k<5;k++){gfx_ellipse(ox+14+k*9,oy+47,5,3,RGBX(200,140,70),255);gfx_ellipse(ox+13+k*9,oy+46,2,1,RGBX(240,200,140),200);}
  if(s==0)for(int k=0;k<4;k++){gfx_rect(ox+12+k*10,oy+44,6,8,RGBX(230,220,200),255);gfx_rect(ox+12+k*10,oy+44,6,2,RGBX(180,60,60),255);}}
 if(s==1){gfx_rect(ox+20,oy+20,24,44,wd,255);gfx_circle(ox+32,oy+22,12,wd,255);gfx_rect(ox+24,oy+30,16,34,RGBX(96,64,44),255);for(float y=oy+34;y<oy+62;y+=7)gfx_rect(ox+25,y,14,5,RGBX(110,74,52),255);mark(ox+24,oy+30,16,34,190,160);
  gfx_circle(ox+32,oy+27,8,lit?RGBX(255,214,120):RGBX(70,90,120),255);mark(ox+24,oy+19,16,16,14,150);if(lit)emcircle(ox+32,oy+27,8);gfx_circle(ox+37,oy+46,1.6f,RGBX(236,200,90),255);}
 if(s==2){gfx_rect(ox+14,oy+20,36,44,RGBX(206,200,190),255);gfx_circle(ox+32,oy+22,18,RGBX(206,200,190),255);gfx_rect(ox+18,oy+22,28,42,RGBX(22,20,24),255);gfx_circle(ox+32,oy+24,14,RGBX(22,20,24),255);mark(ox+18,oy+10,28,54,240,60);}
 if(s==4){gfx_rect(ox+4,oy+20,56,44,RGBX(70,46,34),255);for(float x=ox+6;x<ox+58;x+=8)gfx_rect(x,oy+22,1,40,RGBX(40,26,20),255);mark(ox+4,oy+20,56,44,200,160);gfx_rect(ox+34,oy+26,20,20,RGBX(60,46,40),255);gfx_vgrad(ox+36,oy+28,16,16,g1,g2);mark(ox+36,oy+28,16,16,12,150);if(lit)emrect(ox+36,oy+28,16,16);gfx_rect(ox+8,oy+28,18,34,RGBX(110,70,44),255);}
 if(s==0)for(int k=0;k<8;k++)gfx_rect(ox+4+k*7,oy+17,7,5,(k&1)?RGBX(248,244,236):RGBX(170,40,50),255);}
/* ---- попиксельные материалы ---- */
static void gen_mat(int id){
 for(int y=0;y<TS;y++)for(int x=0;x<TS;x++){float u=x*64.f/TS,v=y*64.f/TS;float r=.5f,g=.5f,b=.5f,h=.5f;int ro=200;float n=fbm2(u*.35f,v*.35f,id,4),f=vn2(u*5,v*5,id+1);
  switch(id){
  case TL_SLATE:{int row=(int)(v/7.f);float off=(row&1)?5.f:0;float sx=fmodf(u+off+40,10.f),sy=fmodf(v,7.f);uint32_t hh=hs((int)((u+off)/10)+row*31,id);float var=rf(hh)*.12f;
   r=.22f+var-n*.05f;g=.25f+var-n*.05f;b=.31f+var-n*.04f;h=.35f+sy/7.f*.4f+f*.05f;if(sx<.5f||sy>6.4f){r*=.6f;g*=.6f;b*=.6f;h=.25f;}ro=140+(int)(var*200);
   if(rf(hh>>4)>.93f){r+=.06f;g+=.05f;}break;}
  case TL_ZINC:{float sx=fmodf(u,16.f);float seam=sx<1.2f?1.f:0;float pat=fbm2(u*.2f,v*.08f,id,4);r=.52f+pat*.1f-f*.03f;g=.55f+pat*.1f-f*.03f;b=.58f+pat*.08f-f*.03f;h=.45f+seam*.4f+f*.02f;
   if(seam){r+=.08f;g+=.08f;b+=.08f;}if(pat>.62f){r-=.08f;g-=.04f;b-=.06f;}ro=95+(int)(pat*60);break;}
  case TL_COBBLE:case TL_COBBLE2:{int row=(int)(v/6.f);float off=(row&1)?3.5f:0;float cx=fmodf(u+off+70,7.f)-3.5f,cy=fmodf(v,6.f)-3.f;uint32_t hh=hs((int)((u+off)/7)+row*57,id);
   float jx=(rf(hh)-.5f)*.8f,jy=(rf(hh>>8)-.5f)*.6f;cx-=jx;cy-=jy;float sx=3.f-.25f-rf(hh>>3)*.4f,sy=2.5f-.2f-rf(hh>>5)*.3f;float d=powf(fabsf(cx)/sx,2.6f)+powf(fabsf(cy)/sy,2.6f);
   float tone=(id==TL_COBBLE?.36f:.40f)+rf(hh>>11)*.10f;float warm=rf(hh>>15)*.05f;
   if(d<1.f){float dome=sqrtf(1-d);r=tone+warm-n*.08f+f*.05f;g=tone-n*.08f+f*.05f;b=tone-warm*.6f-n*.07f+f*.05f;h=.45f+dome*.45f+f*.04f;ro=165+(int)(rf(hh>>2)*40);}
   else{r=.27f-n*.05f;g=.25f-n*.05f;b=.22f-n*.04f;h=.25f+f*.05f;ro=245;}break;}
  case TL_SIDEWALK:{float sx=fmodf(u,32.f),sy=fmodf(v,21.33f);int j=sx<.6f||sy<.6f;uint32_t hh=hs((int)(u/32)+(int)(v/21.33f)*7,id);float t=.66f+rf(hh)*.06f;
   r=t-n*.06f+f*.03f;g=t-.02f-n*.06f+f*.03f;b=t-.05f-n*.05f+f*.03f;h=j?.3f:.55f+n*.08f;if(j){r*=.7f;g*=.7f;b*=.7f;}ro=j?240:205;break;}
  case TL_PLAZA:{float sx=fmodf(u,16.f),sy=fmodf(v,16.f);int j=sx<.5f||sy<.5f;int cell=((int)(u/16)+(int)(v/16))&1;uint32_t hh=hs((int)(u/16)*3+(int)(v/16)*11,id);float t=(cell?.70f:.62f)+rf(hh)*.05f;
   r=t+.02f-n*.07f+f*.02f;g=t-.01f-n*.07f+f*.02f;b=t-.07f-n*.06f+f*.02f;h=j?.3f:.55f+n*.06f;if(j){r*=.66f;g*=.66f;b*=.66f;}ro=j?235:185;break;}
  case TL_DIRT:{float p=vn2(u*2.5f,v*2.5f,id+5);r=.44f-n*.1f+f*.04f;g=.37f-n*.09f+f*.04f;b=.29f-n*.08f;h=.4f+n*.25f+(p>.78f?.2f:0);if(p>.78f){r+=.08f;g+=.07f;b+=.06f;}ro=245;break;}
  case TL_GRAVEL:{float p=vn2(u*3.5f,v*3.5f,id+5),q=vn2(u*7,v*7,id+9);r=.62f-n*.08f+q*.06f;g=.58f-n*.08f+q*.06f;b=.50f-n*.07f+q*.05f;h=.4f+p*.3f+q*.15f;ro=240;break;}
  case TL_GRASS:{float bl=vn2(u*9,v*2.2f,id),bl2=vn2(u*13+5,v*3,id+3);float t=n*.5f+bl*.3f+bl2*.2f;r=.20f+t*.12f;g=.34f+t*.18f;b=.13f+t*.06f;if(vn2(u*.6f,v*.6f,id+11)>.7f){r+=.06f;g+=.02f;}h=.3f+t*.5f;ro=240;break;}
  case TL_HEDGE:{float l=vn2(u*4,v*4,id),l2=vn2(u*9,v*9,id+2);float t=l*.6f+l2*.4f;r=.10f+t*.10f;g=.24f+t*.16f;b=.08f+t*.05f;h=.2f+t*.7f;ro=235;break;}
  case TL_PLANKS:case TL_WOOD:{float pw=id==TL_PLANKS?8.f:64.f;float sx=fmodf(u,pw);int pl=(int)(u/pw);uint32_t hh=hs(pl,id);float grain=sinf((id==TL_PLANKS?v:u)*1.6f+vn2(u*.4f,v*.4f,id+pl)*6.f)*.5f+.5f;
   float t=.36f+rf(hh)*.1f;r=t+grain*.06f-n*.05f;g=t*.72f+grain*.04f-n*.05f;b=t*.5f+grain*.02f-n*.04f;h=.5f+grain*.1f;if(id==TL_PLANKS&&sx<.6f){r*=.4f;g*=.4f;b*=.4f;h=.2f;}
   if(id==TL_WOOD){float ry=fmodf(v,6.f);if(ry<.4f){r*=.6f;g*=.6f;b*=.6f;h=.3f;}}ro=200;break;}
  case TL_BARK:{float s=vn2(u*.8f,v*.2f,id),s2=vn2(u*3,v*.6f,id+4);float t=s*.6f+s2*.4f;r=.24f+t*.1f;g=.19f+t*.07f;b=.14f+t*.05f;h=.2f+t*.7f;ro=245;break;}
  case TL_HAY:{float s=vn2(u*.7f,v*5,id),s2=vn2(u*1.5f+3,v*9,id+2);float t=s*.6f+s2*.4f;r=.70f+t*.18f;g=.55f+t*.15f;b=.22f+t*.08f;h=.3f+t*.6f;ro=250;break;}
  case TL_WHITE:{r=g=b=.92f-n*.04f+f*.02f;h=.5f+n*.04f;ro=170;break;}
  case TL_IRON:{r=.16f+n*.04f;g=.16f+n*.04f;b=.18f+n*.04f;h=.5f+f*.1f;ro=110+(int)(n*60);if(n>.6f){r+=.08f;g+=.03f;}break;}
  case TL_MARBLE:{float vein=fabsf(sinf(u*.25f+fbm2(u*.15f,v*.15f,id,5)*9.f));r=.90f-n*.05f-(vein<.08f?.18f:0);g=.89f-n*.05f-(vein<.08f?.18f:0);b=.86f-n*.04f-(vein<.08f?.15f:0);h=.5f;ro=90;break;}
  case TL_BASE:case TL_CATH:case TL_QUAY:case TL_GOTH:case TL_GOTHL:case TL_ROSE:{float bw=id==TL_BASE?32.f:id==TL_QUAY?24.f:30.f,bh=id==TL_BASE?16.f:id==TL_QUAY?10.f:14.f;int row=(int)(v/bh);float off=(row&1)?bw*.5f:0;float sx=fmodf(u+off,bw),sy=fmodf(v,bh);
   uint32_t hh=hs((int)((u+off)/bw)+row*19,id);float t=(id==TL_BASE?.58f:id==TL_QUAY?.52f:.74f)+rf(hh)*.08f;int j=sx<.7f||sy<.7f;
   r=t-n*.08f+f*.03f;g=t-.015f-n*.08f+f*.03f;b=t-.05f-n*.07f+f*.03f;float e=fminf(fminf(sx,bw-sx),fminf(sy,bh-sy));h=j?.3f:.5f+fminf(e,2.f)*.1f+n*.05f;if(j){r*=.72f;g*=.72f;b*=.72f;}
   if(id==TL_QUAY&&v>44){float w=(v-44)/20.f;r-=w*.18f;g-=w*.12f;b-=w*.1f;}ro=j?240:(id==TL_QUAY?200:215);break;}
  case TL_BRICK:{int row=(int)(v/4.f);float off=(row&1)?8:0;float sx=fmodf(u+off,16.f),sy=fmodf(v,4.f);uint32_t hh=hs((int)((u+off)/16)+row*7,id);int j=sx<.7f||sy<.6f;float var=rf(hh)*.14f;
   r=.56f+var-n*.08f;g=.28f+var*.5f-n*.06f;b=.22f+var*.4f-n*.05f;h=j?.3f:.6f+f*.05f;if(j){r=.58f;g=.55f;b=.5f;}ro=j?240:220;break;}
  case TL_ROOFRED:{float sx=fmodf(u,8.f),sy=fmodf(v+((int)(u/8)&1)*4,8.f);float c=sinf(sx/8.f*3.14159f);r=.58f+c*.1f-n*.08f;g=.28f+c*.05f-n*.05f;b=.20f-n*.04f;h=.3f+c*.4f+sy/8.f*.2f;if(sy>7.3f){r*=.6f;g*=.6f;b*=.6f;}ro=180;break;}
  case TL_AWNING:case TL_CLOTH:{int st=(int)(u/8.f)&1;float w=(sinf(u*12)*sinf(v*12))*.5f+.5f;if(id==TL_AWNING){r=st?.95f:.66f;g=st?.93f:.16f;b=st?.88f:.19f;}else{r=.92f;g=.9f;b=.84f;}
   r-=w*.04f+n*.06f;g-=w*.04f+n*.06f;b-=w*.04f+n*.05f;h=.5f+w*.08f;ro=240;break;}
  case TL_SHUTTER:{float sy=fmodf(v,4.f);float fr=(u<4||u>60||v<4||v>60)?1:0;float sl=sy/4.f;r=g=b=.9f-sl*.2f-n*.05f;h=fr?.7f:.35f+sl*.3f;if(fr){r=g=b=.95f;}ro=180;break;}
  case TL_DOOR:case TL_DOOR2:{float pnl=((u>8&&u<28)||(u>36&&u<56))&&((v>6&&v<26)||(v>32&&v<58))?1:0;float base=id==TL_DOOR?.28f:.18f;float grain=vn2(u*.3f,v*3,id)*.08f;
   r=base+grain+.06f;g=base*.7f+grain;b=base*.5f+grain*.5f;if(id==TL_DOOR2){r=.13f+grain;g=.24f+grain;b=.20f+grain;}h=pnl?.42f:.62f;if(u>30&&u<34){r*=.5f;g*=.5f;b*=.5f;h=.3f;}
   if(fabsf(u-46)<1.5f&&fabsf(v-34)<1.5f){r=.75f;g=.6f;b=.3f;h=.8f;ro=60;}else ro=170;break;}
  case TL_COLUMN:{float fl=fabsf(sinf(u/64.f*3.14159f*12));r=g=.82f-n*.06f;b=.78f-n*.05f;r-=(1-fl)*.08f;g-=(1-fl)*.08f;b-=(1-fl)*.07f;h=.3f+fl*.5f;ro=200;break;}
  case TL_LEAD:{r=.40f+n*.06f;g=.42f+n*.06f;b=.46f+n*.06f;h=.5f+f*.05f;ro=120;break;}
  case TL_BOAT:{float sy=fmodf(v,5.f);float grain=vn2(u*.3f,v*2,id)*.1f;r=.30f+grain;g=.21f+grain*.7f;b=.14f+grain*.5f;if(sy<.5f){r*=.5f;g*=.5f;b*=.5f;}if(v>52){r=.14f;g=.12f;b=.11f;}h=sy<.5f?.3f:.55f;ro=210;break;}
  case TL_LANTERN:{r=1.f;g=.86f;b=.6f;h=.5f;ro=40;break;}
  case TL_SAIL:{r=.86f-n*.08f;g=.82f-n*.08f;b=.72f-n*.06f;h=.5f;ro=240;break;}
  default:continue;}
  put(id,x,y,rgbf(r,g,b),h,ro);}}
static void paint_goth(int id){float ox=(id%16)*64.f,oy=(id/16)*64.f;int lit=id==TL_GOTHL;gfx_target_s(atlas.px,AT,AT,TSC);
 if(id==TL_ROSE){gfx_circle(ox+32,oy+32,29,RGBX(150,144,134),255);gfx_circle(ox+32,oy+32,27,RGBX(60,56,58),255);mark(ox+4,oy+4,56,56,30,140);
  for(int k=0;k<12;k++){float a=k*.5236f;gfx_tri(ox+32,oy+32,ox+32+sinf(a)*25,oy+32-cosf(a)*25,ox+32+sinf(a+.5f)*25,oy+32-cosf(a+.5f)*25,(k&1)?RGBX(170,40,56):RGBX(46,76,160),255);gfx_line(ox+32,oy+32,ox+32+sinf(a)*26,oy+32-cosf(a)*26,1.5f,RGBX(150,144,134),255);}
  gfx_ring(ox+32,oy+32,14,1.5f,RGBX(150,144,134),255);gfx_circle(ox+32,oy+32,6,RGBX(230,190,70),255);gfx_ring(ox+32,oy+32,27,2,RGBX(160,154,144),255);return;}
 gfx_rect(ox+18,oy+22,28,42,RGBX(150,144,134),255);gfx_circle(ox+32,oy+24,14,RGBX(150,144,134),255);gfx_rect(ox+20,oy+24,24,40,RGBX(50,46,48),255);gfx_circle(ox+32,oy+25,12,RGBX(50,46,48),255);mark(ox+18,oy+10,28,54,40,150);
 uint32_t c1[5]={RGBX(170,44,56),RGBX(50,80,170),RGBX(210,170,50),RGBX(60,150,90),RGBX(130,60,160)};
 for(int i=0;i<5;i++){for(int j=0;j<6;j++){uint32_t c=c1[(i+j*2)%5];gfx_rect(ox+21+i*4.6f,oy+26+j*6,4.2f,5.6f,lit?gfx_lerp(c,RGBX(255,240,180),.45f):gfx_lerp(c,0x000000,.4f),255);}}
 gfx_circle(ox+32,oy+25,9,lit?RGBX(255,226,150):RGBX(70,86,120),255);gfx_rect(ox+31.3f,oy+14,1.4f,50,RGBX(150,144,134),255);
 if(lit){emrect(ox+21,oy+25,22,38);emcircle(ox+32,oy+25,9);}}
static void paint_signs(void){static const uint32_t bg[6]={RGBX(120,30,36),RGBX(30,56,92),RGBX(40,70,40),RGBX(150,110,40),RGBX(70,40,30),RGBX(30,30,34)};
 for(int s=0;s<6;s++){int id=TL_SIGN0+s;float ox=(id%16)*64.f,oy=(id/16)*64.f;gfx_target_s(atlas.px,AT,AT,TSC);gfx_rect(ox,oy,64,64,RGBX(214,176,90),255);gfx_rect(ox+3,oy+3,58,58,bg[s],255);mark(ox,oy,64,64,90,170);mark(ox+3,oy+3,58,58,170,140);
  uint32_t g=RGBX(232,200,110);switch(s){
  case 0:gfx_ellipse(ox+32,oy+36,16,10,g,255);gfx_ellipse(ox+32,oy+36,12,7,bg[s],255);gfx_rect(ox+20,oy+20,24,6,g,255);break;/* булочник: крендель */
  case 1:gfx_rect(ox+22,oy+16,20,30,g,255);gfx_rect(ox+26,oy+20,12,22,bg[s],255);gfx_rect(ox+30,oy+44,4,6,g,255);break;/* часовщик/лавка */
  case 2:for(int k=0;k<3;k++)gfx_circle(ox+22+k*10,oy+34,5,g,255);gfx_line(ox+20,oy+22,ox+44,oy+22,3,g,255);break;/* бакалея */
  case 3:gfx_tri(ox+32,oy+14,ox+18,oy+48,ox+46,oy+48,g,255);gfx_rect(ox+30,oy+48,4,6,g,255);break;/* вино: бокал */
  case 4:gfx_rect(ox+18,oy+26,28,18,g,255);gfx_line(ox+18,oy+26,ox+32,oy+16,3,g,255);gfx_line(ox+46,oy+26,ox+32,oy+16,3,g,255);break;/* сапожник/дом */
  default:gfx_ring(ox+32,oy+32,14,3,g,255);gfx_line(ox+32,oy+18,ox+32,oy+46,3,g,255);break;}
  mark(ox+14,oy+12,36,40,80,200);}}
static void paint_palace(int id,int lit){float ox=(id%16)*64.f,oy=(id/16)*64.f;/* окно дворца: высокое, с лучковым верхом и пилястрами */
 int y0=(id/16)*TS,x0=(id%16)*TS;for(int y=0;y<TS;y++)for(int x=0;x<TS;x++){float u=x*64.f/TS,v=y*64.f/TS;float n=fbm2(u*.3f,v*.3f,id,4);int row=(int)(v/6);float j=fmodf(v,6.f)<.4f?1:0;
  float t=.80f-n*.06f-j*.12f;put(id,x,y,rgbf(t,t-.02f,t-.07f),j?.35f:.55f,j?235:210);(void)y0;(void)x0;(void)row;}
 gfx_target_s(atlas.px,AT,AT,TSC);uint32_t st=RGBX(236,230,214);
 gfx_rect(ox+2,oy,6,64,st,255);gfx_rect(ox+56,oy,6,64,st,255);mark(ox+2,oy,6,64,200,200);mark(ox+56,oy,6,64,200,200);
 float wx=ox+18,wy=oy+12,ww=28,wh=46;gfx_rect(wx-3,wy-3,ww+6,wh+6,st,255);gfx_circle(wx+ww/2,wy,ww/2+3,st,255);mark(wx-3,wy-ww/2-3,ww+6,wh+ww/2+6,200,190);
 if(lit){gfx_vgrad(wx,wy,ww,wh,RGBX(255,226,160),RGBX(240,170,90));gfx_circle(wx+ww/2,wy,ww/2,RGBX(255,226,160),255);emrect(wx,wy,ww,wh);emcircle(wx+ww/2,wy,ww/2);}
 else{gfx_vgrad(wx,wy,ww,wh,RGBX(70,86,108),RGBX(26,32,44));gfx_circle(wx+ww/2,wy,ww/2,RGBX(80,96,118),255);}
 mark(wx,wy-ww/2,ww,wh+ww/2,14,150);gfx_rect(wx+ww/2-.7f,wy-ww/2,1.4f,wh+ww/2,RGBX(232,226,214),255);for(int q=1;q<5;q++)gfx_rect(wx,wy+wh*q/5.f,ww,1.2f,RGBX(232,226,214),255);
 gfx_rect(wx-5,wy+wh+3,ww+10,3,st,255);mark(wx-5,wy+wh+3,ww+10,3,200,230);
 gfx_ellipse(wx+ww/2,wy-ww/2-6,6,4,RGBX(214,190,120),255);}
/* листва: отдельная текстура с вырезом (альфа) */
static Tex leaves;
static void build_leaves(void){leaves=tex_new(512,512,0);leaves.cutout=1;leaves.mip=6;
 for(int q=0;q<4;q++){int ox=(q&1)*256,oy=(q>>1)*256;
  for(int i=0;i<900;i++){uint32_t h=hs(i*7+q*1000,77);float cx=24+rf(h)*208,cy=24+rf(h>>7)*208;float dx=cx-128,dy=cy-128;if(dx*dx+dy*dy>110*110)continue;
   float a=rf(h>>3)*6.283f,L=7+rf(h>>9)*9,W=L*.42f;float tone=rf(h>>13);uint32_t c=q==3?rgbf(.42f+tone*.2f,.30f+tone*.12f,.10f):rgbf(.16f+tone*.14f,.30f+tone*.2f+(q==1?.05f:0),.09f+tone*.06f);
   for(int yy=(int)(cy-L);yy<=(int)(cy+L);yy++)for(int xx=(int)(cx-L);xx<=(int)(cx+L);xx++){if(xx<0||yy<0||xx>=256||yy>=256)continue;float px=xx-cx,py=yy-cy,lx=px*cosf(a)+py*sinf(a),ly=-px*sinf(a)+py*cosf(a);
    float e=(lx*lx)/(L*L)+(ly*ly)/(W*W*(1-lx*lx/(L*L+1))+.01f);if(e>1)continue;float sh=.85f+.15f*(1-e);uint32_t cc=gfx_lerp(0,c,sh);if(fabsf(ly)<.6f)cc=gfx_lerp(cc,0xFFFFFF,.12f);leaves.px[(size_t)(oy+yy)*512+ox+xx]=0xFF000000u|cc;}}}}
static void build_atlas(void){atlas=tex_new(AT,AT,0);atlas.aem=1;atlas.nmap=1;atlas.mip=5;atlas.bump=4.f;at_h=malloc((size_t)AT*AT);at_r=malloc((size_t)AT*AT);memset(at_h,128,(size_t)AT*AT);memset(at_r,200,(size_t)AT*AT);atlas.hgt=at_h;atlas.rgh=at_r;
 for(size_t i=0;i<(size_t)AT*AT;i++)atlas.px[i]=0xFFFFFF;
 for(int m=0;m<6;m++)for(int k=0;k<4;k++)for(int l=0;l<2;l++)paint_bay((m*4+k)*2+l,m,k,l);for(int s=0;s<6;s++)for(int l=0;l<2;l++)paint_shop(48+s*2+l,s,l);
 for(int id=60;id<TL_N;id++)gen_mat(id);paint_goth(TL_GOTH);paint_goth(TL_GOTHL);paint_goth(TL_ROSE);paint_signs();paint_palace(TL_PALW,0);paint_palace(TL_PALWL,1);emrect((TL_LANTERN%16)*64.f,(TL_LANTERN/16)*64.f,64,64);
 gfx_target_screen();build_leaves();}
