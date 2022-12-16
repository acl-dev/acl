#include "stdafx.h"
#include "common.h"

#ifdef HAS_IO_URING

#include <dlfcn.h>
#include <liburing.h>
#include "event.h"
#include "event_io_uring.h"

typedef struct EVENT_URING {
	EVENT event;
	struct io_uring ring;
	size_t sqe_size;
	size_t appending;
	size_t loop_count;
} EVENT_URING;

static void event_uring_free(EVENT *ev)
{
	EVENT_URING *ep = (EVENT_URING*) ev;

	io_uring_queue_exit(&ep->ring);
	mem_free(ep);
}

#define	TRY_SUBMMIT(e) do {  \
	if (++(e)->appending >= (e)->sqe_size) {  \
		(e)->appending = 0;  \
		io_uring_submit(&(e)->ring);  \
	}  \
} while (0)

#define	SUBMMIT(e) do {  \
	(e)->appending = 0;  \
	 io_uring_submit(&(e)->ring);  \
} while (0)

static void add_read_wait(EVENT_URING *ep, FILE_EVENT *fe, int tmo_ms)
{
	struct io_uring_sqe *sqe;

	sqe = io_uring_get_sqe(&ep->ring);
	io_uring_prep_poll_add(sqe, fe->fd, POLLIN | POLLHUP | POLLERR);
	fe->reader_ctx.fe = fe;
	io_uring_sqe_set_data(sqe, &fe->reader_ctx);
	sqe->flags = IOSQE_IO_LINK;

	TRY_SUBMMIT(ep);

	fe->rts.tv_sec  = tmo_ms / 1000;
	fe->rts.tv_nsec = (((long long) tmo_ms) % 1000) * 1000000;

	sqe = io_uring_get_sqe(&ep->ring);
	io_uring_prep_link_timeout(sqe, &fe->rts, 0);

	TRY_SUBMMIT(ep);
}

static int event_uring_add_read(EVENT_URING *ep, FILE_EVENT *fe)
{
	if (fe->mask & EVENT_READ) {
		return 0;
	}

	fe->mask |= EVENT_READ;
	fe->reader_ctx.mask = EVENT_READ;

	if (LIKELY(!(fe->mask & (EVENT_POLLIN | EVENT_ACCEPT)))) {
		struct io_uring_sqe *sqe = io_uring_get_sqe(&ep->ring);

		if (fe->mask & EVENT_READV) {
			io_uring_prep_readv(sqe, fe->fd,
				fe->in.readv_ctx.iov,
				fe->in.readv_ctx.cnt,
				fe->in.readv_ctx.off);
		} else if (fe->mask & EVENT_RECV) {
			io_uring_prep_recv(sqe, fe->fd,
				fe->in.recv_ctx.buf,
				fe->in.recv_ctx.len,
				fe->in.recv_ctx.flags);
#if defined(IO_URING_HAS_RECVFROM)
		} else if (fe->mask & EVENT_RECVFROM) {
			io_uring_prep_recvfrom(sqe, fe->fd,
				fe->in.recvfrom_ctx.buf,
				fe->in.recvfrom_ctx.len,
				fe->in.recvfrom_ctx.flags,
				fe->in.recvfrom_ctx.src_addr,
				fe->in.recvfrom_ctx.addrlen);
#endif
		} else if (fe->mask & EVENT_RECVMSG) {
			io_uring_prep_recvmsg(sqe, fe->fd,
				fe->in.recvmsg_ctx.msg,
				fe->in.recvmsg_ctx.flags);
		} else {
			io_uring_prep_read(sqe, fe->fd,
				fe->in.read_ctx.buf,
				fe->in.read_ctx.len,
				fe->in.read_ctx.off);
		}

		fe->reader_ctx.fe = fe;
		io_uring_sqe_set_data(sqe, &fe->reader_ctx);

		TRY_SUBMMIT(ep);
	} else if (fe->mask & EVENT_POLLIN) {
		add_read_wait(ep, fe, fe->r_timeout);
	} else if (fe->mask & EVENT_ACCEPT) {
		struct io_uring_sqe *sqe = io_uring_get_sqe(&ep->ring);
		fe->var.peer.len = (socklen_t) sizeof(fe->var.peer.addr);
		io_uring_prep_accept(sqe, fe->fd,
			(struct sockaddr*) &fe->var.peer.addr,
			(socklen_t*) &fe->var.peer.len, 0);

		fe->reader_ctx.fe = fe;
		io_uring_sqe_set_data(sqe, &fe->reader_ctx);

		TRY_SUBMMIT(ep);
	}

	return 0;
}

