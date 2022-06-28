#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/master/master_conf.hpp"
#endif

#ifndef ACL_CLIENT_ONLY

namespace acl
{

master_conf::master_conf(void)
{
	path_       = NULL;
	cfg_loaded_ = false;

	bool_tbl_   = NULL;
	int_tbl_    = NULL;
	int64_tbl_  = NULL;
	str_tbl_    = NULL;

	cfg_        = NULL;
	bool_cfg_   = NULL;
	int_cfg_    = NULL;
	int64_cfg_  = NULL;
	str_cfg_    = NULL;
}

master_conf::~master_conf(void)
{
	reset();
}

void master_conf::reset(void)
{
	if (path_) {
		acl_myfree(path_);
	}
	if (cfg_) {
		acl_xinetd_cfg_free(cfg_);
		cfg_ = NULL;
	}
	if (int_cfg_) {
		acl_myfree(int_cfg_);
		int_cfg_ = NULL;
	}
	if (int64_cfg_) {
		acl_myfree(int64_cfg_);
		int64_cfg_ = NULL;
	}
	if (str_cfg_) {
		for (int i = 0; str_cfg_[i].name != NULL; i++) {
			if (*str_cfg_[i].target) {
				acl_myfree(*str_cfg_[i].target);
			}
		}
		acl_myfree(str_cfg_);
		str_cfg_ = NULL;
	}
	if (bool_cfg_) {
		acl_myfree(bool_cfg_);
		bool_cfg_ = NULL;
	}

	cfg_loaded_ = false;

	bool_tbl_  = NULL;
	int_tbl_   = NULL;
	int64_tbl_ = NULL;
	str_tbl_   = NULL;
}

void master_conf::load(const char* path)
{
	if (cfg_loaded_) {
		return;
	}

	if (path) {
		cfg_ = acl_xinetd_cfg_load(path);
		if (path_) {
			acl_myfree(path_);
		}
		path_ = acl_mystrdup(path);
	}

	cfg_loaded_ = true;

	load_bool();
	load_int();
	load_int64();
	load_str();
}

const char* master_conf::get_path(void) const
{
	return path_;
}

void master_conf::load_bool(void)
{
	if (!cfg_loaded_ || bool_cfg_ == NULL) {
		return;
	}
	acl_xinetd_params_bool_table(cfg_, bool_cfg_);
}

void master_conf::load_int(void)
{
	if (!cfg_loaded_ || int_cfg_ == NULL) {
		return;
	}
	acl_xinetd_params_int_table(cfg_, int_cfg_);
}

void master_conf::load_int64(void)
{
	if (!cfg_loaded_ || int64_cfg_ == NULL) {
		return;
	}
	acl_xinetd_params_int64_table(cfg_, int64_cfg_);
}

void master_conf::load_str(void)
{
	if (!cfg_loaded_ || str_cfg_ == NULL) {
		return;
	}
	acl_xinetd_params_str_table(cfg_, str_cfg_);
}

void master_conf::set_cfg_bool(master_bool_tbl* table)
{
	if (table == NULL || bool_cfg_) {
		return;
	}

	int  i = 0;
	for (; table[i].name != NULL; i++) {}

	bool_cfg_ = (ACL_CFG_BOOL_TABLE*) acl_mycalloc(i + 1,
		sizeof(ACL_CFG_BOOL_TABLE));

	for (i = 0; table[i].name != NULL; i++) {
		bool_cfg_[i].name = table[i].name;
		bool_cfg_[i].defval = table[i].defval;
		bool_cfg_[i].target = table[i].target;
	}
	bool_cfg_[i].name = NULL;
	load_bool();
}

void master_conf::set_cfg_int(master_int_tbl* table)
{
	if (table == NULL || int_cfg_) {
		return;
	}

	int  i = 0;
	for (; table[i].name != NULL; i++) {}

	int_cfg_ = (ACL_CFG_INT_TABLE*) acl_mycalloc(i + 1,
		sizeof(ACL_CFG_INT_TABLE));

	for (i = 0; table[i].name != NULL; i++) {
		int_cfg_[i].name   = table[i].name;
		int_cfg_[i].defval = table[i].defval;
		int_cfg_[i].target = table[i].target;
		int_cfg_[i].min    = table[i].min;
		int_cfg_[i].max    = table[i].max;
	}
	int_cfg_[i].name = NULL;
	load_int();
}

void master_conf::set_cfg_int64(master_int64_tbl* table)
{
	if (table == NULL || int64_cfg_) {
		return;
	}

	int  i = 0;
	for (i = 0; table[i].name != NULL; i++) {}

	int64_cfg_ = (ACL_CFG_INT64_TABLE*) acl_mycalloc(i + 1,
		sizeof(ACL_CFG_INT64_TABLE));

	for (i = 0; table[i].name != NULL; i++) {
		int64_cfg_[i].name   = table[i].name;
		int64_cfg_[i].defval = table[i].defval;
		int64_cfg_[i].target = table[i].target;
		int64_cfg_[i].min    = table[i].min;
		int64_cfg_[i].max    = table[i].max;
	}

	int64_cfg_[i].name = NULL;
	load_int64();
}

void master_conf::set_cfg_str(master_str_tbl* table)
{
	if (table == NULL || str_cfg_) {
		return;
	}

	int  i = 0;
	for (; table[i].name != NULL; i++) {}

	str_cfg_ = (ACL_CFG_STR_TABLE*) acl_mycalloc(i + 1,
		sizeof(ACL_CFG_STR_TABLE));

	for (i = 0; table[i].name != NULL; i++) {
		str_cfg_[i].name   = table[i].name;
		str_cfg_[i].defval = table[i].defval;
		str_cfg_[i].target = table[i].target;
	}
	str_cfg_[i].name = NULL;
	load_str();
}

ACL_CFG_INT_TABLE* master_conf::get_int_cfg(void) const
{
	return int_cfg_;
}

ACL_CFG_INT64_TABLE* master_conf::get_int64_cfg(void) const
{
	return int64_cfg_;
}

ACL_CFG_STR_TABLE* master_conf::get_str_cfg(void) const
{
	return str_cfg_;
}

ACL_CFG_BOOL_TABLE* master_conf::get_bool_cfg(void) const
{
	return bool_cfg_;
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
