#include "lib_acl.h"

#define STR	acl_vstring_str

static const char* default_data = \
    "{ 'menu name': {\r\n"
    "    'id:file': 'file',\r\n"
    "    'value{': 'File',\r\n"
    "    'popup{}': {\r\n"
    "        'menuitem1}': [\r\n"
    "            {'value': 'New', 'onclick': 'CreateNewDoc()'},\r\n"
    "            {'value': 'Open', 'onclick': 'OpenDoc()'},\r\n"
    "            {'value': 'Close', 'onclick': 'CloseDoc()'}\r\n"
    "        ],\r\n"
    "        'menuname[]': 'hello world',\r\n"
    "        'inner': { 'value' : 'new ', 'value' : 'open' },\r\n"
    "        'menuitem2': [\r\n"
    "            {'value': 'New', 'onclick': 'CreateNewDoc()'},\r\n"
    "            {'value': 'Open', 'onclick': 'OpenDoc()'},\r\n"
    "            {'value': 'Close', 'onclick': 'CloseDoc()'},\r\n"
    "            {{'value': 'Help', 'onclick': 'Help()'}}"
    "        ]\r\n"
    "    }\r\n"
    " }\r\n,"
    " 'help': 'hello world!',\r\n"
    " 'menuitem2': [\r\n"
    "   {'value': 'New', 'onclick': 'CreateNewDoc()'},\r\n"
    "   {'value': 'Open', 'onclick': 'OpenDoc()'},\r\n"
    "   {'value': 'Close', 'onclick': 'CloseDoc()'},\r\n"
    "   [{'value': 'Save', 'onclick': 'SaveDoc()'}]"
    " ]\r\n"
    "}\r\n";

static void test_json_benchmark(int max)
{
	ACL_JSON *json = acl_json_alloc();

	ACL_METER_TIME("-------------bat begin--------------");

	for (int i = 0; i < max; i++)
	{
		const char* ptr = default_data;
		acl_json_update(json, ptr);
		acl_json_reset(json);
	}

	ACL_METER_TIME("-------------bat end--------------");
	acl_json_free(json);
}

static int benchmark_max = 100; 

static void *thread_func(void *ctx acl_unused)
{
	test_json_benchmark(benchmark_max);
	return NULL;
}

static void usage(const char* program)
{
	printf("usage: %s -h[help]\r\n"
		" -c max_threads\r\n"
		" -m benchmark_max\r\n", program);
}

int main(int argc, char** argv)
{
	int   ch, i;
	int   max_threads = 1;
	acl_pthread_attr_t attr;
	acl_pthread_t tid;
	acl_pthread_t* ids;

	acl_pthread_attr_init(&attr);

	while ((ch = getopt(argc, argv, "hm:c:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return (0);
			break;
		case 'm':
			benchmark_max = atoi(optarg);
			break;
		case 'c':
			max_threads = atoi(optarg);
			if (max_threads <= 0)
				max_threads = 1;
			break;
		default:
			break;
		}
	}

	ids = (acl_pthread_t*) acl_mymalloc(max_threads * sizeof(acl_pthread_t));

	for (i = 0; i < max_threads; i++) {
		acl_pthread_create(&tid, &attr, thread_func, NULL);
		ids[i] = tid;
	}

	for (i = 0; i < max_threads; i++)
		acl_pthread_join(ids[i], NULL);

	acl_myfree(ids);

#ifdef	WIN32
	getchar();
#endif

	return (0);
}
