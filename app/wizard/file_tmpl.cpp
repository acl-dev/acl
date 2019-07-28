#include "stdafx.h"
#include "file_tmpl.h"


file_tmpl::file_tmpl(void)
{
}


file_tmpl::~file_tmpl(void)
{
}

file_tmpl& file_tmpl::set_project_name(const char* name)
{
	project_name_ = name;
	path_to_ = name;
	return *this;
}

file_tmpl& file_tmpl::set_path_from(const char* path)
{
	path_from_ = path;
	return *this;
}

tpl_t* file_tmpl::open_tpl(const char* filename)
{
	tpl_t* tpl = tpl_alloc();
	string filepath;
	filepath.format("%s/%s", path_from_.c_str(), filename);
	if (tpl_load(tpl, filepath.c_str()) != TPL_OK)
	{
		printf("load file %s error: %s\r\n",
			filepath.c_str(), last_serror());
		tpl_free(tpl);
		return NULL;
	}
	return tpl;
}

bool file_tmpl::copy_and_replace(const char* from,
	const char* to, bool exec /* = false */)
{
	tpl_t* tpl = open_tpl(from);
	if (tpl == NULL)
		return false;

	tpl_set_field_fmt_global(tpl, "PROGRAM", "%s", project_name_.c_str());

	string filepath;
	filepath << path_to_ << '/' << to;
	if (tpl_save_as(tpl, filepath.c_str()) != TPL_OK)
	{
		printf("save to %s error: %s\r\n", filepath.c_str(),
			last_serror());
		tpl_free(tpl);
		return false;
	}
	printf("create %s ok.\r\n", filepath.c_str());
	tpl_free(tpl);

	if (exec)
	{
#ifndef	WIN32
		chmod(filepath.c_str(), S_IRUSR | S_IWUSR | S_IXUSR |
			S_IRGRP | S_IWGRP | S_IXGRP);
#endif
	}
	return true;
}

void file_tmpl::create_dirs()
{
	acl_make_dirs(project_name_.c_str(), 0755);
}

bool file_tmpl::create_common()
{
	if (copy_and_replace("Makefile", "Makefile") == false)
		return false;
	if (copy_and_replace("valgrind.sh", "valgrind.sh", true) == false)
		return false;

	string file;

	// for vc2003
	file.format("%s.sln", project_name_.c_str());
	if (copy_and_replace("master_service.sln", file.c_str()) == false)
		return false;
	file.format("%s.vcproj", project_name_.c_str());
	if (copy_and_replace("master_service.vcproj", file.c_str()) == false)
		return false;

	// for vc2008
	file.format("%s_vc2008.sln", project_name_.c_str());
	if (copy_and_replace("master_service_vc2008.sln", file.c_str()) == false)
		return false;
	file.format("%s_vc2008.vcproj", project_name_.c_str());
	if (copy_and_replace("master_service_vc2008.vcproj", file.c_str()) == false)
		return false;

	// for vc2010
	file.format("%s_vc2010.sln", project_name_.c_str());
	if (copy_and_replace("master_service_vc2010.sln", file.c_str()) == false)
		return false;
	file.format("%s_vc2010.vcxproj", project_name_.c_str());
	if (copy_and_replace("master_service_vc2010.vcxproj", file.c_str()) == false)
		return false;
	file.format("%s_vc2010.vcxproj.filters", project_name_.c_str());
	if (copy_and_replace("master_service_vc2010.vcxproj.filters", file.c_str()) == false)
		return false;

	// for vc2012
	file.format("%s_vc2012.sln", project_name_.c_str());
	if (copy_and_replace("master_service_vc2012.sln", file.c_str()) == false)
		return false;
	file.format("%s_vc2012.vcxproj", project_name_.c_str());
	if (copy_and_replace("master_service_vc2012.vcxproj", file.c_str()) == false)
		return false;
	file.format("%s_vc2012.vcxproj.filters", project_name_.c_str());
	if (copy_and_replace("master_service_vc2012.vcxproj.filters", file.c_str()) == false)
		return false;

	const char* name = "common_files";
	const FILE_FROM_TO tab[] = {
		{ "stdafx.h", "stdafx.h" },
		{ "stdafx.cpp", "stdafx.cpp" },
		{ NULL, NULL }
	};

	return files_copy(name, tab);
}

bool file_tmpl::file_copy(const char* from, const char* to_in)
{
	string to_buf;
	to_buf.format("%s/%s", path_to_.c_str(), to_in);

	const char* to = to_buf.c_str();

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

bool file_tmpl::files_copy(const char* name, const FILE_FROM_TO* tab)
{
	string from, to;

	from = "tmpl/Makefile.in";
	to.format("%s/Makefile.in", path_to_.c_str());

	if (file_copy(from.c_str(), "Makefile.in") == false)
		return false;

	for (size_t i = 0; tab[i].from != NULL; i++)
	{
		from.format("%s/%s", path_from_.c_str(), tab[i].from);
		//to.format("%s/%s", path_to_.c_str(), tab[i].to);
		if (file_copy(from, tab[i].to) == false)
		{
			printf("create %s failed!\r\n", name);
			return false;
		}
	}

	printf("create %s ok!\r\n", name);
	return true;
}
