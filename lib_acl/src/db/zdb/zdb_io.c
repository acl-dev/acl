#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <string.h>
#include "stdlib/acl_avl.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_cache.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_slice.h"
#include "stdlib/acl_fhandle.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_mystring.h"
#include "db/zdb.h"
#endif

#if 0
#if defined(ACL_LINUX) && !defined(MINGW) && defined(__GNUC__) && __GNUC__ >= 4
# ifndef  _GNU_SOURCE
#  define _GNU_SOURCE
# endif
# include <unistd.h>
# ifdef __USE_LARGEFILE64
#  define PWRITE pwrite64
#  define PREAD  pread64
# endif
#endif
#endif

#ifndef ACL_CLIENT_ONLY

#include "zdb_private.h"

#define	KEY_LEN	21

typedef struct ZDB_IO_BLK {
	zdb_off_t off;
	avl_node_t node;
	char *dat;
	size_t dlen;
	unsigned int flag;
#define	BLK_F_DIRTY	(1 << 0)

	ZDB_IO *io;
} ZDB_IO_BLK;

struct ZDB_IO {
	avl_tree_t blk_tree;	/* 存储所有需要同步至磁盘的缓存数据块 */
	ACL_CACHE *blk_cache;	/* 存储所有缓存数据块 */
	size_t blk_len;
	ACL_SLICE *blk_slice;
	ACL_SLICE *dat_slice;
	ACL_VSTRING *buf;
	ZDB_STORE *store;
};

#define	IO_STREAM(io)	((io)->store->fhandle.fp)
#define	IO_HANDLE(io)	(ACL_VSTREAM_FILE(IO_STREAM((io))))
#define	IO_PATH(io)	(STORE_PATH((io)->store))

static int __n = 0;

static void io_blk_free(ZDB_IO_BLK *blk)
{
	const char *myname = "io_blk_free";
	int   ret;

	if ((blk->flag & BLK_F_DIRTY)) {
		ZDB_IO *io = blk->io;

		avl_remove(&io->blk_tree, blk);

#ifdef	PWRITE
		ret = PWRITE(IO_HANDLE(io), blk->dat, blk->dlen, blk->off);
		if (ret != (int) blk->dlen) {
			acl_msg_error("%s(%d): pwrite to %s error(%s),"
				" ret(%d) != len(%d), off: " ACL_FMT_I64D,
				myname, __LINE__, PATH(IO_STREAM(io)),
				acl_last_serror(), ret,
				(int) blk->dlen, blk->off);
		}
#else
		if (acl_vstream_fseek(IO_STREAM(io), blk->off, SEEK_SET) < 0) {
			acl_msg_error("%s(%d): fseek %s error(%s), off: " ACL_FMT_I64D,
				myname, __LINE__, IO_PATH(io),
				acl_last_serror(), blk->off);
		} else if ((ret = acl_vstream_writen(IO_STREAM(io), blk->dat, blk->dlen))
				== ACL_VSTREAM_EOF)
		{
			acl_msg_error("%s(%d): readn from %s, ret(%d) != size(%d),"
				" off(" ACL_FMT_I64D "), error(%s)", myname, __LINE__,
				IO_PATH(io), ret, (int) blk->dlen,
				blk->off, acl_last_serror());
		}
#endif
	}

	if (blk->io->dat_slice)
		acl_slice_free2(blk->io->dat_slice, blk->dat);
	else
		acl_myfree(blk->dat);
	if (blk->io->blk_slice)
		acl_slice_free2(blk->io->blk_slice, blk);
	else
		acl_myfree(blk);
	__n--;
}

static ZDB_IO_BLK *io_blk_new(ZDB_IO *io)
{
	ZDB_IO_BLK *blk;

	if (io->blk_slice)
		blk = (ZDB_IO_BLK*) acl_slice_alloc(io->blk_slice);
	else
		blk = (ZDB_IO_BLK*) acl_mymalloc(sizeof(ZDB_IO_BLK));
	if (io->dat_slice)
		blk->dat = (char*) acl_slice_alloc(io->dat_slice);
	else
		blk->dat = (char*) acl_mymalloc(io->blk_len);

	blk->flag = 0;
	blk->io = io;
	__n++;
	return (blk);
}

/*----------------------------------------------------------------------------*/

static int cmp_fn(const void *v1, const void *v2)
{
	const ZDB_IO_BLK *n1 = (const ZDB_IO_BLK*) v1;
	const ZDB_IO_BLK *n2 = (const ZDB_IO_BLK*) v2;

	if (n1->off > n2->off)
		return (1);
	else if (n1->off < n2->off)
		return (-1);
	else
		return (0);
}

static void free_blk_cache(const ACL_CACHE_INFO *info acl_unused, void *arg)
{
	ZDB_IO_BLK *blk = (ZDB_IO_BLK*) arg;

	io_blk_free(blk);
}