static void add_write_wait(EVENT_URING *ep, FILE_EVENT *fe, int tmo_ms)
{
	struct io_uring_sqe *sqe;

	sqe = io_uring_get_sqe(&ep->ring);
	io_uring_prep_poll_add(sqe, fe->fd, POLLOUT | POLLHUP | POLLERR);
	fe->writer_ctx.fe = fe;
	io_uring_sqe_set_data(sqe, &fe->writer_ctx);
	sqe->flags = IOSQE_IO_LINK;

	TRY_SUBMMIT(ep);

	fe->wts.tv_sec  = tmo_ms / 1000;
	fe->wts.tv_nsec = (((long long) tmo_ms) % 1000) * 1000000;

	sqe = io_uring_get_sqe(&ep->ring);
	io_uring_prep_link_timeout(sqe, &fe->wts, 0);

	TRY_SUBMMIT(ep);
}

static int event_uring_add_write(EVENT_URING *ep, FILE_EVENT *fe)
{
	if (fe->mask & EVENT_WRITE) {
		return 0;
	}

	fe->mask |= EVENT_WRITE;
	fe->writer_ctx.mask = EVENT_WRITE;
	//fe->writer_ctx.cnt++;

	if (LIKELY(!(fe->mask & (EVENT_POLLOUT | EVENT_CONNECT)))) {
		struct io_uring_sqe *sqe = io_uring_get_sqe(&ep->ring);

		if (fe->mask & EVENT_WRITEV) {
			io_uring_prep_writev(sqe, fe->fd,
				fe->out.writev_ctx.iov,
				fe->out.writev_ctx.cnt,
				fe->out.writev_ctx.off);
		} else if (fe->mask & EVENT_SEND) {
			io_uring_prep_send(sqe, fe->fd,
				fe->out.send_ctx.buf,
				fe->out.send_ctx.len,
				fe->out.send_ctx.flags);
#if defined(IO_URING_HAS_SENDTO)
		} else if (fe->mask & EVENT_SENDTO) {
			io_uring_prep_sendto(sqe, fe->fd,
				fe->out.sendto_ctx.buf,
				fe->out.sendto_ctx.len,
				fe->out.sendto_ctx.flags,
				fe->out.sendto_ctx.dest_addr,
				fe->out.sendto_ctx.addrlen);
#endif
		} else if (fe->mask & EVENT_SENDMSG) {
			io_uring_prep_sendmsg(sqe, fe->fd,
				fe->out.sendmsg_ctx.msg,
				fe->out.sendmsg_ctx.flags);
		} else {
			io_uring_prep_write(sqe, fe->fd,
				fe->out.write_ctx.buf,
				fe->out.write_ctx.len,
				fe->out.write_ctx.off);
		}

		fe->writer_ctx.fe = fe;
		io_uring_sqe_set_data(sqe, &fe->writer_ctx);

		TRY_SUBMMIT(ep);
	} else if (fe->mask & EVENT_POLLOUT) {
		add_write_wait(ep, fe, fe->r_timeout);
	} else if (fe->mask & EVENT_CONNECT) {
		non_blocking(fe->fd, 1);
		struct io_uring_sqe *sqe = io_uring_get_sqe(&ep->ring);
		io_uring_prep_connect(sqe, fe->fd,
			(struct sockaddr*) &fe->var.peer.addr,
			(socklen_t) fe->var.peer.len);

		fe->writer_ctx.fe = fe;
		io_uring_sqe_set_data(sqe, &fe->writer_ctx);

		TRY_SUBMMIT(ep);
	}

	return 0;
}

void event_uring_file_close(EVENT *ev, FILE_EVENT *fe)
{
	EVENT_URING *ep = (EVENT_URING*) ev;
	struct io_uring_sqe *sqe = io_uring_get_sqe(&ep->ring);

	io_uring_prep_close(sqe, fe->fd);
	fe->reader_ctx.fe = fe;
	io_uring_sqe_set_data(sqe, &fe->reader_ctx);
	TRY_SUBMMIT(ep);
}

