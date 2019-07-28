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

/* 偏移量的长度类型 */
typedef acl_int64 zdb_off_t;

/* 键类型定义 */
typedef acl_int64 zdb_key_t;

/* 值存储中空闲数据块链接偏移量类型 */
typedef int zdb_lnk_t;

/* 磁盘分区信息类型 */
typedef struct ZDB_DISK {
	acl_int64 limit;	/* 该分区的总数限制 */
	acl_int64 count;	/* 目前该分区已分配数量 */
	char *path;		/* 磁盘分区路径 */
	int   idisk;		/* 该磁盘分区号 */
	int   priority;		/* 权重优先级 */
	int  *dat_ifiles;	/* 当前可用的 dat_ifile 值存储文件索引号 */
	int   dat_ifiles_size;	/* dat_ifiles 的大小 */
} ZDB_DISK;

/* 调用 zdb_open() 时的 ZDB 配置对象 */
typedef struct ZDB_CFG {
	const char *key_path;	/* 键存储所在的根目录 */
	zdb_key_t key_begin;	/* 键值的起始值 */
	zdb_key_t key_limit;	/* 每个键存储文件中键的最大个数 */
	acl_int64 dat_limit;	/* 每个值存储文件中数据的最大个数 */

	int   key_cache_max;	/* 启用键存储 IO 缓存时的最大缓存数据块个数 */
	int   key_cache_timeout; /* 启用键存储 IO 缓存时每个缓存块的过期时间 */
	int   key_wback_max;	/* 启用键存储 IO 缓存时写缓存的最大缓存数据块个数 */

	int   dat_nstep;	/* 值存储中增加数据块时的步进值 */
	int   blk_dlen;		/* 值存储中数据块中数据部分的长度(等于用户数据长度) */
	int   dat_cache_max;	/* 启用值存储 IO 缓存时的最大缓存数据块个数 */
	int   dat_cache_timeout; /* 启用值存储 IO 缓存时每个缓存块的过期时间 */
	int   dat_wback_max;	/* 启用值存储 IO 缓存时写缓存的最大缓存数据块个数 */
} ZDB_CFG;

/* ZDB 结构类型 */
struct ZDB {
	/* public */

	char *dbname;		/* 数据库名 */
	char *key_path;		/* ZDB 的key存储路径 */
	unsigned int oflags;	/* 打开时的标志位 */
#define	ZDB_FLAG_LINK_BUSY	(1 << 0)  /* 将值存储中的占用数据块连接起来 */
#define	ZDB_FLAG_OPEN_LOCK	(1 << 1)  /* 以加锁模式打开存储句柄 */
#define	ZDB_FLAG_CACHE_DAT	(1 << 2)  /* 是否缓存值存储的数据 */
#define	ZDB_FLAG_CACHE_KEY	(1 << 3)  /* 是否缓存键存储的数据 */
#define	ZDB_FLAG_SLICE_KEY	(1 << 4)  /* 启用值存储时是否采用内存切片方式 */
#define	ZDB_FLAG_SLICE_DAT	(1 << 5)  /* 启用键存储时是否采用内存切片方式 */

	unsigned int status;	/* 状态位 */
#define	ZDB_STAT_KEY_NEW	(1 << 0)  /* 新键 */

	/* private */

	zdb_key_t key_begin;	/* 所有键存储的起始值 */
	zdb_key_t key_limit;	/* 每个键存储中键的个数的最大值 */
	int   key_cache_max;	/* 启用键存储 IO 缓存时的最大缓存数据块个数 */
	int   key_cache_timeout; /* 启用键存储 IO 缓存时每个缓存块的过期时间 */
	int   key_wback_max;	/* 启用键存储 IO 缓存时写缓存的最大缓存数据块个数 */

	acl_int64 dat_limit;	/* 值存储中值对象存储个数限制 */
	int   blk_dlen;		/* 值存储中数据块中每个基础块单元长度 */
	int   dat_nstep;	/* 值存储中每次增加数据块的个数 */
	int   dat_cache_max;	/* 启用值存储 IO 缓存时的最大缓存数据块个数 */
	int   dat_cache_timeout; /* 启用值存储 IO 缓存时每个缓存块的过期时间 */
	int   dat_wback_max;	/* 启用值存储 IO 缓存时写缓存的最大缓存数据块个数 */

	ZDB_DISK *dat_disks;	/* 值存储磁盘分区数组 */

	int   (*key_get)(ZDB*, zdb_key_t, ZDB_BLK_OFF*);
	int   (*key_set) (ZDB*, zdb_key_t, const ZDB_BLK_OFF*);

