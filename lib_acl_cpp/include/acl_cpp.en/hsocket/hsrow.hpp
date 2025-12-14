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
	 * Constructor
	 * @param ncolum {int} Number of columns per record in database query
	 */
	hsrow(int ncolum);
	~hsrow();

	/**
	 * Reset the number of columns per record for data query
	 * @param ncolum {int} Number of columns per record in database query
	 */
	void reset(int ncolum);

	/**
	 * Add column value to this query record
	 * @param value {const char*} Column value
	 * @param dlen {size_t} Column value length
	 */
	void push_back(const char* value, size_t dlen);

	/**
	 * Get query record
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
