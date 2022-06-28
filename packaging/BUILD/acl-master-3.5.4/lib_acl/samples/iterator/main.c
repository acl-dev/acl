#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef WIN32
#define snprintf _snprintf
#endif

static void binhash_iter(void)
{
	ACL_BINHASH *table = acl_binhash_create(10, 0);
	ACL_BINHASH_ITER iter;
	char  key[32], *value;
	int   i;

	for (i = 0; i < 10; i++) {
		snprintf(key, sizeof(key), "key: %d", i);
		value = acl_mymalloc(32);
		snprintf(value, 32, "value: %d", i);
		assert(acl_binhash_enter(table, key, (int) strlen(key) + 1, value));
	}

	i = 0;
	printf("\n>>>acl_binhash_foreach:\n");
	acl_binhash_foreach(iter, table) {
		printf("hash i=%d, [%s]=[%s]\n",
			iter.i, (const char*) acl_binhash_iter_key(iter),
			(char*) acl_binhash_iter_value(iter));
		i++;
	}

	printf(">>>total: %d\n", i);

	i = 0;
	printf("\n>>>acl_binhash_foreach and break:\n");
	acl_binhash_foreach(iter, table) {
		printf("hash i=%d, [%s]=[%s]\n",
			iter.i, (const char*) acl_binhash_iter_key(iter),
			(char*) acl_binhash_iter_value(iter));
		i++;
		if (i == 5) {
			printf("i = %d, break now\n", i);
			break;
		}
	}

	printf("\n>>>acl_binhash_foreach_reverse:\n");
	acl_binhash_foreach_reverse(iter, table) {
		printf("hash i=%d, [%s]=[%s]\n",
			iter.i, (const char*) acl_binhash_iter_key(iter),
			(char*) acl_binhash_iter_value(iter));
		i++;
	}

	acl_binhash_free(table, acl_myfree_fn);
}

static void htable_iter(void)
{
	ACL_SLICE_POOL *slice = acl_slice_pool_create(10, 100,
		ACL_SLICE_FLAG_GC2 | ACL_SLICE_FLAG_RTGC_OFF);
	ACL_HTABLE *table = acl_htable_create3(10, 0, slice);
	ACL_HTABLE_ITER iter;
	char  key[32], *value;
	int   i;

	for (i = 0; i < 20; i++) {
		snprintf(key, sizeof(key), "key: %d", i);
		value = acl_mymalloc(32);
		snprintf(value, 32, "value: %d", i);
		assert(acl_htable_enter(table, key, value));
	}

	i = 0;
	printf("\n>>>acl_htable_foreach:\n");
	acl_htable_foreach(iter, table) {
		printf("hash i=%d, [%s]=[%s]\n",
			iter.i, acl_htable_iter_key(iter),
			(char*) acl_htable_iter_value(iter));
		i++;
	}

	printf(">>>total: %d\n", i);

	i = 0;
	acl_htable_foreach(iter, table) {
		printf("hash i=%d, [%s]=[%s]\n",
			iter.i, acl_htable_iter_key(iter),
			(char*) acl_htable_iter_value(iter));
		i++;
		if (i == 5) {
			printf("i = %d, break now\n", i);
			break;
		}
	}


	i = 0;
	printf("\n>>>acl_htable_foreach_reverse:\n");
	acl_htable_foreach_reverse(iter, table) {
		printf("hash i=%d, [%s]=[%s]\n",
			iter.i, acl_htable_iter_key(iter),
			(char*) acl_htable_iter_value(iter));
		i++;
	}

	printf(">>>total: %d\n", i);

	acl_htable_stat(table);
	acl_htable_free(table, acl_myfree_fn);
	acl_slice_pool_destroy(slice);
}

static void htable_iter2(void)
{
	ACL_HTABLE *table = acl_htable_create(10, 0);
	ACL_ITER iter;
	char  key[32], *value;
	int   i;

	for (i = 0; i < 20; i++) {
		snprintf(key, sizeof(key), "key: %d", i);
		value = acl_mymalloc(32);
		snprintf(value, 32, "value: %d", i);
		assert(acl_htable_enter(table, key, value));
	}

	i = 0;
	printf("\n>>>acl_foreach for htable:\n");
	acl_foreach(iter, table) {
		printf("hash i=%d, [%s]\n", iter.i, (char*) iter.data);
		i++;
	}

	printf(">>>total: %d\n", i);

	i = 0;
	printf("\n>>>acl_foreach_reverse for htable:\n");
	acl_foreach_reverse(iter, table) {
		printf("hash i=%d, [%s]\n", iter.i, (char*) iter.data);
		i++;
	}

	printf(">>>total: %d\n", i);
	acl_htable_free(table, acl_myfree_fn);
}

