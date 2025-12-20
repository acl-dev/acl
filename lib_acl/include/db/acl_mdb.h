#ifndef	ACL_MEMDB_INCLUDE_H
#define	ACL_MEMDB_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"

typedef struct ACL_MDT_NOD ACL_MDT_NOD;
typedef struct ACL_MDT_RES ACL_MDT_RES;
typedef struct ACL_MDT_REC ACL_MDT_REC;
typedef struct ACL_MDT_REF ACL_MDT_REF;
typedef struct ACL_MDT_IDX ACL_MDT_IDX;
typedef struct ACL_MDT ACL_MDT;
typedef struct ACL_MDB ACL_MDB;

/* constraint flag bits for data table key fields */
#define ACL_MDT_FLAG_NUL		(0)
#define ACL_MDT_FLAG_UNI		(1 << 0)	/**< indicates unique */
#define	ACL_MDT_FLAG_KMR		(1 << 1)	/**< indicates cache key in memory */
#define ACL_MDT_FLAG_DMR		(1 << 2)	/**< indicates cache value in memory */
/**< when duplicate keys are allowed, if ACL_MDT_FLAG_UNI |
 * ACL_MDT_FLAG_OOV is specified at the same time, use value
 * instead of key value */
#define ACL_MDT_FLAG_OOV		(1 << 3)

#define	ACL_MDT_FLAG_SLICE1		(1 << 10)	/**< same as ACL_SLICE_FLAG_GC1 */
#define	ACL_MDT_FLAG_SLICE2		(1 << 11)	/**< same as ACL_SLICE_FLAG_GC2 */
#define	ACL_MDT_FLAG_SLICE3		(1 << 12)	/**< same as ACL_SLICE_FLAG_GC3 */
#define	ACL_MDT_FLAG_SLICE_RTGC_OFF	(1 << 13)	/**< disable
							 * real-time garbage
							 * collection for memory
							 * slices */

/************************************************************************/
/*                          in acl_mdb.c                                */
/************************************************************************/

/**
 * Create a memory database object.
 * @param dbname {const char*} database name
 * @param dbtype {const char*} database type: hash/avl
 * @return {ACL_MDB*} database object
 */
ACL_API ACL_MDB *acl_mdb_create(const char *dbname, const char *dbtype);

/**
 * Close and free a memory database.
 * @param mdb {ACL_MDB*} database object
 */
ACL_API void acl_mdb_free(ACL_MDB *mdb);

/**
 * Create a data table on the specified database.
 * @param mdb {ACL_MDB*} database object
 * @param tbl_name {const char*} table name
 * @param tbl_flag {unsigned int} table attribute flag bits
 * @param init_capacity {size_t} initial capacity of each
 *  internal hash table when creating the data table
 * @param key_labels {const char *[]} data table field name array
 * @param flags {unsigned int[]} constraint flag bits
 *  corresponding to key_labels
 * @return {ACL_MDT*} newly created data table object
 */
ACL_API ACL_MDT *acl_mdb_tbl_create(ACL_MDB *mdb, const char *tbl_name,
	unsigned int tbl_flag, size_t init_capacity,
	const char *key_labels[], unsigned int flags[]);

/**
 * Add a new data record.
 * @param mdb {ACL_MDB*} database object
 * @param tbl_name {const char*} data table name
 * @param data {void*} user data pointer
 * @param dlen {unsigned int} data buffer size
 * @param key_labels {const char *[]} data table field name array
 * @param keys {const char *[]} data table field value array
 * @return {ACL_MDB_NOD*} node pointer for the added data
 *  stored in the data table
 */
ACL_API ACL_MDT_NOD *acl_mdb_add(ACL_MDB *mdb, const char *tbl_name,
	void *data, unsigned int dlen,
	const char *key_labels[], const char *keys[]);

