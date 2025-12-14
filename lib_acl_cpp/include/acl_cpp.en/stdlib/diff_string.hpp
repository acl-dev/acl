#pragma once
#include "../acl_cpp_define.hpp"
#include "diff_object.hpp"

namespace acl {

class diff_string : public diff_object {
public:
	/**
	 * Constructor
	 * @param manager {diff_manager&}
	 * @param key {const char*} Key represented as string, non-empty string
	 * @param val {const char*} Value represented as string, non-empty string
	 */
	diff_string(diff_manager& manager, const char* key, const char* val);

	/**
	 * Set the range value for this object
	 * @param range {long long}
	 */
	void set_range(long long range);

	/**
	 * Get the range value for this object
	 * @return {long long}
	 */
	long long get_range() const {
		return range_;
	}

public:
	// override: Implementation of base class pure virtual function
	const char* get_key() const;

	// override: Implementation of base class pure virtual function
	const char* get_val() const;

	// override: Implementation of base class pure function
	bool operator== (const diff_object& obj) const;

	// @override
	bool check_range(long long range_from, long long range_to) const;

private:
	const char* key_;
	const char* val_;
	long long range_;

	// Destructor declared as private, requiring dynamic creation of objects of this class
	~diff_string();
};

} // namespace acl
