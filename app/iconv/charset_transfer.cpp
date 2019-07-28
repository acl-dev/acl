#include "stdafx.h"
#include "charset.h"
#include "charset_transfer.h"

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

static const char UTF8_HEADER[] = { (char) 0xEF, (char) 0xBB, (char) 0xBF };

charset_transfer& charset_transfer::set_from_charset(const char* charset)
{
	from_charset_ = charset;
	return *this;
}

charset_transfer& charset_transfer::set_to_charset(const char* charset)
{
	to_charset_ = charset;
	return *this;
}

charset_transfer& charset_transfer::set_from_path(const char* path)
{
	from_path_ = path;
	return *this;
}

charset_transfer& charset_transfer::set_to_path(const char* path)
{
	to_path_ = path;
	return *this;
}

charset_transfer& charset_transfer::set_utf8bom(bool yes)
{
	utf8_bom_ = yes;
	return *this;
}

bool charset_transfer::check_params(void)
{
	if (from_charset_.empty())
	{
		logger_error("call set_from_charset first!");
		return false;
	}
	if (to_charset_.empty())
	{
		logger_error("call set_to_charset first!");
		return false;
	}
	if (from_path_.empty())
	{
		logger_error("call set_from_path first!");
		return false;
	}
	if (to_path_.empty())
	{
		logger_error("call set_to_path first!");
		return false;
	}
	return true;
}

bool charset_transfer::get_filepath(acl::scan_dir& scan, const char* filename,
	acl::string& from_filepath, acl::string& to_filepath,
	acl::string& to_path)
{
	const char* rpath = scan.curr_path();
	if (rpath == NULL)
	{
		logger_error("curr_path NULL, filename: %s", filename);
		return false;
	}

	SKIP(rpath);
	SKIP(filename);

	if (*rpath == 0)
		from_filepath << filename;
	else
		from_filepath << rpath << SEP << filename;

#if 0
	if (strstr(from_filepath.c_str(), ".svn") != NULL
		|| strstr(from_filepath.c_str(), ".git") != NULL
		|| strstr(from_filepath.c_str(), ".cvs") != NULL
		|| strstr(from_filepath.c_str(), ".inc") != NULL
		|| strstr(from_filepath.c_str(), ".exe") != NULL
		|| strstr(from_filepath.c_str(), ".class") != NULL
		|| strstr(from_filepath.c_str(), ".zip") != NULL
		|| strstr(from_filepath.c_str(), ".rar") != NULL
		|| strstr(from_filepath.c_str(), ".tar") != NULL
		|| strstr(from_filepath.c_str(), ".tar.gz") != NULL
		|| strstr(from_filepath.c_str(), ".tgz") != NULL
		|| strstr(from_filepath.c_str(), ".bzip2") != NULL
		|| strstr(from_filepath.c_str(), ".o") != NULL)
	{
		logger("skip %s", from_filepath.c_str());
		return false;
	}
#else
	static const char* files_ext[] = {
		".c",
		".h",
		".cpp",
		".hpp",
		".cxx",
		".hxx",
		NULL,
	};

	bool match = false;
	for (int i = 0; files_ext[i] != NULL; i++)
	{
		if (from_filepath.rncompare(files_ext[i],
			strlen(files_ext[i]), false) == 0)
		{
			match = true;
			break;
		}
	}
	if (!match)
		return false;
#endif

	to_path << to_path_ << SEP << rpath;
	to_filepath << to_path << SEP << filename;
	return true;
}

bool charset_transfer::check_buff(const acl::string& buf, const char* charset,
	acl::string& res)
{
	if (buf[0] == UTF8_HEADER[0] && buf[1] == UTF8_HEADER[1]
		&& buf[2] == UTF8_HEADER[2])
	{
		res = "utf-8";
	}
	else
	{
		charset_radar r;

		if (r.detact(buf, res) == false)
		{
			res = "uknown";
			return false;
		}
	}

#define EQ !strcasecmp

	if (res.equal("UTF-8", false)
		&& (EQ(charset, "utf-8") || EQ(charset, "utf8")))
	{
		return true;
	}
	else if (res.equal("GB18030", false)
		&& (EQ(charset, "gbk") || EQ(charset, "gb2312")))
	{
		return true;
	}
	else if (res.equal(charset, false))
		return true;
	else
		return false;
}

bool charset_transfer::check_file(const char* filepath,
	const char* charset)
{
	acl::string buf;
	if (acl::ifstream::load(filepath, &buf) == false)
	{
		logger_error("load %s error %s", filepath, acl::last_serror());
		return false;
	}

	acl::string res;
	if (check_buff(buf, charset, res) == false)
	{
		logger("%s, guess: %s, want: %s",
			filepath, res.c_str(), charset);
		return false;
	}
	return true;
}

