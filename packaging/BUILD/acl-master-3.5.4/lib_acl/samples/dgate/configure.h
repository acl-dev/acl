#ifndef __CONFIGURE_INCLUDE_H__
#define __CONFIGURE_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DOMAIN_MAP {
	char  domain[256];
	ACL_ARGV *ip_list;
	int   idx;
} DOMAIN_MAP;

extern char *var_cfg_allow_ip;
extern char *var_cfg_domains;
extern char *var_cfg_domain_unknown;
extern char *var_cfg_dns_name;
extern char *var_cfg_dns_ip;
extern char *var_cfg_dns_neighbor_ip;
extern int   var_cfg_server_port;
extern int   var_cfg_hijack_unknown;
extern int   var_cfg_dns_neighbor_port;

void conf_load(const char *filepath);
int host_allow(const char *ip);
DOMAIN_MAP *domain_map_find(const char *domain);
DOMAIN_MAP *domain_map_unknown(void);

#ifdef __cplusplus
}
#endif

#endif
