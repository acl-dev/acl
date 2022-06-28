#include <unistd.h>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

static void check_time(ACL_SCAN_DIR* scan, const char* path)
{
	struct acl_stat sbuf;
	if (acl_scan_dir_stat(scan, &sbuf) == -1) {
		printf("acl_scan_stat %s error\r\n", path);
		exit(1);
	}

	struct acl_stat sbuf2;
	if (acl_stat(path, &sbuf2) == -1) {
		printf("acl_stat %s error %s\r\n", path, acl::last_serror());
		exit(1);
	}

	acl::string info;
	if (sbuf2.st_atime != sbuf.st_atime) {
		info.format_append("\tst_atime changed, now=%ld, %ld, %ld\r\n",
			time(NULL), sbuf2.st_atime, sbuf.st_atime);
	}
	if (sbuf2.st_ctime != sbuf.st_ctime) {
		info.format_append("\tst_ctime changed, now=%ld, %ld, %ld\r\n",
			time(NULL), sbuf2.st_ctime, sbuf.st_ctime);
	}
	if (sbuf2.st_mtime != sbuf.st_mtime) {
		info.format_append("\tst_mtime changed, now=%ld, %ld, %ld\r\n",
			time(NULL), sbuf2.st_mtime, sbuf.st_mtime);
	}

	acl::rfc822 rfc;
	char atime[256], ctime[256], mtime[256];
	rfc.mkdate_cst(sbuf.st_atime, atime, sizeof(atime));
	rfc.mkdate_cst(sbuf.st_ctime, ctime, sizeof(ctime));
	rfc.mkdate_cst(sbuf.st_mtime, mtime, sizeof(mtime));
	printf("path=%s\r\n", path);
	if (!info.empty()) {
		printf("%s", info.c_str());
	}
	printf("\tst_atime=%s, %ld\r\n"
		"\tst_ctime=%s, %ld\r\n"
		"\tst_mtime=%s, %ld\r\n",
		atime, sbuf.st_atime, ctime, sbuf.st_ctime,
		mtime, sbuf.st_mtime);
}

class myscan_dir : public acl::scan_dir
{
public:
	myscan_dir(void) {}
	~myscan_dir(void) {}

	bool rmdir_callback(const char* path) {
		ACL_SCAN_DIR* scan = this->get_scan_dir();
		check_time(scan, path);

		if (::rmdir(path) == 0) {
			printf(">> %s: rmdir ok, path=%s\r\n",
				__FUNCTION__, path);
			return true;
		} else {
			printf(">> %s: rmdir error=%s, path=%s\r\n",
				__FUNCTION__, acl::last_serror(), path);
			return false;
		}
	}
};

static void scan_rmdir(acl::scan_dir& scan)
{
	int ndirs = 0, nfiles = 0;
	const char* name;
	bool is_file;

	while ((name = scan.next(true, &is_file)) != NULL) {
		printf("[scan %s: %s, path: %s]\r\n",
			is_file ? "filename" : "dirname",
			name, scan.curr_path());
		if (is_file) {
			nfiles++;
		} else {
			ndirs++;
		}

		if (nfiles >= 50 || ndirs >= 50) {
			break;
		}
	}
}

static bool rmdir_on_empty(const char* path, bool recursive, bool rmdir_on)
{
	myscan_dir scan; 

	if (scan.open(path, recursive, rmdir_on) == false) {
		printf("open path: %s error: %s\r\n", path, acl::last_serror());
		return false;
	}


	scan_rmdir(scan);
	return true;
}

static int rmdir_fn(ACL_SCAN_DIR* scan, const char* path, void* ctx)
{
	acl::scan_dir* me = (acl::scan_dir*) ctx;
	assert(scan == me->get_scan_dir());

	check_time(scan, path);
	if (::rmdir(path) == -1) {
		printf(">> %s: rmdir error=%s, path=%s\r\n",
			__FUNCTION__, acl::last_serror(), path);
		return -1;
	} else {
		printf(">> %s: rmdir ok, path=%s\r\n", __FUNCTION__, path);
		return 0;
	}
}

static bool rmdir_on_empty2(const char* path, bool recursive, bool rmdir_on)
{
	acl::scan_dir scan;

	if (scan.open(path, recursive, rmdir_on) == false) {
		printf("open path: %s error: %s\r\n", path, acl::last_serror());
		return false;
	}

	if (rmdir_on) {
		scan.set_rmdir_callback(rmdir_fn, &scan);
	}

	scan_rmdir(scan);
	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -d path\r\n"
		" -R [rmdir_on]\r\n"
		" -r [if recursive, default: false]\r\n"
		" -C [using myscan_dir C++ class]\r\n"
		" -a [if get fullpath, default: false]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch;
	bool  rmdir_on = false, recursive = false, use_class = false;
	acl::string path;
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hd:raRC")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'd':
			path = optarg;
			break;
		case 'r':
			recursive = true;
			break;
		case 'R':
			rmdir_on = true;
			break;
		case 'C':
			use_class = true;
			break;
		default:
			break;
		}
	}

	if (path.empty()) {
		usage(argv[0]);
		return 1;
	}

	if (!path.end_with("/")) {
		path += "/";
	}

	if (use_class) {
		rmdir_on_empty(path, recursive, rmdir_on);
	} else {
		rmdir_on_empty2(path, recursive, rmdir_on);
	}

#ifdef	WIN32
	printf("========================================================\r\n");
	printf("enter any key to exit!\r\n");
	getchar();
#endif

	return (0);
}
