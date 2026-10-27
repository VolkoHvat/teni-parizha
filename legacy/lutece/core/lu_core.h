/* Lutèce Engine — ядро: журнал, память, время, файлы, потоки и задачи, профайлер. */
#ifndef LU_CORE_H
#define LU_CORE_H
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "lu_math.h"

#define LU_VERSION "0.1"
#define LU_ARRAY_N(a) ((int)(sizeof(a) / sizeof((a)[0])))
#define LU_UNUSED(x) (void)(x)

/* ---------- журнал ---------- */
enum { LU_LOG_INFO, LU_LOG_WARN, LU_LOG_ERR };
void lu_log(int level, const char *fmt, ...);
#define LU_INFO(...) lu_log(LU_LOG_INFO, __VA_ARGS__)
#define LU_WARN(...) lu_log(LU_LOG_WARN, __VA_ARGS__)
#define LU_ERR(...) lu_log(LU_LOG_ERR, __VA_ARGS__)
void lu_log_to_file(const char *path);
#define LU_ASSERT(c) do { if (!(c)) { lu_log(LU_LOG_ERR, "assert failed: %s (%s:%d)", #c, __FILE__, __LINE__); abort(); } } while (0)

/* ---------- память ---------- */
void *lu_alloc(size_t n);            /* обнулённая память, падение при нехватке */
void *lu_realloc(void *p, size_t n);
void lu_free(void *p);
char *lu_strdup(const char *s);
/* линейный распределитель: быстрые временные данные на кадр/загрузку */
typedef struct { uint8_t *base; size_t cap, used, peak; } lu_arena;
void lu_arena_init(lu_arena *a, size_t cap);
void *lu_arena_push(lu_arena *a, size_t n, size_t align);
void lu_arena_reset(lu_arena *a);
void lu_arena_free(lu_arena *a);
#define LU_PUSH(a, T, n) ((T *)lu_arena_push((a), sizeof(T) * (size_t)(n), _Alignof(T)))
/* динамический массив: struct { T* v; int n, cap; } */
#define LU_VEC(T) struct { T *v; int n, cap; }
#define lu_vec_push(vec, x) do { if ((vec).n >= (vec).cap) { (vec).cap = (vec).cap ? (vec).cap * 2 : 16; \
    (vec).v = lu_realloc((vec).v, sizeof(*(vec).v) * (size_t)(vec).cap); } (vec).v[(vec).n++] = (x); } while (0)
#define lu_vec_reserve(vec, k) do { if ((vec).cap < (k)) { (vec).cap = (k); (vec).v = lu_realloc((vec).v, sizeof(*(vec).v) * (size_t)(vec).cap); } } while (0)
#define lu_vec_free(vec) do { lu_free((vec).v); (vec).v = NULL; (vec).n = (vec).cap = 0; } while (0)

/* ---------- время ---------- */
double lu_time(void);  /* секунды, монотонно */
void lu_sleep_ms(int ms);

/* ---------- файлы ---------- */
void *lu_file_read(const char *path, size_t *size); /* +0 в конце; освобождать lu_free */
int lu_file_write(const char *path, const void *data, size_t size);

/* ---------- потоки и задачи ---------- */
typedef void (*lu_job_fn)(void *arg, int index);
int lu_jobs_init(int threads);       /* 0 — по числу ядер-1 */
void lu_jobs_shutdown(void);
int lu_jobs_threads(void);
/* выполнить fn(arg, i) для i=0..n-1 на всех ядрах, вернуться по завершении */
void lu_parallel_for(lu_job_fn fn, void *arg, int n);
/* асинхронная задача; счётчик уменьшается по завершении, lu_jobs_wait ждёт нуля (помогая выполнять задачи) */
typedef struct { volatile int pending; } lu_counter;
void lu_job_async(lu_job_fn fn, void *arg, int index, lu_counter *c);
void lu_jobs_wait(lu_counter *c);
typedef struct lu_mutex lu_mutex;
lu_mutex *lu_mutex_new(void);
void lu_mutex_lock(lu_mutex *m);
void lu_mutex_unlock(lu_mutex *m);
void lu_mutex_free(lu_mutex *m);
int lu_atomic_add(volatile int *p, int v); /* возвращает новое значение */

/* ---------- профайлер ---------- */
typedef struct { const char *name; double ms, avg; int calls; } lu_prof_entry;
int lu_prof_begin(const char *name);
void lu_prof_end(int id);
void lu_prof_frame(void);                       /* конец кадра: усреднение */
int lu_prof_list(const lu_prof_entry **out);
#define LU_PROF(name) for (int _lp = lu_prof_begin(name), _once = 1; _once; _once = 0, lu_prof_end(_lp))

/* ---------- строки ---------- */
int lu_utf8_next(const char **s); /* следующая кодовая точка, 0 в конце */
#endif
