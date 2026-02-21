/*
 * Rigorous test suite for stb_aloctr (aloctr).
 * Compile with STB_ALOCTR_IMPLEMENTATION and run; exit 0 = all pass, exit 1 = failure.
 */
#define STB_ALOCTR_IMPLEMENTATION
#include "../stb_aloctr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_failed;
static int g_run;

#define ASSERT(c) do { \
    if (!(c)) { \
        fprintf(stderr, "  FAIL: %s:%d: %s\n", __FILE__, __LINE__, #c); \
        g_failed++; \
    } \
} while (0)

#define RUN(name) do { \
    g_run++; \
    fprintf(stderr, "  [%d] %s\n", g_run, name); \
} while (0)

/* ---- static_init ---- */
static void test_static_init_valid(void)
{
    RUN("static_init valid buffer");
    aloctr aloc = {0};
    uint8_t buf[256];
    bool ok = aloctr_static_init(&aloc, sizeof(buf), buf);
    ASSERT(ok);
    ASSERT(aloc.buffer == buf);
    ASSERT(aloc.count == 0);
    ASSERT(aloc.capacity == sizeof(buf));
    ASSERT(aloc.is_resizable == false);
    aloctr_free(&aloc);
}

static void test_static_init_null_aloc(void)
{
    RUN("static_init NULL aloc");
    uint8_t buf[64];
    bool ok = aloctr_static_init(NULL, sizeof(buf), buf);
    ASSERT(!ok);
}

static void test_static_init_zero_len(void)
{
    RUN("static_init zero-length buffer");
    aloctr aloc = {0};
    uint8_t buf[1];
    bool ok = aloctr_static_init(&aloc, 0, buf);
    ASSERT(ok);
    ASSERT(aloc.capacity == 0);
    aloctr_free(&aloc);
}

/* ---- dynamic_init ---- */
static void test_dynamic_init_valid(void)
{
    RUN("dynamic_init valid");
    aloctr aloc = {0};
    bool ok = aloctr_dynamic_init(&aloc);
    ASSERT(ok);
    ASSERT(aloc.buffer != NULL);
    ASSERT(aloc.count == 0);
    ASSERT(aloc.capacity > 0);
    ASSERT(aloc.is_resizable == true);
    aloctr_free(&aloc);
}

static void test_dynamic_init_null_aloc(void)
{
    RUN("dynamic_init NULL aloc");
    bool ok = aloctr_dynamic_init(NULL);
    ASSERT(!ok);
}

/* ---- aloctr_alloc (null, zero size) ---- */
static void test_alloc_null_aloc(void)
{
    RUN("alloc NULL aloc returns NULL");
    void *p = aloctr_alloc(NULL, 8);
    ASSERT(p == NULL);
}

static void test_alloc_zero_size(void)
{
    RUN("alloc size 0 returns NULL");
    aloctr aloc = {0};
    uint8_t buf[64];
    aloctr_static_init(&aloc, sizeof(buf), buf);
    void *p = aloctr_alloc(&aloc, 0);
    ASSERT(p == NULL);
    aloctr_free(&aloc);
}

/* ---- aloctr_alloc basic ---- */
static void test_alloc_single_block(void)
{
    RUN("alloc single block writable and readable");
    aloctr aloc = {0};
    uint8_t buf[64];
    aloctr_static_init(&aloc, sizeof(buf), buf);
    uint32_t *p = (uint32_t *)aloctr_alloc(&aloc, sizeof(uint32_t));
    ASSERT(p != NULL);
    ASSERT((uint8_t *)p >= buf && (uint8_t *)p < buf + sizeof(buf));
    *p = 0xDEADBEEF;
    ASSERT(*p == 0xDEADBEEFU);
    aloctr_free(&aloc);
}

static void test_alloc_multiple_blocks(void)
{
    RUN("alloc multiple blocks distinct and independent");
    aloctr aloc = {0};
    uint8_t buf[256];
    aloctr_static_init(&aloc, sizeof(buf), buf);
    void *a = aloctr_alloc(&aloc, 16);
    void *b = aloctr_alloc(&aloc, 32);
    void *c = aloctr_alloc(&aloc, 8);
    ASSERT(a != NULL && b != NULL && c != NULL);
    ASSERT(a != b && b != c && a != c);
    memset(a, 0xAA, 16);
    memset(b, 0xBB, 32);
    memset(c, 0xCC, 8);
    ASSERT(*(uint8_t *)a == 0xAA && *(uint8_t *)b == 0xBB && *(uint8_t *)c == 0xCC);
    aloctr_free(&aloc);
}

static void test_alloc_static_full_returns_null(void)
{
    RUN("alloc on full static arena returns NULL");
    aloctr aloc = {0};
    uint8_t buf[32];
    aloctr_static_init(&aloc, sizeof(buf), buf);
    void *p1 = aloctr_alloc(&aloc, 32);
    ASSERT(p1 != NULL);
    void *p2 = aloctr_alloc(&aloc, 1);
    ASSERT(p2 == NULL);
    aloctr_free(&aloc);
}

