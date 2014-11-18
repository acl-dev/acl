#pragma once

class global : public acl::singleton<global>
{
public:
	global();
	~global();

	const char* get_path() const
	{
		return path_.c_str();
	}

	static void get_filename(const char* filepath, acl::string& buf);
protected:
private:
	acl::string path_;
};