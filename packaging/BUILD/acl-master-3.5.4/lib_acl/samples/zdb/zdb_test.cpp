#include "lib_acl.h"
#ifndef WIN32
#include <algorithm>
#include <vector>
#endif
#include "db/zdb.h"
#include "zdb_test.h"
#include "md5.h"

#pragma pack(4)
typedef struct {
	int  n;
	char s[11];
} DUMMY;
#pragma pack(8)

#ifndef	STORE_PATH
#define	STORE_PATH(s)	ACL_FHANDLE_PATH(&(s)->fhandle)
#endif

struct RANGE {
	int from;
	int to;
};

#ifdef WIN32
#define snprintf _snprintf
#endif

/****************************************************************************/

#ifdef WIN32
static const char *var_md5_key = "hello world!";
#endif

static const char *var_str = "hi!";

/* 模拟取得一个随机的整形数组 */

#ifndef WIN32

static int *random_get(int max, int mod)
{
	int  *a = (int*) acl_mymalloc(sizeof(int) * max);
	int   n = 1;

	if (mod > 1) {
		max /= mod;
		mod--;
	} else
		mod = 0;

	for (int i = 0; i < max; i++)
		a[i] = i + mod;

	printf("shuffle: %d\n", n++);
	std::random_shuffle(a, a + max);

	printf("shuffle: %d\n", n++);
	std::random_shuffle(a, a + max);

	printf("shuffle: %d\n", n++);
	std::random_shuffle(a, a + max);

	printf("shuffle: %d\n", n++);
	std::random_shuffle(a, a + max);

	return (a);
}

#else
static int *random_get(int max, int mod)
{
	int   i, j, k, *a, key, off = 0;
	char  buf[64], key_buf[16];
	int   m = 0;

	/* 先分配一个大数组 */
	a = (int*) acl_mymalloc(sizeof(int) * max);

	/* 初始化该大数组 */
	memset(a, -1, sizeof(int) * max);

	printf("begin: max: %d\n", max);

	for (i = 0; i < max; i++) {
		snprintf(buf, sizeof(buf), "key: %d", i);

		/* 先 mda5 */
		MD5Key(buf, strlen(buf), var_md5_key, strlen(var_md5_key),
			key_buf, sizeof(key_buf));

		/* 再取模 */
		key = acl_hash_crc32(key_buf, sizeof(key_buf)) % max;
		if (a[key] == -1) {
			a[key] = i;
			continue;
		}

		acl_assert(a[key] != i);

//		printf("key: %d, %d\n", i, a[key]);

		m++;

		/* 说明该 key 的数组下标已经被占用，需要顺序找一个位置即可 */
		for (j = off; j < max; j++) {
			if (a[j] == -1) {
				a[j] = i;
				for (k = j + 1; k < max; k++) {
					if (a[k] == -1) {
						off = k;
						break;
					}
				}
				break;
			}
		}
		acl_assert(j != max);
	}

	printf("end, m: %d\n", m);
	return (a);
}
#endif

static void random_free(int *a)
{
	acl_myfree(a);
}

/* 随机批量写 */

static int bench_random_add(ZDB *db, int max, int mod)
{
	DUMMY dummy;
	int   i, ret, *a;
	time_t begin;

	ACL_SAFE_STRNCPY(dummy.s, var_str, sizeof(dummy.s));

	a = random_get(max, mod);

	if (mod > 1)
		max /= mod;

	time(&begin);

	printf("---range_random_add: >> max: %d, ", max);
	ACL_METER_TIME(" ");

	for (i = 0; i < max; i++) {
		dummy.n = i;
		ret = zdb_update(db, a[i], NULL, &dummy, sizeof(dummy));
		if (ret < 0) {
			printf("zdb_udpate error, i: %d, key: %d\n", i, a[i]);
			random_free(a);
			return (-1);
		}
		if (i > 0 && i % 100000 == 0) {
			printf("---%s: >> current: i: %d, key: %d, ",
				__FUNCTION__, i, a[i]);
			ACL_METER_TIME(" ");
		}
	}

	printf("---range_add: >> end: current: %d, max: %d, ", i, max);
	ACL_METER_TIME(" ");

	printf("================= update ok, time: %ld ===============\n",
		(long) (time(NULL) - begin));

	random_free(a);
	return (max);
}

/* 随机批量读 */