void event_uring_file_cancel(EVENT *ev, FILE_EVENT *fe_orig, FILE_EVENT *fe)
{
	EVENT_URING *ep = (EVENT_URING*) ev;
	struct io_uring_sqe *sqe = io_uring_get_sqe(&ep->ring);

	io_uring_prep_cancel(sqe, &fe_orig->reader_ctx, 0);
	fe->reader_ctx.fe = fe;
	io_uring_sqe_set_data(sqe, &fe->reader_ctx);
	TRY_SUBMMIT(ep);
}

void event_uring_file_openat(EVENT *ev, FILE_EVENT *fe, int dirfd,
	const char *pathname, int flags, mode_t mode)
{
	EVENT_URING *ep = (EVENT_URING*) ev;
	struct io_uring_sqe *sqe = io_uring_get_sqe(&ep->ring);

	io_uring_prep_openat(sqe, dirfd, pathname, flags, mode);
	fe->reader_ctx.fe = fe;
	io_uring_sqe_set_data(sqe, &fe->reader_ctx);
	TRY_SUBMMIT(ep);
}

void event_uring_file_unlink(EVENT *ev, FILE_EVENT *fe, const char *pathname)
{
	EVENT_URING *ep = (EVENT_URING*) ev;
	struct io_uring_sqe *sqe = io_uring_get_sqe(&ep->ring);

	io_uring_prep_unlink(sqe, pathname, 0);
	fe->reader_ctx.fe = fe;
	io_uring_sqe_set_data(sqe, &fe->reader_ctx);
	TRY_SUBMMIT(ep);
}

#ifdef HAS_STATX
void event_uring_file_statx(EVENT *ev, FILE_EVENT *fe, int dirfd,
	const char *pathname, int flags, unsigned int mask,
	struct statx *statxbuf)
{
	EVENT_URING *ep = (EVENT_URING*) ev;
	struct io_uring_sqe *sqe = io_uring_get_sqe(&ep->ring);

	io_uring_prep_statx(sqe, dirfd, pathname, flags, mask, statxbuf);
	fe->reader_ctx.fe = fe;
	io_uring_sqe_set_data(sqe, &fe->reader_ctx);
	TRY_SUBMMIT(ep);
}
#endif

#ifdef HAS_RENAMEAT2
void event_uring_file_renameat2(EVENT *ev, FILE_EVENT *fe, int olddirfd,
	const char *oldpath, int newdirfd, const char *newpath, unsigned flags)
{
	EVENT_URING *ep = (EVENT_URING*) ev;
	struct io_uring_sqe *sqe = io_uring_get_sqe(&ep->ring);

	io_uring_prep_renameat(sqe, olddirfd, oldpath, newdirfd, newpath, flags);
	fe->reader_ctx.fe = fe;
	io_uring_sqe_set_data(sqe, &fe->reader_ctx);
	TRY_SUBMMIT(ep);
}
#endif

void event_uring_mkdirat(EVENT *ev, FILE_EVENT *fe, int dirfd,
	const char *pathname, mode_t mode)
{
	EVENT_URING *ep = (EVENT_URING*) ev;
	struct io_uring_sqe *sqe = io_uring_get_sqe(&ep->ring);

	io_uring_prep_mkdirat(sqe, dirfd, pathname, mode);
	fe->reader_ctx.fe = fe;
	io_uring_sqe_set_data(sqe, &fe->reader_ctx);
	TRY_SUBMMIT(ep);
}

void event_uring_splice(EVENT *ev, FILE_EVENT *fe, int fd_in, loff_t off_in,
	int fd_out, loff_t off_out, size_t len, unsigned int splice_flags,
	unsigned int sqe_flags, __u8 opcode)
{
	EVENT_URING *ep = (EVENT_URING*) ev;
	struct io_uring_sqe *sqe = io_uring_get_sqe(&ep->ring);

	io_uring_prep_splice(sqe, fd_in, off_in, fd_out, off_out,
		len, splice_flags);
	fe->reader_ctx.fe = fe;
	io_uring_sqe_set_data(sqe, &fe->reader_ctx);
	sqe->flags |= sqe_flags;
	sqe->opcode = opcode;
	TRY_SUBMMIT(ep);
}

