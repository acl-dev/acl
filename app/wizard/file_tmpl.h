#pragma once

struct FILE_FROM_TO 
{
	const char* from;
	const char* to;
};

class file_tmpl
{
public:
	file_tmpl(void);
	~file_tmpl(void);

	file_tmpl& set_project_name(const char* name);
	file_tmpl& set_path_from(const char* path);
	const char* get_project_name() const
	{
		return project_name_.c_str();
	}

	void create_dirs();
	bool copy_and_replace(const char* from,
		const char* to, bool exec = false);
	bool create_common();

	bool file_copy(const char* from, const char* to);
	bool files_copy(const char* name, const FILE_FROM_TO* tab);

	tpl_t* open_tpl(const char* filename);

private:
	acl::string project_name_;
	acl::string path_from_;
	acl::string path_to_;
};