static void binhash_iter2(void)
{
	ACL_BINHASH *table = acl_binhash_create(10, 0);
	ACL_ITER iter;
	char  key[32], *value;
	int   i;

	for (i = 0; i < 20; i++) {
		snprintf(key, sizeof(key), "key: %d", i);
		value = acl_mymalloc(32);
		snprintf(value, 32, "value: %d", i);
		assert(acl_binhash_enter(table, key, (int) strlen(key) + 1, value));
	}

	i = 0;
	printf("\n>>>acl_foreach for binhash:\n");
	acl_foreach(iter, table) {
		printf("hash i=%d, [%s]\n", iter.i, (char*) iter.data);
		i++;
	}

	printf(">>>total: %d\n", i);

	i = 0;
	printf("\n>>>acl_foreach_reverse for binhash:\n");
	acl_foreach_reverse(iter, table) {
		printf("hash i=%d, [%s]\n", iter.i, (char*) iter.data);
		i++;
	}

	printf(">>>total: %d\n", i);
	acl_binhash_free(table, acl_myfree_fn);
}

static void fifo_iter(void)
{
	ACL_FIFO fifo, *fifo_ptr = &fifo;
	ACL_FIFO_ITER iter;
	ACL_ITER iter2;
	ACL_FIFO_INFO *info;
	int   i;
	char *data;

	acl_fifo_init(&fifo);

	for (i = 0; i < 10; i++) {
		data = acl_mymalloc(32);
		snprintf(data, 32, "data: %d", i);
		acl_fifo_push(&fifo, data);
	}

	printf("\n>>> fifo_iter: foreach\n");
	acl_fifo_foreach(iter, fifo_ptr) {
		printf("%s\n", (char*) iter.ptr->data);
	}

	printf("\n>>> fifo_iter: foreach\n");
	acl_fifo_foreach(iter, &fifo) {
		printf("%s\n", (char*) acl_fifo_iter_value(iter));
	}

	printf("\n>>> fifo_iter: foreach_reverse\n");
	acl_fifo_foreach_reverse(iter, &fifo) {
		info = iter.ptr;
		printf("%s\n", (char*) info->data);
	}

	printf("\n>>> acl_foreach for fifo:\n");
	acl_foreach(iter2, &fifo) {
		printf("i: %d, value: %s\n", iter2.i, (char*) iter2.data);
	}

	printf("\n>>> acl_foreach_reverse for fifo:\n");
	acl_foreach_reverse(iter2, &fifo) {
		printf("i: %d, value: %s\n", iter2.i, (char*) iter2.data);
	}

	while (1) {
		data = acl_fifo_pop(&fifo);
		if (data == NULL)
			break;
		acl_myfree(data);
	}
}

static void fifo_iter2(void)
{
	ACL_FIFO *fifo_ptr = acl_fifo_new();
	ACL_ITER iter;
	char *data;
	int   i;

	for (i = 0; i < 10; i++) {
		data = acl_mymalloc(32);
		snprintf(data, 32, "data: %d", i);
		acl_fifo_push(fifo_ptr, data);
	}

	printf("\n>>> fifo_iter: foreach\n");
	acl_foreach(iter, fifo_ptr) {
		printf("%s\n", (char*) iter.data);
	}

	while (1) {
		data = acl_fifo_pop(fifo_ptr);
		if (data == NULL)
			break;
		acl_myfree(data);
	}

	acl_fifo_free(fifo_ptr, NULL);
}

typedef struct {
	char  name[32];
	ACL_RING entry;
} DUMMY;

static void ring_iter(void)
{
	ACL_RING head;
	DUMMY *dummy;
	ACL_RING_ITER iter;
	int   i;

	acl_ring_init(&head);

	for (i = 0; i < 10; i++) {
		dummy = (DUMMY*) acl_mycalloc(1, sizeof(DUMMY));
		snprintf(dummy->name, sizeof(dummy->name), "dummy:%d", i);
		acl_ring_append(&head, &dummy->entry);
	}

	printf("\nring_iter:\n");
	acl_ring_foreach(iter, &head) {
		dummy = acl_ring_to_appl(iter.ptr, DUMMY, entry);
		printf("name: %s\n", dummy->name);
	}

	while (1) {
		iter.ptr = acl_ring_pop_head(&head);
		if (iter.ptr == NULL)
			break;
		dummy = acl_ring_to_appl(iter.ptr, DUMMY, entry);
		acl_myfree(dummy);
	}
}

