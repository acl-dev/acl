#include "acl_cpp/lib_acl.hpp"
#include "lib_acl_cpp1z/db/db_mysql.hpp"


struct group_tbl
{
	std::string group_name;
	std::string uvip_tbl;
	std::string access_tbl;
	std::string access_week_tbl;
	std::string access_month_tbl;
	std::string update_date;
	int disable;
	int add_by_hand;
	int class_level;


};
REFLECTION(
	group_tbl,
	group_name, 
	uvip_tbl, 
	access_tbl, 
	access_week_tbl, 
	access_month_tbl, 
	update_date, 
	disable, 
	add_by_hand, 
	class_level);

void select_as_json(acl::lz::db_mysql &db, int n)
{
	acl::query query;
	query.create_sql("select * from group_tbl where group_name=:group"
		" and uvip_tbl=:test")
		.set_format("group", "group:%d", n)
		.set_format("test", "test");

	std::string json_str;
	if (db.select<group_tbl>(query.to_string().c_str(), json_str))
		std::cout << json_str << std::endl;
}

void select_as_obj(acl::lz::db_mysql &db, int n)
{
	acl::query query;
	query.create_sql("select * from group_tbl where group_name=:group"
		" and uvip_tbl=:test")
		.set_format("group", "group:%d", n)
		.set_format("test", "test");

	if (db.select<group_tbl>(query.to_string().c_str()).first)
		std::cout << "select ok" << std::endl;
	else
		std::cout << "select error" << std::endl;;
}

int main()
{
	acl::string path("F:\\fork\\acl-dev\\acl\\libmysql.dll");
	acl::db_handle::set_loadpath(path);

	acl::string dbaddr("192.168.3.170:3306");
	acl::string dbname("acl_db"), dbuser("root"), dbpass("skyinno251");
	int max = 200;
	acl::lz::db_mysql db(dbaddr, dbname, dbuser, dbpass, 0, false);

	if (db.open() == false)
	{
		printf("open db(%s@%s) error\r\n",
			dbaddr.c_str(), dbname.c_str());
		getchar();
		return 1;
	}

	group_tbl group_tbl_;
	group_tbl_.access_month_tbl = "access_month_tbl11";
	group_tbl_.access_tbl = "access_tbl 111";
	group_tbl_.access_week_tbl = "hello world";
	group_tbl_.add_by_hand = 1;
	group_tbl_.class_level = 1;
	group_tbl_.disable = 0;
	group_tbl_.group_name = "group_name";
	group_tbl_.update_date = "2017-03-14 09:55:00";
	group_tbl_.uvip_tbl = "uvip_tbl 1";

	db.insert(group_tbl_);

	for (size_t i = 0; i < max; i++)
	{
		select_as_json(db, i);
		select_as_obj(db, i);
	}
	return 0;
}