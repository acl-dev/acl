#include "stdafx.h"
#include "charset_transfer.h"

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -f from_charset\r\n"
		" -t to_charset\r\n"
		" -b [when to_charset is utf-8 if BOM header be added]\r\n"
		" -s source_dir\r\n"
		" -d destination_dir\r\n"
		" -c [just check charset only]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int ch;
	acl::string from_charset, to_charset, from_dir, to_dir;
	bool use_bom = false, check_only = false;;

	while ((ch = getopt(argc, argv, "hf:t:bs:d:c")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			from_charset = optarg;
			break;
		case 't':
			to_charset = optarg;
			break;
		case 'b':
			use_bom = true;
			break;
		case 's':
			from_dir = optarg;
			break;
		case 'd':
			to_dir = optarg;
			break;
		case 'c':
			check_only = true;
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);

	if (check_only)
	{
		if (from_dir.empty() || from_charset.empty())
		{
			printf("from_charset or from_dir not set\r\n");
			return 1;
		}

		int n = charset_transfer::check_path(from_dir, from_charset);

		printf("check over: %d, charset: %s\r\n",
			n, from_charset.c_str());
		return 0;
	}

	if (from_charset.empty() || to_charset.empty()
		|| from_dir.empty() || to_dir.empty())
	{
		usage(argv[0]);
		return 0;
	}

	if (chdir(from_dir.c_str()) == -1)
	{
		logger_error("chdir to %s error %s", from_dir.c_str(),
			acl::last_serror());
		return -1;
	}

	charset_transfer transfer;
	transfer.set_from_charset(from_charset)
		.set_to_charset(to_charset)
		.set_from_path(".")
		.set_to_path(to_dir)
		.set_utf8bom(use_bom);

	int n = transfer.transfer();
	printf("transfer over: %d, from_charset: %s, to_charset: %s,"
		" from_path: %s, to_path: %s\r\n", n, from_charset.c_str(),
		to_charset.c_str(), from_dir.c_str(), to_dir.c_str());

	return 0;
}