static void argv_iter(int use_slice)
{
	const char *s = "hello world, you are welcome!";
	ACL_DBUF_POOL *dbuf = use_slice ? acl_dbuf_pool_create(8192) : NULL;
	ACL_ARGV *argv = acl_argv_split3(s, " ,!", dbuf);
	ACL_ITER iter;

	printf("\nacl_foreach for ACL_ARGV:\n");
	acl_foreach(iter, argv) {
		printf(">> i: %d, value: %s\n", iter.i, (char*) iter.data);
	}

	printf("\nacl_foreach_reverse for ACL_ARGV:\n");
	acl_foreach_reverse(iter, argv) {
		printf(">> i: %d, value: %s\n", iter.i, (char*) iter.data);
	}

	if (dbuf == NULL)
		acl_argv_free(argv);
	else
		acl_dbuf_pool_destroy(dbuf);
}

static void array_iter(void)
{
	ACL_ARRAY *a = acl_array_create(10);
	ACL_ITER iter;
	int   i;
	char *ptr;

	for (i = 0; i < 20; i++) {
		ptr = (char*) acl_mymalloc(32);
		snprintf(ptr, 32, "data: %d", i);
		(void) acl_array_append(a, ptr);
	}

	printf("\nacl_foreach for acl_array:\n");
	acl_foreach(iter, a) {
		printf(">> i: %d, value: %s\n", iter.i, (char*) iter.data);
	}

	printf("\nacl_foreach_reverse for acl_array:\n");
	acl_foreach_reverse(iter, a) {
		printf(">> i: %d, value: %s\n", iter.i, (char*) iter.data);
	}

	acl_array_destroy(a, acl_myfree_fn);
}

static void dlink_iter(void)
{
	ACL_DLINK *dlink = acl_dlink_create(10);
	ACL_ITER iter;
	ACL_DITEM *ditem;

	printf("\n>>> dlink_iter:\n");

	(void) acl_dlink_insert(dlink, 1000, 2000);
	printf(">>add 1000 -- 2000\n");
	(void) acl_dlink_insert(dlink, 1, 1);
	printf(">>add 1 -- 1\n");
	(void) acl_dlink_insert(dlink, 2, 3);
	printf(">>add 2 -- 3\n");
	(void) acl_dlink_insert(dlink, 4, 8);
	printf(">>add 10 -- 100\n");
	(void) acl_dlink_insert(dlink, 10, 100);
	printf(">>add 99 -- 200\n");
	(void) acl_dlink_insert(dlink, 99, 200);
	printf(">>add 201 -- 300\n");
	(void) acl_dlink_insert(dlink, 201, 300);
	printf(">>add 201 -- 300\n");
	(void) acl_dlink_insert(dlink, 300, 400);
	printf(">>add 300 -- 400\n");
	(void) acl_dlink_insert(dlink, 500, 600);
	printf(">>add 500 -- 600\n");

	printf("\n>>>acl_foreach for acl_dlink:\n");
	acl_foreach(iter, dlink) {
		ditem = (ACL_DITEM*) iter.data;
		printf(">> i: %d, range: %lld -- %lld\n",
			iter.i, ditem->begin, ditem->end);
	}

	acl_dlink_free(dlink);
}

static void cache_free(const ACL_CACHE_INFO *info acl_unused, void *ptr)
{
	acl_myfree(ptr);
}

static void cache_iter(void)
{
	ACL_CACHE *cache = acl_cache_create(100, 120, cache_free);
	char *ptr, key[32];
	ACL_ITER iter;
	int   i;

	for (i = 0; i < 10; i++) {
		snprintf(key, sizeof(key), "key-%d", i);
		ptr = (char*) acl_mymalloc(32);
		snprintf(ptr, 32, "data-%d", i);
		(void) acl_cache_enter(cache, key, ptr);
	}

	printf("\n>>>acl_foreach for acl_cache:\n");
	acl_foreach(iter, cache) {
		printf(">>i: %d, key: %s,  value: %s\n",
			iter.i, iter.key, (char*) iter.data);
	}

	printf("\n>>>acl_foreach_reverse for acl_cache:\n");
	acl_foreach_reverse(iter, cache) {
		printf(">>i: %d, key: %s, value: %s\n",
			iter.i, iter.key, (char*) iter.data);
	}

	acl_cache_free(cache);
}

typedef struct {
	char  name[32];
	int   i;
} TIMER_ITEM;

