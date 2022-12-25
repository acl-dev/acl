#include "stdafx.h"
#include "common.h"

#include "fiber.h"
#include "hook.h"
#include "io.h"

#if defined(HAS_IO_URING)
static int uring_wait_read(FILE_EVENT *fe)
{
	while (1) {
		int err;

		// Must clear the EVENT_READ flags in order to set IO event
		// for each IO process.
		fe->mask &= ~EVENT_READ;
		fe->reader_ctx.res = 0;

		if (fiber_wait_read(fe) < 0) {
			return -1;
		}

		if (fe->mask & (EVENT_ERR | EVENT_HUP | EVENT_NVAL)) {
			err = acl_fiber_last_error();
			fiber_save_errno(err);
			return -1;
		}

		if (acl_fiber_canceled(fe->fiber_r)) {
			acl_fiber_set_error(fe->fiber_r->errnum);
			return -1;
		}

		if (fe->reader_ctx.res >= 0) {
			return fe->reader_ctx.res;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			if (!(fe->type & TYPE_EVENTABLE)) {
				fiber_file_free(fe);
			}
			return -1;
		}
	}
}

static int iocp_wait_read(FILE_EVENT *fe)
{
	int ret;

	// Add one reference to prevent being released prematurely.
	file_event_refer(fe);
	ret = uring_wait_read(fe);
	file_event_unrefer(fe);
	return ret;
}

int fiber_iocp_read(FILE_EVENT *fe, char *buf, int len)
{
	fe->in.read_ctx.buf = buf;
	fe->in.read_ctx.len = len;

	return iocp_wait_read(fe);
}
#endif  // HAS_IO_URING

#if defined(HAS_IOCP)
static int iocp_wait_read(FILE_EVENT *fe)
{
	while (1) {
		int err;

		fe->mask &= ~EVENT_READ;

		if (fiber_wait_read(fe) < 0) {
			msg_error("%s(%d): fiber_wait_read error=%s, fd=%d",
				__FUNCTION__, __LINE__, last_serror(),
				(int) fe->fd);
			return -1;
		}

		if (fe->mask & EVENT_ERR) {
			err = acl_fiber_last_error();
			fiber_save_errno(err);
			return -1;
		}

		if (acl_fiber_canceled(fe->fiber_r)) {
			acl_fiber_set_error(fe->fiber_r->errnum);
			return -1;
		}

		if (fe->res >= 0) {
			return fe->res;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			if (fe->type != TYPE_SPIPE) {
				fiber_file_free(fe);
			}
			return -1;
		}
	}
}

int fiber_iocp_read(FILE_EVENT *fe, char *buf, int len)
{
	/* If the socket type is UDP, We must check the fixed buffer first,
	 * which maybe used in iocp_add_read() and set for polling read status.
	 */
	if (fe->sock_type == SOCK_DGRAM
		&& fe->rbuf == fe->packet && fe->res > 0) {

		if (fe->res < len) {
			len = fe->res;
		}
		memcpy(buf, fe->packet, len);
		fe->rbuf = NULL;
		fe->res = 0;
		return len;
	}

	
	fe->rbuf  = buf;
	fe->rsize = len;
	fe->res   = 0;
	return iocp_wait_read(fe);
}
#endif // HAS_IOCP

// After calling fiber_wait_read():
// The fiber_wait_read will return three status:
// 1: The fd is a valid socket/pipe/fifo, which can be
//    monitored by event engine, such as epoll, select or poll;
// 0: The fd isn't a socket/pipe/fifo, maybe a file, and can't
//    be monitored by event engine and can read directly;
// -1: The fd isn't a valid descriptor, just return error, and
//   the fe should be freed.

// After calling acl_fiber_canceled():
// If the suspending fiber wakeup for the reason that it was
// killed by the other fiber which called acl_fiber_kill and
// want to close the fd owned by the current fiber, we just
// set the errno status and return -1, and the killer fiber
// will close the fd in acl_fiber_close API.

// After calling error_again();
// Check if the fd can monitored by event, if the fd
// isn't monitored by the event engine, the above
// fiber_wait_read() must return 0, so we must free
// the fe here. Because epoll can only monitor socket
// fd, not including file fd, the event_add_read will
// not monitor the file fd in fiber_wait_read.

