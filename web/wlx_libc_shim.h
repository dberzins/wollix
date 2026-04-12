// wlx_libc_shim.h — minimal libc for wollix bare-wasm32 builds
// Provides the ~21 libc symbols wollix.h requires, without WASI or Emscripten.
// Only used when compiling with --target=wasm32-unknown-unknown -nostdlib.

#ifndef WLX_LIBC_SHIM_H_
#define WLX_LIBC_SHIM_H_

#include <stddef.h>
#include <stdint.h>

// ============================================================================
// stdlib.h — memory allocation + abort
// ============================================================================

void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void *realloc(void *ptr, size_t size);
void  free(void *ptr);

__attribute__((import_module("env"), import_name("abort")))
extern void __wlx_import_abort(void);

_Noreturn static inline void abort(void) {
    __wlx_import_abort();
    __builtin_unreachable();
}

// ============================================================================
// string.h — memory and string operations
// ============================================================================

void  *memset(void *s, int c, size_t n);
void  *memcpy(void *dest, const void *src, size_t n);
void  *memmove(void *dest, const void *src, size_t n);
size_t strlen(const char *s);
char  *strncpy(char *dest, const char *src, size_t n);

// ============================================================================
// math.h — float functions (wasm f32 intrinsics)
// ============================================================================

float floorf(float x);
float ceilf(float x);
float roundf(float x);
float fabsf(float x);
float sqrtf(float x);
float fmodf(float x, float y);

// ============================================================================
// stdio.h — minimal snprintf + stubs
// ============================================================================

typedef struct { int _fd; } FILE;
extern FILE *stderr;

int snprintf(char *buf, size_t size, const char *fmt, ...);
int vsnprintf(char *buf, size_t size, const char *fmt, __builtin_va_list ap);

// fprintf/printf: no-ops in bare-wasm builds (no fd_write).
// WLX_TODO/WLX_UNREACHABLE call fprintf then abort — abort still works.
static inline int fprintf(FILE *stream, const char *fmt, ...) {
    (void)stream; (void)fmt;
    return 0;
}
static inline int printf(const char *fmt, ...) {
    (void)fmt;
    return 0;
}

#endif // WLX_LIBC_SHIM_H_