static void test_alloc_dynamic_resizes(void)
{
    RUN("alloc in dynamic arena triggers resize; new allocations valid");
    aloctr aloc = {0};
    ASSERT(aloctr_dynamic_init(&aloc));
    size_t cap0 = aloc.capacity;
    (void)aloctr_alloc(&aloc, 8);
    /* Alloc until we force a resize (mremap may move buffer; don't use old ptrs after) */
    while (aloc.capacity == cap0) {
        void *p = aloctr_alloc(&aloc, 4096);
        if (!p) break;
        memset(p, 0xDD, 4096);
    }
    ASSERT(aloc.capacity > cap0);
    /* Allocation after resize must work and be readable */
    void *after = aloctr_alloc(&aloc, 8);
    ASSERT(after != NULL);
    *(uint64_t *)after = 0x123456789ABCDEF0ULL;
    ASSERT(*(uint64_t *)after == 0x123456789ABCDEF0ULL);
    aloctr_free(&aloc);
}

/* ---- aloctr_alloc_aligned ---- */
static void test_alloc_aligned_alignments(void)
{
    RUN("alloc_aligned respects 1, 2, 4, 8, 16");
    aloctr aloc = {0};
    ASSERT(aloctr_dynamic_init(&aloc));
    for (size_t align = 1; align <= 16; align *= 2) {
        void *p = aloctr_alloc_aligned(&aloc, 1, align);
        ASSERT(p != NULL);
        ASSERT(((uintptr_t)p % align) == 0);
    }
    aloctr_free(&aloc);
}

static void test_alloc_aligned_padding(void)
{
    RUN("alloc_aligned inserts padding when needed");
    aloctr aloc = {0};
    ASSERT(aloctr_dynamic_init(&aloc));
    void *p1 = aloctr_alloc_aligned(&aloc, 1, 8);
    void *p2 = aloctr_alloc_aligned(&aloc, 1, 8);
    ASSERT(p1 != NULL && p2 != NULL);
    ASSERT((uintptr_t)p1 % 8 == 0);
    ASSERT((uintptr_t)p2 % 8 == 0);
    ASSERT((char *)p2 >= (char *)p1 + 8);
    aloctr_free(&aloc);
}

static void test_alloc_aligned_null_aloc(void)
{
    RUN("alloc_aligned NULL aloc returns NULL");
    void *p = aloctr_alloc_aligned(NULL, 8, 8);
    ASSERT(p == NULL);
}

static void test_alloc_aligned_zero_size(void)
{
    RUN("alloc_aligned size 0 returns NULL");
    aloctr aloc = {0};
    uint8_t buf[64];
    aloctr_static_init(&aloc, sizeof(buf), buf);
    void *p = aloctr_alloc_aligned(&aloc, 0, 8);
    ASSERT(p == NULL);
    aloctr_free(&aloc);
}

/* ---- aloctr_alloc default alignment ---- */
static void test_alloc_default_alignment(void)
{
    RUN("alloc uses natural alignment (max_align_t)");
    aloctr aloc = {0};
    ASSERT(aloctr_dynamic_init(&aloc));
    size_t align = _Alignof(max_align_t);
    void *p = aloctr_alloc(&aloc, 1);
    ASSERT(p != NULL);
    ASSERT(((uintptr_t)p % align) == 0);
    aloctr_free(&aloc);
}

/* ---- aloctr_reset ---- */
static void test_reset_reuse(void)
{
    RUN("reset allows reuse of same buffer");
    aloctr aloc = {0};
    uint8_t buf[128];
    aloctr_static_init(&aloc, sizeof(buf), buf);
    void *a1 = aloctr_alloc(&aloc, 16);
    ASSERT(a1 != NULL);
    aloctr_reset(&aloc);
    ASSERT(aloc.count == 0);
    ASSERT(aloc.buffer == buf);
    void *a2 = aloctr_alloc(&aloc, 16);
    ASSERT(a2 != NULL);
    ASSERT(a2 == a1);
    aloctr_free(&aloc);
}

static void test_reset_dynamic(void)
{
    RUN("reset on dynamic allocator reuses capacity");
    aloctr aloc = {0};
    ASSERT(aloctr_dynamic_init(&aloc));
    size_t cap = aloc.capacity;
    (void)aloctr_alloc(&aloc, 100);
    aloctr_reset(&aloc);
    ASSERT(aloc.count == 0);
    ASSERT(aloc.capacity == cap);
    ASSERT(aloc.is_resizable == true);
    void *p = aloctr_alloc(&aloc, 8);
    ASSERT(p != NULL);
    aloctr_free(&aloc);
}

static void test_reset_null_aloc(void)
{
    RUN("reset NULL aloc is no-op");
    aloctr_reset(NULL);
}

