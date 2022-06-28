#include "StdAfx.h"
#include "dict.h"
#include "debug_var.h"
#include "dict_pool.h"

#define STR acl_vstring_str
#define LEN ACL_VSTRING_LEN

typedef struct POOL_PARTION {
	ACL_VSTRING *path;
	int   db_cnt;
	int   obj_cnt;
} POOL_PARTION;

struct DICT_POOL_DB {
	DICT_POOL *pool;
	ACL_VSTRING *dpath;
	DICT *dict_read;
	DICT *dict_write;
	acl_pthread_mutex_t lock;
	int   seqcnt;
	POOL_PARTION *partion;
};

struct DICT_POOL {
	char *dict_name;
	int   pool_size;
	unsigned (*hash_fn)(const void *key, size_t len);
	int   dict_cur;
	DICT_POOL_DB *dbpool;

	POOL_PARTION *partions;
	int   partions_size;
};

void dict_pool_init(void)
{
	dict_init();
}

DICT_POOL *dict_pool_new(const char **partions, int partions_size,
	const char *dict_type, const char *dict_path,
	const char *dict_name, int pool_size)
{
	const char *myname = "dict_pool_new";
	DICT_POOL *pool = (DICT_POOL*) acl_mycalloc(1, sizeof(DICT_POOL));
	int   i, j;
	int   open_flags;
	int   dict_flags = /* DICT_FLAG_LOCK | */ DICT_FLAG_DUP_REPLACE | DICT_FLAG_TRY0NULL;
	int   two_db = 0;

	if (partions == NULL)
		acl_msg_fatal("%s(%d): partions null", myname, __LINE__);
	if (partions_size <= 0)
		acl_msg_fatal("%s(%d): partions_size(%d) <= 0", myname, __LINE__, partions_size);
	if (dict_type == NULL || *dict_type == 0)
		acl_msg_fatal("%s(%d): dict_type invalid", myname, __LINE__);
	if (dict_path == NULL || *dict_path == 0)
		acl_msg_fatal("%s(%d): dict_path invalid", myname, __LINE__);
	if (dict_name == NULL || *dict_name == 0)
		acl_msg_fatal("%s(%d): dict_name invalid", myname, __LINE__);
	if (pool_size <= 0)
		acl_msg_fatal("%s(%d): pool_size(%d) invalid", myname, __LINE__, pool_size);

	if (strncasecmp(dict_type, "cdb", 3) == 0) {
		two_db = 1;
	} else if (strncasecmp(dict_type, "btree", 5) == 0) {
		two_db = 0;
		open_flags = O_CREAT | O_RDWR;
	} else if (strncasecmp(dict_type, "hash", 4) == 0) {
		two_db = 0;
		open_flags = O_CREAT | O_RDWR;
	} else if (strncasecmp(dict_type, "tc", 2) == 0) {
		two_db = 0;
		open_flags = O_CREAT | O_RDWR;
	} else {
		acl_msg_fatal("unknown dict type from %s", dict_type);
	}

	pool->dbpool = (DICT_POOL_DB*) acl_mycalloc(pool_size, sizeof(DICT_POOL_DB));
	pool->partions = (POOL_PARTION*) acl_mycalloc(partions_size, sizeof(POOL_PARTION));
	pool->partions_size = partions_size;

	for (i = 0; i < partions_size; i++) {
		pool->partions[i].path = acl_vstring_alloc(256);
		acl_vstring_strcpy(pool->partions[i].path, partions[i]);
		pool->partions[i].db_cnt = 0;
		pool->partions[i].obj_cnt = 0;
	}

	for (j = 0, i = 0; i < pool_size; i++) {
		acl_pthread_mutex_init(&pool->dbpool[i].lock, NULL);
		pool->dbpool[i].dpath = acl_vstring_alloc(256);
		if (j >= partions_size)
			j = 0;
		acl_vstring_sprintf(pool->dbpool[i].dpath, "%s:%s/%s/%s_%d",
			dict_type, STR(pool->partions[j].path), dict_path, dict_name, i);
		pool->partions[j].db_cnt++;
		pool->dbpool[i].partion = &pool->partions[j];
		j++;

		pool->dbpool[i].seqcnt = 0;
		if (two_db) {
			open_flags = O_WRONLY|O_CREAT|O_TRUNC;
			pool->dbpool[i].dict_write = dict_open(STR(pool->dbpool[i].dpath), open_flags, dict_flags);
			if (pool->dbpool[i].dict_write == NULL)
				acl_msg_fatal("%s(%d): open dict(%s) error",
					myname, __LINE__, STR(pool->dbpool[i].dpath));

			open_flags = O_RDONLY;
			pool->dbpool[i].dict_read = dict_open(STR(pool->dbpool[i].dpath), open_flags, dict_flags);
			if (pool->dbpool[i].dict_read == NULL)
				acl_msg_fatal("%s(%d): open dict(%s) error",
					myname, __LINE__, STR(pool->dbpool[i].dpath));
		} else {
			open_flags = O_CREAT | O_RDWR;
			pool->dbpool[i].dict_read = dict_open(STR(pool->dbpool[i].dpath), open_flags, dict_flags);
			if (pool->dbpool[i].dict_read == NULL)
				acl_msg_fatal("%s(%d): open dict(%s) error",
					myname, __LINE__, STR(pool->dbpool[i].dpath));

			pool->dbpool[i].dict_write = pool->dbpool[i].dict_read;
#if 0
			dict_register(STR(pool->dbpool[i].dpath), pool->dbpool[i].dict);
#endif
		}
	}

	pool->dict_name = acl_mystrdup(dict_name);
	pool->pool_size = pool_size;
	pool->hash_fn   = acl_hash_crc32;
	return (pool);
}

