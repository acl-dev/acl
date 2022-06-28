#include "acl_cpp/mime/mime_xxcode.hpp"
#include <getopt.h>
#include <stdio.h>

static void encoding(const char* s)
{
	acl::mime_xxcode coder(false, false);
	acl::string out;

	coder.encode_update(s, strlen(s), &out);
	coder.encode_finish(&out);
	printf(">>>%s\n", out.c_str());
}

static void decoding(const char* s)
{
	acl::mime_xxcode coder(false, false);
	acl::string out;

	coder.decode_update(s, strlen(s), &out);
	coder.decode_finish(&out);
	printf(">>>%s\n", out.c_str());
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -e plain -d txt\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch;
	void (*action)(const char*) = NULL;
	acl::string s;

	while ((ch = getopt(argc, argv, "he:d:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return (0);
		case 'e':
			action = encoding;
			s = optarg;
			break;
		case 'd':
			action = decoding;
			s = optarg;
			break;
		default:
			break;
		}
	}

	if (action && !s.empty())
		action(s);
	else
		usage(argv[0]);

	return (0);
}