static int bench_random_get(ZDB *db, int max, int mod)
{
	DUMMY *dummy;
	int   i, *a;
	size_t size;
	ZDB_BLK *blk;
	ZDB_BLK_OFF blk_off;
	time_t begin;
	char  buf[64];

	a = random_get(max, mod);

	if (mod > 1)
		max /= mod;

	time(&begin);

	printf("---range_random_get: >> begin: max: %d, ", max);
	ACL_METER_TIME(" ");

	for (i = 0; i < max; i++) {
		snprintf(buf, sizeof(buf), "key: %d", i);
		blk = zdb_lookup(db, a[i], &size, &blk_off);
		if (blk == NULL) {
			printf("zdb_lookup error, i: %d\n", i);
			random_free(a);
			return (-1);
		}
		dummy = (DUMMY*) zdb_blk_data(blk);
		if (i > 0 && i % 100000 == 0) {
			printf("---%s: >> current: i: %d, n: %d,"
				" key: %d, blk_off: %lld, ", __FUNCTION__,
				i, dummy->n, a[i], blk_off.offset);
			ACL_METER_TIME(" ");
		}
		zdb_blk_free(blk);
	}

	printf("---range_get: >> end: current: %d, max: %d, ", i, max);
	ACL_METER_TIME(" ");

	printf("================= lookup ok, time: %ld ===============\n",
		(long) (time(NULL) - begin));

	random_free(a);
	return (max);
}

/****************************************************************************/

/* 顺序批量写 */

static int bench_add(ZDB *db, int max)
{
	time_t begin;
	int   i, ret, delta = 100000;
	DUMMY dummy;

	if (max <= 100)
		delta = 0;
	time(&begin);

	ACL_SAFE_STRNCPY(dummy.s, var_str, sizeof(dummy.s));

	ACL_METER_TIME("----begin---");
	for (i = 0; i < max; i++) {
		dummy.n = i;
		ret = zdb_update(db, i, NULL, &dummy, sizeof(dummy));
		if (ret < 0) {
			printf("zdb_udpate error, i: %d\n", i);
			return (-1);
		}
		if (delta > 0 && i % delta == 0) {
			printf("---%s: i: %d, ", __FUNCTION__, i);
			ACL_METER_TIME("-----");
		} else if (delta == 0)
			printf("---%s: i: %d\n", __FUNCTION__, i);
	}
	printf("---%s: max: %d, ", __FUNCTION__, max);
	ACL_METER_TIME("----end---");

	printf("================= add ok, time: %ld ===============\n",
		(long) (time(NULL) - begin));

	return (max);
}

/* 顺序批量修改 */

static int bench_update(ZDB *db, int max, int num)
{
	time_t begin;
	int   i, ret, j, delta = 100000;
	DUMMY *dummy;

	if (max <= 100)
		delta = 0;

	time(&begin);

	dummy = (DUMMY*) acl_mycalloc(num, sizeof(DUMMY));
	for (i = 0; i < num; i++) {
		snprintf(dummy[i].s, sizeof(dummy[i].s), "%d", (int) i);
	}

	ACL_METER_TIME("----begin---");
	for (i = 0; i < max; i++) {
		for (j = 0; j < num; j++)
			dummy[j].n = 0;
		ret = zdb_update(db, i, NULL, dummy, num * sizeof(DUMMY));
		if (ret < 0) {
			printf("zdb_udpate error, i: %d\n", i);
			return (-1);
		}
		if (delta > 0 && i % delta == 0) {
			printf("---%s: i: %d, ", __FUNCTION__, i);
			ACL_METER_TIME("-----");
		} else if (delta == 0)
			printf("---%s: i: %d\n", __FUNCTION__, i);
	}
	printf("---%s: max: %d, ", __FUNCTION__, max);
	ACL_METER_TIME("----end---");

	printf("================= update ok, time: %ld ===============\n",
		(long) (time(NULL) - begin));

	acl_myfree(dummy);

	return (max);
}

/* 顺序批量读 */

static int bench_get(ZDB *db, int max)
{
	ZDB_BLK *blk;
	DUMMY *dummy;
	time_t begin;
	size_t size;
	int   i, delta = 100000;
	ZDB_BLK_OFF blk_off;

	if (max <= 100)
		delta = 0;

	printf(">> max: %d, delta: %d\n", max, delta);
	time(&begin);
	ACL_METER_TIME("----begin---");

	//for (i = 0; i < max; i++) {
	for (i = max - 1; i >= 0; i--) {
		blk = zdb_lookup(db, i, &size, &blk_off);
		if (blk == NULL) {
			printf("zdb_lookup error, i: %d\n", i);
			return (-1);
		}

		dummy = (DUMMY*) zdb_blk_data(blk);
		if (delta == 0)
			printf("---%s: i: %d, n: %d, %s, size: %d,"
				" blk_off: %lld, \n", __FUNCTION__,
				i, dummy[0].n, dummy[0].s,
				(int) size, blk_off.offset);
		else if (delta > 0 && i % delta == 0) {
			printf("---%s: i: %d, n: %d, %s, size: %d,"
				" blk_off: %lld, ", __FUNCTION__,
				i, dummy[0].n, dummy[0].s,
				(int) size, blk_off.offset);
			ACL_METER_TIME("");
		}

		zdb_blk_free(blk);
	}
	printf("---%s: max: %d, ", __FUNCTION__, max);
	ACL_METER_TIME("----end---");

	printf("================= lookup ok, time: %ld ===============\n",
		(long) (time(NULL) - begin));
	return (max);
}

/****************************************************************************/

static int zdb_test1(ZDB *db)
{
	DUMMY dummy[2], *pdummy;
	ZDB_BLK *blk;
	size_t size;
	zdb_key_t key = 10000;
	int   ret, i;

	dummy[0].n = (int) key;
	ACL_SAFE_STRNCPY(dummy[0].s, var_str, sizeof(dummy[0].s));
	dummy[1].n = (int) key + 1;
	ACL_SAFE_STRNCPY(dummy[1].s, var_str, sizeof(dummy[1].s));

	printf(">>> first update, key: %lld, ", key);
	ret = zdb_update(db, key, NULL, &dummy, sizeof(dummy) / 2);
	printf("zdb_update: return %d\n\n", ret);

	printf(">>> second update, key: %lld, ", key);
	ret = zdb_update(db, key, NULL, &dummy, sizeof(dummy) / 2);

	blk = zdb_lookup(db, key, &size, NULL);
	if (blk) {
		printf("zdb_lookup: key(%lld), size: %u\n", key, (unsigned) size);

		pdummy = (DUMMY*) zdb_blk_data(blk);
		for (i = 0; i < (int) (size / sizeof(DUMMY)); i++) {
			printf("zdb_lookup: n: %d, s: %s\n", pdummy->n, pdummy->s);
			pdummy++;
		}
		zdb_blk_free(blk);
	} else
		printf("zdb_lookup: key(%lld), return null\n", key);

	printf(">>> third update, key: %lld\n", key);
	ret = zdb_update(db, key, NULL, &dummy, sizeof(dummy) / 2);

	return (ret);
}

