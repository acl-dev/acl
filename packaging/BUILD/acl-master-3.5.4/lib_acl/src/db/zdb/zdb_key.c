#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include <string.h>
#include "init/acl_init.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_debug.h"
#include "stdlib/acl_fhandle.h"
#include "stdlib/acl_mymalloc.h"
#include "db/zdb.h"

#endif

#ifndef ACL_CLIENT_ONLY

#include "zdb_private.h"

int key_store_header_sync(ZDB_KEY_STORE *store)
{
	const char *myname = "key_store_header_sync";
	int   ret;

	ret = ZDB_WRITE((ZDB_STORE*) store, &store->hdr, sizeof(store->hdr), 0);
	if (ret == -1)
		acl_msg_error("%s(%d): zdb_write to %s error(%s)",
			myname, __LINE__, STORE_PATH((ZDB_STORE*) store), acl_last_serror());
	return (ret);
}

static void dummy(void *ptr acl_unused)
{

}

static void free_tls(void *ptr)
{
	acl_myfree(ptr);
}

static void *__tls = NULL;
#ifndef HAVE_NO_ATEXIT
static void main_free_tls(void)
{
	if (__tls) {
		acl_myfree(__tls);
		__tls = NULL;
	}
}
#endif

static acl_pthread_key_t  once_key;
static void once_init(void)
{
	if ((unsigned long) acl_pthread_self() == acl_main_thread_self()) {
		acl_pthread_key_create(&once_key, dummy);
#ifndef HAVE_NO_ATEXIT
		atexit(main_free_tls);
#endif
	} else
		acl_pthread_key_create(&once_key, free_tls);
}

static acl_pthread_once_t once_control = ACL_PTHREAD_ONCE_INIT;
static ZDB_BLK_OFF *get_tls(void)
{
	ZDB_BLK_OFF *ptr;

	(void) acl_pthread_once(&once_control, once_init);
	ptr = (ZDB_BLK_OFF*) acl_pthread_getspecific(once_key);
	if (ptr == NULL) {
		ptr = (ZDB_BLK_OFF *) acl_mymalloc(sizeof(ZDB_BLK_OFF));
		acl_pthread_setspecific(once_key, ptr);
		if ((unsigned long) acl_pthread_self() == acl_main_thread_self())
			__tls = ptr;
	}
	return ptr;
}

/**
 * 获得迭代器头部数据
 * @param iter {ACL_ITER*} 迭代器指针
 * @return {const void*} 数据地址
 */
static const void *key_iter_head(ACL_ITER *iter, struct ZDB_KEY_STORE *store)
{
	zdb_off_t key_off;
	int   ret;
	ZDB_BLK_OFF *blk_off = get_tls();

	iter->key = NULL;
	iter->klen = 0;
	for (iter->i = 0; iter->i < store->hdr.key_limit; iter->i++) {
		key_off = KEY_OFF(((ZDB_STORE*) store)->db, iter->i);
		ret = ZDB_READ((ZDB_STORE*) store, blk_off,
				sizeof(ZDB_BLK_OFF), key_off);
		if (ret == -1) {
			iter->data = iter->ptr = NULL;
			return (NULL);
		}
		if (blk_off->offset >= 0 && blk_off->inode >= 0) {
			iter->data = iter->ptr = blk_off;
			iter->i++;  /* 指向下一个位置 */
			return (iter->ptr);
		}
	}

	iter->data = iter->ptr = NULL;
	return (NULL);
}

/**
 * 获得迭代器的那一个数据
 * @param iter {ACL_ITER*} 迭代器指针
 * @return {const void*} 数据地址
 */
static const void *key_iter_next(ACL_ITER *iter, struct ZDB_KEY_STORE *store)
{
	zdb_off_t key_off;
	int   ret;
	ZDB_BLK_OFF *blk_off= get_tls();

	for (; iter->i < store->hdr.key_limit; iter->i++) {
		key_off = KEY_OFF(((ZDB_STORE*) store)->db, iter->i);
		ret = ZDB_READ((ZDB_STORE*) store, blk_off,
				sizeof(ZDB_BLK_OFF), key_off);
		if (ret == -1) {
			iter->data = iter->ptr = NULL;
			return (NULL);
		}
		if (blk_off->offset >= 0 && blk_off->inode >= 0) {
			iter->data = iter->ptr = blk_off;
			iter->i++;  /* 指向下一个位置 */
			return (iter->ptr);
		}
	}

	iter->data = iter->ptr = NULL;
	return (NULL);
}

