#include "stdafx.h"
#include "common.h"

#include "fiber.h"
#include "hook.h"

// In the API connect() being hooked in hook/socket.c, the STATUS_NDUBLOCK
// flag was set and the fd was in non-block status in order to return imaginary
// from connecting process.

#if defined(HAS_IO_URING)
# define CHECK_SET_NBLOCK(_fd) do { \
	if (var_hook_sys_api && !EVENT_IS_IO_URING(fiber_io_event())) { \
		FILE_EVENT *_fe = fiber_file_get(_fd); \
		if (_fe && IS_NDUBLOCK(_fe)) { \
			non_blocking(_fd, NON_BLOCKING); \
			CLR_NDUBLOCK(_fe); \
		} \
	} \
} while (0)
#else
# define CHECK_SET_NBLOCK(_fd) do { \
	if (var_hook_sys_api) { \
		FILE_EVENT *_fe = fiber_file_get(_fd); \
		if (_fe && IS_NDUBLOCK(_fe)) { \
			non_blocking(_fd, NON_BLOCKING); \
			CLR_NDUBLOCK(_fe); \
		} \
	} \
} while (0)
#endif

static int wait_write(FILE_EVENT *fe)
{
	CLR_POLLING(fe);

	if (fiber_wait_write(fe) < 0) {
		fiber_file_free(fe);
		return -1;
	}

	if (fe->mask & (EVENT_ERR | EVENT_HUP | EVENT_NVAL)) {
		int err = acl_fiber_last_error();
		fiber_save_errno(err);
		return -1;
	}

	if (acl_fiber_canceled(fe->fiber_w)) {
		acl_fiber_set_error(fe->fiber_w->errnum);
		return -1;
	}

	return 0;
}