#if defined(_WIN32) || defined(_WIN64)
#define FIBER_READ(_fn, _fe, ...) do {                                       \
    ssize_t ret;                                                             \
    int err;                                                                 \
    if (IS_READABLE((_fe))) {                                                \
        CLR_READABLE((_fe));                                                 \
    } else if (fiber_wait_read((_fe)) < 0) {                                 \
        return -1;                                                           \
    }                                                                        \
    if (acl_fiber_canceled((_fe)->fiber_r)) {                                \
        acl_fiber_set_error((_fe)->fiber_r->errnum);                         \
        return -1;                                                           \
    }                                                                        \
    if ((_fn) == NULL) {                                                     \
        hook_once();                                                         \
    }                                                                        \
    ret = (*_fn)((_fe)->fd, __VA_ARGS__);                                    \
    if (ret >= 0) {                                                          \
        return ret;                                                          \
    }                                                                        \
    err = acl_fiber_last_error();                                            \
    fiber_save_errno(err);                                                   \
    if (!error_again(err)) {                                                 \
        if (!((_fe)->type & TYPE_EVENTABLE)) {                               \
            fiber_file_free((_fe));                                          \
        }                                                                    \
        return -1;                                                           \
    }                                                                        \
} while (1)
#else
#define FIBER_READ(_fn, _fe, _args...) do {                                  \
    ssize_t ret;                                                             \
    int err;                                                                 \
    if (IS_READABLE((_fe))) {                                                \
        CLR_READABLE((_fe));                                                 \
    } else if (fiber_wait_read((_fe)) < 0) {                                 \
        return -1;                                                           \
    }                                                                        \
    if (acl_fiber_canceled((_fe)->fiber_r)) {                                \
        acl_fiber_set_error((_fe)->fiber_r->errnum);                         \
        return -1;                                                           \
    }                                                                        \
    if ((_fn) == NULL) {                                                     \
        hook_once();                                                         \
    }                                                                        \
    ret = (*_fn)((_fe)->fd, ##_args);                                        \
    if (ret >= 0) {                                                          \
        return ret;                                                          \
    }                                                                        \
    err = acl_fiber_last_error();                                            \
    fiber_save_errno(err);                                                   \
    if (!error_again(err)) {                                                 \
        if (!((_fe)->type & TYPE_EVENTABLE)) {                               \
            fiber_file_free((_fe));                                          \
        }                                                                    \
        return -1;                                                           \
    }                                                                        \
} while (1)
#endif

#define FILE_ALLOC(f, t, fd) do {                                            \
    (f) = file_event_alloc(fd);                                              \
    (f)->fiber_r->status = FIBER_STATUS_NONE;                                \
    (f)->fiber_w->status = FIBER_STATUS_NONE;                                \
    (f)->mask   = (t);                                                       \
    (f)->type   = TYPE_EVENTABLE;                                            \
} while (0)

#ifdef SYS_UNIX

ssize_t fiber_read(FILE_EVENT *fe,  void *buf, size_t count)
{
	CLR_POLLING(fe);

#ifdef HAS_IO_URING
	// One FILE_EVENT can be used by multiple fibers with the same
	// EVENT_BUSY_READ or EVENT_BUSY_WRITE in the same time. But can be
	// used by two fibers that one is a reader and the other is a writer,
	// because there're two different objects for reader and writer.
	if (EVENT_IS_IO_URING(fiber_io_event())) {

#define SET_READ(f) do {                                                     \
    (f)->in.read_ctx.buf = buf;                                              \
    (f)->in.read_ctx.len = (int) count;                                      \
    (f)->mask |= EVENT_READ;                                                 \
} while (0)

		int ret;

		if (!(fe->busy & EVENT_BUSY_READ)) {
			SET_READ(fe);

			fe->busy |= EVENT_BUSY_READ;
			ret = iocp_wait_read(fe);
			fe->busy &= ~EVENT_BUSY_READ;
		} else {
			FILE_ALLOC(fe, 0, fe->fd);
			SET_READ(fe);

			ret = iocp_wait_read(fe);
			file_event_unrefer(fe);
		}
		return ret;
	}
#endif

	FIBER_READ(sys_read, fe, buf, count);
}

ssize_t fiber_readv(FILE_EVENT *fe, const struct iovec *iov, int iovcnt)
{
	CLR_POLLING(fe);

#ifdef HAS_IO_URING
	if (EVENT_IS_IO_URING(fiber_io_event())) {

#define SET_READV(f) do {                                                    \
    (f)->in.readv_ctx.iov = iov;                                             \
    (f)->in.readv_ctx.cnt = iovcnt;                                          \
    (f)->in.readv_ctx.off = 0;                                               \
    (f)->mask |= EVENT_READV;                                                \
} while (0)

		int ret;

		if (!(fe->busy & EVENT_BUSY_READ)) {
			SET_READV(fe);

			fe->busy |= EVENT_BUSY_READ;
			ret = iocp_wait_read(fe);
			fe->busy &= ~EVENT_BUSY_READ;
		} else {
			FILE_ALLOC(fe, 0, fe->fd);
			SET_READV(fe);

			ret = iocp_wait_read(fe);
			file_event_unrefer(fe);
		}
		return ret;
	}
