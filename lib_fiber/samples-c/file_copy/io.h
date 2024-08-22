#ifndef	__IO_INCLUDE_H__
#define	__IO_INCLUDE_H__

ssize_t pread_pwrite(int from, int to, off_t off, ssize_t size);
ssize_t sendfile_copy(int from, int to, off_t off, ssize_t size);
ssize_t splice_copy(int pipefd[2], int from, int to, off_t off, ssize_t len);

#endif
