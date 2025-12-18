#ifndef ACL_CHUNK_CHAIN_INCLUDE_H
#define ACL_CHUNK_CHAIN_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"

/**
 * Chunk chain type definition.
 */
typedef struct ACL_CHAIN ACL_CHAIN;

/**
 * Create a chunk chain object.
 * @param init_size {size_t} Initial size of chunk data dynamic memory
 * @param off_begin {acl_int64} Chunk data block start position
 * @return {ACL_CHAIN*} Chunk chain object
 */
ACL_API ACL_CHAIN *acl_chain_new(size_t init_size, acl_int64 off_begin);

/**
 * Free chunk chain object.
 * @param chain {ACL_CHAIN*} Chunk chain object
 */
ACL_API void acl_chain_free(ACL_CHAIN *chain);

/**
 * Set chunk chain data block's next offset position.
 * @param chain {ACL_CHAIN*} Chunk chain object
 * @param from_next {acl_int64} Chunk data block's next offset position
 */
ACL_API void acl_chain_set_from_next(ACL_CHAIN *chain, acl_int64 from_next);

/**
 * Reset chunk chain, set start position to specified value.
 * @param chain {ACL_CHAIN*} Chunk chain object
 * @param off_begin {acl_int64} Chunk chain data block start position
 */
ACL_API void acl_chain_reset(ACL_CHAIN *chain, acl_int64 off_begin);

/**
 * Get current chunk chain's data block's next position.
 * @param chain {ACL_CHAIN*} Chunk chain object
 * @return {acl_int64} Chunk data block's next position
 */
ACL_API acl_int64 acl_chain_from_next(ACL_CHAIN *chain);

/**
 * Get current chunk chain's start position.
 * @param chain {ACL_CHAIN*} Chunk chain object
 * @return {acl_int64} Chunk chain start position
 */
ACL_API acl_int64 acl_chain_off_begin(ACL_CHAIN *chain);

/**
 * Get current chunk chain's data block's start storage pointer address.
 * @param chain {ACL_CHAIN*} Chunk chain object
 * @return {const char*} Chunk data block's start storage pointer address
 */
ACL_API const char *acl_chain_data(ACL_CHAIN *chain);

/**
 * Get current chunk chain's data block's data length.
 * @param chain {ACL_CHAIN*} Chunk chain object
 * @return {int} Chunk data block's data length
 */
ACL_API int acl_chain_data_len(ACL_CHAIN *chain);

/**
 * Get number of chunks in current chunk chain.
 * @param chain {ACL_CHAIN*} Chunk chain object
 * @return {int} Number of chunks
 */
ACL_API int acl_chain_size(ACL_CHAIN *chain);

/**
 * Get total length of all chunks in current chunk chain.
 * @param chain {ACL_CHAIN*} Chunk chain object
 * @return {int} Total length of chunks
 */
ACL_API int acl_chain_chunk_data_len(ACL_CHAIN *chain);

/**
 * Add a data chunk to chunk chain, internally automatically removes duplicates.
 * @param chain {ACL_CHAIN*} Chunk chain object
 * @param data {const void*} Data chunk pointer
 * @param from {acl_int64} Chunk data block start position
 * @param dlen {int} Data chunk length
 */
ACL_API void acl_chain_add(ACL_CHAIN *chain, const void *data,
	acl_int64 from, int dlen);

/**
 * Print information about all chunks in current chunk chain and chunk data block start positions.
 * @param chain {ACL_CHAIN*} Chunk chain object
 */
ACL_API void acl_chain_list(ACL_CHAIN *chain);

#ifdef __cplusplus
}
#endif

#endif
