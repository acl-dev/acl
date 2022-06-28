#include "lib_acl.h"
#ifdef WIN32
#include <io.h>
#include <direct.h>
#define access _access
#define chdir _chdir
#define getcwd _getcwd
#endif // WIN32
#include "acl_cpp/lib_acl.hpp"

#ifdef WIN32
#define SEP	'\\'
#else
#define SEP	'/'
#endif

// 去年路径前的 "./" 或 ".\"，因为在 WIN32 下
#define SKIP(ptr) do  \
{  \
	if (*ptr == '.' && *(ptr + 1) == '/')  \
		ptr += 2;  \
	else if (*ptr == '.' && *(ptr + 1) == '\\')  \
		ptr += 2;  \
} while (0)

static bool cmp_file(acl::scan_dir& scan, const char* name,
	const acl::string& to_path, int* ndiff)
{
	const char* rpath = scan.curr_path();
	if (rpath == NULL)
	{
		logger_error("get current path error: %s, file: %s",
			acl::last_serror(), name);
		return false;
	}

	SKIP(rpath);
	SKIP(name);

//	printf(">>rpath: %s\r\n", rpath);
//	printf(">>name: %s\r\n", name);

	acl::string from_filepath;
	if (*rpath == 0)
		from_filepath << name;
	else
		from_filepath << rpath << SEP << name;

	// 只读方式打开源文件，如果打开失败，则直接返回 false
	acl::ifstream from_fp;
	if (from_fp.open_read(from_filepath.c_str()) == false)
	{
		logger_error("open source file: %s error: %s",
			from_filepath.c_str(), acl::last_serror());
		return false;
	}

	acl::string to_pathbuf;
	acl::string to_filepath;
	to_pathbuf << to_path << SEP << rpath;
	to_filepath << to_path << SEP << rpath << SEP << name;

	//printf("from_filepath: %s, to_filepath: %s\r\n",
	//	from_fp.file_path(), to_filepath.c_str());

	// 尝试打开目标文件，如果目标文件不存在，则将计数器加1
	acl::ifstream to_fp;
	if (to_fp.open_read(to_filepath.c_str()) == false)
	{
		(*ndiff)++;
		logger("file diff, from: %s, to: %s", from_filepath.c_str(),
			to_filepath.c_str());
		return true;
	}

	// 先比较目标文件与源文件大小是否相同
	acl_int64 length;
	if ((length = to_fp.fsize()) != from_fp.fsize())
	{
		(*ndiff)++;
		logger("file diff, from: %s, to: %s", from_filepath.c_str(),
			to_filepath.c_str());
		return true;
	}

	// 再比较目标文件与源文件内容是否相同
	char from_buf[4096], to_buf[4096];
	int from_len, to_len;
	acl_int64 read_len = 0;

	while (true)
	{
		from_len = from_fp.read(from_buf, sizeof(from_buf), false);
		if (from_len == -1)
		{
			if (read_len == length)
				return true;
#ifdef WIN32
			logger_error("read from file(%s) error(%s),"
				"file size: %I64d read len: %I64d",
				from_fp.file_path(), acl::last_serror(),
				length, read_len);
#else
			logger_error("read from file(%s) error(%s),"
				"file size: %lld, read len: %lld",
				from_fp.file_path(), acl::last_serror(),
				length, read_len);
#endif

			return false;
		}

		read_len += from_len;
		to_len = to_fp.read(to_buf, from_len, true);
		if (to_len == -1)
		{
			(*ndiff)++;
			logger("file diff, from: %s, to: %s",
				from_filepath.c_str(), to_filepath.c_str());
			return true;
		}

		if (memcmp(from_buf, to_buf, to_len) != 0)
		{
			(*ndiff)++;
			logger("file diff, from: %s, to: %s",
				from_filepath.c_str(), to_filepath.c_str());
			return true;
		}
	}
}

static bool check_dir(acl::scan_dir& scan, const char* to, int* ndiff)
{
	const char* rpath = scan.curr_path();
	if (rpath == false)
	{
		logger_error("get from's path error: %s, to: %s",
			acl::last_serror(), to);
		return false;
	}

	SKIP(rpath);

	acl::string to_path;
	to_path << to << SEP << rpath;
	// printf(">>to_path: %s, to: %s\r\n", to_path.c_str(), to);

	if (access(to_path.c_str(), 0) == 0)
		return true;

	(*ndiff)++;
	logger("dir diff: %s, error: %s", to_path.c_str(), acl::last_serror());

	return true;
}

static void do_cmp(const acl::string& from, const acl::string& to)
{
	acl::scan_dir scan;
	if (scan.open(from.c_str()) == false)
	{
		logger_error("open path: %s error: %s",
			from.c_str(), acl::last_serror());
		return;
	}

	const char* name;
	bool  is_file;
	int   nfiles = 0, ndirs = 0, nfiles_diff = 0, ndirs_diff = 0;
	while ((name = scan.next(false, &is_file)) != NULL)
	{
		SKIP(name);

		if (is_file)
		{
			if (cmp_file(scan, name, to, &nfiles_diff) == false)
			{
				printf(">>cmp failed, name: %s\r\n", name);
				break;
			}
			nfiles++;
		}
		else if (check_dir(scan, to, &ndirs_diff) == false)
		{
			printf(">>check_dir failed, name: %s\r\n", name);
			break;
		}
		else
			ndirs++;

		if ((nfiles + ndirs) % 100 == 0)
		{
			printf("current file count: diff %d / scaned %d, "
				"dir count: diff %d / scaned %d\r",
				nfiles_diff, nfiles, ndirs_diff, ndirs);
			fflush(stdout);
		}
	}

	printf("total file count: diff %d / scaned %d, dir count: "
		"diff %d / scaned %d\r\n", nfiles_diff, nfiles,
		ndirs_diff, ndirs);
}

static void usage(const char* procname)
{
	logger_error("usage: %s -h [help] -f from_path -t to_path", procname);
}

int main(int argc, char* argv[])
{
	int  ch;
	acl::string path_from, path_to;
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hf:t:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			path_from = optarg;
			break;
		case  't':
			path_to = optarg;
			break;
		default:
			break;
		}
	}

	if (path_from.empty() || path_to.empty())
	{
		usage(argv[0]);
		return 1;
	}

	if (path_from == path_to)
	{
		logger_error("path_from(%s) == path_to(%s)", path_from.c_str(),
			path_to.c_str());
		usage(argv[0]);
		return 1;
	}

	if (chdir(path_from.c_str()) == -1)
	{
		logger_error("chdir to %s error: %s",
			path_from.c_str(), acl::last_serror());
		return 1;
	}

	char path[256];
	if (getcwd(path, sizeof(path)) == NULL)
	{
		logger_error("getcwd error: %s", path);
		return 1;
	}
	logger_error("current path: %s", path);

	do_cmp(".", path_to);

	logger("enter any key to exit");
	getchar();

	return (0);
}
