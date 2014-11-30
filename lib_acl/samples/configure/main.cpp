#include "lib_acl.h"
#include <assert.h>
#include <errno.h>
#include <string.h>

static int var_a_count;
static int var_b_count;
static int var_c_count;

static char *var_a_str;
static char *var_b_str;
static char *var_c_str;
static char *var_event_mode;

static int var_a_bool;
static int var_b_bool;
static int var_c_bool;

static ACL_CFG_INT_TABLE __conf_int_tab[] = {
	{ "a_count", 1, &var_a_count, 0, 0 },
	{ "b_count", 1, &var_b_count, 0, 0 },
	{ "c_count", 1, &var_c_count, 0, 0 },
	{ 0, 0, 0, 0, 0 },
};

static ACL_CFG_STR_TABLE __conf_str_tab[] = {
	{ "a_str", "testa", &var_a_str },
	{ "b_str", "testb", &var_b_str },
	{ "c_str", "testc", &var_c_str },
	{ "aio_event_mode", "select", &var_event_mode },
	{ 0, 0, 0 },
};

static ACL_CFG_BOOL_TABLE __conf_bool_tab[] = {
	{ "a_bool", 0, &var_a_bool },
	{ "b_bool", 0, &var_b_bool },
	{ "c_bool", 0, &var_c_bool },
	{ 0, 0, 0 },
};

static void list_all(const ACL_XINETD_CFG_PARSER *cfg)
{
	int   i, n;
	char *name, *value;

	n = acl_xinetd_cfg_size(cfg);
	for (i = 0; i < n; i++) {
		if (acl_xinetd_cfg_index(cfg, i, &name, &value) < 0)
			break;
		printf("%s = %s\r\n", name, value);
	}
}

static void check_cf(const char *filename)
{
	ACL_XINETD_CFG_PARSER *cfg;

	cfg = acl_xinetd_cfg_load(filename);
	assert(cfg);

	acl_xinetd_params_int_table(cfg, __conf_int_tab);
	acl_xinetd_params_str_table(cfg, __conf_str_tab);
	acl_xinetd_params_bool_table(cfg, __conf_bool_tab);

	printf("var_a_str=%s\r\n", var_a_str);
	printf("var_b_str=%s\r\n", var_b_str);
	printf("var_c_str=%s\r\n", var_c_str);
	printf("var_a_count=%d\r\n", var_a_count);
	printf("var_b_count=%d\r\n", var_b_count);
	printf("var_c_count=%d\r\n", var_c_count);
	printf("var_a_bool=%d\r\n", var_a_bool);
	printf("var_b_bool=%d\r\n", var_b_bool);
	printf("var_c_bool=%d\r\n", var_c_bool);

	printf("var_event_mode=%s\r\n", var_event_mode);

	list_all(cfg);

	for (int i = 0; __conf_str_tab[i].name != NULL; i++)
	{
		if (*__conf_str_tab[i].target)
			acl_myfree(*__conf_str_tab[i].target);
	}	

	acl_xinetd_cfg_free(cfg);

}

static void list_cf(const char *filename)
{
	ACL_XINETD_CFG_PARSER *cfg;

	cfg = acl_xinetd_cfg_load(filename);
	if (cfg == NULL)
		printf("error=%s\r\n", strerror(errno));
	assert(cfg);
	list_all(cfg);
	acl_xinetd_cfg_free(cfg);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -l [list] -f conf_file\r\n", procname);
}

int main(int argc, char *argv[])
{
	char  ch;
	char  cf[256];
	int   l = 0;

	cf[0] = 0;

	while ((ch = getopt(argc, argv, "hlf:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 'l':
			l = 1;
			break;
		case 'f':
			ACL_SAFE_STRNCPY(cf, optarg, sizeof(cf));
			break;
		default:
			break;
		}
	}

	if (cf[0] == 0) {
		usage(argv[0]);
		return (0);
	}

	if (l)
		list_cf(cf);
	else
		check_cf(cf);

	return (0);
}