static int zdb_test2(ZDB *db)
{
	DUMMY dummy1[1], dummy2[2], dummy3[3], *pdummy;
	ZDB_BLK *blk;
	int  key = 100, ret;
	size_t size, i;

	dummy1[0].n = key;
	ACL_SAFE_STRNCPY(dummy1[0].s, "1: hello world!", sizeof(dummy1[0].s));

	dummy2[0].n = key;
	ACL_SAFE_STRNCPY(dummy2[0].s, "2: hello world!", sizeof(dummy2[0].s));
	dummy2[1].n = key;
	ACL_SAFE_STRNCPY(dummy2[1].s, "2: hello world!", sizeof(dummy2[1].s));

	dummy3[0].n = key;
	ACL_SAFE_STRNCPY(dummy3[0].s, "3: hello world!", sizeof(dummy3[0].s));
	dummy3[1].n = key;
	ACL_SAFE_STRNCPY(dummy3[1].s, "3: hello world!", sizeof(dummy3[1].s));
	dummy3[2].n = key;
	ACL_SAFE_STRNCPY(dummy3[2].s, "3: hello world!", sizeof(dummy3[2].s));

	ret = zdb_update(db, key, NULL, &dummy1, sizeof(dummy1));
	if (ret < 0) {
		printf("zdb_update: set dummy1 error\n");
		return (-1);
	}
	blk = zdb_lookup(db, key, &size, NULL);
	if (blk == NULL) {
		printf("zdb_lookup: get dummy1 error\n");
		return (-1);
	}
	pdummy = (DUMMY*) zdb_blk_data(blk);
	for (i = 0; i < sizeof(dummy1) / sizeof(DUMMY); i++) {
		printf("dummy1: key: %d, n: %d, s: %s\n", key, pdummy->n, pdummy->s);
		pdummy++;
	}
	zdb_blk_free(blk);

	ret = zdb_update(db, key, NULL, &dummy2, sizeof(dummy2));
	if (ret < 0) {
		printf("zdb_update: set dummy2 error\n");
		return (-1);
	}
	blk = zdb_lookup(db, key, &size, NULL);
	if (blk == NULL) {
		printf("zdb_lookup: get dummy2 error\n");
		return (-1);
	}
	pdummy = (DUMMY*) zdb_blk_data(blk);
	for (i = 0; i < sizeof(dummy2) / sizeof(DUMMY); i++) {
		printf("dummy2: key: %d, n: %d, s: %s\n", key, pdummy->n, pdummy->s);
		pdummy++;
	}
	zdb_blk_free(blk);

	ret = zdb_update(db, key, NULL, &dummy3, sizeof(dummy3));
	if (ret < 0) {
		printf("zdb_update: set dummy3 error\n");
		return (-1);
	}
	blk = zdb_lookup(db, key, &size, NULL);
	if (blk == NULL) {
		printf("zdb_lookup: get dummy3 error\n");
		return (-1);
	}
	pdummy = (DUMMY*) zdb_blk_data(blk);
	for (i = 0; i < sizeof(dummy3) / sizeof(DUMMY); i++) {
		printf("dummy3: key: %d, n: %d, s: %s\n", key, pdummy->n, pdummy->s);
		pdummy++;
	}
	zdb_blk_free(blk);

	return (3);
}

static int key_walk_fn(ZDB_KEY_STORE *store)
{
	const char *myname = "key_walk_fn";
	ACL_ITER iter;
	ZDB_BLK_OFF *blk_off;
	int   i = 0;

	printf(">>> acl_foreach for %s now:\n", STORE_PATH((ZDB_STORE*) store));
	acl_foreach(iter, (ZDB_STORE*) store) {
		blk_off = (ZDB_BLK_OFF*) iter.data;
		if (i > 0 && (i % 100000) == 0) {
			printf("%s: i:%d, key: %d, blk_off: %lld, inode: %d, ",
					myname, i, iter.i, blk_off->offset, blk_off->inode);
			ACL_METER_TIME("-");
		}
		i++;
	}
	return (i);
}

static int test_zdb_key_walk(ZDB *db)
{
	const char *myname = "test_zdb_key_walk";
	int   ret;
	time_t begin;

	time(&begin);
	ret = zdb_key_walk(db, key_walk_fn);
	printf(">>>%s, acl_foreach for key, total: %d, time: %ld\n",
		myname, ret, (long) (time(NULL) - begin));
	return (0);
}

static int dat_walk_fn(ZDB_DAT_STORE *store)
{
	const char *myname = "dat_walk_fn";
	int   i = 0;
	ZDB_BLK *blk;
	DUMMY *dummy;
	ACL_ITER iter;

	printf(">>>%s, acl_foreach for %s now:\n", myname, STORE_PATH((ZDB_STORE*) store));

	acl_foreach(iter, (ZDB_STORE*) store) {
		blk = (ZDB_BLK*) iter.data;
		dummy = (DUMMY*) zdb_blk_data(blk);
		if (i > 0 && (i % 100000) == 0) {
			printf("%s: i:%d,key:%lld,data:%s,inext:%d,",
				myname, i, blk->hdr.key, dummy->s, iter.i);
			ACL_METER_TIME("-");
		} else if (store->hdr.count <= 100) {
			printf("%s: i:%d, key:%lld, data:%s, inext:%d\n",
				myname, i, blk->hdr.key, dummy->s, iter.i);
		}
		i++;
	}

	return  (i);
}

static int test_zdb_dat_walk(ZDB *db)
{
	const char *myname = "test_zdb_dat_walk";
	time_t begin;
	int   ret;

	time(&begin);
	ret = zdb_dat_walk(db, dat_walk_fn);
	printf(">>>%s, acl_foreach for dat, total: %d, time: %ld\n",
		myname, ret, (long) (time(NULL) - begin));
	return (ret);
}

