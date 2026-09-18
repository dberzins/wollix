// test_defaults_once_impl.c - the implementation half of test_defaults_once,
// built with the normal flags so the library's own macro calls (which
// override defaults on purpose) are not subject to the once-only check.
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_editor.h"
