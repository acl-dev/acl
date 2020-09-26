#ifndef __BOOST_JUMP_INCLUDE_H__
#define __BOOST_JUMP_INCLUDE_H__

typedef void* fcontext_t;

typedef struct {
	fcontext_t fctx;
	void *data;
} transfer_t;

extern transfer_t jump_fcontext(fcontext_t const to, void *vp);
extern fcontext_t make_fcontext(void *sp, size_t size, void (*fn)(transfer_t));

#if 0
typedef struct {
	fcontext_t *from;
	fcontext_t *to;
} s_jump_t;

static void swap_fcontext(fcontext_t *old_ctx, fcontext_t *new_ctx) {
	s_jump_t jump;
	transfer_t trans;
	s_jump_t *jmp;

	jump.from  = old_ctx;
	jump.to    = new_ctx;
	trans      = jump_fcontext(new_ctx, (void*)&jump);
	jmp        = (s_jump_t*) trans.data;
	*jmp->from = trans.fctx;
}
#endif

#endif
