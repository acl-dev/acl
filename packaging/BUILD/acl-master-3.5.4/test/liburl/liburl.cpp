#include "liburl.h"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/url_coder.hpp"

using namespace acl;

void test_url_coder(void)
{
	url_coder coder1;

	coder1.set("name1", "value1");
	coder1.set("name2", 2);
	coder1.set("name3", true, "value%d", 3);
	coder1.set("name4", "中国人");
	printf("coder1 >> %s, name1: %s, name2: %s, name3: %s, name4: %s\r\n",
		coder1.to_string().c_str(),
		coder1["name1"], coder1["name2"], coder1["name3"], coder1["name4"]);
	coder1.del("name1");
	const char* ptr = coder1["name1"];
	printf("coder1 >> %s, name1: %s, name2: %s, name3: %s, name4: %s\r\n",
		coder1.to_string().c_str(), ptr ? ptr : "null",
		coder1["name2"], coder1["name3"], coder1["name4"]);

	//////////////////////////////////////////////////////////////////////////

	url_coder coder2;

	coder2 = coder1;
	coder2.set("name5", "&=value5=&");
	ptr = coder2["name1"];
	printf("--------------------------------------------------------\r\n");
	printf("coder2 >> %s, name1: %s, name2: %s, name3: %s, name4: %s, name5: %s\r\n",
		coder2.to_string().c_str(), ptr ? ptr : "null",
		coder2["name2"], coder2["name3"], coder2["name4"], coder2["name5"]);

	//////////////////////////////////////////////////////////////////////////

	url_coder coder3(coder2);

	coder3.set("name5", 5);
	coder3.set("name6", "=&外国人&=");
	ptr = coder3["name1"];
	printf("--------------------------------------------------------\r\n");
	printf("coder3 >> %s, name1: %s, name2: %s, name3: %s, name4: %s, name5: %s, name6: %s\r\n",
		coder3.to_string().c_str(), ptr ? ptr : "null",
		coder3["name2"], coder3["name3"], coder3["name4"],
		coder3["name5"], coder3["name6"]);

	//////////////////////////////////////////////////////////////////////////

	url_coder coder4;
	const char* s = "name1=value1&name2=2&name3=value3&name4=%D6%D0%B9%FA%C8%CB";
	coder4.decode(s);
	printf("--------------------------------------------------------\r\n");
	printf("coder4 >> %s, name1: %s, name2: %s, name3: %s, name4: %s\r\n",
		coder4.to_string().c_str(),
		coder4["name1"], coder4["name2"], coder4["name3"], coder4["name4"]);
}
