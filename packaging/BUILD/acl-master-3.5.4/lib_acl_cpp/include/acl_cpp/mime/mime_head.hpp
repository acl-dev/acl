#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"
#include <list>

#if !defined(ACL_MIME_DISABLE)

namespace acl {

typedef struct HEADER
{
	char *name;
	char *value;
} HEADER;

class ACL_CPP_API mime_head : public noncopyable
{
public:
	mime_head(void);
	~mime_head(void);

	const string& get_boundary(void) const;
	const char* get_ctype(void) const;
	const char* get_stype(void) const;
	const string& sender(void) const;
	const string& from(void) const;
	const string& replyto(void) const;
	const string& returnpath(void) const;
	const string& subject(void) const;
	const std::list<char*>& to_list(void) const;
	const std::list<char*>& cc_list(void) const;
	const std::list<char*>& bcc_list(void) const;
	const std::list<char*>& rcpt_list(void) const;
	const std::list<HEADER*>& header_list(void) const;
	const char* header_value(const char* name) const;
	int header_values(const char* name, std::list<const char*>* values) const;

	mime_head& set_sender(const char*);
	mime_head& set_from(const char*);
	mime_head& set_replyto(const char*);
	mime_head& set_returnpath(const char*);
	mime_head& set_subject(const char*);
	mime_head& add_to(const char*);
	mime_head& add_cc(const char*);
	mime_head& add_bcc(const char*);
	mime_head& add_rcpt(const char*);
	mime_head& add_header(const char*, const char*);
	mime_head& set_type(const char*, const char*);
	mime_head& set_boundary(const char*);

	void build_head(string& buf, bool clean);

	mime_head& reset(void);

protected:
private:
	string* m_boundary;
	std::list<char*>* m_rcpts;
	std::list<char*>* m_tos;
	std::list<char*>* m_ccs;
	std::list<char*>* m_bccs;
	std::list<HEADER*>* m_headers;
	string* m_sender;
	string* m_from;
	string* m_replyto;
	string* m_returnpath;
	string* m_subject;

	string m_ctype;
	string m_stype;
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