/**
 * 初始化键存储
 * @param store {ZDB_KEY_STORE*}
 * @return {int} 0: ok; -1: error
 */
static int key_store_init(ZDB_KEY_STORE *store)
{
	const char *myname = "key_store_init";
	zdb_key_t i;
	ZDB_BLK_OFF blk_off;
	int   ret;

	/* 初始化键存储的头部信息 */

	if ((((ZDB_STORE*) store)->db->oflags & ZDB_FLAG_CACHE_KEY) == 0)
		ret = acl_vstream_buffed_writen(((ZDB_STORE*) store)->fhandle.fp,
				&store->hdr, sizeof(store->hdr));
	else
		ret = ZDB_WRITE((ZDB_STORE*) store, &store->hdr, sizeof(store->hdr), 0);

	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): write header to %s error(%s)",
			myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
			acl_last_serror());
		return (-1);
	}

	blk_off.offset = -1;
	blk_off.inode = -1;

	if ((((ZDB_STORE*) store)->db->oflags & ZDB_FLAG_CACHE_KEY) == 0) {
		for (i = 0; i < store->hdr.key_limit; i++) {
			ret = acl_vstream_buffed_writen(((ZDB_STORE*) store)->fhandle.fp,
					&blk_off, sizeof(blk_off));
			if (ret == ACL_VSTREAM_EOF) {
				acl_msg_error("%s(%d): write to %s error(%s)",
					myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
					acl_last_serror());
				return (-1);
			}
		}
	} else {
		for (i = 0; i < store->hdr.key_limit; i++) {
			ret = ZDB_WRITE((ZDB_STORE*) store, &blk_off, sizeof(blk_off),
					(zdb_off_t) sizeof(store->hdr) +
					(zdb_off_t) sizeof(blk_off) * i);
			if (ret == ACL_VSTREAM_EOF) {
				acl_msg_error("%s(%d): write to %s error(%s)",
					myname, __LINE__,
					STORE_PATH((ZDB_STORE*) store),
					acl_last_serror());
				return (-1);
			}
		}
	}

	if ((((ZDB_STORE*) store)->db->oflags & ZDB_FLAG_CACHE_KEY) == 0) {
		/* 刷新写的缓冲区至磁盘 */
		if (acl_vstream_fflush(((ZDB_STORE*) store)->fhandle.fp)
				== ACL_VSTREAM_EOF)
		{
			acl_msg_error("%s(%d): fflush to %s error %s", myname,
				__LINE__, STORE_PATH((ZDB_STORE*) store),
				acl_last_serror());
			return (-1);
		}
	}
	return (0);
}

/**
 * 打开键存储时的回调函数
 * @param fh {ACL_FHANDLE*} 新打开的文件句柄
 * @param arg {void*} 参数
 * @return {int} 0: ok; -1: error, 若返回 -1 则新打开的文件句柄会自动被关闭
 */