/**
 * Probe whether the corresponding field value exists in the data table.
 * @param mdb {ACL_MDB*} database object
 * @param tbl_name {const char*} data table name
 * @param key_label {const char*} data table field name
 * @param key {const char*} data table field key value
 * @return {int} 0: not found, != 0: found
 */
ACL_API int acl_mdb_probe(ACL_MDB *mdb, const char *tbl_name,
	const char *key_label, const char *key);

/**
 * Query results matching conditions in the database.
 * @param mdb {ACL_MDB*} database object
 * @param tbl_name {const char*} data table name
 * @param key_label {const char*} field name in the data table
 * @param key {const char*} field value in the data table
 * @param from {int} query result offset, which is the
 *  starting position for storage
 * @param limit {int} query result limit, maximum number
 * @return {ACL_MDT_RES*} query result set, if the query
 *  result is empty, returns NULL
 */
ACL_API ACL_MDT_RES *acl_mdb_find(ACL_MDB *mdb, const char *tbl_name,
	const char *key_label, const char *key, int from, int limit);

/**
 * List results of a certain range in a certain data table in the database.
 * @param mdb {ACL_MDB*} database object
 * @param tbl_name {const char*} data table name
 * @param from {int} query result offset, which is the
 *  starting position for storage
 * @param limit {int} query result limit, maximum number
 * @return {ACL_MDT_RES*} query result set, if the query
 *  result is empty, returns NULL
 */
ACL_API ACL_MDT_RES *acl_mdb_list(ACL_MDB *mdb, const char *tbl_name,
	int from, int limit);

/**
 * Delete a data record in the database.
 * @param mdb {ACL_MDB*} database object
 * @param tbl_name {const char*} data table name
 * @param key_label {const char*} data table field name
 * @param key {const char*} data table field value
 * @param onfree_fn {void (*)(void*, unsigned int)}
  *	callback function called when freeing user data
 * @return {int} number of deleted records
 */
ACL_API int acl_mdb_del(ACL_MDB *mdb, const char *tbl_name,
	const char *key_label, const char *key,
	void (*onfree_fn)(void*, unsigned int));

/**
 * Traverse data nodes in a certain data table in the database.
 * @param mdb {ACL_MDB*} database object
 * @param tbl_name {const char*} data table name
 * @param walk_fn callback function, if this function returns 0, traversal stops
 * @param from {int} query result offset, which is the
 *  starting position for storage
 * @param limit {int} query result limit, maximum number
 * @return {int} number of traversed data nodes
 */
ACL_API int acl_mdb_walk(ACL_MDB *mdb, const char *tbl_name,
	int (*walk_fn)(const void*, unsigned int),
	int from, int limit);

/**
 * Total number of elements in a certain data table in the database.
 * @param mdb {ACL_MDB*} database object
 * @param tbl_name {const char*} data table name
 * @return {int} >=0
 */
ACL_API int acl_mdb_cnt(ACL_MDB *mdb, const char *tbl_name);

/************************************************************************/
/*                          in acl_mdt.c                                */
/************************************************************************/

/**
 * Create a data table.
 * @param dbtype {const char *} database type: hash/avl
 * @param tbl_name {const char*} table name
 * @param tbl_flag {unsigned int} table attribute flag bits
 * @param init_capacity {size_t} initial capacity of each internal hash table
 * @param key_labels {const char *[]} array of all field names, can be NULL
 * @param flags {unsigned int[]} constraint flag bits
 *  corresponding to key_labels
 * @return {ACL_MDT*} newly created data table object
 */
ACL_API ACL_MDT *acl_mdt_create(const char *dbtype, const char *tbl_name,
	unsigned int tbl_flag, size_t init_capacity,
	const char *key_labels[], unsigned int flags[]);

/**
 * Free a memory table.
 * @param mdt {ACL_MDT*} memory data table object
 */
ACL_API void acl_mdt_free(ACL_MDT *mdt);

/**
 * Add a new data record to the data table.
 * @param mdt {ACL_MDT*} data table object
 * @param data {void*} user dynamic data, if the ACL_MDT_FLAG_DMR flag bit
 *  is not set, the internal will copy this dynamic data
 * @param dlen {unsigned int} data buffer length
 * @param key_labels {const char*[]} data table field name array, can be NULL
 * @param keys {const char*[]} data table field value array, can be NULL
 * @return {ACL_MDT_NOD*} node pointer for the added data
 */
ACL_API ACL_MDT_NOD *acl_mdt_add(ACL_MDT *mdt, void *data,
	unsigned int dlen, const char *key_labels[], const char *keys[]);

/**
 * Probe whether the corresponding field value exists in the data table.
 * @param mdt {ACL_MDT*} data table object
 * @param key_label {const char*} data table field name
 * @param key {const char*} data table field key value
 * @return {int} 0: not found, != 0: found
 */
ACL_API int acl_mdt_probe(ACL_MDT *mdt, const char *key_label, const char *key);

/**
 * Query result set for a certain field value in the data table.
 * @param mdt {ACL_MDT*} data table object
 * @param key_label {const char*} data table field name
 * @param key {const char*} data table field key value
 * @param from {int} query result offset, which is the
 *  starting position for storage
 * @param limit {int} query result limit, maximum number
 * @return {ACL_MDT_REC*} result set corresponding to a certain field value
 */
ACL_API ACL_MDT_RES *acl_mdt_find(ACL_MDT *mdt, const char *key_label,
	const char *key, int from, int limit);

/**
 * Sequentially list all data node sets within a certain
 * range in the data table.
 * @param mdt {ACL_MDT*} data table object
 * @param from {int} query result offset, which is the
 *  starting position for storage
 * @param limit {int} query result limit, maximum number
 * @return {ACL_MDT_REC*} result set corresponding to a certain field value
 */
ACL_API ACL_MDT_RES *acl_mdt_list(ACL_MDT *mdt, int from, int limit);

/**
 * Delete result set corresponding to a certain field key
 * value in the data table.
 * @param mdt {ACL_MDT*} data table object
 * @param key_label {const char*} data table field name
 * @param key {const char*} data table field key value
 * @param onfree_fn {void (*)(void*, unsigned int}
 *	user callback function for freeing dynamic data
 * @return {int} number of freed data nodes
 */
ACL_API int acl_mdt_delete(ACL_MDT *mdt, const char *key_label,
	const char *key, void (*onfree_fn)(void*, unsigned int));

/**
 * Traverse data nodes in the data table, returning user data pointer.
 * @param mdt {ACL_MDT*} data table object
 * @param walk_fn callback function, if it returns 0, traversal stops
 * @param from {int} query result offset, which is the
 *  starting position for storage
 * @param len {int} query result limit, maximum number
 * @return {int} total traversed data length
 */
ACL_API int acl_mdt_walk(ACL_MDT *mdt, int (*walk_fn)(const void*, unsigned int),
	int from, int len);

/**
 * Get the next data node from the query result set.
 * @param res {ACL_MDT_RES*} data result set
 * @return {void*} user-identifiable dynamic data, if NULL, indicates
 *  the result set has no more data
 */
ACL_API const void *acl_mdt_fetch_row(ACL_MDT_RES *res);

/**
 * Get the number of records in the query result set.
 * @param res {ACL_MDT_RES*} data result set
 * @return {int} 0: result set is empty; > 0: result set is not empty
 */
ACL_API int acl_mdt_row_count(ACL_MDT_RES *res);

/**
 * Free the dynamic memory of the query result set, but does not free the
 * actual data nodes.
 * @param res {ACL_MDT_RES*} data result set
 */
ACL_API void acl_mdt_res_free(ACL_MDT_RES *res);

/**
 * Total number of elements in the data table.
 * @param mdt {ACL_MDT*} data table object
 * @return {int} >=0
 */
ACL_API int acl_mdt_cnt(ACL_MDT *mdt);

#ifdef	__cplusplus
}
#endif

#endif

