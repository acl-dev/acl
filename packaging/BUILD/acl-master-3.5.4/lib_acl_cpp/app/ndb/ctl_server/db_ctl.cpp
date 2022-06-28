#include "StdAfx.h"
#include "db_mysql.hpp"

#include "driver_mysql.h"
#include "db_conf.h"
#include "db.h"
#include "db_host.h"
#include "db_ctl.h"

struct NAME_TYPE
{
	unsigned int id;
	char name[65];
	name_type_t type;
};

struct DB_HOST
{
	unsigned int id_db;
	unsigned int id_idx_host;
	acl_int64 count;
};

struct DB_TBL
{
	unsigned int id_db;
	unsigned int id_tbl;
	acl_int64 count;
};

struct TBL_IDX 
{
	unsigned int id_idx;
	unsigned int id_db;
	unsigned int id_tbl;
	bool unique;
	idx_type_t type;
};

db_ctl::db_ctl(void)
{
	driver_ = new driver_mysql(var_cfg_mysql_dbaddr, var_cfg_mysql_dbname,
		var_cfg_mysql_dbuser, var_cfg_mysql_dbpass,
		var_cfg_mysql_dbpool_limit, var_cfg_mysql_dbpool_dbping,
		var_cfg_mysql_dbpool_timeout);
	errnum_ = DB_CTL_OK;
	ctl_conn_ = new acl::db_mysql(var_cfg_mysql_dbaddr,
		var_cfg_mysql_dbname, var_cfg_mysql_dbuser,
		var_cfg_mysql_dbpass);
	lock_ = new acl::locker();
	ctl_conn_lock_ = new acl::locker();
}

db_ctl::~db_ctl(void)
{
	delete driver_;
	delete ctl_conn_;

	std::map<std::string, database*>::iterator it = dbs_.begin();
	for (; it != dbs_.end(); it++)
		delete it->second;

	std::vector<idx_host*>::iterator idx_host_it = idx_hosts_.begin();
	for (; idx_host_it != idx_hosts_.end(); idx_host_it++)
		delete (*idx_host_it);

	std::vector<dat_host*>::iterator dat_host_it = dat_hosts_.begin();
	for (; dat_host_it != dat_hosts_.end(); dat_host_it++)
		delete (*dat_host_it);

	std::list<NAME_TYPE*>::iterator name_it = names_.begin();
	for (; name_it != names_.end(); name_it++)
		acl_myfree(*name_it);

	std::list<DB_HOST*>::iterator db_host_it = db_hosts_.begin();
	for (; db_host_it != db_hosts_.end(); db_host_it++)
		acl_myfree(*db_host_it);

	std::list<DB_TBL*>::iterator db_tbl_it = db_tbls_.begin();
	for (; db_tbl_it != db_tbls_.end(); db_tbl_it++)
		acl_myfree(*db_tbl_it);

	std::list<TBL_IDX*>::iterator tbl_idx_it = tbl_idxes_.begin();
	for (; tbl_idx_it != tbl_idxes_.end(); tbl_idx_it++)
		acl_myfree(*tbl_idx_it);

	delete lock_;
	delete ctl_conn_lock_;
}

#define NCP(x, y, z)	ACL_SAFE_STRNCPY((x), (y), (z))

void db_ctl::load()
{
	load_names();
	load_idx_hosts();
	load_dat_hosts();
	load_db_hosts();
	load_db_tbls();
	load_tbl_idxes();

	// 将从数据库中查询的信息进行关联构建
	build_db();
}

database* db_ctl::db_open(const char* dbname, const char* dbuser /* = NULL */,
	const char* dbpass /* = NULL */)
{
	lock_->lock();
	std::map<std::string, database*>::iterator it = dbs_.find(dbname);
	if (it == dbs_.end())
	{
		lock_->unlock();
		return NULL;
	}
	lock_->unlock();
	return it->second;
}