static int key_store_on_open(ACL_FHANDLE *fh, void *arg)
{
	const char *myname = "key_store_on_open";
	ZDB *db = (ZDB*) arg;
	ZDB_KEY_STORE *store = (ZDB_KEY_STORE*) fh;
	int   ret;

	if (fh->size != sizeof(ZDB_KEY_STORE))
		acl_msg_fatal("%s: fh->size(%d) != ZDB_KEY_STORE's size(%d)",
			myname, (int) fh->size, (int) sizeof(ZDB_KEY_STORE));
	((ZDB_STORE*) store)->db = db;
	((ZDB_STORE*) store)->flag = STORE_FLAG_KEY;
	if ((db->oflags & ZDB_FLAG_SLICE_KEY))
		((ZDB_STORE*) store)->flag |= STORE_FLAG_IO_SLICE;

	/* 如果是新文件则初始化 */

	if (fh->fsize == 0) {
		acl_debug(ZDB_DBG_KEY, 1) ("%s: begin init %s ...", myname, STR(db->path_tmp));
		store->hdr.key_limit = db->key_limit;
		store->hdr.key_count = 0;
		store->hdr.key_begin = 0;

		if ((db->oflags & ZDB_FLAG_CACHE_KEY) != 0) {
			((ZDB_STORE*) store)->cache_max = db->key_cache_max;
			((ZDB_STORE*) store)->cache_timeout = db->key_cache_timeout;
			((ZDB_STORE*) store)->wback_max = db->key_wback_max;
			zdb_io_cache_open((ZDB_STORE*) store, sizeof(ZDB_BLK_OFF));
		}

		if (key_store_init(store) < 0) {
			acl_msg_error("%s(%d): key_store_init error", myname,  __LINE__);
			return (-1);
		}
		acl_debug(ZDB_DBG_KEY, 1) ("%s: init %s ok", myname, STR(db->path_tmp));
		return (0);
	}
	
	/* 如果是旧文件则读取文件头信息, 且应进行检验 */

	if ((((ZDB_STORE*) store)->db->oflags & ZDB_FLAG_CACHE_KEY) == 0)
		ret = acl_vstream_readn(((ZDB_STORE*) store)->fhandle.fp,
			       	&store->hdr, sizeof(store->hdr));
	else
		ret = ZDB_READ((ZDB_STORE*) store, &store->hdr,
				sizeof(store->hdr), 0);
	
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): read key header from %s error(%s)",
			myname, __LINE__, STR(db->path_tmp), acl_last_serror());
		return (-1);
	}

	if ((db->oflags & ZDB_FLAG_CACHE_KEY) != 0) {
		((ZDB_STORE*) store)->cache_max = db->key_cache_max;
		((ZDB_STORE*) store)->cache_timeout = db->key_cache_timeout;
		((ZDB_STORE*) store)->wback_max = db->key_wback_max;
		zdb_io_cache_open((ZDB_STORE*) store, sizeof(ZDB_BLK_OFF));
	}

	return (0);
}

/**
 * 关闭键存储时的回调函数
 * @param fh {ACL_FHANDLE*} 文件句柄
 */
static void key_store_on_close(ACL_FHANDLE *fh)
{
	const char *myname = "key_store_on_close";
	ZDB_KEY_STORE *store = (ZDB_KEY_STORE*) fh;

	key_store_header_sync(store);
	zdb_io_cache_close((ZDB_STORE*) store);
	acl_debug(ZDB_DBG_KEY, 2) ("%s(%d): sync header ok, close %s now, nrefer: %d",
		myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
		((ZDB_STORE*) store)->fhandle.nrefer);
}

ZDB_KEY_STORE *zdb_key_store_open2(ZDB *db, const char *filepath)
{
	const char *myname = "zdb_key_store_open2";
	ZDB_KEY_STORE *store;
	unsigned int oflags = ACL_FHANDLE_O_NOATIME;

	if ((db->oflags & ZDB_FLAG_OPEN_LOCK) != 0)
		oflags |= ACL_FHANDLE_O_MLOCK;

	/* 打开或创建一个文件句柄 */
	store = (ZDB_KEY_STORE*) acl_fhandle_open(sizeof(ZDB_KEY_STORE), oflags,
			filepath, key_store_on_open, db,
			key_store_on_close);
	if (store == NULL) {
		acl_msg_error("%s(%d): acl_fhandle_open %s error(%s)",
			myname, __LINE__, filepath, acl_last_serror());
		return (NULL);
	} else {
		((ZDB_STORE*) store)->iter_head = (STORE_ITER) key_iter_head;
		((ZDB_STORE*) store)->iter_next = (STORE_ITER) key_iter_next;
	}
	return (store);
}


ZDB_KEY_STORE *zdb_key_store_open(ZDB *db, zdb_key_t key)
{
	const char *myname = "zdb_key_store_open";
	int   inode;

	/* 计算出该 key 所在的存储文件的存储目录的标识号 */
	inode = KEY_INODE(db, key);
	if (inode < 0 || inode > 65353) {
		acl_msg_error("%s(%d): inode(%d) invalid, key(" ACL_FMT_I64D ")",
			myname, __LINE__, inode, key);
		return (NULL);
	}

#ifdef	INCLUDE_PATH
	acl_vstring_sprintf(db->path_tmp, "%s/%d/%s_%d.key",
		db->key_path, inode, db->dbname, inode);
#else
	acl_vstring_sprintf(db->path_tmp, "%s/%s_%d.key",
		db->key_path, db->dbname, inode);
#endif

	return (zdb_key_store_open2(db, STR(db->path_tmp)));
}

