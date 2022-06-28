#pragma once

extern char* var_cfg_mysql_dbaddr;
extern char* var_cfg_mysql_dbname;
extern char* var_cfg_mysql_dbuser;
extern char* var_cfg_mysql_dbpass;

extern int   var_cfg_mysql_dbpool_limit;
extern int   var_cfg_mysql_dbpool_timeout;
extern int   var_cfg_mysql_dbpool_dbping;

/**
 * 加载配置项
 * @param path {const char*} 配置文件路径
 */
bool db_conf_load(const char* path);

/**
 * 卸载配置内容项内存
 */
void db_conf_unload(void);