void zdb_io_cache_open(ZDB_STORE *store, size_t blk_len)
{
	ZDB_IO *io = (ZDB_IO*) acl_mycalloc(1, sizeof(ZDB_IO));
#ifdef	_LP64
	unsigned int flag = ACL_SLICE_FLAG_GC2 | ACL_SLICE_FLAG_RTGC_OFF | ACL_SLICE_FLAG_LP64_ALIGN;
#else
	unsigned int flag = ACL_SLICE_FLAG_GC2 | ACL_SLICE_FLAG_RTGC_OFF;
#endif
	int   page_size = 4096 * 10;

	if ((int) blk_len >= page_size)
		page_size = (int) blk_len * 100;

	io->store = store;
	io->blk_len = blk_len;
	avl_create(&io->blk_tree, cmp_fn, sizeof(ZDB_IO_BLK),
			offsetof(ZDB_IO_BLK, node));
	io->blk_cache = acl_cache_create(store->cache_max,
		store->cache_timeout, free_blk_cache);
	if ((store->flag & STORE_FLAG_IO_SLICE)) {
		io->blk_slice = acl_slice_create("blk_slice", 0,
			(int) sizeof(ZDB_IO_BLK), flag);
		io->dat_slice = acl_slice_create("dat_slice",
			page_size, (int) blk_len, flag);
	} else {
		io->blk_slice = NULL;
		io->dat_slice = NULL;
	}
	io->buf = acl_vstring_alloc(page_size);
	store->io = io;
}

void zdb_io_cache_close(ZDB_STORE *store)
{
	if (!store->io)
		return;

	(void) zdb_io_cache_sync(store);
	avl_destroy(&store->io->blk_tree);
	acl_vstring_free(store->io->buf);
	acl_cache_free(store->io->blk_cache);
	if (store->io->dat_slice)
		acl_slice_destroy(store->io->dat_slice);
	if (store->io->blk_slice)
		acl_slice_destroy(store->io->blk_slice);
	acl_myfree(store->io);
}

int zdb_io_cache_sync(ZDB_STORE *store)
{
	const char *myname = "zdb_io_cache_sync";
	ZDB_IO_BLK *blk_first, *blk_next, *blk_iter;
	int   ret, n, dlen = 0;
	zdb_off_t off;
	ZDB_IO *io = store->io;

	if (!io)
		return (0);

	while (1) {
		blk_first = (ZDB_IO_BLK*) avl_first(&io->blk_tree);
		if (blk_first == NULL)
			break;

		off = blk_first->off;
		blk_iter = blk_first;
		acl_vstring_memcpy(io->buf, blk_iter->dat, blk_iter->dlen);

		/* 尽量将连续数据组合成一块数据写，可以减少 IO 次数 */
		n = 0;
		while (1) {
			blk_next = (ZDB_IO_BLK*) AVL_NEXT(&io->blk_tree, blk_iter);
			if (blk_next == NULL)
				break;
			if (blk_iter->off + (int) blk_iter->dlen != blk_next->off)
				break;

			avl_remove(&io->blk_tree, blk_iter);

			/* 防止在 io_blk_free 再次调用 avl_remove */
			blk_iter->flag &= ~BLK_F_DIRTY;
			blk_iter = blk_next;
			acl_vstring_memcat(io->buf, blk_iter->dat, blk_iter->dlen);
			n++;
		}

#ifdef	PWRITE
		ret = PWRITE(IO_HANDLE(io), STR(io->buf), LEN(io->buf), off);
		if (ret != (int) LEN(io->buf)) {
			acl_msg_error("%s(%d): pwrite to %s error(%s),"
				" ret(%d) != len(%d), off: " ACL_FMT_I64D, myname,
				__LINE__, PATH(IO_STREAM(io)), acl_last_serror(),
				ret, (int) LEN(io->buf), off);
			return (-1);
		}
#else
		if (acl_vstream_fseek(IO_STREAM(io), off, SEEK_SET) < 0) {
			acl_msg_error("%s(%d): fseek %s error(%s), off: " ACL_FMT_I64D,
				myname, __LINE__, IO_PATH(io), acl_last_serror(), off);
			return (-1);
		}

		ret = acl_vstream_writen(IO_STREAM(io), STR(io->buf), LEN(io->buf));
		if (ret == ACL_VSTREAM_EOF) {
			acl_msg_error("%s(%d): readn from %s, ret(%d) != size(%d),"
				" off(" ACL_FMT_I64D "), error(%s)", myname, __LINE__,
				IO_PATH(io), ret, (int) LEN(io->buf),
				off, acl_last_serror());
			return (-1);
		}
#endif

		dlen += (int) LEN(io->buf);
		if (n == 0) {
			avl_remove(&io->blk_tree, blk_first);
			/* 防止在 io_blk_free 再次调用 avl_remove */
			blk_first->flag &= ~BLK_F_DIRTY;
		}
	}

	return (dlen);
}

