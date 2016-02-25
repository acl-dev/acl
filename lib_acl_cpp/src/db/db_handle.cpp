#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include <assert.h>
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/db/db_pool.hpp"
#include "acl_cpp/db/query.hpp"
#include "acl_cpp/db/db_handle.hpp"
#endif

namespace acl
{

//////////////////////////////////////////////////////////////////////////

db_row::db_row(const std::vector<const char*>& names)
: names_(names)
{

}

db_row::~db_row()
{

}

const char* db_row::field_name(size_t ifield) const
{
	if (ifield >= names_.size())
	{
		logger_error("ifield: %d > names_.size: %d",
			(int) ifield, (int) names_.size());
		return NULL;
	}
	return names_[ifield];
}

const char* db_row::field_value(const char* name) const
{
	size_t   i, n = names_.size();

	// 必须保证表中字段名的个数与行记录的值的个数相等
	acl_assert(names_.size() == values_.size());

	// 通过扫描字段名找出字段值的下标位置
	for (i = 0; i < n; i++)
	{
		if (strcasecmp(name, names_[i]) == 0)
			break;
	}
	if (i == n)
	{
		logger_error("cloumn not exist, name: %s", name);
		return (NULL);
	}

	// 直接返回相应下标的字段值
	return values_[i];
}

const char* db_row::operator [](const char* name) const
{
	return field_value(name);
}

const char* db_row::field_value(size_t ifield) const
{
	if (ifield >= values_.size())
	{
		logger_error("ifield(%d) invalid, values_.size: %d",
			(int) ifield, (int) values_.size());
		return NULL;
	}

	return values_[ifield];
}

const char* db_row::operator [](size_t ifield) const
{
	return field_value(ifield);
}

int db_row::field_int(size_t ifield, int null_value /* = 0 */) const
{
	const char* ptr = field_value(ifield);
	if (ptr == NULL)
		return null_value;
	else
		return atoi(ptr);
}

int db_row::field_int(const char* name, int null_value /* = 0 */) const
{
	const char* ptr = field_value(name);
	if (ptr == NULL)
		return null_value;
	else
		return atoi(ptr);
}

acl_int64 db_row::field_int64(size_t ifield, acl_int64 null_value /* = 0 */) const
{
	const char* ptr = field_value(ifield);
	if (ptr == NULL)
		return null_value;
	else
		return acl_atoi64(ptr);
}

acl_int64 db_row::field_int64(const char* name, acl_int64 null_value /* = 0 */) const
{
	const char* ptr = field_value(name);
	if (ptr == NULL)
		return null_value;
	else
		return acl_atoi64(ptr);
}

double db_row::field_double(size_t ifield, double null_value /* = 0.0 */) const
{
	const char* ptr = field_value(ifield);
	if (ptr == NULL)
		return null_value;
	else
		return atof(ptr);
}

double db_row::field_double(const char* name, double null_value /* = 0.0 */) const
{
	const char* ptr = field_value(name);
	if (ptr == NULL)
		return null_value;
	else
		return atof(ptr);
}

const char* db_row::field_string(size_t ifield) const
{
	const char* ptr = field_value(ifield);
	if (ptr == NULL)
		return NULL;
	else
		return ptr;
}

const char* db_row::field_string(const char* name) const
{
	const char* ptr = field_value(name);
	if (ptr == NULL)
		return NULL;
	else
		return ptr;
}

void db_row::push_back(const char* value)
{
	values_.push_back(value);
}

size_t db_row::length() const
{
	return values_.size();
}

//////////////////////////////////////////////////////////////////////////

db_rows::db_rows()
{

}

db_rows::~db_rows()
{
	std::vector<db_row*>::iterator it = rows_.begin();
	for (; it != rows_.end(); ++it)
		delete (*it);
}

const std::vector<const db_row*>& db_rows::get_rows(
	const char* name, const char* value)
{
	// 先清空上一次的临时结果集
	rows_tmp_.clear();

	if (empty())
		return rows_tmp_;

	size_t icolumn, ncolumn = names_.size();

	// 通过扫描字段名找出字段值的下标位置
	for (icolumn = 0; icolumn < ncolumn; icolumn++)
	{
		if (strcasecmp(name, names_[icolumn]) == 0)
			break;
	}

	const db_row* row;
	const char* ptr = 0;

	// 比较对应下标相同的字段值的行记录
	size_t nrow = rows_.size();
	for (size_t irow = 0; irow < nrow; irow++)
	{
		row = rows_[irow];
		acl_assert(row->length() == ncolumn);
		ptr = (*row)[icolumn];
		if (ptr && strcmp(ptr, value) == 0)
			rows_tmp_.push_back(row);
	}

	return rows_tmp_;
}

const std::vector<db_row*>& db_rows::get_rows() const
{
	return rows_;
}

const db_row* db_rows::operator [](size_t idx) const
{
	if (idx >= rows_.size())
	{
		logger_error("idx(%d) >= rows_.size %d",
			(int) idx, (int) rows_.size());
		return NULL;
	}

	db_row* row = rows_[idx];
	return row;
}

bool db_rows::empty() const
{
	return rows_.empty();
}

size_t db_rows::length() const
{
	return rows_.size();
}

//////////////////////////////////////////////////////////////////////////

db_handle::db_handle()
: result_(NULL)
, id_(NULL)
{
	time(&when_);
}

db_handle::~db_handle()
{
	if (id_)
		acl_myfree(id_);
	free_result();
}

bool db_handle::open()
{
	// 调用虚方法的子类实现过程
	return dbopen();
}

bool db_handle::exec_select(query& query)
{
	return sql_select(query.to_string().c_str());
}

bool db_handle::exec_update(query& query)
{
	return sql_update(query.to_string().c_str());
}

string& db_handle::escape_string(const char* in, size_t len, string& out)
{
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

void db_handle::print_out(size_t max /* = 0 */) const
{
	// 列出查询结果方法二
	for (size_t i = 0; i < length(); i++)
	{
		if (max > 0 && i >= max)
			continue;

		const acl::db_row* row = (*this)[i];

		for (size_t j = 0; j < row->length(); j++)
			printf("%s, ", (*row)[j]);
		printf("\r\n");
	}

	printf("total result: %d\n", (int) length());
}


const db_rows* db_handle::get_result() const
{
	return result_;
}

const std::vector<const db_row*>* db_handle::get_rows(
	const char* name, const char* value)
{
	if (result_ == NULL)
		return NULL;
	const std::vector<const db_row*>& rows = result_->get_rows(name, value);
	return &rows;
}

const std::vector<db_row*>* db_handle::get_rows() const
{
	if (result_ == NULL)
		return NULL;
	const std::vector<db_row*>& rows = result_->get_rows();
	return &rows;
}

const db_row* db_handle::get_first_row() const
{
	if (result_ == NULL)
		return NULL;

	const std::vector<db_row*>& rows = result_->get_rows();
	const acl::db_row* first_row = rows[0];
	acl_assert(first_row);
	return first_row;
}

void db_handle::free_result()
{
	if (result_)
	{
		delete result_;
		result_ = NULL;
	}
}

const db_row* db_handle::operator [](size_t idx) const
{
	if (result_ == NULL)
		return NULL;
	if (idx >= result_->length())
		return (NULL);
	return (*result_)[idx];
}

size_t db_handle::length() const
{
	if (result_ == NULL)
		return 0;
	else
		return result_->length();
}

bool db_handle::empty() const
{
	return length() == 0 ? true : false;
}

db_handle& db_handle::set_id(const char* id)
{
	if (id == NULL || *id == 0)
		return *this;
	if (id_)
		acl_myfree(id_);
	id_ = acl_mystrdup(id);
	return *this;
}

db_handle& db_handle::set_when(time_t now)
{
	when_ = now;
	return *this;
}

static string __loadpath;

void db_handle::set_loadpath(const char* path)
{
	if (path && *path)
		__loadpath = path;
}

const char* db_handle::get_loadpath()
{
	return __loadpath.empty() ? NULL : __loadpath.c_str();
}

} // namespace acl