#if 0
// Some problems can't be resolve current, so I use another way to do it.

void event_uring_sendfile(EVENT *ev, FILE_EVENT *fe, int out, int in,
	off64_t off, size_t cnt)
{
	EVENT_URING *ep = (EVENT_URING*) ev;
	struct io_uring_sqe *sqe = io_uring_get_sqe(&ep->ring);
	unsigned flags = SPLICE_F_MOVE | SPLICE_F_MORE; // | SPLICE_F_NONBLOCK;

	io_uring_prep_splice(sqe, in, off, fe->var.pipefd[1], -1, cnt, flags);
	fe->reader_ctx.fe = fe;
	io_uring_sqe_set_data(sqe, &fe->reader_ctx);
	sqe->flags |= IOSQE_IO_LINK | SPLICE_F_FD_IN_FIXED | IOSQE_ASYNC;
	sqe->opcode = IORING_OP_SPLICE;

	TRY_SUBMMIT(ep);

	flags = 0;
	sqe = io_uring_get_sqe(&ep->ring);
	io_uring_prep_splice(sqe, fe->var.pipefd[0], -1, out, -1, cnt, flags);
	fe->reader_ctx.fe = fe;
	io_uring_sqe_set_data(sqe, &fe->reader_ctx);
	sqe->opcode = IORING_OP_SPLICE;

	TRY_SUBMMIT(ep);
}
#endif

static int event_uring_del_read(EVENT_URING *ep UNUSED, FILE_EVENT *fe)
{
	if (!(fe->mask & EVENT_READ)) {
		return 0;
	}

	fe->mask &= ~EVENT_READ;
	return 0;
}

static int event_uring_del_write(EVENT_URING *ep UNUSED, FILE_EVENT *fe)
{
	if (!(fe->mask & EVENT_WRITE)) {
		return 0;
	}

	fe->mask &= ~EVENT_WRITE;
	return 0;
}

#define	ERR	(POLLERR | POLLHUP | POLLNVAL)

static void handle_read(EVENT *ev, FILE_EVENT *fe, int res)
{
	fe->reader_ctx.res = res;

	if (LIKELY(!(fe->mask & (EVENT_ACCEPT | EVENT_POLLIN)))) {
		if ((fe->type & TYPE_FILE) && res > 0) {
			if (fe->mask & EVENT_READV) {
				fe->in.readv_ctx.off += res;
			} else {
				fe->in.read_ctx.off  += res;
			}
		}
	} else if (fe->mask & EVENT_ACCEPT) {
		// Do nothing!
	} else if (fe->mask & EVENT_POLLIN) {
		if (res & (POLLIN | ERR)) {
			if (res & POLLERR) {
				fe->mask |= EVENT_ERR;
			}
			if (res & POLLHUP) {
				fe->mask |= EVENT_HUP;
			}
			if (res & POLLNVAL) {
				fe->mask |= EVENT_NVAL;;
			}

			fe->mask &= ~EVENT_POLLIN;
			CLR_READWAIT(fe);
		} else {
			msg_error("%s(%d): unknown res=%d, fd=%d",
				__FUNCTION__, __LINE__, res, fe->fd);
		}
	}

	fe->mask &= ~(EVENT_READ | EVENT_READV | EVENT_RECV
			| EVENT_RECVFROM | EVENT_RECVMSG);
	fe->r_proc(ev, fe);
}