/* ---- aloctr_free ---- */
static void test_free_static_clears_state(void)
{
    RUN("free static clears buffer/count/capacity");
    aloctr aloc = {0};
    uint8_t buf[64];
    aloctr_static_init(&aloc, sizeof(buf), buf);
    aloctr_alloc(&aloc, 8);
    aloctr_free(&aloc);
    ASSERT(aloc.buffer == NULL);
    ASSERT(aloc.count == 0);
    ASSERT(aloc.capacity == 0);
    ASSERT(aloc.is_resizable == false);
}

static void test_free_dynamic_unmaps(void)
{
    RUN("free dynamic leaves allocator in clean state");
    aloctr aloc = {0};
    ASSERT(aloctr_dynamic_init(&aloc));
    void *p = aloctr_alloc(&aloc, 8);
    ASSERT(p != NULL);
    aloctr_free(&aloc);
    ASSERT(aloc.buffer == NULL);
    ASSERT(aloc.count == 0);
    ASSERT(aloc.capacity == 0);
    ASSERT(aloc.is_resizable == false);
}

static void test_free_null_aloc(void)
{
    RUN("free NULL aloc is no-op");
    aloctr_free(NULL);
}

static void test_double_free_safe(void)
{
    RUN("double free is safe (no crash)");
    aloctr aloc = {0};
    uint8_t buf[64];
    aloctr_static_init(&aloc, sizeof(buf), buf);
    aloctr_free(&aloc);
    aloctr_free(&aloc);
}

/* ---- edge cases ---- */
static void test_many_small_allocations(void)
{
    RUN("many small allocations fill arena correctly");
    aloctr aloc = {0};
    uint8_t buf[512];
    aloctr_static_init(&aloc, sizeof(buf), buf);
    void *ptrs[64];
    for (int i = 0; i < 64; i++) {
        ptrs[i] = aloctr_alloc(&aloc, 8);
        if (ptrs[i]) *(uint64_t *)ptrs[i] = (uint64_t)i;
    }
    for (int i = 0; i < 64 && ptrs[i]; i++)
        ASSERT(*(uint64_t *)ptrs[i] == (uint64_t)i);
    aloctr_free(&aloc);
}

static void test_overflow_returns_null(void)
{
    RUN("huge size (overflow) returns NULL");
    aloctr aloc = {0};
    uint8_t buf[64];
    aloctr_static_init(&aloc, sizeof(buf), buf);
    (void)aloctr_alloc(&aloc, 8);
    /* aligned_count is now 16 or similar; size that would overflow: size > SIZE_MAX - aligned_count */
    size_t huge = SIZE_MAX - 8;
    void *p = aloctr_alloc_aligned(&aloc, huge, 1);
    ASSERT(p == NULL);
    aloctr_free(&aloc);
}

static void test_alignment_zero_treated_as_one(void)
{
    RUN("alignment 0 treated as 1");
    aloctr aloc = {0};
    uint8_t buf[32];
    aloctr_static_init(&aloc, sizeof(buf), buf);
    void *p = aloctr_alloc_aligned(&aloc, 4, 0);
    ASSERT(p != NULL);
    aloctr_free(&aloc);
}

static void test_stress_alloc_reset_alloc(void)
{
    RUN("stress: alloc -> reset -> alloc repeated");
    aloctr aloc = {0};
    uint8_t buf[256];
    aloctr_static_init(&aloc, sizeof(buf), buf);
    for (int round = 0; round < 20; round++) {
        void *a = aloctr_alloc(&aloc, 8);
        void *b = aloctr_alloc(&aloc, 16);
        ASSERT(a != NULL && b != NULL);
        *(uint64_t *)a = (uint64_t)round;
        aloctr_reset(&aloc);
    }
    aloctr_free(&aloc);
}

int main(void)
{
    fprintf(stderr, "aloctr test suite\n");
    fprintf(stderr, "----------------\n");

    test_static_init_valid();
    test_static_init_null_aloc();
    test_static_init_zero_len();

    test_dynamic_init_valid();
    test_dynamic_init_null_aloc();

    test_alloc_null_aloc();
    test_alloc_zero_size();
    test_alloc_single_block();
    test_alloc_multiple_blocks();
    test_alloc_static_full_returns_null();
    test_alloc_dynamic_resizes();
    test_alloc_default_alignment();

    test_alloc_aligned_alignments();
    test_alloc_aligned_padding();
    test_alloc_aligned_null_aloc();
    test_alloc_aligned_zero_size();

    test_reset_reuse();
    test_reset_dynamic();
    test_reset_null_aloc();

    test_free_static_clears_state();
    test_free_dynamic_unmaps();
    test_free_null_aloc();
    test_double_free_safe();

    test_many_small_allocations();
    test_overflow_returns_null();
    test_alignment_zero_treated_as_one();
    test_stress_alloc_reset_alloc();

    fprintf(stderr, "----------------\n");
    if (g_failed) {
        fprintf(stderr, "FAILED: %d / %d\n", g_failed, g_run);
        return 1;
    }
    fprintf(stderr, "PASSED: %d\n", g_run);
    return 0;
}
