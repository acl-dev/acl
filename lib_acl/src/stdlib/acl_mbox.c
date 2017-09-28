#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include <errno.h>
#include <string.h>
#include "stdlib/acl_define.h"
#include "thread/acl_pthread.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_ypipe.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_iostuff.h"
#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_mbox.h"

#endif

struct ACL_MBOX {
	ACL_SOCKET in;
	ACL_SOCKET out;
	size_t nsend;
	size_t nread;
	ACL_YPIPE *ypipe;
	acl_pthread_mutex_t *lock;
};

static const char __key[] = "k";

ACL_MBOX *acl_mbox_create(void)
{
	ACL_MBOX *mbox;
	ACL_SOCKET fds[2];

	if (acl_sane_socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
		acl_msg_error("%s(%d), %s: acl_duplex_pipe error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		return NULL;
	}

	mbox        = (ACL_MBOX *) acl_mymalloc(sizeof(ACL_MBOX));
	mbox->in    = fds[0];
	mbox->out   = fds[1];
	mbox->nsend = 0;
	mbox->nread = 0;
	mbox->ypipe = acl_ypipe_new();
	mbox->lock  = (acl_pthread_mutex_t *)
		acl_mycalloc(1, sizeof(acl_pthread_mutex_t));

	if (acl_pthread_mutex_init(mbox->lock, NULL) != 0)
		acl_msg_fatal("%s(%d), %s: acl_pthread_mutex_init error",
			__FILE__, __LINE__, __FUNCTION__);

	return mbox;
}

void acl_mbox_free(ACL_MBOX *mbox, void (*free_fn)(void*))
{
	acl_socket_close(mbox->in);
	acl_socket_close(mbox->out);
	acl_ypipe_free(mbox->ypipe, free_fn);
	acl_pthread_mutex_destroy(mbox->lock);
	acl_myfree(mbox->lock);
	acl_myfree(mbox);
}

int acl_mbox_send(ACL_MBOX *mbox, void *msg)
{
	int ret;

	acl_pthread_mutex_lock(mbox->lock);
	acl_ypipe_write(mbox->ypipe, msg);
	ret = acl_ypipe_flush(mbox->ypipe);
	acl_pthread_mutex_unlock(mbox->lock);
	if (ret == 0)
		return 0;

	mbox->nsend++;

	ret = acl_socket_write(mbox->out, __key, sizeof(__key), 0, NULL, NULL);
	if (ret == -1)
		return -1;
	else
		return 0;
}

void *acl_mbox_read(ACL_MBOX *mbox, int timeout, int *success)
{
	int   ret;
	char  kbuf[sizeof(__key)];
	void *msg = acl_ypipe_read(mbox->ypipe);

	if (msg != NULL) {
		if (success)
			*success = 1;
		return msg;
	}

	mbox->nread++;

#ifdef ACL_UNIX
	if (timeout >= 0 && acl_read_poll_wait(mbox->in, timeout) < 0)
#else
	if (timeout >= 0 && acl_read_select_wait(mbox->in, timeout) < 0)
#endif
	{
		if (acl_last_error() == ACL_ETIMEDOUT) {
			if (success)
				*success = 1;
		} else if (success)
			*success = 0;
		return NULL;
	}

	ret = acl_socket_read(mbox->in, kbuf, sizeof(kbuf), 0, NULL, NULL);
	if (ret == -1) {
		if (success)
			*success = 0;
		return NULL;
	}

	if (kbuf[0] != __key[0]) {
		acl_msg_error("%s(%d), %s: read invalid, ch=%c, ascii=%d, "
			"key=%c, ret=%d", __FILE__, __LINE__, __FUNCTION__,
			kbuf[0], kbuf[0], __key[0], ret);
		if (success)
			*success = 0;
		return NULL;
	}

	if (success)
		*success = 1;

	return acl_ypipe_read(mbox->ypipe);
}

size_t acl_mbox_nsend(ACL_MBOX *mbox)
{
	return mbox->nsend;
}

size_t acl_mbox_nread(ACL_MBOX *mbox)
{
	return mbox->nread;
}