static void handle_write(EVENT *ev, FILE_EVENT *fe, int res)
{
	fe->writer_ctx.res = res;

	if (LIKELY(!(fe->mask & (EVENT_CONNECT | EVENT_POLLOUT)))) {
		if ((fe->type & TYPE_FILE) && res > 0) {
			if (fe->mask & EVENT_WRITEV) {
				fe->out.writev_ctx.off += res;
			} else {
				fe->out.write_ctx.off  += res;
			}
		}
	} else if (fe->mask & EVENT_CONNECT) {
		// Do nothing!
	} else if (fe->mask & EVENT_POLLOUT) {
		if (res & (POLLOUT | ERR)) {
			if (res & POLLERR) {
				fe->mask |= EVENT_ERR;
			}
			if (res & POLLHUP) {
				fe->mask |= EVENT_HUP;
			}
			if (res & POLLNVAL) {
				fe->mask |= EVENT_NVAL;;
			}

			fe->mask &= ~EVENT_POLLOUT;
			CLR_WRITEWAIT(fe);
		} else {
			msg_error("%s(%d): unknown res=%d, fd=%d",
				__FUNCTION__, __LINE__, res, fe->fd);
		}
	}

	fe->mask &= ~(EVENT_WRITE | EVENT_WRITEV | EVENT_SEND
			| EVENT_SENDTO | EVENT_SENDMSG);
	if (fe->w_proc) {
		fe->w_proc(ev, fe);
	}
}

static void handle_one(EVENT *ev, IO_URING_CTX *ctx, int res)
{
	FILE_EVENT *fe = ctx->fe;

	if (fe == NULL) { // xxx?
		msg_warn("%s(%d): fe null here?", __FUNCTION__, __LINE__);
		return;
	}

	if (ctx == &fe->reader_ctx && ctx->mask == EVENT_READ && fe->r_proc) {
		fe->mask &= ~EVENT_READ;
		handle_read(ev, fe, res);
		ctx->mask = 0;
		return;
	}

	if (ctx == &fe->writer_ctx && ctx->mask == EVENT_WRITE && fe->w_proc) {
		//fe->writer_ctx.cnt--;
		fe->mask &= ~EVENT_WRITE;
		ctx->mask = 0;
		handle_write(ev, fe, res);
		return;
	}

	if (fe->r_proc == NULL) {
		return;
	}

#define	FLAGS	(EVENT_FILE_CLOSE \
		| EVENT_FILE_CANCEL \
		| EVENT_FILE_OPENAT \
		| EVENT_FILE_UNLINK \
		| EVENT_FILE_STAT \
		| EVENT_FILE_STATX \
		| EVENT_FILE_RENAMEAT \
		| EVENT_FILE_RENAMEAT2 \
		| EVENT_DIR_MKDIRAT \
		| EVENT_SPLICE)

	fe->reader_ctx.res = res;

	if (fe->mask & FLAGS) {
		fe->r_proc(ev, fe);
	} else {
		msg_error("%s(%d): unknown mask=%u, fd=%d, fe=%p, "
			"EVENT_SYSIO=%s, EVENT_READ=%s, EVENT_POLLIN=%s",
			__FUNCTION__, __LINE__, (fe->mask & ~FLAGS), fe->fd, fe,
			fe->mask & EVENT_SYSIO ? "yes" : "no",
			fe->mask & EVENT_READ ? "yes" : "no",
			fe->mask & EVENT_POLLIN ? "yes" : "no");
	}
}

static int peek_more(EVENT_URING *ep)
{
	struct io_uring_cqe *cqe;
	unsigned head, count = 0;
	IO_URING_CTX *ctx;
	int ret;

	io_uring_for_each_cqe(&ep->ring, head, cqe) {
		count++;
		ctx = (IO_URING_CTX*) io_uring_cqe_get_data(cqe);
		ret = cqe->res;
		//io_uring_cqe_seen(&ep->ring, cqe);

		if (ret == -ENOBUFS) {
			msg_error("%s(%d): ENOBUFS error", __FUNCTION__, __LINE__);
			return -1;
		}

		if (ret == -ETIME || ret == -ECANCELED || ctx == NULL) {
			continue;
		}

		handle_one((EVENT*) ep, ctx, ret);
	}

	if (count > 0) {
		io_uring_cq_advance(&ep->ring, count);
	}

	return count;
}