#endif

	FIBER_READ(sys_readv, fe, iov, iovcnt);
}

ssize_t fiber_recvmsg(FILE_EVENT *fe, struct msghdr *msg, int flags)
{
	CLR_POLLING(fe);

#ifdef HAS_IO_URING
	if (EVENT_IS_IO_URING(fiber_io_event())) {

#define SET_RECVMSG(f) do {                                                  \
    (f)->in.recvmsg_ctx.msg   = msg;                                         \
    (f)->in.recvmsg_ctx.flags = flags;                                       \
    (f)->mask |= EVENT_RECVMSG;                                              \
} while (0)

		int ret;

		if (!(fe->busy & EVENT_BUSY_READ)) {
			SET_RECVMSG(fe);

			fe->busy |= EVENT_BUSY_READ;
			ret = iocp_wait_read(fe);
			fe->busy &= ~EVENT_BUSY_READ;
		} else {
			FILE_ALLOC(fe, 0, fe->fd);
			SET_RECVMSG(fe);

			ret = iocp_wait_read(fe);
			file_event_unrefer(fe);
		}
		return ret;
	}
#endif

	FIBER_READ(sys_recvmsg, fe, msg, flags);
}

#endif  // SYS_UNIX

ssize_t fiber_recv(FILE_EVENT *fe, void *buf, size_t len, int flags)
{
	CLR_POLLING(fe);

#if defined(HAS_IOCP)
	if (EVENT_IS_IOCP(fiber_io_event())) {
		return fiber_iocp_read(fe, buf, len);
	}
#elif defined(HAS_IO_URING)
	if (EVENT_IS_IO_URING(fiber_io_event())) {

#define SET_RECV(f) do {                                                     \
    (f)->in.recv_ctx.buf   = buf;                                            \
    (f)->in.recv_ctx.len   = (unsigned) len;                                 \
    (f)->in.recv_ctx.flags = flags;                                          \
    (f)->mask |= EVENT_RECV;                                                 \
} while (0)

		int ret;

		if (!(fe->busy & EVENT_BUSY_READ)) {
			SET_RECV(fe);

			fe->busy |= EVENT_BUSY_READ;
			ret = iocp_wait_read(fe);
			fe->busy &= ~EVENT_BUSY_READ;
		} else {
			FILE_ALLOC(fe, 0, fe->fd);
			SET_RECV(fe);

			ret = iocp_wait_read(fe);
			file_event_unrefer(fe);
		}
		return ret;
	}
#endif

	FIBER_READ(sys_recv, fe, buf, len, flags);
}

ssize_t fiber_recvfrom(FILE_EVENT *fe, void *buf, size_t len,
	int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
	CLR_POLLING(fe);

#if  defined(HAS_IOCP)
	if (EVENT_IS_IOCP(fiber_io_event())) {
		return fiber_iocp_read(fe, buf, len);
	}
#elif  defined(HAS_IO_URING) && defined(IO_URING_HAS_RECVFROM)
	if (EVENT_IS_IO_URING(fiber_io_event())) {

#define SET_RECVFROM(f) do {                                                 \
    (f)->in.recvfrom_ctx.buf      = buf;                                     \
    (f)->in.recvfrom_ctx.len      = (unsigned) len;                          \
    (f)->in.recvfrom_ctx.flags    = flags;                                   \
    (f)->in.recvfrom_ctx.src_addr = src_addr;                                \
    (f)->in.recvfrom_ctx.addrlen  = addrlen;                                 \
    (f)->mask |= EVENT_RECVFROM;                                             \
} while (0)

		int ret;

		if (!(fe->busy & EVENT_BUSY_READ)) {
			SET_RECVFROM(fe);

			fe->busy |= EVENT_BUSY_READ;
			ret = iocp_wait_read(fe);
			fe->busy &= ~EVENT_BUSY_READ;
		} else {
			FILE_ALLOC(fe, 0, fe->fd);
			SET_RECVFROM(fe);

			ret = iocp_wait_read(fe);
			file_event_unrefer(fe);
		}
		return ret;
	}
#endif

	FIBER_READ(sys_recvfrom, fe, buf, len, flags, src_addr, addrlen);
}
