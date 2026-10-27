/* Lutèce Engine — платформа Windows: окно Win32, OpenGL 3.3 core (WGL), Raw Input для мыши, XInput, waveOut. */
#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include "lu_platform.h"
#include "../gfx/lu_gl.h"

static HWND hwnd; static HDC hdc; static HGLRC hrc; static HMODULE glmod;
static int running, fullscreen, cur_hidden;
static WINDOWPLACEMENT wp_prev = {sizeof(WINDOWPLACEMENT), 0, 0, {0, 0}, {0, 0}, {0, 0, 0, 0}};
static lu_input in;
static char gl_info[256];

static void msg_box(const char *utf8) {
    wchar_t w[2048]; MultiByteToWideChar(CP_UTF8, 0, utf8, -1, w, 2048);
    MessageBoxW(hwnd, w, L"Lutèce", MB_ICONERROR | MB_OK);
}
static int map_vk(WPARAM vk) {
    if (vk >= 'A' && vk <= 'Z') return (int)vk;
    if (vk >= '0' && vk <= '9') return (int)vk;
    if (vk >= VK_F1 && vk <= VK_F12) return LU_KEY_F1 + (int)(vk - VK_F1);
    switch (vk) {
    case VK_SPACE: return LU_KEY_SPACE; case VK_RETURN: return LU_KEY_ENTER; case VK_ESCAPE: return LU_KEY_ESC;
    case VK_TAB: return LU_KEY_TAB; case VK_BACK: return LU_KEY_BACKSPACE; case VK_SHIFT: return LU_KEY_SHIFT;
    case VK_CONTROL: return LU_KEY_CTRL; case VK_MENU: return LU_KEY_ALT; case VK_UP: return LU_KEY_UP;
    case VK_DOWN: return LU_KEY_DOWN; case VK_LEFT: return LU_KEY_LEFT; case VK_RIGHT: return LU_KEY_RIGHT;
    case VK_OEM_3: return LU_KEY_GRAVE; case VK_HOME: return LU_KEY_HOME; case VK_END: return LU_KEY_END;
    case VK_PRIOR: return LU_KEY_PGUP; case VK_NEXT: return LU_KEY_PGDN; case VK_DELETE: return LU_KEY_DELETE;
    }
    return 0;
}
static void key_set(int k, int down) {
    if (k <= 0 || k >= LU_KEY_N) return;
    if (down && !in.down[k]) in.pressed[k] = 1;
    if (!down && in.down[k]) in.released[k] = 1;
    in.down[k] = (uint8_t)down;
}
void lu_input_inject(int key, int down) { key_set(key, down); }

static LRESULT CALLBACK wndproc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CLOSE: running = 0; return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_SYSKEYDOWN:
        if (w == VK_RETURN && (l & (1 << 29))) { lu_platform_fullscreen(!fullscreen); return 0; }
        if (w == VK_F10 || w == VK_MENU) { key_set(map_vk(w), 1); return 0; }
        break;
    case WM_SYSKEYUP: key_set(map_vk(w), 0); break;
    case WM_KEYDOWN: key_set(map_vk(w), 1); return 0;
    case WM_KEYUP: key_set(map_vk(w), 0); return 0;
    case WM_CHAR: if (w >= 32 && in.ntext < 32) in.text[in.ntext++] = (uint32_t)w; return 0;
    case WM_LBUTTONDOWN: key_set(LU_MOUSE_LEFT, 1); SetCapture(h); return 0;
    case WM_LBUTTONUP: key_set(LU_MOUSE_LEFT, 0); ReleaseCapture(); return 0;
    case WM_RBUTTONDOWN: key_set(LU_MOUSE_RIGHT, 1); return 0;
    case WM_RBUTTONUP: key_set(LU_MOUSE_RIGHT, 0); return 0;
    case WM_MBUTTONDOWN: key_set(LU_MOUSE_MIDDLE, 1); return 0;
    case WM_MBUTTONUP: key_set(LU_MOUSE_MIDDLE, 0); return 0;
    case WM_MOUSEWHEEL: in.wheel += (float)GET_WHEEL_DELTA_WPARAM(w) / WHEEL_DELTA; return 0;
    case WM_MOUSEMOVE: in.mouse_x = (short)LOWORD(l); in.mouse_y = (short)HIWORD(l); return 0;
    case WM_INPUT: {
        RAWINPUT ri; UINT sz = sizeof ri;
        if (GetRawInputData((HRAWINPUT)l, RID_INPUT, &ri, &sz, sizeof(RAWINPUTHEADER)) != (UINT)-1 &&
            ri.header.dwType == RIM_TYPEMOUSE && !(ri.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)) {
            in.mouse_dx += (float)ri.data.mouse.lLastX; in.mouse_dy += (float)ri.data.mouse.lLastY;
        }
        break;
    }
    case WM_KILLFOCUS: memset(in.down, 0, sizeof in.down); return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