static int dat_stat_fn(ZDB_DAT_STORE *store)
{
	const char *myname = "dat_stat_fn";
	ZDB_DAT_HDR hdr;

	memcpy(&hdr, &store->hdr, sizeof(ZDB_DAT_HDR));

	printf("%s(%d): %s's hdr status\n",
		myname, __LINE__, STORE_PATH((ZDB_STORE*) store));
	printf("%s(%d):\tlimit: %lld, size: %lld, used count: %lld\n",
		myname, __LINE__, hdr.limit, hdr.size, hdr.count);
	printf("%s(%d):\tnstep: %d, blk_hdr_dlen: %d, blk_dlen: %d,"
		" blk_count: %d\n", myname, __LINE__, hdr.nstep,
		hdr.blk_hdr_dlen, hdr.blk_dlen, hdr.blk_count);
	printf("%s(%d):\tihead_idle: %d, ihead_busy: %d\n",
		myname, __LINE__, hdr.ihead_idle, hdr.ihead_busy);

	return (0);
}

static int test_zdb_dat_stat(ZDB *db)
{
	return (zdb_dat_walk(db, dat_stat_fn));
}

static int key_stat_fn(ZDB_KEY_STORE *store)
{
	const char *myname = "key_stat_fn";
	ZDB_KEY_HDR key_hdr;

	memcpy(&key_hdr, &store->hdr, sizeof(ZDB_KEY_HDR));

	printf(">> %s: %s's status:\n", myname, STORE_PATH((ZDB_STORE*) store));
	printf(">>\tkey_limit: %lld, key_count: %lld, key_begin: %lld\n",
		key_hdr.key_limit, key_hdr.key_count, key_hdr.key_begin);

	return (0);
}

static int test_zdb_key_stat(ZDB *db)
{
	return (zdb_key_walk(db, key_stat_fn));
}

static int dat_check_fn(ZDB_DAT_STORE *store)
{
	return (zdb_dat_check(store, NULL));
}

static int test_zdb_dat_check(ZDB *db)
{
	return (zdb_dat_walk(db, dat_check_fn));
}

static int key_check_fn(ZDB_KEY_STORE *store)
{
	return (zdb_key_check(store, NULL));
}

static int test_zdb_key_check(ZDB *db)
{
	return (zdb_key_walk(db, key_check_fn));
}