void zdb_key_store_close(ZDB_KEY_STORE *store)
{
	acl_fhandle_close(&((ZDB_STORE*) store)->fhandle, 120);
}

int zdb_key_set(ZDB *db, zdb_key_t key, const ZDB_BLK_OFF *blk_off)
{
	const char *myname = "zdb_key_set";
	zdb_off_t key_off;
	ZDB_KEY_STORE *store = NULL;
	int   ret, inode;

#undef	RETURN
#define	RETURN(x) do {  \
	if (store)  \
		zdb_key_store_close(store);  \
	return ((x));  \
} while (0)

	store = zdb_key_store_open(db, key);
	if (store == NULL) {
		acl_msg_error("%s(%d): open key(" ACL_FMT_I64D ") store error",
			myname, __LINE__, key);
		RETURN (-1);
	}

	/* 计算 key 在键存储中的位置 */
	
	inode = KEY_INODE(store->store.db, key);
	if (inode < 0 || inode > 65353) {
		acl_msg_error("%s(%d): %s, inode(%d) invalid, key("
			ACL_FMT_I64D ")", myname, __LINE__,
			STORE_PATH(&store->store), inode, key);
		RETURN (-1);
	}

	key_off = KEY_OFF(store->store.db, key);
	if (key_off < (zdb_off_t) sizeof(ZDB_KEY_HDR)) {
		acl_msg_error("%s(%d): %s, key_off(" ACL_FMT_I64D
			") < ZDB_KEY_HDR's size(%d), key(" ACL_FMT_I64D
			"), inode(%d), key_limit(" ACL_FMT_I64D ")",
			myname, __LINE__, STORE_PATH(&store->store), key_off,
			(int) sizeof(ZDB_KEY_HDR), key, inode, store->store.db->key_limit);
		RETURN (-1);
	}

	ret = ZDB_WRITE((ZDB_STORE*) store, blk_off, sizeof(ZDB_BLK_OFF), key_off);
	if (ret == -1) {
		acl_msg_error("%s(%d): write to %s error %s, blk_off("
			ACL_FMT_I64D ", %d), key(" ACL_FMT_I64D ")",
			myname, __LINE__, STORE_PATH(&store->store), acl_last_serror(),
			blk_off->offset, blk_off->inode, key);
		RETURN (-1);
	}

	/* 如果为新数据插入则增加计数器 */

	if ((db->status & ZDB_STAT_KEY_NEW))
		store->hdr.key_count++;
	RETURN (0);
}

