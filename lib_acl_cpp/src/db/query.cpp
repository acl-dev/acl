#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include <assert.h>
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/db/query.hpp"
#endif

namespace acl
{

query::query()
: sql_buf_(NULL)
{
}

query::~query()
{
	delete sql_buf_;
	reset();
}

query& query::create_sql(const char* sql_fmt, ...)
{
	va_list ap;
	va_start(ap, sql_fmt);
	sql_.vformat(sql_fmt, ap);
	va_end(ap);

	return *this;
}

query& query::create(const char* sql)
{
	sql_ = sql;
	return *this;
}

bool query::append_key(string& buf, char* key)
{
	acl_lowercase(key);
	std::map<string, query_param*>::iterator it = params_.find(key);
	if (it == params_.end())
	{
		logger_warn("unknown key: %s", key);
		buf.append(key);
		return false;
	}

	char fmt[256];

	query_param* param = it->second;
	switch (param->type)
	{
	case DB_PARAM_CHAR:
		buf.format_append("'%c'", param->v.c);
		break;
	case DB_PARAM_SHORT:
		buf.format_append("%d", param->v.s);
		break;
	case DB_PARAM_INT32:
		buf.format_append("%d", param->v.n);
		break;
	case DB_PARAM_INT64:
		buf.format_append("%lld", param->v.l);
		break;
	case DB_PARAM_STR:
		buf.format_append("'%s'",
			escape(param->v.S, param->dlen, buf_).c_str());
		break;
	case DB_PARAM_FLOAT:
		safe_snprintf(fmt, sizeof(fmt), "%%.%df", param->precision);
		buf.format_append(fmt, param->v.f);
		break;
	case DB_PARAM_DOUBLE:
		safe_snprintf(fmt, sizeof(fmt), "%%.%df", param->precision);
		buf.format_append(fmt, param->v.d);
		break;
	default:
		logger_error("unknown type: %d", param->type);
		break;
	}

	return true;
}

const string& query::to_string()
{
	if (params_.empty())
		return sql_;
	if (sql_buf_ == NULL)
		sql_buf_ = NEW string(sql_.length() + 32);
	else
		sql_buf_->clear();

#define SKIP_WHILE(cond, ptr) { while(*ptr && (cond)) ptr++; }

	char last_ch;
	char* src = sql_.c_str(), *ptr, *key;
	while (*src != 0)
	{
		ptr = strchr(src, ':');
		if (ptr == NULL)
		{
			sql_buf_->append(src);
			break;
		}
		else if (*++ptr == 0)
		{
			sql_buf_->append(src);
			logger_warn("the last char is ':'");
			break;
		}

		sql_buf_->append(src, ptr - src - 1);
		key = ptr;

		SKIP_WHILE(*ptr != ',' && *ptr != ';'
			&& *ptr != ' ' && *ptr != '\t'
			&& *ptr != '(' && *ptr != ')'
			&& *ptr != '\r' && *ptr != '\n', ptr);
		if (ptr - key == 1)
		{
			logger_warn("only found: ':%c'", *ptr);
			sql_buf_->append(key, ptr - key + 1);
			src = ptr + 2;
			continue;
		}

		last_ch = *ptr;
		*ptr = 0;
		(void) append_key(*sql_buf_, key);
		*ptr = last_ch;

		if (last_ch == '\0')
			break;
		src = ptr;
	}

	return *sql_buf_;
}

void query::del_param(const string& key)
{
	std::map<string, query_param*>::iterator it = params_.find(key);
	if (it != params_.end())
	{
		acl_myfree(it->second);
		params_.erase(it);
	}
}

query& query::set_parameter(const char* name, const char *value)
{
	string key(name);
	key.lower();
	del_param(key);

	size_t len = strlen(value);
	query_param* param = (query_param*)
		acl_mymalloc(sizeof(query_param) + len + 1);
	param->type = DB_PARAM_STR;
	memcpy(param->v.S, value, len);
	param->v.S[len] = 0;
	param->dlen = (int) len;

	params_[key] = param;
	return *this;
}

query& query::set_parameter(const char* name, char value)
{
	string key(name);
	key.lower();
	del_param(key);

	query_param* param = (query_param*) acl_mymalloc(sizeof(query_param));
	param->type = DB_PARAM_CHAR;
	param->v.c = value;
	param->dlen = sizeof(char);

	params_[key] = param;
	return *this;
}

query& query::set_parameter(const char* name, short value)
{
	string key(name);
	key.lower();
	del_param(key);

	query_param* param = (query_param*) acl_mymalloc(sizeof(query_param));
	param->type = DB_PARAM_SHORT;
	param->v.s = value;
	param->dlen = sizeof(short);

	params_[key] = param;
	return *this;
}

query& query::set_parameter(const char* name, int value)
{
	string key(name);
	key.lower();
	del_param(key);

	query_param* param = (query_param*) acl_mymalloc(sizeof(query_param));
	param->type = DB_PARAM_INT32;
	param->v.n = value;
	param->dlen = sizeof(int);

	params_[key] = param;
	return *this;
}

query& query::set_parameter(const char* name, acl_int64 value)
{
	string key(name);
	key.lower();
	del_param(key);

	query_param* param = (query_param*) acl_mymalloc(sizeof(query_param));
	param->type = DB_PARAM_INT64;
	param->v.l = value;
	param->dlen = sizeof(long long int);

	params_[key] = param;
	return *this;
}

query& query::set_parameter(const char* name, float value, int precision /* = 8 */)
{
	string key(name);
	key.lower();
	del_param(key);

	query_param* param = (query_param*) acl_mymalloc(sizeof(query_param));
	param->type = DB_PARAM_FLOAT;
	param->v.f = value;
	param->dlen = sizeof(float);
	if (precision >= 0)
		param->precision = precision;
	else
		param->precision = 8;

	params_[key] = param;
	return *this;
}

query& query::set_parameter(const char* name, double value, int precision /* = 8 */)
{
	string key(name);
	key.lower();
	del_param(key);

	query_param* param = (query_param*) acl_mymalloc(sizeof(query_param));
	param->type = DB_PARAM_DOUBLE;
	param->v.d = value;
	param->dlen = sizeof(double);
	if (precision >= 0)
		param->precision = precision;
	else
		param->precision = 8;

	params_[key] = param;
	return *this;
}

query& query::set_date(const char* name, time_t value,
	const char* fmt /* = "%Y-%m-%d %H:%M:%S" */)
{
	string key(name);
	key.lower();
	del_param(key);

	string buf(128);
	if (to_date(value, buf, fmt) == NULL)
	{
		logger_error("to_date_time failed, time: %ld", (long) value);
		return *this;
	}

	size_t len = buf.length();
	query_param* param = (query_param*)
		acl_mymalloc(sizeof(query_param) + len + 1);
	param->type = DB_PARAM_STR;
	memcpy(param->v.S, buf.c_str(), len);
	param->v.S[len] = 0;
	param->dlen = (int) len;

	params_[key] = param;
	return *this;
}

query& query::set_format(const char* name, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	(void) set_vformat(name, fmt, ap);
	va_end(ap);

	return *this;
}

query& query::set_vformat(const char* name, const char* fmt, va_list ap)
{
	string key(name);
	key.lower();
	del_param(key);

	string val;
	val.vformat(fmt, ap);
	size_t len = val.length();
	query_param* param = (query_param*)
		acl_mymalloc(sizeof(query_param) + len + 1);
	param->type = DB_PARAM_STR;
	memcpy(param->v.S, val.c_str(), len);
	param->v.S[len] = 0;
	param->dlen = (int) len;

	params_[key] = param;
	return *this;
}

void query::reset()
{
	std::map<string, query_param*>::iterator it = params_.begin();
	for (; it != params_.end(); ++it)
		acl_myfree(it->second);
	params_.clear();
}

const string& query::escape(const char* in, size_t len, string& out)
{
	out.clear();

	for (size_t i = 0; i < len; i++, in++)
	{
		switch (*in) {
		case 0:			/* Must be escaped for 'mysql' */
			out += '\\';
			out += '0';
			break;
		case '\n':		/* Must be escaped for logs */
			out += '\\';
			out += 'n';
			break;
		case '\r':
			out += '\\';
			out += 'r';
			break;
		case '\\':
			out += '\\';
			out += '\\';
			break;
		case '\'':
			out += '\\';
			out += '\'';
			break;
		case '"':		/* Better safe than sorry */
			out += '\\';
			out += '"';
			break;
		case '\032':		/* This gives problems on Win32 */
			out += '\\';
			out += 'Z';
			break;
		default:
			out += *in;
			break;
		}
	}

	return out;
}

const char* query::to_date(time_t t, string& out,
	const char* fmt /* = "%Y-%m-%d %H:%M:%S" */)
{
	char buf[256];

	if (fmt == NULL || *fmt == 0)
		fmt = "%Y-%m-%d %H:%M:%S";
	
	
	struct tm* local_ptr;

#ifdef ACL_WINDOWS
# ifdef __STDC_WANT_SECURE_LIB__

	struct tm local;

	if (localtime_s(&local, &t) != 0)
	{
		logger_error("localtime_s failed, t: %ld", (long) t);
		return NULL;
	}
	local_ptr = &local;
# else
	local_ptr = localtime(&t);
	if (local_ptr == NULL)
	{
		logger_error("localtime failed, t: %ld", (long) t);
		return NULL;
	}
# endif
#else

	struct tm local;

	if ((local_ptr = localtime_r(&t, &local)) == NULL)
	{
		logger_error("localtime_r failed, t: %ld", (long) t);
		return NULL;
	}
#endif
	if (strftime(buf, sizeof(buf), fmt, local_ptr) == 0)
	{
		logger_error("strftime failed, t: %ld, fmt: %s",
			(long) t, fmt);
		return NULL;
	}

	out = buf;
	return out.c_str();
}

} // namespace acl
