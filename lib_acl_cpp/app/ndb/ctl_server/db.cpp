#include "StdAfx.h"
#include "db.h"

db_idx::db_idx(db_tbl* tbl, const char* name, unsigned int id, idx_type_t type)
{
	acl_assert(tbl);
	tbl_ = tbl;
	acl_assert(name && *name);
	name_ = acl_mystrdup(name);
	acl_lowercase(name_);
	id_ = id;
	type_ = type;
}

db_idx::~db_idx()
{
	acl_myfree(name_);
}

//////////////////////////////////////////////////////////////////////////

db_tbl::db_tbl(database* db, const char* name, unsigned int id)
{
	acl_assert(db);
	db_ = db;
	acl_assert(name && *name);
	name_ = acl_mystrdup(name);
	acl_lowercase(name_);
	id_ = id;
}

db_tbl::~db_tbl()
{
	acl_myfree(name_);
	std::list<db_idx*>::iterator it = idxes_.begin();
	for (; it != idxes_.end(); it++)
		delete (*it);
}

void db_tbl::add_idx(db_idx* idx)
{
	idxes_.push_back(idx);
}

//////////////////////////////////////////////////////////////////////////

database::database(const char* name, unsigned int id)
{
	acl_assert(name && *name);
	name_ = acl_mystrdup(name);
	acl_lowercase(name_);
	id_ = id;
	lock_ = new acl::locker();
}

database::~database(void)
{
	acl_myfree(name_);

	std::map<std::string, db_tbl*>::iterator it = tables_.begin();
	for (; it != tables_.end(); it++)
		delete it->second;
	delete lock_;
}

void database::add_tbl(db_tbl* tbl)
{
	lock_->lock();
	tables_[tbl->get_name()] = tbl;
	lock_->unlock();
}

void database::add_idx_host(idx_host* host)
{
	lock_->lock();
	// 需要检查是否是重复添加
	std::vector<idx_host*>::iterator it = idx_hosts_.begin();
	for (; it != idx_hosts_.end(); it++)
	{
		if (host == *it)
		{
			lock_->unlock();
			return;
		}
	}
	idx_hosts_.push_back(host);
	lock_->unlock();
}
