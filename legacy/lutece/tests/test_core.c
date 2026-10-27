/* Модульные тесты ядра Lutèce */
#include "../core/lu_core.h"
#include <stdio.h>
static int fails;
#define CHECK(c) do { if (!(c)) { printf("  ПРОВАЛ: %s (строка %d)\n", #c, __LINE__); fails++; } } while (0)
static int near(float a, float b, float e) { return fabsf(a - b) <= e; }
static int v3near(v3 a, v3 b, float e) { return near(a.x, b.x, e) && near(a.y, b.y, e) && near(a.z, b.z, e); }
static volatile int sum_par[1000];
static void job_sq(void *arg, int i) { LU_UNUSED(arg); sum_par[i] = i * i; }
static volatile int async_hits;
static void job_inc(void *arg, int i) { LU_UNUSED(arg); LU_UNUSED(i); lu_atomic_add(&async_hits, 1); }
int main(void) {
    printf("математика…\n");
    quat q = quat_axis(V3(0, 1, 0), LU_PI / 2);
    CHECK(v3near(quat_rot(q, V3(1, 0, 0)), V3(0, 0, -1), 1e-5f));
    m4 R = m4_from_quat(q);
    CHECK(v3near(m4_point(R, V3(1, 0, 0)), V3(0, 0, -1), 1e-5f));
    m4 T = m4_mul(m4_translate(V3(1, 2, 3)), m4_mul(R, m4_scale(V3(2, 2, 2))));
    m4 Ti = m4_inverse(T); v3 p = V3(.3f, -1, 4);
    CHECK(v3near(m4_point(Ti, m4_point(T, p)), p, 1e-4f));
    xform a = {V3(1, 0, 0), quat_axis(V3(0, 0, 1), .7f), V3(1, 1, 1)}, b = {V3(0, 2, 0), quat_axis(V3(1, 0, 0), -.3f), V3(1, 1, 1)};
    xform ab = xf_mul(a, b);
    CHECK(v3near(xf_point(ab, p), xf_point(a, xf_point(b, p)), 1e-4f));
    CHECK(v3near(m4_point(m4_from_xform(ab), p), xf_point(ab, p), 1e-4f));
    CHECK(v3near(xf_point(xf_inv(ab), xf_point(ab, p)), p, 1e-4f));
    quat s = quat_slerp(Q_ID(), q, .5f);
    CHECK(v3near(quat_rot(s, V3(1, 0, 0)), V3(cosf(LU_PI / 4), 0, -sinf(LU_PI / 4)), 1e-4f));
    quat ft = quat_from_to(V3(1, 0, 0), V3(0, 1, 0));
    CHECK(v3near(quat_rot(ft, V3(1, 0, 0)), V3(0, 1, 0), 1e-5f));
    m4 P = m4_persp(1.f, 16.f / 9, .1f, 100.f), Vw = m4_look(V3(0, 0, 5), V3(0, 0, 0), V3(0, 1, 0));
    frustum fr = frustum_from(m4_mul(P, Vw));
    CHECK(frustum_sphere(&fr, V3(0, 0, 0), .5f));
    CHECK(!frustum_sphere(&fr, V3(0, 0, 10), .5f));
    CHECK(!frustum_sphere(&fr, V3(50, 0, 0), .5f));
    aabb bx = {V3(-1, -1, -1), V3(1, 1, 1)};
    CHECK(frustum_aabb(&fr, bx));
    printf("память…\n");
    lu_arena ar; lu_arena_init(&ar, 1 << 16);
    int *ia = LU_PUSH(&ar, int, 100); double *da = LU_PUSH(&ar, double, 10);
    CHECK(((uintptr_t)da & 7) == 0); ia[99] = 5; CHECK(ia[99] == 5); lu_arena_reset(&ar); CHECK(ar.used == 0); lu_arena_free(&ar);
    LU_VEC(int) vv = {0}; for (int i = 0; i < 1000; i++) lu_vec_push(vv, i); CHECK(vv.n == 1000 && vv.v[999] == 999); lu_vec_free(vv);
    printf("задачи…\n");
    lu_jobs_init(0);
    lu_parallel_for(job_sq, NULL, 1000);
    int ok = 1; for (int i = 0; i < 1000; i++) if (sum_par[i] != i * i) ok = 0; CHECK(ok);
    lu_counter c = {0}; for (int i = 0; i < 5000; i++) lu_job_async(job_inc, NULL, i, &c);
    lu_jobs_wait(&c); CHECK(async_hits == 5000);
    printf("строки…\n");
    const char *u = "Лютеция"; int n = 0, cp, first = 0; while ((cp = lu_utf8_next(&u))) { if (!n) first = cp; n++; }
    CHECK(n == 7 && first == 0x41B);
    LU_PROF("тест") { lu_sleep_ms(1); }
    const lu_prof_entry *pe; CHECK(lu_prof_list(&pe) == 1 && pe[0].ms >= .5);
    printf(fails ? "ЕСТЬ ОШИБКИ: %d\n" : "все тесты ядра пройдены\n", fails);
    lu_jobs_shutdown();
    return fails != 0;
}
