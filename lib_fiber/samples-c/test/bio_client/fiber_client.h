/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   Dongcheng District, Beijing, RPC 100006
 *   E-mail: shuxin.zheng@net263.com
 * 
 * VERSION
 *   Wed 11 Jan 2017 02:02:19 PM CST
 */

#ifndef __FIBER_CLIENT_INCLUDE_H__
#define __FIBER_CLIENT_INCLUDE_H__

typedef struct DATA {
	ACL_RING entry;
	void  *dat;
	size_t len;
	int    eof;
} DATA;

typedef struct FIBER_CTX {
	int fd;
	ACL_RING link;
	ACL_FIBER_SEM *sem_data;
	ACL_FIBER_SEM *sem_exit;
	ACL_FIBER *reader;
	ACL_FIBER *writer;
} FIBER_CTX;

extern int  __nconnect;
extern int  __count;
extern char __listen_ip[64];
extern int  __listen_port;
extern int  __listen_qlen;
extern int  __rw_timeout;
extern int  __echo_data;
extern int  __stack_size;
extern int __max_length;

void fiber_reader(ACL_FIBER *fiber, void *ctx);
void fiber_writer(ACL_FIBER *fiber, void *ctx);
void echo_client(int fd);
int  check_read(int fd, int timeout);
int  check_write(int fd, int timeout);

#endif
