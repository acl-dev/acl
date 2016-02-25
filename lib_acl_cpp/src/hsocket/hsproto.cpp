#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include <string.h>
#include <stdio.h>
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/escape.hpp"
#include "acl_cpp/hsocket/hsrow.hpp"
#include "acl_cpp/hsocket/hserror.hpp"
#include "acl_cpp/hsocket/hsproto.hpp"
#endif

namespace acl
{

#define	MAX_INT	2147483647

hsproto::hsproto(bool cache_enable)
: debugOn_(false)
, cache_enable_(cache_enable)
, buf_ptr_(NULL)
{
}

hsproto::~hsproto()
{
	reset();
	clear_cache();
}

bool hsproto::build_open(string& out, int id,
	const char* dbn, const char* tbl,
	const char* idx, const char* flds)
{
	out.clear();
	out << "P\t" << id << '\t' << dbn << '\t' << tbl
		<< '\t' << idx << '\t' << flds << '\n';
	return (true);
}

bool hsproto::build_get(string& out, int id, const char* values[], int num,
	const char* cond /* = "=" */, int nlimit /* = 0 */, int noffset /* = 0 */)
{
	if (nlimit <= 0)
		nlimit = MAX_INT;
	if (noffset < 0)
		noffset = 0;
	char buf[32], *limit_offset = NULL;
	if (nlimit > 1)
	{
		safe_snprintf(buf, sizeof(buf), "%d\t%d", nlimit, noffset);
		limit_offset = buf;
	}

	build_request(out, id, cond, values, num, limit_offset, 0, 0, 0);
	return (true);
}

bool hsproto::build_get(string& out, int id, int nfld,
	const char* first_value, ...)
{
	va_list ap;
	char* ptr;
	char** pptr;

	pptr = (char**) acl_mycalloc((size_t) nfld, sizeof(char*));
	va_start(ap, first_value);

	pptr[0] = (char*) first_value;
	int   i = 1;
	while ((ptr = va_arg(ap, char*)) != NULL)
	{
		if (i >= nfld)
			break;
		pptr[i] = ptr;
		i++;
	}
	va_end(ap);

	bool ret = build_get(out, id, (const char**) pptr, i, "=", 0, 0);
	acl_myfree(pptr);
	return (ret);
}

bool hsproto::build_mod(string& out, int id, const char* values[], int num,
	const char* to_values[], int to_num, const char* cond /* = "=" */,
	int nlimit /* = 0 */, int noffset /* = 0 */)
{
	if (nlimit <= 0)
		nlimit = MAX_INT;
	if (noffset < 0)
		noffset = 0;
	char buf[32], *limit_offset = NULL;

	safe_snprintf(buf, sizeof(buf), "%d\t%d", nlimit, noffset);
	build_request(out, id, cond, values,
		num, limit_offset, 'U', to_values, to_num);
	return (true);
}

bool hsproto::build_del(string& out, int id, const char* values[],
	int num, const char* cond /* = "=" */,
	int nlimit /* = 0 */, int noffset /* = 0 */)
{
	if (nlimit <= 0)
		nlimit = 1;
	if (noffset < 0)
		noffset = 0;
	char buf[32], *limit_offset = NULL;
	if (nlimit > 1)
	{
		safe_snprintf(buf, sizeof(buf), "%d\t%d", nlimit, noffset);
		limit_offset = buf;
	}

	build_request(out, id, cond, values, num, limit_offset, 'D', 0, 0);
	return (true);
}

bool hsproto::build_del(string& out, int id, int nfld, const char* first_value, ...)
{
	va_list ap;
	char* ptr;
	char** pptr;

	pptr = (char**) acl_mycalloc((size_t) nfld, sizeof(char*));
	va_start(ap, first_value);

	pptr[0] = (char*) first_value;
	int   i = 1;
	while ((ptr = va_arg(ap, char*)) != NULL)
	{
		if (i >= nfld)
			break;
		pptr[i] = ptr;
		i++;
	}
	va_end(ap);

	bool ret = build_del(out, id, (const char**) pptr, i, "=", 0, 0);
	acl_myfree(pptr);
	return (ret);

}

bool hsproto::build_add(string& out, int id, const char* values[], int num)
{
	build_request(out, id, "+", values, num, 0, 0, 0, 0);
	return (true);
}

bool hsproto::build_add(string& out, int id, int nfld, const char* first_value, ...)
{
	va_list ap;
	char* ptr;
	char** pptr;

	pptr = (char**) acl_mycalloc((size_t) nfld, sizeof(char*));
	va_start(ap, first_value);

	pptr[0] = (char*) first_value;
	int   i = 1;
	while ((ptr = va_arg(ap, char*)) != NULL)
	{
		if (i >= nfld)
			break;
		pptr[i] = ptr;
		i++;
	}
	va_end(ap);

	bool ret = build_add(out, id, (const char**) pptr, i);
	acl_myfree(pptr);
	return (ret);
}

void hsproto::build_request(string& buf, int id, const char* oper,
	const char* values[], int num,
	const char* limit_offset, char mop,
	const char* to_values[], int to_num)
{
	char  idbuf[32], numbuf[32];

	buf.clear();

	safe_snprintf(idbuf, sizeof(idbuf), "%d", id);
	safe_snprintf(numbuf, sizeof(numbuf), "%d", num);
	buf << idbuf << "\t" << oper << "\t" << numbuf;

	int  i;
	for (i = 0; i < num; i++)
	{
		buf << "\t";
		escape(values[i], strlen(values[i]), buf);
	}

	if (limit_offset)
		buf << "\t" << limit_offset;

	if (mop)
		buf << "\t" << mop;

	if (to_values && to_num > 0)
	{
		for (i = 0; i < to_num; i++)
		{
			buf << "\t";
			escape(to_values[i], strlen(to_values[i]), buf);
		}
	}

	buf << "\n";
}

static const char* dummy_ok = "ok";

bool hsproto::parse_respond(int nfld, string& buf,
	int& errnum, const char*& serror)
{
	serror = dummy_ok;

	if (buf.empty())
	{
		errnum = HS_ERR_EMPTY;
		logger_error("respond empty");
		return (false);
	}

	buf_ptr_ = buf.c_str();
	char *last = buf_ptr_, *save = NULL;
	while (*buf_ptr_)
	{
		if (*buf_ptr_ == '\t')
		{
			save = buf_ptr_;
			*buf_ptr_++ = 0;
			break;
		}
		buf_ptr_++;
	}
	errnum = atoi(last);
	if (*buf_ptr_ == 0)
	{
		if (save)
			*save = '\t';
		errnum = HS_ERR_INVALID_REPLY;
		logger_error("respond(%s) invalid", buf.c_str());
		return (false);
	}

	last = buf_ptr_;
	save = NULL;
	while (*buf_ptr_)
	{
		if (*buf_ptr_ == '\t')
		{
			save = buf_ptr_;
			*buf_ptr_++ = 0;
			break;
		}
		buf_ptr_++;
	}

	ntoken_ = atoi(last);
	if (ntoken_ <= 0)
	{
		if (errnum != 0)
		{
			errnum = HS_ERR_INVALID_REPLY;
			logger_error("ntoken(%d) invalid", ntoken_);
			return (false);
		}
		return (false);
	}

	if (ntoken_ > nfld)
		ntoken_ = nfld;

	if (errnum == 0)
		return (true);

	serror = buf_ptr_;
	return (false);
}

hsrow* hsproto::get_next_row()
{
	static const char *dummy_nil = "";

	if (ntoken_ <= 0 || buf_ptr_ == NULL || *buf_ptr_ == 0)
		return (NULL);

	hsrow* row;

	if (!rows_cache_.empty())
	{
		row = rows_cache_[rows_cache_.size() - 1];
		rows_cache_.pop_back();
		row->reset(ntoken_);
	}
	else
		row = NEW hsrow(ntoken_);

	char* last;
	int  i;
	for (i = 0; i < ntoken_; i++)
	{
		if (*buf_ptr_ == '\0')
		{
			row->push_back(dummy_nil, 1);
			break;
		}
		if (*buf_ptr_ == '\t')
		{
			row->push_back(dummy_nil, 1);
			buf_ptr_++;
			continue;
		}

		last = buf_ptr_;
		buf_ptr_ = strchr(buf_ptr_, '\t');
		if (buf_ptr_ == NULL)
		{
			row->push_back(last, strlen(last));
			break;
		}

		*buf_ptr_++ = 0;

		row->push_back(last, strlen(last));
	}

	if (debugOn_)
	{
		std::vector<const char*>::const_iterator cit =
			row->get_row().begin();
		for (; cit != row->get_row().end(); ++cit)
		{
			printf(">>>%s\n", *cit);
		}
	}

	return (row);
}

const std::vector<hsrow*>& hsproto::get()
{
	while (true)
	{
		hsrow* row = get_next_row();
		if (row == NULL)
			break;
		rows_.push_back(row);
	}
	return (rows_);
}

void hsproto::reset()
{
	std::vector<hsrow*>::iterator it = rows_.begin();
	for (; it != rows_.end(); ++it)
	{
		if (cache_enable_)
			rows_cache_.push_back(*it);
		else
			delete (*it);
	}
	rows_.clear();
}

void hsproto::clear_cache()
{
	std::vector<hsrow*>::iterator it = rows_cache_.begin();
	for (; it != rows_cache_.end(); ++it)
		delete (*it);
	rows_cache_.clear();
}

}  // namespace acl