static void zdb_io_cache_add(ZDB_IO *io, const void *buf,
	size_t len, zdb_off_t off, int dirty)
{
	ZDB_IO_BLK *blk;
	char  key[KEY_LEN];

	if (io->blk_len < len)
		return;

	acl_i64toa(off, key, sizeof(key));

	blk = io_blk_new(io);
	memcpy(blk->dat, buf, len);
	blk->off = off;
	blk->dlen = len;
	if (dirty) {
		blk->flag |= BLK_F_DIRTY;
		/* 添加进写缓存中 */
		avl_add(&io->blk_tree, blk);

		/* 同步写缓存中的数据块至磁盘 */
		if ((int) avl_numnodes(&io->blk_tree) >= io->store->wback_max) {
			(void) zdb_io_cache_sync(io->store);
		}
	}

	/* 添加进总缓存中 */
	(void) acl_cache_enter(io->blk_cache, key, blk);
}

static int zdb_io_cache_write(ZDB_IO *io, const void *buf,
	size_t len, zdb_off_t off)
{
	ZDB_IO_BLK *blk;
	char  key[KEY_LEN];

	if (io->blk_len < len)
		return (0);

	/* 先查询缓存中是否存在 */

	acl_i64toa(off, key, sizeof(key));

	blk = (ZDB_IO_BLK*) acl_cache_find(io->blk_cache, key);
	if (blk) {
		if (len > blk->dlen)
			blk->dlen = len;
		memcpy(blk->dat, buf, len);  /* just override */
		if ((blk->flag & BLK_F_DIRTY))  /* 说明已经在写缓存了 */
			return (int) (len);
		/* 需要添加进写缓存 */
		blk->flag |= BLK_F_DIRTY;
		avl_add(&io->blk_tree, blk);
		return (int) (len);
	}

	/* 说明是新数据 */

	zdb_io_cache_add(io, buf, len, off, 1);
	return (int) (len);
}

static int zdb_io_cache_read(ZDB_IO *io, void *buf,
	size_t size, zdb_off_t off)
{
	ZDB_IO_BLK *blk;
	char  key[KEY_LEN];

	if (size > io->blk_len)
		return (0);

	acl_i64toa(off, key, sizeof(key));

	blk = (ZDB_IO_BLK*) acl_cache_find(io->blk_cache, key);
	if (blk != NULL && blk->dlen >= size) {
		memcpy(buf, blk->dat, size);
		return ((int) size);
	}

	return (0);
}

int zdb_write(ZDB_STORE *store, const void *buf, size_t len, zdb_off_t off)
{
	const char *myname = "zdb_write";
	int   ret = 0;

	if (store->io != NULL) {
		ret = zdb_io_cache_write(store->io, buf, len, off);
		if (ret > 0)
			return (ret);
	}

#ifdef	PWRITE
	ret = PWRITE(STORE_FILE(store), buf, len, off);

	if (ret != (int) len) {
		acl_msg_error("%s(%d): pwrite to %s, ret(%d) != len(%d)",
			myname, __LINE__, STORE_PATH(store), ret, (int) len);
		return (ACL_VSTREAM_EOF);
	}
#else
	if (acl_vstream_fseek(STORE_STREAM(store), off, SEEK_SET) < 0) {
		acl_msg_error("%s(%d): fseek %s error(%s), off: " ACL_FMT_I64D,
			myname, __LINE__, STORE_PATH(store), acl_last_serror(), off);
		return (ACL_VSTREAM_EOF);
	}

	ret = acl_vstream_writen(STORE_STREAM(store), buf, len);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): readn from %s, ret(%d) != size(%d),"
			" off(" ACL_FMT_I64D "), error(%s)", myname, __LINE__,
			STORE_PATH(store), ret, (int) len, off, acl_last_serror());
		return (ACL_VSTREAM_EOF);
	}
#endif

	return (ret);
}

int zdb_read(ZDB_STORE *store, void *buf, size_t size, zdb_off_t off)
{
	const char *myname = "zdb_read";
	int   ret;

	if (store->io != NULL) {
		ret = zdb_io_cache_read(store->io, buf, size, off);
		if (ret > 0)
			return (ret);
	}

#ifdef	PREAD
	ret = PREAD(STORE_FILE(store), buf, size, off);
	if (ret != (int) size) {
		acl_msg_error("%s(%d): pread from %s, ret(%d) != size(%d),"
			" off(" ACL_FMT_I64D "), error(%s)", myname, __LINE__,
			STORE_PATH(store), ret, (int) size, off, acl_last_serror());
		return (ACL_VSTREAM_EOF);
	}
#else
	if (acl_vstream_fseek(STORE_STREAM(store), off, SEEK_SET) < 0) {
		acl_msg_error("%s(%d): fseek %s error(%s), off: " ACL_FMT_I64D,
			myname, __LINE__, STORE_PATH(store), acl_last_serror(), off);
		return (ACL_VSTREAM_EOF);
	}

	ret = acl_vstream_readn(STORE_STREAM(store), buf, size);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): readn from %s, ret(%d) != size(%d),"
			" off(" ACL_FMT_I64D "), error(%s)", myname, __LINE__,
			STORE_PATH(store), ret, (int) size, off, acl_last_serror());
		return (ACL_VSTREAM_EOF);
	}
#endif

	if (store->io != NULL) {
		/* 添加进缓存中 */
		zdb_io_cache_add(store->io, buf, size, off, 0);
	}

	return (ret);
}

/*----------------------------------------------------------------------------*/

#endif /* ACL_CLIENT_ONLY */
