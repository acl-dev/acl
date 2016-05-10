#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stdlib/url_coder.hpp"
#endif

namespace acl
{

void url_coder::init_dbuf(dbuf_guard* dbuf)
{
	if (dbuf != NULL)
	{
		dbuf_ = dbuf;
		dbuf_internal_ = NULL;
	}
	else
	{
		dbuf_internal_ = new dbuf_guard;
		dbuf_ = dbuf_internal_;
	}
}

url_coder::url_coder(bool nocase /* = true */, dbuf_guard* dbuf /* = NULL */)
	: dbuf_obj(dbuf)
	, nocase_(nocase)
{
	init_dbuf(dbuf);

	buf_ = NEW string(128);
}

url_coder::url_coder(const url_coder& coder, dbuf_guard* dbuf /* = NULL */)
	: dbuf_obj(dbuf)
{
	init_dbuf(dbuf);

	buf_ = NEW string(coder.buf_->c_str());
	nocase_ = coder.nocase_;

	std::vector<URL_NV*>::const_iterator cit = coder.params_.begin();
	for (; cit != coder.params_.end(); ++cit)
		set((*cit)->name, (*cit)->value);
}

url_coder::~url_coder()
{
	reset();

	delete buf_;
	delete dbuf_internal_;
}

const url_coder& url_coder::operator =(const url_coder& coder)
{
	nocase_ = coder.nocase_;
	*buf_ = coder.buf_->c_str();

	std::vector<URL_NV*>::const_iterator cit = coder.params_.begin();
	for (; cit != coder.params_.end(); ++cit)
		set((*cit)->name, (*cit)->value);

	return *this;
}

void url_coder::reset()
{
	params_.clear();
	buf_->clear();
	dbuf_->dbuf_reset();
}

void url_coder::encode(string& buf, bool clean /* = true */) const
{
	if (clean)
		buf.clear();

	ACL_DBUF_POOL *dbuf = dbuf_->get_dbuf().get_dbuf();
	std::vector<URL_NV*>::const_iterator cit = params_.begin();
	char* name, *value;

	for (; cit != params_.end(); ++cit)
	{
		if (cit != params_.begin())
			buf << '&';
		name = acl_url_encode((*cit)->name, dbuf);
		value = acl_url_encode((*cit)->value, dbuf);
		buf << name << '=' << value;
		dbuf_->dbuf_free(name);
		dbuf_->dbuf_free(value);
	}
}

const string& url_coder::to_string() const
{
	encode(*buf_);
	return *buf_;
}

void url_coder::decode(const char* str)
{
	ACL_DBUF_POOL *dbuf = dbuf_->get_dbuf().get_dbuf();
	ACL_ARGV* tokens = acl_argv_split3(str, "&", dbuf);
	ACL_ITER iter;

	acl_foreach(iter, tokens)
	{
		char* name = (char*) iter.data;
		char* value = strchr(name, '=');
		if (value == NULL || *(value + 1) == 0)
			continue;
		*value++ = 0;
		name = acl_url_decode(name, dbuf);
		value = acl_url_decode(value, dbuf);
		URL_NV* param = (URL_NV*) dbuf_->dbuf_alloc(sizeof(URL_NV));
		param->name = name;
		param->value = value;
		params_.push_back(param);
	}
}

url_coder& url_coder::set(const char* name, const char* value,
	bool override /* = true */)
{
	if (name == NULL || *name == 0 || value == NULL || *value == 0)
	{
		//logger_error("invalid input: name: [%s], value: [%s]",
		//	name ? name : "null", value ? value : "null");
		return *this;
	}

	if (override)
	{
		int (*cmp)(const char*, const char*) = nocase_
			? strcasecmp : strcmp;

		std::vector<URL_NV*>::iterator it = params_.begin();
		for (; it != params_.end(); ++it)
		{
			if (cmp((*it)->name, name) == 0)
			{
				params_.erase(it);
				break;
			}
		}
	}

	URL_NV* param = (URL_NV*) dbuf_->dbuf_alloc(sizeof(URL_NV));
	param->name = dbuf_->dbuf_strdup(name);
	param->value = dbuf_->dbuf_strdup(value);
	params_.push_back(param);
	return *this;
}

url_coder& url_coder::set(const char* name, int value,
	bool override /* = true */)
{
	char buf[24];

#ifdef ACL_WINDOWS
# if _MSC_VER >= 1500
	_snprintf_s(buf, sizeof(buf), sizeof(buf), "%d", value);
# else
	_snprintf(buf, sizeof(buf), "%d", value);
# endif
#else
	snprintf(buf, sizeof(buf), "%d", value);
#endif
	return set(name, buf, override);
}

url_coder& url_coder::set(const char* name, bool override, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	set(name, fmt, ap, override);
	va_end(ap);
	return *this;
}

url_coder& url_coder::set(const char* name, const char* fmt, va_list ap,
	bool override /* = true */)
{
	string buf;
	buf.vformat(fmt, ap);
	return set(name, buf.c_str(), override);
}

const char* url_coder::get(const char* name) const
{
	int (*cmp)(const char*, const char*) = nocase_ ? strcasecmp : strcmp;
	std::vector<URL_NV*>::const_iterator cit = params_.begin();

	for (; cit != params_.end(); ++cit)
	{
		if (cmp((*cit)->name, name) == 0)
			return (*cit)->value;
	}

	return NULL;
}

const char* url_coder::operator [](const char* name) const
{
	return get(name);
}

bool url_coder::del(const char* name)
{
	int (*cmp)(const char*, const char*) = nocase_ ? strcasecmp : strcmp;
	std::vector<URL_NV*>::iterator it = params_.begin();

	for (; it != params_.end(); ++it)
	{
		if (cmp((*it)->name, name) == 0)
		{
			params_.erase(it);
			return true;
		}
	}

	return false;
}

} // namespace acl end