unsigned int db_ctl::db_add_name(const char* name, name_type_t type)
{
	char sql[256];

	// 先添加数据库记录
	snprintf(sql, sizeof(sql), "insert into tbl_name_type(name, type)"
		" values('%s', %d)", name, (int) type);

	// 不必关心数据库记录是否已经存在
	(void) ctl_conn_->sql_update(sql);

	// 查询数据库名所对应的ID号
	snprintf(sql, sizeof(sql), "select id from tbl_name_type"
		" where name='%s' and type=0", name);
	if (ctl_conn_->sql_select(sql) == false)
	{
		logger_error("create db(%s) error", name);
		return (unsigned int) -1;
	}
	const acl::db_row* first_row = ctl_conn_->get_first_row();
	if (first_row == NULL)
	{
		logger_error("create db(%s) error, sql(%s)", name, sql);
		return (unsigned int) -1;
	}

	unsigned int id = (unsigned int) first_row->field_int("id", -1);
	if (id == (unsigned int) -1)
	{
		ctl_conn_->free_result();
		logger_error("create db(%s) error, id(-1) invalid", name);
		return (unsigned int) -1;
	}

	ctl_conn_->free_result();
	return id;
}

database* db_ctl::db_create(const char* dbname, const char* dbuser /* = NULL */,
	const char* dbpass /* = NULL */)
{
	// 因为对 ctl_db 只有一个数据库连接，所以需要加锁
	ctl_conn_lock_->lock();
	unsigned int id = db_add_name(dbname, NAME_TYPE_DB);
	// 释放数据库连接互斥锁
	ctl_conn_lock_->unlock();

	if (id == (unsigned int) -1)
	{
		logger_error("add dbname(%s) to tbl_name_type error", dbname);
		return NULL;
	}

	// 先加锁
	lock_->lock();

	// 先检查是否已经存在了
	std::map<std::string, database*>::iterator it = dbs_.find(dbname);
	if (it != dbs_.end())
	{
		lock_->unlock();
		return it->second;
	}
	database* db = new database(dbname, id);
	db_host_set(db);  // 新创建的数据库对象需要设置其索引服务器位置
	dbs_[dbname] = db;

	// 解锁
	lock_->unlock();
	return db;
}

bool db_ctl::db_host_set(database* db)
{
	char  sql[256];
	std::vector<idx_host*>::iterator it = idx_hosts_.begin();
	ctl_conn_lock_->lock();
	for (; it != idx_hosts_.end(); it++)
	{
		idx_host* host = *it;
		snprintf(sql, sizeof(sql),
			"insert into tbl_db_host(id_db, id_idx_host, count)"
			" values(%u, %u, 0)", db->get_id(), host->get_id());
		if (ctl_conn_->sql_update(sql) == false)
		{
			ctl_conn_lock_->unlock();
			logger_error("sql(%s) error", sql);
			return false;
		}
		db->add_idx_host(host);
	}

	ctl_conn_lock_->unlock();
	return true;
}

// 添加数据表对象，需要先在数据库中添加表记录，然后获得相应ID
db_tbl* db_ctl::db_add_tbl(database* db, const char* tbl_name)
{
	// 因为对 ctl_db 只有一个数据库连接，所以需要加锁
	ctl_conn_lock_->lock();
	unsigned int id = db_add_name(tbl_name, NAME_TYPE_TBL);
	// 释放数据库连接互斥锁
	ctl_conn_lock_->unlock();

	if (id == (unsigned int) -1)
	{
		logger_error("add tbl_name(%s) to tbl_name_type error", tbl_name);
		return NULL;
	}

	lock_->lock();
	add_name(tbl_name, id, NAME_TYPE_TBL);
	add_tbl(db->get_id(), id, 0);
	lock_->unlock();

	// 将表对象添加进数据库对象中
	db_tbl* tbl = new db_tbl(db, tbl_name, id);
	db->add_tbl(tbl);
	return tbl;
}

db_idx* db_ctl::db_add_idx(db_tbl* tbl, const char* tbl_idx,
	idx_type_t idx_type)
{
	// 因为对 ctl_db 只有一个数据库连接，所以需要加锁
	ctl_conn_lock_->lock();
	unsigned int id = db_add_name(tbl_idx, NAME_TYPE_TBL);
	// 释放数据库连接互斥锁
	ctl_conn_lock_->unlock();

	if (id == (unsigned int) -1)
	{
		logger_error("add tbl_idx(%s) to tbl_name_type error", tbl_idx);
		return NULL;
	}

	lock_->lock();
	lock_->unlock();
	return NULL;
}

db_idx* db_ctl::db_add_idx(database* db, const char* tbl_name,
	const char* tbl_idx, idx_type_t idx_type)
{
	return NULL;
}

