/* Lutèce Engine — реализация ядра */
#include "lu_core.h"
#include <stdio.h>
#include <stdarg.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#endif

/* ================= журнал ================= */
static FILE *log_file;
void lu_log_to_file(const char *path) { if (log_file) fclose(log_file); log_file = fopen(path, "w"); }
void lu_log(int level, const char *fmt, ...) {
    static const char *tag[3] = {"", "[предупреждение] ", "[ошибка] "};
    char buf[2048]; va_list a; va_start(a, fmt); vsnprintf(buf, sizeof buf, fmt, a); va_end(a);
    FILE *o = level == LU_LOG_ERR ? stderr : stdout;
    fprintf(o, "%s%s\n", tag[level < 0 || level > 2 ? 0 : level], buf); fflush(o);
    if (log_file) { fprintf(log_file, "%s%s\n", tag[level < 0 || level > 2 ? 0 : level], buf); fflush(log_file); }
}

/* ================= память ================= */
void *lu_alloc(size_t n) { void *p = calloc(1, n ? n : 1); if (!p) { LU_ERR("нет памяти (%zu байт)", n); abort(); } return p; }
void *lu_realloc(void *p, size_t n) { void *q = realloc(p, n ? n : 1); if (!q) { LU_ERR("нет памяти (%zu байт)", n); abort(); } return q; }
void lu_free(void *p) { free(p); }
char *lu_strdup(const char *s) { size_t n = strlen(s) + 1; char *r = lu_alloc(n); memcpy(r, s, n); return r; }
void lu_arena_init(lu_arena *a, size_t cap) { a->base = lu_alloc(cap); a->cap = cap; a->used = a->peak = 0; }
void *lu_arena_push(lu_arena *a, size_t n, size_t align) {
    size_t at = (a->used + align - 1) & ~(align - 1);
    if (at + n > a->cap) { LU_ERR("арена переполнена: нужно %zu, есть %zu", at + n, a->cap); abort(); }
    a->used = at + n; if (a->used > a->peak) a->peak = a->used;
    void *p = a->base + at; memset(p, 0, n); return p;
}
void lu_arena_reset(lu_arena *a) { a->used = 0; }
void lu_arena_free(lu_arena *a) { lu_free(a->base); memset(a, 0, sizeof *a); }

/* ================= время ================= */
double lu_time(void) {
#ifdef _WIN32
    static LARGE_INTEGER f; LARGE_INTEGER c; if (!f.QuadPart) QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c); return (double)c.QuadPart / (double)f.QuadPart;
#else
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9;
#endif
}
void lu_sleep_ms(int ms) {
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    usleep((useconds_t)ms * 1000);
#endif
}

/* ================= файлы ================= */
void *lu_file_read(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb"); if (!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return NULL; }
    char *p = lu_alloc((size_t)n + 1);
    if (fread(p, 1, (size_t)n, f) != (size_t)n) { fclose(f); lu_free(p); return NULL; }
    fclose(f); if (size) *size = (size_t)n; return p;
}
int lu_file_write(const char *path, const void *data, size_t size) {
    FILE *f = fopen(path, "wb"); if (!f) return 0;
    int ok = fwrite(data, 1, size, f) == size; fclose(f); return ok;
}

/* ================= мьютексы, условные переменные, атомики ================= */
#ifdef _WIN32
struct lu_mutex { CRITICAL_SECTION cs; };
typedef CONDITION_VARIABLE lu_cond;
static void cond_init(lu_cond *c) { InitializeConditionVariable(c); }
static void cond_wait(lu_cond *c, lu_mutex *m) { SleepConditionVariableCS(c, &m->cs, INFINITE); }
static void cond_signal(lu_cond *c) { WakeConditionVariable(c); }
static void cond_broadcast(lu_cond *c) { WakeAllConditionVariable(c); }
lu_mutex *lu_mutex_new(void) { lu_mutex *m = lu_alloc(sizeof *m); InitializeCriticalSection(&m->cs); return m; }
void lu_mutex_lock(lu_mutex *m) { EnterCriticalSection(&m->cs); }
void lu_mutex_unlock(lu_mutex *m) { LeaveCriticalSection(&m->cs); }
void lu_mutex_free(lu_mutex *m) { if (!m) return; DeleteCriticalSection(&m->cs); lu_free(m); }
int lu_atomic_add(volatile int *p, int v) { return (int)InterlockedExchangeAdd((volatile LONG *)p, v) + v; }
#else
struct lu_mutex { pthread_mutex_t m; };
typedef pthread_cond_t lu_cond;
static void cond_init(lu_cond *c) { pthread_cond_init(c, NULL); }
static void cond_wait(lu_cond *c, lu_mutex *m) { pthread_cond_wait(c, &m->m); }
static void cond_signal(lu_cond *c) { pthread_cond_signal(c); }
static void cond_broadcast(lu_cond *c) { pthread_cond_broadcast(c); }
lu_mutex *lu_mutex_new(void) { lu_mutex *m = lu_alloc(sizeof *m); pthread_mutex_init(&m->m, NULL); return m; }
void lu_mutex_lock(lu_mutex *m) { pthread_mutex_lock(&m->m); }
void lu_mutex_unlock(lu_mutex *m) { pthread_mutex_unlock(&m->m); }
void lu_mutex_free(lu_mutex *m) { if (!m) return; pthread_mutex_destroy(&m->m); lu_free(m); }
int lu_atomic_add(volatile int *p, int v) { return __atomic_add_fetch(p, v, __ATOMIC_SEQ_CST); }
#endif

