#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include <string.h>
#include <stdio.h>
#include "stdlib/acl_msg.h"
#include "stdlib/acl_debug.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_meter_time.h"
#include "stdlib/acl_sane_basename.h"
#include "thread/acl_pthread.h"
#include "db/zdb.h"

#endif

#ifndef ACL_CLIENT_ONLY

#include "zdb_private.h"

int dat_store_header_sync(ZDB_DAT_STORE *store)
{
	const char *myname = "dat_store_header_sync";
	int   ret;

	ret = ZDB_WRITE((ZDB_STORE*) store, &store->hdr, sizeof(store->hdr), 0);
	if (ret == -1)
		acl_msg_error("%s(%d): zdb_write to %s error(%s)", myname,
			__LINE__, STORE_PATH(&store->store), acl_last_serror());
	return (ret);
}

/**
 * 释放 ACL_VSTRING 回调函数
 * @param arg {void*} 回调参数, 可转换成 ACL_VSTRING 对象
 */
static void free_vstring_fn(void *arg)
{
	ACL_VSTRING *s = (ACL_VSTRING*) arg;

	acl_vstring_free(s);
}

/**
 * 确保 ZDB.dat_ifiles 的数组容量够用
 * @param db {ZDB*}
 * @param size {int} 数组容量的大小要求
 */
static void dat_ifiles_space(ZDB *db, int idisk, int size)
{
	int   i;

	/* xxx: 只所以加1是因为下标是从1开始的, 而 C 语言中的数组下载是从 0 开始的,
	 * 所以说 db->dat_disks[0] 是给浪费掉了 :( --- zsx
	 */
	size++;

	if (db->dat_disks[idisk].dat_ifiles == NULL) {
		if (size < 16)
			size = 16;
		db->dat_disks[idisk].dat_ifiles = (int*)
			acl_mycalloc(size, sizeof(int));
		db->dat_disks[idisk].dat_ifiles_size = size;
		return;
	}

	if (size < db->dat_disks[idisk].dat_ifiles_size)
		return;
	db->dat_disks[idisk].dat_ifiles = (int*)
		acl_myrealloc(db->dat_disks[idisk].dat_ifiles, size * sizeof(int));

	for (i = db->dat_disks[idisk].dat_ifiles_size; i < size; i++)
		db->dat_disks[idisk].dat_ifiles[i] = 0;
	db->dat_disks[idisk].dat_ifiles_size = size;
}

/**
 * 计算值存储的相对路径号
 * @param db {ZDB*}
 * @param len {int} 数据的长度
 * @return {int} >= 0: ok; -1: error
 */
static int dat_inode(ZDB *db, int len)
{
	const char *myname = "dat_inode";
	int  inode;

	/* 所给的长度必须为基础块的整数倍 */

	if (len % db->blk_dlen != 0) {
		acl_msg_error("%s(%d): len(%d) %% db->blk_dlen(%d) != 0, invalid",
			myname, __LINE__, len, db->blk_dlen);
		return (-1);
	}

	/* 根据数据块长度计算出基础块的个数, 同时也是存储文件所在目录的标识号 */

	inode = len / db->blk_dlen;
	if (inode > DIR_LIMIT) {
		acl_msg_error("%s(%d): inode(%d) > %u, too large",
			myname, __LINE__, inode, DIR_LIMIT);
		return (-1);
	}

	return (inode);
}

/**
 * 获得值存储的文件全路径
 * @param db {ZDB*}
 * @param buf {ACL_VSTRING*} 存储结果的缓冲区
 * @param inode {int} 路径号
 * @param ifile {int} 文件号
 * @return {ACL_VSTRING*} 结果缓冲区地址
 **/
