#ifndef TYPEOS_PTHREAD_H
#define TYPEOS_PTHREAD_H

typedef int pthread_mutex_t;
typedef int pthread_cond_t;

#define PTHREAD_MUTEX_INITIALIZER 0
#define PTHREAD_COND_INITIALIZER 0

static inline int pthread_mutex_lock(pthread_mutex_t* m) {
	(void)m;
	return 0;
}
static inline int pthread_mutex_unlock(pthread_mutex_t* m) {
	(void)m;
	return 0;
}
static inline int pthread_cond_init(pthread_cond_t* c, const void* a) {
	(void)c;
	(void)a;
	return 0;
}
static inline int pthread_cond_destroy(pthread_cond_t* c) {
	(void)c;
	return 0;
}
static inline int pthread_cond_wait(pthread_cond_t* c, pthread_mutex_t* m) {
	(void)c;
	(void)m;
	return 0;
}
static inline int pthread_cond_signal(pthread_cond_t* c) {
	(void)c;
	return 0;
}
static inline int pthread_cond_timedwait(pthread_cond_t* c, pthread_mutex_t* m, const void* abst) {
	(void)c;
	(void)m;
	(void)abst;
	return 0;
}

#endif