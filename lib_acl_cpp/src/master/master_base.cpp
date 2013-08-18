#include "acl_stdafx.hpp"
#include "acl_cpp/master/master_base.hpp"

namespace acl
{
master_base::master_base()
{
	daemon_mode_ = false;
	proc_inited_ = false;
}

master_base::~master_base()
{

}

void master_base::set_cfg_bool(master_bool_tbl* table)
{
	if (table == NULL)
		return;
	conf_.set_cfg_bool(table);
}

void master_base::set_cfg_int(master_int_tbl* table)
{
	if (table == NULL)
		return;
	conf_.set_cfg_int(table);
}

void master_base::set_cfg_int64(master_int64_tbl* table)
{
	if (table == NULL)
		return;
	conf_.set_cfg_int64(table);
}

void master_base::set_cfg_str(master_str_tbl* table)
{
	if (table == NULL)
		return;
	conf_.set_cfg_str(table);
}

bool master_base::daemon_mode() const
{
	return daemon_mode_;
}

}  // namespace acl