void zdb_test_main(const char *cmd)
{
	const char *myname = "zdb_test";
	ZDB *db;
	const char *dbname = "test", *key_path = "./var";

	/*ZDB_FLAG_CACHE_DAT | ZDB_FLAG_CACHE_KEY | ZDB_FLAG_SLICE_KEY | ZDB_FLAG_SLICE_DAT;*/
	unsigned int oflags = 0;

	ZDB_CFG zdb_cfg;
	zdb_key_t key_begin = 0, key_limit = 1000000, dat_limit = 10000000, blk_dlen = sizeof(DUMMY);
	int   ret, max = 10000000, use_random = 0, walk_all = 0, dat_nstep = 1000, mod = 1;
	int   cache_key_limit = 0, cache_key_timeout = 0;
	int   cmd_add = 0, cmd_get = 0, cmd_update = 0, update_num = 2;
	int   cmd_stat = 0, cmd_check = 0;
	ACL_ARGV *argv = acl_argv_split(cmd, ":");
	ACL_ITER iter;
	char *name, *value, *ptr;
	char  usage[] = "help:stat:check:link:random:walk:add:get:update={d}"
		":max={d}:key_limit={d}:dat_limit={d}:nstep={d}:mod={d}:cache_key_limit={d}"
		":cache_key_timeout={d}";

	acl_make_dirs(key_path, 0700);

	acl_foreach(iter, argv) {
		name = ptr = (char*) iter.data;
		if (strcasecmp(name, "help") == 0) {
			printf("usage: %s\n", usage);
			acl_argv_free(argv);
			return;
		} else if (strcasecmp(name, "stat") == 0) {
			cmd_stat = 1;
			continue;
		} else if (strcasecmp(name, "check") == 0) {
			cmd_check = 1;
			continue;
		} else if (strcasecmp(name, "link") == 0) {
			oflags |= ZDB_FLAG_LINK_BUSY;
			continue;
		} else if (strcasecmp(name, "random") == 0) {
			use_random = 1;
			continue;
		} else if (strcasecmp(name, "walk") == 0) {
			walk_all = 1;
			continue;
		} else if (strcasecmp(name, "add") == 0) {
			cmd_add = 1;
			continue;
		} else if (strcasecmp(name, "get") == 0) {
			cmd_get = 1;
			continue;
		}

		name = acl_mystrtok(&ptr, "=");
		if (name == NULL)
			continue;
		value = acl_mystrtok(&ptr, "=");
		if (value == NULL)
			continue;
		if (strcasecmp(name, "max") == 0) {
			ret = atoi(value);
			if (ret > 0)
				max = ret;
		} else if (strcasecmp(name, "key_limit") == 0) {
			ret = atoi(value);
			if (ret > 0)
				key_limit = ret;
		} else if (strcasecmp(name, "dat_limit") == 0) {
			ret = atoi(value);
			if (ret > 0)
				dat_limit = ret;
		} else if (strcasecmp(name, "update") == 0) {
			ret = atoi(value);
			if (ret > 0) {
				update_num = ret;
				cmd_update = 1;
			}
		} else if (strcasecmp(name, "nstep") == 0) {
			ret = atoi(value);
			if (ret > 0)
				dat_nstep = ret;
		} else if (strcasecmp(name, "mod") == 0) {
			ret = atoi(value);
			if (ret > 0)
				mod = ret;
		} else if (strcasecmp(name, "cache_key_limit") == 0) {
			ret = atoi(value);
			if (ret > 0)
				cache_key_limit = ret;
		} else if (strcasecmp(name, "cache_key_timeout") == 0) {
			ret = atoi(value);
			if (ret > 0)
				cache_key_timeout = ret;
		}
	}

	acl_argv_free(argv);

	memset(&zdb_cfg, 0, sizeof(zdb_cfg));

	/* 初始化 ZDB 存储引擎 */
	zdb_init();

	/* 开始配置 ZDB 配置对象 */
	zdb_cfg.key_path = key_path;
	zdb_cfg.key_begin = key_begin;
	zdb_cfg.key_limit = key_limit;
	zdb_cfg.dat_limit = dat_limit;
	zdb_cfg.blk_dlen = (int) blk_dlen;
	zdb_cfg.dat_nstep = dat_nstep;  /* 值存储文件每次增加的数据块个数 */
	if (cache_key_limit > 0 && cache_key_timeout > 0) {
		zdb_cfg.key_cache_max = cache_key_limit * 2;
		zdb_cfg.key_cache_timeout = cache_key_timeout;
		zdb_cfg.key_wback_max = cache_key_limit;
		oflags |= ZDB_FLAG_CACHE_KEY | ZDB_FLAG_SLICE_KEY;
		printf("===========open key\n");
	} else {
		zdb_cfg.key_cache_max = 0;
		zdb_cfg.key_cache_timeout = 0;
		zdb_cfg.key_wback_max = 0;
	}
	zdb_cfg.dat_cache_max = 0;
	zdb_cfg.dat_cache_timeout = 0;
	zdb_cfg.dat_wback_max = 0;

	/* 打开一个 ZDB 数据库对象 */
	db = zdb_open(dbname, oflags, &zdb_cfg);
	if (db == NULL)
		acl_msg_fatal("%s: zdb open error(%s)", myname, acl_last_serror());

	printf("DUMMY.sizeof: %d\n", (int) sizeof(DUMMY));

	if (0) {
		ret = zdb_test1(db);
		if (ret > 0)
			ret = zdb_test2(db);
	} else
		ret = 1;

	if (!use_random) {
		if (ret > 0 && cmd_add)
			ret = bench_add(db, max);
		if (ret > 0 && cmd_get)
			ret = bench_get(db, max);
		if (ret > 0 && cmd_update)
			ret = bench_update(db, max, update_num);
	} else {
		if (ret > 0 && cmd_add)
			ret = bench_random_add(db, max, mod);
		if (ret > 0 && cmd_get)
			ret = bench_random_get(db, max, mod);
	}

	if (walk_all) {
		if (ret > 0)
			ret = test_zdb_dat_walk(db);
		if (ret >= 0)
			ret = test_zdb_key_walk(db);
	}

	if (cmd_stat) {
		if (ret >= 0)
			ret = test_zdb_dat_stat(db);
		if (ret >= 0)
			ret = test_zdb_key_stat(db);
	}

	if (cmd_check) {
		if (ret >= 0)
			ret = test_zdb_dat_check(db);
		if (ret >= 0)
			ret = test_zdb_key_check(db);
	}

	zdb_close(db);
	zdb_end();
}
