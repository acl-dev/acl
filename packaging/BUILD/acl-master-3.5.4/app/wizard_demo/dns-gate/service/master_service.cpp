#include "stdafx.h"
#include "dgate_db.h"
#include "dgate_service.h"
#include "black_list.h"
#include "rules/rules_option.h"
#include "master_service.h"

//////////////////////////////////////////////////////////////////////////////
// ÅäÖÃÄÚÈÝÏî

char* var_cfg_upstream_addr;
char* var_cfg_display_disabled;
char* var_cfg_redis_addr;
char* var_cfg_redis_pass;
char* var_cfg_sqlite_path;
char* var_cfg_dbfile;
char* var_cfg_black_list;
char* var_cfg_rules_file;
char* var_cfg_manage_addr;
acl::master_str_tbl var_conf_str_tab[] = {
	{ "upstream_addr", "114.114.114.114|53", &var_cfg_upstream_addr },
	{ "display_disabled", "", &var_cfg_display_disabled },
	{ "redis_addr", "", &var_cfg_redis_addr },
	{ "redis_pass", "", &var_cfg_redis_pass },
	{ "sqlite_path", "", &var_cfg_sqlite_path },
	{ "dbfile", "", &var_cfg_dbfile },
	{ "black_list", "", &var_cfg_black_list },
	{ "rules_file", "", &var_cfg_rules_file },
	{ "manage_addr", "0.0.0.0:8688", &var_cfg_manage_addr },

	{ 0, 0, 0 }
};

acl::master_bool_tbl var_conf_bool_tab[] = {

	{ 0, 0, 0 }
};

acl::master_int_tbl var_conf_int_tab[] = {

	{ 0, 0 , 0 , 0, 0 }
};

acl::master_int64_tbl var_conf_int64_tab[] = {

	{ 0, 0 , 0 , 0, 0 }
};

acl::token_tree var_display_disabled;
acl::redis_client_cluster* var_redis_conns = NULL;
dgate_db* var_db = NULL;
black_list* var_black_list;
rules_option* var_rules_option;

//////////////////////////////////////////////////////////////////////////////

master_service::master_service(void)
{
	var_black_list = new black_list;
	var_rules_option = new rules_option;
}

master_service::~master_service(void)
{
	delete var_black_list;
	delete var_rules_option;
}

void master_service::on_read(acl::socket_stream* stream)
{
	int   n;
	char  buf[4096];

	if ((n = stream->read(buf, sizeof(buf), false)) == -1) {
		return;
	}

	dgate_push_request(var_db, stream, stream->get_peer(true), buf, n);
	//logger("read from %s, %d bytes", stream->get_peer(true), n);
}

void master_service::thread_on_init(void)
{
	logger(">>thread_on_init<<<");
}

void master_service::proc_on_bind(acl::socket_stream&)
{
}

void master_service::proc_on_init(void)
{
	if (var_cfg_display_disabled && *var_cfg_display_disabled) {
		acl::string buf(var_cfg_display_disabled);
		buf.lower();
		const std::vector<acl::string>& tokens = buf.split2(",; \t\r\n");
		for (std::vector<acl::string>::const_iterator cit = tokens.begin();
			cit != tokens.end(); ++cit) {
			logger("Not show: %s", (*cit).c_str());
			var_display_disabled.insert(*cit);
		}
	}

	if (var_cfg_redis_addr && *var_cfg_redis_addr) {
		var_redis_conns = new acl::redis_client_cluster;
		var_redis_conns->set(var_cfg_redis_addr, 0, 10, 10);
		if (var_cfg_redis_pass && *var_cfg_redis_pass) {
			var_redis_conns->set_password("default", var_cfg_redis_pass);
		}
	}

	if (var_cfg_black_list && *var_cfg_black_list) {
		var_black_list->add_list(var_cfg_black_list);
	}

	if (var_cfg_rules_file && *var_cfg_rules_file) {
		var_rules_option->load(var_cfg_rules_file);
	}

	logger("rules file=%s", var_cfg_rules_file);

	open_db();
	dgate_service_start();
}

void master_service::open_db(void)
{
	if (var_cfg_sqlite_path == NULL || *var_cfg_sqlite_path == 0) {
		return;
	}
	if (var_cfg_dbfile == NULL || *var_cfg_dbfile == 0) {
		return;
	}

	acl::db_handle::set_loadpath(var_cfg_sqlite_path);
	var_db = new dgate_db(var_cfg_dbfile);
	if (!var_db->open()) {
		logger_error("opend db=%s error", var_cfg_dbfile);
		delete var_db;
		var_db = NULL;
	} else {
		logger("opend db=%s ok", var_cfg_dbfile);
	}
}

void master_service::proc_on_exit(void)
{
	logger(">>>proc_on_exit<<<");
	delete var_redis_conns;
}

bool master_service::proc_on_sighup(acl::string&)
{
	logger(">>>proc_on_sighup<<<");
	return true;
}
