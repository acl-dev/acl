#ifndef ACL_BITS_MAP_INCLUDE_H
#define ACL_BITS_MAP_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"

/**
 * Bit map structure type definition.
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
 * Allocate bit map memory space.
 * @param mask {ACL_BITS_MASK*) ACL_BITS_MASK pointer
 * @param nmax {size_t/unsigned int/unsigned short/unsigned
 *  char} Maximum value, can be maximum value. Then
 *  (mask)->data occupies memory space size. E.g., if
 *  nmax=4294967295, representing maximum unsigned int
 *  value, then (mask)->data_len=536870912, and (mask)->data
 *  occupies 536870912 Bytes; if nmax=65535, representing
 *  unsigned short value, then (mask)->data_len=8192, and
 *  (mask)->data occupies 8192 bytes
 */
#define	ACL_BITS_MASK_ALLOC(mask, nmax) do { \
	size_t _byte_len = ACL_BITS_MASK_BYTES_NEEDED(nmax); \
	(mask)->data = (char*) acl_mymalloc(_byte_len); \
	memset((mask)->data, 0, _byte_len); \
	(mask)->data_len = _byte_len; \
} while (0)

/**
 * Reallocate bit map memory space.
 * @param mask {ACL_BITS_MASK*) ACL_BITS_MASK pointer
 * @param nmax {size_t/unsigned int/unsigned short/unsigned
 *  char} Maximum value, can be maximum value. Then
 *  (mask)->data occupies memory space size. E.g., if
 *  nmax=4294967295, representing maximum unsigned int
 *  value, then (mask)->data_len=536870912, and (mask)->data
 *  occupies 536870912 Bytes; if nmax=65535, representing
 *  unsigned short value, then (mask)->data_len=8192, and
 *  (mask)->data occupies 8192 bytes
 */
#define	ACL_BITS_MASK_REALLOC(mask, nmax) do { \
	size_t _byte_len = ACL_BITS_MASK_BYTES_NEEDED(nmax); \
	size_t _old_len = (mask)->data_len; \
	(mask)->data = (char*) acl_myrealloc((mask)->data, _byte_len); \
	memset((mask)->data + _old_len, 0, _byte_len - _old_len); \
	(mask)->data_len = _byte_len; \
} while (0)

/**
 * Free bit map's internal dynamic space.
 * @param mask {ACL_BITS_MASK*) ACL_BITS_MASK pointer
 */
#define	ACL_BITS_MASK_FREE(mask)	acl_myfree((mask)->data)

/* Set operations, modeled after FD_ZERO/SET/ISSET/CLR. */

/**
 * Zero bit map's internal dynamic space.
 * @param mask {ACL_BITS_MASK*) ACL_BITS_MASK pointer
 */
#define	ACL_BITS_MASK_ZERO(mask) \
	memset((mask)->data, 0, (mask)->data_len);

/**
 * Set a number stored in bit map's dynamic space.
 * @param number {unsigned int} Number value
 * @param mask {ACL_BITS_MASK*) ACL_BITS_MASK pointer
 */
#define	ACL_BITS_MASK_SET(number, mask) \
	(ACL_BITS_MASK_FD_BYTE((number), (mask)) |= ACL_BITS_MASK_FD_BIT(number))

/**
 * Check whether a certain number is stored in bit map's dynamic space.
 * @param number {unsigned int} Number value
 * @param mask {ACL_BITS_MASK*) ACL_BITS_MASK pointer
 * @return {int} 0: does not exist; != 0: exists
 */
#define	ACL_BITS_MASK_ISSET(number, mask) \
	(ACL_BITS_MASK_FD_BYTE((number), (mask)) & ACL_BITS_MASK_FD_BIT(number))

/**
 * Clear a certain number from bit map's dynamic space.
 * @param number {unsigned int} Number value
 * @param mask {ACL_BITS_MASK*) ACL_BITS_MASK pointer
 */
#define	ACL_BITS_MASK_CLR(number, mask) \
	(ACL_BITS_MASK_FD_BYTE((number), (mask)) &= ~ACL_BITS_MASK_FD_BIT(number))

#ifdef __cplusplus
}
#endif

#endif
