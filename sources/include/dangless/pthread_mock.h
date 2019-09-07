#ifndef DANGLESS_PTHREAD_MOCK_H
#define DANGLESS_PTHREAD_MOCK_H

// Unfortunately we can't just define these as mock types, as GLIBC's stdlib.h (used e.g. by dangless_malloc.c) includes <bits/pthreadtypes.h> with conflicting type definitions.
// So we just include that header everywhere, and make up our own initializers and hope for the best...

//typedef int pthread_mutex_t;
//typedef int pthread_once_t;

#include <bits/pthreadtypes.h>

#define pthread_mutex_lock(...) /* empty */
#define pthread_mutex_unlock(...) /* empty */
#define pthread_once(x, f) f()

#define PTHREAD_MUTEX_INITIALIZER { 0 }
#define PTHREAD_ONCE_INIT { 0 }

#endif
