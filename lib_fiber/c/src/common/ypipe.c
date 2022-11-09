#include "stdafx.h"

#include "atomic.h"
#include "yqueue.h"
#include "ypipe.h"

struct YPIPE {
	YQUEUE *queue;
	void **w;
	void **r;
	void **f;
	ATOMIC *c;
	unsigned long reads;
	unsigned long writes;
};

YPIPE *ypipe_new(void)
{
	YPIPE *self = (YPIPE *) calloc(1, sizeof(YPIPE));
	void **item;

	self->queue = yqueue_new();
	yqueue_push(self->queue);
	item    = yqueue_back(self->queue);;
	self->w = item;
	self->f = item;
	self->r = item;
	self->c = atomic_new();
	atomic_set(self->c, item);

	return self;
}
void ypipe_free(YPIPE *self, void(*free_data_fun)(void*))
{
	yqueue_free(self->queue, free_data_fun);
	atomic_free(self->c);
	free(self);
}

void *ypipe_read(YPIPE *self)
{
	void *value;

	if (!ypipe_check_read(self)) {
		return NULL;
	}

	value = *yqueue_front(self->queue);
	yqueue_pop(self->queue);
	self->reads++;

	return value;
}

void ypipe_write(YPIPE *self, void *data)
{
	*yqueue_back(self->queue) = data;
	yqueue_push(self->queue);
	self->f = yqueue_back(self->queue);
	self->writes++;
}

int ypipe_flush(YPIPE *self)
{
	if (self->w == self->f) {
		return 0;
	}

	if (atomic_cas(self->c, self->w, self->f) != self->w) {
		atomic_set(self->c, self->f);
		self->w = self->f;
		return 1;
	}

	self->w = self->f;
	return 0;
}

int ypipe_check_read(YPIPE *self)
{
	void **front = yqueue_front(self->queue);

	if (front != self->r && self->r && *self->r) {
		return 1;
	}

	self->r = (void **) atomic_cas(self->c, front, NULL);

	if (front == self->r || !(self->r)) {
		return 0;
	}

	return 1;
}