	ZDB_BLK *(*dat_get)(ZDB*, const ZDB_BLK_OFF*, zdb_key_t*, size_t*);
	int   (*dat_add)(ZDB*, zdb_key_t, const void*, int);
	int   (*dat_update)(ZDB*, zdb_key_t, const ZDB_BLK_OFF*,
			const void*, size_t);

	/* private */

	/* 以下为临时变量 */

	ACL_VSTRING *path_tmp;	/* 临时用的存储文件的全路径, 主要为了参数传递 */
	int   blk_count_tmp;	/* 临时用的块数, 主要为了参数传递 */
	int   inode_tmp;	/* 临时用的相对路径号，主要为了参数传递 */
};

/* xxx: 为了保证跨平台性，以下结构定义都是4字节对齐的 */

#ifdef	ACL_SUNOS5
#pragma pack(4)
#else
#pragma pack(push, 4)
#endif

/* 值存储头 */
struct ZDB_DAT_HDR {
	acl_int64  limit;	/* 值存储中值对象存储个数限制 */
	acl_int64  size;	/* 当前值存储文件已经分配的对象存储的个数 */
	acl_int64  count;	/* 当前值存储中的总数 */
	acl_int64  reserv1;	/* 保留字段 */
	acl_int64  reserv2;	/* 保留字段 */
	acl_int64  reserv3;	/* 保留字段 */
	acl_int64  reserv4;	/* 保留字段 */
	int   nstep;		/* 每次增加时值存储文件大小时的步进值大小 */
	int   blk_hdr_dlen;	/* 值存储中每块数据的头部长度: ZDB_BLK_HDR.sizeof */
	int   blk_dlen;		/* 值存储中数据块中每个基础块单元长度 */
	int   blk_count;	/* 该值存储中的数据块中基础块个数 */

	/* 第一个空闲块的位置的真实具体位置的计算公式为:
	 * zdb_off_t off_head = ZDB_DAT_HDR.sizeof +
	 *     ZDB_DAT_HDR.ihead_idle * (ZDB_DAT_HDR.blk_hdr_dlen +
	 *         ZDB_DAT_HDR.blk_dlen * ZDB_DAT_HDR.blk_count);
	 */
	zdb_lnk_t ihead_idle;

	/* 第一个数据块的位置的真实具体位置的计算公式为:
	 * zdb_off_t off_head = ZDB_DAT_HDR.sizeof +
	 *     ZDB_DAT_HDR.ihead_busy * (ZDB_DAT_HDR.blk_hdr_dlen +
	 *         ZDB_DAT_HDR.blk_dlen * ZDB_DAT_HDR.blk_count);
	 */
	zdb_lnk_t ihead_busy;
	zdb_lnk_t itail_busy;
	int   dummy;		/* 保证8字节对齐 */
};

/* 键存储的头 */
struct ZDB_KEY_HDR {
	zdb_key_t  key_limit;	/* 该键存储的最大容量 */
	zdb_key_t  key_count;	/* 当前所有键的总数 */
	zdb_key_t  key_begin;	/* 键的起始值 */
	acl_int64  reserv1;	/* 保留字段 */
	acl_int64  reserv2;	/* 保留字段 */
	acl_int64  reserv3;	/* 保留字段 */
	acl_int64  reserv4;	/* 保留字段 */
};

