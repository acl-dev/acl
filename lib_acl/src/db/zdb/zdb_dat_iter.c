#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "init/acl_init.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "db/zdb.h"

#endif

#ifndef ACL_CLIENT_ONLY

#include "zdb_private.h"

typedef struct {
	ZDB_BLK *blk;
	int  blk_len;
} BLK_CTX_T;

static void dummy(void *ptr acl_unused)
{

}

static void free_tls(void *ptr)
{
	BLK_CTX_T *ctx = (BLK_CTX_T*) ptr;
	acl_myfree(ctx->blk);
	acl_myfree(ctx);
}

static void *__tls = NULL;
#ifndef HAVE_NO_ATEXIT
static void main_free_tls(void)
{
	if (__tls) {
		BLK_CTX_T *ctx = (BLK_CTX_T*) __tls;
		acl_myfree(ctx->blk);
		acl_myfree(ctx);
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
static BLK_CTX_T *tls_alloc(int len)
{
	BLK_CTX_T *ptr;

	(void) acl_pthread_once(&once_control, once_init);
	ptr = (BLK_CTX_T*) acl_pthread_getspecific(once_key);
	if (ptr == NULL) {
		ptr = acl_mymalloc(sizeof(BLK_CTX_T));
		ptr->blk = acl_mymalloc(len);
		ptr->blk_len = len;
		acl_pthread_setspecific(once_key, ptr);
		if ((unsigned long) acl_pthread_self() == acl_main_thread_self())
			__tls = ptr;
		return ptr;
	} else if (ptr->blk_len >= len)
		return ptr;

	acl_myfree(ptr->blk);
	ptr->blk = acl_mymalloc(len);
	ptr->blk_len = len;
	return ptr;
}

static const void *dat_iter_get_next(ZDB_DAT_STORE *store, ACL_ITER *iter)
{
	const char *myname = "dat_iter_get_next";
	zdb_off_t blk_off;
	int  ret;
	int  blk_len = (int) BLK_LEN(store);
	BLK_CTX_T *ctx = tls_alloc(blk_len);
	ZDB_BLK *blk = ctx->blk;

    if (iter->i >= store->hdr.size) {
        iter->data = iter->ptr = NULL;
        return NULL;
    }
    
    blk_off = BLK_HDR_OFF(store, iter->i);
    ret = ZDB_READ((ZDB_STORE*) store, blk, blk_len, blk_off);
    if (ret == -1 ) {
        acl_msg_error("%s(%d): zdb_read %s error, blk_off("
            ACL_FMT_I64D ")", myname, __LINE__,
				STORE_PATH((ZDB_STORE*) store), blk_off);
        iter->data = iter->ptr = NULL;
        return NULL;
    }
    if (blk->hdr.key == -1) {
        iter->data = iter->ptr = NULL;
        return NULL;
    }

    iter->data = iter->ptr = blk;
    iter->dlen = blk_len;
    iter->i++;  /* 保留下一个索引位置 */
    return iter->ptr;
}

/**
 * 获得迭代器头部数据
 * @param iter {ACL_ITER*} 迭代器指针
 * @return {const void*} 数据地址
 */
static const void *dat_iter_head(ACL_ITER *iter, struct ZDB_DAT_STORE *store)
{
#ifdef	ZDB_LINK_BUSY
	const char *myname = "dat_iter_head";
	zdb_off_t blk_off;
	int   ret;
	int  blk_len = (int) BLK_LEN(store);
	BLK_CTX_T *ctx = tls_alloc(blk_len);
	ZDB_BLK *blk = ctx->blk;
#endif

	iter->key = NULL;
	iter->klen = 0;

	if (!(((ZDB_STORE*) store)->db->oflags & ZDB_FLAG_LINK_BUSY)) {
		iter->i = 0;
		return dat_iter_get_next(store, iter);
	}

#ifdef	ZDB_LINK_BUSY
	if (store->hdr.ihead_busy < 0) {
		iter->data = iter->ptr = NULL;
		return NULL;
	}

	blk_off = BLK_HDR_OFF(store, store->hdr.ihead_busy);
	ret = ZDB_READ((ZDB_STORE*) store, __blk, blk_len, blk_off);
	if (ret == -1) {
		acl_msg_error("%s(%d): zdb_read %s error(%s), blk_off("
			ACL_FMT_I64D ")", myname, __LINE__,
			STORE_PATH((ZDB_STORE*) store), blk_off);
		iter->data = iter->ptr = NULL;
		return NULL;
	}
	iter->data = iter->ptr = blk;
	iter->dlen = blk_len;
	iter->i = (int) blk->hdr.inext_busy;  /* 保留下一个索引位置 */
	return iter->ptr;
#else
	iter->i = 0;
	return dat_iter_get_next(store, iter);
#endif
}

/**
 * 获得迭代器的那一个数据
 * @param iter {ACL_ITER*} 迭代器指针
 * @return {const void*} 数据地址
 */
static const void *dat_iter_next(ACL_ITER *iter, struct ZDB_DAT_STORE *store)
{
#ifdef	ZDB_LINK_BUSY
	const char *myname = "dat_iter_next";
	zdb_off_t blk_off;
	int  ret;
	int  blk_len = (int) BLK_LEN(store);
	BLK_CTX_T *ctx = tls_alloc(blk_len);
	ZDB_BLK *blk = ctx->blk;
#endif

	if (!(((ZDB_STORE*) store)->db->oflags & ZDB_FLAG_LINK_BUSY)) {
		return dat_iter_get_next(store, iter);
	}

#ifdef	ZDB_LINK_BUSY

	if (iter->i < 0) {
		iter->data = iter->ptr = NULL;
		return (NULL);
	}

	blk_off = BLK_HDR_OFF(store, iter->i);
	ret = ZDB_READ((ZDB_STORE*) store, __blk, blk_len, blk_off);
	if (ret == -1) {
		acl_msg_error("%s(%d): zdb_read %s error(%s), blk_off("
			ACL_FMT_I64D ")", myname, __LINE__,
			STORE_PATH((ZDB_STORE*) store), blk_off);
		iter->data = iter->ptr = NULL;
		return NULL;
	}
	iter->data = iter->ptr = blk;
	iter->dlen = blk_len;
	iter->i = (int) blk->hdr.inext_busy;  /* 保留下一个索引位置 */
	return iter->ptr;
#else
	return dat_iter_get_next(store, iter);
#endif
}

/************************************************************************/

static void dummy2(void *ptr acl_unused)
{

}

static void free_tls2(void *ptr)
{
	acl_myfree(ptr);
}

static void *__tls2 = NULL;
#ifndef HAVE_NO_ATEXIT
static void main_free_tls2(void)
{
	if (__tls2) {
		acl_myfree(__tls2);
		__tls2 = NULL;
	}
}
#endif

static acl_pthread_key_t  once_key2;
static void once_init2(void)
{
	if ((unsigned long) acl_pthread_self() == acl_main_thread_self()) {
		acl_pthread_key_create(&once_key2, dummy2);
#ifndef HAVE_NO_ATEXIT
		atexit(main_free_tls2);
#endif
	} else
		acl_pthread_key_create(&once_key2, free_tls2);
}

static acl_pthread_once_t once_control2 = ACL_PTHREAD_ONCE_INIT;
static ZDB_BLK_HDR *get_blk_hdr(void)
{
	ZDB_BLK_HDR *ptr;

	(void) acl_pthread_once(&once_control2, once_init2);
	ptr = (ZDB_BLK_HDR*) acl_pthread_getspecific(once_key2);
	if (ptr == NULL) {
		ptr = (ZDB_BLK_HDR *) acl_mymalloc(sizeof(ZDB_BLK_HDR));
		acl_pthread_setspecific(once_key2, ptr);
		if ((unsigned long) acl_pthread_self() == acl_main_thread_self())
			__tls2 = ptr;
	}
	return ptr;
}

static const void *hdr_iter_get_next(ZDB_DAT_STORE *store, ACL_ITER *iter)
{
	const char *myname = "hdr_iter_get_next";
	zdb_off_t blk_off;
	int  ret;
	int  hdr_len = (int) BLK_HDR_LEN(store);
	ZDB_BLK_HDR *blk_hdr = get_blk_hdr();

    if (iter->i >= store->hdr.size) {
        iter->data = iter->ptr = NULL;
        return NULL;
    }
    
    blk_off = BLK_HDR_OFF(store, iter->i);
    ret = ZDB_READ((ZDB_STORE*) store, blk_hdr, hdr_len, blk_off);
    if (ret == -1 ) {
        acl_msg_error("%s(%d): zdb_read %s error, blk_off("
            ACL_FMT_I64D ")", myname, __LINE__,
            STORE_PATH((ZDB_STORE*) store), blk_off);
        iter->data = iter->ptr = NULL;
        return NULL;
    }
    if (blk_hdr->key == -1) {
        iter->data = iter->ptr = NULL;
        return NULL;
    }
    
    iter->data = iter->ptr = blk_hdr;
    iter->dlen = hdr_len;
    iter->i++;  /* 保留下一个索引位置 */
    return iter->ptr;
}

/**
 * 获得迭代器头部数据
 * @param iter {ACL_ITER*} 迭代器指针
 * @return {const void*} 数据地址
 */
static const void *hdr_iter_head(ACL_ITER *iter, struct ZDB_DAT_STORE *store)
{
#ifdef	ZDB_LINK_BUSY
	const char *myname = "hdr_iter_head";
	zdb_off_t blk_off;
	int   ret;
	ZDB_BLK_HDR *blk_hdr = get_blk_hdr();
	int   hdr_len = (int) BLK_HDR_LEN(store);
#endif

	iter->key = NULL;
	iter->klen = 0;

	if (!(((ZDB_STORE*) store)->db->oflags & ZDB_FLAG_LINK_BUSY)) {
		iter->i = 0;
		return hdr_iter_get_next(store, iter);
	}

#ifdef	ZDB_LINK_BUSY
	if (store->hdr.ihead_busy < 0) {
		iter->data = iter->ptr = NULL;
		return NULL;
	}

	blk_off = BLK_HDR_OFF(store, store->hdr.ihead_busy);
	ret = ZDB_READ((ZDB_STORE*) store, &__blk_hdr, hdr_len, blk_off);
	if (ret == -1) {
		acl_msg_error("%s(%d): zdb_read %s error(%s), blk_off("
			ACL_FMT_I64D ")", myname, __LINE__,
			STORE_PATH((ZDB_STORE*) store), blk_off);
		iter->data = iter->ptr = NULL;
		return NULL;
	}
	iter->data = iter->ptr = blk_hdr;
	iter->dlen = hdr_len;
	iter->i = (int) blk_hdr->inext_busy;  /* 保留下一个索引位置 */
	return iter->ptr;
#else
	iter->i = 0;
	return hdr_iter_get_next(store, iter);
#endif
}

/**
 * 获得迭代器的那一个数据
 * @param iter {ACL_ITER*} 迭代器指针
 * @return {const void*} 数据地址
 */
static const void *hdr_iter_next(ACL_ITER *iter, struct ZDB_DAT_STORE *store)
{
#ifdef	ZDB_LINK_BUSY
	const char *myname = "hdr_iter_next";
	zdb_off_t blk_off;
	ZDB_BLK_HDR *blk_hdr = get_blk_hdr();
	int  ret;
	int   hdr_len = (int) BLK_HDR_LEN(store);
#endif

	if (!(((ZDB_STORE*) store)->db->oflags & ZDB_FLAG_LINK_BUSY)) {
		return hdr_iter_get_next(store, iter);
	}

#ifdef	ZDB_LINK_BUSY

	if (iter->i < 0) {
		iter->data = iter->ptr = NULL;
		return NULL;
	}

	blk_off = BLK_HDR_OFF(store, iter->i);
	ret = ZDB_READ((ZDB_STORE*) store, &__blk_hdr, hdr_len, blk_off);
	if (ret == -1) {
		acl_msg_error("%s(%d): zdb_read %s error(%s), blk_off("
			ACL_FMT_I64D ")", myname, __LINE__,
			STORE_PATH((ZDB_STORE*) store), blk_off);
		iter->data = iter->ptr = NULL;
		return NULL;
	}
	iter->data = iter->ptr = blk_hdr;
	iter->dlen = hdr_len;
	iter->i = (int) blk_hdr->inext_busy;  /* 保留下一个索引位置 */
	return iter->ptr;
#else
	return hdr_iter_get_next(store, iter);
#endif
}

void zdb_dat_iter_set(ZDB_DAT_STORE *store, int read_data)
{
	if (read_data) {
		((ZDB_STORE*) store)->iter_head = (STORE_ITER) dat_iter_head;
		((ZDB_STORE*) store)->iter_next = (STORE_ITER) dat_iter_next;
	} else {
		((ZDB_STORE*) store)->iter_head = (STORE_ITER) hdr_iter_head;
		((ZDB_STORE*) store)->iter_next = (STORE_ITER) hdr_iter_next;
	}
}

#endif /* ACL_CLIENT_ONLY */
