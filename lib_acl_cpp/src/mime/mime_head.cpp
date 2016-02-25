#include "acl_stdafx.hpp"
#include "internal/mime_state.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/mime/rfc822.hpp"
#include "acl_cpp/mime/mime_head.hpp"
#endif

namespace acl {

const static acl::string  __dummy;
const static std::list<char*> __dummyList;
const static std::list<HEADER*> __dummyHeaderList;

mime_head::mime_head()
	: m_ctype(16)
	, m_stype(16)
{
	m_rcpts = NULL;
	m_tos = NULL;
	m_ccs = NULL;
	m_bccs = NULL;
	m_headers = NULL;
	m_sender = NULL;
	m_from = NULL;
	m_replyto = NULL;
	m_returnpath = NULL;
	m_subject = NULL;
	m_boundary = NULL;
}

mime_head::~mime_head()
{
	reset();
}

mime_head& mime_head::reset()
{
	delete m_sender;
	m_sender = NULL;
	delete m_from;
	m_from = NULL;
	delete m_replyto;
	m_replyto = NULL;
	delete m_returnpath;
	m_returnpath = NULL;
	delete m_subject;
	m_subject = NULL;

	std::list<char*>::iterator it;

	if (m_rcpts) {
		for (it = m_rcpts->begin(); it != m_rcpts->end(); ++it) {
			acl_myfree(*it);
		}
		delete m_rcpts;
		m_rcpts = NULL;
	}

	if (m_tos) {
		for (it = m_tos->begin(); it != m_tos->end(); ++it) {
			acl_myfree(*it);
		}
		delete m_tos;
		m_tos = NULL;
	}

	if (m_ccs) {
		for (it = m_ccs->begin(); it != m_ccs->end(); ++it) {
			acl_myfree(*it);
		}
		delete m_ccs;
		m_ccs = NULL;
	}

	if (m_bccs) {
		for (it = m_bccs->begin(); it != m_bccs->end(); ++it) {
			acl_myfree(*it);
		}
		delete m_bccs;
		m_bccs = NULL;
	}

	if (m_headers) {
		std::list<HEADER*>::iterator it1 = m_headers->begin();
		for (; it1 != m_headers->end(); ++it1) {
			acl_myfree((*it1)->name);
			acl_myfree((*it1)->value);
			acl_myfree((*it1));
		}
		delete m_headers;
		m_headers = NULL;
	}

	if (m_boundary) {
		delete m_boundary;
		m_boundary = NULL;
	}

	return (*this);
}

const char* mime_head::get_ctype() const
{
	return m_ctype.c_str();
}

const char* mime_head::get_stype() const
{
	return m_stype.c_str();
}

const acl::string& mime_head::sender() const
{
	return (m_sender ? *m_sender : __dummy);
}

const acl::string& mime_head::from() const
{
	return (m_from ? *m_from : __dummy);
}

const acl::string& mime_head::replyto() const
{
	return (m_replyto ? *m_replyto : __dummy);
}

const acl::string& mime_head::returnpath() const
{
	return (m_returnpath ? *m_returnpath : __dummy);
}

const acl::string& mime_head::subject() const
{
	return (m_subject ? *m_subject : __dummy);
}

const std::list<char*>& mime_head::to_list() const
{
	return (m_tos ? (*m_tos) : __dummyList);
}

const std::list<char*>& mime_head::cc_list() const
{
	return (m_ccs ? (*m_ccs) : __dummyList);
}

const std::list<char*>& mime_head::bcc_list() const
{
	return (m_bccs ? (*m_bccs) : __dummyList);
}

const std::list<char*>& mime_head::rcpt_list() const
{
	return (m_rcpts ? (*m_rcpts) : __dummyList);
}

const std::list<HEADER*>& mime_head::header_list() const
{
	if (m_headers)
		return (*m_headers);
	return (__dummyHeaderList);
}

const char* mime_head::header_value(const char* name) const
{
	if (m_headers == NULL)
		return (NULL);
	std::list<HEADER*>::const_iterator cit = m_headers->begin();
	for (; cit != m_headers->end(); ++cit)
	{
		if (strcasecmp((*cit)->name, name) == 0)
			return ((*cit)->value);
	}

	return (NULL);
}

int mime_head::header_values(const char* name, std::list<const char*>* values) const
{
	if (m_headers == NULL)
		return (0);

	int   n = 0;
	std::list<HEADER*>::const_iterator cit = m_headers->begin();
	for (; cit != m_headers->end(); ++cit)
	{
		if (strcasecmp((*cit)->name, name) == 0)
		{
			values->push_back((*cit)->value);
			n++;
		}
	}
	return (n);
}

mime_head& mime_head::set_sender(const char* addr)
{
	if (m_sender)
		*m_sender = addr;
	else
		m_sender = NEW acl::string(addr);
	return (*this);
}

mime_head& mime_head::set_from(const char* addr)
{
	if (m_from)
		*m_from = addr;
	else
		m_from = NEW acl::string(addr);
	return (*this);
}

mime_head& mime_head::set_replyto(const char* addr)
{
	if (m_replyto)
		*m_replyto = addr;
	else
		m_replyto = NEW acl::string(addr);
	return (*this);
}

mime_head& mime_head::set_returnpath(const char* addr)
{
	if (m_returnpath)
		*m_returnpath = addr;
	else
		m_returnpath = NEW acl::string(addr);
	return (*this);
}

mime_head& mime_head::set_subject(const char* s)
{
	if (m_subject)
		*m_subject = s;
	else
		m_subject = NEW acl::string(s);
	return (*this);
}

mime_head& mime_head::add_to(const char* addr)
{
	if (m_tos == NULL)
		m_tos = NEW std::list<char*>;
	m_tos->push_back(acl_mystrdup(addr));
	add_rcpt(addr);
	return (*this);
}

mime_head& mime_head::add_cc(const char* addr)
{
	if (m_ccs == NULL)
		m_ccs = NEW std::list<char*>;
	m_ccs->push_back(acl_mystrdup(addr));
	add_rcpt(addr);
	return (*this);
}

mime_head& mime_head::add_bcc(const char* addr)
{
	if (m_bccs == NULL)
		m_bccs = NEW std::list<char*>;
	m_bccs->push_back(acl_mystrdup(addr));
	add_rcpt(addr);
	return (*this);
}

mime_head& mime_head::add_rcpt(const char* addr)
{
	if (m_rcpts == NULL)
		m_rcpts = NEW std::list<char*>;
	m_rcpts->push_back(acl_mystrdup(addr));
	return (*this);
}

mime_head& mime_head::add_header(const char* name, const char* value)
{
	if (m_headers == NULL)
		m_headers = NEW std::list<HEADER*>;
	HEADER *header = (HEADER*) acl_mycalloc(1, sizeof(HEADER));
	header->name = acl_mystrdup(name);
	header->value = acl_mystrdup(value);
	m_headers->push_back(header);
	return (*this);
}

mime_head& mime_head::set_type(const char* ctype, const char* stype)
{
	if (ctype && *ctype)
		m_ctype = ctype;
	if (stype && *stype)
		m_stype = stype;
	return (*this);
}

mime_head& mime_head::set_boundary(const char* s)
{
	if (m_boundary == NULL)
		m_boundary = NEW acl::string(s);
	else
		*m_boundary = s;
	return (*this);
}

const acl::string& mime_head::get_boundary() const
{
	if (m_boundary)
		return (*m_boundary);
	else
		return (__dummy);
}

static void append_recipients(string& out, const char* tagname,
	const std::list<char*>& recipients)
{
	if (recipients.empty())
		return;
	std::list<char*>::const_iterator cit = recipients.begin();
	out.format_append("%s: %s", tagname, *cit);
	++cit;
	if (cit == recipients.end())
	{
		out.append("\r\n");
		return;
	}
	for (; cit != recipients.end(); ++cit)
		out.format_append(",\r\n %s", *cit);
	out.append("\r\n");
}

void mime_head::build_head(string& out, bool clean)
{
	if (clean)
		out.clear();
	if (m_headers)
	{
		std::list<HEADER*>::const_iterator cit = m_headers->begin();
		for (; cit != m_headers->end(); ++cit)
		{
			out.format_append("%s: %s\r\n",
				(*cit)->name, (*cit)->value);
		}
	}
	char buf[64];
	rfc822 rfc;
	rfc.mkdate_cst(time(NULL), buf, sizeof(buf));
	out.format_append("Date: %s\r\n", buf);

	if (m_from)
		out.format_append("From: %s\r\n", m_from->c_str());
	if (m_replyto)
		out.format_append("Reply-To: %s\r\n", m_replyto->c_str());
	if (m_returnpath)
		out.format_append("Return-Path: %s\r\n", m_returnpath->c_str());
	if (m_tos)
		append_recipients(out, "To", *m_tos);
	if (m_ccs)
		append_recipients(out, "Cc", *m_ccs);
	if (m_bccs)
		append_recipients(out, "Bcc", *m_bccs);
	if (m_subject)
		out.format_append("Subject: %s\r\n", m_subject->c_str());
	out.append("MIME-Version: 1.0\r\n");
	out.format_append("Content-Type: %s/%s", get_ctype(), get_stype());
	if (m_boundary)
		out.format_append(";\r\n\tboundary=\"%s\"\r\n",
			m_boundary->c_str());
	else
		out.append("\r\n");

	out.append("\r\n");
}

} // namespace acl
