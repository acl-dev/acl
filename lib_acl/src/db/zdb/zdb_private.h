#ifndef	__ZDB_PRIVATE_INCLUDE_H__
#define	__ZDB_PRIVATE_INCLUDE_H__

#ifndef ACL_CLIENT_ONLY

#include "db/zdb.h"

#define	ZDB_DBG_BASE		500
#define	ZDB_DBG_GETK		(ZDB_DBG_BASE)
#define	ZDB_DBG_GETD		(ZDB_DBG_BASE + 1)
#define	ZDB_DBG_ADDD		(ZDB_DBG_BASE + 2)
#define	ZDB_DBG_KEY		(ZDB_DBG_BASE + 3)
#define	ZDB_DBG_DAT		(ZDB_DBG_BASE + 4)

#define	ZDB_KEY_LIMIT		(unsigned int) -1
#define	ZDB_DAT_FILE_LIMIT	10000	/* 值存储目录下每个目录下最大的文件个数 */
#define	ZDB_KEY_DIR_LIMIT	10	/* 键存储目录个数 */
#define	ZDB_DAT_DIR_LIMIT	10	/* 值存储目录个数 */

typedef const void *(*STORE_ITER)(ACL_ITER*, ZDB_STORE*);

/*--------------------------- 一些简单方便的宏定义 ---------------------------*/

#define	DISK_BITS	8
#define	DISK_MASK	0xff
#define	DISK_LIMIT	(1 << 8)

#define	DIR_BITS	24
#define	DIR_MASK	0xffffff
#define	DIR_LIMIT	(1 << 24)

/**
 * 取得 ACL_VSTRING 对象中的数据地址
 * @param x {ACL_VSTRING*}
 * @return {char*}
 */
#ifndef	STR
#define	STR(x)	acl_vstring_str((x))
#endif

/**
 * 取得 ACL_VSTRING 对象中数据长度
 * @param x {ACL_VSTRING*}
 * @return {size_t} 数据长度
 */
#ifndef	LEN
#define	LEN(x)	ACL_VSTRING_LEN((x))
#endif

/**
 * 取得 ACL_VSTREAM 对象中文件路径
 * @param x {ACL_VSTREAM*}
 * @return {char*} 文件路径
 */
#define	PATH(x)	ACL_VSTREAM_PATH((x))

/**
 * 取得存储中文件句柄的文件路径
 * @param s {ZDB_KEY_STORE* || ZDB_DAT_STORE*}
 * @return {char*} 文件路径
 */
#define	STORE_PATH(s)	PATH((s)->fhandle.fp)

/**
 * 取得存储中文件描述符
 * @param s {ZDB_KEY_STORE* || ZDB_DAT_STORE*}
 * @return {ACL_FILE_HANDLE} 文件描述符
 */
#define	STORE_FILE(s)	ACL_VSTREAM_FILE((s)->fhandle.fp)

/**
 * 获得存储文件的 ACL_VSTREAM 流
 * @param s {ZDB_KEY_STORE* || ZDB_DAT_STORE*}
 * @return {ACL_VSTREAM*}
 */
#define	STORE_STREAM(s)	((s)->fhandle.fp)

/**
 * 根据键值取得键存储的相对路径号
 * @param z {ZDB*}
 * @param k {zdb_key_t}
 * @return {int} 相对路径号
 */
#define	KEY_INODE(z, k)	(int) ((k) / (z)->key_limit)

/**
 * 由键值及键限制值取得该键值的余数
 * @param z {ZDB*}
 * @param k {zdb_key_t}
 * @return {zdb_key_t} 余数
 */
#define	KEY_MOD(z, k) ((k) % (z)->key_limit)

/**
 * 根据键值取得该键在键存储文件中的偏移位置
 * @param z {ZDB*}
 * @param k {zdb_key_t}
 * @return {zdb_off_t}
 */
#define	KEY_OFF(z, k)  (  \
	((k) - KEY_INODE((z), (k)) * (zdb_off_t) (z)->key_limit) \
	* (zdb_off_t) sizeof(ZDB_BLK_OFF)  \
	+ (zdb_off_t) sizeof(ZDB_KEY_HDR)  )

