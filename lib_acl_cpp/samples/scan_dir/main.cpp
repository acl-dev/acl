#include <map>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

static void ls_file(acl::scan_dir& scan, const char* path, bool recursive,
	bool fullpath)
{
	if (scan.open(path, recursive) == false)
	{
		logger_error("open path: %s error: %s",
			path, acl::last_serror());
		return;
	}

	int nfiles = 0;
	const char* filename;

	while ((filename = scan.next_file(fullpath)) != NULL)
	{
		logger("filename: %s, path: %s", filename, scan.curr_path());
		nfiles++;
	}

	logger("===========================================================");
	logger("total file count: %d", nfiles);
}

static void ls_dir(acl::scan_dir& scan, const char* path, bool recursive,
	bool fullpath)
{
	if (scan.open(path, recursive) == false)
	{
		logger_error("open path: %s error: %s",
			path, acl::last_serror());
		return;
	}

	int ndirs = 0;
	const char* dirname;

	while ((dirname = scan.next_dir(fullpath)) != NULL)
	{
		logger("dirname: %s, path: %s", dirname, scan.curr_path());
		ndirs++;
	}

	logger("===========================================================");
	logger("total dir count: %d", ndirs);
}

static void ls_all(acl::scan_dir& scan, const char* path, bool recursive,
	bool fullpath)
{
	if (scan.open(path, recursive) == false)
	{
		logger_error("open path: %s error: %s",
			path, acl::last_serror());
		return;
	}

	int ndirs = 0, nfiles = 0;
	const char* name;
	bool is_file;

	while ((name = scan.next(fullpath, &is_file)) != NULL)
	{
		logger("%s: %s, path: %s", is_file ? "filename" : "dirname",
			name, scan.curr_path());
		if (is_file)
			nfiles++;
		else
			ndirs++;
	}

	logger("===========================================================");
	logger("total dir count: %d, file count: %d", ndirs, nfiles);
}

static void usage(const char* procname)
{
	logger("usage: %s -h [help] -d path -t type[dir|file|all] "
		"-r [if recursive, default: false] "
		"-a [if get fullpath, default: false]", procname);
}

int main(int argc, char* argv[])
{
	int   ch;
	bool  recursive = false, fullpath = false;
	acl::string path, mode;
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hd:t:ra")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'd':
			path = optarg;
			break;
		case 't':
			mode = optarg;
			break;
		case 'r':
			recursive = true;
			break;
		case 'a':
			fullpath = true;
			break;
		default:
			break;
		}
	}

	if (path.empty())
	{
		usage(argv[0]);
		return 1;
	}

	acl::scan_dir scan;

	if (mode == "dir")
		ls_dir(scan, path, recursive, fullpath);
	else if (mode == "file")
		ls_file(scan, path, recursive, fullpath);
	else
		ls_all(scan, path, recursive, fullpath);

#ifdef	WIN32
	logger("===========================================================");
	logger("enter any key to exit");
	getchar();
#endif

	return (0);
}