int zdb_key_get(ZDB *db, zdb_key_t key, ZDB_BLK_OFF *blk_off)
{
	const char *myname = "zdb_key_get";
	zdb_off_t key_off;
	ZDB_KEY_STORE *store = NULL;
	int   ret, inode;

#undef	RETURN
#define	RETURN(x) do {  \
	if (store)  \
		zdb_key_store_close(store);  \
	return ((x));  \
} while (0)

	store = zdb_key_store_open(db, key);
	if (store == NULL) {
		acl_msg_error("%s(%d): open key(" ACL_FMT_I64D ") store error",
			myname, __LINE__, key);
		RETURN (-1);
	}

	/* 计算 key 在键存储中的位置 */

	inode = KEY_INODE(store->store.db, key);
	if (inode < 0 || inode > 65353) {
		acl_msg_error("%s(%d): %s, inode(%d) invalid, key("
			ACL_FMT_I64D ")", myname, __LINE__,
			STORE_PATH(&store->store), inode, key);
		RETURN (-1);
	}

	key_off = KEY_OFF(store->store.db, key);
	if (key_off < (int) sizeof(ZDB_KEY_HDR)) {
		acl_msg_error("%s(%d): %s, key_off(" ACL_FMT_I64D
			") < ZDB_KEY_HDR's size(%d), key(" ACL_FMT_I64D
			"), inode(%d), key_limit(" ACL_FMT_I64D ")",
			myname, __LINE__, STORE_PATH(&store->store), key_off,
			(int) sizeof(ZDB_KEY_HDR), key, inode, store->store.db->key_limit);
		RETURN (-1);
	}

	ret = ZDB_READ((ZDB_STORE*) store, blk_off, sizeof(ZDB_BLK_OFF), key_off);

	acl_debug(ZDB_DBG_GETK, 2)
		("%s(%d): zdb_read ret: %d, blk_off: " ACL_FMT_I64D
		", key_off: " ACL_FMT_I64D,
		myname, __LINE__, ret, blk_off->offset, key_off);

	if (ret == -1) {
		acl_msg_error("%s(%d): zdb_read from %s error %s,"
			" key(" ACL_FMT_I64D "), key_off(" ACL_FMT_I64D ")",
			myname, __LINE__, STORE_PATH(&store->store),
			acl_last_serror(), key, key_off);
		RETURN (-1);
	}

	if (blk_off->offset < 0 || blk_off->inode < 0) {
		acl_debug(ZDB_DBG_GETK, 2)
			("%s(%d): blk_off(" ACL_FMT_I64D ") from %s invalid for"
			 " key(" ACL_FMT_I64D "), key_off(" ACL_FMT_I64D ")",
			myname, __LINE__, blk_off->offset,
			STORE_PATH(&store->store), key, key_off);
		RETURN (0);
	}

	/* 只有 blk_off->offset >= 0 && blk_off->inode >= 0 时才表明找到值位置索引 */
	RETURN (1);
}

int zdb_key_status(ZDB *db, const char *filepath, ZDB_KEY_HDR *key_hdr)
{
	const char *myname = "zdb_key_status";
	ZDB_KEY_STORE *store;

	store = zdb_key_store_open2(db, filepath);
	if (store == NULL) {
		acl_msg_error("%s(%d): open %s error(%s)",
			myname, __LINE__, filepath, acl_last_serror());
		return (-1);
	}

	memcpy(key_hdr, &store->hdr, sizeof(ZDB_KEY_HDR));
	zdb_key_store_close(store);
	return (0);
}

int zdb_key_check(ZDB_KEY_STORE *store, ZDB_KEY_HDR *key_hdr)
{
	const char *myname = "zdb_key_check";
	acl_int64 nused = 0;
	int   failed = 0;
	ACL_ITER iter;

	if (key_hdr)
		memcpy(key_hdr, &store->hdr, sizeof(ZDB_KEY_HDR));

	/*  扫描所有被使用的数据结点 */

	acl_msg_info("%s(%d): begin check %s's used key",
		myname, __LINE__, STORE_PATH((ZDB_STORE*) store));

	acl_foreach(iter, (ZDB_STORE*) store) {
		nused++;
	}

	if (nused != store->hdr.key_count) {
		acl_msg_error("%s(%d): nused(" ACL_FMT_I64D
			") != store->hdr.key_count(" ACL_FMT_I64D ")"
			" for %s, please repair it!", myname, __LINE__,
			nused, store->hdr.key_count, STORE_PATH(&store->store));
		failed = 1;
	} else {
		acl_msg_info("%s(%d): check used key ok for %s,"
			" store->hdr.key_count(" ACL_FMT_I64D ")", myname, __LINE__,
			STORE_PATH(&store->store), store->hdr.key_count);
	}
	return (failed ? -1 : 0);
}

int zdb_key_check3(ZDB *db, const char *filepath, ZDB_KEY_HDR *key_hdr)
{
	const char *myname = "zdb_key_check3";
	ZDB_KEY_STORE *store;
	int   ret;

	store = zdb_key_store_open2(db, filepath);
	if (store == NULL) {
		acl_msg_error("%s(%d): open %s error(%s)",
			myname, __LINE__, filepath, acl_last_serror());
		return (-1);
	}

	ret = zdb_key_check(store, key_hdr);
	zdb_key_store_close(store);
	return (ret);
}