/* ---------- XInput (загружается динамически: игра работает и без него) ---------- */
typedef struct { WORD wButtons; BYTE bLeftTrigger, bRightTrigger; SHORT sThumbLX, sThumbLY, sThumbRX, sThumbRY; } XPAD;
typedef struct { DWORD dwPacketNumber; XPAD Gamepad; } XSTATE;
typedef DWORD(WINAPI *PFN_XGET)(DWORD, XSTATE *);
static PFN_XGET xget; static double xpoll_t;
static float stick(SHORT v) { float f = v / 32767.f; float dz = .24f; if (fabsf(f) < dz) return 0; return (f - (f > 0 ? dz : -dz)) / (1 - dz); }
static void pad_poll(void) {
    if (!xget) return;
    uint32_t prev = in.pad_down; XSTATE s;
    if (xget(0, &s) != 0) { in.pad_connected = 0; in.pad_down = 0; return; }
    in.pad_connected = 1; XPAD *g = &s.Gamepad; uint32_t b = 0; WORD w = g->wButtons;
    static const WORD XB[14] = {0x1000, 0x2000, 0x4000, 0x8000, 0x0100, 0x0200, 0x0020, 0x0010, 0x0040, 0x0080, 1, 2, 4, 8};
    for (int i = 0; i < 14; i++) if (w & XB[i]) b |= 1u << i;
    in.pad_down = b; in.pad_pressed |= b & ~prev;
    in.pad_lx = stick(g->sThumbLX); in.pad_ly = stick(g->sThumbLY); in.pad_rx = stick(g->sThumbRX); in.pad_ry = stick(g->sThumbRY);
    in.pad_lt = g->bLeftTrigger / 255.f; in.pad_rt = g->bRightTrigger / 255.f;
}

/* ---------- OpenGL ---------- */
static void *getproc(const char *n) {
    void *p = (void *)wglGetProcAddress(n);
    if (!p || p == (void *)1 || p == (void *)2 || p == (void *)3 || p == (void *)-1) {
        if (!glmod) glmod = LoadLibraryA("opengl32.dll");
        p = (void *)GetProcAddress(glmod, n);
    }
    return p;
}
typedef HGLRC(WINAPI *PFN_CCA)(HDC, HGLRC, const int *);
typedef BOOL(WINAPI *PFN_SWAPI)(int);
static int gl_start(int vsync) {
    PIXELFORMATDESCRIPTOR pfd; memset(&pfd, 0, sizeof pfd);
    pfd.nSize = sizeof pfd; pfd.nVersion = 1; pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 32; pfd.cDepthBits = 24; pfd.cStencilBits = 8; pfd.iLayerType = PFD_MAIN_PLANE;
    int pf = ChoosePixelFormat(hdc, &pfd); if (!pf || !SetPixelFormat(hdc, pf, &pfd)) return 0;
    HGLRC tmp = wglCreateContext(hdc); if (!tmp || !wglMakeCurrent(hdc, tmp)) return 0;
    PFN_CCA cca = (PFN_CCA)(void *)wglGetProcAddress("wglCreateContextAttribsARB");
    if (cca) {
        int at[] = {0x2091, 3, 0x2092, 3, 0x9126, 1, 0};            /* 3.3 core */
        hrc = cca(hdc, 0, at);
        if (!hrc) { int at2[] = {0x2091, 3, 0x2092, 3, 0x9126, 2, 0}; hrc = cca(hdc, 0, at2); } /* совместимый профиль */
    }
    if (!hrc) hrc = tmp; else { wglMakeCurrent(NULL, NULL); wglDeleteContext(tmp); if (!wglMakeCurrent(hdc, hrc)) return 0; }
    const char *miss = lu_gl_load(getproc);
    if (miss) {
        char b[400]; snprintf(b, sizeof b, "Видеокарта или драйвер не поддерживают OpenGL 3.3 (нет функции %s).\nОбновите драйвер видеокарты.", miss);
        msg_box(b); return -1;
    }
    PFN_SWAPI sw = (PFN_SWAPI)(void *)wglGetProcAddress("wglSwapIntervalEXT"); if (sw) sw(vsync ? 1 : 0);
    snprintf(gl_info, sizeof gl_info, "%s | %s", (const char *)glGetString(GL_RENDERER), (const char *)glGetString(GL_VERSION));
    return 1;
}

