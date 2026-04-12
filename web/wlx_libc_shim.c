// wlx_libc_shim.c — implementations for the bare-wasm32 libc shim
// Compiled alongside wollix when targeting wasm32-unknown-unknown -nostdlib.

#include "wlx_libc_shim.h"

// ============================================================================
// Bump allocator
// ============================================================================
//
// Simplest possible allocator: a bump pointer growing upward from __heap_base.
// Each allocation is prefixed with an 8-byte header storing the block size so
// that realloc can copy the right number of bytes.
//
// free() is a no-op. wollix's allocation pattern is grow-at-init and stable
// across frames, so fragmentation is not a practical concern.

extern unsigned char __heap_base;

static unsigned char *heap_ptr  = 0;
static unsigned char *heap_end  = 0;

#define ALLOC_ALIGN 8
#define PAGE_SIZE 65536 // 64KB

typedef struct {
    size_t size;     // usable payload size (not including header)
} Alloc_Header;

static void heap_init(void) {
    if (heap_ptr == 0) {
        heap_ptr = &__heap_base;
        // Current memory size in pages (64KB each)
        size_t pages = __builtin_wasm_memory_size(0);
        heap_end = (unsigned char *)(pages * PAGE_SIZE);
    }
}

static void heap_ensure(size_t needed) {
    if (heap_ptr + needed > heap_end) {
        size_t deficit = (size_t)(heap_ptr + needed - heap_end);
        size_t pages = (deficit + PAGE_SIZE - 1) / PAGE_SIZE;
        if (__builtin_wasm_memory_grow(0, pages) == (size_t)-1) {
            __wlx_import_abort();
            __builtin_unreachable();
        }
        heap_end += pages * PAGE_SIZE;
    }
}

static unsigned char *align_up(unsigned char *p, size_t align) {
    size_t addr = (size_t)p;
    size_t mask = align - 1;
    return (unsigned char *)((addr + mask) & ~mask);
}

void *malloc(size_t size) {
    heap_init();
    if (size == 0) size = 1;

    unsigned char *base = align_up(heap_ptr, ALLOC_ALIGN);
    size_t total = sizeof(Alloc_Header) + size;
    heap_ensure((size_t)(base - heap_ptr) + total);

    Alloc_Header *hdr = (Alloc_Header *)base;
    hdr->size = size;
    void *payload = base + sizeof(Alloc_Header);
    heap_ptr = align_up(base + total, ALLOC_ALIGN);
    return payload;
}

void *calloc(size_t count, size_t size) {
    size_t total = count * size;
    void *p = malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void *realloc(void *ptr, size_t new_size) {
    if (ptr == 0) return malloc(new_size);
    if (new_size == 0) { free(ptr); return 0; }

    Alloc_Header *hdr = (Alloc_Header *)((unsigned char *)ptr - sizeof(Alloc_Header));
    size_t old_size = hdr->size;

    void *new_ptr = malloc(new_size);
    if (new_ptr) {
        size_t copy_size = old_size < new_size ? old_size : new_size;
        memcpy(new_ptr, ptr, copy_size);
    }
    return new_ptr;
}

void free(void *ptr) {
    (void)ptr; // bump allocator — no-op
}

// ============================================================================
// string.h implementations
// ============================================================================

void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    unsigned char val = (unsigned char)c;
    for (size_t i = 0; i < n; i++) p[i] = val;
    return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    if (d < s) {
        for (size_t i = 0; i < n; i++) d[i] = s[i];
    } else if (d > s) {
        for (size_t i = n; i > 0; i--) d[i - 1] = s[i - 1];
    }
    return dest;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len] != '\0') len++;
    return len;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = '\0';
    return dest;
}

// ============================================================================
// math.h implementations
// ============================================================================

float floorf(float x) {
    return __builtin_floorf(x);
}

float ceilf(float x) {
    return __builtin_ceilf(x);
}

