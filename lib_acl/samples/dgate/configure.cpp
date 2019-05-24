#include "stdafx.h"
#include <string.h>
#include "configure.h"

char *var_cfg_allow_ip;		// 允许访问的 IP 地址范围
char *var_cfg_domains;		// 本地域名映射关系
char *var_cfg_domain_unknown;	// 当域名不存在时返回给客户端的其它替换地址
char *var_cfg_dns_name;		// 本域名服务器的名称
char *var_cfg_dns_ip;		// 本域名服务器的 IP
char *var_cfg_dns_neighbor_ip;	// 上游 DNS 服务器 IP

static ACL_CFG_STR_TABLE __conf_str_tab[] = {
	{ "allow_ip", "0.0.0.0:255.255.255.254", &var_cfg_allow_ip },
	{ "domain_map", "", &var_cfg_domains },
	{ "domain_unknown", "211.157.110.179:211.157.110.185", &var_cfg_domain_unknown },
	{ "dns_name", "mytest.com.cn", &var_cfg_dns_name },
	{ "dns_ip", "211.157.110.179", &var_cfg_dns_ip },
	{ "dns_neighbor_ip", "8.8.8.8", &var_cfg_dns_neighbor_ip },
	{ 0, 0, 0 },
};

int   var_cfg_hijack_unknown;	// 是否需要对不存在域名进行劫持

static ACL_CFG_BOOL_TABLE __conf_bool_tab[] = {
	{ "hijack_unknown", 0, &var_cfg_hijack_unknown },
	{ 0, 0, 0 },
};

int   var_cfg_server_port;	// 本机绑定的端口号
int   var_cfg_dns_neighbor_port;// 上游 DNS 服务器 PORT

static ACL_CFG_INT_TABLE __conf_int_tab[] = {
	{ "server_port", 53, &var_cfg_server_port, 0, 0 },
	{ "dns_neighbor_port", 53, &var_cfg_dns_neighbor_port, 0, 0 },
	{ 0, 0, 0, 0, 0 }
};

static ACL_IPLINK *__allow_ip = NULL;
static ACL_HTABLE *__domain_map_table = NULL;
static DOMAIN_MAP *__domain_unknown = NULL;
static int   __all_to_one_ip = 0;

static void conf_allow_ip(const char *ip_list)
{
	const char *myname = "conf_allow_ip";
	ACL_ARGV *argv;
	char *ipfrom, *ipto;
	int   i;

	__allow_ip = acl_iplink_create(10);
	argv = acl_argv_split(ip_list, ",; \t");
	for (i = 0; i < argv->argc; i++) {
		ipfrom = argv->argv[i];
		ipto = strchr(ipfrom, ':');
		if (ipto == NULL) {
			acl_msg_warn("%s(%d): ip_list(%s) invalid",
				myname, __LINE__, ip_list);
			continue;
		}
		*ipto++ = 0;
		acl_iplink_insert(__allow_ip, ipfrom, ipto);
	}
	acl_argv_free(argv);
}

static void domain_map_add_unknown(void)
{
	const char *myname = "domain_map_add_unknown";

	__domain_unknown = (DOMAIN_MAP*) acl_mycalloc(1, sizeof(DOMAIN_MAP));
	__domain_unknown->idx = 0;
	ACL_SAFE_STRNCPY(__domain_unknown->domain, "unknown",
		sizeof(__domain_unknown->domain));
	__domain_unknown->ip_list = acl_argv_split(var_cfg_domain_unknown, ":");
	if (__domain_unknown->ip_list == NULL)
		acl_msg_fatal("%s(%d): domain_unknown(%s) invalid",
			myname, __LINE__, var_cfg_domain_unknown);
}

static void domain_map_add(const ACL_ARGV *ip_list)
{
	const char *myname = "domain_map_add";
	DOMAIN_MAP *domain_map;
	int   i;

	if (ip_list->argc < 2)
		acl_msg_fatal("%s(%d): ip_list->argc(%d) < 2",
			myname, __LINE__, ip_list->argc);

	domain_map = (DOMAIN_MAP*) acl_mycalloc(1, sizeof(DOMAIN_MAP));
	domain_map->idx = 0;
	ACL_SAFE_STRNCPY(domain_map->domain, ip_list->argv[0],
		sizeof(domain_map->domain));
	acl_lowercase(domain_map->domain);

	domain_map->ip_list = acl_argv_alloc(10);
	for (i = 1; i < ip_list->argc; i++)
		acl_argv_add(domain_map->ip_list, ip_list->argv[i], NULL);

	acl_htable_enter(__domain_map_table, domain_map->domain, domain_map);
	if (strcasecmp(domain_map->domain, "all") == 0)
		__all_to_one_ip = 1;
}

static void conf_domains_map(const char *domains)
{
	const char *myname = "conf_domains_map";
	ACL_ARGV *argv, *ip_list;
	int   i;

	if (domains == NULL || *domains == 0)
		return;
	argv = acl_argv_split(domains, ",; \t");

	for (i = 0; i < argv->argc; i++) {
		ip_list = acl_argv_split(argv->argv[i], ":");
		if (ip_list->argc < 2) {
			acl_msg_warn("%s(%d): invalid data(%s)",
				myname, __LINE__, argv->argv[i]);
			acl_argv_free(ip_list);
		} else {
			domain_map_add(ip_list);
			acl_argv_free(ip_list);
		}
	}

	acl_argv_free(argv);
}

void conf_load(const char *filepath)
{
	const char *myname = "conf_load";
	ACL_XINETD_CFG_PARSER *cfg;

	__domain_map_table = acl_htable_create(100, 0);

	cfg = acl_xinetd_cfg_load(filepath);
	if (cfg == NULL) {
		acl_msg_error("%s(%d): load confiugre(%s) error",
			myname, __LINE__, filepath);
		return;
	}

	acl_xinetd_params_bool_table(cfg, __conf_bool_tab);
	acl_xinetd_params_int_table(cfg, __conf_int_tab);
	acl_xinetd_params_str_table(cfg, __conf_str_tab);

	conf_allow_ip(var_cfg_allow_ip);
	domain_map_add_unknown();
	conf_domains_map(var_cfg_domains);

	acl_xinetd_cfg_free(cfg);
}

int host_allow(const char *ip)
{
	if (__allow_ip == NULL)
		return (1);
	if (acl_iplink_lookup_str(__allow_ip, ip) != NULL)
		return (1);
	return (0);
}

DOMAIN_MAP *domain_map_find(const char *domain)
{
	char  domain_buf[256];
	DOMAIN_MAP *domain_map;

	if (__all_to_one_ip)
		ACL_SAFE_STRNCPY(domain_buf, "all", sizeof(domain_buf));
	else
		ACL_SAFE_STRNCPY(domain_buf, domain, sizeof(domain_buf));

	acl_lowercase(domain_buf);

	domain_map = (DOMAIN_MAP*) acl_htable_find(
		__domain_map_table, domain_buf);
	return (domain_map);
}

DOMAIN_MAP *domain_map_unknown(void)
{
	const char *myname = "domain_map_unknown";

	if (__domain_unknown == NULL)
		acl_msg_fatal("%s(%d): __domain_unknown null",
			myname, __LINE__);

	return (__domain_unknown);
}
