#ifndef __JMP_INCLUDE_H__
#define __JMP_INCLUDE_H__

#ifdef __linux__

# if defined(__i386__)
#  define	JMP_BUF_CNT	6
# elif defined(__x86_64__)
#  define	JMP_BUF_CNT	8
# elif defined(__sparc__) && defined(__arch64__)
#  define	JMP_BUF_CNT	6
# elif defined(__powerpc__)
#  define	JMP_BUF_CNT	26
# elif defined(__aarch64__)
#  define	JMP_BUF_CNT	64
# elif defined(__arm__)
#  define	JMP_BUF_CNT	65
# elif defined(__mips__)
#  define JMP_BUF_CNT		12
# elif defined(__s390x__)
#  define JMP_BUF_CNT		18
# elif defined(__riscv)
#  define JMP_BUF_CNT     	64
# else
#  define	JMP_BUF_CNT	1
# endif

typedef struct _label_t { long long unsigned val[JMP_BUF_CNT]; } label_t;
extern int SETJMP(label_t *) __attribute__ ((__nothrow__));
extern void LONGJMP(label_t *) __attribute__((__noreturn__));

#endif  // __linux__

#endif