static ACL_VSTRING *dat_filepath(ZDB *db, ACL_VSTRING *buf, int idisk, int inode, int ifile)
{
	static acl_pthread_key_t buf_key = (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES;
	ACL_VSTRING *buf_safe;
	static ACL_VSTRING *__buf_unsafe = NULL;

	buf_safe = (ACL_VSTRING*) acl_pthread_tls_get(&buf_key);
	if (buf_safe == NULL) {
		if (buf_key == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES) {
			if (__buf_unsafe == NULL)
				__buf_unsafe = acl_vstring_alloc(256);
			buf_safe = __buf_unsafe;
		} else {
			buf_safe = acl_vstring_alloc(256);
			acl_pthread_tls_set(buf_key, buf_safe, free_vstring_fn);
		}
	}

	if (buf == NULL)
		buf = buf_safe;

#ifdef	INCLUDE_PATH
	acl_vstring_sprintf(buf, "%s/%d/%s_%d_%d.dat",
		db->dat_disks[idisk].path, inode, db->dbname, inode, ifile);
#else
	acl_vstring_sprintf(buf, "%s/%s_%d_%d.dat",
		db->dat_disks[idisk].path, db->dbname, inode, ifile);
#endif
	return (buf);
}

/**
 * 获得文件的所在路径
 * @param db {ZDB*}
 * @param buf {ACL_VSTRING*} 存储结果的缓冲区
 * @param inode {int} 路径号
 * @return {ACL_VSTRING*} 结果缓冲区地址
 */
static ACL_VSTRING *dat_path(ZDB *db, ACL_VSTRING *buf, int idisk, int inode acl_unused)
{
	static acl_pthread_key_t buf_key = (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES;
	ACL_VSTRING *buf_safe;
	static ACL_VSTRING *__buf_unsafe = NULL;

	buf_safe = (ACL_VSTRING*) acl_pthread_tls_get(&buf_key);
	if (buf_safe == NULL) {
		if (buf_key == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES) {
			if (__buf_unsafe == NULL)
				__buf_unsafe = acl_vstring_alloc(256);
			buf_safe = __buf_unsafe;
		} else {
			buf_safe = acl_vstring_alloc(256);
			acl_pthread_tls_set(buf_key, buf_safe, free_vstring_fn);
		}
	}

	if (buf == NULL)
		buf = buf_safe;

#ifdef  INCLUDE_PATH
	acl_vstring_sprintf(buf, "%s/%d", db->dat_disks[idisk].path, inode);
#else
	acl_vstring_sprintf(buf, "%s", db->dat_disks[idisk].path);
#endif

	return (buf);
}

#ifdef	ZDB_LINK_BUSY

/**
 * 将新的占用块链接到占用链中
 * @param store {ZDB_DAT_STORE*}
 * @param blk {ZDB_BLK*}
 * @parem ilnk {zdb_lnk_t}
 * @return {int} 0: ok; -1: error
 */
static int busy_blk_link(ZDB_DAT_STORE *store, ZDB_BLK *blk, zdb_lnk_t ilnk)
{
	const char *myname = "busy_blk_link";
	ZDB_BLK *hdr_blk = NULL;
	zdb_off_t off;
	int  ret, blk_len;

#undef	RETURN
#define	RETURN(x) do {  \
	if (hdr_blk)  \
		zdb_blk_free(hdr_blk);  \
	return (x);  \
} while (0)
	
	if (store->hdr.itail_busy == -1) {
		if (store->hdr.ihead_busy != -1) {
			acl_msg_error("%s(%d): %s, ihead_busy(%d) != -1",
				myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
				store->hdr.ihead_busy);
			RETURN (-1);
		}
		store->hdr.itail_busy = ilnk;
		store->hdr.ihead_busy = ilnk;
		blk->hdr.inext_busy = -1;
		blk->hdr.iprev_busy = -1;
		RETURN (0);
	} else if (store->hdr.ihead_busy == -1) {
		acl_msg_error("%s(%d): %s, ihead_busy(%d) invalid", myname,
			__LINE__, STORE_PATH((ZDB_STORE*) store),
			store->hdr.ihead_busy);
		RETURN (-1);
	}

	off = BLK_HDR_OFF(store, store->hdr.ihead_busy);
	if (off <= 0) {
		acl_msg_error("%s(%d): %s, off(" ACL_FMT_I64D ") invalid",
			myname, __LINE__, STORE_PATH((ZDB_STORE*) store), off);
		RETURN (-1);
	}

	blk_len = BLK_LEN(store);
	hdr_blk = (ZDB_BLK*) acl_mymalloc(blk_len);
	ret = ZDB_READ((ZDB_STORE*) store, hdr_blk, blk_len, off);
	if (ret == -1) {
		acl_msg_error("%s(%d): zdb_read %s error(%s)", myname, __LINE__,
			STORE_PATH((ZDB_STORE*) store), acl_last_serror());
		RETURN (-1);
	}

	if (hdr_blk->hdr.blk_ilnk != store->hdr.ihead_busy) {
		acl_msg_error("%s(%d): %s, blk_ilnk(%d) != ihead_busy(%d)",
			myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
			hdr_blk->hdr.blk_ilnk, store->hdr.ihead_busy);
		RETURN (-1);
	}

	hdr_blk->hdr.iprev_busy = ilnk;
	blk->hdr.inext_busy = store->hdr.ihead_busy;
	blk->hdr.iprev_busy = -1;
	store->hdr.ihead_busy = ilnk;  /* 更新占用块链头 */

	/* 更新某占用块的头信息 */
	ret = ZDB_WRITE((ZDB_STORE*) store, hdr_blk, blk_len, off);
	if (ret == -1) {
		acl_msg_error("%s(%d): zdb_write to %s error(%s)",
			myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
			acl_last_serror());
		RETURN (-1);
	}

	RETURN (0);
}

static int busy_blk_unlink(ZDB_DAT_STORE *store, ZDB_BLK *blk, zdb_lnk_t ilnk)
{
	const char *myname = "busy_blk_unlink";
	ZDB_BLK_HDR *blk_hdr = NULL;
	zdb_off_t off;
	int   ret;

#undef	RETURN
#define	RETURN(x) do {  \
	if (blk_hdr)  \
		acl_myfree(blk_hdr);  \
	return (x);  \
} while (0)

	if (ilnk < 0) {
		acl_msg_error("%s(%d): %s, ilnk(%d) invalid",
			myname, __LINE__, STORE_PATH((ZDB_STORE*) store), ilnk);
		RETURN (-1);
	}

	if (store->hdr.ihead_busy == -1) {
		acl_msg_error("%s(%d): ihead_busy(%d) invalid",
			myname, __LINE__, store->hdr.ihead_busy);
		RETURN (-1);
	} else if (store->hdr.itail_busy == -1) {
		acl_msg_error("%s(%d): itail_busy(%d) invalid",
			myname, __LINE__, store->hdr.itail_busy);
		RETURN (-1);
	}

	if (blk->hdr.iprev_busy == -1) {
		/* 该占用块应该是头部块 */

		if (store->hdr.ihead_busy != ilnk) {
			acl_msg_error("%s(%d): %s, ihead_busy(%d) != ilnk(%d)",
				myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
				store->hdr.ihead_busy, ilnk);
			RETURN (-1);
		}
		if (store->hdr.itail_busy == ilnk) {
			/* 说明该 blk 是最后一个占用块了 */

			store->hdr.ihead_busy = store->hdr.itail_busy = -1;
			blk->hdr.inext_busy = blk->hdr.iprev_busy = -1;
			RETURN (0);
		} else if (blk->hdr.inext_busy < 0) {
			acl_msg_error("%s(%d): %s, inext_busy(%d) invalid",
				myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
				blk->hdr.inext_busy);
			RETURN (-1);
		}

		blk_hdr = (ZDB_BLK_HDR*) acl_mymalloc(sizeof(ZDB_BLK_HDR));
		off = BLK_HDR_OFF(store, blk->hdr.inext_busy);
		ret = ZDB_READ((ZDB_STORE*) store, blk_hdr, sizeof(ZDB_BLK_HDR), off);
		if (ret == -1) {
			acl_msg_error("%s(%d): zdb_read %s error(%s)",
				myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
				acl_last_serror());
			RETURN (-1);
		} else if (blk_hdr->blk_ilnk != blk->hdr.inext_busy) {
			acl_msg_error("%s(%d): blk_ilnk(%d) != inext_busy(%d)",
				myname, __LINE__, blk_hdr->blk_ilnk,
				blk->hdr.inext_busy);
			RETURN (-1);
		}

		blk_hdr->iprev_busy = -1;
		ret = ZDB_WRITE((ZDB_STORE*) store, blk_hdr, sizeof(ZDB_BLK_HDR), off);
		if (ret == -1) {
			acl_msg_error("%s(%d): zdb_write %s error(%s)",
				myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
				acl_last_serror());
			RETURN (-1);
		}

		store->hdr.ihead_busy = blk_hdr->blk_ilnk;
		RETURN (0);
	}

	if (blk->hdr.inext_busy == -1) {
		/* 说明该占用块应该为尾部块 */

		if (store->hdr.itail_busy != ilnk) {
			acl_msg_error("%s(%d): %s, itail_busy(%d) != ilnk(%d)",
				myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
				store->hdr.itail_busy, ilnk);
			RETURN (-1);
		}

		/* xxx: sanity check */
		if (store->hdr.ihead_busy == ilnk) {
			/* 说明该 blk 是最后一个占用块了 */

			store->hdr.ihead_busy = store->hdr.itail_busy = -1;
			blk->hdr.inext_busy = blk->hdr.iprev_busy = -1;
			RETURN (0);
		} else if (blk->hdr.iprev_busy < 0) {
			acl_msg_error("%s(%d): %s, iprev_busy(%d) invalid",
				myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
				blk->hdr.iprev_busy);
			RETURN (-1);
		}

		blk_hdr = (ZDB_BLK_HDR*) acl_mymalloc(sizeof(ZDB_BLK_HDR));
		off = BLK_HDR_OFF(store, blk->hdr.iprev_busy);
		ret = ZDB_READ((ZDB_STORE*) store, blk_hdr, sizeof(ZDB_BLK_HDR), off);
		if (ret == -1) {
			acl_msg_error("%s(%d): zdb_read %s error(%s)",
				myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
				acl_last_serror());
			RETURN (-1);
		} else if (blk_hdr->blk_ilnk != blk->hdr.iprev_busy) {
			acl_msg_error("%s(%d): blk_ilnk(%d) != iprev_busy(%d)",
				myname, __LINE__, blk_hdr->blk_ilnk,
				blk->hdr.iprev_busy);
			RETURN (-1);
		}

		blk_hdr->inext_busy = -1;
		ret = ZDB_WRITE((ZDB_STORE*) store, blk_hdr, sizeof(ZDB_BLK_HDR), off);
		if (ret == -1) {
			acl_msg_error("%s(%d): zdb_write %s error(%s)",
				myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
				acl_last_serror());
			RETURN (-1);
		}

		store->hdr.itail_busy = blk_hdr->blk_ilnk;
		RETURN (0);
	}

	/* 说明该占用块应该为中间块 */

	/* 更新后一个占用块的头信息 */

	blk_hdr = (ZDB_BLK_HDR*) acl_mymalloc(sizeof(ZDB_BLK_HDR));
	off = BLK_HDR_OFF(store, blk->hdr.inext_busy);
	ret = ZDB_READ((ZDB_STORE*) store, blk_hdr, sizeof(ZDB_BLK_HDR), off);
	if (ret == -1) {
		acl_msg_error("%s(%d): zdb_read %s error(%s)",
			myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
			acl_last_serror());
		RETURN (-1);
	} else if (blk_hdr->blk_ilnk != blk->hdr.inext_busy) {
		acl_msg_error("%s(%d): blk_ilnk(%d) != inext_busy(%d)",
			myname, __LINE__, blk_hdr->blk_ilnk,
			blk->hdr.inext_busy);
		RETURN (-1);
	}

	blk_hdr->iprev_busy = blk->hdr.iprev_busy;
	ret = ZDB_WRITE((ZDB_STORE*) store, blk_hdr, sizeof(ZDB_BLK_HDR), off);
	if (ret == -1) {
		acl_msg_error("%s(%d): zdb_write %s error(%s)",
			myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
			acl_last_serror());
		RETURN (-1);
	}

	/* 更新前一个占用块的头信息 */

	off = BLK_HDR_OFF(store, blk->hdr.iprev_busy);
	ret = ZDB_READ((ZDB_STORE*)store, blk_hdr, sizeof(ZDB_BLK_HDR), off);
	if (ret == -1) {
		acl_msg_error("%s(%d): zdb_read %s error(%s)",
			myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
			acl_last_serror());
		RETURN (-1);
	} else if (blk_hdr->blk_ilnk != blk->hdr.iprev_busy) {
		acl_msg_error("%s(%d): blk_ilnk(%d) != iprev_busy(%d)",
			myname, __LINE__, blk_hdr->blk_ilnk,
			blk->hdr.iprev_busy);
		RETURN (-1);
	}

	blk_hdr->inext_busy = blk->hdr.inext_busy;
	ret = ZDB_WRITE((ZDB_STORE*) store, blk_hdr, sizeof(ZDB_BLK_HDR), off);
	if (ret == -1) {
		acl_msg_error("%s(%d): zdb_write %s error(%s)",
			myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
			acl_last_serror());
		RETURN (-1);
	}

	blk->hdr.inext_busy = blk->hdr.iprev_busy = 1;
	RETURN (0);
}

#endif  /* ZDB_LINK_BUSY */

/**
 * 增加并初始化值存储的数据块
 * @param store {ZDB_DAT_STORE*} 值存储句柄
 * @param count {acl_int64} 增加的数据块个数
 * @return {int} 0: ok; -1: error
 */
static int dat_store_blk_add(ZDB_DAT_STORE *store, acl_int64 count)
{
	const char *myname = "dat_store_blk_add";
	ZDB_BLK *blk;
	acl_int64 i;
	int   ret;

	/* 检查输入是否合法 */
	if (count <= 0) {
		acl_msg_error("%s(%d): count(" ACL_FMT_I64D ") invalid",
			myname, __LINE__, count);
		return (-1);
	}

	/* 检查是否已经达到分配限制个数 */
	if (store->hdr.size >= store->hdr.limit) {
		acl_msg_error("%s(%d): store(%s)'s size(" ACL_FMT_I64D
			") >= limit(" ACL_FMT_I64D ")",
			myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
			store->hdr.size, store->hdr.limit);
		return (-1);
	}

	count += store->hdr.size;  /* 将 count 设为最大值 */
	if (count > store->hdr.limit)
		count = store->hdr.limit;  /* 必须保证不能超过限制 */

	if ((((ZDB_STORE*) store)->db->oflags & ZDB_FLAG_CACHE_DAT) == 0) {
		/* 将文件指针置尾 */
		if (acl_vstream_fseek(((ZDB_STORE*) store)->fhandle.fp, 0, SEEK_END) < 0) {
			acl_msg_error("%s(%d): fseek %s error(%s)",
				myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
				acl_last_serror());
			return (-1);
		}
	}

	/* 计算真实的 ZDB_BLK 的空间大小并分配一个新的 ZDB_BLK 对象 */
	blk = (ZDB_BLK*) acl_mycalloc(1, (size_t) BLK_LEN(store));

	/* 顺序初始化值存储中的各个数据块, 并形成单向链 */
	
	if ((((ZDB_STORE*) store)->db->oflags & ZDB_FLAG_CACHE_DAT) == 0) {
		for (i = store->hdr.size; i < count; i++) {
			blk->hdr.inext_idle = (zdb_lnk_t) i + 1;
			/* 当达到最大值时表示下一个位置无效 */
			if (blk->hdr.inext_idle == count)
				blk->hdr.inext_idle = -1;
			blk->hdr.blk_ilnk = (zdb_lnk_t) i;  /* 标识自己的索引位置号, 校验用 */
			blk->hdr.inext_idle = store->hdr.ihead_idle;  /* 与链连接 */
			blk->hdr.key = -1;  /* 所有的初始键为 -1 */

#ifdef	ZDB_LINK_BUSY
			blk->hdr.inext_busy = -1;
			blk->hdr.iprev_busy = -1;
#endif

			store->hdr.ihead_idle = (zdb_lnk_t) i;  /* 更新值存储头的头空闲指针 */

			ret = acl_vstream_buffed_writen(((ZDB_STORE*) store)->fhandle.fp,
					blk, (size_t) BLK_LEN(store));
			if (ret == ACL_VSTREAM_EOF) {
				acl_myfree(blk);
				acl_msg_error("%s(%d): write to %s error(%s)", myname,
					__LINE__, STORE_PATH((ZDB_STORE*) store),
					acl_last_serror());
				return (-1);
			}
		}

	} else {
		for (i = store->hdr.size; i < count; i++) {
			blk->hdr.inext_idle = (zdb_lnk_t) i + 1;
			/* 当达到最大值时表示下一个位置无效 */
			if (blk->hdr.inext_idle == count)
				blk->hdr.inext_idle = -1;
			blk->hdr.blk_ilnk = (zdb_lnk_t) i;  /* 标识自己的索引位置号, 校验用 */
			blk->hdr.inext_idle = store->hdr.ihead_idle;  /* 与链连接 */
			blk->hdr.key = -1;  /* 所有的初始键为 -1 */

#ifdef	ZDB_LINK_BUSY
			blk->hdr.inext_busy = -1;
			blk->hdr.iprev_busy = -1;
#endif

			store->hdr.ihead_idle = (zdb_lnk_t) i;  /* 更新值存储头的头空闲指针 */

			ret = ZDB_WRITE((ZDB_STORE*) store, blk, (size_t) BLK_HDR_LEN(store),
					BLK_HDR_OFF(store, i));  /* 只需初始化块头信息 */
			if (ret == ACL_VSTREAM_EOF) {
				acl_myfree(blk);
				acl_msg_error("%s(%d): write to %s error(%s)", myname,
					__LINE__, STORE_PATH((ZDB_STORE*) store),
					acl_last_serror());
				return (-1);
			}
		}
	}

	acl_myfree(blk);

	if ((((ZDB_STORE*) store)->db->oflags & ZDB_FLAG_CACHE_DAT) == 0) {
		/* 刷新写的缓冲区至磁盘 */
		if (acl_vstream_fflush(((ZDB_STORE*)store)->fhandle.fp) == ACL_VSTREAM_EOF) {
			acl_msg_error("%s(%d): fflush to %s error %s",
				myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
				acl_last_serror());
			return (-1);
		}
	}

	store->hdr.size = count;  /* 更新值存储总分配的数据块个数 */
	return (0);
}

