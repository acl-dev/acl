#include "stdafx.h"
#include "fiber/libfiber.h"

#include "msg.h"
#include "pthread_patch.h"
#include "ypipe.h"
#include "iostuff.h"
#include "mbox.h"

#if defined(__linux__) && !defined(MINGW)
# if defined(ALPINE)
#  include <sys/eventfd.h>
#  define HAS_EVENTFD
# else
#  include <linux/version.h>
#  if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
#   include <sys/eventfd.h>
#   define HAS_EVENTFD
#  else
#   undef  HAS_EVENTFD
#  endif
# endif
#else
#  undef  HAS_EVENTFD
#endif

//#  undef  HAS_EVENTFD

struct MBOX {
	socket_t in;
	socket_t out;
	size_t nsend;
	size_t nread;
	YPIPE *ypipe;
	pthread_mutex_t *lock;
};

socket_t mbox_in(MBOX *mbox)
{
	return mbox->in;
}

socket_t mbox_out(MBOX *mbox)
{
	return mbox->out;
}

MBOX *mbox_create(unsigned type)
{
	MBOX *mbox;
	socket_t fds[2];

#if defined(HAS_EVENTFD)
# if defined(ALPINE)
	int flags = 0;
# elif INUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
	int flags = FD_CLOEXEC;
# else
	int flags = 0;
# endif
	fds[0] = eventfd(0, flags);
	fds[1] = fds[0];
#else
	if (sane_socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
		msg_error("%s(%d), %s: duplex_pipe error %s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
		return NULL;
	}
#endif

	mbox        = (MBOX *) malloc(sizeof(MBOX));
	mbox->in    = fds[0];
	mbox->out   = fds[1];
	mbox->nsend = 0;
	mbox->nread = 0;
	mbox->ypipe = ypipe_new();

	if (type == MBOX_T_MPSC) {
		mbox->lock  = (pthread_mutex_t *)
			calloc(1, sizeof(pthread_mutex_t));

		if (pthread_mutex_init(mbox->lock, NULL) != 0) {
			msg_fatal("%s(%d), %s: pthread_mutex_init error",
				__FILE__, __LINE__, __FUNCTION__);
		}
	} else {
		mbox->lock = NULL;
	}

	return mbox;
}

void mbox_free(MBOX *mbox, void (*free_fn)(void*))
{
	CLOSE_SOCKET(mbox->in);
	if (mbox->out != mbox->in) {
		CLOSE_SOCKET(mbox->out);
	}
	ypipe_free(mbox->ypipe, free_fn);
	if (mbox->lock != NULL) {
		pthread_mutex_destroy(mbox->lock);
		free(mbox->lock);
	}
	free(mbox);
}

int mbox_send(MBOX *mbox, void *msg)
{
	int ret;
	long long n = 1;
	pthread_mutex_t *lock = mbox->lock;

	if (lock) {
		pthread_mutex_lock(lock);
	}

	ypipe_write(mbox->ypipe, msg);
	ret = ypipe_flush(mbox->ypipe);

#if defined(HAS_EVENTFD)
	if (lock) {
		pthread_mutex_unlock(lock);
	}
#endif
	if (ret == 0) {
#if !defined(HAS_EVENTFD)
		if (lock) {
			pthread_mutex_unlock(lock);
		}
#endif
		return 0;
	}

	mbox->nsend++;

#if defined(_WIN32) || defined(_WIN64)
	ret = (int) acl_fiber_send(mbox->out, (const char*) &n, (int) sizeof(n), 0);
#else
	ret = (int) acl_fiber_write(mbox->out, &n, sizeof(n));
#endif

#if !defined(HAS_EVENTFD)
	if (lock) {
		pthread_mutex_unlock(lock);
	}
#endif

	if (ret == -1) {
		msg_error("%s(%d), %s: mbox write %d error %s", __FILE__,
			__LINE__, __FUNCTION__, mbox->out, last_serror());
		return -1;
	}

	return 0;
}

void *mbox_read(MBOX *mbox, int timeout, int *success)
{
	int  ret;
	long long n;
	void *msg = ypipe_read(mbox->ypipe);

	if (msg != NULL) {
		if (success) {
			*success = 1;
		}
		return msg;
	} else if (timeout == 0) {
		if (success) {
			*success = 1;
		}
		return NULL;
	}

	mbox->nread++;

	if (timeout > 0 && read_wait(mbox->in, timeout) < 0) {
		if (acl_fiber_last_error() == FIBER_ETIME) {
			if (success) {
				*success = 1;
			}
		} else if (success) {
			*success = 0;
		}
		return NULL;
	}

#if defined(_WIN32) || defined(_WIN64)
	ret = (int) acl_fiber_recv(mbox->in, (char*) &n, (int) sizeof(n), 0);
#else
	ret = (int) acl_fiber_read(mbox->in, &n, sizeof(n));
#endif

	if (ret == -1) {
		if (success) {
			*success = 0;
		}
		return NULL;
	}

	if (success) {
		*success = 1;
	}

	return ypipe_read(mbox->ypipe);
}

size_t mbox_nsend(MBOX *mbox)
{
	return mbox->nsend;
}

size_t mbox_nread(MBOX *mbox)
{
	return mbox->nread;
}