#if defined(HAS_IO_URING)
static int iocp_wait_write(FILE_EVENT *fe)
{
	while (1) {
		int err;

		fe->mask &= ~EVENT_WRITE;
#ifdef HAS_IO_URING
		fe->writer_ctx.res = -1;
#endif

		if (wait_write(fe) == -1) {
			return -1;
		}

#ifdef HAS_IO_URING
		if (fe->writer_ctx.res >= 0) {
			return fe->writer_ctx.res;
		}
#endif

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
#endif

#if defined(HAS_IO_URING)
int fiber_iocp_write(FILE_EVENT *fe, const char *buf, int len)
{
	fe->out.write_ctx.buf = buf;
	fe->out.write_ctx.len = len;
	return iocp_wait_write(fe);
}
#endif // HAS_IO_URING

#define	CHECK_WRITE_RESULT(_fe, _n) do {                                     \
	int _err;                                                            \
	if (_n >= 0) {                                                       \
		return _n;                                                   \
	}                                                                    \
	_err = acl_fiber_last_error();                                       \
	fiber_save_errno(_err);                                              \
	if (!error_again(_err)) {                                            \
		return -1;                                                   \
	}                                                                    \
	if (wait_write(_fe) == -1) {                                         \
		return -1;                                                   \
	}                                                                    \
} while (0)

#ifdef SYS_UNIX
ssize_t fiber_write(FILE_EVENT *fe, const void *buf, size_t count)
{
	CLR_POLLING(fe);

#if defined(HAS_IO_URING)
	if (EVENT_IS_IO_URING(fiber_io_event()) && !(fe->mask & EVENT_SYSIO)) {
		fe->out.write_ctx.buf = buf;
		fe->out.write_ctx.len = (unsigned) count;

		return iocp_wait_write(fe);
	}
#endif

	CHECK_SET_NBLOCK(fe->fd);

	while (1) {
		ssize_t n = (*sys_write)(fe->fd, buf, count);

		CHECK_WRITE_RESULT(fe, n);
	}
}

ssize_t fiber_writev(FILE_EVENT *fe, const struct iovec *iov, int iovcnt)
{
	CLR_POLLING(fe);

#if defined(HAS_IO_URING)
	if (EVENT_IS_IO_URING(fiber_io_event())) {
		fe->out.writev_ctx.iov = iov;
		fe->out.writev_ctx.cnt = iovcnt;
		fe->out.writev_ctx.off = 0;
		fe->mask |= EVENT_WRITEV;

		return iocp_wait_write(fe);
	}
#endif

	CHECK_SET_NBLOCK(fe->fd);

	while (1) {
		int n = (int) (*sys_writev)(fe->fd, iov, iovcnt);

		CHECK_WRITE_RESULT(fe, n);
	}
}
#endif

ssize_t fiber_send(FILE_EVENT *fe, const void *buf, size_t len, int flags)
{
	CLR_POLLING(fe);

#if defined(HAS_IO_URING)
	if (EVENT_IS_IO_URING(fiber_io_event())) {
		fe->out.send_ctx.buf   = buf;
		fe->out.send_ctx.len   = (unsigned) len;
		fe->out.send_ctx.flags = flags;
		fe->mask |= ~EVENT_SEND;

		return iocp_wait_write(fe);
	}
#endif

	CHECK_SET_NBLOCK(fe->fd);

	while (1) {
		int n = (int) (*sys_send)(fe->fd, buf, len, flags);

		CHECK_WRITE_RESULT(fe, n);
	}
}

ssize_t fiber_sendto(FILE_EVENT *fe, const void *buf, size_t len,
	int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
	CLR_POLLING(fe);

#if defined(HAS_IO_URING) && defined(IO_URING_HAS_SENDTO)
	if (EVENT_IS_IO_URING(fiber_io_event())) {
		fe->out.sendto_ctx.buf       = buf;
		fe->out.sendto_ctx.len       = (unsigned) len;
		fe->out.sendto_ctx.flags     = flags;
		fe->out.sendto_ctx.dest_addr = dest_addr;
		fe->out.sendto_ctx.addrlen   = addrlen;
		fe->mask |= EVENT_SENDTO;

		return iocp_wait_write(fe);
	}
#endif

	CHECK_SET_NBLOCK(fe->fd);

	while (1) {
		int n = (int) (*sys_sendto)(fe->fd, buf, len, flags,
				dest_addr, addrlen);

		CHECK_WRITE_RESULT(fe, n);
	}
}

#ifdef SYS_UNIX
ssize_t fiber_sendmsg(FILE_EVENT *fe, const struct msghdr *msg, int flags)
{
	CLR_POLLING(fe);

#if defined(HAS_IO_URING)
	if (EVENT_IS_IO_URING(fiber_io_event())) {
		fe->out.sendmsg_ctx.msg   = msg;
		fe->out.sendmsg_ctx.flags = flags;
		fe->mask |= EVENT_SENDMSG;

		return iocp_wait_write(fe);
	}
#endif
	CHECK_SET_NBLOCK(fe->fd);

	while (1) {
		ssize_t n = (*sys_sendmsg)(fe->fd, msg, flags);

		CHECK_WRITE_RESULT(fe, n);
	}
}
#endif

#if defined(__USE_LARGEFILE64) && !defined(DISABLE_HOOK_IO)

ssize_t fiber_sendfile64(socket_t out_fd, int in_fd, off64_t *offset, size_t count)
{
#ifdef	HAS_IO_URING
	if (EVENT_IS_IO_URING(fiber_io_event())) {
		return file_sendfile(out_fd, in_fd, offset, count);
	}
#endif

	CHECK_SET_NBLOCK(out_fd);

	while (1) {
		ssize_t n = (*sys_sendfile64)(out_fd, in_fd, offset, count);
		FILE_EVENT *fe;
		int err;

		if (n >= 0) {
			return n;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			return -1;
		}

		fe = fiber_file_open_write(out_fd);
		CLR_POLLING(fe);

		if (fiber_wait_write(fe) < 0) {
			msg_error("%s(%d): fiber_wait_write error=%s, fd=%d",
				__FUNCTION__, __LINE__, last_serror(), (int) out_fd);
			fiber_file_free(fe);
			return -1;
		}

		if (IS_CLOSING(fe)) {
			msg_info("%s(%d): fd=%d being closing",
				__FUNCTION__, __LINE__, (int) out_fd);
			return 0;
		}

		if (fe->mask & (EVENT_ERR | EVENT_HUP | EVENT_NVAL)) {
			msg_error("%s(%d): fd=%d error",
				__FUNCTION__, __LINE__, out_fd);
			return -1;
		}

		if (acl_fiber_canceled(fe->fiber_w)) {
			acl_fiber_set_error(fe->fiber_w->errnum);
			return -1;
		}
	}
}

#endif
