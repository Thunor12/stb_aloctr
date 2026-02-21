#ifndef STB_ARENA_H
#define STB_ARENA_H

// A simple allocator lib
// Static alloc arenas
// Dynamic alloc arenas

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/// @brief The main allocator struct
typedef struct aloctr
{
    void* buffer;
    bool is_resizable;
    size_t count;
    size_t capacity;
} aloctr;

/// @brief Initializes the allocator from a static buffer
bool aloctr_static_init(aloctr* aloc, size_t buf_len, uint8_t buf[buf_len]);

/// @brief Initializes the allocator to perform dynamic allocations
bool aloctr_dynamic_init(aloctr* aloc);

/// @brief Allocates size bytes with default alignment (suitable for any standard type)
void *aloctr_alloc(aloctr* aloc, size_t size);

/// @brief Allocates size bytes with the given alignment (must be a power of two)
void *aloctr_alloc_aligned(aloctr* aloc, size_t size, size_t alignment);

/// @brief Resets the allocator for reuse: count = 0, buffer and capacity unchanged
void aloctr_reset(aloctr* aloc);

/// @brief Frees all memory in the allocator
void aloctr_free(aloctr* aloc);

#ifdef ALOCTR_IMPLEMENTATION

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Define ALOCTR_LOG(fmt, ...) before including with ALOCTR_IMPLEMENTATION to enable logging, e.g.:
 * #define ALOCTR_LOG(fmt, ...) fprintf(stderr, "[aloctr] " fmt, ##__VA_ARGS__) */
#ifndef ALOCTR_LOG
#define ALOCTR_LOG(...) ((void)0)
#endif

/* ---------------------------------------------------------------------------
 * Platform-specific OS abstraction: page size, alloc, free, resize (may move).
 * Resize returns new pointer or NULL; caller must update aloc->buffer.
 * --------------------------------------------------------------------------- */
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)

#include <windows.h>

static size_t aloctr_os_pagesize(void)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (size_t)si.dwPageSize;
}

static void *aloctr_os_alloc(size_t size)
{
    return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

static void aloctr_os_free(void *ptr, size_t size)
{
    (void)size;
    VirtualFree(ptr, 0, MEM_RELEASE);
}

static void *aloctr_os_resize(void *ptr, size_t old_size, size_t new_size)
{
    void *n = VirtualAlloc(NULL, new_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!n) return NULL;
    memcpy(n, ptr, old_size);
    VirtualFree(ptr, 0, MEM_RELEASE);
    return n;
}

#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)

#include <sys/mman.h>
#include <unistd.h>

static size_t aloctr_os_pagesize(void)
{
    return (size_t)sysconf(_SC_PAGESIZE);
}

static void *aloctr_os_alloc(size_t size)
{
    void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (p == MAP_FAILED) ? NULL : p;
}

static void aloctr_os_free(void *ptr, size_t size)
{
    munmap(ptr, size);
}

static void *aloctr_os_resize(void *ptr, size_t old_size, size_t new_size)
{
    void *n = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (n == MAP_FAILED) return NULL;
    memcpy(n, ptr, old_size);
    munmap(ptr, old_size);
    return n;
}

#else
/* Fallback: malloc/realloc. No virtual memory; works on any hosted C implementation. */
#define ALOCTR_OS_USE_MALLOC

static size_t aloctr_os_pagesize(void)
{
    return 4096;
}

static void *aloctr_os_alloc(size_t size)
{
    return malloc(size);
}

static void aloctr_os_free(void *ptr, size_t size)
{
    (void)size;
    free(ptr);
}

static void *aloctr_os_resize(void *ptr, size_t old_size, size_t new_size)
{
    (void)old_size;
    return realloc(ptr, new_size);
}

#endif

/* ---------------------------------------------------------------------------
 * Allocator implementation (platform-agnostic)
 * --------------------------------------------------------------------------- */

bool aloctr_static_init(aloctr* aloc, size_t buf_len, uint8_t buf[buf_len])
{
    if(!aloc) { return false; }

    aloc->buffer = buf;
    aloc->count = 0UL;
    aloc->capacity = buf_len;
    aloc->is_resizable = false;

    return true;
}

bool aloctr_dynamic_init(aloctr* aloc)
{
    if(!aloc) { return false; }

    size_t ps = aloctr_os_pagesize();
    ALOCTR_LOG("Page size = %zu\n", ps);

    aloc->buffer = aloctr_os_alloc(ps);
    if(!aloc->buffer) {
#ifndef ALOCTR_OS_USE_MALLOC
        perror("aloctr: failed to alloc");
#endif
        return false;
    }

    aloc->count = 0UL;
    aloc->capacity = ps;
    aloc->is_resizable = true;

    return true;
}

static size_t aloctr_align_up(size_t value, size_t alignment)
{
    if(alignment == 0) { alignment = 1; }
    return (value + alignment - 1) & ~(alignment - 1);
}

void *aloctr_alloc_aligned(aloctr* aloc, size_t size, size_t alignment)
{
    if(!aloc) { return NULL; }
    if(0UL == size) { return NULL; }
    if(alignment == 0) { alignment = 1; }

    size_t aligned_count = aloctr_align_up(aloc->count, alignment);
    if(size > SIZE_MAX - aligned_count) { return NULL; }
    size_t new_count = aligned_count + size;

    if(new_count > aloc->capacity)
    {
        if(!aloc->is_resizable) { return NULL; }
        if(aloc->capacity > SIZE_MAX / 2) { return NULL; }

        size_t new_cap = aloc->capacity * 2;
        ALOCTR_LOG("Remapping from %zu to %zu\n", aloc->capacity, new_cap);
        void *new_buf = aloctr_os_resize(aloc->buffer, aloc->capacity, new_cap);
        if(!new_buf) return NULL;
        aloc->buffer = new_buf;
        aloc->capacity = new_cap;
    }

    void *p = (char *)aloc->buffer + aligned_count;
    aloc->count = new_count;
    return p;
}

void *aloctr_alloc(aloctr* aloc, size_t size)
{
    return aloctr_alloc_aligned(aloc, size, _Alignof(max_align_t));
}

void aloctr_reset(aloctr* aloc)
{
    if(!aloc) { return; }
    aloc->count = 0UL;
}

void aloctr_free(aloctr* aloc)
{
    if(!aloc) { return; }
    if(aloc->is_resizable && aloc->buffer)
    {
        ALOCTR_LOG("Freeing dynamic memory (%p, size %zu)\n", aloc->buffer, aloc->capacity);
        aloctr_os_free(aloc->buffer, aloc->capacity);
    }

    aloc->buffer = NULL;
    aloc->count = 0UL;
    aloc->capacity = 0UL;
    aloc->is_resizable = false;
}

#endif
#endif