int charset_transfer::check_path(const char* path, const char* charset)
{
	acl::scan_dir scan;
	if (scan.open(path, true) == false)
	{
		logger_error("open %s error %s", path, acl::last_serror());
		return -1;
	}

	const char* filepath;
	int count = 0;
	while ((filepath = scan.next_file(true)) != NULL)
	{
		if (check_file(filepath, charset))
			count++;
	}

	return count;
}

int charset_transfer::transfer(bool recursive /* = true */)
{
	if (check_params() == false)
		return -1;

	if (from_charset_.equal(to_charset_, false))
	{
		logger("to_charset_ is same as from_charset_(%s)",
			from_charset_.c_str());
		return 0;
	}

	acl::scan_dir scan;

	if (scan.open(from_path_, recursive) == false)
	{
		logger_error("open dir %s error %s", from_path_.c_str(),
			acl::last_serror());
		return -1;
	}

	int count = 0;
	const char* filename;

	while ((filename = scan.next_file(false)) != NULL)
	{
		acl::string from_filepath, to_filepath, to_path;
		if (!get_filepath(scan, filename, from_filepath,
			to_filepath, to_path))
		{
			continue;
		}

		if (access(to_path.c_str(), 0) != 0
			&& (acl_make_dirs(to_path.c_str(), 0755) == -1))
		{
			logger_error("acl_make_dirs %s error %s",
				to_path.c_str(), acl::last_serror());
			continue;
		}

		if (transfer(from_filepath, to_filepath))
		{
			logger("transfer to %s OK!", to_filepath.c_str());
			count++;
		}
	}

	return count;
}

bool charset_transfer::transfer(const char* from_file, const char* to_file)
{
	if (from_charset_.empty())
	{
		logger_error("from_charset_ empty, file_path: %s", from_file);
		return false;
	}
	if (to_charset_.empty())
	{
		logger_error("to_charset_ empty, file_path: %s", from_file);
		return false;
	}
	if (to_charset_.equal(from_charset_, false))
	{
		logger("charset is same: %s, file_path: %s",
			to_charset_.c_str(), from_file);
		return false;
	}

	acl::string buf;
	if (acl::ifstream::load(from_file, &buf) == false)
	{
		logger_error("load file %s error %s", from_file,
			acl::last_serror());
		return false;
	}
	if (buf.empty())
	{
		logger("file empty, file_path: %s", from_file);
		return false;
	}

	acl::string charset_res;
	if (check_buff(buf, to_charset_, charset_res))
		return save_to(buf, to_file);

//	printf("to_charset_: %s, charset_res: %s\r\n",
//		to_charset_.c_str(), charset_res.c_str());

	if (!from_charset_.equal("utf-8", false) &&
		!from_charset_.equal("utf8", false))
	{
		if (buf[0] == UTF8_HEADER[0]
			&& buf[1] == UTF8_HEADER[1]
			&& buf[2] == UTF8_HEADER[2])
		{
			logger_warn("skip %s, utf8 header in no utf8 file, %s",
				from_file, from_charset_.c_str());
			return save_to(buf, to_file);
		}
	}

	acl::charset_conv conv;
	acl::string res;
	if (conv.convert(from_charset_, to_charset_, buf.c_str(),
		buf.size(), &res) == false)
	{
		logger_error("charset convert error: %s, file: %s",
			conv.serror(), from_file);
		return save_to(buf, to_file);
	}

	acl::ofstream fp;
	if (fp.open_write(to_file) == false)
	{
		logger_error("open_write %s error %s", to_file,
			acl::last_serror());
		return false;
	}

	if ((to_charset_.equal("utf-8", false)
		|| to_charset_.equal("utf8", false)) && utf8_bom_)
	{
		if (fp.write(UTF8_HEADER, 3) == -1)
		{
			logger_error("write UTF8_HEADER error %s, file: %s",
				acl::last_serror(), to_file);
			return false;
		}
	}

	if (fp.write(res) == -1)
	{
		logger_error("write to %s error %s",
			to_file, acl::last_serror());
		return false;
	}

	return true;
}

bool charset_transfer::save_to(const acl::string& buf, const char* to_file)
{
	acl::ofstream fp;
	if (fp.open_write(to_file) == false)
	{
		logger_error("open_write %s error %s", to_file,
			acl::last_serror());
		return false;
	}
	if (fp.write(buf) == -1)
	{
		logger_error("write to %s error %s",
			to_file, acl::last_serror());
		return false;
	}

	return true;
}
