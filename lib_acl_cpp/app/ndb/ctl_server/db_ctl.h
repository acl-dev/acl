#pragma once
#include "db.h"

class acl::db_handle;  // db_handle.hpp
class acl::locker;  // locker.hpp

class db_driver;
class database;
class idx_host;
class dat_host;

struct NAME_TYPE;
struct DB_HOST;
struct DB_TBL;
struct TBL_IDX;

typedef enum
{
	NAME_TYPE_DB = 0,
	NAME_TYPE_TBL = 1,
	NAME_TYPE_IDX = 2
} name_type_t;

enum
{
	DB_CTL_OK = 0,
	DB_CTL_ERR = 1,
};

class db_ctl
{
public:
	db_ctl(void);
	~db_ctl(void);

	/**
	 * 加载数据库信息
	 */
	void load();

	/**
	 * 打开指定数据库中的数据库对象，如果数据库不存在，返回NULL
	 * @param dbname {const char*} 数据库名称
	 * @param dbuser {const char*} 用户名
	 * @param dbpass {const char*} 用户密码
	 * @return {database*} 数据库对象，如果返回 NULL 则表示失败
	 */
	database* db_open(const char* dbname, const char* dbuser = NULL,
		const char* dbpass = NULL);

	/**
	 * 创建数据库
	 * @param dbname {const char*} 数据库名称
	 * @param dbuser {const char*} 用户名
	 * @param dbpass {const char*} 用户密码
	 * @return {database*} 数据库对象，如果返回 NULL 则表示失败
	 */
	database* db_create(const char* dbname, const char* dbuser = NULL,
		const char* dbpass = NULL);

	/**
	 * 关闭数据库
	 * @param {database*} db
	 */
	void db_close(database* db);

	/**
	 * 向数据库中添加数据表
	 * @param db {database*} 数据库名
	 * @param tbl_name {const char*} 数据表名
	 * @return {db_tbl*} 非空值表示添加成功
	 */
	db_tbl* db_add_tbl(database* db, const char* tbl_name);

	/**
	 * 向数据表中添加索引项
	 * @param tbl {db_tbl*} 数据表对象
	 * @param tbl_idx {const char*} 数据表索引名
	 * @param idx_type {idx_type_t} 表索引的字段类型
	 * @return {db_idx*} 非空值表示添加成功
	 */
	db_idx* db_add_idx(db_tbl* tbl, const char* tbl_idx, idx_type_t idx_type);

	/**
	 * 向数据表中添加索引项
	 * @param db {database*} 数据库对象
	 * @param tbl_name {const char*} 数据表名
	 * @param tbl_idx {const char*} 数据表索引名
	 * @param idx_type {idx_type_t} 表索引的字段类型
	 * @return {db_idx*} 非空值表示添加成功
	 */
	db_idx* db_add_idx(database* db, const char* tbl_name,
		const char* tbl_idx, idx_type_t idx_type);

protected:
	/**
	 * 设置索引数据库的主机至对应的数据库对象中，并添加进数据库中
	 * @param db {database*}
	 * @return {bool}
	 */
	bool db_host_set(database* db);
private:
	int  errnum_;
	db_driver* driver_;
	acl::locker* lock_;  // db_ctl 对象的互斥锁
	acl::locker* ctl_conn_lock_; // ctl_conn_ 数据库连接的互斥锁
	acl::db_handle* ctl_conn_;  // 控制数据库的连接
	std::map<std::string, database*> dbs_;  // 数据库列表
	std::vector<idx_host*> idx_hosts_;  // 索引服务器列表
	std::vector<dat_host*> dat_hosts_;  // 数据服务器列表

	std::list<NAME_TYPE*> names_;
	std::list<DB_HOST*> db_hosts_;
	std::list<DB_TBL*> db_tbls_;
	std::list<TBL_IDX*> tbl_idxes_;

	// 加载表 tbl_name_type
	int load_names(void);

	// 加载表 tbl_idx_host
	int load_idx_hosts(void);

	// 加载表 tbl_dat_host
	int load_dat_hosts(void);

	// 加载表 tbl_db_host
	int load_db_hosts(void);

	// 加载表 tbl_db_tbl
	int load_db_tbls(void);

	// 加载表 tbl_tbl_idx
	int load_tbl_idxes(void);

	// 从 NAME_TYPE 的集合中根据ID号及类型获得指定的 NAME_TYPE 结构对象
	NAME_TYPE* get_name(unsigned int id, name_type_t type) const;

	// 添加 NAME_TYPE对象至 names_ 集合中
	void add_name(const char* name, unsigned int id, name_type_t type);

	// 添加新的名字记录至数据库，并返回其ID号
	unsigned int db_add_name(const char* name, name_type_t type);

	// 构建数据表关联信息
	void build_db(void);

	// 从数据库集合中根据ID号获得指定的数据库对象
	database* get_db(unsigned int id) const;

	// 从索引服务器集合中取得对应ID号的索引服务器对象
	idx_host* get_idx_host(unsigned int id) const;

	// 添加表结构对象至数据库对象中
	void add_tbl(database* db, DB_TBL* tbl);

	// 添加新的表记录对象至 db_tbls_ 集合中
	void add_tbl(unsigned int id_db, unsigned int id_tbl,
		long long int count);
};
