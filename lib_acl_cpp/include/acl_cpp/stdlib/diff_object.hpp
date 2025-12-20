#pragma once
#include "../acl_cpp_define.hpp"
#include "dbuf_pool.hpp"

namespace acl {

class diff_manager;

/**
 * Pure virtual class for difference set comparison. Subclasses must inherit
 * this class and implement its pure virtual methods.
 * This class inherits from dbuf_obj class, facilitating unified management and
 * destruction by dbuf_guard
 */
class diff_object : public dbuf_obj {
public:
	/**
	 * Constructor
	 * @param manager {diff_manager&}
	 */
	diff_object(diff_manager& manager);

	virtual ~diff_object() {}

	/**
	 * Pure virtual interface, get the key string of this object
	 * @return {const char*} Must return a non-empty string
	 */
	virtual const char* get_key() const = 0;

	/**
	 * Pure virtual interface, get the value string of this object
	 * @return {const char*} Must return a non-empty string
	 */
	virtual const char* get_val() const = 0;

	/**
	 * Pure virtual interface, used to compare two objects
	 * @param obj {const diff_object&}
	 * @return {bool} Whether the two objects are equal
	 */
	virtual bool operator== (const diff_object& obj) const = 0;

	/**
	 * Whether it is redundant data outside the given range (closed interval)
	 * @param range_from {long long} Start position
	 * @param range_to {long long} End position
	 * @return {bool} Whether it is a redundant data object exceeding the given
	 * range
	 */
	virtual bool check_range(long long range_from, long long range_to) const {
		(void) range_from;
		(void) range_to;
		return false;
	}

protected:
	diff_manager& manager_;
};

} // namespace acl

