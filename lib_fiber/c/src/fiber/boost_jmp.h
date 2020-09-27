#ifndef __BOOST_JUMP_INCLUDE_H__
#define __BOOST_JUMP_INCLUDE_H__

typedef void* fcontext_t;

typedef struct {
	fcontext_t fctx;
	void *data;
} transfer_t;

extern transfer_t jump_fcontext(fcontext_t const to, void *vp);
extern fcontext_t make_fcontext(void *sp, size_t size, void (*fn)(transfer_t));

#endif
