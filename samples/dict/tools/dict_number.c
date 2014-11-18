/*
 * Proof-of-concept test program. Create, update or read a database. When
 * the input is a name=value pair, the database is updated, otherwise the
 * program assumes that the input specifies a lookup key and prints the
 * corresponding value.
 */

/* System library. */
#include "lib_acl.h"
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#ifdef ACL_UNIX
#include <unistd.h>
#include <signal.h>
#endif
#include "dict.h"
#include "unescape.h"
#include "dict_test.h"
#include "md5.h"

#ifdef ACL_MS_WINDOWS
#include <io.h>
#define isatty _isatty
#endif

#define	STR	acl_vstring_str
#define	LEN	ACL_VSTRING_LEN

void dict_number_main(int max)
{
#if 0
	char   *dict_name = "btree:./var/test";
#elif	0
	char   *dict_name = "tc:*#bnum=10000000";
#elif	0
	char   *dict_name = "tc:+#capnum=10000000";
#elif	0
	char   *dict_name = "tc:./var/test.tch";
#elif	0
	char   *dict_name = "tc:./var/test.tcb";
#elif	0
	char   *dict_name = "tc:./var/test.tct";
#else
	char   *dict_name = "tc:./var/test.10000.tcf#width=12#limsiz=2684354560";
#endif
	int     open_flags = 0;
	int     dict_flags = /* DICT_FLAG_LOCK | */ DICT_FLAG_DUP_REPLACE | DICT_FLAG_TRY0NULL;
	DICT   *dict;
	int     i;

	char   *value;
	ACL_VSTRING *key_buf = acl_vstring_alloc(256);
	ACL_VSTRING *value_buf = acl_vstring_alloc(256);
	size_t  value_size;
	char buf[256];
	bool b = true;

	if (b)
		b = false;

#define	ADD_BENCH1(from, to) do {  \
    for (i = from; i < to; i++) {  \
	acl_vstring_sprintf(key_buf, "%d:key", i);  \
	acl_vstring_sprintf(value_buf, "value:%d", i);  \
	dict_update(dict_name, STR(key_buf), STR(value_buf), LEN(value_buf));  \
    }  \
}while (0)

#define	ADD_BENCH2(from, to) do {  \
    const char *__key = "hello world";  \
    char  __buf[33];  \
    for (i = from; i < to; i++) {  \
	acl_vstring_sprintf(key_buf, "%d:key", i);  \
	MD5String(STR(key_buf), __key, strlen(__key), __buf, sizeof(__buf));  \
	acl_vstring_sprintf(value_buf, "value:%d", i);  \
	dict_update(dict_name, __buf, STR(value_buf), LEN(value_buf));  \
    }  \
}while (0)

#define	ADD_BENCH3(from, to) do {  \
    for (i = from; i < to; i++) {  \
	acl_vstring_sprintf(key_buf, "%d", i);  \
	acl_vstring_sprintf(value_buf, "value:%d:", i);  \
	dict_update(dict_name, STR(key_buf), STR(value_buf), LEN(value_buf));  \
    }  \
}while (0)

#define	ADD_BENCH_DUMMY(from, to)

#define	ADD_BENCH	ADD_BENCH3

	acl_init();
	dict_init();

	open_flags = O_CREAT | O_RDWR | O_TRUNC;
	dict = dict_open(dict_name, open_flags, dict_flags);
	dict_register(dict_name, dict);

	ACL_METER_TIME("--- begin add ---");

	ADD_BENCH((max * 0)/10 + 1, (max * 1)/10);
	ACL_METER_TIME("--- max * 0 / 10 over ---");

	ADD_BENCH((max * 9)/10, max);
	ACL_METER_TIME("--- max * 9 / 10 over ---");

	ADD_BENCH((max * 8)/10, (max * 9)/10);
	ACL_METER_TIME("--- max * 8 / 10 over ---");

	ADD_BENCH((max * 4)/10, (max * 5)/10);
	ACL_METER_TIME("--- max * 4 / 10 over ---");

	ADD_BENCH((max * 1)/10, (max * 2)/10);
	ACL_METER_TIME("--- max * 1 / 10 over ---");

	ADD_BENCH((max * 2)/10, (max * 3)/10);
	ACL_METER_TIME("--- max * 2 / 10 over ---");

	ADD_BENCH((max * 5)/10, (max * 6)/10);
	ACL_METER_TIME("--- max * 5 / 10 over ---");

	ADD_BENCH((max * 3)/10, (max * 4)/10);
	ACL_METER_TIME("--- max * 3 / 10 over ---");

	ADD_BENCH((max * 7)/10, (max * 8)/10);
	ACL_METER_TIME("--- max * 7 / 10 over ---");

	ADD_BENCH((max * 6)/10, (max * 7)/10);
	ACL_METER_TIME("--- max * 6 / 10 over ---");

	ACL_METER_TIME("---  end add, begin get  ---");

	for (i = 1; i < max; i++) {
		acl_vstring_sprintf(key_buf, "%d", i);
		if (dict_lookup(dict_name, STR(key_buf), &value, &value_size) == NULL) {
			acl_msg_error("key: (%s) no exist", STR(key_buf));
			break;
		} else
			acl_myfree(value);
		if ((i * 10) % max == 0) {
			snprintf(buf, sizeof(buf), "--- get i=%d ok ---", i);
			ACL_METER_TIME(buf);
		}
	}

	snprintf(buf, sizeof(buf), "--- end get i=%d ok ---", i);
	ACL_METER_TIME(buf);

	acl_vstring_free(key_buf);
	acl_vstring_free(value_buf);
	printf("unregister %s\n", dict_name);
	dict_unregister(dict_name);
	printf("close %s\n", dict_name);
}
