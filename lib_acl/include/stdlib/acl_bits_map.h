#ifndef ACL_BITS_MAP_INCLUDE_H
#define ACL_BITS_MAP_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"

/**
 * 位映射结构类型定义
 */
typedef struct ACL_BITS_MASK {
	char   *data;		/**< bit mask */
	size_t  data_len;	/**< data byte count */
} ACL_BITS_MASK;

/**
 * Bits per byte, byte in vector, bit offset in byte, bytes perset
 */
#define	ACL_BITS_MASK_NBBY	(8)
#define	ACL_BITS_MASK_FD_BYTE(number, mask) \
	(((unsigned char *) (mask)->data)[(number) / ACL_BITS_MASK_NBBY])
#define	ACL_BITS_MASK_FD_BIT(number)	(1 << ((number) % ACL_BITS_MASK_NBBY))
#define	ACL_BITS_MASK_BYTES_NEEDED(len) \
	(size_t) (((acl_int64) (len) + (ACL_BITS_MASK_NBBY - 1)) / ACL_BITS_MASK_NBBY)
#define	ACL_BITS_MASK_BYTE_COUNT(mask)	((mask)->data_len)

/* Memory management. */
/**
 * 分配位映射对象空间
 * @param mask {ACL_BITS_MASK*) ACL_BITS_MASK 指针
 * @param nmax {size_t/unsigned int/unsigned short/unsigned char} 最大值，以此值来
 *  计算 (mask)->data 占的内存空间大小，如：当 nmax=4294967295, 即最大整数值时，则
 *  (mask)->data_len=536870912, 即 (mask)->data 占用 536870912 Bytes; 当 nmax=65535,
 *  即最大 unsigned short 值时，则 (mask)->data_len=8192, 即 (mask)->data 占用 8192 字节
 */
#define	ACL_BITS_MASK_ALLOC(mask, nmax) do { \
	size_t _byte_len = ACL_BITS_MASK_BYTES_NEEDED(nmax); \
	(mask)->data = (char*) acl_mymalloc(_byte_len); \
	memset((mask)->data, 0, _byte_len); \
	(mask)->data_len = _byte_len; \
} while (0)

/**
 * 重分配位映射对象空间
 * @param mask {ACL_BITS_MASK*) ACL_BITS_MASK 指针
 * @param nmax {size_t/unsigned int/unsigned short/unsigned char} 最大值，以此值来
 *  计算 (mask)->data 占的内存空间大小，如：当 nmax=4294967295, 即最大整数值时，则
 *  (mask)->data_len=536870912, 即 (mask)->data 占用 536870912 Bytes; 当 nmax=65535,
 *  即最大 unsigned short 值时，则 (mask)->data_len=8192, 即 (mask)->data 占用 8192 字节
 */
#define	ACL_BITS_MASK_REALLOC(mask, nmax) do { \
	size_t _byte_len = ACL_BITS_MASK_BYTES_NEEDED(nmax); \
	size_t _old_len = (mask)->data_len; \
	(mask)->data = (char*) acl_myrealloc((mask)->data, _byte_len); \
	memset((mask)->data + _old_len, 0, _byte_len - _old_len); \
	(mask)->data_len = _byte_len; \
} while (0)

/**
 * 释放位映射对象的内部动态空间
 * @param mask {ACL_BITS_MASK*) ACL_BITS_MASK 指针
 */
#define	ACL_BITS_MASK_FREE(mask)	acl_myfree((mask)->data)

/* Set operations, modeled after FD_ZERO/SET/ISSET/CLR. */

/**
 * 将位映射对象的内部动态空间清零
 * @param mask {ACL_BITS_MASK*) ACL_BITS_MASK 指针
 */
#define	ACL_BITS_MASK_ZERO(mask) \
	memset((mask)->data, 0, (mask)->data_len);

/**
 * 将整数映射为位存储在位映射对象的动态空间中
 * @param number {unsigned int} 整数值
 * @param mask {ACL_BITS_MASK*) ACL_BITS_MASK 指针
 */
#define	ACL_BITS_MASK_SET(number, mask) \
	(ACL_BITS_MASK_FD_BYTE((number), (mask)) |= ACL_BITS_MASK_FD_BIT(number))

/**
 * 判断某个整数是否存储在位映射对象的动态空间中
 * @param number {unsigned int} 整数值
 * @param mask {ACL_BITS_MASK*) ACL_BITS_MASK 指针
 * @return {int} 0: 不存在；!= 0: 存在
 */
#define	ACL_BITS_MASK_ISSET(number, mask) \
	(ACL_BITS_MASK_FD_BYTE((number), (mask)) & ACL_BITS_MASK_FD_BIT(number))

/**
 * 将某个整数从位映射对象的动态空间中清除掉
 * @param number {unsigned int} 整数值
 * @param mask {ACL_BITS_MASK*) ACL_BITS_MASK 指针
 */
#define	ACL_BITS_MASK_CLR(number, mask) \
	(ACL_BITS_MASK_FD_BYTE((number), (mask)) &= ~ACL_BITS_MASK_FD_BIT(number))

#ifdef __cplusplus
}
#endif

#endif
