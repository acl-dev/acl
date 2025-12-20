#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include <string>
#include "singleton.hpp"

#define	ACL_COUNTER_INIT(thread_safe)                                         \
	acl::class_counter::get_instance().init(thread_safe)

// Increment a certain character flag once. flagName is a unique character
// object defined by yourself.
#define ACL_COUNTER_INC(flagName)                                             \
	acl::class_counter::get_instance().inc(#flagName)

// Decrement a certain character flag once.
#define ACL_COUNTER_DEC(flagName)                                             \
	acl::class_counter::get_instance().dec(#flagName)

#define	ACL_COUNTER_COUNT(flagName)                                           \
	acl::class_counter::get_instance().count(#flagName)

#define	ACL_COUNTER_PRINT()                                                   \
	acl::class_counter::get_instance().print()

namespace acl {

class thread_mutex;

class ACL_CPP_API class_counter : public singleton<class_counter> {
public:
	/**
	 * Constructor.
	 * @param clean {bool} Whether to automatically clear counter objects with
	 * count 0.
	 */
	class_counter(bool clean = true);
	~class_counter();

	/**
	 * Can call this method during process initialization (not required) to
	 * initialize, specifying whether internal locking is needed.
	 * If this method is not called, internal automatically adds thread lock.
	 */
	void init(bool thread_safe = true);

	/**
	 * @brief Increment count corresponding to name by 1. When name does not exist,
	 * set to 1.
	 */
	void inc(const char *name);

	/**
	 * @brief Decrement count corresponding to name by 1. When name does not exist,
	 * output error log.
	 */
	void dec(const char *name);

	/**
	 * @brief Get statistical count of name object.
	 */
	long long count(const char *name);

	/**
	 * @brief Output count statistics.
	 * @param flag Caller flag.
	 */
	void print(const char *flag = NULL);

private:
	std::map<std::string, long long> names_;
	thread_mutex* lock_;
	bool clean_;
};

}  // namespace acl
