#include "acl_cpp/lib_acl.hpp"

#ifdef WIN32
#define strdup _strdup
#endif

int main(void)
{
	char* s = strdup("aaa\r\n\r\n"
			"bbb\r\n\n"
			"ccc\n\r\n"
			"ddd\n\n"
			"eee\r\n"
			"fff\n"
			"ggg\r\n\r\r\n"
			"hhh\r\r\r\n\r\n"
			"iii\r\n");
	char* ptr = s;
	acl::string buf;
	acl::string head;
	int  nleft;

	std::vector<acl::string> first;

	while (*ptr != 0)
	{
		buf += *ptr;

		head.clear();
		int offset = buf.find_blank_line(&nleft, &head);
		if (offset > 0)
		{
			printf("ok, findit, offset: %d, offset: %d, "
				"header: [%s]\r\n",
				offset, nleft, head.c_str());
			first.push_back(head);
		}
		ptr++;
	}

	printf("-------------------------------------------------------\r\n");

	buf.find_reset();
	printf("total len: %d\r\n", (int) buf.length());

	std::vector<acl::string> second;

	while (true)
	{
		head.clear();
		int offset = buf.find_blank_line(&nleft, &head);
		if (offset > 0)
		{
			printf("ok, findit, offset: %d, nleft: %d, len: %d,"
				" header: [%s]\r\n", offset, nleft,
				(int) head.length(), head.c_str());
			second.push_back(head);
		}
		if (nleft == 0)
			break;
	}

	printf("-------------------------------------------------------\r\n");

	buf.find_reset();
	int  last_offset = 0;

	std::vector<acl::string> third;

	while (true)
	{
		int offset = buf.find_blank_line(&nleft);
		if (offset > 0)
		{
			head.clear();
			size_t len = buf.substr(head, last_offset,
					offset - last_offset);
			if (len  == 0)
			{
				printf("error, last_offset: %d, offset: %d\r\n",
					last_offset, offset);
				break;
			}
			printf("ok, offset: %d, last_offset: %d, len: %d, %d,"
				" buf: [%s]\r\n", offset, last_offset,
				(int) len, (int) head.length(), head.c_str());
			last_offset = offset;
			third.push_back(head);
		}
		if (nleft == 0)
			break;
	}

	free(s);

	printf("-------------------------------------------------------\r\n");

	if (first != second)
		printf("error, first != second\r\n");
	else if (second != third)
		printf("error, second != third\r\n");
	else
		printf("All OK\r\n\r\n");

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif

	return 0;
}