static int store_init_on_open(ACL_FHANDLE *fh, void *arg)
{
	const char *myname = "store_init_on_open";
	ZDB_KEY_STORE *store = (ZDB_KEY_STORE*) fh;
	ZDB *db = (ZDB*) arg;
	ZDB_BLK_OFF blk_off;
	zdb_key_t key;
	int   ret;

	if (fh->size != sizeof(ZDB_KEY_STORE))
		acl_msg_fatal("%s: fh->size(%d) != ZDB_KEY_STORE's size(%d)",
			myname, (int) fh->size, (int) sizeof(ZDB_KEY_STORE));
	((ZDB_STORE*) store)->db = db;
	((ZDB_STORE*) store)->flag = STORE_FLAG_KEY;
	if ((db->oflags & ZDB_FLAG_SLICE_KEY))
		((ZDB_STORE*) store)->flag |= STORE_FLAG_IO_SLICE;

	acl_debug(ZDB_DBG_KEY, 1) ("%s: begin init %s ...", myname, STR(db->path_tmp));
	store->hdr.key_limit = db->key_limit;
	store->hdr.key_count = 0;
	store->hdr.key_begin = 0;

	/* 初始化键存储的头部信息 */
	ret = acl_vstream_buffed_writen(((ZDB_STORE*) store)->fhandle.fp,
			&store->hdr, sizeof(store->hdr));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): write header to %s error(%s)",
			myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
			acl_last_serror());
		return (-1);
	}

	blk_off.offset = -1;
	blk_off.inode = -1;
	for (key = 0; key < store->hdr.key_limit; key++) {
		ret = acl_vstream_buffed_writen(((ZDB_STORE*) store)->fhandle.fp,
				&blk_off, sizeof(blk_off));
		if (ret == ACL_VSTREAM_EOF) {
			acl_msg_error("%s(%d): write to %s error(%s)", myname,
				__LINE__, STORE_PATH((ZDB_STORE*) store),
				acl_last_serror());
			return (-1);
		}
		if ((key % 5000000) == 0) {
			acl_msg_info("%s(%d): %s, key: " ACL_FMT_I64D ", key_limit: "
				ACL_FMT_I64D, myname, __LINE__,
				STORE_PATH((ZDB_STORE*) store), key, db->key_limit);
		}
	}

	/* 刷新写的缓冲区至磁盘 */
	if (acl_vstream_fflush(((ZDB_STORE*) store)->fhandle.fp) == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): fflush to %s error %s", myname, __LINE__,
			STORE_PATH((ZDB_STORE*) store), acl_last_serror());
		return (-1);
	}


	acl_debug(ZDB_DBG_KEY, 1) ("%s: init %s ok", myname, STR(db->path_tmp));
	return (0);
}

static void store_init_on_close(ACL_FHANDLE *fh)
{
	ZDB_KEY_STORE *store = (ZDB_KEY_STORE*) fh;

	key_store_header_sync(store);
	zdb_io_cache_close((ZDB_STORE*) store);
}

int zdb_key_init(ZDB *db, zdb_key_t key_begin, zdb_key_t key_end)
{
	const char *myname = "zdb_key_init";
	ZDB_KEY_STORE *store;
	zdb_key_t key;
	int   inode;

	acl_msg_info("%s(%d): key_begin: " ACL_FMT_I64D ", key_end: "
		ACL_FMT_I64D ", key_limit: " ACL_FMT_I64D,
		myname, __LINE__, key_begin, key_end, db->key_limit);

	for (key = key_begin; key < key_end;) {
		inode = KEY_INODE(db, key);
		acl_vstring_sprintf(db->path_tmp, "%s/%s_%d.key",
			db->key_path, db->dbname, inode);
		/* 打开或创建一个文件句柄 */
		acl_msg_info("%s(%d): open %s", myname, __LINE__, STR(db->path_tmp));
		store = (ZDB_KEY_STORE*) acl_fhandle_open(sizeof(ZDB_KEY_STORE),
				0, STR(db->path_tmp), store_init_on_open,
				db, store_init_on_close);
		if (store == NULL) {
			acl_msg_error("%s(%d): acl_fhandle_open %s error(%s)",
				myname, __LINE__, STR(db->path_tmp),
				acl_last_serror());
			return (-1);
		}

		acl_msg_info("%s(%d): close %s", myname, __LINE__, STR(db->path_tmp));
		acl_fhandle_close(&((ZDB_STORE*) store)->fhandle, 0);
		key += db->key_limit;
	}

	return (0);
}

#endif /* ACL_CLIENT_ONLY */
