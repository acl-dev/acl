#ifndef	ACL_ZDB_INCLUDE_H
#define	ACL_ZDB_INCLUDE_H

#include "../stdlib/acl_define.h"
#include "../stdlib/acl_vstring.h"
#include "../stdlib/acl_fhandle.h"

#ifndef ACL_CLIENT_ONLY

typedef struct ZDB ZDB;
typedef struct ZDB_KEY_HDR ZDB_KEY_HDR;
typedef struct ZDB_BLK ZDB_BLK;
typedef struct ZDB_BLK_OFF ZDB_BLK_OFF;
typedef struct ZDB_DAT_HDR ZDB_DAT_HDR;
typedef struct ZDB_KEY_STORE ZDB_KEY_STORE;
typedef struct ZDB_DAT_STORE ZDB_DAT_STORE;
typedef struct ZDB_IO ZDB_IO;

/* file offset type definition */
typedef acl_int64 zdb_off_t;

/* key type definition */
typedef acl_int64 zdb_key_t;

/* link index type for data blocks in value storage */
typedef int zdb_lnk_t;

/* disk partition information structure */
typedef struct ZDB_DISK {
	acl_int64 limit;	/* maximum capacity of this partition */
	acl_int64 count;	/* currently allocated capacity of this partition */
	char *path;		/* disk partition path */
	int   idisk;		/* this disk partition number */
	int   priority;		/* priority level */
	int  *dat_ifiles;	/* currently used dat_ifile value storage file numbers */
	int   dat_ifiles_size;	/* size of dat_ifiles */
} ZDB_DISK;

/* configuration object for ZDB when calling zdb_open() */
typedef struct ZDB_CFG {
	const char *key_path;	/* parent directory where key storage is located */
	zdb_key_t key_begin;	/* key value starting value */
	zdb_key_t key_limit;	/* maximum number of keys in each key storage file */
	acl_int64 dat_limit;	/* maximum data capacity in each value storage file */

	int   key_cache_max;	/* maximum cache database count
				 * when key storage IO is enabled */
	int   key_cache_timeout; /* timeout for each cache when
				   * key storage IO is enabled */
	int   key_wback_max;	/* maximum write-back cache
				 * database count when key storage
				 * IO is enabled */

	int   dat_nstep;	/* step value when value storage expands database */
	int   blk_dlen;		/* length of data block part in
				 * value storage database
				 * (excluding user data length) */
	int   dat_cache_max;	/* maximum cache database count
				 * when value storage IO is enabled */
	int   dat_cache_timeout; /* timeout for each cache when
				   * value storage IO is enabled */
	int   dat_wback_max;	/* maximum write-back cache
				 * database count when value
				 * storage IO is enabled */
} ZDB_CFG;

/* ZDB structure definition */
struct ZDB {
	/* public */

	char *dbname;		/* database name */
	char *key_path;		/* ZDB key storage path */
	unsigned int oflags;	/* open-time flag bits */
#define	ZDB_FLAG_LINK_BUSY	(1 << 0)  /* whether to link busy
					 * data blocks in value storage */
#define	ZDB_FLAG_OPEN_LOCK	(1 << 1)  /* open storage file in exclusive mode */
#define	ZDB_FLAG_CACHE_DAT	(1 << 2)  /* whether to cache value
					 * storage database */
#define	ZDB_FLAG_CACHE_KEY	(1 << 3)  /* whether to cache key storage database */
#define	ZDB_FLAG_SLICE_KEY	(1 << 4)  /* whether to use memory
					 * slice mode when allocating
					 * key storage */
#define	ZDB_FLAG_SLICE_DAT	(1 << 5)  /* whether to use memory
					 * slice mode when allocating
					 * value storage */

	unsigned int status;	/* status bits */
#define	ZDB_STAT_KEY_NEW	(1 << 0)  /* new key */

	/* private */

	zdb_key_t key_begin;	/* key storage starting value */
	zdb_key_t key_limit;	/* maximum number of keys in each key storage */
	int   key_cache_max;	/* maximum cache database count
				 * when key storage IO is enabled */
	int   key_cache_timeout; /* timeout for each cache when
				   * key storage IO is enabled */
	int   key_wback_max;	/* maximum write-back cache
				 * database count when key storage
				 * IO is enabled */

	acl_int64 dat_limit;	/* maximum data capacity in value storage */
	int   blk_dlen;		/* length of each data block unit in value storage database */
	int   dat_nstep;	/* number of databases when value storage expands */
	int   dat_cache_max;	/* maximum cache database count
				 * when value storage IO is enabled */
	int   dat_cache_timeout; /* timeout for each cache when
				   * value storage IO is enabled */
	int   dat_wback_max;	/* maximum write-back cache
				 * database count when value
				 * storage IO is enabled */

	ZDB_DISK *dat_disks;	/* value storage disk partition array */

	int   (*key_get)(ZDB*, zdb_key_t, ZDB_BLK_OFF*);
	int   (*key_set) (ZDB*, zdb_key_t, const ZDB_BLK_OFF*);

	ZDB_BLK *(*dat_get)(ZDB*, const ZDB_BLK_OFF*, zdb_key_t*, size_t*);
	int   (*dat_add)(ZDB*, zdb_key_t, const void*, int);
	int   (*dat_update)(ZDB*, zdb_key_t, const ZDB_BLK_OFF*,
			const void*, size_t);

	/* private */

	/* temporary variables */

	ACL_VSTRING *path_tmp;	/* temporary storage file full
				 * path, mainly for debugging */
	int   blk_count_tmp;	/* temporary block count, mainly for debugging */
	int   inode_tmp;	/* temporary inode number, mainly for debugging */
};

/* xxx: To ensure cross-platform compatibility, all structures
 * below are 4-byte aligned */

#ifdef	ACL_SUNOS5
#pragma pack(4)
#else
#pragma pack(push, 4)
#endif

/* value storage header */
struct ZDB_DAT_HDR {
	acl_int64  limit;	/* maximum data capacity in value storage */
	acl_int64  size;	/* number of data blocks already
				 * allocated in current value
				 * storage file */
	acl_int64  count;	/* number of data in current value storage */
	acl_int64  reserv1;	/* reserved field */
	acl_int64  reserv2;	/* reserved field */
	acl_int64  reserv3;	/* reserved field */
	acl_int64  reserv4;	/* reserved field */
	int   nstep;		/* step value when value storage file size is small */
	int   blk_hdr_dlen;	/* header length of each data in
				 * value storage: ZDB_BLK_HDR.sizeof */
	int   blk_dlen;		/* length of each data block unit in value storage database */
	int   blk_count;	/* number of data blocks in this value storage */

	/* calculation formula for the actual file position of the first idle block:
	 * zdb_off_t off_head = ZDB_DAT_HDR.sizeof +
	 *     ZDB_DAT_HDR.ihead_idle * (ZDB_DAT_HDR.blk_hdr_dlen +
	 *         ZDB_DAT_HDR.blk_dlen * ZDB_DAT_HDR.blk_count);
	 */
	zdb_lnk_t ihead_idle;

	/* calculation formula for the actual file position of the first busy block:
	 * zdb_off_t off_head = ZDB_DAT_HDR.sizeof +
	 *     ZDB_DAT_HDR.ihead_busy * (ZDB_DAT_HDR.blk_hdr_dlen +
	 *         ZDB_DAT_HDR.blk_dlen * ZDB_DAT_HDR.blk_count);
	 */
	zdb_lnk_t ihead_busy;
	zdb_lnk_t itail_busy;
	int   dummy;		/* ensure 8-byte alignment */
};

/* key storage header */
struct ZDB_KEY_HDR {
	zdb_key_t  key_limit;	/* maximum number of keys in this key storage */
	zdb_key_t  key_count;	/* current number of keys */
	zdb_key_t  key_begin;	/* key starting value */
	acl_int64  reserv1;	/* reserved field */
	acl_int64  reserv2;	/* reserved field */
	acl_int64  reserv3;	/* reserved field */
	acl_int64  reserv4;	/* reserved field */
};

/* data block header structure definition */
typedef struct ZDB_BLK_HDR {
	zdb_key_t  key;		/* key corresponding to key storage */

	/* identifies the position of the data block in the
	 * database, checksum, and ensures 8-byte alignment */
	zdb_lnk_t  blk_ilnk;

	/* through inext_idle links idle data blocks into a
	 * linked list, a value of -1 indicates no next idle
	 * block, the first idle block position, calculation
	 * formula for the actual file position of the next idle
	 * block:
	 * zdb_off_t off_next = ZDB_BLK_HDR.sizeof +
	 *     ZDB_BLK_HDR.inext_idle * (ZDB_BLK_HDR.blk_hdr_dlen +
	 *         ZDB_DAT_HDR.blk_dlen * ZDB_DAT_HDR.blk_count);
	 */
	zdb_lnk_t  inext_idle;

#ifdef	ZDB_LINK_BUSY
	/* through inext_busy/iprev_busy links busy data blocks
	 * into a doubly linked list, a value of -1 indicates no
	 * next busy block, the first busy block position,
	 * calculation formula for the actual file position of
	 * the next busy block:
	 * zdb_off_t off_next = ZDB_BLK_HDR.sizeof +
	 *     ZDB_BLK_HDR.inext_busy * (ZDB_BLK_HDR.blk_hdr_dlen +
	 *         ZDB_DAT_HDR.blk_dlen * ZDB_DAT_HDR.blk_count);
	 * zdb_off_t off_prev = ZDB_BLK_HDR.sizeof +
	 *     ZDB_BLK_HDR.inext_prev * (ZDB_BLK_HDR.blk_hdr_dlen +
	 *         ZDB_DAT_HDR.blk_dlen * ZDB_DAT_HDR.blk_count);
	 */
	zdb_lnk_t  inext_busy;
	zdb_lnk_t  iprev_busy;
#endif
} ZDB_BLK_HDR;

/* offset information stored in key storage for value storage */
struct ZDB_BLK_OFF {
	zdb_off_t offset;	/* offset information */
	int   inode;		/* inode information */
};

/* data block structure definition */
struct ZDB_BLK {
	ZDB_BLK_HDR hdr;
	char  dat[1];		/* user data stored in each data block in value storage */
};

#ifdef	ACL_SUNOS5
#pragma pack(0)  /* restore to cancel 4-byte alignment */
#else
#pragma pack(pop)  /* restore to cancel 4-byte alignment */
#endif

/* common storage structure */
typedef struct ZDB_STORE {
	ACL_FHANDLE fhandle;	/* file handle */
	ZDB *db;		/* configuration object */
	ZDB_IO *io;		/* IO cache object */
	int   cache_max;	/* maximum cache database count when storage IO is enabled */
	int   cache_timeout;	/* timeout for each cache when storage IO is enabled */
	int   wback_max;	/* write-back database count */

	unsigned int flag;	/* flag bits */
#define	STORE_FLAG_KEY		(1 << 0)  /* indicates key storage */
#define	STORE_FLAG_DAT		(1 << 1)  /* indicates value storage */
#define	STORE_FLAG_IO_SLICE	(1 << 2)  /* whether to use memory
					 * slice allocation for ZDB_IO */

	/* for acl_iterator */

	/* Get the head element of the container */
	const void *(*iter_head)(ACL_ITER*, struct ZDB_STORE*);
	/* Get the next element of the container */
	const void *(*iter_next)(ACL_ITER*, struct ZDB_STORE*);
} ZDB_STORE;

/* key storage */
struct ZDB_KEY_STORE {
	ZDB_STORE store;
	ZDB_KEY_HDR hdr;	/* key storage header */
};


/* value storage */
struct ZDB_DAT_STORE {
	ZDB_STORE store;
	ZDB_DAT_HDR hdr;	/* value storage header */
};

#ifdef	__cplusplus
extern "C" {
#endif

/*-------------------------  in zdb_test.c  ----------------------*/

/**
 * zdb test function
 */
ACL_API void zdb_test(const char *cmd);

/*----------------------------   in zdb.c   ----------------------*/

/* in zdb.c */

/**
 * Initialize zdb when the program starts.
 */
ACL_API void zdb_init(void);

/**
 * Release some internal resources of zdb before the program exits.
 */
ACL_API void zdb_end(void);

/**
 * Synchronize ZDB database information to disk.
 * @param db {ZDB*}
 */
ACL_API void zdb_sync(ZDB *db);

/**
 * Open or create a ZDB database.
 * @param dbname {const char*} ZDB database name
 * @param oflags {unsigned int} flag bits when opening ZDB
 * @param cfg {const ZDB_CFG*} configuration object when opening ZDB
 * @return {ZDB*} ZDB database object
 */
ACL_API ZDB *zdb_open(const char *dbname, unsigned int oflags, const ZDB_CFG *cfg);

/**
 * Close ZDB database object.
 * @param db {ZDB*} ZDB database object
 */
ACL_API void zdb_close(ZDB *db);

/**
 * Look up the corresponding value data in the ZDB database.
 * @param db {ZDB*} ZDB database object
 * @param key {zdb_key_t} key value
 * @param size {size_t*} if this pointer address is non-NULL,
 *  it will store the queried data length, which is the
 *  length of data stored in ZDB_BLK.dat
 * @param blk_off_buf {ZDB_BLK_OFF*} if non-NULL, stores the
 *  offset information of the value
 * @return {ZDB_BLK*} NULL: not found; !NULL: ZDB_BLK->dat is
 *  the user data address, the ZDB_BLK object needs to be
 *  freed with acl_myfree()/1 after use
 * Note:
 *  If the size pointer address is non-NULL, the caller's
 *  variable must be of size_t type, not int type, because
 *  on 64-bit systems size_t is 8 bytes long, while int is 4
 *  bytes long. When internally writing to the size address,
 *  it will automatically write the size_t type actual space
 *  length. If the caller's size is of int type, it will
 *  cause memory overflow, which may cause unpredictable
 *  problems for the caller, and valgrind cannot detect this
 *  problem!
 */
ACL_API ZDB_BLK *zdb_lookup(ZDB *db, zdb_key_t key, size_t *size, ZDB_BLK_OFF *blk_off_buf);

/**
 * Get user data from ZDB_BLK.
 * @param b {ZDB_BLK*}
 * @return {void*}
 */
#define	zdb_blk_data(b)	((b)->dat)

/**
 * Free the data space returned by zdb_lookup()/3.
 * @param b {ZDB_BLK*}
 */
#define	zdb_blk_free(b)	acl_myfree((b))

/**
 * Add or update data in the ZDB database.
 * @param db {ZDB*} ZDB database object
 * @param key {zdb_key_t} key value
 * @param blk_off_saved {const ZDB_BLK_OFF*} return result
 *  from the last call to zdb_lookup()/4, so that repeated
 *  queries can be avoided for efficiency. If the return
 *  result from calling zdb_lookup()/4 is empty, then when
 *  calling zdb_update()/5, this value should be NULL
 * @param dat {const void*} data address corresponding to key
 * @param len {size_t} dat data length
 * @return {int} 0: data not updated; -1: failure; 1: success
 */
ACL_API int zdb_update(ZDB *db, zdb_key_t key, const ZDB_BLK_OFF *blk_off_saved,
	const void *dat, size_t len);

/*--------------------------------- in zdb_key.c -----------------*/

/**
 * Synchronize key storage header to disk.
 * @param store {ZDB_KEY_STORE*} key storage object
 * @return {int} -1: error, > 0 ok
 */
ACL_API int key_store_header_sync(ZDB_KEY_STORE *store);

/**
 * Open key storage based on key value.
 * @param db {ZDB*}
 * @param key {zdb_key_t} key value
 * @return {ZDB_KEY_STORE*} !NULL: ok; NULL: error
 */
ACL_API ZDB_KEY_STORE *zdb_key_store_open(ZDB *db, zdb_key_t key);

/**
 * Open key storage based on file path.
 * @param db {ZDB*}
 * @param filepath {const char*} file path
 * @return {ZDB_KEY_STORE*} !NULL: ok; NULL: error
 */
ACL_API ZDB_KEY_STORE *zdb_key_store_open2(ZDB *db, const char *filepath);

/**
 * Close key storage.
 * @param store {ZDB_KEY_STORE*} key storage object
 */
ACL_API void zdb_key_store_close(ZDB_KEY_STORE *store);

/**
 * Set the value position value for a key position in key storage.
 * @param db {ZDB*}
 * @param key {zdb_key_t}
 * @param blk_off {const ZDB_BLK_OFF*}
 * @return {int} 0: ok; -1: error
 */
ACL_API int zdb_key_set(ZDB *db, zdb_key_t key, const ZDB_BLK_OFF *blk_off);

/**
 * Get the corresponding value storage offset information for
 * a key from key storage.
 * @param db {ZDB*}
 * @param key {zdb_key_t} key value
 * @param blk_off {ZDB_BLK_OFF*} storage for the result
 * @return {int} 1: indicates found, 0: indicates not found, -1: indicates error
 */
ACL_API int zdb_key_get(ZDB *db, zdb_key_t key, ZDB_BLK_OFF *blk_off);

/**
 * Query key storage header status.
 * @param db {ZDB*}
 * @param filepath {const char*} key storage file full path
 * @param key_hdr {ZDB_KEY_HDR*} memory address for storing
 *  key storage header information
 * @return {int} 0: ok; -1: error
 */
ACL_API int zdb_key_status(ZDB *db, const char *filepath, ZDB_KEY_HDR *key_hdr);

/**
 * Check key status in key storage.
 * @param db {ZDB*}
 * @param filepath {const char*} key storage file full path
 * @param key_hdr {ZDB_KEY_HDR*} if non-NULL, stores the key
 *  storage header information memory address
 * @return {int} 0: ok; -1: error
 */
ACL_API int zdb_key_check3(ZDB *db, const char *filepath, ZDB_KEY_HDR *key_hdr);
ACL_API int zdb_key_check(ZDB_KEY_STORE *store, ZDB_KEY_HDR *key_hdr);

/**
 * Initialize key storage.
 * @param db {ZDB*}
 * @param key_begin {zdb_key_t} starting key value
 * @param key_end {zdb_key_t} ending key value
 * @return {int} 0: ok; -1: error
 */
ACL_API int zdb_key_init(ZDB *db, zdb_key_t key_begin, zdb_key_t key_end);
/*--------------------------------- in zdb_dat.c -----------------*/

/**
 * Synchronize value storage header to disk.
 * @param store {ZDB_DAT_STORE*} value storage object
 * @return {int} -1: error, > 0 ok
 */
ACL_API int dat_store_header_sync(ZDB_DAT_STORE *store);

/**
 * Close value storage.
 * @param store {ZDB_DAT_STORE*} value storage object
 */
ACL_API void zdb_dat_store_close(ZDB_DAT_STORE *store);

/**
 * Open or create value storage.
 * @param filepath {const char*} value storage file full path
 * @return {ZDB_DAT_STORE*} !NULL: ok; NULL: error
 */
ACL_API ZDB_DAT_STORE *zdb_dat_store_open(ZDB *db, const char *filepath);

/**
 * Get the corresponding data from value storage.
 * @param db {ZDB*}
 * @param blk_off {const ZDB_BLK_OFF*}
 * @param key {zdb_key_t*} storage for key value
 * @param size {size_t*} storage for data length
 * @return {void*} data address, if not found or data is
 *  empty, returns NULL, otherwise non-NULL, needs to be
 *  freed with zdb_blk_free()/1 after use
 * Note:
 *  If the size pointer address is non-NULL, the caller's
 *  variable must be of size_t type, not int type, because
 *  on 64-bit systems size_t is 8 bytes long, while int is
 *  4 bytes long. When internally writing to the size
 *  address, it will automatically write the size_t type
 *  actual space length. If the caller's size is of int type,
 *  it will cause memory overflow, which may cause
 *  unpredictable problems for the caller, and valgrind
 *  cannot detect this problem!
 */
ACL_API ZDB_BLK *zdb_dat_get(ZDB *db, const ZDB_BLK_OFF *blk_off,
	zdb_key_t *key, size_t *size);

/**
 * Add data to value storage.
 * @param db {ZDB*}
 * @param key {zdb_key_t} key value
 * @param dat {const void*} data address
 * @param len {size_t} dat data length
 * @return {int} 0: data not added; 1: data added; -1: failure
 */
ACL_API int zdb_dat_add(ZDB *db, zdb_key_t key, const void *dat, int len);

/**
 * Update data information in value storage.
 * @param db {ZDB*}
 * @param key {zdb_key_t} key
 * @param blk_off {const ZDB_BLK_OFF*} offset information for the corresponding
 *  value stored in key storage
 * @param dat {const void*} data address
 * @param len {size_t} dat data length
 * @return {int} 0: data not updated; 1: data updated; -1: failure
 */
ACL_API int zdb_dat_update(ZDB *db, zdb_key_t key, const ZDB_BLK_OFF *blk_off,
	const void *dat, size_t len);

/**
 * Get value storage header information.
 * @param db {ZDB*}
 * @param filepath {const char*} value storage file path
 * @param dat_hdr {ZDB_DAT_HDR*} storage object
 * @return {int} 0: ok; -1: error
 */
ACL_API int zdb_dat_stat(ZDB *db, const char *filepath, ZDB_DAT_HDR *dat_hdr);

/**
 * Check data block integrity in value storage.
 * @param db {ZDB*}
 * @param filepath {const char*} value storage file path
 * @param dat_hdr {ZDB_DAT_HDR*} if non-NULL, stores value storage header information
 * @return {int} 0: ok; -1: error
 */
ACL_API int zdb_dat_check3(ZDB *db, const char *filepath, ZDB_DAT_HDR *dat_hdr);
ACL_API int zdb_dat_check(ZDB_DAT_STORE *store, ZDB_DAT_HDR *dat_hdr);

/*--------------------   in zdb_key_walk.c -----------------------*/
ACL_API int zdb_key_walk(ZDB *db, int (*walk_fn)(ZDB_KEY_STORE *store));

/*--------------------   in zdb_dat_walk.c -----------------------*/
ACL_API int zdb_dat_walk(ZDB *db, int (*walk_fn)(ZDB_DAT_STORE *store));

/*--------------------   in zdb_key_iter.c -----------------------*/

/*--------------------   in zdb_dat_iter.c -----------------------*/
/**
 * Set value storage iterator.
 * @param store {ZDB_DAT_STORE*}
 * @param read_data {int} whether to read data blocks from the database,
 *  !0: indicates to read data blocks from the database;
 *  0: indicates not to read data blocks, only headers
 */
ACL_API void zdb_dat_iter_set(ZDB_DAT_STORE *store, int read_data);

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif
