#include "stdafx.h"
#include "common.h"

#include "fiber.h"
#include "hook.h"

#ifdef	HAS_IO_URING
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../event/event_io_uring.h"

#define	CHECK_API(name, fn) do {  \
	if ((fn) == NULL) {  \
		hook_once();  \
		if ((fn) == NULL) {  \
			msg_error("%s: %s NULL", __FUNCTION__, (name));  \
			return -1;  \
		}  \
	}  \
} while (0)

#define	FILE_ALLOC(fe, type) do {  \
	(fe) = file_event_alloc(-1);  \
	(fe)->fiber_r = acl_fiber_running();  \
	(fe)->fiber_w = acl_fiber_running();  \
	(fe)->fiber_r->status = FIBER_STATUS_NONE;  \
	(fe)->r_proc = file_default_callback;  \
	(fe)->mask = (type);  \
} while (0)

static void file_default_callback(EVENT *ev UNUSED, FILE_EVENT *fe)
{
	if (fe->fiber_r->status != FIBER_STATUS_READY) {
		acl_fiber_ready(fe->fiber_r);
	}
}

int file_close(EVENT *ev, FILE_EVENT *fe)
{
	CHECK_API("sys_close", sys_close);

	if (!var_hook_sys_api) {
		return (*sys_close)(fe->fd);
	}

	if (!EVENT_IS_IO_URING(ev)) {
		return (*sys_close)(fe->fd);
	}

	fe->fiber_r = acl_fiber_running();
	fe->fiber_r->status = FIBER_STATUS_NONE;
	fe->r_proc = file_default_callback;
	fe->mask = EVENT_FILE_CLOSE;

	event_uring_file_close(ev, fe);

	fiber_io_inc();
	acl_fiber_switch();
	fiber_io_dec();

	fe->mask &= ~EVENT_FILE_CLOSE;

	if (fe->fd == 0) {
		file_event_unrefer(fe);
		return 0;
	} else {
		acl_fiber_set_error(fe->fd);
		file_event_unrefer(fe);
		return -1;
	}
}

int openat(int dirfd, const char *pathname, int flags, ...)
{
	FILE_EVENT *fe;
	EVENT *ev;
	mode_t mode;
	va_list ap;

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	CHECK_API("sys_openat", sys_openat);

	if (!var_hook_sys_api) {
		return (*sys_openat)(dirfd, pathname, flags, mode);
	}

	fiber_io_check();
	ev = fiber_io_event();
	if (!EVENT_IS_IO_URING(ev)) {
		return (*sys_openat)(dirfd, pathname, flags, mode);
	}

	FILE_ALLOC(fe, EVENT_FILE_OPENAT);
	fe->rbuf = strdup(pathname);

	event_uring_file_openat(ev, fe, dirfd, fe->rbuf, flags, mode);

	fiber_io_inc();
	acl_fiber_switch();
	fiber_io_dec();

	fe->mask &= ~EVENT_FILE_OPENAT;
	free(fe->rbuf);
	fe->rbuf = NULL;

	if (fe->fd >= 0) {
		fiber_file_set(fe);
		fe->type = TYPE_FILE | TYPE_EVENTABLE;
		return fe->fd;
	}

	acl_fiber_set_error(-fe->fd);
	file_event_unrefer(fe);
	return -1;
}

int open(const char *pathname, int flags, ...)
{
	mode_t mode;
	va_list ap;

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	return openat(AT_FDCWD, pathname, flags, mode);
}

int unlink(const char *pathname)
{
	FILE_EVENT *fe;
	EVENT *ev;

	CHECK_API("sys_unlink", sys_unlink);

	if (!var_hook_sys_api) {
		return (*sys_unlink)(pathname);
	}

	fiber_io_check();
	ev = fiber_io_event();
	if (!EVENT_IS_IO_URING(ev)) {
		return (*sys_unlink)(pathname);
	}

	FILE_ALLOC(fe, EVENT_FILE_UNLINK);
	fe->rbuf = strdup(pathname);

	event_uring_file_unlink(ev, fe, fe->rbuf);

	fiber_io_inc();
	acl_fiber_switch();
	fiber_io_dec();

	fe->mask &= ~EVENT_FILE_UNLINK;
	free(fe->rbuf);
	fe->rbuf = NULL;

	if (fe->fd == 0) {
		file_event_unrefer(fe);
		return 0;
	} else {
		acl_fiber_set_error(-fe->fd);
		file_event_unrefer(fe);
		return -1;
	}
}