int db_ctl::load_names(void)
{
	const char* sql = "select id, name, type from tbl_name";
	if (ctl_conn_->sql_select(sql) == false)
	{
		logger_error("sql(%s) error", sql);
		return -1;
	}

	const std::vector<acl::db_row*>* rows = ctl_conn_->get_rows();
	if (rows == NULL)
	{
		logger_error("tbl_name empty");
		return 0;
	}

	unsigned int id;
	int   type;
	const char* name;
	size_t i;
	for (i = 0; i < rows->size(); i++)
	{
		const acl::db_row* row = (*rows)[i];
		id = row->field_int("id", -1);
		if (id == (unsigned int) -1)
		{
			logger_error("invalid id");
			continue;
		}
		name = row->field_string("name");
		if (name == NULL)
		{
			logger_error("invalid name");
			continue;
		}
		type = row->field_int("type", -1);
		if (type < NAME_TYPE_DB || type > NAME_TYPE_IDX)
		{
			logger_error("invalid type");
			continue;
		}
		add_name(name, id, (name_type_t) type);
	}

	ctl_conn_->free_result();
	return (int) i;
}

int db_ctl::load_idx_hosts(void)
{
	const char* sql = "select id, addr, count from tbl_host_idx";
	if (ctl_conn_->sql_select(sql) == false)
	{
		logger_error("sql(%s) error", sql);
		return -1;
	}

	const std::vector<acl::db_row*>* rows = ctl_conn_->get_rows();
	if (rows == NULL)
	{
		logger_warn("tbl_host_idx empty");
		return 0;
	}

	unsigned int id;
	const char* addr;
	acl_int64 count;
	size_t i;
	for (i = 0; i < rows->size(); i++)
	{
		const acl::db_row* row = (*rows)[i];
		if ((id = row->field_int("id", -1)) == (unsigned int) -1)
		{
			logger_error("id(-1) invalid");
			continue;
		}
		else if ((addr = row->field_string("addr")) == NULL)
		{
			logger_error("name null");
			continue;
		}
		else if ((count = row->field_int64("count", -1)) == -1)
		{
			logger_error("count invalid");
			continue;
		}
		idx_host* host = new idx_host(id, addr, count);
		idx_hosts_.push_back(host);
	}
	ctl_conn_->free_result();
	return (int) i;
}

int db_ctl::load_dat_hosts(void)
{
	const char* sql = "select id, addr, count, priority from tbl_host_dat";
	if (ctl_conn_->sql_select(sql) == false)
	{
		logger_error("sql(%s) invalid", sql);
		return -1;
	}

	const std::vector<acl::db_row*>* rows = ctl_conn_->get_rows();
	if (rows == NULL)
	{
		logger_warn("tbl_host_dat empty");
		return 0;
	}

	unsigned int id;
	const char* addr;
	acl_int64 count;
	int   priority;

	size_t i;
	for (i = 0; i < rows->size(); i++)
	{
		const acl::db_row* row = (*rows)[i];
		if ((id = row->field_int("id", -1)) == (unsigned int) -1)
		{
			logger_warn("id(-1) invalid");
			continue;
		}
		else if ((addr = row->field_string("addr")) == NULL)
		{
			logger_warn("addr null");
			continue;
		}
		else if ((count = row->field_int64("count", -1)) == -1)
		{
			logger_warn("count(-1) invalid");
			continue;
		}
		priority = row->field_int("priority", 0);
		dat_host* host = new dat_host(id, addr, count, priority);
		dat_hosts_.push_back(host);
	}

	ctl_conn_->free_result();
	return (int) i;
}

