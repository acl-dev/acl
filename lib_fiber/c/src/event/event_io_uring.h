#ifndef EVENT_IO_URING_INCLUDE_H
#define EVENT_IO_URING_INCLUDE_H

#include "event.h"

#ifdef HAS_EPOLL

EVENT *event_io_uring_create(int size);
void event_uring_file_open(EVENT* ev, FILE_EVENT *fe, const char* pathname,
	int flags, mode_t mode);

#endif

#endif