/**
 * 初始化值存储
 * @param store {ZDB_DAT_STORE*} 值存储句柄
 * @param db {ZDB*} ZDB 句柄
 * @return {int} 0: ok; -1: error
 */
static int dat_store_init(ZDB_DAT_STORE *store, ZDB *db)
{
	const char *myname = "dat_store_init";
	int   ret;

	/* 初始化值存储的头信息 */

	store->hdr.limit = db->dat_limit;
	store->hdr.nstep = db->dat_nstep > 0 ? db->dat_nstep : (int) db->dat_limit;
	store->hdr.size = 0;
	store->hdr.count = 0;
	store->hdr.blk_hdr_dlen = sizeof(ZDB_BLK_HDR);
	store->hdr.blk_dlen = db->blk_dlen;
	store->hdr.blk_count = db->blk_count_tmp;
	store->hdr.ihead_idle = -1;  /* 指向一个空位置 */

#ifdef	ZDB_LINK_BUSY
	store->hdr.ihead_busy = -1;  /* 没有占用数据块 */
	store->hdr.itail_busy = -1;  /* 没有占用数据块 */
#endif

	if ((((ZDB_STORE*) store)->db->oflags & ZDB_FLAG_CACHE_DAT) == 0)
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
 
	return (0);
}

/**
 * 打开值存储文件句柄时的回调函数
 * @param fh {ACL_FHANDLE*} 文件句柄
 * @param arg {void*} 参数
 * @return {int} 0: ok; -1: error
 */
static int dat_store_on_open(ACL_FHANDLE *fh, void *arg)
{
	const char *myname = "dat_store_on_open";
	ZDB_DAT_STORE *store = (ZDB_DAT_STORE*) fh;
	ZDB *db = (ZDB*) arg;
	int   ret;

	if (fh->size != sizeof(ZDB_DAT_STORE))
		acl_msg_fatal("%s: fh->size(%d) != ZDB_DAT_STORE's size(%d)",
			myname, (int) fh->size, (int) sizeof(ZDB_DAT_STORE));
	((ZDB_STORE*) store)->db = db;
	((ZDB_STORE*) store)->flag = STORE_FLAG_DAT;
	if ((db->oflags & ZDB_FLAG_SLICE_DAT))
		((ZDB_STORE*) store)->flag |= STORE_FLAG_IO_SLICE;

	/* 如果是新文件则初始化 */

	if (fh->fsize == 0) {
		acl_debug(ZDB_DBG_DAT, 1) ("%s: begin init %s ...",
			myname, STORE_PATH((ZDB_STORE*) store));
		if (dat_store_init(store, db) < 0) {
			acl_msg_error("%s(%d): %s, dat_store_init error",
				myname, __LINE__, STORE_PATH((ZDB_STORE*) store));
			return (-1);
		}
		acl_debug(ZDB_DBG_DAT, 1) ("%s: init %s ok",
			myname, STORE_PATH((ZDB_STORE*) store));

		if ((db->oflags & ZDB_FLAG_CACHE_DAT) != 0) {
			((ZDB_STORE*) store)->cache_max = db->dat_cache_max;
			((ZDB_STORE*) store)->cache_timeout = db->dat_cache_timeout;
			((ZDB_STORE*) store)->wback_max = db->dat_wback_max;
			zdb_io_cache_open((ZDB_STORE*) store, (size_t) BLK_LEN(store));
		}

		/* 开始添加并初始化值存储中的数据块 */
		if (dat_store_blk_add(store, store->hdr.nstep) < 0) {
			acl_msg_error("%s(%d): dat_store_blk_add error",
				myname, __LINE__);
			return (-1);
		}
		zdb_dat_iter_set(store, 1);
		return (0);
	}

	/* 如果是旧文件则读取文件头信息, 且应进行检验 */

	if ((((ZDB_STORE*) store)->db->oflags & ZDB_FLAG_CACHE_DAT) == 0)
		ret = acl_vstream_readn(((ZDB_STORE*) store)->fhandle.fp,
				&store->hdr, sizeof(store->hdr));
	else
		ret = ZDB_READ((ZDB_STORE*) store, &store->hdr, sizeof(store->hdr), 0);

	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): %s, read key header from %s error(%s)",
			myname, __LINE__, __FILE__, STORE_PATH((ZDB_STORE*) store), acl_last_serror());
		return (-1);
	}

	if ((db->oflags & ZDB_FLAG_CACHE_DAT) != 0) {
		((ZDB_STORE*) store)->cache_max = db->dat_cache_max;
		((ZDB_STORE*) store)->cache_timeout = db->dat_cache_timeout;
		((ZDB_STORE*) store)->wback_max = db->dat_wback_max;
		zdb_io_cache_open((ZDB_STORE*) store, (size_t) BLK_LEN(store));
	}

	zdb_dat_iter_set(store, 1);
	return (0);
}

