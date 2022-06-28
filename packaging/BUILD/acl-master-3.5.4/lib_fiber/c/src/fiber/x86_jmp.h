#ifndef __JMP_DEF_INCLUDE_H__
#define __JMP_DEF_INCLUDE_H__

# if defined(__x86_64__)
#  if defined(__AVX__)
#   define CLOBBER \
	, "ymm0", "ymm1", "ymm2", "ymm3", "ymm4", "ymm5", "ymm6", "ymm7", \
	"ymm8", "ymm9", "ymm10", "ymm11", "ymm12", "ymm13", "ymm14", "ymm15"
#  else
#   define CLOBBER
#  endif

//	asm(".cfi_undefined rip;\r\n")
#  define SETJMP(ctx) ({\
	int ret; \
	asm("lea     LJMPRET%=(%%rip), %%rcx\n\t"\
	"xor     %%rax, %%rax\n\t"\
	"mov     %%rbx, (%%rdx)\n\t"\
	"mov     %%rbp, 8(%%rdx)\n\t"\
	"mov     %%r12, 16(%%rdx)\n\t"\
	"mov     %%rsp, 24(%%rdx)\n\t"\
	"mov     %%r13, 32(%%rdx)\n\t"\
	"mov     %%r14, 40(%%rdx)\n\t"\
	"mov     %%r15, 48(%%rdx)\n\t"\
	"mov     %%rcx, 56(%%rdx)\n\t"\
	"mov     %%rdi, 64(%%rdx)\n\t"\
	"mov     %%rsi, 72(%%rdx)\n\t"\
	"LJMPRET%=:\n\t"\
	: "=a" (ret)\
	: "d" (ctx)\
	: "memory", "rcx", "r8", "r9", "r10", "r11", \
	"xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7", \
	"xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15"\
	CLOBBER\
	); \
	ret; \
})

#  define LONGJMP(ctx) \
	asm("movq   (%%rax), %%rbx\n\t"\
	"movq   8(%%rax), %%rbp\n\t"\
	"movq   16(%%rax), %%r12\n\t"\
	"movq   24(%%rax), %%rdx\n\t"\
	"movq   32(%%rax), %%r13\n\t"\
	"movq   40(%%rax), %%r14\n\t"\
	"mov    %%rdx, %%rsp\n\t"\
	"movq   48(%%rax), %%r15\n\t"\
	"movq   56(%%rax), %%rdx\n\t"\
	"movq   64(%%rax), %%rdi\n\t"\
	"movq   72(%%rax), %%rsi\n\t"\
	"jmp    *%%rdx\n\t"\
	: : "a" (ctx) : "rdx" \
	)

# elif defined(__i386__)

#  define SETJMP(ctx) ({\
	int ret; \
	asm("movl   $LJMPRET%=, %%eax\n\t"\
	"movl   %%eax, (%%edx)\n\t"\
	"movl   %%ebx, 4(%%edx)\n\t"\
	"movl   %%esi, 8(%%edx)\n\t"\
	"movl   %%edi, 12(%%edx)\n\t"\
	"movl   %%ebp, 16(%%edx)\n\t"\
	"movl   %%esp, 20(%%edx)\n\t"\
	"xorl   %%eax, %%eax\n\t"\
	"LJMPRET%=:\n\t"\
	: "=a" (ret) : "d" (ctx) : "memory"); \
	ret; \
	})

#  define LONGJMP(ctx) \
	asm("movl   (%%eax), %%edx\n\t"\
	"movl   4(%%eax), %%ebx\n\t"\
	"movl   8(%%eax), %%esi\n\t"\
	"movl   12(%%eax), %%edi\n\t"\
	"movl   16(%%eax), %%ebp\n\t"\
	"movl   20(%%eax), %%esp\n\t"\
	"jmp    *%%edx\n\t"\
	: : "a" (ctx) : "edx" \
	)

# endif
#endif
