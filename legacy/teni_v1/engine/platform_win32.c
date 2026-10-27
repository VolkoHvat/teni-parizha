/* Платформенный слой Windows: окно, OpenGL 3.3 (WGL), ввод, таймер, звук (waveOut), текст интерфейса (GDI → слой HUD). */
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include <stdio.h>
#include "engine.h"
#include "r3d.h"
#include "gl33.h"
static HWND hwnd;static HDC hdc;static HGLRC hrc;static int running=1,fullscreen=0;static WINDOWPLACEMENT wp_prev={sizeof(WINDOWPLACEMENT)};
static int ncores=1,cursor_hidden=0;static HFONT fcache[24];static int fpx[24],fbd[24],nf;
void platform_quit(void){running=0;}
static HFONT getfont(int px,int bold){for(int i=0;i<nf;i++)if(fpx[i]==px&&fbd[i]==bold)return fcache[i];
 if(nf>=24){for(int i=0;i<nf;i++)DeleteObject(fcache[i]);nf=0;}
 HFONT f=CreateFontW(-px,0,0,0,bold?FW_BOLD:FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Georgia");
 fcache[nf]=f;fpx[nf]=px;fbd[nf]=bold;return fcache[nf++];}
/* ---- текст: GDI рисует белым по чёрному в маску, маска смешивается в слой интерфейса нужным цветом ---- */
static HDC mdc;static HBITMAP mbmp,mold;static uint32_t*mpx;static int mw,mh;
static void mask_ensure(void){if(mdc&&mw==fbw&&mh==fbh)return;if(mdc){SelectObject(mdc,mold);DeleteObject(mbmp);DeleteDC(mdc);}
 BITMAPINFO bi;memset(&bi,0,sizeof bi);bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=fbw;bi.bmiHeader.biHeight=-fbh;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
 mdc=CreateCompatibleDC(NULL);mbmp=CreateDIBSection(mdc,&bi,DIB_RGB_COLORS,(void**)&mpx,NULL,0);mold=(HBITMAP)SelectObject(mdc,mbmp);mw=fbw;mh=fbh;SetBkMode(mdc,TRANSPARENT);SetTextColor(mdc,RGB(255,255,255));}
static void blend_mask(RECT r,uint32_t col,int amax){if(r.left<0)r.left=0;if(r.top<0)r.top=0;if(r.right>fbw)r.right=fbw;if(r.bottom>fbh)r.bottom=fbh;
 int cr=(col>>16)&255,cg=(col>>8)&255,cb=col&255;
 for(int y=r.top;y<r.bottom;y++){uint32_t*m=&mpx[(size_t)y*fbw],*d=&fb[(size_t)y*fbw];for(int x=r.left;x<r.right;x++){int a=(int)((m[x]>>8)&255);if(!a)continue;a=a*amax/255;int ia=255-a;uint32_t p=d[x];
  int R=(cr*a+((p>>16)&255)*ia+127)/255,G=(cg*a+((p>>8)&255)*ia+127)/255,B=(cb*a+(p&255)*ia+127)/255,A=a+((p>>24)*ia+127)/255;d[x]=((uint32_t)A<<24)|((uint32_t)R<<16)|((uint32_t)G<<8)|(uint32_t)B;}}}
void platform_draw_text(void){if(!tqn)return;mask_ensure();if(!mpx)return;float sc=(float)rs;
 for(int i=0;i<tqn;i++){TextCmd*t=&tq[i];wchar_t ws[520];MultiByteToWideChar(CP_UTF8,0,t->s,-1,ws,520);
  int px=(int)(t->size*sc+.5f);if(px<8)px=8;HFONT old=(HFONT)SelectObject(mdc,getfont(px,t->flags&TF_BOLD));
  RECT r={(int)(t->x*sc),(int)(t->y*sc),(int)((t->x+t->w)*sc),(int)((t->y+t->h)*sc)};UINT fmt=DT_WORDBREAK|DT_NOPREFIX|((t->flags&TF_CENTER)?DT_CENTER:(t->flags&TF_RIGHT)?DT_RIGHT:DT_LEFT);
  int o=sc>=1.5f?2:1;RECT big={r.left,r.top,r.right+o+1,r.bottom+o+1};if(big.right>fbw)big.right=fbw;if(big.bottom>fbh)big.bottom=fbh;if(big.left<0)big.left=0;if(big.top<0)big.top=0;
  if(!(t->flags&TF_NOSHADOW)){FillRect(mdc,&big,(HBRUSH)GetStockObject(BLACK_BRUSH));RECT s=r;OffsetRect(&s,o,o);DrawTextW(mdc,ws,-1,&s,fmt);GdiFlush();blend_mask(big,RGBX(10,8,24),220);}
  FillRect(mdc,&big,(HBRUSH)GetStockObject(BLACK_BRUSH));DrawTextW(mdc,ws,-1,&r,fmt);GdiFlush();blend_mask(big,t->col,255);SelectObject(mdc,old);
  if(big.top<hud_y0)hud_y0=big.top;if(big.bottom-1>hud_y1)hud_y1=big.bottom-1;}}
static int mapkey(WPARAM vk){
 switch(vk){case 'W':return K_W;case 'A':return K_A;case 'S':return K_S;case 'D':return K_D;case VK_UP:return K_UP;case VK_DOWN:return K_DOWN;case VK_LEFT:return K_LEFT;case VK_RIGHT:return K_RIGHT;
 case VK_SPACE:return K_SPACE;case VK_SHIFT:return K_SHIFT;case VK_CONTROL:return K_CTRL;case 'E':return K_E;case 'F':return K_F;case 'Q':return K_Q;case 'R':return K_R;case VK_TAB:return K_TAB;case VK_RETURN:return K_ENTER;case VK_ESCAPE:return K_ESC;}return -1;}
static void toggle_fs(void){DWORD st=GetWindowLong(hwnd,GWL_STYLE);
 if(!fullscreen){MONITORINFO mi={sizeof(mi)};GetWindowPlacement(hwnd,&wp_prev);
  if(GetMonitorInfo(MonitorFromWindow(hwnd,MONITOR_DEFAULTTOPRIMARY),&mi)){SetWindowLong(hwnd,GWL_STYLE,st&~WS_OVERLAPPEDWINDOW);
   SetWindowPos(hwnd,HWND_TOP,mi.rcMonitor.left,mi.rcMonitor.top,mi.rcMonitor.right-mi.rcMonitor.left,mi.rcMonitor.bottom-mi.rcMonitor.top,SWP_NOOWNERZORDER|SWP_FRAMECHANGED);fullscreen=1;}}
 else{SetWindowLong(hwnd,GWL_STYLE,st|WS_OVERLAPPEDWINDOW);SetWindowPlacement(hwnd,&wp_prev);SetWindowPos(hwnd,NULL,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_FRAMECHANGED);fullscreen=0;}}
static int qual_manual;
static LRESULT CALLBACK wndproc(HWND h,UINT m,WPARAM w,LPARAM l){
 switch(m){
 case WM_CLOSE:running=0;return 0;
 case WM_DESTROY:PostQuitMessage(0);return 0;
 case WM_ERASEBKGND:return 1;
 case WM_KEYDOWN:{if(w==VK_F2){if(!(l&0x40000000)){gfx_quality=(gfx_quality+1)%4;qual_manual=1;}return 0;}if(w==VK_F11){if(!(l&0x40000000))toggle_fs();return 0;}
  int k=mapkey(w);if(k>=0){int rep=(l&0x40000000)!=0;key_down[k]=1;if(!rep||(k>=K_UP&&k<=K_RIGHT))key_hit[k]=1;}return 0;}
 case WM_KEYUP:{int k=mapkey(w);if(k>=0)key_down[k]=0;return 0;}
 case WM_KILLFOCUS:memset(key_down,0,sizeof key_down);return 0;
 case WM_SYSKEYDOWN:if(w==VK_F10)return 0;if(w==VK_RETURN&&(l&(1<<29))){toggle_fs();return 0;}break;
 }return DefWindowProcW(h,m,w,l);}
/* --- пул потоков (для фоновых расчётов) --- */
#define MAXW 7
static HANDLE wk_go[MAXW],wk_done[MAXW];static int nwk;static job_fn pj_fn;static void*pj_arg;static volatile LONG pj_next;static int pj_n;
static void pj_run(void){for(;;){int i=(int)InterlockedIncrement(&pj_next)-1;if(i>=pj_n)break;pj_fn(pj_arg,i);}}
static DWORD WINAPI wk_main(LPVOID p){int k=(int)(INT_PTR)p;for(;;){WaitForSingleObject(wk_go[k],INFINITE);pj_run();SetEvent(wk_done[k]);}return 0;}
static void pool_start(void){nwk=ncores-1;if(nwk>MAXW)nwk=MAXW;if(nwk<0)nwk=0;int ok=0;
 for(int k=0;k<nwk;k++){wk_go[ok]=CreateEventW(NULL,FALSE,FALSE,NULL);wk_done[ok]=CreateEventW(NULL,FALSE,FALSE,NULL);if(!wk_go[ok]||!wk_done[ok])break;
  HANDLE t=CreateThread(NULL,0,wk_main,(LPVOID)(INT_PTR)ok,0,NULL);if(!t)break;CloseHandle(t);ok++;}nwk=ok;}
void platform_parallel(job_fn fn,void*arg,int n){pj_fn=fn;pj_arg=arg;pj_n=n;InterlockedExchange(&pj_next,0);for(int k=0;k<nwk;k++)SetEvent(wk_go[k]);pj_run();if(nwk)WaitForMultipleObjects(nwk,wk_done,TRUE,INFINITE);}
/* --- звук: waveOut, 4 буфера по 1024 кадра, отдельный поток --- */
#define AFR 1024
static HWAVEOUT hwo;static WAVEHDR whdr[4];static short abuf[4][AFR*2];static HANDLE aev;
static DWORD WINAPI audio_thread(LPVOID p){(void)p;
 for(int i=0;i<4;i++){game_audio(abuf[i],AFR);memset(&whdr[i],0,sizeof(WAVEHDR));whdr[i].lpData=(LPSTR)abuf[i];whdr[i].dwBufferLength=AFR*4;
  if(waveOutPrepareHeader(hwo,&whdr[i],sizeof(WAVEHDR))!=MMSYSERR_NOERROR)return 0;waveOutWrite(hwo,&whdr[i],sizeof(WAVEHDR));}
 for(;;){WaitForSingleObject(aev,50);for(int i=0;i<4;i++)if(whdr[i].dwFlags&WHDR_DONE){game_audio(abuf[i],AFR);waveOutWrite(hwo,&whdr[i],sizeof(WAVEHDR));}}return 0;}
static void audio_start(void){WAVEFORMATEX wf;memset(&wf,0,sizeof wf);wf.wFormatTag=WAVE_FORMAT_PCM;wf.nChannels=2;wf.nSamplesPerSec=44100;wf.wBitsPerSample=16;wf.nBlockAlign=4;wf.nAvgBytesPerSec=44100*4;
 aev=CreateEventW(NULL,FALSE,FALSE,NULL);if(!aev)return;
 if(waveOutOpen(&hwo,WAVE_MAPPER,&wf,(DWORD_PTR)aev,0,CALLBACK_EVENT)!=MMSYSERR_NOERROR)return;
 HANDLE t=CreateThread(NULL,0,audio_thread,NULL,0,NULL);if(t){SetThreadPriority(t,THREAD_PRIORITY_ABOVE_NORMAL);CloseHandle(t);}}
/* --- OpenGL --- */
static HMODULE glmod;
static void*getproc(const char*n){void*p=(void*)wglGetProcAddress(n);if(p==0||p==(void*)1||p==(void*)2||p==(void*)3||p==(void*)-1){if(!glmod)glmod=LoadLibraryA("opengl32.dll");p=(void*)GetProcAddress(glmod,n);}return p;}
typedef HGLRC(WINAPI*PFNCCA)(HDC,HGLRC,const int*);typedef BOOL(WINAPI*PFNSWAP)(int);
static void fail(const char*msg){wchar_t w[2200];MultiByteToWideChar(CP_UTF8,0,msg,-1,w,2200);MessageBoxW(hwnd,w,L"Тени Парижа",MB_ICONERROR|MB_OK);}
static int gl_start(void){PIXELFORMATDESCRIPTOR pfd;memset(&pfd,0,sizeof pfd);pfd.nSize=sizeof pfd;pfd.nVersion=1;pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;pfd.iPixelType=PFD_TYPE_RGBA;pfd.cColorBits=32;pfd.cDepthBits=24;pfd.cStencilBits=8;pfd.iLayerType=PFD_MAIN_PLANE;
 int pf=ChoosePixelFormat(hdc,&pfd);if(!pf||!SetPixelFormat(hdc,pf,&pfd))return 0;HGLRC tmp=wglCreateContext(hdc);if(!tmp||!wglMakeCurrent(hdc,tmp))return 0;
 PFNCCA cca=(PFNCCA)(void*)wglGetProcAddress("wglCreateContextAttribsARB");
 if(cca){int at[]={0x2091,3,0x2092,3,0x9126,1,0};hrc=cca(hdc,0,at);if(!hrc){int at2[]={0x2091,3,0x2092,3,0x9126,2,0};hrc=cca(hdc,0,at2);}}
 if(!hrc){hrc=tmp;}else{wglMakeCurrent(NULL,NULL);wglDeleteContext(tmp);if(!wglMakeCurrent(hdc,hrc))return 0;}
 const char*miss=gl_load(getproc);if(miss){char b[300];snprintf(b,sizeof b,"Видеокарта или драйвер не поддерживают OpenGL 3.3 (нет функции %s).\nОбновите драйвер видеокарты.",miss);fail(b);return -1;}
 PFNSWAP sw=(PFNSWAP)(void*)wglGetProcAddress("wglSwapIntervalEXT");if(sw)sw(1);
 if(r3_init()){char b[2400];snprintf(b,sizeof b,"Не удалось подготовить графику:\n%s",r3_error());fail(b);return -1;}return 1;}
int WINAPI WinMain(HINSTANCE hi,HINSTANCE hp,LPSTR cmd,int show){
 (void)hp;(void)cmd;(void)show;{SYSTEM_INFO si;GetSystemInfo(&si);ncores=(int)si.dwNumberOfProcessors;if(ncores<1)ncores=1;if(ncores>8)ncores=8;}pool_start();SetProcessDPIAware();timeBeginPeriod(1);
 WNDCLASSEXW wc;memset(&wc,0,sizeof wc);wc.cbSize=sizeof wc;wc.style=CS_OWNDC;wc.lpfnWndProc=wndproc;wc.hInstance=hi;wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.lpszClassName=L"TPWin";
 RegisterClassExW(&wc);RECT r={0,0,1600,900};{int sw=GetSystemMetrics(SM_CXSCREEN),sh=GetSystemMetrics(SM_CYSCREEN);if(sw<1700||sh<1000){r.right=1280;r.bottom=720;}}AdjustWindowRect(&r,WS_OVERLAPPEDWINDOW,FALSE);
 hwnd=CreateWindowExW(0,L"TPWin",L"Тени Парижа — 1789",WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,r.right-r.left,r.bottom-r.top,0,0,hi,0);
 if(!hwnd)return 1;hdc=GetDC(hwnd);
 int g=gl_start();if(g<=0){if(g==0)fail("Не удалось создать контекст OpenGL. Игре нужна видеокарта с поддержкой OpenGL 3.3 и свежий драйвер.");return 1;}
 {RECT cr;GetClientRect(hwnd,&cr);int s=(cr.bottom+180)/360;gfx_set_scale(s<1?1:s>4?4:s);}
 LARGE_INTEGER f,t0,t1;QueryPerformanceFrequency(&f);game_init();audio_start();QueryPerformanceCounter(&t0);double acc=0,slow=0;int dropped=0;
 while(running){
  MSG m;while(PeekMessageW(&m,0,0,0,PM_REMOVE)){if(m.message==WM_QUIT)running=0;TranslateMessage(&m);DispatchMessageW(&m);}
  {if(mouse_want&&GetForegroundWindow()==hwnd){RECT cr;GetClientRect(hwnd,&cr);POINT c={cr.right/2,cr.bottom/2},cur;ClientToScreen(hwnd,&c);GetCursorPos(&cur);mouse_dx+=cur.x-c.x;mouse_dy+=cur.y-c.y;SetCursorPos(c.x,c.y);if(!cursor_hidden){ShowCursor(FALSE);cursor_hidden=1;}}
   else if(cursor_hidden){ShowCursor(TRUE);cursor_hidden=0;}}
  QueryPerformanceCounter(&t1);double dt=(double)(t1.QuadPart-t0.QuadPart)/(double)f.QuadPart;t0=t1;if(dt>0.1)dt=0.1;acc+=dt;
  {static double fa=1.0/60;fa=fa*.95+dt*.05;g_fps=(float)(1.0/fa);
   if(!qual_manual&&gfx_quality>0){if(fa>1.0/26)slow+=dt;else slow=0;if(slow>4.0&&dropped<2){gfx_quality--;dropped++;slow=0;}}}
  int steps=0;while(acc>=1.0/60&&steps<5){game_update(1.0f/60);memset(key_hit,0,sizeof key_hit);mouse_dx=mouse_dy=0;acc-=1.0/60;steps++;}
  if(acc>1.0/60*5)acc=0;
  RECT cr;GetClientRect(hwnd,&cr);int cw=cr.right,ch=cr.bottom;if(cw<16||ch<16||IsIconic(hwnd)){Sleep(10);continue;}
  {int s=(ch+180)/360;s=s<1?1:s>4?4:s;if(s!=rs)gfx_set_scale(s);}
  r3_frame_begin(cw,ch);gfx_begin();game_render();platform_draw_text();r3_frame_end();SwapBuffers(hdc);}
 timeEndPeriod(1);return 0;}
