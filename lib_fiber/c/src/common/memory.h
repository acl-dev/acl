#ifndef __MEMORY_HEAD_H__
#define __MEMORY_HEAD_H__

void *stack_alloc(size_t size);
void *stack_calloc(size_t size);
void stack_free(void *ptr);

void *mem_malloc(size_t size);
void mem_free(void *ptr);
void *mem_calloc(size_t nmemb, size_t size);
void *mem_realloc(void *ptr, size_t size);
void mem_stat(void);
char *mem_strdup(const char *s);

# if defined(__linux__) && defined(USE_INLINE_MEMCPY)
#  include <stddef.h>

// The below comes from Clickhouse.
#  ifdef __SSE2__
#   include <emmintrin.h>
#  endif

#  define HAS_INLINE_MEMCPY

static inline void * inline_memcpy(void * __restrict dst_, const void * __restrict src_, size_t size)
{
    /// We will use pointer arithmetic, so char pointer will be used.
    /// Note that __restrict makes sense (otherwise compiler will reload data from memory
    /// instead of using the value of registers due to possible aliasing).
    char * __restrict dst = (char * __restrict)(dst_);
    const char * __restrict src = (const char * __restrict)(src_);

    /// Standard memcpy returns the original value of dst. It is rarely used but we have to do it.
    /// If you use memcpy with small but non-constant sizes, you can call inline_memcpy directly
    /// for inlining and removing this single instruction.
    void * ret = dst;

tail:
    /// Small sizes and tails after the loop for large sizes.
    /// The order of branches is important but in fact the optimal order depends on the distribution of sizes in your application.
    /// This order of branches is from the disassembly of glibc's code.
    /// We copy chunks of possibly uneven size with two overlapping movs.
    /// Example: to copy 5 bytes [0, 1, 2, 3, 4] we will copy tail [1, 2, 3, 4] first and then head [0, 1, 2, 3].
    if (size <= 16)
    {
        if (size >= 8)
        {
            /// Chunks of 8..16 bytes.
            __builtin_memcpy(dst + size - 8, src + size - 8, 8);
            __builtin_memcpy(dst, src, 8);
        }
        else if (size >= 4)
        {
            /// Chunks of 4..7 bytes.
            __builtin_memcpy(dst + size - 4, src + size - 4, 4);
            __builtin_memcpy(dst, src, 4);
        }
        else if (size >= 2)
        {
            /// Chunks of 2..3 bytes.
            __builtin_memcpy(dst + size - 2, src + size - 2, 2);
            __builtin_memcpy(dst, src, 2);
        }
        else if (size >= 1)
        {
            /// A single byte.
            *dst = *src;
        }
        /// No bytes remaining.
    }
    else
    {
        /// Medium and large sizes.
        if (size <= 128)
        {
            /// Medium size, not enough for full loop unrolling.

            /// We will copy the last 16 bytes.
            _mm_storeu_si128((__m128i *)(dst + size - 16), _mm_loadu_si128((const __m128i *)(src + size - 16)));

            /// Then we will copy every 16 bytes from the beginning in a loop.
            /// The last loop iteration will possibly overwrite some part of already copied last 16 bytes.
            /// This is Ok, similar to the code for small sizes above.
            while (size > 16)
            {
                _mm_storeu_si128((__m128i *)(dst), _mm_loadu_si128((const __m128i *)(src)));
                dst += 16;
                src += 16;
                size -= 16;
            }
        }
        else
        {
            /// Large size with fully unrolled loop.

            /// Align destination to 16 bytes boundary.
            size_t padding = (16 - ((size_t)(dst) & 15)) & 15;

            /// If not aligned - we will copy first 16 bytes with unaligned stores.
            if (padding > 0)
            {
                __m128i head = _mm_loadu_si128((const __m128i*)(src));
                _mm_storeu_si128((__m128i*)(dst), head);
                dst += padding;
                src += padding;
                size -= padding;
            }

            /// Aligned unrolled copy. We will use half of available SSE registers.
            /// It's not possible to have both src and dst aligned.
            /// So, we will use aligned stores and unaligned loads.
            __m128i c0, c1, c2, c3, c4, c5, c6, c7;

            while (size >= 128)
            {
                c0 = _mm_loadu_si128((const __m128i*)(src) + 0);
                c1 = _mm_loadu_si128((const __m128i*)(src) + 1);
                c2 = _mm_loadu_si128((const __m128i*)(src) + 2);
                c3 = _mm_loadu_si128((const __m128i*)(src) + 3);
                c4 = _mm_loadu_si128((const __m128i*)(src) + 4);
                c5 = _mm_loadu_si128((const __m128i*)(src) + 5);
                c6 = _mm_loadu_si128((const __m128i*)(src) + 6);
                c7 = _mm_loadu_si128((const __m128i*)(src) + 7);
                src += 128;
                _mm_store_si128(((__m128i*)(dst) + 0), c0);
                _mm_store_si128(((__m128i*)(dst) + 1), c1);
                _mm_store_si128(((__m128i*)(dst) + 2), c2);
                _mm_store_si128(((__m128i*)(dst) + 3), c3);
                _mm_store_si128(((__m128i*)(dst) + 4), c4);
                _mm_store_si128(((__m128i*)(dst) + 5), c5);
                _mm_store_si128(((__m128i*)(dst) + 6), c6);
                _mm_store_si128(((__m128i*)(dst) + 7), c7);
                dst += 128;

                size -= 128;
            }

            /// The latest remaining 0..127 bytes will be processed as usual.
            goto tail;
        }
    }

    return ret;
}

#  endif  // __linux__

#endif
