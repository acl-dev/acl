#include "acl_stdafx.hpp"
#include "../mime/internal/header_token.hpp"
#include "../mime/internal/mime_state.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/http/http_ctype.hpp"
#endif

namespace acl
{

http_ctype::http_ctype()
{
	ctype_ = NULL;
	stype_ = NULL;
	name_ = NULL;
	charset_ = NULL;
	bound_ = NULL;
}

http_ctype::~http_ctype()
{
	reset();
}

http_ctype& http_ctype::operator = (const http_ctype& ctype)
{
	reset();

	if (ctype.ctype_ && *ctype.ctype_)
		ctype_ = acl_mystrdup(ctype.ctype_);
	if (ctype.stype_ && *ctype.stype_)
		stype_ = acl_mystrdup(ctype.stype_);
	if (ctype.charset_ && *ctype.charset_)
		charset_ = acl_mystrdup(ctype.charset_);
	if (ctype.name_ && *ctype.name_)
		name_ = acl_mystrdup(ctype.name_);
	if (ctype.bound_ && !ctype.bound_->empty())
		bound_ = NEW string(*ctype.bound_);

	return *this;
}

void http_ctype::reset()
{
	if (ctype_)
	{
		acl_myfree(ctype_);
		ctype_ = NULL;
	}
	if (stype_)
	{
		acl_myfree(stype_);
		stype_ = NULL;
	}
	if (charset_)
	{
		acl_myfree(charset_);
		charset_ = NULL;
	}
	if (name_)
	{
		acl_myfree(name_);
		name_ = NULL;
	}
	if (bound_)
	{
		delete bound_;
		bound_ = NULL;
	}
}

#define RFC2045_TSPECIALS	"()<>@,;:\\\"/[]?="
#define EQUAL(x, y)	!strcasecmp((x), (y))
#define TOKEN_MATCH(tok, text) \
	((tok).type == HEADER_TOK_TOKEN && EQUAL((tok).u.value, (text)))

bool http_ctype::parse(const char* cp)
{
	reset();

#define PARSE_CONTENT_TYPE_HEADER(t, ptr) \
	header_token(t, MIME_MAX_TOKEN, \
		buffer, ptr, RFC2045_TSPECIALS, ';')

	HEADER_TOKEN token[MIME_MAX_TOKEN];
	ssize_t tok_count;
	ACL_VSTRING* buffer = acl_vstring_alloc(64);

	tok_count = PARSE_CONTENT_TYPE_HEADER(token, &cp);
	if (tok_count < 0)
	{
		acl_vstring_free(buffer);
		return false;
	}

	ctype_ = acl_mystrdup(token[0].u.value);

	if (TOKEN_MATCH(token[0], "multipart"))
	{
		if (tok_count >= 3 && token[1].type == '/')
			stype_ = acl_mystrdup(token[2].u.value);
		while ((tok_count = PARSE_CONTENT_TYPE_HEADER(token, &cp)) >= 0)
		{
			if (tok_count < 3 || token[1].type != '=')
				continue;
			if (TOKEN_MATCH(token[0], "boundary"))
			{
				if (bound_ == NULL)
					bound_ = NEW string(64);
				/* 需要添加 "--" 做为分隔符的前导符 */
				*bound_ = "--";
				*bound_ += token[2].u.value;
				break;
			}
		}
	}
	else if (TOKEN_MATCH(token[0], "text"))
	{
		if (tok_count >= 3 && token[1].type == '/')
			stype_ = acl_mystrdup(token[2].u.value);
		while ((tok_count = PARSE_CONTENT_TYPE_HEADER(token, &cp)) >= 0)
		{
			if (tok_count < 3 || token[1].type != '=')
				continue;
			if (TOKEN_MATCH(token[0], "charset"))
			{
				charset_ = acl_mystrdup(token[2].u.value);
				break;
			}
		}
	}
	else if (TOKEN_MATCH(token[0], "image"))
	{
		if (tok_count >= 3 && token[1].type == '/')
			stype_ = acl_mystrdup(token[2].u.value);
		while ((tok_count = PARSE_CONTENT_TYPE_HEADER(token, &cp)) >= 0)
		{
			if (tok_count < 3 || token[1].type != '=')
				continue;
			if (TOKEN_MATCH(token[0], "name"))
			{
				name_ = acl_mystrdup(token[2].u.value);
				break;
			}
		}
	}
	else if (TOKEN_MATCH(token[0], "application"))
	{
		if (tok_count >= 3 && token[1].type == '/')
			stype_ = acl_mystrdup(token[2].u.value);
		while ((tok_count = PARSE_CONTENT_TYPE_HEADER(token, &cp)) >= 0)
		{
			if (tok_count < 3 || token[1].type != '=')
				continue;
			if (TOKEN_MATCH(token[0], "name")) {
				name_ = acl_mystrdup(token[2].u.value);
				break;
			}
		}
	}
	else if (tok_count >= 3 && token[1].type == '/')
		stype_ = acl_mystrdup(token[1].u.value);

	acl_vstring_free(buffer);
	return true;
}

const char* http_ctype::get_ctype() const
{
	return ctype_;
}

const char* http_ctype::get_stype() const
{
	return stype_;
}

const char* http_ctype::get_bound() const
{
	if (bound_ == NULL || bound_->empty())
		return NULL;
	return bound_->c_str();
}

const char* http_ctype::get_name() const
{
	return name_;
}

const char* http_ctype::get_charset() const
{
	return charset_;
}

} // namespace acl
