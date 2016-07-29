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
	ACL_VSTREAM *in;
	ACL_VSTREAM *out;
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
	mbox->in    = acl_vstream_fdopen(fds[0], O_RDONLY, sizeof(__key),
			0, ACL_VSTREAM_TYPE_SOCK);
	mbox->out   = acl_vstream_fdopen(fds[1], O_WRONLY, sizeof(__key),
			0, ACL_VSTREAM_TYPE_SOCK);
	mbox->nsend = 0;
	mbox->nread = 0;
	mbox->ypipe = acl_ypipe_new();
	mbox->lock  = acl_pthread_mutex_create();

	return mbox;
}

void acl_mbox_free(ACL_MBOX *mbox, void (*free_fn)(void*))
{
	acl_vstream_close(mbox->in);
	acl_vstream_close(mbox->out);
	acl_ypipe_free(mbox->ypipe, free_fn);
	acl_pthread_mutex_destroy(mbox->lock);
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

	if (acl_vstream_writen(mbox->out, __key, sizeof(__key) - 1)
		== ACL_VSTREAM_EOF)
	{
		return -1;
	} else
		return 0;

}

void *acl_mbox_read(ACL_MBOX *mbox, int timeout, int *success)
{
	int   ret;
	char  kbuf[sizeof(__key)];
	void *msg = acl_ypipe_read(mbox->ypipe);

	if (msg != NULL) {
		if (success)
			*success = 0;
		return msg;
	}

	mbox->nread++;
	mbox->in->rw_timeout = timeout;

	ret = acl_vstream_readn(mbox->in, kbuf, sizeof(kbuf) - 1);
	if (ret == ACL_VSTREAM_EOF) {
		if (mbox->in->errnum == ACL_ETIMEDOUT) {
			if (success)
				*success = 0;
			return NULL;
		}

		if (success)
			*success = -1;
		return NULL;
	}

	if (kbuf[0] != __key[0]) {
		acl_msg_error("%s(%d), %s: read invalid: %c",
			__FILE__, __LINE__, __FUNCTION__, kbuf[0]);
		if (success)
			*success = -1;
		return NULL;
	}

	if (success)
		*success = 0;
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
