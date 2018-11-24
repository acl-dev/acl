#include <map>
#include <vector>
#include <algorithm>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

static void ls_file(acl::scan_dir& scan, const char* path, bool recursive,
	bool fullpath)
{
	if (scan.open(path, recursive) == false) {
		logger_error("open path: %s error: %s",
			path, acl::last_serror());
		return;
	}

	int nfiles = 0;
	const char* filename;

	while ((filename = scan.next_file(fullpath)) != NULL) {
		logger("filename: %s, path: %s", filename, scan.curr_path());
		nfiles++;
	}

	logger("===========================================================");
	logger("total file count: %d", nfiles);
}

static void ls_dir(acl::scan_dir& scan, const char* path, bool recursive,
	bool fullpath)
{
	if (scan.open(path, recursive) == false) {
		logger_error("open path: %s error: %s",
			path, acl::last_serror());
		return;
	}

	int ndirs = 0;
	const char* dirname;

	while ((dirname = scan.next_dir(fullpath)) != NULL) {
		logger("dirname: %s, path: %s", dirname, scan.curr_path());
		ndirs++;
	}

	logger("===========================================================");
	logger("total dir count: %d", ndirs);
}

static void ls_all(acl::scan_dir& scan, const char* path, bool recursive,
	bool fullpath)
{
	if (scan.open(path, recursive) == false) {
		logger_error("open path: %s error: %s",
			path, acl::last_serror());
		return;
	}

	int ndirs = 0, nfiles = 0;
	const char* name;
	bool is_file;

	while ((name = scan.next(fullpath, &is_file)) != NULL) {
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

static bool get_relative_path(const char* spath, const char* filepath,
	acl::string& rpath)
{
	size_t n = strlen(spath);
	if (strncmp(filepath, spath, n) != 0)
		return false;
	filepath += n;
	if (*filepath == 0)
		return false;
	rpath = filepath;
	return true;
}

static bool compare_files(const char* sfile, const char* dfile)
{
	acl::string sbuf, dbuf;

	if (acl::ifstream::load(sfile, &sbuf) == false) {
		logger_error("load sfile %s error %s", sfile, acl::last_serror());
		return false;
	}

	if (acl::ifstream::load(dfile, &dbuf) == false) {
		logger_error("load dfile %s error %s", dfile, acl::last_serror());
		return false;
	}

	return sbuf == dbuf;
}

static void diff_path(acl::scan_dir& scan, const char* spath, const char* dpath,
	bool recursive, const char* file_types, bool show_all)
{
	if (scan.open(spath, recursive) == false) {
		logger_error("open path: %s error: %s",
			spath, acl::last_serror());
		return;
	}

	acl::string buf(file_types);
	std::vector<acl::string>& types = buf.split2(",;");
	const char* filepath;

	(void) dpath;
	int n = 0;
	while ((filepath = scan.next_file(true)) != NULL) {
		acl::string path;
		acl::string& file = path.basename(filepath);
		const char* type = strrchr(file.c_str(), '.');
		if (type == NULL || *(++type) == 0)
			continue;

		std::vector<acl::string>::const_iterator it =
			std::find(types.begin(), types.end(),  type);
		if (it == types.end())
			continue;

		path.clear();
		if (get_relative_path(spath, filepath, path) == false)
			continue;

		acl::string dfilepath;
		dfilepath << dpath << path;
		n++;
		bool equal = compare_files(filepath, dfilepath);
		if (!equal) {
			printf("diff %s\t%s\r\n", filepath, dfilepath.c_str());
		} else if (show_all) {
			printf("same %s\t%s\r\n", filepath, dfilepath.c_str());
		}
	}
	printf("---scan over %d files---\r\n", n);
}

static void usage(const char* procname)
{
	logger("usage: %s -h [help]\r\n"
		" -t type[dir|file|all|diff]\r\n"
		" -s source path\r\n"
		" -d destination path\r\n"
		" -e file_types\r\n"
		" -A [show all status including same files]\r\n"
		" -r [if recursive, default: false]\r\n"
		" -a [if get fullpath, default: false]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch;
	bool  recursive = false, fullpath = false, show_all = false;
	acl::string dpath, spath, mode, types("c;cpp;cc;cxx;h;hpp;hxx");
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hs:d:t:rae:A")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			spath = optarg;
			break;
		case 'd':
			dpath = optarg;
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
		case 'e':
			types = optarg;
			break;
		case 'A':
			show_all = true;
			break;
		default:
			break;
		}
	}

	if (spath.empty()) {
		usage(argv[0]);
		return 1;
	}

	if (!spath.end_with("/"))
		spath += "/";

	acl::scan_dir scan;

	if (mode == "dir")
		ls_dir(scan, spath, recursive, fullpath);
	else if (mode == "file")
		ls_file(scan, spath, recursive, fullpath);
	else if (mode == "diff") {
		if (dpath.empty()) {
			usage(argv[0]);
			return 1;
		}
		if (!dpath.end_with("/"))
			dpath += "/";
		diff_path(scan, spath, dpath, recursive, types, show_all);
	} else
		ls_all(scan, spath, recursive, fullpath);

#ifdef	WIN32
	logger("===========================================================");
	logger("enter any key to exit");
	getchar();
#endif

	return (0);
}
