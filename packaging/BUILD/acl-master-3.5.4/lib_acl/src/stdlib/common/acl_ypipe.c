#include "StdAfx.h"

#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_atomic.h"
#include "stdlib/acl_yqueue.h"
#include "stdlib/acl_ypipe.h"

#endif

struct ACL_YPIPE {
	ACL_YQUEUE *queue;
	void **w;
	void **r;
	void **f;
	ACL_ATOMIC *c;
	acl_int64   reads;
	acl_int64   writes;
};

ACL_YPIPE *acl_ypipe_new(void)
{
	ACL_YPIPE *self = (ACL_YPIPE *) acl_mycalloc(1, sizeof(ACL_YPIPE));
	void **item;

	self->queue = acl_yqueue_new();
	acl_yqueue_push(self->queue);
	item    = acl_yqueue_back(self->queue);;
	self->w = item;
	self->f = item;
	self->r = item;
	self->c = acl_atomic_new();
	acl_atomic_set(self->c, item);

	return self;
}
void acl_ypipe_free(ACL_YPIPE *self, void(*free_data_fun)(void*))
{
	acl_yqueue_free(self->queue, free_data_fun);
	acl_atomic_free(self->c);
	acl_myfree(self);
}

void *acl_ypipe_read(ACL_YPIPE *self)
{
	void *value;

	if (!acl_ypipe_check_read(self))
		return NULL;

	value = *acl_yqueue_front(self->queue);
	acl_yqueue_pop(self->queue);
	self->reads++;

	return value;
}

void acl_ypipe_write(ACL_YPIPE *self, void *data)
{
	*acl_yqueue_back(self->queue) = data;
	acl_yqueue_push(self->queue);
	self->f = acl_yqueue_back(self->queue);
	self->writes++;
}

int acl_ypipe_flush(ACL_YPIPE *self)
{
	if (self->w == self->f)
		return 0;

	if (acl_atomic_cas(self->c, self->w, self->f) != self->w) {
		acl_atomic_set(self->c, self->f);
		self->w = self->f;
		return 1;
	}

	self->w = self->f;
	return 0;
}

int acl_ypipe_check_read(ACL_YPIPE *self)
{
	void **front = acl_yqueue_front(self->queue);

	if (front != self->r && self->r && *self->r)
		return 1;

	self->r = (void **) acl_atomic_cas(self->c, front, NULL);

	if (front == self->r || !(self->r))
		return 0;

	return 1;
}
