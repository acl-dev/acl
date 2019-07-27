#include "stdafx.h"
#include <list>
#include <vector>
#include <map>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include "struct.h"  // 鐢� gson 宸ュ叿鏍规嵁 struct.stub 杞崲鑰屾垚
#include "struct.gson.h"    // 鐢� gson 宸ュ叿鏍规嵁 struct.stub 鐢熸垚

// 搴忓垪鍖栬繃绋�
static void serialize(void)
{
	user u;

	u.name = "zsxxsz";
	u.domain = "263.net";
	u.age = 11;
	u.male = true;

	acl::json json;

	// 灏� user 瀵硅薄杞崲涓� json 瀵硅薄
	acl::json_node& node = acl::gson(json, u);

	printf("serialize:\r\n");
	printf("json: %s\r\n", node.to_string().c_str());
	printf("\r\n");
}

// 鍙嶅簭鍒楀寲杩囩▼
static void deserialize(void)
{
	const char *s = "{\"name\": \"zsxxsz\", \"domain\": \"263.net\", \"age\": 11, \"male\": true, \"n0\": 10, \"n1\": 0, \"n2\": 1, \"n3\": 1.1, \"n4\": 0.0 }";
	printf("deserialize:\r\n");

	acl::json json;
	json.update(s);
	user u;

	// 灏� json 瀵硅薄杞崲涓� user 瀵硅薄
	std::pair<bool, std::string> ret = acl::gson(json.get_root(), u);

	// 濡傛灉杞崲澶辫触锛屽垯鎵撳嵃杞崲澶辫触鍘熷洜
	if (ret.first == false)
		printf("error: %s\r\n", ret.second.c_str());
	else
		printf("ok, name: %s, domain: %s, age: %d, male: %s, n0=%d, n1=%.2f, n2=%.2f, n3=%.2f, n4=%.2f\r\n",
			u.name.c_str(), u.domain.c_str(), u.age,
			u.male ? "yes" : "no", u.n0, u.n1, u.n2, u.n3, u.n4);
}

int main(void)
{
	acl::string buf;
	std::string sbuf("hello world");
	buf << sbuf << "\r\n" << &sbuf << "\r\n";
	buf += sbuf;
	buf += "\r\n";
	buf += &sbuf;
	printf("buf=[%s]\r\n", buf.c_str());

	sbuf += "\r\n";
	sbuf += buf;
	printf("sbuf=[%s]\r\n", sbuf.c_str());

	buf = sbuf;
	printf("buf=[%s]\r\n", buf.c_str());

	sbuf.clear();
	buf >> sbuf;
	printf("sbuf=[%s]\r\n", sbuf.c_str());

	buf = sbuf;
	sbuf.clear();
	buf >> &sbuf;
	printf("sbuf=[%s]\r\n", sbuf.c_str());

	serialize();
	deserialize();
	printf("Enter any key to continue ..."); fflush(stdout); getchar();
	return 0;
}
