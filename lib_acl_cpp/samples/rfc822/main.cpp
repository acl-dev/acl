#include "acl_cpp/mime/rfc822.hpp"

int main(void)
{
	acl::rfc822 rfc;

	const char* addrs = "=?gb2312?B?1dSx+A==?= <zhaobing@51iker.com>;\r\n"
		"\t\"=?GB2312?B?t+vBosn6?=\" <fenglisheng@51iker.com>;\r\n"
		"  \"zhengshuxin1\" \"zhengshuxin2\" <zhengshuxin1@51iker.com>;\r\n"
		"\"zhengshuxin3\";\"zhengshuxin4\" <zhengshuxin2@51iker.com>;"
		"<xuganghui@51iker.com>;<wangwenquan@51iker.com>";
	const std::list<acl::rfc822_addr*>& addr_list = rfc.parse_addrs(addrs);
	std::list<acl::rfc822_addr*>::const_iterator cit = addr_list.begin();
	for (; cit != addr_list.end(); cit++)
	{
		const acl::rfc822_addr* addr = *cit;
		printf("addr: %s, comment: %s\n", addr->addr,
			addr->comment ? addr->comment : "null");
	}

	printf("===================================================\r\n");
	const std::list<acl::rfc822_addr*>& addr_list2 = rfc.parse_addrs("helloworld");
	if (addr_list2.empty())
		printf("Failed\n");
	else
		printf("Ok\n");
	printf("===================================================\r\n");

	const char* addrs2[] = {
		"=?gb2312?B?1dSx+A==?= <zhaobing@51iker.com>;",
		"\"=?GB2312?B?t+vBosn6?=\" <fenglisheng@51iker.com>;",
		"\"zhengshuxin1\" \"zhengshuxin2\" <zhengshuxin1@51iker.com>;",
		"\"zhengshuxin3\";\"zhengshuxin4\" <zhengshuxin2@51iker.com>;",
		"<xuganghui@51iker.com>;",
		"<wangwenquan@51iker.com>",
		"\"SYD - Rywak, Ray\" <ray.rywak@ijsglobal.com>;",
		NULL
	};

	for (int i = 0; addrs2[i] != NULL; i++)
	{
		const acl::rfc822_addr* addr = rfc.parse_addr(addrs2[i]);
		printf("addr: %s, comment: %s\n", addr->addr,
			addr->comment ? addr->comment : "null");
	}

	printf("===================================================\r\n");

	time_t now = time(NULL);
	char  buf[64];
	rfc.mkdate(now, buf, sizeof(buf), acl::tzone_gmt);
	printf(">>date: %s, %ld\n", buf, now);

	time_t t = rfc.parse_date(buf);
	printf(">> now: %ld\n", t);

	printf("===================================================\r\n");
	struct ADDR_CHECK 
	{
		const char* addr;
		bool ret;
	};
	const ADDR_CHECK addr3[] = {
		{ "=?gb2312?B?1dSx+A==?= <zhaobing@51iker.com>", true },
		{ "\"=?GB2312?B?t+vBosn6?=\" <fenglisheng@51iker.com>;", true },
		{ "\"=?GB2312?B?t+vBosn6?=\" <fenglisheng@51iker.com>,", true },
		{ "\"=?GB2312?B?t+vBosn6?=\" <fenglisheng@51iker.com>;", true },
		{ "\"zhengshuxin1\" \"zhengshuxin2\" <zhengshuxin1@51iker.com>;", true },
		{ "\"zhengshuxin1, ;zhengshuxin2\" <zhengshuxin1@51iker.com>;", true },
		{ "<xuganghui@51iker.com>;", true },
		{ "<wangwenquan@51iker.com>", true },
		{ "xxx@xxx.xxx", true },
		{ "xxx.xxx@xxx.xxx.xxx", true },
		{ "x_x-x@xxx.xxx", true },
		{ "x._x@xxx.xxx", true },
		{ "x@x.x", true },
		{ "@xxx.xxx", false },
		{ "_xxx@xxx.xxx", false },
		{ "xx_@xx.xx", false },
		{ "xx@", false },
		{ "xx@x", false },
		{ "xx@_x.xx", false },
		{ "xx@.x.xx", false },
		{ "xx@xx.xx.", false },
		{ "xx@xx_xx", false },
		{ "xx@_", false },
		{ "@@xx.xx@xx.xx", false },
		{ "жа@xx.xx", false },
		{ "~@xx.xx", false },
		{ "#@xx.xx", false },
		{ "*@xx.xx", false },
		{ ";xxx@xxx.xxx", false },
		{ ",xxx@xxx.xxx", false },
		{ " ;xxx@xxx.xxx", false },
		{ "x;xxx@xxx.xxx", false },
		{ "\";\" <xxx@xxx.xxx>", true },
		{ "&xxx@xxx.xxx", false },
		{ "xxx_@xxx.xxx", false },
		{ "xxx.@xxx.xxx", false },

		{ NULL, false }
	};

	int   nerror = 0;
	for (int i = 0; addr3[i].addr != NULL; i++)
	{
		if (rfc.check_addr(addr3[i].addr) != addr3[i].ret)
		{
			nerror++;
			printf("addr3[%d]: %s, check failed ...\r\n",
				i, addr3[i].addr);
		}
	}
	if (nerror == 0)
		printf("check mail addr ok...\r\n");
	else
		printf("check mail addr failed...\r\n");

#ifdef WIN32
	printf("enter any key to exit ...\r\n");
	getchar();
#endif
	return (0);
}
