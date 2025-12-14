#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

#ifndef ACL_CLIENT_ONLY

struct ACL_XINETD_CFG_PARSER;
struct ACL_CFG_INT_TABLE;
struct ACL_CFG_INT64_TABLE;
struct ACL_CFG_STR_TABLE;
struct ACL_CFG_BOOL_TABLE;

namespace acl {

typedef struct master_int_tbl {
	const char *name;
	int  defval;
	int *target;
	int  min;
	int  max;
} master_int_tbl;

typedef struct master_str_tbl {
	const char *name;
	const char *defval;
	char **target;
} master_str_tbl;

typedef struct master_bool_tbl {
	const char *name;
	int   defval;
	int  *target;
} master_bool_tbl;

typedef struct master_int64_tbl {
	const char *name;
#if defined(_WIN32) || defined(_WIN64)
	__int64  defval;
	__int64 *target;
	__int64  min;
	__int64  max;
#else
	long long int  defval;
	long long int *target;
	long long int  min;
	long long int  max;
#endif
} master_int64_tbl;

class master_base;

class ACL_CPP_API master_conf : public noncopyable {
public:
	master_conf();
	~master_conf();

	/**
	 * Set bool type configuration item
	 * @param table {master_bool_tbl*}
	 */
	void set_cfg_bool(master_bool_tbl* table);

	/**
	 * Set int type configuration item
	 * @param table {master_int_tbl*}
	 */
	void set_cfg_int(master_int_tbl* table);

	/**
	 * Set int64 type configuration item
	 * @param table {master_int64_tbl*}
	 */
	void set_cfg_int64(master_int64_tbl* table);

	/**
	 * Set string type configuration item
	 * @param table {master_str_tbl*}
	 */
	void set_cfg_str(master_str_tbl* table);

	/**
	 * Load configuration file
	 * @param path {const char*} Full path of configuration file
	 */
	void load(const char* path);

	/**
	 * Get the configuration file path set by load
	 * @return {const char*} Returns NULL if configuration file path was not set
	 */
	const char* get_path() const;

	/**
	 * Reset configuration parser state, release previously allocated resources.
	 * After calling this function,
	 * the memory of string configuration items obtained previously will be
	 * released, so they are forbidden to use;
	 * After calling this function, the configuration parser object can be used
	 * again to parse other
	 * configuration files
	 */
	void reset();

	ACL_CFG_INT_TABLE* get_int_cfg() const;
	ACL_CFG_INT64_TABLE* get_int64_cfg() const;
	ACL_CFG_STR_TABLE* get_str_cfg() const;
	ACL_CFG_BOOL_TABLE* get_bool_cfg() const;

private:
	char* path_;
	bool  cfg_loaded_;

	ACL_XINETD_CFG_PARSER* cfg_;
	ACL_CFG_INT_TABLE*  int_cfg_;
	ACL_CFG_INT64_TABLE* int64_cfg_;
	ACL_CFG_STR_TABLE*  str_cfg_;
	ACL_CFG_BOOL_TABLE* bool_cfg_;

	master_int_tbl* int_tbl_;
	master_str_tbl* str_tbl_;
	master_bool_tbl* bool_tbl_;
	master_int64_tbl* int64_tbl_;

	void load_str();
	void load_bool();
	void load_int();
	void load_int64();
};

} // namespace acl

#endif // ACL_CLIENT_ONLY