int db_ctl::load_db_hosts(void)
{
	const char* sql = "select id_db, id_idx_host, count from tbl_db_host";
	if ((ctl_conn_->sql_select(sql)) == false)
	{
		logger_error("sql(%s) error", sql);
		return -1;
	}

	const std::vector<acl::db_row*>* rows = ctl_conn_->get_rows();
	if (rows == NULL)
	{
		logger_warn("tbl_db_host empty");
		return 0;
	}

	unsigned int id_db, id_idx_host;
	acl_int64 count;

	size_t i;
	for (i = 0; i < rows->size(); i++)
	{
		const acl::db_row* row = (*rows)[i];
		if ((id_db = row->field_int("id_db", -1)) == (unsigned int) -1)
		{
			logger_warn("id_db(-1) invalid");
			continue;
		}
		else if ((id_idx_host = row->field_int("id_idx_host", -1))
			== (unsigned int) -1)
		{
			logger_warn("id_idx_host invalid");
			continue;
		}
		else if ((count = row->field_int64("count", -1)) == -1)
		{
			logger_warn("count invalid");
			continue;
		}
		DB_HOST* host = (DB_HOST*) acl_mymalloc(sizeof(DB_HOST));
		host->id_db = id_db;
		host->id_idx_host = id_idx_host;
		host->count = count;
		db_hosts_.push_back(host);
	}

	ctl_conn_->free_result();
	return (int) i;
}

int db_ctl::load_db_tbls(void)
{
	const char* sql = "select id_db, id_tbl, count from tbl_db_tbl";
	if ((ctl_conn_->sql_select(sql)) == false)
	{
		logger_error("sql(%s) error", sql);
		return -1;
	}

	const std::vector<acl::db_row*>* rows = ctl_conn_->get_rows();
	if (rows == NULL)
	{
		logger_warn("tbl_db_tbl empty");
		return 0;
	}

	unsigned int id_db, id_tbl;
	acl_int64 count;

	size_t i;
	for (i = 0; i < rows->size(); i++)
	{
		const acl::db_row* row = (*rows)[i];
		if ((id_db = row->field_int("id_db", -1)) == (unsigned int) -1)
		{
			logger_warn("id_db invalid");
			continue;
		}
		else if ((id_tbl = row->field_int("id_tbl", -1)) ==
			(unsigned int) -1)
		{
			logger_warn("id_tbl invalid");
			continue;
		}
		else if ((count = row->field_int64("count", -1)) == -1)
		{
			logger_warn("count invalid");
			continue;
		}
		add_tbl(id_db, id_tbl, count);
	}

	ctl_conn_->free_result();
	return (int) i;
}

int db_ctl::load_tbl_idxes(void)
{
	const char* sql = "select id_idx, id_db, id_tbl, unique, type from tbl_tbl_idx";
	if ((ctl_conn_->sql_select(sql)) == false)
	{
		logger_error("sql(%s) error", sql);
		return -1;
	}

	const std::vector<acl::db_row*>* rows = ctl_conn_->get_rows();
	if (rows == NULL)
	{
		logger_warn("tbl_tbl_idx empty");
		return 0;
	}

	unsigned int id_idx, id_db, id_tbl, unique, type;

	size_t i;
	for (i = 0; i < rows->size(); i++)
	{
		const acl::db_row* row = (*rows)[i];
		if ((id_idx = row->field_int("id_idx", -1)) ==
			(unsigned int) -1)
		{
			logger_warn("id_idx invalid");
			continue;
		}
		else if ((id_db = row->field_int("id_db", -1)) ==
			(unsigned int) -1)
		{
			logger_warn("id_db invalid");
			continue;
		}
		else if ((id_tbl = row->field_int("id_tbl", -1)) ==
			(unsigned int) -1)
		{
			logger_warn("id_tbl invalid");
			continue;
		}
		type = row->field_int("type", -1);
		if (type < IDX_TYPE_STR || type > IDX_TYPE_INT64)
		{
			logger_warn("type(%d) invalid", type);
			continue;
		}

		unique = row->field_int("unique");
		TBL_IDX* idx = (TBL_IDX*) acl_mymalloc(sizeof(TBL_IDX));
		idx->id_idx = id_idx;
		idx->id_db = id_db;
		idx->id_tbl = id_tbl;
		idx->unique = unique ? true : false;
		idx->type = (idx_type_t) type;
		tbl_idxes_.push_back(idx);
	}

	ctl_conn_->free_result();
	return (int) i;
}

database* db_ctl::get_db(unsigned int id) const
{
	std::map<std::string, database*>::const_iterator it = dbs_.begin();
	for (; it != dbs_.end(); it++)
	{
		if (it->second->get_id() == id)
			return it->second;
	}
	return NULL;
}

idx_host* db_ctl::get_idx_host(unsigned int id) const
{
	std::vector<idx_host*>::const_iterator it = idx_hosts_.begin();
	for (; it != idx_hosts_.end(); it++)
	{
		if (id == (*it)->get_id())
			return *it;
	}
	return NULL;
}