static void timer_iter(void)
{
	ACL_TIMER *timer = acl_timer_new();
	TIMER_ITEM *item;
	ACL_ITER iter;
	int   i;

	for (i = 0; i < 10; i++) {
		item = (TIMER_ITEM*) acl_mycalloc(1, sizeof(TIMER_ITEM));
		snprintf(item->name, sizeof(item->name), "name-%d", i);
		item->i = i;
		acl_timer_request(timer, item, i);
	}

	printf("\n>>>acl_foreach for ACL_TIMER:\n");

	acl_foreach(iter, timer) {
		const ACL_TIMER_INFO *info;

		item = (TIMER_ITEM*) iter.data;
		info = acl_iter_info(iter, timer);
		printf("\titem->name=%s, item->i=%d, when: %d\n",
			item->name, item->i, (int) info->when);
	}

	printf("\n>>>acl_foreach_reverse for ACL_TIMER:\n");

	acl_foreach_reverse(iter, timer) {
		const ACL_TIMER_INFO *info;

		item = (TIMER_ITEM*) iter.data;
		info = acl_iter_info(iter, timer);
		printf("\titem->name=%s, item->i=%d, when: %d\n",
			item->name, item->i, (int) info->when);
		acl_myfree(item);
	}
	acl_timer_free(timer, NULL);
}

static void netdb_iter(void)
{
	ACL_DNS_DB *dns_db = acl_netdb_new("www.test.com");
	ACL_ITER iter;

	acl_netdb_add_addr(dns_db, "127.0.0.1", 80);
	acl_netdb_add_addr(dns_db, "192.168.0.1", 80);
	acl_netdb_add_addr(dns_db, "192.168.0.2", 80);
	acl_netdb_add_addr(dns_db, "192.168.0.2", 80);
	acl_netdb_add_addr(dns_db, "192.168.0.3", 80);

	printf("\n>>>acl_foreach for ACL_DNS_DB\n");

	acl_foreach(iter, dns_db) {
		const ACL_HOST_INFO *info, *info2;

		info = (const ACL_HOST_INFO*) iter.data;
		info2 = acl_iter_info(iter, dns_db);
		printf("\tip=%s, %s; port=%d, %d\n",
			info->ip, info2->ip, info->hport, info2->hport);
	}

	printf("\n>>>acl_foreach_reverse for ACL_DNS_DB\n");

	acl_foreach_reverse(iter, dns_db) {
		const ACL_HOST_INFO *info, *info2;

		info = (const ACL_HOST_INFO*) iter.data;
		info2 = acl_iter_info(iter, dns_db);
		printf("\tip=%s, %s; port=%d, %d\n",
			info->ip, info2->ip, info->hport, info2->hport);
	}
	acl_netdb_free(dns_db);
}

static void ifconf_iter(void)
{
	ACL_IFCONF *ifconf = acl_get_ifaddrs();
	ACL_IFADDR *ifaddr;
	ACL_ITER iter;

	printf("\nacl_foreach for ifconf_iter\n");
	if (ifconf) {
		acl_foreach(iter, ifconf) {
			ifaddr = (ACL_IFADDR*) iter.data;
#ifdef ACL_MS_WINDOWS
			printf("\tname: %s, desc: %s, addr: %s\n",
				ifaddr->name, ifaddr->desc, ifaddr->addr);
#else
			printf("\tname: %s, addr: %s\n", ifaddr->name, ifaddr->addr);
#endif
		}
	}

	printf("\nacl_foreach_reverse for ifconf_iter\n");
	acl_foreach_reverse(iter, ifconf) {
		ifaddr = (ACL_IFADDR*) iter.data;
#ifdef ACL_MS_WINDOWS
		printf("\tname: %s, desc: %s, addr: %s\n",
			ifaddr->name, ifaddr->desc, ifaddr->addr);
#else
		printf("\tname: %s, addr: %s\n", ifaddr->name, ifaddr->addr);
#endif
	}

	acl_free_ifaddrs(ifconf);
}

int main(int argc, char *argv[])
{
#if 0
	acl_memory_debug_start();
	acl_memory_debug_stack(1);
	acl_debug_malloc_init(NULL, "log.txt");
#endif
	int  use_slice;

	if (argc == 2 && strcasecmp(argv[1], "slice") == 0)
		use_slice = 1;
	else
		use_slice = 0;

	fifo_iter();
	fifo_iter2();

	ring_iter();

	htable_iter();
	htable_iter2();

	binhash_iter();
	binhash_iter2();

	argv_iter(use_slice);
	array_iter();
	dlink_iter();
	cache_iter();
	timer_iter();
	netdb_iter();
	ifconf_iter();
#ifdef ACL_MS_WINDOWS
	printf("enter any key to exit ...\n");
	getchar();
#endif
	return (0);
}
