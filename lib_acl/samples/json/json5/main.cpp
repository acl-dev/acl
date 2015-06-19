#include "lib_acl.h"
#include "cJSON.h"

#if 1
static const char* default_data = \
    "{ \"menu name\": {\r\n"
    "    \"id file\": \"file\",\r\n"
    "    \"value\": \"File\",\r\n"
    "    \"popup\": {\r\n"
    "        \"menuitem1\": [\r\n"
    "            {\"value\": \"New\", \"onclick\": \"CreateNewDoc()\"},\r\n"
    "            {\"value\": \"Open\", \"onclick\": \"OpenDoc()\"},\r\n"
    "            {\"value\": \"Close\", \"onclick\": \"CloseDoc()\"}\r\n"
    "        ],\r\n"
    "        \"menuname\": \"hello world\",\r\n"
    "        \"inner\": { \"value\" : \"new \", \"value\" : \"open\" },\r\n"
    "        \"menuitem2\": [\r\n"
    "            {\"value\": \"New\", \"onclick\": \"CreateNewDoc()\"},\r\n"
    "            {\"value\": \"Open\", \"onclick\": \"OpenDoc()\"},\r\n"
    "            {\"value\": \"Close\", \"onclick\": \"CloseDoc()\"},\r\n"
    "            {\"value\": \"Help\", \"onclick\": \"Help()\"}"
    "        ]\r\n"
    "    }\r\n"
    " },\r\n"
    " \"help\": \"hello world!\",\r\n"
    " \"menuitem2\": [\r\n"
    "   {\"value\": \"New\", \"onclick\": \"CreateNewDoc()\"},\r\n"
    "   {\"value\": \"Open\", \"onclick\": \"OpenDoc()\"},\r\n"
    "   {\"value\": \"Close\", \"onclick\": \"CloseDoc()\"},\r\n"
    "   [{\"value\": \"Save\", \"onclick\": \"SaveDoc()\"}]"
    " ]\r\n"
    "}\r\n";
#else
static const char* default_data = \
    "{ \"menu name\": \"hello\"}\r\n";
#endif

static void test_json_benchmark(int max)
{
	cJSON *json;

	ACL_METER_TIME("-------------bat begin--------------");

	for (int i = 0; i < max; i++)
	{
		const char* ptr = default_data;
		json = cJSON_Parse(ptr);
		if (json == NULL)
		{
			printf("parse error: %s\r\n", ptr);
			break;
		}
		cJSON_Delete(json);
	}

	ACL_METER_TIME("-------------bat end--------------");
}

static void usage(const char* program)
{
	printf("usage: %s -h[help]\n"
		" -m benchmark_max\n", program);
}

int main(int argc, char** argv)
{
	int   ch;
	int   benchmark_max = 100;

	while ((ch = getopt(argc, argv, "hm:")) > 0)
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
		default:
			break;
		}
	}

	test_json_benchmark(benchmark_max);

#ifdef	WIN32
	getchar();
#endif

	return (0);
}