float roundf(float x) {
    float floor_value = __builtin_floorf(x);
    float diff = x - floor_value;

    if (diff < 0.5f) return floor_value;
    if (diff > 0.5f) return floor_value + 1.0f;
    return x < 0.0f ? floor_value : floor_value + 1.0f;
}

float fabsf(float x) {
    return __builtin_fabsf(x);
}

float sqrtf(float x) {
    return __builtin_sqrtf(x);
}

float fmodf(float x, float y) {
    if (y == 0.0f) return 0.0f;
    return x - (float)((int)(x / y)) * y;
}

// ============================================================================
// stdio.h — minimal snprintf / vsnprintf
// ============================================================================
//
// Supports: %s, %d, %i, %u, %f, %.Nf, %%, %c, %p, %x, %X, %ld, %li, %lu, %zu
// Does NOT support: width specifiers, left-align, padding, etc.
// Adequate for wollix slider labels and gallery status bar.

FILE _stderr_obj = { 2 };
FILE *stderr = &_stderr_obj;

static int fmt_int(char *buf, size_t remaining, int value) {
    char tmp[16];
    int len = 0;
    unsigned int uval;

    if (value < 0) {
        if (remaining > 0) { *buf = '-'; }
        uval = (unsigned int)(-value);
        int digits = fmt_int(buf + 1, remaining > 1 ? remaining - 1 : 0, (int)uval);
        return 1 + digits;
    }

    // special case for 0
    if (value == 0) {
        if (remaining > 0) *buf = '0';
        return 1;
    }

    uval = (unsigned int)value;
    while (uval > 0) {
        tmp[len++] = '0' + (char)(uval % 10);
        uval /= 10;
    }
    for (int i = 0; i < len; i++) {
        if ((size_t)i < remaining) buf[i] = tmp[len - 1 - i];
    }
    return len;
}

static int fmt_uint(char *buf, size_t remaining, unsigned int value) {
    char tmp[16];
    int len = 0;

    if (value == 0) {
        if (remaining > 0) *buf = '0';
        return 1;
    }

    while (value > 0) {
        tmp[len++] = '0' + (char)(value % 10);
        value /= 10;
    }
    for (int i = 0; i < len; i++) {
        if ((size_t)i < remaining) buf[i] = tmp[len - 1 - i];
    }
    return len;
}

static int fmt_hex(char *buf, size_t remaining, unsigned int value, int upper) {
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char tmp[16];
    int len = 0;

    if (value == 0) {
        if (remaining > 0) *buf = '0';
        return 1;
    }

    while (value > 0) {
        tmp[len++] = digits[value & 0xF];
        value >>= 4;
    }
    for (int i = 0; i < len; i++) {
        if ((size_t)i < remaining) buf[i] = tmp[len - 1 - i];
    }
    return len;
}

static int fmt_size_t(char *buf, size_t remaining, size_t value) {
    char tmp[24];
    int len = 0;

    if (value == 0) {
        if (remaining > 0) *buf = '0';
        return 1;
    }

    while (value > 0) {
        tmp[len++] = '0' + (char)(value % 10);
        value /= 10;
    }
    for (int i = 0; i < len; i++) {
        if ((size_t)i < remaining) buf[i] = tmp[len - 1 - i];
    }
    return len;
}

static int fmt_float(char *buf, size_t remaining, double value, int precision) {
    int written = 0;

    if (value < 0) {
        if (remaining > 0) { buf[0] = '-'; }
        written++;
        value = -value;
        buf++;
        if (remaining > 0) remaining--;
    }

    unsigned int int_part = (unsigned int)value;
    int n = fmt_uint(buf, remaining, int_part);
    written += n;
    buf += n;
    if ((size_t)n < remaining) remaining -= (size_t)n; else remaining = 0;

    if (precision > 0) {
        if (remaining > 0) { buf[0] = '.'; }
        written++;
        buf++;
        if (remaining > 0) remaining--;

        double frac = value - (double)int_part;
        for (int i = 0; i < precision; i++) {
            frac *= 10.0;
            int digit = (int)frac;
            if (digit > 9) digit = 9;
            if (remaining > 0) { buf[0] = '0' + (char)digit; remaining--; }
            written++;
            buf++;
            frac -= (double)digit;
        }
    }

    return written;
}

