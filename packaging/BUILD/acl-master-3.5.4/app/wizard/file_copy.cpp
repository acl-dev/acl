#include "stdafx.h"
#include "file_copy.h"

bool file_copy(const char* from, const char* to)
{
	if (strcasecmp(from, to) == 0)
	{
		printf("from(%s) == to(%s)\r\n", from, to);
		return false;
	}

	ifstream in;
	if (in.open_read(from) == false)
	{
		printf("open %s error: %s\r\n", from, last_serror());
		return false;
	}

	ofstream out;
	if (out.open_write(to) == false)
	{
		printf("open %s error: %s\r\n", from, last_serror());
		return false;
	}

	string buf;

	while (!in.eof())
	{
		if (in.gets(buf, false) == false)
			break;
		if (out.write(buf) < 0)
		{
			printf("write to %s error: %s\r\n", to, last_serror());
			return false;
		}
	}

	printf("create %s ok\r\n", to);
	return true;
}

bool files_copy(const char* name, const FILE_FROM_TO* tab,
	const char* from_path, const char* to_path)
{
	string from, to;

	from = "tmpl/Makefile.in";
	to.format("%s/Makefile.in", to_path);
	if (file_copy(from.c_str(), to.c_str()) == false)
		return false;

	for (size_t i = 0; tab[i].from != NULL; i++)
	{
		from.format("%s/%s", from_path, tab[i].from);
		to.format("%s/%s", to_path, tab[i].to);
		if (file_copy(from, to) == false)
		{
			printf("create %s failed!\r\n", name);
			return false;
		}
	}

	printf("create %s ok!\r\n", name);
	return true;
}
