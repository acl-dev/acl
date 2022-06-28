#include "lib_acl.h"
#include <assert.h>
#include "dict_pool.h"
#include "md5.h"
#include "dict_test.h"

static int   __add = 0, __search = 0, __seq = 0, __seqdel = 0, __sort = 0;
static acl_pthread_pool_t *__thrpool = NULL;
static acl_pthread_mutex_t __lock;
static int   __thrcnt = 32, __begin = 0, __max = 10000, __dbcnt = 6;
static int   __value_size = 100;
static int   __nwrite = 0, __nread = 0;
static int   __report_base = 10000;
static char  __key_pre[] = "key5key5key5key5key5key5key5key5key5key5key5key5key5key5";
static DICT_POOL *__dict_pool;

#define LOCK	acl_pthread_mutex_lock(&__lock)
#define UNLOCK  acl_pthread_mutex_unlock(&__lock)

static const char *__partions[] = {
	"./cache1",
	"./cache2",
	"./cache3",
	"./cache4"
};
static int   __partions_size = 4;

static void init(const char *dict_type, const char *dict_path, const char *dict_name)
{
	acl_init();

	__thrpool = acl_thread_pool_create(__thrcnt, 5);
	dict_pool_init();
	__dict_pool = dict_pool_new(__partions, __partions_size, dict_type,
			dict_path, dict_name, __dbcnt);
}

static void end(void)
{
	printf("input any key to close db\r\n");
	getchar();
	if (__dict_pool)
		dict_pool_free(__dict_pool);
}

#define	STR	acl_vstring_str
#define	LEN	ACL_VSTRING_LEN

typedef struct KEY {
	char *key;
} KEY;

static int cmp_fn(const void *ptr1, const void *ptr2)
{
	const KEY *key1 = (const KEY*) ptr1;
	const KEY *key2 = (const KEY*) ptr2;

	return (strcmp(key1->key, key2->key));
}

static void thread_write_fn(void *arg)
{
	char *id = (char*) arg;
	char  key[256], buf[256];
	ACL_VSTRING *value = acl_vstring_alloc(__value_size);
	time_t begin, last, now;
	DICT_POOL_DB *db;
	int   i, n, j;
	KEY *key_array;

	key_array = (KEY*) acl_mycalloc(__max, sizeof(KEY));

	n = __value_size;
	acl_vstring_sprintf(value, "%u:", (unsigned) acl_pthread_self());
	n -= LEN(value);
	for (i = 0; i < n; i++)
		ACL_VSTRING_ADDCH(value, 'v');
	ACL_VSTRING_TERMINATE(value);

	time(&begin);
	last = begin;
	for (j = 0, i = __begin; i < __begin + __max; i++) {
		snprintf(buf, sizeof(buf), "%s:%s:%d", __key_pre, id, i);
#if 0
		MDString(buf, key, sizeof(key));
#else
		acl_uint64 k = acl_hash_crc64(buf, strlen(buf));
		snprintf(key, sizeof(key), "%llu", k);
#endif
		key_array[j++].key = acl_mystrdup(key);
	}

	if (__max <= 100) {
		printf("before sort\n");
		for (i = 0; i < __max; i++) {
			printf("key[%d]: %s\n", i, key_array[i].key);
		}
	}

	if (__sort)
		qsort(key_array, __max, sizeof(KEY), cmp_fn);

	if (__max <= 100) {
		printf("after sort\n");
		for (i = 0; i < __max; i++) {
			printf("key[%d]: %s\n", i, key_array[i].key);
		}
	}

	for (i = 0; i < __max; i++) {
		char *ptr;
		size_t size;

		db = dict_pool_db(__dict_pool, key_array[i].key, strlen(key_array[i].key));
		dict_pool_db_lock(db);
		ptr = dict_pool_db_get(db, key_array[i].key, strlen(key_array[i].key), &size);
		if (ptr != NULL) {
			printf("key: %s exist now, size: %d\n", key_array[i].key, (int) size);
			acl_myfree(ptr);
		}
		dict_pool_db_set(db, key_array[i].key, strlen(key_array[i].key), STR(value), LEN(value));

		ptr = dict_pool_db_get(db, key_array[i].key, strlen(key_array[i].key), &size);
		if (ptr == NULL) {
			printf("key: %s not add into db\n", key_array[i].key);
		} else
			acl_myfree(ptr);
		dict_pool_db_unlock(db);

		if (i > 0 && i % __report_base == 0) {
			time(&now);
			printf("thread %u add one, i=%d, time=%ld, key=%s\r\n",
				(unsigned) acl_pthread_self(), i, now - last, key_array[i].key);
			last = now;
		}
		LOCK;
		__nwrite++;
		UNLOCK;
	}

	for (i = 0; i < __max; i++) {
		acl_myfree(key_array[i].key);
	}
	acl_myfree(key_array);
	acl_vstring_free(value);
	acl_myfree(id);
	printf("thread %u add over, i=%d, time=%ld\r\n",
		(unsigned) acl_pthread_self(), i, time(NULL) - begin);
}