int vsnprintf(char *buf, size_t size, const char *fmt, __builtin_va_list ap) {
    size_t pos = 0;              // position in output buffer
    int total = 0;               // total chars that would be written (for return value)
    size_t limit = size > 0 ? size - 1 : 0; // leave room for NUL

    #define PUTC(ch) do { if (pos < limit) buf[pos] = (ch); pos++; total++; } while(0)

    while (*fmt) {
        if (*fmt != '%') {
            PUTC(*fmt);
            fmt++;
            continue;
        }
        fmt++; // skip '%'

        // Parse precision for %f: %.Nf
        int precision = 2; // default
        int has_precision = 0;
        int is_long = 0;
        int is_size = 0;

        if (*fmt == '.') {
            fmt++;
            has_precision = 1;
            precision = 0;
            while (*fmt >= '0' && *fmt <= '9') {
                precision = precision * 10 + (*fmt - '0');
                fmt++;
            }
        }

        // Length modifiers
        if (*fmt == 'l') { is_long = 1; fmt++; }
        else if (*fmt == 'z') { is_size = 1; fmt++; }

        switch (*fmt) {
            case 's': {
                const char *s = __builtin_va_arg(ap, const char *);
                if (!s) s = "(null)";
                while (*s) { PUTC(*s); s++; }
                break;
            }
            case 'd': case 'i': {
                long val;
                if (is_long) val = __builtin_va_arg(ap, long);
                else val = (long)__builtin_va_arg(ap, int);
                char tmp[24];
                int n = fmt_int(tmp, sizeof(tmp), (int)val);
                for (int i = 0; i < n; i++) PUTC(tmp[i]);
                break;
            }
            case 'u': {
                if (is_size) {
                    size_t val = __builtin_va_arg(ap, size_t);
                    char tmp[24];
                    int n = fmt_size_t(tmp, sizeof(tmp), val);
                    for (int i = 0; i < n; i++) PUTC(tmp[i]);
                } else {
                    unsigned int val;
                    if (is_long) val = (unsigned int)__builtin_va_arg(ap, unsigned long);
                    else val = __builtin_va_arg(ap, unsigned int);
                    char tmp[16];
                    int n = fmt_uint(tmp, sizeof(tmp), val);
                    for (int i = 0; i < n; i++) PUTC(tmp[i]);
                }
                break;
            }
            case 'x': case 'X': {
                unsigned int val = __builtin_va_arg(ap, unsigned int);
                char tmp[16];
                int n = fmt_hex(tmp, sizeof(tmp), val, *fmt == 'X');
                for (int i = 0; i < n; i++) PUTC(tmp[i]);
                break;
            }
            case 'f': {
                double val = __builtin_va_arg(ap, double);
                if (!has_precision) precision = 2;
                char tmp[32];
                int n = fmt_float(tmp, sizeof(tmp), val, precision);
                for (int i = 0; i < n; i++) PUTC(tmp[i]);
                break;
            }
            case 'c': {
                char c = (char)__builtin_va_arg(ap, int);
                PUTC(c);
                break;
            }
            case 'p': {
                void *p = __builtin_va_arg(ap, void *);
                PUTC('0'); PUTC('x');
                char tmp[16];
                int n = fmt_hex(tmp, sizeof(tmp), (unsigned int)(uintptr_t)p, 0);
                for (int i = 0; i < n; i++) PUTC(tmp[i]);
                break;
            }
            case '%': {
                PUTC('%');
                break;
            }
            default:
                PUTC('%');
                PUTC(*fmt);
                break;
        }
        fmt++;
    }

    #undef PUTC

    if (size > 0) buf[pos < limit ? pos : limit] = '\0';
    return total;
}

int snprintf(char *buf, size_t size, const char *fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    int result = vsnprintf(buf, size, fmt, ap);
    __builtin_va_end(ap);
    return result;
}