/**
 * 关闭值存储文件句柄时的回调函数
 * @param fh {ACL_FHANDLE*} 文件句柄
 */
static void dat_store_on_close(ACL_FHANDLE *fh)
{
	const char *myname = "dat_store_on_close";
	ZDB_DAT_STORE *store = (ZDB_DAT_STORE*) fh;

	dat_store_header_sync(store);
	zdb_io_cache_close((ZDB_STORE*) store);
	acl_debug(ZDB_DBG_DAT, 2) ("%s(%d): sync header ok, close %s now",
		myname, __LINE__, STORE_PATH((ZDB_STORE*) store));
}

void zdb_dat_store_close(ZDB_DAT_STORE *store)
{
	acl_fhandle_close(&((ZDB_STORE*) store)->fhandle, 120);
}

ZDB_DAT_STORE *zdb_dat_store_open(ZDB *db, const char *filepath)
{
	const char *myname = "zdb_dat_store_open";
	ZDB_DAT_STORE *store;
	unsigned int oflags = ACL_FHANDLE_O_NOATIME /* | ACL_FHANDLE_O_DIRECT */;

	if ((db->oflags & ZDB_FLAG_OPEN_LOCK) != 0)
		oflags |= ACL_FHANDLE_O_MLOCK;

	acl_vstring_strcpy(db->path_tmp, filepath);
	store = (ZDB_DAT_STORE*) acl_fhandle_open(sizeof(ZDB_DAT_STORE), oflags,
			filepath, dat_store_on_open, db, dat_store_on_close);
	if (store == NULL)
		acl_msg_error("%s(%d): open file(%s) error(%s)",
			myname, __LINE__, filepath, acl_last_serror());
	return (store);
}

ZDB_BLK *zdb_dat_get(ZDB *db, const ZDB_BLK_OFF *blk_off, zdb_key_t *key, size_t *size)
{
	const char *myname = "zdb_dat_get";
	int   idisk, inode, ifile, ret, blk_dlen;
	ZDB_DAT_STORE *store;
	const ACL_VSTRING *path;
	zdb_lnk_t blk_ilnk;
	zdb_off_t off;
	ZDB_BLK *blk;

	idisk = (blk_off->inode >> DIR_BITS) & DISK_MASK;
	if (idisk < 0 || idisk > DISK_LIMIT) {
		acl_msg_error("%s(%d): idisk(%d) invalid",
			myname, __LINE__, idisk);
		return (NULL);
	}

	inode = blk_off->inode & DIR_MASK;
	if (inode < 0 || inode > DIR_LIMIT) {
		acl_msg_error("%s(%d): inode(%d) invalid",
			myname, __LINE__, inode);
		return (NULL);
	}

	/* 判断在值存储中的偏移值 */
	if (blk_off->offset < 0) {
		acl_msg_error("%s(%d): blk_off_old(" ACL_FMT_I64D ") invalid",
			myname, __LINE__, blk_off->offset);
		return (NULL);
	}

	ifile = (int) blk_off->offset / (int) db->dat_limit;  /* 文件结点号 */

	path = dat_filepath(db, NULL, idisk, inode, ifile);
	store = zdb_dat_store_open(db, STR(path));
	if (store == NULL) {
		acl_msg_error("%s(%d): open %s error(%s)",
			myname, __LINE__, STR(path), acl_last_serror());
		return (NULL);
	}

	/* 计算在文件中的真实位置 */

	blk_ilnk = (int) blk_off->offset - (zdb_lnk_t) db->dat_limit * ifile;  /* 文件位置索引值 */
	off = BLK_HDR_OFF(store, blk_ilnk);

	/* 计算真实的 ZDB_BLK 的空间大小 */

	blk_dlen = (int) BLK_LEN(store);

	/* 分配一个 ZDB_BLK 对象 */

	blk = (ZDB_BLK*) acl_mycalloc(1, blk_dlen);

	ret = ZDB_READ((ZDB_STORE*) store, blk, blk_dlen, off);

	acl_debug(ZDB_DBG_GETD, 2)
		("%s(%d): %s, zdb_read(%d), blk_off: " ACL_FMT_I64D
		", idisk: %d, inode: %d,"
		 " ifile: %d, ilnk: %d, dat_limit: " ACL_FMT_I64D ", blk_dlen: %d",
		 myname, __LINE__, STORE_PATH((ZDB_STORE*) store), ret, off,
		 idisk, inode, ifile, blk_ilnk, db->dat_limit, blk_dlen);

	if (ret == -1) {
		acl_msg_error("%s(%d): zdb_read %s error(%s), blk_off("
			ACL_FMT_I64D "), blk_dlen(%d), idisk: %d, inode: %d, ifile: %d",
			myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
			acl_last_serror(), off, blk_dlen,
			idisk, inode, ifile);
		zdb_blk_free(blk);
		zdb_dat_store_close(store);  /* 关闭值存储 */
		return (NULL);
	}

	zdb_dat_store_close(store);  /* 关闭值存储 */

	if (key)
		*key = blk->hdr.key;
	if (size)
		*size = (size_t) (BLK_LEN(store) - BLK_HDR_LEN(store));
	return (blk);
}

int zdb_dat_add(ZDB *db, zdb_key_t key, const void *dat, int len)
{
	const char *myname = "zdb_dat_add";
	ZDB_DAT_STORE *store = NULL, *store_tmp;
	int   n, blk_dlen, idisk, inode, ifile;
	ZDB_BLK *blk = NULL;
	ZDB_BLK_OFF blk_off;
	zdb_off_t off;
	zdb_lnk_t ilnk;
	const ACL_VSTRING *path;

#undef	RETURN
#define	RETURN0(x) do { \
	return ((x)); \
} while (0)