void dict_pool_free(DICT_POOL *pool)
{
	const char *myname = "dict_pool_free";
	int  i;

	if (pool == NULL)
		acl_msg_fatal("%s(%d): dict_name invalid", myname, __LINE__);

	for (i = 0; i < pool->pool_size; i++) {
#if 0
		dict_unregister(STR(pool->dbpool[i].dpath));
#endif
		if (pool->dbpool[i].dict_read == pool->dbpool[i].dict_write) {
			if (pool->dbpool[i].dict_read != NULL)
				DICT_CLOSE(pool->dbpool[i].dict_read);
		} else {
			if (pool->dbpool[i].dict_read != NULL)
				DICT_CLOSE(pool->dbpool[i].dict_read);
			if (pool->dbpool[i].dict_write != NULL)
				DICT_CLOSE(pool->dbpool[i].dict_write);
		}
		acl_vstring_free(pool->dbpool[i].dpath);
		acl_pthread_mutex_destroy(&pool->dbpool[i].lock);
	}

	for (i = 0; i < pool->partions_size; i++) {
		acl_vstring_free(pool->partions[i].path);
	}

	acl_myfree(pool->partions);
	acl_myfree(pool->dbpool);
	acl_myfree(pool->dict_name);
	acl_myfree(pool);
}

int  dict_pool_set(DICT_POOL *pool, char *key, size_t key_len, char *value, size_t len)
{
	unsigned int n;

	n = (pool->hash_fn(key, key_len)) % (pool->pool_size);
	dict_pool_db_lock(&pool->dbpool[n]);
	DICT_PUT(pool->dbpool[n].dict_write, key, key_len, value, len);
	pool->dbpool[n].partion->obj_cnt++;
	dict_pool_db_unlock(&pool->dbpool[n]);
	return (0);
}

char *dict_pool_get(DICT_POOL *pool, char *key, size_t key_len, size_t *size)
{
	const char *myname = "dict_pool_get";
	unsigned int n;
	char *value;

	n = (pool->hash_fn(key, key_len)) % (pool->pool_size);
	dict_pool_db_lock(&pool->dbpool[n]);
	if (DICT_GET(pool->dbpool[n].dict_read, key, key_len, &value, size) == NULL) {
		if (dict_errno == DICT_ERR_RETRY)
			acl_msg_error("%s(%d): soft error", myname, __LINE__);
	}
	dict_pool_db_unlock(&pool->dbpool[n]);
	return (value);
}

int  dict_pool_del(DICT_POOL *pool, char *key, size_t key_len)
{
	unsigned int n;
	int   ret;

	n = (pool->hash_fn(key, key_len)) % (pool->pool_size);
	dict_pool_db_lock(&pool->dbpool[n]);
	ret = DICT_DEL(pool->dbpool[n].dict_write, key, key_len);
	if (ret == 0)
		pool->dbpool[n].partion->obj_cnt--;
	dict_pool_db_unlock(&pool->dbpool[n]);
	return (ret);
}

void dict_pool_seq_reset(DICT_POOL *pool)
{
	int   i;

	pool->dict_cur = 0;

	for (i = 0; i < pool->pool_size; i++) {
		DICT_RESET(pool->dbpool[i].dict_read);
		pool->dbpool[i].seqcnt = 0;
	}
}

