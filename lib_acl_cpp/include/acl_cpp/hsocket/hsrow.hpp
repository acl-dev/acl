#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <vector>

#ifndef ACL_CLIENT_ONLY

namespace acl {

class string;

class ACL_CPP_API hsrow : public noncopyable
{
public:
	/**
	 * 构造函数
	 * @param ncolum {int} 数据库查询时每条记录的列数
	 */
	hsrow(int ncolum);
	~hsrow();

	/**
	 * 重置数据查询时第条记录的列数
	 * @param ncolum {int} 数据库查询时每条记录的列数
	 */
	void reset(int ncolum);

	/**
	 * 向该查询记录中添加列数值
	 * @param value {const char*} 列值
	 * @param dlen {size_t} 列值长度
	 */
	void push_back(const char* value, size_t dlen);

	/**
	 * 取得查询记录
	 * @return {const std::vector<const char*>&}
	 */
	const std::vector<const char*>& get_row() const;
private:
	std::vector<const char*> row_;
	int     ncolum_;
	int     icolum_;
	string* colums_;
};

}  // namespace acl

#endif // ACL_CLIENT_ONLY
