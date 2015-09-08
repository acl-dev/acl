#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/string.hpp"

namespace acl {

class mime_code;
class ostream;

class ACL_CPP_API mail_attach
{
public:
	mail_attach(const char* filepath, const char* content_type,
		const char* charset);
	~mail_attach();

	mail_attach& set_content_id(const char* id);

	const char* get_filepath() const
	{
		return filepath_.c_str();
	}

	const char* get_filename() const
	{
		return filename_.c_str();
	}

	const char* get_content_type() const
	{
		return ctype_.c_str();
	}

	const char* get_content_id() const
	{
		return cid_.c_str();
	}

	bool save_to(mime_code* coder, string& out);
	bool save_to(mime_code* coder, ostream& out);
	void build_header(const char* transfer_encoding, string& out);

private:
	string filepath_;
	string filename_;
	string ctype_;
	string cid_;
	string charset_;
};

} // namespace acl
