#include "acl_stdafx.hpp"
#include "../mime/internal/header_token.hpp"
#include "../mime/internal/mime_state.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/http/http_ctype.hpp"
#endif

#if !defined(ACL_MIME_DISABLE)

namespace acl {

http_ctype::http_ctype() {}

http_ctype::~http_ctype() {}

http_ctype& http_ctype::operator = (const http_ctype& ctype) {
	reset();

	ctype_   = ctype.ctype_;
	stype_   = ctype.stype_;
	charset_ = ctype.charset_;
	name_    = ctype.name_;
	bound_   = ctype.bound_;

	return *this;
}

void http_ctype::reset() {
	ctype_.clear();
	stype_.clear();
	charset_.clear();
	name_.clear();
	bound_.clear();
}

#define RFC2045_TSPECIALS	"()<>@,;:\\\"/[]?="
#define EQUAL(x, y)	!strcasecmp((x), (y))
#define TOKEN_MATCH(tok, text) \
	((tok).type == HEADER_TOK_TOKEN && EQUAL((tok).u.value, (text)))

#define PARSE_CONTENT_TYPE(t, ptr) \
	header_token(t, MIME_MAX_TOKEN, buffer, ptr, RFC2045_TSPECIALS, ';')

static void parse_token(const char* cp, ACL_VSTRING* buffer, HEADER_TOKEN* token,
	  ssize_t tok_count, const char* tag, std::string& stype, std::string& value) {
	if (tok_count >= 3 && token[1].type == '/') {
		stype = token[2].u.value;
	}
	while ((tok_count = PARSE_CONTENT_TYPE(token, &cp)) >= 0) {
		if (tok_count < 3 || token[1].type != '=') {
			continue;
		}
		if (TOKEN_MATCH(token[0], tag)) {
			value = token[2].u.value;
			break;
		}
	}
}

bool http_ctype::parse(const char* cp) {
	reset();

	HEADER_TOKEN token[MIME_MAX_TOKEN];
	ACL_VSTRING* buffer = acl_vstring_alloc(64);
	ssize_t tok_count = PARSE_CONTENT_TYPE(token, &cp);
	if (tok_count < 0) {
		acl_vstring_free(buffer);
		return false;
	}

	ctype_ = token[0].u.value;

	if (TOKEN_MATCH(token[0], "multipart")) {
		parse_token(cp, buffer, token, tok_count, "boundary", stype_, bound_);
	} else if (TOKEN_MATCH(token[0], "text")) {
		parse_token(cp, buffer, token, tok_count, "charset", stype_, charset_);
	} else if (TOKEN_MATCH(token[0], "image")
		 || TOKEN_MATCH(token[0], "application")) {
		parse_token(cp, buffer, token, tok_count, "name", stype_, name_);
	} else if (tok_count >= 3 && token[1].type == '/') {
		stype_ = token[2].u.value;
	}

	acl_vstring_free(buffer);
	return true;
}

const char* http_ctype::get_ctype() const {
	return ctype_.empty() ? NULL : ctype_.c_str();
}

const char* http_ctype::get_stype() const {
	return stype_.empty() ? NULL : stype_.c_str();
}

const char* http_ctype::get_bound() const {
	return bound_.empty() ? NULL : bound_.c_str();
}

const char* http_ctype::get_name() const {
	return name_.empty() ? NULL : name_.c_str();
}

const char* http_ctype::get_charset() const {
	return charset_.empty() ? NULL : charset_.c_str();
}

http_ctype& http_ctype::set_ctype(const char* ctype) {
	if (ctype == NULL) {
		return *this;
	}
	ctype_ = ctype;
	return *this;
}

http_ctype& http_ctype::set_stype(const char* stype) {
	if (stype == NULL) {
		return *this;
	}
	stype_ = stype;
	return *this;
}

http_ctype& http_ctype::set_bound(const char* boundary) {
	if (boundary == NULL) {
		return *this;
	}
	bound_ = boundary;
	return *this;
}

http_ctype& http_ctype::set_charset(const char* charset) {
	if (charset == NULL) {
		return *this;
	}
	charset_ = charset;
	return *this;
}

http_ctype& http_ctype::set_name(const char* name) {
	if (name == NULL) {
		return *this;
	}
	name_ = name;
	return *this;
}

bool http_ctype::to_string(std::string& buf) const {
	if (ctype_.empty() || stype_.empty()) {
		return false;
	}

	buf = ctype_;
	buf += "/";
	buf += stype_;
	if (!charset_.empty()) {
		buf += "; charset=\"%s\"";
		buf += charset_;
	}
	if (!name_.empty()) {
		buf += "; name=%s";
		buf += name_;
	}
	if (!bound_.empty()) {
		buf += "; boundary=%s";
		buf += bound_;
	}
	return true;
}

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
