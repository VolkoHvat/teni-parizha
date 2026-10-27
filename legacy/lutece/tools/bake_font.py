# Запекание SDF-атласа шрифта для Lutèce (шрифт DejaVu — свободная лицензия Bitstream Vera/DejaVu)
import sys, numpy as np
from PIL import Image, ImageFont, ImageDraw
from scipy.ndimage import distance_transform_edt as edt
font_path, out_c = sys.argv[1], sys.argv[2]
PX=40; SPREAD=5; CELL=PX+2*SPREAD+2
chars=list(range(32,127))+list(range(0x410,0x450))+[0x401,0x451]+[ord(c) for c in "«»—–…№°éèêëàâçôûùîïœÉÈÊÀÇÔ’‘“”•×·€"]
chars=sorted(set(chars))
font=ImageFont.truetype(font_path,PX)
asc,desc=font.getmetrics()
AW=1024; cols=AW//CELL; rows=(len(chars)+cols-1)//cols; AH=rows*CELL

atlas=np.zeros((AH,AW),np.uint8); meta=[]
for i,cp in enumerate(chars):
    ch=chr(cp); cx=(i%cols)*CELL; cy=(i//cols)*CELL
    img=Image.new('L',(CELL*2,CELL*2),0); d=ImageDraw.Draw(img)
    ox,oy=SPREAD+1,SPREAD+1
    d.text((ox,oy),ch,font=font,fill=255)
    a=np.array(img,np.float32)/255.0
    inside=a>0.5
    dist_out=edt(~inside); dist_in=edt(inside)
    sd=np.where(inside,dist_in-0.5,-(dist_out-0.5))
    v=np.clip(128+sd*(127.0/SPREAD),0,255).astype(np.uint8)
    adv=font.getlength(ch)
    bb=font.getbbox(ch)  # x0,y0,x1,y1 относительно начала текста
    if bb[2]<=bb[0]: bb=(0,0,0,0)
    x0=max(0,bb[0]+ox-SPREAD); y0=max(0,bb[1]+oy-SPREAD); x1=bb[2]+ox+SPREAD; y1=bb[3]+oy+SPREAD
    w=min(CELL,x1-x0); h=min(CELL,y1-y0)
    if bb==(0,0,0,0): w=h=0
    atlas[cy:cy+h,cx:cx+w]=v[y0:y0+h,x0:x0+w]
    # смещение левого верхнего угла относительно (перо, верх строки)
    meta.append((cp,cx,cy,w,h,x0-ox,y0-oy,adv))
with open(out_c,'w') as f:
    f.write('/* SDF-атлас шрифта DejaVu Serif Condensed (лицензия DejaVu: свободное распространение). Сгенерировано tools/bake_font.py */\n')
    f.write('#include <stdint.h>\n')
    f.write(f'const int lu_font_w={AW},lu_font_h={AH},lu_font_px={PX},lu_font_spread={SPREAD},lu_font_ascent={asc},lu_font_descent={desc},lu_font_n={len(meta)};\n')
    f.write('const float lu_font_glyphs[][8]={\n')
    for m in meta: f.write('{%d,%d,%d,%d,%d,%d,%d,%.2ff},\n'%m)
    f.write('};\nconst unsigned char lu_font_atlas[]=\n"')
    data=atlas.tobytes(); line=0
    for k,b in enumerate(data):
        s='\\%03o'%b if (b<32 or b>126 or b in (34,92,63)) else chr(b)
        # не оставлять восьмеричную escape-последовательность перед цифрой
        f.write(s); line+=len(s)
        if line>200: f.write('"\n"'); line=0
    f.write('";\n')
Image.fromarray(atlas).save(out_c.replace('.c','_preview.png'))
print(len(meta),'глифов, атлас',AW,'x',AH)
