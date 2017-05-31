#include "stdafx.h"
#include <vector>
#include <string>
#include <iostream>

static void create_files(const std::vector<std::string>& files)
{
	acl::gsoner gr;
	gr.read_multi_file(files);
	gr.parse_code();
	gr.gen_gson();
}

static bool copy_file(const char* from, const char* to)
{
	acl::string buf;
	if (acl::ifstream::load(from, &buf) == false)
	{
		printf("read %s error %s\r\n", from, acl::last_serror());
		return false;
	}

	acl::ofstream out;
	if (out.open_trunc(to) == false)
	{
		printf("open %s error %s\r\n", to, acl::last_serror());
		return false;
	}

	if (out.write(buf) == -1)
	{
		printf("write to %s error %s\r\n", to, acl::last_serror());
		return false;
	}

	return true;
}

static bool copy_files(const char* to_path)
{
	const char* files[] = {
		"gson.cpp",
		"gson.h",
		NULL,
	};

	acl::string buf, filepath;

	for (size_t i = 0; files[i] != NULL; i++)
	{
		buf.basename(files[i]);
		filepath.format("%s/%s", to_path, buf.c_str());

		if (copy_file(files[i], filepath) == false)
			return false;
	}

	return true;
}

static void scan_path(std::vector<std::string>& files)
{
	acl::scan_dir scan;

	if (scan.open(".") == false)
	{
		printf("scan open error %s\r\n", acl::last_serror());
		return;
	}

	const char* file;

	while ((file = scan.next_file(false)) != NULL)
	{
		if (acl_strrncasecmp(file, ".stub", 2) == 0)
		{
			char buf[1024];

			snprintf(buf, sizeof(buf), "%s", file);
			char* dot = strrchr(buf, '.');
			assert(dot);
			*dot = 0;
			strcat(buf, ".h");

			assert(copy_file(file, buf));

			files.push_back(buf);
		}
	}
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -f header_file\r\n"
		" -d header_file_path\r\n", procname);
}

int main(int argc, char* argv[])
{
	std::vector<std::string> files;
	std::string path;
	int  ch;

	while ((ch = getopt(argc, argv, "hf:d:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			files.push_back(optarg);
			break;
		case 'd':
			path = optarg;
			break;
		default:
			break;
		}
	}

	char buf[1024];
	if (getcwd(buf, sizeof(buf)) == NULL)
	{
		printf("getcwd error %s\r\n", acl::last_serror());
		return 1;
	}

	if (!path.empty())
	{
		if (chdir(path.c_str()) < 0)
		{
			printf("chdir to %s error %s\r\n", path.c_str(),
				acl::last_serror());
			return 1;
		}

		scan_path(files);
	}

	if (files.empty())
	{
		usage(argv[0]);
		return 1;
	}

	create_files(files);

	// return the saved working path
	if (chdir(buf) < 0)
	{
		printf("chdir to %s error %s\r\n", buf, acl::last_serror());
		return 1;
	}
	if(path.empty())
	{
		path = files.front();
		std::size_t pos = path.find_last_of('/');
		if (pos == std::string::npos)
			pos = path.find_last_of('\\');
		if (pos == std::string::npos)
			return 0;
		path = path.substr(0, pos);
		if(path.empty() == false)
			copy_files(path.c_str());
	}
	return 0;
}