#define	RETURN(x) do {  \
	if (store)  \
		zdb_dat_store_close(store);  \
	if (blk)  \
		zdb_blk_free(blk);  \
	return ((x));  \
} while (0)

	inode = dat_inode(db, len);
	if (inode < 0) {
		acl_msg_error("%s(%d): inode(%d) invalid, dat len(%d)",
			myname, __LINE__, inode, len);
		RETURN (-1);
	}

	/* xxx: 因为每个目录号值与该目录下值存储的数据中的数据块个数相等 */
	db->blk_count_tmp = inode;  /* xxx: 设置此临时量主要为了参数传递 */

	/* 尝试遍历该存储目录下所有可用的存储文件: 有可能是旧文件, 也有可能是新文件 */
	
	idisk = zdb_disk_select(db);
	if (idisk < 0) {
		acl_msg_error("%s(%d): no disk available", myname, __LINE__);
		RETURN (-1);
	}

	dat_ifiles_space(db, idisk, inode);  /* 确保 db->dat_ifiles 数组空间够用 */
	ifile = db->dat_disks[idisk].dat_ifiles[inode];
	if (ifile < 0)
		ifile = 0;

	/* 选择可用的值存储句柄 */

	for (; ifile < ZDB_DAT_FILE_LIMIT; ifile++) {
		path = dat_filepath(db, NULL, idisk, inode, ifile);
		acl_debug(ZDB_DBG_ADDD, 2) ("%s(%d): path(%s)",
			myname, __LINE__, STR(path));
		store_tmp = zdb_dat_store_open(db, STR(path));
		if (store_tmp == NULL) {
			acl_msg_error("%s(%d): zdb_dat_store_open %s error(%s)",
				myname, __LINE__, STR(path), acl_last_serror());
			RETURN (-1);
		}

		/* 是否有可用空闲块? */

		if (store_tmp->hdr.limit <= store_tmp->hdr.count) {
			/* 关闭已满了的值存储 */
			zdb_dat_store_close(store_tmp);
			continue;
		}

		/* 如果需要增加值存储空间则增加 */
		if (store_tmp->hdr.size <= store_tmp->hdr.count) {
			n = dat_store_blk_add(store_tmp, store_tmp->hdr.nstep);
			if (n < 0) {
				acl_msg_error("%s(%d): add blk to %s error",
					myname, __LINE__,
					STORE_PATH((ZDB_STORE*) store_tmp));
				zdb_dat_store_close(store_tmp);
				RETURN (-1);
			}
		}

		store = store_tmp;
		db->dat_disks[idisk].dat_ifiles[inode] = ifile;  /* 缓存该文件索引号 */
		break;
	}

	if (store == NULL) {
		acl_msg_error("%s(%d): too many ifile(%d), ZDB_DAT_FILE_LIMIT(%d),"
			" idisk(%d), inode(%d), path(%s)", myname, __LINE__, ifile,
			ZDB_DAT_FILE_LIMIT, idisk, inode,
			acl_sane_dirname(NULL, STR(dat_path(db, NULL, idisk, inode))));
		RETURN (-1);
	}

	if (store->hdr.ihead_idle < 0) {
		acl_msg_error("%s(%d): %s, ihead_idle(%d) invalid, limit("
			ACL_FMT_I64D "), count(" ACL_FMT_I64D ")", myname,
			__LINE__, STORE_PATH((ZDB_STORE*) store),
			store->hdr.ihead_idle, store->hdr.limit,
			store->hdr.count);
		RETURN (-1);
	}

	/* 使用第一个空闲块 */
	ilnk = store->hdr.ihead_idle;

	/* 计算真实的 ZDB_BLK 的空间大小 */
	blk_dlen = (int) BLK_LEN(store);

	/* 计算第一个空闲块的物理位置 */
	off = BLK_HDR_OFF(store, ilnk);

	acl_debug(ZDB_DBG_ADDD, 2)
		("%s(%d): blk_count: %d, blk_dlen: %d, blk_off(" ACL_FMT_I64D
		"), blk_hdr_dlen: %d, len: %d", myname, __LINE__,
		 store->hdr.blk_count, store->hdr.blk_dlen,
		off, store->hdr.blk_hdr_dlen, len);

	/* 分配一个 ZDB_BLK 对象 */
	blk = (ZDB_BLK*) acl_mymalloc(blk_dlen);
	if (blk == NULL)
		acl_msg_fatal("%s(%d): calloc error(%s)",
			myname, __LINE__, acl_last_serror());

	/* 读取该空闲数据块头信息 */
	n = ZDB_READ((ZDB_STORE*) store, blk, (size_t) BLK_HDR_LEN(store), off);
	if (n == -1) {
		acl_msg_error("%s(%d): zdb_read %s error(%s), ihead_idle(%d),"
			" blk_dlen(%d), blk_off(" ACL_FMT_I64D "), blk_hdr_dlen(%d)",
			myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
			acl_last_serror(), ilnk, blk_dlen, off,
			store->hdr.blk_hdr_dlen);
		RETURN (-1);
	}

	/* xxx: 该 blk 其实就是第一个空闲块, 参见上面 ilnk 的形成 */

	blk->hdr.key = key;
	store->hdr.ihead_idle = blk->hdr.inext_idle;  /* 更新空闲块链头 */
	blk->hdr.inext_idle = -1;  /* 从空闲块链中分离 */

#ifdef	ZDB_LINK_BUSY
	if ((db->oflags & ZDB_FLAG_LINK_BUSY) != 0) {
		if (busy_blk_link(store, blk, ilnk) < 0)
			RETURN (-1);
	}
#endif

	acl_debug(ZDB_DBG_ADDD, 2)
		("%s(%d): blk_off: " ACL_FMT_I64D ", inext: %d, key: "
		ACL_FMT_I64D, myname, __LINE__, off, ilnk, key);

	/* 更新当前空闲数据块信息 */

	memcpy(blk->dat, dat, len);  /* 拷贝源数据内容 */

	n = ZDB_WRITE((ZDB_STORE*) store, blk, blk_dlen, off);  /* 更新数据块信息 */
	if (n == -1) {
		acl_msg_error("%s(%d): zdb_write to %s error(%s)",
			myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
			acl_last_serror());
		RETURN (-1);
	}

	/* 更新值存储头信息 */

	store->hdr.count++;

	/* 将 blk_off 映射为键存储中的值 */

	blk_off.offset = ilnk + db->dat_limit * ifile;  /* 累加之前所有的文件中记录的值个数之和 */
	if (blk_off.offset < 0) {
		acl_msg_error("%s(%d): blk_off.offset(" ACL_FMT_I64D
			") too large", myname, __LINE__, blk_off.offset);
		RETURN (-1);
	}

	acl_debug(ZDB_DBG_ADDD, 2)
		("%s(%d): blk_off: " ACL_FMT_I64D ", ihead_idle: %d, dat_limit: "
		ACL_FMT_I64D ", inode: %d, ifile: %d",
		myname, __LINE__, blk_off.offset, store->hdr.ihead_idle,
		db->dat_limit, inode, ifile);

	/* 将 idisk 与 inode 组合存储 */
	blk_off.inode = (idisk << DIR_BITS) + inode;

	/* 更新键存储中数据索引值 */
	if (db->key_set(db, key, &blk_off) < 0) {
		acl_msg_error("%s(%d): set key store error(%s)",
			myname, __LINE__, acl_last_serror());
		RETURN (-1);
	}

	RETURN (1);
}

