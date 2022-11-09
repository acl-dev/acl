#include "stdafx.h"

#include "atomic.h"
#include "yqueue.h"

#define  CHUNK_SIZE 10

typedef struct chunk_t {
	void  *value[CHUNK_SIZE];
	struct chunk_t *prev;
	struct chunk_t *next;
} chunk_t;

struct YQUEUE {
	chunk_t   *begin_chunk;
	int        begin_pos;
	chunk_t   *back_chunk;
	int        back_pos;
	chunk_t   *end_chunk;
	int        end_pos;
	unsigned long long pushs;
	unsigned long long pops;
	ATOMIC *spare_chunk;
};

YQUEUE *yqueue_new(void)
{
	YQUEUE *self;

	self = (YQUEUE *) malloc(sizeof(YQUEUE));
	memset(self, 0, sizeof(YQUEUE));
	self->begin_chunk = (chunk_t *) malloc(sizeof(chunk_t));
	memset(self->begin_chunk, 0, sizeof(chunk_t));
	self->begin_pos   = 0;
	self->back_chunk  = NULL;
	self->back_pos    = 0;
	self->end_chunk   = self->begin_chunk;
	self->end_pos     = 0;
	self->spare_chunk = atomic_new();
	atomic_set(self->spare_chunk, NULL);

	return self;
}
static void chunk_data_free(chunk_t *chunk, int begin, int end,
	void(*free_fn)(void*))
{
	int i;
	assert(chunk);

	for (i = begin; i <= end; i++) {
		if(chunk->value[i]) {
			free_fn(chunk->value[i]);
		}
	}
}
void yqueue_free(YQUEUE *self, void(*free_fn)(void*))
{
	chunk_t *cs;

	if (free_fn) {
		if (self->begin_chunk == self->back_chunk) {
			chunk_data_free(self->begin_chunk, self->begin_pos,
				self->back_pos, free_fn);
		} else {
			chunk_t *begin = self->begin_chunk;
			chunk_t *end = self->back_chunk;
			int begin_pos = self->begin_pos;
			for (; begin != end; begin = begin->next) {
				chunk_data_free(begin, begin_pos,
					CHUNK_SIZE - 1, free_fn);
				begin_pos = 0;
			}
			chunk_data_free(end, 0, self->back_pos, free_fn);
			begin_pos = 0;
		}
	}

	do {
		chunk_t *o;

		if (self->begin_chunk == self->end_chunk) {
			free(self->begin_chunk);
			break;
		}

		o = self->begin_chunk;
		self->begin_chunk = self->begin_chunk->next;
		free(o);

	} while (1);

	cs = (chunk_t *) atomic_xchg(self->spare_chunk, NULL);
	if (cs) {
		free(cs);
	}

	atomic_free(self->spare_chunk);
	free(self);
}

void **yqueue_front(YQUEUE *self)
{
	return &self->begin_chunk->value[self->begin_pos];
}

void **yqueue_back(YQUEUE *self)
{
	return &self->back_chunk->value[self->back_pos];
}

void yqueue_push(YQUEUE *self)
{
	chunk_t *sc;

	self->pushs++;
	self->back_chunk = self->end_chunk;
	self->back_pos   = self->end_pos;

	self->end_pos++;
	if (self->end_pos != CHUNK_SIZE) {
		return;
	}

	sc = (chunk_t *) atomic_xchg(self->spare_chunk, NULL);

	if (sc) {
		self->end_chunk->next = sc;
		sc->prev = self->end_chunk;
	} else {
		self->end_chunk->next = (chunk_t *) malloc(sizeof(chunk_t));
		memset(self->end_chunk->next, 0, sizeof(chunk_t));
		assert(self->end_chunk);
		self->end_chunk->next->prev = self->end_chunk;
	}

	self->end_chunk = self->end_chunk->next;
	self->end_pos = 0;
}

void yqueue_pop(YQUEUE *self)
{
	self->begin_pos++;
	self->pops++;

	if (self->begin_pos == CHUNK_SIZE) {
		chunk_t *cs;
		chunk_t *o = self->begin_chunk;

		self->begin_chunk = self->begin_chunk->next;
		self->begin_chunk->prev = NULL;
		self->begin_pos = 0;
		memset(o, 0, sizeof(chunk_t));
		cs = (chunk_t *) atomic_xchg(self->spare_chunk, o);
		if (cs) {
			free(cs);
		}
	}
}
