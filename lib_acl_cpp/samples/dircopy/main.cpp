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

static bool copy_file(acl::ifstream& in, const acl::string& to_path,
	const acl::string& to_filepath, int* ncopied)
{
	if (in.fseek(0, SEEK_SET) < 0)
	{
		logger_error("fseek from file: %s error: %s",
			in.file_path(), acl::last_serror());
		return false;
	}

	if (access(to_path.c_str(), 0) != 0)
	{
		if (acl_make_dirs(to_path.c_str(), 0755) == -1)
		{
			logger_error("create dirs: %s error: %s",
				to_path.c_str(), acl::last_serror());
			return false;
		}
	}

	acl_int64 length = in.fsize();
	if (length < 0)
	{
		logger_error("get file(%s)'s size error: %s", in.file_path(),
			acl::last_serror());
		return false;
	}

	acl::ofstream out;

	if (out.open_trunc(to_filepath.c_str()) == false)
	{
		logger_error("ope_trunc file: %s error: %s",
			to_filepath.c_str(), acl::last_serror());
		return false;
	}

	logger("copying from file: %s, to file: %s", in.file_path(),
		to_filepath.c_str());

	char  buf[4096];
	int   ret;
	acl_int64 nread = 0;

	while (true)
	{
		ret = in.read(buf, sizeof(buf), false);
		if (ret == -1)
		{
			if (nread == length)
				break;

			logger_error("read from file: %s error: %s, "
				"to file: %s, nread: %lld, length: %lld",
				in.file_path(), acl::last_serror(),
				to_filepath.c_str(), nread, length);
			return false;
		}

		nread += ret;

		if (out.write(buf, ret) == -1)
		{
			logger_error("write to file: %s error: %s",
				to_filepath.c_str(), acl::last_serror());
			return false;
		}
	}

	(*ncopied)++;

	return true;
}

static bool cmp_copy(acl::scan_dir& scan, const char* name,
	const acl::string& to_path, int* ncopied)
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

	if (strstr(from_filepath.c_str(), ".svn") != NULL
		|| strstr(from_filepath.c_str(), ".git") != NULL
		|| strstr(from_filepath.c_str(), ".cvs") != NULL
		|| strstr(from_filepath.c_str(), ".inc") != NULL
		|| strstr(from_filepath.c_str(), ".exe") != NULL
		|| strstr(from_filepath.c_str(), ".zip") != NULL
		|| strstr(from_filepath.c_str(), ".rar") != NULL
		|| strstr(from_filepath.c_str(), ".tar") != NULL
		|| strstr(from_filepath.c_str(), ".tar.gz") != NULL
		|| strstr(from_filepath.c_str(), ".tgz") != NULL
		|| strstr(from_filepath.c_str(), ".bzip2") != NULL
		|| strstr(from_filepath.c_str(), ".o") != NULL)
	{
		return true;
	}

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

	acl::ifstream to_fp;
	if (to_fp.open_read(to_filepath.c_str()) == false)
	{
		//printf("open to file: %s error: %s\r\n", to_filepath.c_str(),
		//	acl::last_serror());
		return copy_file(from_fp, to_pathbuf, to_filepath, ncopied);
	}


	acl_int64 length;
	if ((length = to_fp.fsize()) != from_fp.fsize())
	{
		printf("to fsize: %ld, from fsize: %ld, to file: %s, "
			"from file: %s\r\n", (long) to_fp.fsize(),
			(long) from_fp.fsize(), to_filepath.c_str(),
			from_filepath.c_str());
		to_fp.close();
		return copy_file(from_fp, to_pathbuf, to_filepath, ncopied);
	}

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
			to_fp.close();
			printf("fread from to file error: %s\r\n",
				acl::last_serror());
			return copy_file(from_fp, to_pathbuf,
					to_filepath, ncopied);
		}

		if (memcmp(from_buf, to_buf, to_len) != 0)
		{
			to_fp.close();
			logger("string not equal, from: %s, to: %s",
				from_fp.file_path(), to_filepath.c_str());
			return copy_file(from_fp, to_pathbuf,
					to_filepath, ncopied);
		}
	}
}

static bool check_dir(acl::scan_dir& scan, const char* to, int* ncopied)
{
	const char* rpath = scan.curr_path();
	if (rpath == NULL)
	{
		logger_error("get from's path error: %s, to: %s",
			acl::last_serror(), to);
		return false;
	}

	SKIP(rpath);

	acl::string to_path;
	to_path << to << SEP << rpath;

	if (strstr(to_path.c_str(), ".svn") != NULL
		|| strstr(to_path.c_str(), ".git") != NULL
		|| strstr(to_path.c_str(), ".cvs") != NULL)
	{
		return true;
	}

	// printf(">>to_path: %s, to: %s\r\n", to_path.c_str(), to);

	if (access(to_path.c_str(), 0) == 0)
		return true;

	int ret = acl_make_dirs(to_path.c_str(), 0755);
	if (ret == 0)
	{
		(*ncopied)++;
		return true;
	}

	logger_error("make dirs(%s) error: %s",
		to_path.c_str(), acl::last_serror());
	return false;
}

static void do_copy(const acl::string& from, const acl::string& to)
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
	int   nfiles = 0, ndirs = 0, nfiles_copied = 0, ndirs_copied = 0;
	while ((name = scan.next(false, &is_file)) != NULL)
	{
		SKIP(name);

		if (is_file)
		{
			if (cmp_copy(scan, name, to, &nfiles_copied) == false)
			{
				printf(">>cm_copy failed, name: %s\r\n", name);
				break;
			}
			nfiles++;
		}
		else if (check_dir(scan, to, &ndirs_copied) == false)
		{
			printf(">>check_dir failed, name: %s\r\n", name);
			break;
		}
		else
			ndirs++;

		if ((nfiles + ndirs) % 100 == 0)
		{
			printf("current file count: copied %d / scaned %d, "
				"dir count: copied %d / scaned %d\r",
				nfiles_copied, nfiles, ndirs_copied, ndirs);
			fflush(stdout);
		}
	}

	printf("total file count: copyied %d / scaned %d, dir count: "
		"copied %d / scaned %d\r\n", nfiles_copied, nfiles,
		ndirs_copied, ndirs);
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
	logger("current path: %s", path);

	do_copy(".", path_to);

	logger("enter any key to exit");
	getchar();

	return (0);
}