int zdb_dat_update(ZDB *db, zdb_key_t key, const ZDB_BLK_OFF *blk_off,
	const void *dat, size_t len)
{
	const char *myname = "zdb_dat_update";
	int   idisk, inode, inode_new, ifile, ret;
	const ACL_VSTRING *path;
	ZDB_DAT_STORE *store = NULL;
	zdb_lnk_t  ilnk;
	zdb_off_t  off;
	ZDB_BLK *blk = NULL;

#undef	RETURN
#define	RETURN(x) do {  \
	if (store)  \
		zdb_dat_store_close(store);  \
	if (blk)  \
		zdb_blk_free(blk);  \
	return (x);  \
} while (0)

	idisk = (blk_off->inode >> DIR_BITS) & DISK_MASK;
	if (idisk < 0 || idisk > DISK_LIMIT) {
		acl_msg_error("%s(%d): idisk(%d) invalid",
			myname, __LINE__, idisk);
		RETURN (-1);
	}

	inode = blk_off->inode & DIR_MASK;
	if (inode < 0 || inode > DIR_LIMIT) {
		acl_msg_error("%s(%d): inode(%d) invalid",
			myname, __LINE__, inode);
		RETURN (-1);
	}

	/* 判断在值存储中的偏移值 */
	if (blk_off->offset < 0) {
		acl_msg_error("%s(%d): blk_off_old(" ACL_FMT_I64D
			") invalid", myname, __LINE__, blk_off->offset);
		RETURN (-1);
	}

	/* 计算新的 inode 相对路径号 */
	inode_new = dat_inode(db, (int) len);
	if (inode_new < 0) {
		acl_msg_error("%s(%d): inode(%d) invalid, dat len(%d)",
			myname, __LINE__, inode_new, (int) len);
		RETURN (-1);
	}

	ifile = (int) blk_off->offset / (int) db->dat_limit;  /* 文件结点号 */

	/* 文件位置索引值 */
	ilnk = (zdb_lnk_t) (blk_off->offset - db->dat_limit * ifile);

	path = dat_filepath(db, NULL, idisk, inode, ifile);
	store = zdb_dat_store_open(db, STR(path));
	if (store == NULL) {
		acl_msg_error("%s(%d): open %s error(%s)",
			myname, __LINE__, STR(path), acl_last_serror());
		RETURN (-1);
	}

	if (inode_new == inode) {
		/* 更新旧位置的数据信息 */

		/* 计算真实的存储位置中数据起始位置值 */
		off = BLK_DAT_OFF(store, ilnk);

		/* 写入新数据 */
		ret = ZDB_WRITE((ZDB_STORE*) store, dat, len, off);

		if (ret == -1) {
			acl_msg_error("%s(%d): zdb_write to %s error(%s)",
				myname, __LINE__, STR(path), acl_last_serror());
			RETURN (-1);
		}
		RETURN (1);
	}

	/* 需要移动数据块数据至其它值存储中 */

	/* 该值存储中肯定就有占用数据块 */

	if (store->hdr.count <= 0) {
		acl_msg_error("%s(%d): %s, store->hdr.count(" ACL_FMT_I64D
			") <= 0, ihead_idle(%d), inode(%d, %d), inode_new(%d, %d),"
			" key(" ACL_FMT_I64D "), blk_off(" ACL_FMT_I64D ")",
			myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
			store->hdr.count, store->hdr.ihead_idle,
			idisk, inode, inode_new >> DISK_BITS,
			inode_new & DIR_MASK, key, blk_off->offset);
		store->hdr.count = 0;  /* xxx: reset to 0 */
		RETURN (-1);
	}

	/* 计算真实的存储位置中数据块头起始位置值 */
	off = BLK_HDR_OFF(store, ilnk);

	/* 需要移动值的位置且使旧位置变为空闲块 */

	blk = (ZDB_BLK*) acl_mymalloc((size_t) BLK_LEN(store));  /* 分配数据块 */

	/* 读取该占用数据块的头部信息 */
	ret = ZDB_READ((ZDB_STORE*) store, blk, (size_t) BLK_HDR_LEN(store), off);
	if (ret == -1) {
		acl_msg_error("%s(%d): zdb_read %s error(%s),"
			" blk_hdr_dlen(%d), blk_off(" ACL_FMT_I64D ")",
			myname, __LINE__, STORE_PATH((ZDB_STORE*) store),
			acl_last_serror(), (int) BLK_HDR_LEN(store), blk_off->offset);
		RETURN (-1);
	}

	blk->hdr.key = -1;
	blk->hdr.inext_idle = store->hdr.ihead_idle;  /* 与空闲数据块链连接 */

#ifdef	ZDB_LINK_BUSY
	if ((db->oflags & ZDB_FLAG_LINK_BUSY) != 0) {
		if (busy_blk_unlink(store, blk, ilnk) < 0) {
			acl_msg_error("%s(%d): %s, unlink busy blk error",
				myname, __LINE__, STORE_PATH((ZDB_STORE*) store));
			RETURN (-1);
		}
	}
#endif

	/* 更新值存储头信息 */

	store->hdr.ihead_idle = ilnk;
	store->hdr.count--;

	/* 调整缓存的文件索引号 */

	dat_ifiles_space(db, idisk, inode);
	if (inode < db->dat_disks[idisk].dat_ifiles[inode])
		db->dat_disks[idisk].dat_ifiles[inode] = inode;

	/* 只写数据块的头部信息, 将该数据块变为空闲块 */
	ret = ZDB_WRITE((ZDB_STORE*) store, blk, (size_t) BLK_HDR_LEN(store), off);
	if (ret == -1) {
		acl_msg_error("%s(%d): prwrite %s error(%s), blk_off("
			ACL_FMT_I64D ", " ACL_FMT_I64D ")",
			myname, __LINE__, STR(path),
			acl_last_serror(), blk_off->offset, off);
		RETURN (-1);
	}

	db->dat_disks[idisk].count--;  /* 将所在磁盘分区的分配数量减 1 */
	if (db->dat_disks[idisk].count < 0) {
		acl_msg_error("%s(%d): count(%lld) < 0 in %s",
			myname, __LINE__, db->dat_disks[idisk].count,
			db->dat_disks[idisk].path);
		RETURN (-1);
	}

	/* 向另一个值存储中添加新记录 */

	ret = zdb_dat_add(db, key, dat, (int) len);
	RETURN (ret);
}