/**
 * 数据块头部长度
 * @param s {ZDB_DAT_STORE*}
 * @return {int}
 */
#define	BLK_HDR_LEN(s)	((zdb_off_t) (s)->hdr.blk_hdr_dlen)

/**
 * 数据块长度(包含数据头部分)
 * @param s {ZDB_DAT_STORE*}
 * @return {int}
 */
#define BLK_LEN(s)  (  \
	BLK_HDR_LEN((s)) +  \
	(zdb_off_t) (s)->hdr.blk_dlen * (zdb_off_t) (s)->hdr.blk_count  )

/**
 * 计算数据块中数据头在值存储中的偏移位置
 * @param s {ZDB_DAT_STORE*}
 * @param x {zdb_lnk_t} 文件存储位置索引号
 * @return {zdb_off_t}
 */
#define	BLK_HDR_OFF(s, x)  (  \
	(zdb_off_t) sizeof(ZDB_DAT_HDR) + BLK_LEN((s)) * (zdb_off_t) ((x))  )

/**
 * 计算数据块中数据部分在值存储中的偏移位置
 * @param s {ZDB_DAT_STORE*}
 * @param x {zdb_lnk_t} 文件存储位置索引号
 * @return {zdb_off_t}
 */
#define	BLK_DAT_OFF(s, x)  \
	(BLK_HDR_OFF(s, x) + (zdb_off_t) sizeof(ZDB_BLK_HDR))

#ifdef	__cplusplus
extern "C" {
#endif

/*----------------------------   in zdb.c   ----------------------------------*/

/**
 * 从磁盘分区中选择合适的分区节点
 * @param db {ZDB*}
 */
int zdb_disk_select(ZDB *db);

/*--------------------------------- in zdb_io.c ------------------------------*/

#undef	DEBUG_ZDB_RW

#ifdef	DEBUG_ZDB_RW
#define	ZDB_WRITE(s, buf, len, off)  (  \
    acl_msg_info("%s(%d), %s: call zdb_write(%s, %ld, %lld)\n",  \
	__FILE__, __LINE__, __FUNCTION__, STORE_PATH((s)),  \
	(size_t) len, (zdb_off_t) off), 1 ?  \
    zdb_write((s), (buf), (len), (off)) : ACL_VSTREAM_EOF  )

#define	ZDB_READ(s, buf, len, off)  (  \
    acl_msg_info("%s(%d), %s: call zdb_read(%s, %ld, %lld)\n",  \
	__FILE__, __LINE__, __FUNCTION__, STORE_PATH((s)),  \
	(size_t) len, (zdb_off_t) off), 1 ?  \
    zdb_read((s), (buf), (len), (off)) : ACL_VSTREAM_EOF  )
#else
#define	ZDB_WRITE	zdb_write
#define	ZDB_READ	zdb_read
#endif

int zdb_io_cache_sync(ZDB_STORE *store);
void zdb_io_cache_open(ZDB_STORE *store, size_t blk_len);
void zdb_io_cache_close(ZDB_STORE *store);

/**
 * 封装了 pwrite64 的写接口
 * @param store {ZDB_STORE*} 文件句柄
 * @param buf {const void*} 数据地址
 * @param len {size_t} 数据长度
 * @param off {zdb_off_t} 文件中的位置偏移量
 * @return {int} > 0: ok; -1: error
 */
int zdb_write(ZDB_STORE *store, const void *buf, size_t len, zdb_off_t off);

/**
 * 封装了 pread64 的读接口
 * @param store {ZDB_STORE*} 文件句柄
 * @param buf {const void*} 缓冲区地址
 * @param len {size_t} 缓冲区长度
 * @param off {zdb_off_t} 文件中的位置偏移量
 * @return {int} > 0: ok; -1: error
 */
int zdb_read(ZDB_STORE *store, void *buf, size_t size, zdb_off_t off);

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif
