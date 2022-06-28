#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include <stdarg.h>
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/escape.hpp"
#include "acl_cpp/hsocket/hsrow.hpp"
#include "acl_cpp/hsocket/hstable.hpp"
#include "acl_cpp/hsocket/hsproto.hpp"
#include "acl_cpp/hsocket/hserror.hpp"
#include "acl_cpp/hsocket/hsclient.hpp"
#endif

#ifndef ACL_CLIENT_ONLY

namespace acl
{

#define	MAX_INT	2147483647

static const char* dummy_ok = "ok";
static const char* dummy_unknown = "uknown error";

hsclient::hsclient(const char* addr, bool cache_enable /* = true */,
	bool retry_enable /* = true */)
: debugOn_(false)
, proto_(cache_enable)
, retry_enable_(retry_enable)
, id_max_(0)
, tbl_curr_(NULL)
, error_(HS_ERR_OK)
{
	addr_ = acl_mystrdup(addr);
	serror_ = dummy_ok;
}

hsclient::~hsclient(void)
{
	acl_myfree(addr_);
	clear_tables();
}

bool hsclient::open_tbl(const char* dbn, const char* tbl,
	const char* idx, const char* flds, bool auto_open /* = true */)
{
	string key(dbn);

	key << '|' << tbl << '|' << idx << '|' << flds;
	key.lower();

	if (stream_.opened()) {
		std::map<string, hstable*>::iterator it = tables_.find(key);
		if (it != tables_.end()) {
			tbl_curr_ = it->second;
			return true;
		}
	}

	if (!auto_open) {
		error_    = HS_ERR_NOT_OPEN;
		tbl_curr_ = NULL;
		return false;
	}
	return open_tbl(dbn, tbl, idx, flds, key.c_str());
}


bool hsclient::open_tbl(const char* dbn, const char* tbl,
	const char* idx, const char* flds, const char* key)
{
	if (!stream_.opened()) {
		clear_tables();

		if (!stream_.open(addr_, 60, 60)) {
			error_    = HS_ERR_CONN;
			tbl_curr_ = NULL;
			logger_error("open %s error(%s)",
				addr_, acl_last_serror());
			return false;
		}
	}

	tbl_curr_ = NEW hstable(id_max_++, dbn, tbl, idx, flds);

	cond_def_[0] = '=';
	cond_def_[1] = 0;

	buf_.clear();
	buf_ << "P\t" << tbl_curr_->id_ << '\t' << dbn << '\t' << tbl
		<< '\t' << idx << '\t' << flds << '\n';

	if (!stream_.write(buf_)) {
		error_ = HS_ERR_WRITE;
		close_stream();
		delete tbl_curr_;
		tbl_curr_ = NULL;
		logger_error("send(%s) to %s error", buf_.c_str(), addr_);
		return false;
	}

	buf_.clear();
	if (!stream_.gets(buf_)) {
		error_ = HS_ERR_READ;
		close_stream();
		delete tbl_curr_;
		tbl_curr_ = NULL;
		logger_error("open error for read from %s", addr_);
		return false;
	}

	bool ret = proto_.parse_respond(tbl_curr_->nfld_, buf_, error_, serror_);
	if (ret) {
		tables_[key] = tbl_curr_;
	} else {
		delete tbl_curr_;
		tbl_curr_ = NULL;
	}
	return ret;
}

void hsclient::clear_tables(void)
{
	std::map<string, hstable*>::iterator it = tables_.begin();
	for (; it != tables_.end(); ++it) {
		delete it->second;
	}
	tables_.clear();
}

void hsclient::close_stream(void)
{
	// 关闭流连接，但并不释放流对象
	stream_.close();

	// 必须清除与该流对象相关的已经打开的表对象
	clear_tables();
}

const std::vector<hsrow*>& hsclient::get(const char* values[], int num,
	const char* cond /* = "=" */, int nlimit /* = 0 */,
	int noffset /* = 0 */)
{
	proto_.reset();

	if (tbl_curr_ == NULL) {
		error_ = HS_ERR_NOT_OPEN;
		logger_error("tbl not opened yet!");
		return proto_.get();
	} else if (values == NULL || values[0] == NULL) {
		error_ = HS_ERR_PARAMS;
		logger_error("values null");
		return proto_.get();
	} else if (num <= 0 || num > tbl_curr_->nfld_) {
		error_ = HS_ERR_PARAMS;
		logger_error("num(%d) invalid, nfld(%d)",
			num, tbl_curr_->nfld_);
		return proto_.get();
	} else if (cond == NULL || *cond == 0) {
		error_ = HS_ERR_PARAMS;
		logger_error("cond null");
		return proto_.get();
	}

	if (nlimit <= 0) {
		nlimit = MAX_INT;
	}
	if (noffset < 0) {
		noffset = 0;
	}
	char buf[32], *limit_offset = NULL;
	if (nlimit > 1) {
		safe_snprintf(buf, sizeof(buf), "%d\t%d", nlimit, noffset);
		limit_offset = buf;
	}

	(void) query(cond, values, num, limit_offset, (char) 0, 0, 0);
	return proto_.get();
}

const std::vector<hsrow*>& hsclient::get(const char* first_value, ...)
{
	if (tbl_curr_ == NULL) {
		error_ = HS_ERR_NOT_OPEN;
		logger_error("tbl not opened yet!");
		proto_.reset();
		return proto_.get();
	}

	va_list ap;
	char *ptr;

	va_start(ap, first_value);

	tbl_curr_->values_[0] = (char*) first_value;
	int   i = 1;
	while ((ptr = va_arg(ap, char*)) != NULL) {
		if (i >= tbl_curr_->nfld_) {
			break;
		}
		tbl_curr_->values_[i] = ptr;
		i++;
	}
	va_end(ap);

	return get((const char**) tbl_curr_->values_, i, cond_def_, 0, 0);
}

bool hsclient::mod(const char* values[], int num,
	const char* to_values[], int to_num, const char* cond /* = "=" */,
	int nlimit /* = 0 */, int noffset /* = 0 */)
{
	if (tbl_curr_ == NULL) {
		error_ = HS_ERR_NOT_OPEN;
		logger_error("tbl not opened yet!");
		return false;
	} else if (values == NULL || values[0] == NULL) {
		error_ = HS_ERR_PARAMS;
		logger_error("values null");
		return false;
	} else if (num <= 0 || num > tbl_curr_->nfld_) {
		error_ = HS_ERR_PARAMS;
		logger_error("num(%d) invalid, nfld(%d)",
			num, tbl_curr_->nfld_);
		return false;
	} else if (cond == NULL || *cond == 0) {
		error_ = HS_ERR_PARAMS;
		logger_error("cond null");
		return false;
	} else if (to_values == NULL || to_values[0] == NULL) {
		error_ = HS_ERR_PARAMS;
		logger_error("to_values null");
		return false;
	} else if (to_num <= 0 || to_num > tbl_curr_->nfld_) {
		error_ = HS_ERR_PARAMS;
		logger_error("to_num(%d) invalid, nfld(%d)",
			to_num, tbl_curr_->nfld_);
		return false;
	}

	if (nlimit <= 0) {
		nlimit = MAX_INT;
	}
	if (noffset < 0) {
		noffset = 0;
	}
	char buf[32];
	safe_snprintf(buf, sizeof(buf), "%d\t%d", nlimit, noffset);
	return query(cond, values, num, buf, 'U', to_values, to_num);
}

bool hsclient::del(const char* values[], int num,
	const char* cond /* = "=" */, int nlimit /* = 1 */, int noffset /* = 0 */)
{
	if (tbl_curr_ == NULL) {
		error_ = HS_ERR_NOT_OPEN;
		logger_error("tbl not opened yet!");
		return false;
	} else if (values == NULL || values[0] == NULL) {
		error_ = HS_ERR_PARAMS;
		logger_error("values null");
		return false;
	} else if (num <= 0 || num > tbl_curr_->nfld_) {
		error_ = HS_ERR_PARAMS;
		logger_error("num(%d) invalid, nfld(%d)",
			num, tbl_curr_->nfld_);
		return false;
	} else if (cond == NULL || *cond == 0) {
		error_ = HS_ERR_PARAMS;
		logger_error("cond null");
		return false;
	}

	if (nlimit <= 0) {
		nlimit = 1;
	}
	if (noffset < 0) {
		noffset = 0;
	}
	char buf[32], *limit_offset = NULL;
	if (nlimit > 1) {
		safe_snprintf(buf, sizeof(buf), "%d\t%d", nlimit, noffset);
		limit_offset = buf;
	}

	return query(cond, values, num, limit_offset, 'D', 0, 0);
}

bool hsclient::fmt_del(const char* first_value, ...)
{
	if (tbl_curr_ == NULL) {
		error_ = HS_ERR_NOT_OPEN;
		logger_error("tbl not opened yet!");
		return false;
	}

	va_list ap;
	char *ptr;

	va_start(ap, first_value);

	tbl_curr_->values_[0] = NULL;
	int   i = 0;
	while ((ptr = va_arg(ap, char*)) != NULL) {
		if (i >= tbl_curr_->nfld_) {
			break;
		}
		tbl_curr_->values_[i] = ptr;
		i++;
	}
	va_end(ap);

	return del((const char**) tbl_curr_->values_, i, cond_def_, 0, 0);
}

bool hsclient::add(const char* values[], int num)
{
	if (tbl_curr_ == NULL) {
		error_ = HS_ERR_NOT_OPEN;
		logger_error("tbl not opened yet!");
		return false;
	} else if (values == NULL || values[0] == NULL) {
		error_ = HS_ERR_PARAMS;
		logger_error("input invalid");
		return false;
	} else if (num <= 0 || num > tbl_curr_->nfld_) {
		error_ = HS_ERR_PARAMS;
		logger_error("num(%d) invalid, nfld(%d)",
			num, tbl_curr_->nfld_);
		return false;
	} 

	return query("+", values, num, NULL, 0, NULL, 0);
}

bool hsclient::fmt_add(const char* first_value, ...)
{
	if (tbl_curr_ == NULL) {
		error_ = HS_ERR_NOT_OPEN;
		logger_error("tbl not opened yet!");
		return false;
	}

	va_list ap;
	char *ptr;

	va_start(ap, first_value);

	tbl_curr_->values_[0] = NULL;
	int   i = 0;
	while ((ptr = va_arg(ap, char*)) != NULL) {
		if (i >= tbl_curr_->nfld_) {
			break;
		}
		tbl_curr_->values_[i] = ptr;
		i++;
	}
	va_end(ap);

	return add((const char**) tbl_curr_->values_, i);
}

bool hsclient::query(const char* oper, const char* values[], int num,
	const char* limit_offset, char mop,
	const char* to_values[], int to_num)
{
	acl_assert(tbl_curr_);

	bool retried = false;

	while (true) {
		// 创建请求数据
		hsproto::build_request(buf_, tbl_curr_->id_, oper, values,
			num, limit_offset, mop, to_values, to_num);

		if (debugOn_) {
			printf("%s(%d)>>>send: (%s)\n",
				__FUNCTION__, __LINE__, buf_.c_str());
		}

		// 与数据库通信并从数据库获得结果
		if (chat()) {
			break;
		}

		// 如果与数据库通信失败当允许重试时若重试也失败则返回错误
		if (retry_enable_ == false || retried) {
			close_stream();
			return false;
		}

		retried = true;

		// 先缓冲当前表结构中的信息
		string dbn(tbl_curr_->dbn_), tbl(tbl_curr_->tbl_);
		string idx(tbl_curr_->idx_), flds(tbl_curr_->flds_);

		// 先关闭旧的连接对象及所有的表对象
		close_stream();

		// 再重新打开连接对象并打开表对象
		if (!open_tbl(dbn.c_str(), tbl.c_str(), idx.c_str(),
			flds.c_str(), true)) {

			logger_error("reopen error");
			return (false);
		}
	}

	if (debugOn_) {
		printf("%s(%d): gets: (%s)\n",
			__FUNCTION__, __LINE__, buf_.c_str());
	}
	return proto_.parse_respond(tbl_curr_->nfld_, buf_, error_, serror_);
}

bool hsclient::chat(void)
{
	if (!stream_.write(buf_)) {
		error_ = HS_ERR_WRITE;
		logger_error("send(%s) error(%s)",
			buf_.c_str(), acl_last_serror());
		return false;
	}

	if (!stream_.gets(buf_)) {
		error_ = HS_ERR_READ;
		logger_error("gets error(%s)", acl_last_serror());
		return false;
	}

	error_ = HS_ERR_OK;
	return true;
}

int hsclient::get_error(void) const
{
	return error_;
}

const char* hsclient::get_serror(int errnum) const
{
	return hserror::get_serror(errnum);
}

const char* hsclient::get_last_serror(void) const
{
	if (error_ == 0) {
		return dummy_ok;
	}
	const char* ptr = hserror::get_serror(error_);
	if (ptr != dummy_unknown) {
		return ptr;
	}
	return serror_;
}

const char* hsclient::get_addr(void) const
{
	return addr_;
}

int  hsclient::get_id(void) const
{
	if (tbl_curr_ == NULL) {
		logger_warn("tbl not open!");
		return -1;
	}
	return tbl_curr_->id_;
}

void hsclient::debug_enable(bool on)
{
	debugOn_ = on;
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
