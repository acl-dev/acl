#ifndef EVENT_IO_URING_INCLUDE_H
#define EVENT_IO_URING_INCLUDE_H

#include "event.h"

#ifdef HAS_IO_URING

EVENT *event_io_uring_create(int size);

void event_uring_file_close(EVENT *ev, FILE_EVENT *fe);
void event_uring_file_cancel(EVENT *ev, FILE_EVENT *fe_orig, FILE_EVENT *fe);
void event_uring_file_openat(EVENT* ev, FILE_EVENT *fe, int dirfd,
	const char* pathname, int flags, mode_t mode);
void event_uring_file_unlink(EVENT *ev, FILE_EVENT *fe, const char *pathname);

# ifdef HAS_STATX
void event_uring_file_statx(EVENT *ev, FILE_EVENT *fe, int dirfd,
	const char *pathname, int flags, unsigned int mask,
	struct statx *statxbuf);
# endif

# ifdef HAS_RENAMEAT2
void event_uring_file_renameat2(EVENT *ev, FILE_EVENT *fe, int olddirfd,
	const char *oldpath, int newdirfd, const char *newpath, unsigned flags);
# endif

void event_uring_mkdirat(EVENT *ev, FILE_EVENT *fe, int dirfd,
	const char *pathname, mode_t mode);
void event_uring_splice(EVENT *ev, FILE_EVENT *fe, int fd_in, loff_t off_in,
	int fd_out, loff_t off_out, size_t len, unsigned int splice_flags,
	unsigned int sqe_flags, __u8 opcode);

#endif

#endif