static void thread_read_fn(void *arg)
{
	char *id = (char*) arg;
	char  key[256], *value, buf[256];
	size_t value_size;
	time_t begin, last, now;
	DICT_POOL_DB *db;
	int   i;

	time(&begin);
	last = begin;
	for (i = __begin; i < __begin + __max; i++) {
		snprintf(buf, sizeof(buf), "%s:%s:%i", __key_pre, id, i);
		acl_uint64 k = acl_hash_crc64(buf, strlen(buf));
		snprintf(key, sizeof(key), "%llu", k);

		db = dict_pool_db(__dict_pool, key, strlen(key));
		dict_pool_db_lock(db);
		value = dict_pool_db_get(db, key, strlen(key), &value_size);
		dict_pool_db_unlock(db);

		if (value == NULL) {
			acl_vstream_printf("%s: %s\n", key,
				dict_errno == DICT_ERR_RETRY ? "soft error" : "not found");
		} else {
			if (i > 0 && i % __report_base == 0) {
				time(&now);
				printf(">>key=%s, time=%ld\r\n", key, now - last);
				last = now;
			}
			acl_myfree(value);
			LOCK;
			__nread++;
			UNLOCK;
		}
	}
	printf("thread %u read over, i=%d, time=%ld\r\n",
		(unsigned) acl_pthread_self(), i, time(NULL) - begin);
	acl_myfree(id);
}

static void thread_seq_fn(void *arg acl_unused)
{
	char  *key, *value;
	size_t key_size, value_size;
	time_t begin, last, now;
	ACL_VSTRING *key_buf = acl_vstring_alloc(100);
	int   i = 0;

	time(&begin);
	last = begin;
	while (1) {
		if (dict_pool_seq(__dict_pool, &key, &key_size, &value, &value_size) != 0)
			break;
		i++;
		if (i > 0 && i % __report_base == 0) {
			time(&now);
			acl_vstring_memcpy(key_buf, key, key_size);
			ACL_VSTRING_TERMINATE(key_buf);
			printf(">>key: %s, time=%ld\r\n", STR(key_buf), now - last);
			last = now;
		}
		acl_myfree(key);
		acl_myfree(value);
		LOCK;
		__nread++;
		UNLOCK;
	}

	acl_vstring_free(key_buf);
	printf("thread %u seq read over, i=%d, time=%ld\r\n",
		(unsigned) acl_pthread_self(), i, time(NULL) - begin);
}

static void thread_seqdel_fn(void *arg acl_unused)
{
	char  *key, *value;
	size_t key_size, value_size;
	time_t begin, last, now;
	ACL_VSTRING *key_buf = acl_vstring_alloc(100);
	int   i = 0;

	time(&begin);
	last = begin;
	while (1) {
		if (dict_pool_seq(__dict_pool, &key, &key_size, &value, &value_size) != 0)
			break;
		i++;
		if (i > 0 && i % __report_base == 0) {
			time(&now);
			acl_vstring_memcpy(key_buf, key, key_size);
			ACL_VSTRING_TERMINATE(key_buf);
			printf(">>key: %s, time=%ld\r\n", STR(key_buf), now - last);
			last = now;
		}
		if (i > 0 && i % 2 == 0)
			dict_pool_seq_delcur(__dict_pool);
		free(key);
		free(value);
		LOCK;
		__nread++;
		UNLOCK;
	}

	acl_vstring_free(key_buf);
	printf("thread %u seq read over, i=%d, time=%ld\r\n",
		(unsigned) acl_pthread_self(), i, time(NULL) - begin);
}