int zdb_dat_stat(ZDB *db, const char *filepath, ZDB_DAT_HDR *dat_hdr)
{
	const char *myname = "zdb_dat_stat";
	ZDB_DAT_STORE *store;

	store = zdb_dat_store_open(db, filepath);
	if (store == NULL) {
		acl_msg_error("%s(%d): open %s error(%s)",
			myname, __LINE__, filepath, acl_last_serror());
		return (-1);
	}

	memcpy(dat_hdr, &store->hdr, sizeof(ZDB_DAT_HDR));
	zdb_dat_store_close(store);
	return (0);
}

int zdb_dat_check(ZDB_DAT_STORE *store, ZDB_DAT_HDR *dat_hdr)
{
	const char *myname = "zdb_dat_check";
	acl_int64 nidle = 0, nused = 0;
	ZDB_BLK *blk;
	zdb_lnk_t ilnk;
	zdb_off_t blk_off;
	int   blk_len, ret, failed = 0;
	ACL_ITER iter;
	time_t begin;

	acl_msg_info("%s(%d): %s: checking ......",
		myname, __LINE__, STORE_PATH((ZDB_STORE*) store));
	acl_msg_info("%s(%d):\tHDR STATUS:", myname, __LINE__);
	acl_msg_info("%s(%d):\tlimit: " ACL_FMT_I64D ", size: "
		ACL_FMT_I64D ", count: " ACL_FMT_I64D,
		myname, __LINE__, store->hdr.limit,
		store->hdr.size, store->hdr.count);
	acl_msg_info("%s(%d):\tnstep: %d, blk_hdr_dlen: %d, blk_dlen: %d,"
		" blk_count: %d", myname, __LINE__, store->hdr.nstep,
		store->hdr.blk_hdr_dlen, store->hdr.blk_dlen,
		store->hdr.blk_count);
	acl_msg_info("%s(%d):\tihead_idle: %d, ihead_busy: %d",
		myname, __LINE__, store->hdr.ihead_idle, store->hdr.ihead_busy);

	if (dat_hdr)
		memcpy(dat_hdr, &store->hdr, sizeof(ZDB_DAT_HDR));

	/* 扫描所有被使用的数据块结点 */

	acl_msg_info("%s(%d):\tBegin check used blk ......", myname, __LINE__);

	time(&begin);

	/* 仅检查数据块的头 */
	zdb_dat_iter_set(store, 1);
	acl_foreach(iter, (ZDB_STORE*) store) {
		nused++;
		if (nused > 0 && nused % 10000 == 0) {
			ZDB_BLK_HDR *blk_hdr = (ZDB_BLK_HDR*) iter.data;

			printf("\tnused: " ACL_FMT_I64D ", key: "
				ACL_FMT_I64D ", dlen: %d, ",
				nused, blk_hdr->key, iter.dlen);
			ACL_METER_TIME("-");
		}
	}

	if (nused != store->hdr.count) {
		acl_msg_error("%s(%d):\terror, nused(" ACL_FMT_I64D
			") != store->hdr.count(" ACL_FMT_I64D
			") for %s, please repair it!, time: %ld", myname, __LINE__,
			nused, store->hdr.count, STORE_PATH((ZDB_STORE*) store),
			(long) (time(NULL) - begin));
		failed = 1;
	} else {
		acl_msg_info("%s(%d):\tOk, check used blk over, busy blk: "
			ACL_FMT_I64D ", time: %ld",
			myname, __LINE__, store->hdr.count, time(NULL) - begin);
	}

	/* 允许遍历数据块的数据体 */
	zdb_dat_iter_set(store, 1);

	/* 开始扫描所有的空闲数据块结点 */

	acl_msg_info("%s(%d):\tBegin check idle blk ......", myname, __LINE__);

	time(&begin);

	ret = 0;
	blk_len = (int) BLK_LEN(store);
	blk = (ZDB_BLK*) acl_mymalloc(blk_len);
	ilnk = store->hdr.ihead_idle;
	while (ilnk >= 0) {
		blk_off = BLK_HDR_OFF(store, ilnk);
		ret = ZDB_READ((ZDB_STORE*) store, blk, blk_len, blk_off);
		if (ret == -1) {
			acl_msg_error("%s(%d): zdb_read %s error(%s), blk_off("
				ACL_FMT_I64D ")", myname, __LINE__,
				STORE_PATH((ZDB_STORE*) store),
				acl_last_serror(), blk_off);
			failed = 1;
			break;
		}
		ilnk = blk->hdr.inext_idle;
		nidle++;
	}

	if (ret >= 0 && nidle == store->hdr.size - store->hdr.count)
		acl_msg_info("%s(%d):\tOk, check idle blk over , idle blk: "
			ACL_FMT_I64D ", time: %ld",
			myname, __LINE__, nidle, time(NULL) - begin);
	else {
		acl_msg_error("%s(%d):\tcheck idle blk error for %s, time: %ld",
			myname, __LINE__, STORE_PATH((ZDB_STORE*) store), time(NULL) - begin);
		acl_msg_error("%s(%d): \tret: %d, idle blk: " ACL_FMT_I64D
			", store->hdr.size - store->hdr.count: " ACL_FMT_I64D,
			myname, __LINE__, ret, nidle,
			store->hdr.size - store->hdr.count);
		failed = 1;
	}

	zdb_blk_free(blk);

	return (failed ? -1 : 0);
}

int zdb_dat_check3(ZDB *db, const char *filepath, ZDB_DAT_HDR *dat_hdr)
{
	const char *myname = "zdb_dat_check3";
	ZDB_DAT_STORE *store;
	int   ret;

	store = zdb_dat_store_open(db, filepath);
	if (store == NULL) {
		acl_msg_error("%s(%d): open %s error(%s)",
			myname, __LINE__, filepath, acl_last_serror());
		return (-1);
	}

	ret = zdb_dat_check(store, dat_hdr);
	zdb_dat_store_close(store);
	return (ret);
}

#endif /* ACL_CLIENT_ONLY */