/* ================= система задач ================= */
typedef struct { lu_job_fn fn; void *arg; int index; lu_counter *c; } Job;
#define JQ_CAP 4096
static Job jq[JQ_CAP];
static int jq_head, jq_tail, jq_count;
static lu_mutex *jq_mx;
static lu_cond jq_cv, jq_done_cv;
static int nthreads, jobs_quit;

static int job_pop(Job *out) { /* под замком */
    if (!jq_count) return 0;
    *out = jq[jq_head]; jq_head = (jq_head + 1) % JQ_CAP; jq_count--; return 1;
}
static void job_run(Job *j) {
    j->fn(j->arg, j->index);
    if (j->c) {
        if (lu_atomic_add(&j->c->pending, -1) == 0) { lu_mutex_lock(jq_mx); cond_broadcast(&jq_done_cv); lu_mutex_unlock(jq_mx); }
    }
}
#ifdef _WIN32
static DWORD WINAPI worker(LPVOID p)
#else
static void *worker(void *p)
#endif
{
    LU_UNUSED(p);
    for (;;) {
        Job j; lu_mutex_lock(jq_mx);
        while (!jq_count && !jobs_quit) cond_wait(&jq_cv, jq_mx);
        if (jobs_quit && !jq_count) { lu_mutex_unlock(jq_mx); break; }
        job_pop(&j); lu_mutex_unlock(jq_mx);
        job_run(&j);
    }
    return 0;
}
int lu_jobs_init(int threads) {
    if (jq_mx) return nthreads;
    if (threads <= 0) {
#ifdef _WIN32
        SYSTEM_INFO si; GetSystemInfo(&si); threads = (int)si.dwNumberOfProcessors - 1;
#else
        threads = (int)sysconf(_SC_NPROCESSORS_ONLN) - 1;
#endif
    }
    if (threads < 0) threads = 0;
    if (threads > 31) threads = 31;
    jq_mx = lu_mutex_new(); cond_init(&jq_cv); cond_init(&jq_done_cv); jobs_quit = 0;
    for (int i = 0; i < threads; i++) {
#ifdef _WIN32
        HANDLE h = CreateThread(NULL, 0, worker, NULL, 0, NULL); if (!h) break; CloseHandle(h);
#else
        pthread_t t; if (pthread_create(&t, NULL, worker, NULL)) break; pthread_detach(t);
#endif
        nthreads++;
    }
    LU_INFO("Lutèce: потоков-исполнителей %d", nthreads);
    return nthreads;
}
void lu_jobs_shutdown(void) {
    if (!jq_mx) return;
    lu_mutex_lock(jq_mx); jobs_quit = 1; cond_broadcast(&jq_cv); lu_mutex_unlock(jq_mx);
    lu_sleep_ms(5);
}
int lu_jobs_threads(void) { return nthreads; }
void lu_job_async(lu_job_fn fn, void *arg, int index, lu_counter *c) {
    if (c) lu_atomic_add(&c->pending, 1);
    if (!jq_mx || !nthreads) { Job j = {fn, arg, index, c}; job_run(&j); return; }
    lu_mutex_lock(jq_mx);
    while (jq_count >= JQ_CAP) { /* очередь полна — выполняем сами */
        Job j; job_pop(&j); lu_mutex_unlock(jq_mx); job_run(&j); lu_mutex_lock(jq_mx);
    }
    jq[jq_tail] = (Job){fn, arg, index, c}; jq_tail = (jq_tail + 1) % JQ_CAP; jq_count++;
    cond_signal(&jq_cv); lu_mutex_unlock(jq_mx);
}
void lu_jobs_wait(lu_counter *c) {
    while (c->pending > 0) {
        Job j; int got = 0;
        lu_mutex_lock(jq_mx);
        if (jq_count) got = job_pop(&j);
        else if (c->pending > 0) cond_wait(&jq_done_cv, jq_mx);
        lu_mutex_unlock(jq_mx);
        if (got) job_run(&j);
    }
}
typedef struct { lu_job_fn fn; void *arg; int n; volatile int next; } PFor;
static void pfor_body(void *p, int unused) {
    LU_UNUSED(unused); PFor *pf = p;
    for (;;) { int i = lu_atomic_add(&pf->next, 1) - 1; if (i >= pf->n) break; pf->fn(pf->arg, i); }
}
void lu_parallel_for(lu_job_fn fn, void *arg, int n) {
    if (n <= 0) return;
    if (!jq_mx || !nthreads || n == 1) { for (int i = 0; i < n; i++) fn(arg, i); return; }
    PFor pf = {fn, arg, n, 0}; lu_counter c = {0};
    int helpers = nthreads < n - 1 ? nthreads : n - 1;
    for (int k = 0; k < helpers; k++) lu_job_async(pfor_body, &pf, 0, &c);
    pfor_body(&pf, 0);
    lu_jobs_wait(&c);
}