static void run(void)
{
	int   i;
	char *id;

	if (__add) {
		acl_vstream_printf("start write thread...\r\n");
		for (i = 0; i < __thrcnt; i++) {
			id = acl_mymalloc(10);
			sprintf(id, "%d", i);
			printf("add one thread i=%d\n", i);
			acl_pthread_pool_add(__thrpool, thread_write_fn, id);
			printf("add one thread ok, i=%d\n", i);
		}
		while (1) {
			i = acl_pthread_pool_size(__thrpool);
			if (i == 0)
				break;
			printf("> current threads in thread pool is: %d, nwrite=%d\r\n", i, __nwrite);
			sleep(1);
		}
		acl_vstream_printf("write threads exit now\r\n");
	}

	if (__search) {
		acl_vstream_printf("start read thread...\r\n");
		for (i = 0; i < __thrcnt; i++) {
			id = acl_mymalloc(10);
			sprintf(id, "%d", i);
			acl_pthread_pool_add(__thrpool, thread_read_fn, id);
		}

		while (1) {
			i = acl_pthread_pool_size(__thrpool);
			if (i == 0)
				break;
			printf("> current threads in thread pool is: %d, nread=%d\r\n", i, __nread);
			sleep(1);
		}
		acl_vstream_printf("read threads exit now\r\n");
	}

	if (__seq) {
		acl_vstream_printf("start seq thread ...\r\n");
		for (i = 0; i < __thrcnt; i++) {
			acl_pthread_pool_add(__thrpool, thread_seq_fn, NULL);
		}

		while (1) {
			i = acl_pthread_pool_size(__thrpool);
			if (i == 0)
				break;
			printf("> current threads in thread pool is: %d, nread=%d\r\n", i, __nread);
			sleep(1);
		}
		acl_vstream_printf("seq read threads exit now\r\n");
	}

	if (__seqdel) {
		acl_vstream_printf("start seqdel thread ...\r\n");
		for (i = 0; i < __thrcnt; i++) {
			acl_pthread_pool_add(__thrpool, thread_seqdel_fn, NULL);
		}

		while (1) {
			i = acl_pthread_pool_size(__thrpool);
			if (i == 0)
				break;
			printf("> current threads in thread pool is: %d, nread=%d\r\n", i, __nread);
			sleep(1);
		}
		acl_vstream_printf("seqdel read threads exit now\r\n");
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -a [sort the key] -p page_size -c cache_size\r\n"
		"-o read|rw  -f from -n count -t thread_count -d db_count\r\n"
		"-o add -f from -n count -t thread_count -d db_count -s value_size\r\n"
		"-o seq -t thread_count\r\n"
		"-o seqdel -t thread_count\r\n"
		"-o test type:file read|write|create [fold]\r\n"
		"-o test_number -n count\r\n",
		procname);	
}

int main(int argc, char *argv[])
{
#if 1
	const char *dict_type = "btree";
	const char *dict_path = "dbpath";
	const char *dict_name = "test";
#else
	const char *dict_type = "cdb";
	const char *dict_path = "dbpath";
	const char *dict_name = "test";
#endif
	char  ch, oper[32];

	oper[0] = 0;
	while ((ch = getopt(argc, argv, "ho:n:f:t:d:s:ap:c:")) > 0) {
		switch (ch) {
			case 'h':
				usage(argv[0]);
				exit (0);
			case 'o':
				ACL_SAFE_STRNCPY(oper, optarg, sizeof(oper));
				break;
			case 'n':
				__max = atoi(optarg);
				if (__max < 0)
					__max = 10000;
				break;
			case 'f':
				__begin = atoi(optarg);
				if (__begin < 0)
					__begin = 0;
				break;
			case 't':
				__thrcnt = atoi(optarg);
				if (__thrcnt <= 0 || __thrcnt >= 1000)
					__thrcnt = 1;
				break;
			case 'd':
				__dbcnt = atoi(optarg);
				if (__dbcnt <= 0 || __dbcnt > 256)
					__dbcnt = 32;
				break;
			case 's':
				__value_size = atoi(optarg);
				if (__value_size <= 0)
					__value_size = 100;
				break;
			case 'a':
				__sort = 1;
				break;
			case 'p':
				dict_db_page_size = atoi(optarg);
				break;
			case 'c':
				dict_db_cache_size = atoi(optarg);
				break;
			default:
				usage(argv[0]);
				exit (0);
		}
	}
	if (strcasecmp(oper, "test") == 0) {
		dict_test_main(argc, argv);
		return (0);
	} else if (strcasecmp(oper, "test_number") == 0) {
		dict_number_main(__max);
		return (0);
	} else if (strcasecmp(oper, "add") == 0) {
		__add = 1;
	} else if (strcasecmp(oper, "read") == 0) {
		__search = 1;
	} else if (strcasecmp(oper, "rw") == 0) {
		__add = 1;
		__search = 1;
	} else if (strcasecmp(oper, "seq") == 0) {
		__seq = 1;
	} else if (strcasecmp(oper, "seqdel") == 0) {
		__seqdel = 1;
	} else {
		usage(argv[0]);
		exit (0);
	}

	init(dict_type, dict_path, dict_name);
	run();
	end();
	printf("input any key to exit now\r\n");
	getchar();
	return (0);
}