int lu_platform_init(const lu_window_desc *d) {
    SetProcessDPIAware(); timeBeginPeriod(1);
    HINSTANCE hi = GetModuleHandleW(NULL);
    WNDCLASSEXW wc; memset(&wc, 0, sizeof wc);
    wc.cbSize = sizeof wc; wc.style = CS_OWNDC; wc.lpfnWndProc = wndproc; wc.hInstance = hi;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.hIcon = LoadIcon(NULL, IDI_APPLICATION); wc.lpszClassName = L"LuteceWnd";
    RegisterClassExW(&wc);
    int w = d->width > 0 ? d->width : 1600, h = d->height > 0 ? d->height : 900;
    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    if (w > sw - 80 || h > sh - 100) { w = sw * 3 / 4; h = w * 9 / 16; }
    RECT r = {0, 0, w, h}; AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    wchar_t title[256]; MultiByteToWideChar(CP_UTF8, 0, d->title ? d->title : "Lutèce", -1, title, 256);
    hwnd = CreateWindowExW(0, L"LuteceWnd", title, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                           r.right - r.left, r.bottom - r.top, 0, 0, hi, 0);
    if (!hwnd) return 1;
    hdc = GetDC(hwnd);
    int g = gl_start(d->vsync);
    if (g <= 0) { if (g == 0) msg_box("Не удалось создать контекст OpenGL. Нужна видеокарта с поддержкой OpenGL 3.3 и свежий драйвер."); return 2; }
    RAWINPUTDEVICE rid = {0x01, 0x02, 0, hwnd}; RegisterRawInputDevices(&rid, 1, sizeof rid);
    HMODULE xi = LoadLibraryA("xinput1_4.dll"); if (!xi) xi = LoadLibraryA("xinput9_1_0.dll"); if (!xi) xi = LoadLibraryA("xinput1_3.dll");
    if (xi) xget = (PFN_XGET)(void *)GetProcAddress(xi, "XInputGetState");
    running = 1; if (d->fullscreen) lu_platform_fullscreen(1);
    return 0;
}
void lu_platform_shutdown(void) {
    lu_audio_stop();
    if (hrc) { wglMakeCurrent(NULL, NULL); wglDeleteContext(hrc); hrc = 0; }
    if (hwnd) { ReleaseDC(hwnd, hdc); DestroyWindow(hwnd); hwnd = 0; }
    timeEndPeriod(1);
}
int lu_platform_pump(void) {
    memset(in.pressed, 0, sizeof in.pressed); memset(in.released, 0, sizeof in.released);
    in.mouse_dx = in.mouse_dy = in.wheel = 0; in.ntext = 0; in.pad_pressed = 0;
    MSG m;
    while (PeekMessageW(&m, 0, 0, 0, PM_REMOVE)) { if (m.message == WM_QUIT) running = 0; TranslateMessage(&m); DispatchMessageW(&m); }
    double t = lu_time(); if (t - xpoll_t > 1.0 / 120) { pad_poll(); xpoll_t = t; }
    /* захват мыши: курсор скрыт и удерживается в центре окна */
    if (in.mouse_captured && GetForegroundWindow() == hwnd) {
        RECT cr; GetClientRect(hwnd, &cr); POINT c = {cr.right / 2, cr.bottom / 2}; ClientToScreen(hwnd, &c); SetCursorPos(c.x, c.y);
        if (!cur_hidden) { ShowCursor(FALSE); cur_hidden = 1; }
    } else if (cur_hidden) { ShowCursor(TRUE); cur_hidden = 0; }
    if (!in.mouse_captured || GetForegroundWindow() != hwnd) { in.mouse_dx = in.mouse_dy = 0; }
    return running;
}
void lu_platform_swap(void) { SwapBuffers(hdc); }
void lu_platform_size(int *w, int *h) { RECT r; GetClientRect(hwnd, &r); *w = r.right; *h = r.bottom; if (IsIconic(hwnd)) *w = *h = 0; }
unsigned lu_platform_fbo(void) { return 0; }
void lu_platform_fullscreen(int on) {
    DWORD st = (DWORD)GetWindowLongW(hwnd, GWL_STYLE);
    if (on && !fullscreen) {
        MONITORINFO mi = {sizeof(mi), {0, 0, 0, 0}, {0, 0, 0, 0}, 0}; GetWindowPlacement(hwnd, &wp_prev);
        if (GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
            SetWindowLongW(hwnd, GWL_STYLE, (LONG)(st & ~WS_OVERLAPPEDWINDOW));
            SetWindowPos(hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            fullscreen = 1;
        }
    } else if (!on && fullscreen) {
        SetWindowLongW(hwnd, GWL_STYLE, (LONG)(st | WS_OVERLAPPEDWINDOW)); SetWindowPlacement(hwnd, &wp_prev);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        fullscreen = 0;
    }
}
int lu_platform_is_fullscreen(void) { return fullscreen; }
void lu_platform_title(const char *utf8) { wchar_t w[256]; MultiByteToWideChar(CP_UTF8, 0, utf8, -1, w, 256); SetWindowTextW(hwnd, w); }
void lu_platform_quit(void) { running = 0; }
const char *lu_platform_gl_info(void) { return gl_info; }
const lu_input *lu_input_get(void) { return &in; }
void lu_input_capture_mouse(int on) { in.mouse_captured = on; }

/* ---------- звук: waveOut, 16 бит, 4 буфера по 1024 кадра, отдельный поток ---------- */
#define AFR 1024
static HWAVEOUT hwo; static WAVEHDR whdr[4]; static short abuf[4][AFR * 2]; static float fbuf[AFR * 2];
static HANDLE aev; static volatile int a_run; static lu_audio_fn a_fn; static void *a_user;
static void a_fill(short *dst) {
    memset(fbuf, 0, sizeof fbuf); a_fn(fbuf, AFR, a_user);
    for (int i = 0; i < AFR * 2; i++) { float v = fbuf[i]; v = v > 1 ? 1 : v < -1 ? -1 : v; dst[i] = (short)(v * 32000); }
}
static DWORD WINAPI audio_thread(LPVOID p) {
    LU_UNUSED(p);
    for (int i = 0; i < 4; i++) {
        a_fill(abuf[i]); memset(&whdr[i], 0, sizeof(WAVEHDR));
        whdr[i].lpData = (LPSTR)abuf[i]; whdr[i].dwBufferLength = AFR * 4;
        if (waveOutPrepareHeader(hwo, &whdr[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) return 0;
        waveOutWrite(hwo, &whdr[i], sizeof(WAVEHDR));
    }
    while (a_run) {
        WaitForSingleObject(aev, 50);
        for (int i = 0; i < 4; i++) if (whdr[i].dwFlags & WHDR_DONE) { a_fill(abuf[i]); waveOutWrite(hwo, &whdr[i], sizeof(WAVEHDR)); }
    }
    return 0;
}
int lu_audio_start(int rate, lu_audio_fn fn, void *user) {
    WAVEFORMATEX wf; memset(&wf, 0, sizeof wf);
    wf.wFormatTag = WAVE_FORMAT_PCM; wf.nChannels = 2; wf.nSamplesPerSec = (DWORD)rate; wf.wBitsPerSample = 16;
    wf.nBlockAlign = 4; wf.nAvgBytesPerSec = (DWORD)rate * 4;
    a_fn = fn; a_user = user;
    aev = CreateEventW(NULL, FALSE, FALSE, NULL); if (!aev) return 1;
    if (waveOutOpen(&hwo, WAVE_MAPPER, &wf, (DWORD_PTR)aev, 0, CALLBACK_EVENT) != MMSYSERR_NOERROR) return 2;
    a_run = 1;
    HANDLE t = CreateThread(NULL, 0, audio_thread, NULL, 0, NULL);
    if (t) { SetThreadPriority(t, THREAD_PRIORITY_ABOVE_NORMAL); CloseHandle(t); }
    return 0;
}
void lu_audio_stop(void) { if (!a_run) return; a_run = 0; Sleep(60); waveOutReset(hwo); waveOutClose(hwo); }

/* точка входа: приложение определяет lu_main */
int lu_main(int argc, char **argv);
int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cmd, int show) {
    LU_UNUSED(hi); LU_UNUSED(hp); LU_UNUSED(cmd); LU_UNUSED(show);
    return lu_main(__argc, __argv);
}
