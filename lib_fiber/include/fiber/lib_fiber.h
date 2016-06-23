#ifndef LIB_FIBER_INCLUDE_H
#define LIB_FIBER_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FIBER FIBER;

FIBER *fiber_create(void (*fn)(FIBER *, void *), void *arg, size_t size);
int fiber_id(const FIBER *fiber);
int fiber_self(void);
void fiber_set_errno(FIBER *fiber, int errnum);
int fiber_errno(FIBER *fiber);
int fiber_status(const FIBER *fiber);
int fiber_yield(void);
void fiber_ready(FIBER *fiber);
void fiber_switch(void);
void fiber_schedule(void);

void fiber_io_stop(void);
unsigned int fiber_delay(unsigned int milliseconds);
unsigned int fiber_sleep(unsigned int seconds);
FIBER *fiber_create_timer(unsigned int milliseconds,
	void (*fn)(FIBER *, void *), void *ctx);
void fiber_reset_timer(FIBER *timer, unsigned int milliseconds);

void fiber_set_dns(const char* ip, int port);

/* fiber locking */

typedef struct FIBER_LOCK FIBER_LOCK;
typedef struct FIBER_RWLOCK FIBER_RWLOCK;

FIBER_LOCK *fiber_lock_create(void);
void fiber_lock_free(FIBER_LOCK *l);
void fiber_lock(FIBER_LOCK *l);
int fiber_trylock(FIBER_LOCK *l);
void fiber_unlock(FIBER_LOCK *l);

FIBER_RWLOCK *fiber_rwlock_create(void);
void fiber_rwlock_free(FIBER_RWLOCK *l);
void fiber_rwlock_rlock(FIBER_RWLOCK *l);
int fiber_rwlock_tryrlock(FIBER_RWLOCK *l);
void fiber_rwlock_wlock(FIBER_RWLOCK *l);
int fiber_rwlock_trywlock(FIBER_RWLOCK *l);
void fiber_rwlock_runlock(FIBER_RWLOCK *l);
void fiber_rwlock_wunlock(FIBER_RWLOCK *l);

/* channel communication */

typedef struct CHANNEL CHANNEL;

CHANNEL* channel_create(int elemsize, int bufsize);
void channel_free(CHANNEL *c);
int channel_send(CHANNEL *c, void *v);
int channel_send_nb(CHANNEL *c, void *v);
int channel_recv(CHANNEL *c, void *v);
int channel_recv_nb(CHANNEL *c, void *v);
int channel_sendp(CHANNEL *c, void *v);
void *channel_recvp(CHANNEL *c);
int channel_sendp_nb(CHANNEL *c, void *v);
void *channel_recvp_nb(CHANNEL *c);
int channel_sendul(CHANNEL *c, unsigned long val);
unsigned long channel_recvul(CHANNEL *c);
int channel_sendul_nb(CHANNEL *c, unsigned long val);
unsigned long channel_recvul_nb(CHANNEL *c);

/* master fibers server */

void fiber_server_main(int argc, char *argv[],
	void (*service)(ACL_VSTREAM*, void*), void *ctx, int name, ...);

#ifdef __cplusplus
}
#endif

#endif
