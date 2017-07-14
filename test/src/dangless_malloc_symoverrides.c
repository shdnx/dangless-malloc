#if 0

#include <malloc.h>

#include "dangless_malloc.h"

// strong overrides of the libc memory allocation symbols
// when this all gets moved to a library, --whole-archive will have to be used when linking against the library so that these symbols will get picked up instead of the libc one
__strong_alias(malloc, dangless_malloc);
__strong_alias(calloc, dangless_calloc);
__strong_alias(realloc, dangless_realloc);
__strong_alias(posix_memalign, dangless_posix_memalign);
__strong_alias(free, dangless_free);

#endif