#ifndef DANGLESS_PTHREAD_MOCK_H
#define DANGLESS_PTHREAD_MOCK_H

#define pthread_mutex_lock(...)
#define pthread_mutex_unlock(...)
#define pthread_once(x, f) f()

#define PTHREAD_MUTEX_INITIALIZER {}
#define PTHREAD_ONCE_INIT { 0 }

#endif