/* ================= профайлер ================= */
#define PROF_MAX 64
static lu_prof_entry prof[PROF_MAX]; static int nprof; static double prof_t0[PROF_MAX];
int lu_prof_begin(const char *name) {
    int i; for (i = 0; i < nprof; i++) if (prof[i].name == name || !strcmp(prof[i].name, name)) break;
    if (i == nprof) { if (nprof >= PROF_MAX) return -1; prof[nprof].name = name; nprof++; }
    prof_t0[i] = lu_time(); return i;
}
void lu_prof_end(int id) { if (id < 0) return; prof[id].ms += (lu_time() - prof_t0[id]) * 1000.0; prof[id].calls++; }
void lu_prof_frame(void) { for (int i = 0; i < nprof; i++) { prof[i].avg = prof[i].avg * .9 + prof[i].ms * .1; prof[i].ms = 0; prof[i].calls = 0; } }
int lu_prof_list(const lu_prof_entry **out) { *out = prof; return nprof; }

/* ================= строки ================= */
int lu_utf8_next(const char **ps) {
    const unsigned char *s = (const unsigned char *)*ps; int c = *s;
    if (!c) return 0;
    if (c < 0x80) { *ps += 1; return c; }
    if ((c & 0xE0) == 0xC0 && s[1]) { *ps += 2; return ((c & 0x1F) << 6) | (s[1] & 0x3F); }
    if ((c & 0xF0) == 0xE0 && s[1] && s[2]) { *ps += 3; return ((c & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F); }
    if ((c & 0xF8) == 0xF0 && s[1] && s[2] && s[3]) { *ps += 4; return ((c & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F); }
    *ps += 1; return '?';
}

/* ================= математика (не-inline часть) ================= */
m4 m4_inverse(m4 a) {
    const float *m = a.m; float inv[16];
    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];
    float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    m4 r; if (fabsf(det) < 1e-20f) return m4_id();
    det = 1.f / det; for (int i = 0; i < 16; i++) r.m[i] = inv[i] * det; return r;
}
aabb aabb_transform(aabb b, m4 m) {
    v3 c = aabb_center(b), h = aabb_half(b), nc = m4_point(m, c), nh;
    nh.x = fabsf(m.m[0]) * h.x + fabsf(m.m[4]) * h.y + fabsf(m.m[8]) * h.z;
    nh.y = fabsf(m.m[1]) * h.x + fabsf(m.m[5]) * h.y + fabsf(m.m[9]) * h.z;
    nh.z = fabsf(m.m[2]) * h.x + fabsf(m.m[6]) * h.y + fabsf(m.m[10]) * h.z;
    aabb r = {v3_sub(nc, nh), v3_add(nc, nh)}; return r;
}
frustum frustum_from(m4 vp) {
    frustum f; const float *m = vp.m;
    /* строки матрицы (column-major): row i = m[i], m[4+i], m[8+i], m[12+i] */
    v4 r0 = V4(m[0], m[4], m[8], m[12]), r1 = V4(m[1], m[5], m[9], m[13]), r2 = V4(m[2], m[6], m[10], m[14]), r3 = V4(m[3], m[7], m[11], m[15]);
    v4 p[6] = {{r3.x + r0.x, r3.y + r0.y, r3.z + r0.z, r3.w + r0.w}, {r3.x - r0.x, r3.y - r0.y, r3.z - r0.z, r3.w - r0.w},
               {r3.x + r1.x, r3.y + r1.y, r3.z + r1.z, r3.w + r1.w}, {r3.x - r1.x, r3.y - r1.y, r3.z - r1.z, r3.w - r1.w},
               {r3.x + r2.x, r3.y + r2.y, r3.z + r2.z, r3.w + r2.w}, {r3.x - r2.x, r3.y - r2.y, r3.z - r2.z, r3.w - r2.w}};
    for (int i = 0; i < 6; i++) { float l = sqrtf(p[i].x * p[i].x + p[i].y * p[i].y + p[i].z * p[i].z); if (l > 0) { p[i].x /= l; p[i].y /= l; p[i].z /= l; p[i].w /= l; } f.p[i] = p[i]; }
    return f;
}
int frustum_aabb(const frustum *f, aabb b) {
    for (int i = 0; i < 6; i++) {
        const v4 *p = &f->p[i];
        v3 v = V3(p->x > 0 ? b.mx.x : b.mn.x, p->y > 0 ? b.mx.y : b.mn.y, p->z > 0 ? b.mx.z : b.mn.z);
        if (p->x * v.x + p->y * v.y + p->z * v.z + p->w < 0) return 0;
    }
    return 1;
}
int frustum_sphere(const frustum *f, v3 c, float r) {
    for (int i = 0; i < 6; i++) { const v4 *p = &f->p[i]; if (p->x * c.x + p->y * c.y + p->z * c.z + p->w < -r) return 0; }
    return 1;
}
