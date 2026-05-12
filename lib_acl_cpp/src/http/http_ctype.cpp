#include "acl_stdafx.hpp"
#include "../mime/internal/header_token.hpp"
#include "../mime/internal/mime_state.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/http/http_ctype.hpp"
#endif

#if !defined(ACL_MIME_DISABLE)

namespace acl {

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

	if (ctype.ctype_ && *ctype.ctype_) {
		ctype_ = acl_mystrdup(ctype.ctype_);
	}
	if (ctype.stype_ && *ctype.stype_) {
		stype_ = acl_mystrdup(ctype.stype_);
	}
	if (ctype.charset_ && *ctype.charset_) {
		charset_ = acl_mystrdup(ctype.charset_);
	}
	if (ctype.name_ && *ctype.name_) {
		name_ = acl_mystrdup(ctype.name_);
	}
	if (ctype.bound_ && !ctype.bound_->empty()) {
		bound_ = NEW string(*ctype.bound_);
	}

	return *this;
}

void http_ctype::reset()
{
	if (ctype_) {
		acl_myfree(ctype_);
		ctype_ = NULL;
	}
	if (stype_) {
		acl_myfree(stype_);
		stype_ = NULL;
	}
	if (charset_) {
		acl_myfree(charset_);
		charset_ = NULL;
	}
	if (name_) {
		acl_myfree(name_);
		name_ = NULL;
	}
	if (bound_) {
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

#define PARSE_CONTENT_TYPE(t, ptr) \
	header_token(t, MIME_MAX_TOKEN, \
		buffer, ptr, RFC2045_TSPECIALS, ';')

	HEADER_TOKEN token[MIME_MAX_TOKEN];
	ssize_t tok_count;
	ACL_VSTRING* buffer = acl_vstring_alloc(64);

	tok_count = PARSE_CONTENT_TYPE(token, &cp);
	if (tok_count < 0) {
		acl_vstring_free(buffer);
		return false;
	}

	ctype_ = acl_mystrdup(token[0].u.value);

	if (TOKEN_MATCH(token[0], "multipart")) {
		if (tok_count >= 3 && token[1].type == '/')
			stype_ = acl_mystrdup(token[2].u.value);
		while ((tok_count = PARSE_CONTENT_TYPE(token, &cp)) >= 0) {
			if (tok_count < 3 || token[1].type != '=') {
				continue;
			}
			if (TOKEN_MATCH(token[0], "boundary")) {
				if (bound_ == NULL) {
					bound_ = NEW string(64);
				}
				*bound_ = token[2].u.value;
				break;
			}
		}
	} else if (TOKEN_MATCH(token[0], "text")) {
		if (tok_count >= 3 && token[1].type == '/') {
			stype_ = acl_mystrdup(token[2].u.value);
		}

		while ((tok_count = PARSE_CONTENT_TYPE(token, &cp)) >= 0) {
			if (tok_count < 3 || token[1].type != '=') {
				continue;
			}
			if (TOKEN_MATCH(token[0], "charset")) {
				charset_ = acl_mystrdup(token[2].u.value);
				break;
			}
		}
	} else if (TOKEN_MATCH(token[0], "image")) {
		if (tok_count >= 3 && token[1].type == '/') {
			stype_ = acl_mystrdup(token[2].u.value);
		}

		while ((tok_count = PARSE_CONTENT_TYPE(token, &cp)) >= 0) {
			if (tok_count < 3 || token[1].type != '=') {
				continue;
			}
			if (TOKEN_MATCH(token[0], "name")) {
				name_ = acl_mystrdup(token[2].u.value);
				break;
			}
		}
	} else if (TOKEN_MATCH(token[0], "application")) {
		if (tok_count >= 3 && token[1].type == '/') {
			stype_ = acl_mystrdup(token[2].u.value);
		}

		while ((tok_count = PARSE_CONTENT_TYPE(token, &cp)) >= 0) {
			if (tok_count < 3 || token[1].type != '=') {
				continue;
			}

			if (TOKEN_MATCH(token[0], "name")) {
				name_ = acl_mystrdup(token[2].u.value);
				break;
			}
		}
	} else if (tok_count >= 3 && token[1].type == '/') {
		stype_ = acl_mystrdup(token[2].u.value);
	}

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
	if (bound_ == NULL || bound_->empty()) {
		return NULL;
	}
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

http_ctype& http_ctype::set_ctype(const char* ctype) {
	if (ctype == NULL || *ctype == 0) {
		return *this;
	}
	if (ctype_) {
		acl_myfree(ctype_);
	}
	ctype_ = acl_mystrdup(ctype);
	return *this;
}

http_ctype& http_ctype::set_stype(const char* stype) {
	if (stype == NULL || *stype == 0) {
		return *this;
	}
	if (stype_) {
		acl_myfree(stype_);
	}
	stype_ = acl_mystrdup(stype);
	return *this;
}

http_ctype& http_ctype::set_bound(const char* boundary) {
	if (boundary == NULL || *boundary == 0) {
		return *this;
	}
	if (bound_ == NULL) {
		bound_ = NEW string(64);
	}
	bound_->copy(boundary);
	return *this;
}

http_ctype& http_ctype::set_charset(const char* charset) {
	if (charset == NULL || *charset == 0) {
		return *this;
	}
	if (charset_) {
		acl_myfree(charset_);
	}
	charset_ = acl_mystrdup(charset);
	return *this;
}

http_ctype& http_ctype::set_name(const char* name) {
	if (name == NULL || *name == 0) {
		return *this;
	}
	if (name_) {
		acl_myfree(name_);
	}
	name_ = acl_mystrdup(name);
	return *this;
}

bool http_ctype::to_string(string& buf) const {
	if (ctype_ == NULL || *ctype_ == 0) {
		return false;
	}
	if (stype_ == NULL || *stype_ == 0) {
		return false;
	}
	buf.format("%s/%s", ctype_, stype_);
	if (charset_ && *charset_) {
		buf.format_append("; charset=%s", charset_);
	}
	if (name_ && *name_) {
		buf.format_append("; name=%s", name_);
	}
	if (bound_ && bound_->size() > 2) {
		buf.format_append("; boundary=%s", bound_->c_str());
	}
	return true;
}

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