static int submit_and_wait(EVENT_URING *ep, int timeout)
{
	struct __kernel_timespec ts, *tp;
	struct io_uring_cqe *cqe;
	IO_URING_CTX *ctx;
	int ret;

	if (timeout >= 0) {
		ts.tv_sec  = timeout / 1000;
		ts.tv_nsec = (((long long) timeout) % 1000) * 1000000;
		tp         = &ts;
	} else {
		ts.tv_sec  = 0;
		ts.tv_nsec = 0;
		tp         = NULL;
	}

AGAIN:
	if (ep->appending > 0) {
		ep->appending = 0;
		ret = io_uring_submit_and_wait_timeout(&ep->ring, &cqe,
				1, tp, NULL);
	} else {
		ret = io_uring_wait_cqes(&ep->ring, &cqe, 1, tp, NULL);
	}

	if (ret < 0) {
		if (ret == -ETIME) {
			return 0;
		} else if (ret == -EAGAIN) {
			return 0;
		} else if (ret == -EINTR) {
			goto AGAIN;
		}

		msg_error("%s(%d): io_uring_wait_cqe error=%s",
			__FUNCTION__, __LINE__, strerror(-ret));
		return -1;
	}

	if (cqe == NULL) {
		return 0;
	}

	ret = cqe->res;
	ctx = (IO_URING_CTX*) io_uring_cqe_get_data(cqe);

	io_uring_cqe_seen(&ep->ring, cqe);

	if (ret == -ENOBUFS) {
		msg_error("%s(%d): ENOBUFS error", __FUNCTION__, __LINE__);
		return -1;
	}

	if (ret == -ETIME || ret == -ECANCELED || ctx == NULL) {
		return 1;
	}

	handle_one((EVENT*) ep, ctx, ret);
	return 1;
}

static int event_uring_wait(EVENT *ev, int timeout)
{
	EVENT_URING *ep = (EVENT_URING*) ev;
	int ret, count = 0;

	ep->loop_count++;
	ret = submit_and_wait(ep, timeout);
	if (ret == 0) {
		return 0;
	} else if (ret < 0) {
		return -1;
	}
	count += ret;

	ret = peek_more(ep);
	if (ret == 0) {
		return count;
	} else if (ret < 0) {
		return -1;
	}

	count += ret;
	//usleep(10000);

	return count;
}

static int event_uring_checkfd(EVENT *ev UNUSED, FILE_EVENT *fe UNUSED)
{
	return 0;
}

static long event_uring_handle(EVENT *ev)
{
	EVENT_URING *ep = (EVENT_URING *) ev;
	return (long) &ep->ring;
}

static const char *event_uring_name(void)
{
	return "io_uring";
}

EVENT *event_io_uring_create(int size)
{
	EVENT_URING *eu = (EVENT_URING *) mem_calloc(1, sizeof(EVENT_URING));
	struct io_uring_params params;
	int ret;

	if (size <= 0 || size > 2048) {
		eu->sqe_size = 2048;
	} else {
		eu->sqe_size = size;
	}

	// XXX: Don't write log here to avoid IO write recursive!

	//eu->sqe_size = 256;

	memset(&params, 0, sizeof(params));
	//params.flags = IORING_SETUP_SQPOLL;

	ret = io_uring_queue_init_params(eu->sqe_size, &eu->ring, &params);
	if (ret < 0) {
		printf("%s(%d): init io_uring error=%s, size=%zd\r\n",
			__FUNCTION__, __LINE__, strerror(-ret), eu->sqe_size);
		abort();
	} else {
		printf("%s(%d): thread-%lu init io_uring ok, size=%zd\r\n",
			__FUNCTION__, __LINE__, thread_self(), eu->sqe_size);
	}

	if (!(params.features & IORING_FEAT_FAST_POLL)) {
		printf("IORING_FEAT_FAST_POLL not available in the kernel\r\n");
	} else {
		printf("IORING_FEAT_FAST_POLL is available in the kernel\r\n");
	}

	eu->appending    = 0;

	eu->event.name   = event_uring_name;
	eu->event.handle = (acl_handle_t (*)(EVENT *)) event_uring_handle;
	eu->event.free   = event_uring_free;
	eu->event.flag   = EVENT_F_IO_URING;

	eu->event.event_wait = event_uring_wait;
	eu->event.checkfd    = (event_oper *) event_uring_checkfd;
	eu->event.add_read   = (event_oper *) event_uring_add_read;
	eu->event.add_write  = (event_oper *) event_uring_add_write;
	eu->event.del_read   = (event_oper *) event_uring_del_read;
	eu->event.del_write  = (event_oper *) event_uring_del_write;

	return (EVENT*) eu;
}

#endif /* HAS_IO_URING */
