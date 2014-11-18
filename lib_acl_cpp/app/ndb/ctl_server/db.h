#pragma once

class acl::locker;
class db_tbl;
class idx_host;
class database;

// 索引字段的类型
typedef enum
{
	IDX_TYPE_STR = 0,	// 字符串类型
	IDX_TYPE_BOOL,		// 布尔类型
	IDX_TYPE_INT16,		// 16位整数
	IDX_TYPE_INT32,		// 32位整数
	IDX_TYPE_INT64		// 64位整数
} idx_type_t;

/**
 * 数据库表对象中的索引对象
 */
class db_idx
{
public:
	/**
	 * 表索引对象构造函数
	 * @param tbl {db_tbl*} 表对象
	 * @param name {const char*} 索引名称
	 * @param id {unsigned int} 索引对应的ID号
	 * @param type {idx_type_t} 索引字段的数据类型
	 */
	db_idx(db_tbl* tbl, const char* name, unsigned int id, idx_type_t type);
	~db_idx();

	const char* get_name() const
	{
		return name_;
	}

	unsigned int get_id() const
	{
		return id_;
	}

	db_tbl* get_tbl() const
	{
		return tbl_;
	}

	idx_type_t get_type() const
	{
		return type_;
	}
protected:
private:
	db_tbl* tbl_; // 所属的数据表对象
	char* name_;  // 索引名
	unsigned int id_;  // 索引ID号
	idx_type_t type_;  // 索引字段的数据类型
};

class db_tbl
{
public:
	/**
	 * 表对象构造函数
	 * @param db {database*} 数据库对象
	 * @param name {const char*} 数据表名
	 * @param id {unsigned int} 数据表对应的ID号
	 */
	db_tbl(database* db, const char* name, unsigned int id);
	~db_tbl();

	const char* get_name() const
	{
		return name_;
	}

	unsigned int get_id() const
	{
		return id_;
	}

	database* get_db() const
	{
		return db_;
	}

	void add_idx(db_idx* idx);
private:
	database* db_; // 所属的数据库对象
	char* name_;  // 数据表名
	unsigned int id_;  // 数据表的ID号
	std::list<db_idx*> idxes_;  // 表索引集合
};

class database
{
public:
	/**
	 * 数据库对象构造函数
	 * @param name {const char*} 数据库名称
	 * @param id {unsigned int} 数据库对应的ID号
	 */
	database(const char* name, unsigned int id);
	~database();

	const char* get_name() const
	{
		return name_;
	}

	unsigned int get_id() const
	{
		return id_;
	}

	void add_tbl(db_tbl*);
	void add_idx_host(idx_host* host);
protected:
private:
	char* name_;  // 数据库名
	unsigned int id_;  // 数据库的ID号
	std::map<std::string, db_tbl*> tables_;  // 数据表集合
	std::vector<idx_host*> idx_hosts_;  // 所对应的索引服务器的集合

	acl::locker* lock_;  // 数据库锁
};