int renameat2(int olddirfd, const char *oldpath,
	int newdirfd, const char *newpath, unsigned int flags)
{
	FILE_EVENT *fe;
	EVENT *ev;

	CHECK_API("sys_renameat2", sys_renameat2);

	if (!var_hook_sys_api) {
		return (*sys_renameat2)(olddirfd, oldpath, newdirfd, newpath, flags);
	}

	fiber_io_check();
	ev = fiber_io_event();
	if (!EVENT_IS_IO_URING(ev)) {
		return (*sys_renameat2)(olddirfd, oldpath, newdirfd, newpath, flags);
	}

	FILE_ALLOC(fe, EVENT_FILE_RENAMEAT2);
	fe->rbuf = strdup(oldpath);
	fe->var.path = strdup(newpath);

	event_uring_file_renameat2(ev, fe, olddirfd, fe->rbuf,
		newdirfd, fe->var.path, flags);

	fiber_io_inc();
	acl_fiber_switch();
	fiber_io_dec();

	fe->mask &= ~EVENT_FILE_RENAMEAT2;
	free(fe->rbuf);
	free(fe->var.path);

	if (fe->fd == 0) {
		file_event_unrefer(fe);
		return 0;
	} else {
		acl_fiber_set_error(fe->fd);
		file_event_unrefer(fe);
		return -1;
	}
}

int renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath)
{
	return renameat2(olddirfd, oldpath, newdirfd, newpath, 0);
}

int rename(const char *oldpath, const char *newpath)
{
	return renameat(AT_FDCWD, oldpath, AT_FDCWD, newpath);
}

int statx(int dirfd, const char *pathname, int flags, unsigned int mask,
	struct statx *statxbuf)
{
	FILE_EVENT *fe;
	EVENT *ev;

	CHECK_API("sys_statx", sys_statx);

	if (!var_hook_sys_api) {
		return (*sys_statx)(dirfd, pathname, flags, mask, statxbuf);
	}

	fiber_io_check();
	ev = fiber_io_event();
	if (!EVENT_IS_IO_URING(ev)) {
		return (*sys_statx)(dirfd, pathname, flags, mask, statxbuf);
	}

	FILE_ALLOC(fe, EVENT_FILE_STATX);
	fe->rbuf = strdup(pathname);
	memcpy(&fe->var.statxbuf, statxbuf, sizeof(struct statx));

	event_uring_file_statx(ev, fe, dirfd, fe->rbuf, flags, mask,
		&fe->var.statxbuf);

	fiber_io_inc();
	acl_fiber_switch();
	fiber_io_dec();

	fe->mask &= ~EVENT_FILE_STATX;
	free(fe->rbuf);
	fe->rbuf = NULL;

	if (fe->fd == 0) {
		file_event_unrefer(fe);
		memcpy(statxbuf, &fe->var.statxbuf, sizeof(struct statx));
		return 0;
	} else {
		acl_fiber_set_error(fe->fd);
		file_event_unrefer(fe);
		return -1;
	}
}

int stat(const char *pathname, struct stat *statbuf)
{
	int flags = AT_STATX_SYNC_AS_STAT;
	unsigned int mask = STATX_ALL;
	struct statx statxbuf;

	if (statx(AT_FDCWD, pathname, flags, mask, &statxbuf) == -1) {
		return -1;
	}

	statbuf->st_dev = statxbuf.stx_dev_major;
	statbuf->st_ino = statxbuf.stx_ino;
	statbuf->st_mode = statxbuf.stx_mode;
	statbuf->st_nlink = statxbuf.stx_nlink;
	statbuf->st_uid = statxbuf.stx_uid;
	statbuf->st_gid = statxbuf.stx_gid;
	statbuf->st_rdev = statxbuf.stx_rdev_major;
	statbuf->st_size = statxbuf.stx_size;
	statbuf->st_blksize = statxbuf.stx_blksize;
	statbuf->st_blocks = statxbuf.stx_blocks;
	statbuf->st_atim.tv_sec = statxbuf.stx_atime.tv_sec;
	statbuf->st_mtim.tv_sec = statxbuf.stx_mtime.tv_sec;
	statbuf->st_ctim.tv_sec = statxbuf.stx_ctime.tv_sec;
	return 0;
} 

int mkdirat(int dirfd, const char *pathname, mode_t mode)
{
	FILE_EVENT *fe;
	EVENT *ev;

	CHECK_API("sys_mkdirat", sys_mkdirat);

	if (!var_hook_sys_api) {
		return (*sys_mkdirat)(dirfd, pathname, mode);
	}

	fiber_io_check();
	ev = fiber_io_event();
	if (!EVENT_IS_IO_URING(ev)) {
		return (*sys_mkdirat)(dirfd, pathname, mode);
	}

	FILE_ALLOC(fe, EVENT_DIR_MKDIRAT);
	fe->rbuf = strdup(pathname);

	event_uring_mkdirat(ev, fe, dirfd, fe->rbuf, mode);

	fiber_io_inc();
	acl_fiber_switch();
	fiber_io_dec();

	fe->mask &= ~EVENT_DIR_MKDIRAT;
	free(fe->rbuf);

	if (fe->fd == 0) {
		file_event_unrefer(fe);
		return 0;
	} else {
		acl_fiber_set_error(fe->fd);
		file_event_unrefer(fe);
		return -1;
	}
}

#endif // HAS_IO_URING

