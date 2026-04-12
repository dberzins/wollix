// Redirect header: resolves #include <assert.h> to the wollix bare-wasm shim.
#ifndef _WLX_REDIRECT_ASSERT_H_
#define _WLX_REDIRECT_ASSERT_H_

#include "../wlx_libc_shim.h"

#ifdef NDEBUG
  #define assert(expr) ((void)0)
#else
  #define assert(expr) \
      ((expr) ? (void)0 : (fprintf(stderr, "assert failed: %s\n", #expr), abort()))
#endif

#endif
