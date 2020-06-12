#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/http/http_header.hpp"
#include "acl_cpp/http/HttpCookie.hpp"
#endif

namespace acl
{

HttpCookie::HttpCookie(const char* name, const char* value,
	dbuf_guard* dbuf /* = NULL */)
{
	if (dbuf != NULL) {
		dbuf_          = dbuf;
		dbuf_internal_ = NULL;
	} else {
		dbuf_internal_ = NEW dbuf_guard;
		dbuf_          = dbuf_internal_;
	}

	acl_assert(name && *name && value);
	name_     = dbuf_->dbuf_strdup(name);
	value_    = dbuf_->dbuf_strdup(value);
	dummy_[0] = 0;
}

HttpCookie::HttpCookie(dbuf_guard* dbuf /* = NULL */)
{
	if (dbuf != NULL) {
		dbuf_          = dbuf;
		dbuf_internal_ = NULL;
	} else {
		dbuf_internal_ = NEW dbuf_guard;
		dbuf_          = dbuf_internal_;
	}

	name_     = NULL;
	value_    = NULL;
	dummy_[0] = 0;
}

HttpCookie::HttpCookie(const HttpCookie* cookie, dbuf_guard* dbuf /* = NULL */)
{
	if (dbuf != NULL) {
		dbuf_          = dbuf;
		dbuf_internal_ = NULL;
	} else {
		dbuf_internal_ = NEW dbuf_guard;
		dbuf_          = dbuf_internal_;
	}

	dummy_[0] = 0;

	acl_assert(cookie);

	if (cookie->name_) {
		name_ = dbuf_->dbuf_strdup(cookie->name_);
	} else {
		name_ = NULL;
	}
	if (cookie->value_) {
		value_ = dbuf_->dbuf_strdup(cookie->value_);
	} else {
		value_ = NULL;
	}

	std::list<HTTP_PARAM*>::const_iterator cit = cookie->params_.begin();
	for (; cit != cookie->params_.end(); ++cit) {
		HTTP_PARAM* param = (HTTP_PARAM*)
			dbuf_->dbuf_alloc(sizeof(HTTP_PARAM));
		param->name  = dbuf_->dbuf_strdup((*cit)->name);
		param->value = dbuf_->dbuf_strdup((*cit)->value);
		params_.push_back(param);
	}
}

HttpCookie::~HttpCookie(void)
{
	delete dbuf_internal_;
}

void HttpCookie::destroy(void)
{
	delete this;
}

bool HttpCookie::splitNameValue(char* data, HTTP_PARAM* param)
{
#define SKIP_SPECIAL(x) { while (*(x) == ' ' || *(x) == '\t' || *(x) == '=') (x)++; }
#define SKIP_WHILE(cond, x) { while(*(x) && (cond)) (x)++; }

	// 开始解析过程
	param->name = data;

	// 去掉开头无用的特殊字符
	SKIP_SPECIAL(param->name);
	if (*(param->name) == 0) {
		return false;
	}

	// 找到 '='
	param->value = param->name;
	SKIP_WHILE(*(param->value) != '=', param->value);
	if (*(param->value) != '=') {
		return false;
	}

	// 去掉 '=' 前面的空格
	char* ptr       = param->value - 1;
	*param->value++ = 0;
	while (ptr > param->name && (*ptr == ' ' || *ptr == '\t')) {
		*ptr-- = 0;
	}

	// 去掉 value 开始的无效字符
	SKIP_SPECIAL(param->value);

	// 找到 value 值的结束位置
	// 允许 value = "\0"
	ptr = param->value + strlen(param->value) - 1;
	while (ptr >= param->value && (*ptr == ' ' || *ptr == '\t')) {
		*ptr-- = 0;
	}

	return true;
}

// value 格式：xxx=xxx; domain=xxx; expires=xxx; path=xxx
bool HttpCookie::setCookie(const char* value)
{
	if (value == NULL || *value == 0) {
		return false;
	}

	ACL_ARGV* tokens = acl_argv_split(value, ";");
	acl_assert(tokens->argc > 0);

	HTTP_PARAM param;

	// 从第一个 name=value 字段中取得 cookie 名及 cookie 值
	if (splitNameValue(tokens->argv[0], &param) == false) {
		acl_argv_free(tokens);
		return false;
	}
	// name 肯定非 "\0"，而 value 可以为 "\0"
	name_  = dbuf_->dbuf_strdup(param.name);
	value_ = dbuf_->dbuf_strdup(param.value);

	for (int i = 1; i < tokens->argc; i++) {
		if (splitNameValue(tokens->argv[i], &param) == false) {
			continue;
		}
		if (*(param.value) == 0) {
			continue;
		}
		if (strcasecmp(param.name, "domain") == 0) {
			setDomain(param.value);
		} else if (strcasecmp(param.name, "expires") == 0) {
			setExpires(param.value);
		} else if (strcasecmp(param.name, "path") == 0) {
			setPath(param.value);
		} else {
			add(param.name, param.value);
		}
	}

	acl_argv_free(tokens);
	return true;
}

HttpCookie& HttpCookie::setDomain(const char* domain)
{
	add("Domain", domain);
	return *this;
}

HttpCookie& HttpCookie::setPath(const char* path)
{
	add("Path", path);
	return *this;
}

HttpCookie& HttpCookie::setExpires(time_t timeout)
{
	if (timeout > 0) {
		time_t n = time(NULL);
		n += timeout;

		char buf[64];
		http_header::date_format(buf, sizeof(buf), n);
		add("Expires", buf);
	}
	return *this;
}

HttpCookie& HttpCookie::setExpires(const char* expires)
{
	if (expires && *expires) {
		add("Expires", expires);
	}
	return *this;
}

HttpCookie& HttpCookie::setMaxAge(int max_age)
{
	char tmp[20];
	safe_snprintf(tmp, sizeof(tmp), "%d", max_age);
	add("Max-Age", tmp);
	return *this;
}

HttpCookie& HttpCookie::add(const char* name, const char* value)
{
	if (name == NULL || *name == 0 || value == NULL) {
		return *this;
	}

	HTTP_PARAM* param = (HTTP_PARAM*) dbuf_->dbuf_alloc(sizeof(HTTP_PARAM));
	param->name  = dbuf_->dbuf_strdup(name);
	param->value = dbuf_->dbuf_strdup(value);
	params_.push_back(param);
	return *this;
}

const char* HttpCookie::getName(void) const
{
	if (name_ == NULL) {
		return dummy_;
	}
	return name_;
}

const char* HttpCookie::getValue(void) const
{
	if (value_ == NULL) {
		return dummy_;
	}
	return value_;
}

const char* HttpCookie::getExpires(void) const
{
	return getParam("Expires");
}

const char* HttpCookie::getDomain(void) const
{
	return getParam("Domain");
}

const char* HttpCookie::getPath(void) const
{
	return getParam("Path");
}

int HttpCookie::getMaxAge(void) const
{
	const char* ptr = getParam("Max-Age");
	if (ptr == NULL || *ptr == 0) {
		return -1;
	}
	return atoi(ptr);
}

const char* HttpCookie::getParam(const char* name,
	bool case_insensitive /* = true */) const
{
	std::list<HTTP_PARAM*>::const_iterator cit = params_.begin();
	for (; cit != params_.end(); ++cit) {
		if (case_insensitive) {
			if (strcasecmp((*cit)->name, name) == 0) {
				return (*cit)->value;
			}
		} else if (strcasecmp((*cit)->name, name) == 0) {
			return (*cit)->value;
		}
	}
	return dummy_;
}

const std::list<HTTP_PARAM*>& HttpCookie::getParams(void) const
{
	return params_;
}

} // namespace acl end