/* 块数据的头结构类型 */
typedef struct ZDB_BLK_HDR {
	zdb_key_t  key;		/* 对应于键存储中的键 */

	/* 标识本数据块的索引位置, 校验用, 同时保证了 8 字节对齐 */
	zdb_lnk_t  blk_ilnk;

	/* 由 inext_idle 将空闲数据块连接成一个单向链表, 若此值为 -1 则表示
	 * 非空闲块, 后一个空闲块位置, 后一个空闲块的真实具体位置的计算公式为:
	 * zdb_off_t off_next = ZDB_BLK_HDR.sizeof +
	 *     ZDB_BLK_HDR.inext_idle * (ZDB_BLK_HDR.blk_hdr_dlen +
	 *         ZDB_DAT_HDR.blk_dlen * ZDB_DAT_HDR.blk_count);
	 */
	zdb_lnk_t  inext_idle;

#ifdef	ZDB_LINK_BUSY
	/* 由 inext_busy/iprev_busy 将占用数据块连接成一个双向链表, 若此值为 -1
	 * 则表示非占用块, 后一个占用块位置, 后一个占用块的真实具体位置的计算公式为:
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

/* 键存储中存储的值存储的偏移量类型 */
struct ZDB_BLK_OFF {
	zdb_off_t offset;	/* 偏移量信息 */
	int   inode;		/* 路径信息 */
};

/* 块数据结构类型 */
struct ZDB_BLK {
	ZDB_BLK_HDR hdr;
	char  dat[1];		/* 值存储中每块数据的体数据 */
};

#ifdef	ACL_SUNOS5
#pragma pack(0)  /* 以下取消4字节对齐限制 */
#else
#pragma pack(pop)  /* 以下取消4字节对齐限制 */
#endif

/* 通用存储句柄结构 */
typedef struct ZDB_STORE {
	ACL_FHANDLE fhandle;	/* 文件句柄 */
	ZDB *db;		/* 引用对象 */
	ZDB_IO *io;		/* IO 句柄对象 */
	int   cache_max;	/* 启用存储 IO 缓存时的最大缓存数据块个数 */
	int   cache_timeout;	/* 启用存储 IO 缓存时每个缓存块的过期时间 */
	int   wback_max;	/* 写缓存中数据块的限制 */

	unsigned int flag;	/* 标志位 */
#define	STORE_FLAG_KEY		(1 << 0)  /* 表示是键存储 */
#define	STORE_FLAG_DAT		(1 << 1)  /* 表示是值存储 */
#define	STORE_FLAG_IO_SLICE	(1 << 2)  /* 是否启用 ZDB_IO 的内存切片分配方式 */

	/* for acl_iterator */

	/* 取迭代器头函数 */
	const void *(*iter_head)(ACL_ITER*, struct ZDB_STORE*);
	/* 取迭代器下一个函数 */
	const void *(*iter_next)(ACL_ITER*, struct ZDB_STORE*);
} ZDB_STORE;

/* 键存储 */
struct ZDB_KEY_STORE {
	ZDB_STORE store;
	ZDB_KEY_HDR hdr;	/* 键存储头 */
};


/* 值存储 */
struct ZDB_DAT_STORE {
	ZDB_STORE store;
	ZDB_DAT_HDR hdr;	/* 值存储头 */
};

#ifdef	__cplusplus
extern "C" {
#endif

/*-------------------------  in zdb_test.c  ----------------------------------*/

/**
 * zdb 测试函数
 */
ACL_API void zdb_test(const char *cmd);

/*----------------------------   in zdb.c   ----------------------------------*/

/* in zdb.c */

/**
 * 程序开始运行时需要初始化 zdb
 */
ACL_API void zdb_init(void);

/**
 * 程序退出前需要释放 zdb 内部一些资源
 */
ACL_API void zdb_end(void);

/**
 * 更新ZDB相关信息至磁盘
 * @param db {ZDB*}
 */
ACL_API void zdb_sync(ZDB *db);

/**
 * 打开或创建一个 ZDB 数据库
 * @param dbname {const char*} ZDB 数据库名称
 * @param oflags {unsigned int} 打开 ZDB 库时的标志位
 * @param cfg {const ZDB_CFG*} 打开 ZDB 时的配置对象
 * @return {ZDB*} ZDB 数据库句柄
 */
ACL_API ZDB *zdb_open(const char *dbname, unsigned int oflags, const ZDB_CFG *cfg);

/**
 * 关闭 ZDB 数据库句柄
 * @param db {ZDB*} ZDB 数据库句柄
 */
ACL_API void zdb_close(ZDB *db);

/**
 * 在 ZDB 数据库中查找相应键值的数据
 * @param db {ZDB*} ZDB 数据库句柄
 * @param key {zdb_key_t} 键值
 * @param size {size_t*} 若此指针非空且查询结果也非空则存储查询结果的数据长度,
 *  即是 ZDB_BLK.dat 中存储数据的长度
 * @param blk_off_buf {ZDB_BLK_OFF*} 若非空则存储所查值的偏移位置索引号
 * @return {ZDB_BLK*} NULL: 未找到; !NULL: ZDB_BLK->dat 为用户数据的地址,
 *  其中 ZDB_BLK 对象可以用 acl_myfree()/1 进行释放
 * 注意:
 *  当 size 指针地址非空时，调用者的类型必须是 size_t 类型而非 int 类型，因为在
 *  64位机上 size_t 为8个字节长度，而 int 为4个字节长度，内部在对 size 地址赋值
 *  时，编译器会自动按 size_t 的真实空间长度赋值，如果调用者的 size 为 int 类型，
 *  则就会出现空间越办现象，从而会使整个程序出现莫名其妙的问题，并且用 valgrind
 *  也查不出这个错误来!
 */
ACL_API ZDB_BLK *zdb_lookup(ZDB *db, zdb_key_t key, size_t *size, ZDB_BLK_OFF *blk_off_buf);

/**
 * 从 ZDB_BLK 中取得用户数据
 * @param b {ZDB_BLK*}
 * @return {void*}
 */
#define	zdb_blk_data(b)	((b)->dat)

/**
 * 释放由 zdb_lookup()/3 返回的数据空间
 * @param b {ZDB_BLK*}
 */
#define	zdb_blk_free(b)	acl_myfree((b))

/**
 * 添加或更新 ZDB 数据库中的数据
 * @param db {ZDB*} ZDB 数据库句柄
 * @param key {zdb_key_t} 键值
 * @param blk_off_saved {const ZDB_BLK_OFF*} 上次调用 zdb_lookup()/4 时的返回结果,
 *  从而通过重复利用查询结果来提高效率, 如果在调用 zdb_lookup()/4 时返回结果为空,
 *  则调用 zdb_update()/5 时必须将此值置 NULL
 * @param dat {const void*} 键 key 所对应的数据地址
 * @param len {size_t} dat 数据长度
 * @return {int} 0: 未更新或添加; -1: 出错; 1: 成功
 */
ACL_API int zdb_update(ZDB *db, zdb_key_t key, const ZDB_BLK_OFF *blk_off_saved,
	const void *dat, size_t len);

/*--------------------------------- in zdb_key.c -----------------------------*/

/**
 * 同步键存储头至磁盘
 * @param store {ZDB_KEY_STORE*} 值存储句柄
 * @return {int} -1: error, > 0 ok
 */
ACL_API int key_store_header_sync(ZDB_KEY_STORE *store);

/**
 * 根据键值打开键存储
 * @param db {ZDB*}
 * @param key {zdb_key_t} 键值
 * @return {ZDB_KEY_STORE*} !NULL: ok; NULL: error
 */
ACL_API ZDB_KEY_STORE *zdb_key_store_open(ZDB *db, zdb_key_t key);

/**
 * 根据文件名打开键存储
 * @param db {ZDB*}
 * @param filepath {const char*} 文件名
 * @return {ZDB_KEY_STORE*} !NULL: ok; NULL: error
 */
ACL_API ZDB_KEY_STORE *zdb_key_store_open2(ZDB *db, const char *filepath);

/**
 * 关闭键存储
 * @param store {ZDB_KEY_STORE*} 键存储句柄
 */
ACL_API void zdb_key_store_close(ZDB_KEY_STORE *store);

/**
 * 设置键存储中键位置的值位置值
 * @param db {ZDB*}
 * @param key {zdb_key_t}
 * @param blk_off {const ZDB_BLK_OFF*}
 * @return {int} 0: ok; -1: error
 */
ACL_API int zdb_key_set(ZDB *db, zdb_key_t key, const ZDB_BLK_OFF *blk_off);

/**
 * 根据键值从键存储中取得该键所对应的数据索引位置
 * @param db {ZDB*}
 * @param key {zdb_key_t} 键值
 * @param blk_off {ZDB_BLK_OFF*} 存储结果的对象
 * @return {int} 1: 表示查到, 0: 表示未查到, -1: 表示出错
 */
ACL_API int zdb_key_get(ZDB *db, zdb_key_t key, ZDB_BLK_OFF *blk_off);

/**
 * 查询键存储头的状态
 * @param db {ZDB*}
 * @param filepath {const char*} 键存储文件全路径
 * @param key_hdr {ZDB_KEY_HDR*} 用来存放键存储头信息的内存地址
 * @return {int} 0: ok; -1: error
 */
ACL_API int zdb_key_status(ZDB *db, const char *filepath, ZDB_KEY_HDR *key_hdr);

/**
 * 遍历键存储中的键的状态
 * @param db {ZDB*}
 * @param filepath {const char*} 键存储文件全路径
 * @param key_hdr {ZDB_KEY_HDR*} 若非空则用来存放键存储头信息的内存地址
 * @return {int} 0: ok; -1: error
 */
ACL_API int zdb_key_check3(ZDB *db, const char *filepath, ZDB_KEY_HDR *key_hdr);
ACL_API int zdb_key_check(ZDB_KEY_STORE *store, ZDB_KEY_HDR *key_hdr);

/**
 * 初始化键存储
 * @param db {ZDB*}
 * @param key_begin {zdb_key_t} 起始键值
 * @param key_end {zdb_key_t} 结束键值
 * @return {int} 0: ok; -1: error
 */
ACL_API int zdb_key_init(ZDB *db, zdb_key_t key_begin, zdb_key_t key_end);
/*--------------------------------- in zdb_dat.c -----------------------------*/

/**
 * 同步值存储头至磁盘
 * @param store {ZDB_DAT_STORE*} 值存储句柄
 * @return {int} -1: error, > 0 ok
 */
ACL_API int dat_store_header_sync(ZDB_DAT_STORE *store);

/**
 * 关闭值存储
 * @param store {ZDB_DAT_STORE*} 值存储句柄
 */
ACL_API void zdb_dat_store_close(ZDB_DAT_STORE *store);

/**
 * 打开或创建值存储
 * @param filepath {const char*} 值存储文件全路径
 * @return {ZDB_DAT_STORE*} !NULL: ok; NULL: error
 */
ACL_API ZDB_DAT_STORE *zdb_dat_store_open(ZDB *db, const char *filepath);

/**
 * 从值存储中取得对应的数据
 * @param db {ZDB*}
 * @param blk_off {const ZDB_BLK_OFF*}
 * @param key {zdb_key_t*} 存储键的值
 * @param size {size_t*} 存储数据的长度
 * @return {void*} 数据地址，若找到则不为空，找不到或出错则为空, 若不为空,
 *  则需要用 zdb_blk_free()/1 进行释放
 * 注意:
 *  当 size 指针地址非空时，调用者的类型必须是 size_t 类型而非 int 类型，因为在
 *  64位机上 size_t 为8个字节长度，而 int 为4个字节长度，内部在对 size 地址赋值
 *  时，编译器会自动按 size_t 的真实空间长度赋值，如果调用者的 size 为 int 类型，
 *  则就会出现空间越办现象，从而会使整个程序出现莫名其妙的问题，并且用 valgrind
 *  也查不出这个错误来!
 */
ACL_API ZDB_BLK *zdb_dat_get(ZDB *db, const ZDB_BLK_OFF *blk_off,
	zdb_key_t *key, size_t *size);

/**
 * 向值存储中添加新数据
 * @param db {ZDB*}
 * @param key {zdb_key_t} 键值
 * @param dat {const void*} 数据地址
 * @param len {size_t} dat 数据长度
 * @return {int} 0: 数据没有更新; 1: 数据更新; -1: 出错
 */
ACL_API int zdb_dat_add(ZDB *db, zdb_key_t key, const void *dat, int len);

/**
 * 更新值存储中的数据信息
 * @param db {ZDB*}
 * @param key {zdb_key_t} 键
 * @param blk_off {const ZDB_BLK_OFF*} 存储于键存储中相应值的位置信息
 * @param dat {const void*} 数据地址
 * @param len {size_t} dat 数据长度
 * @return {int} 0: 数据没有更新; 1: 数据更新; -1: 出错
 */
ACL_API int zdb_dat_update(ZDB *db, zdb_key_t key, const ZDB_BLK_OFF *blk_off,
	const void *dat, size_t len);

/**
 * 读取值存储的头信息
 * @param db {ZDB*}
 * @param filepath {const char*} 值存储文件名
 * @param dat_hdr {ZDB_DAT_HDR*} 存储结果
 * @retur {int} 0: ok; -1: error
 */
ACL_API int zdb_dat_stat(ZDB *db, const char *filepath, ZDB_DAT_HDR *dat_hdr);

/**
 * 检查值存储中的数据块的正确性
 * @param db {ZDB*}
 * @param filepath {const char*} 值存储文件名
 * @param dat_hdr {ZDB_DAT_HDR*} 若非空则存储值存储头信息
 * @return {int} 0: ok; -1: error
 */
ACL_API int zdb_dat_check3(ZDB *db, const char *filepath, ZDB_DAT_HDR *dat_hdr);
ACL_API int zdb_dat_check(ZDB_DAT_STORE *store, ZDB_DAT_HDR *dat_hdr);

/*--------------------   in zdb_key_walk.c -----------------------------------*/
ACL_API int zdb_key_walk(ZDB *db, int (*walk_fn)(ZDB_KEY_STORE *store));

/*--------------------   in zdb_dat_walk.c -----------------------------------*/
ACL_API int zdb_dat_walk(ZDB *db, int (*walk_fn)(ZDB_DAT_STORE *store));

/*--------------------   in zdb_key_iter.c -----------------------------------*/

/*--------------------   in zdb_dat_iter.c -----------------------------------*/
/**
 * 设置值存储的迭代器
 * @param store {ZDB_DAT_STORE*}
 * @param read_data {int} 是否需要读取数据块中的数据部分, !0: 表示读取数据块中的
 *  数据部分; 0: 仅读取数据块中的头
 */
ACL_API void zdb_dat_iter_set(ZDB_DAT_STORE *store, int read_data);

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif
