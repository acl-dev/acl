#include "stdafx.h"
#include "http_job.h"
#include "db_store.h"
#include "master_service.h"

////////////////////////////////////////////////////////////////////////////////
// 配置内容项

char* var_cfg_http_url_list;
char* var_cfg_dns_ip;
char* var_cfg_dbpath;
char* var_cfg_dbcharset;
acl::master_str_tbl var_conf_str_tab[] = {
	{ "http_url_list", "http://www.baidu.com/", &var_cfg_http_url_list },
	{ "dns_ip", "8.8.8.8", &var_cfg_dns_ip },
	{ "dbpath", "./check.db", &var_cfg_dbpath },
	{ "dbcharset", "utf-8", &var_cfg_dbcharset },

	{ 0, 0, 0 }
};

acl::master_bool_tbl var_conf_bool_tab[] = {

	{ 0, 0, 0 }
};

int   var_cfg_http_cocurrent;
int   var_cfg_dns_port;
int   var_cfg_conn_timeout;
int   var_cfg_rw_timeout;
acl::master_int_tbl var_conf_int_tab[] = {
	{ "http_cocurrent", 10, &var_cfg_http_cocurrent, 0, 0 },
	{ "dns_port", 53, &var_cfg_dns_port, 0, 0 },
	{ "conn_timeout", 60, &var_cfg_conn_timeout, 0, 0 },
	{ "rw_timeout", 60, &var_cfg_rw_timeout, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

acl::master_int64_tbl var_conf_int64_tab[] = {

	{ 0, 0 , 0 , 0, 0 }
};

////////////////////////////////////////////////////////////////////////////////

acl::thread_pool* var_thrpool;
acl::db_pool* var_dbpool;

master_service::master_service()
{
}

master_service::~master_service()
{
}

void master_service::on_trigger()
{
	std::vector<acl::string>::const_iterator cit = url_list_.begin();
	for (; cit != url_list_.end(); ++cit)
	{
		const char* url = (*cit).c_str();
		for (int i = 0; i < var_cfg_http_cocurrent; i++)
		{
			// 创建一个工作线程任务，并交由线程池去执行
			acl::thread_job* job = new http_job(*var_thrpool, url,
				var_cfg_dns_ip, var_cfg_dns_port);
			var_thrpool->execute(job);
		}
	}

	// 等待线程池结束
	var_thrpool->wait();
}

void master_service::proc_on_init()
{
	// 取得需要探测的 URL 列表
	acl::string buf(var_cfg_http_url_list);
	const std::list<acl::string>& tokens = buf.split(",; \t");
	std::list<acl::string>::const_iterator cit = tokens.begin();
	for (; cit != tokens.end(); ++cit)
	{
		url_list_.push_back(*cit);
		logger("add url: %s", (*cit).c_str());
	}

	// 创建数据库连接池
	var_dbpool = new acl::sqlite_pool(var_cfg_dbpath,
		var_cfg_http_cocurrent * url_list_.size());

	// 创建或打开数据库
	db_store store;
	if (store.db_create() == false)
		logger_fatal("create db failed!");

	// 创建线程池并设置参数
	var_thrpool = new acl::thread_pool;
	var_thrpool->set_limit(var_cfg_http_cocurrent);
	var_thrpool->set_idle(300);

	// 启动线程池过程
	var_thrpool->start();
}

void master_service::proc_on_exit()
{
	// 销毁线程池
	delete var_thrpool;

	// 销毁数据库连接池
	delete var_dbpool;
}
