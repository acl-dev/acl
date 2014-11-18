#include <string>
#include "acl_cpp/mime/mime_code.hpp"
#include "acl_cpp/stream/fstream.hpp"
#include "acl_cpp/mime/mime_quoted_printable.hpp"

int main(void)
{
	acl::mime_quoted_printable mime;
	acl::string out;
	const char *ptr, *end;

#if 1
	const char *text =
		"Ðì¸Õ»Ô <xuganghui> changed: \r\n"
		"What    |Removed             |Added\r\n"
		"-------------------------------------\r\n"
		"Status|NEW                   |ASSIGNED\r\n"
		"AssignedTo|xuganghui         |zhengshuxin\r\n";
#else
	const char *text = "È±ÏÝ 410";
#endif

	ptr = text;
	end = text + strlen(text);

	while (ptr < end) {
		mime.encode_update(ptr, 1, &out);
		ptr++;
	}
	mime.encode_finish(&out);

	printf("encode result length: %d\r\n", (int) out.length());
	printf("(%s)\r\n", out.c_str());

#if 1
	const char *qp = "=D0=EC=B8=D5=BB=D4 <xuganghui> changed:=20\r\n"
		"What    |Removed             |Added\r\n"
		"-------------------------------------\r\n"
		"Status|NEW                   |ASSIGNED\r\n"
		"AssignedTo|xuganghui         |zhengshuxin\r\n";
#else
	const char *qp = "=C8=B1=CF=DD 410";
#endif

	out.clear();

	ptr = qp;
	end = qp + strlen(qp);

	mime.reset();
	mime.add_invalid(true);
	while (ptr < end) {
		mime.decode_update(ptr, 1, &out);
		ptr++;
	}
	mime.decode_finish(&out);
	if (out.length() > 0) {
		printf("decode result length: %d\r\n", (int) out.length());
		printf("(%s)\r\n", out.c_str());
	}

	acl::fstream fp;
	fp.open_trunc("out.txt");
	fp << out.c_str();
	fp.close();

	out.clear();
	//static const unsigned char to_tab[] =
	//	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	//static const unsigned char to_tab[] =
	//	"+-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	//acl::mime_code::create_decode_tab(to_tab, &out);
	//printf("%s", out.c_str());
	getchar();

	return (0);
}
