#ifndef EVENT_IO_URING_INCLUDE_H
#define EVENT_IO_URING_INCLUDE_H

#include "event.h"

#ifdef HAS_EPOLL

EVENT *event_io_uring_create(int size);

void event_uring_file_close(EVENT *ev, FILE_EVENT *fe);
void event_uring_file_openat(EVENT* ev, FILE_EVENT *fe, int dirfd,
	const char* pathname, int flags, mode_t mode);
void event_uring_file_unlink(EVENT *ev, FILE_EVENT *fe, const char *pathname);
void event_uring_file_statx(EVENT *ev, FILE_EVENT *fe, int dirfd,
	const char *pathname, int flags, unsigned int mask,
	struct statx *statxbuf);
void event_uring_file_renameat2(EVENT *ev, FILE_EVENT *fe, int olddirfd,
	const char *oldpath, int newdirfd, const char *newpath, unsigned flags);
void event_uring_mkdirat(EVENT *ev, FILE_EVENT *fe, int dirfd,
	const char *pathname, mode_t mode);

#endif

#endif