void db_ctl::build_db(void)
{
	std::list<NAME_TYPE*>::iterator name_type_it = names_.begin();
	// 先从中提取出数据库标识
	for (; name_type_it != names_.end(); name_type_it++)
	{
		if ((*name_type_it)->type != NAME_TYPE_DB)
			continue;
		database* db = new database((*name_type_it)->name,
			(*name_type_it)->id);
		dbs_[db->get_name()] = db;
	}

	// 取出所有的数据表对象，并将之加入对应的数据库对象中
	std::list<DB_TBL*>::iterator tbl_it = db_tbls_.begin();
	for (; tbl_it != db_tbls_.end(); tbl_it++)
	{
		database* db = get_db((*tbl_it)->id_db);
		if (db != NULL)
			add_tbl(db, *tbl_it);
		else
			logger_warn("tbl's(%d) db(%d) no exist",
				(*tbl_it)->id_tbl, (*tbl_it)->id_db);
	}

	// 取出所有的将索引服务器对象，并将之加入对应的数据库对象中
	std::list<DB_HOST*>::iterator host_it = db_hosts_.begin();
	for (; host_it != db_hosts_.end(); host_it++)
	{
		idx_host* host = get_idx_host((*host_it)->id_idx_host);
		if (host == NULL)
		{
			logger_warn("idx_host(%d) not exist",
				(*host_it)->id_idx_host);
			continue;
		}

		database* db = get_db(host->get_id());
		if (db == NULL)
		{
			logger_warn("idx_host(%d)'s db no exist",
				host->get_id());
			continue;
		}
		db->add_idx_host(host);
	}
}

NAME_TYPE* db_ctl::get_name(unsigned int id, name_type_t type) const
{
	std::list<NAME_TYPE*>::const_iterator it = names_.begin();
	for (; it != names_.end(); it++)
	{
		if ((*it)->id == id && (*it)->type == type)
			return *it;
	}

	return NULL;
}

void db_ctl::add_name(const char* name, unsigned int id, name_type_t type)
{
	if (get_name(id, type) != NULL)
		return;
	NAME_TYPE* nt = (NAME_TYPE*) acl_mycalloc(1, sizeof(NAME_TYPE));
	snprintf(nt->name, sizeof(nt->name), "%s", name);
	nt->id = id;
	nt->type = type;
}

void db_ctl::add_tbl(database* db, DB_TBL* dbTbl)
{
	// 需要检查该表对象是否与命名表中的记录一致
	NAME_TYPE* name_type = NULL;
	std::list<NAME_TYPE*>::iterator name_type_it = names_.begin();
	for (; name_type_it != names_.end(); name_type_it++)
	{
		if ((*name_type_it)->id == dbTbl->id_tbl &&
			(*name_type_it)->type == NAME_TYPE_TBL)
		{
			name_type = *name_type_it;
			break;
		}
	}
	if (!name_type)
		return;

	db_tbl* tbl = new db_tbl(db, name_type->name, name_type->id);
	db->add_tbl(tbl);

	// 添加所有的表索引对象至所属的表对象中
	std::list<TBL_IDX*>::iterator idx_it = tbl_idxes_.begin();
	for (; idx_it != tbl_idxes_.end(); idx_it++)
	{
		if ((*idx_it)->id_tbl != tbl->get_id() &&
			(*idx_it)->id_db != db->get_id())
		{
			continue;
		}

		NAME_TYPE* nt = get_name((*idx_it)->id_idx, NAME_TYPE_IDX);
		if (nt == NULL)
			logger_warn("id_idx: %d invalid", (*idx_it)->id_idx);
		else
		{
			db_idx* idx = new db_idx(tbl, nt->name,
				nt->id, (*idx_it)->type);
			tbl->add_idx(idx);  // 添加索引对象至数据表中
		}
	}
}

void db_ctl::add_tbl(unsigned int id_db, unsigned int id_tbl,
	long long int count)
{
	DB_TBL* tbl = (DB_TBL*) acl_mymalloc(sizeof(DB_TBL));
	tbl->id_db = id_db;
	tbl->id_tbl = id_tbl;
	tbl->count = count;
	db_tbls_.push_back(tbl);
}