int dict_pool_seq_delcur(DICT_POOL *pool)
{
	const char *myname = "dict_pool_seq_delcur";
	int   ret;

	if (pool->dict_cur < 0)
		acl_msg_fatal("%s(%d), %s): dict_cur(%d) < 0",
			__FILE__, __LINE__, myname, pool->dict_cur);
	if (pool->dict_cur >= pool->pool_size)
		acl_msg_fatal("%s(%d), %s): dict_cur(%d) >= pool_size(%d)",
			__FILE__, __LINE__, myname,
			pool->dict_cur, pool->pool_size);
	dict_pool_db_lock(&pool->dbpool[pool->dict_cur]);
	ret = DICT_DELCUR(pool->dbpool[pool->dict_cur].dict_read);
	dict_pool_db_unlock(&pool->dbpool[pool->dict_cur]);
	return (ret);
}

int dict_pool_seq(DICT_POOL *pool, char **key, size_t *key_size,
	char **val, size_t *val_size)
{
	int   ret;
	
	while (1) {
		if (pool->dict_cur < 0)
			pool->dict_cur = 0;
		else if (pool->dict_cur >= pool->pool_size) {
			*key = NULL;
			*key_size = 0;
			*val = NULL;
			*val_size = 0;
			pool->dict_cur = 0;
			return (-1);
		}

		dict_pool_db_lock(&pool->dbpool[pool->dict_cur]);
		if (pool->dbpool[pool->dict_cur].seqcnt == 0)
			ret = DICT_SEQ(pool->dbpool[pool->dict_cur].dict_read,
				DICT_SEQ_FUN_FIRST, key, key_size, val, val_size);
		else
			ret = DICT_SEQ(pool->dbpool[pool->dict_cur].dict_read,
				DICT_SEQ_FUN_NEXT, key, key_size, val, val_size);
		dict_pool_db_unlock(&pool->dbpool[pool->dict_cur]);

		if (ret == 0) {
			pool->dbpool[pool->dict_cur].seqcnt++;
			return (0);
		}
		pool->dbpool[pool->dict_cur].seqcnt = 0;

		/* 返回非0值，有可能是当前的DB结点已经遍历完毕, xxx: 应该进一步判断才是 */
		pool->dict_cur++;
	}
}

DICT_POOL_DB *dict_pool_db(DICT_POOL *pool, const char *key, size_t key_len)
{
	unsigned int n;

	n = (pool->hash_fn(key, key_len)) % (pool->pool_size);
	return (&pool->dbpool[n]);
}

const char *dict_pool_db_path(DICT_POOL_DB *db)
{
	return (acl_vstring_str(db->dpath));
}

void dict_pool_db_lock(DICT_POOL_DB *db)
{
	acl_pthread_mutex_lock(&db->lock);
}

void dict_pool_db_unlock(DICT_POOL_DB *db)
{
	acl_pthread_mutex_unlock(&db->lock);
}

int  dict_pool_db_set(DICT_POOL_DB *db, char *key, size_t key_len, char *value, size_t len)
{
	DICT_PUT(db->dict_write, key, key_len, value, len);
	db->partion->obj_cnt++;
	return (0);
}

char *dict_pool_db_get(DICT_POOL_DB *db, char *key, size_t key_len, size_t *size)
{
	const char *myname = "dict_pool_db_get";
	char *value;

	if (DICT_GET(db->dict_read, key, key_len, &value, size) == NULL) {
		if (dict_errno == DICT_ERR_RETRY)
			acl_msg_error("%s(%d): soft error", myname, __LINE__);
	}
	return (value);
}

int dict_pool_db_del(DICT_POOL_DB *db, char *key, size_t key_len)
{
	int   ret;

	ret = DICT_DEL(db->dict_write, key, key_len);
	if (ret == 0)
		db->partion->obj_cnt--;
	return (ret);
}

void dict_pool_db_seq_reset(DICT_POOL_DB *db)
{
	DICT_RESET(db->dict_read);
}

int dict_pool_db_seq(DICT_POOL_DB *db, char **key, size_t *key_size,
	char **val, size_t *val_size)
{
	int   ret;

	if (db->seqcnt == 0)
		ret = DICT_SEQ(db->dict_read, DICT_SEQ_FUN_FIRST,
			key, key_size, val, val_size);
	else
		ret = DICT_SEQ(db->dict_read, DICT_SEQ_FUN_NEXT,
			key, key_size, val, val_size);
	if (ret == 0) {
		db->seqcnt++;
		return (0);
	}
	return (-1);
}

