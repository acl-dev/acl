#include "lib_acl.h"
#include "ring.h"

typedef struct DUMMY {
	char s[8];
	int  n;
	struct DUMMY *next;
	RING entry;
} DUMMY;

static void slice_alloc(int n)
{
	int   i;
	ACL_SLICE *slice = acl_slice_create("slice_alloc", 0, sizeof(DUMMY), ACL_SLICE_FLAG_GC1);
	DUMMY *ptr, *head = NULL;

	printf("in slice_alloc\n");
	ACL_METER_TIME(NULL);
	for (i = 0; i < n; i++) {
		ptr = (DUMMY*) acl_slice_alloc(slice);
		if (head != NULL)
			ptr->next = head;
		else
			ptr->next = NULL;
	}
	ACL_METER_TIME(NULL);

	i = 0;
	while (i++ < 10) {
		printf("sleep one second\n");
		sleep(1);
	}

	ACL_METER_TIME(NULL);
	acl_slice_destroy(slice);
	ACL_METER_TIME(NULL);
}

static void sys_alloc(int n)
{
	int   i;
	DUMMY *ptr, *head = NULL;

	printf("in sys_alloc\n");
	ACL_METER_TIME(NULL);
	for (i = 0; i < n; i++) {
		ptr = (DUMMY*) acl_mymalloc(sizeof(DUMMY));
		if (head != NULL)
			ptr->next = head;
		else
			ptr->next = NULL;
		head = ptr;
	}
	ACL_METER_TIME(NULL);

	i = 0;
	while (i++ < 10) {
		printf("sleep one second\n");
		sleep(1);
	}

	ACL_METER_TIME(NULL);
	while (head) {
		ptr = head;
		head = head->next;
		acl_myfree(ptr);
	}
	ACL_METER_TIME(NULL);
}

static void slice_gc1(void)
{
	DUMMY head;
	DUMMY *ptr0, *ptr1, *ptr2, *ptr3;
	ACL_SLICE *slice = acl_slice_create("slice_gc1", 4096 * 64, sizeof(DUMMY),
		ACL_SLICE_FLAG_GC1 /* | ACL_SLICE_FLAG_RTGC_OFF */);

	ring_init(&head.entry);

	ptr0 = acl_slice_alloc(slice);
	ptr1 = acl_slice_alloc(slice);
	ptr2 = acl_slice_alloc(slice);
	ptr3 = acl_slice_alloc(slice);

	memset(ptr0->s, 'a', sizeof(ptr0->s));
	memset(ptr1->s, 'a', sizeof(ptr0->s));
	memset(ptr2->s, 'a', sizeof(ptr0->s));
	memset(ptr3->s, 'a', sizeof(ptr0->s));

	ptr0->n = 0;
	ptr1->n = 1;
	ptr2->n = 2;
	ptr3->n = 3;

	ring_prepend(&head.entry, &ptr0->entry);
	ring_prepend(&head.entry, &ptr1->entry);
	ring_prepend(&head.entry, &ptr2->entry);
	ring_prepend(&head.entry, &ptr3->entry);

	acl_slice_free2(slice, ptr3);
	acl_slice_free2(slice, ptr2);
	acl_slice_free2(slice, ptr1);
	acl_slice_free2(slice, ptr0);
	acl_slice_destroy(slice);
}

static void slice_gc2(void)
{
	int   i, max = 10000000;
	ACL_BINHASH *table;
#if 1
	unsigned int flag = ACL_BINHASH_FLAG_SLICE2;
#else
	unsigned int flag = 0;
#endif

	table = acl_binhash_create(max, flag);

	ACL_METER_TIME("---- begin add to hash ------");
	for (i = max - 1; i >= 0; i--) {
		acl_binhash_enter(table, &i, sizeof(i), 0);
	}

	ACL_METER_TIME("---- end add and begin free table ---");
	acl_binhash_free(table, 0);
	ACL_METER_TIME("--- end free table ---");
}

static void slice_gc(void)
{
	ACL_SLICE *slice;
	DUMMY *head = NULL, *ptr;
	int   i, n = 10240000;
	int   nsleep = 0;
	ACL_SLICE_STAT sbuf;
	unsigned int flag = ACL_SLICE_FLAG_GC2 /* | ACL_SLICE_FLAG_RTGC_OFF */;

	acl_memory_debug_start();

	printf("sizeof DUMMY: %d\n", (int) sizeof(DUMMY));

	slice = acl_slice_create("slice_gc", 1024 * 100, sizeof(DUMMY), flag);
	ACL_METER_TIME("------ begin alloc ------");
	for (i = 0; i < n; i++) {
		ptr = (DUMMY*) acl_slice_alloc(slice);
		if (head != NULL)
			ptr->next = head;
		else
			ptr->next = NULL;
		head = ptr;
	}

	ACL_METER_TIME("------ end alloc and begin free ------");

	acl_memory_stat();

	while (nsleep-- > 0) {
		printf("sleep 1 second\n");
		sleep(1);
	}

	acl_slice_stat(slice, &sbuf);
	printf("islots: %d, nslots: %d, total length: %d, nbuf: %d\n",
		(int) sbuf.islots, (int) sbuf.nslots,
		(int) sbuf.length, (int) sbuf.nbuf);

	while (head) {
		ptr = head;
		head = head->next;
		if ((flag & ACL_SLICE_FLAG_GC1))
			acl_slice_free2(slice, ptr);
		else {
			acl_slice_free(ptr);
		}
	}
	ACL_METER_TIME("------ end free ------");

	acl_slice_stat(slice, &sbuf);
	printf("call slice gc, sizeof(DUMMY): %d, islots: %d, nslots: %d, total length: %d, nbuf: %d\n",
		(int) sizeof(DUMMY), (int) sbuf.islots, (int) sbuf.nslots,
		(int) sbuf.length, (int) sbuf.nbuf);
	i = 0;

	ACL_METER_TIME("------ and begin gc ------");
	printf("------------ islots: %d, nslots: %d, nbuf: %d ------\n", sbuf.islots, sbuf.nslots, sbuf.nbuf);

	while (sbuf.nbuf > 0 && (sbuf.flag & ACL_SLICE_FLAG_GC1) == 0) {
		acl_slice_gc(slice);
		acl_slice_stat(slice, &sbuf);
		i++;
	}
	ACL_METER_TIME("------ end gc and begin destroy ------");
	acl_slice_destroy(slice);
	ACL_METER_TIME("------ end destroy ------");
	printf("slice gc over, call %d times\n", i);

	acl_memory_debug_stop();
}

static void mem_slice(void)
{
	char *ptr;

	acl_mem_slice_init(8, 10240, 100, ACL_SLICE_FLAG_GC2 | ACL_SLICE_FLAG_RTGC_OFF);

	ptr = acl_mystrdup("hello world");
	printf("ptr: %s\n", ptr);
	ptr = (char*) acl_myrealloc(ptr, 1024);
	printf("ptr: %s\n", ptr);
	acl_myfree(ptr);
	acl_mem_slice_destroy();
}

int main(int argc acl_unused, char *argv[] acl_unused)
{
	int   n = 10000000;

	mem_slice(); return (0);
	slice_gc(); return (0);
	slice_gc2(); return (0);
	slice_gc1(); return (0);

	slice_alloc(n);
	sys_alloc(n);
	printf("enter any key to exit\n");
	getchar();

	return (0);
}
