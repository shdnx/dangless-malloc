#ifndef DANGLESS_PTHREAD_MOCK_H
#define DANGLESS_PTHREAD_MOCK_H

#define pthread_mutex_t int
#define pthread_once_t int

#define pthread_mutex_lock(...)
#define pthread_mutex_unlock(...)
#define pthread_once(x, f) f()

#define PTHREAD_MUTEX_INITIALIZER 0
#define PTHREAD_ONCE_INIT 0

#endif
