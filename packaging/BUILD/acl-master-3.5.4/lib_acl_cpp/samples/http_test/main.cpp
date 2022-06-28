#include <list>
#include "acl_cpp/lib_acl.hpp"

using namespace acl;

int main(void)
{
	const char* data = "U_TRS1=000000f6.66721803.4fd1681c.8c48b73a; path=/; expires=Mon, 06-Jun-22 02:49:00 GMT; domain=.sina.com.cn";
	HttpCookie* cookie = new HttpCookie();

	printf("Set-Cookie: %s\r\n", data);

	if (cookie->setCookie(data) == false)
	{
		printf("parse cookie(%s) error\r\n", data);
		cookie->destroy();
		return -1;
	}

	printf("cookie name: %s\r\n", cookie->getName());
	printf("cookie value: %s\r\n", cookie->getValue());
	printf("domain: %s\r\n", cookie->getDomain());
	printf("path: %s\r\n", cookie->getPath());
	printf("max-age: %d\r\n", cookie->getMaxAge());

	const std::list<HTTP_PARAM*>& params = cookie->getParams();
	std::list<HTTP_PARAM*>::const_iterator cit = params.begin();
	for (; cit != params.end(); ++cit)
		printf(">>%s=%s\r\n", (*cit)->name, (*cit)->value);

	cookie->destroy();

	const char* s = "encrypted/json";
	acl::http_ctype hc;
	if (hc.parse(s)) {
		const char* ctype = hc.get_ctype();
		const char* stype = hc.get_stype();
		printf("ctype=%s, stype=%s\r\n", ctype, stype);
	}
	return 0;
}
