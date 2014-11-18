#include "lib_acl.h"
#include "service_var.h"

char *var_cfg_debug_msg;
char *var_cfg_smtpd_addr;
char *var_cfg_mail_from;
char *var_cfg_mail_to;
char *var_cfg_mail_cc;
char *var_cfg_mail_bcc;
char *var_cfg_auth_user;
char *var_cfg_auth_pass;
char *var_cfg_warn_mail;
char *var_cfg_smtp_helo;
char *var_cfg_sms_addr;
char *var_cfg_host_ip;

ACL_CFG_STR_TABLE var_conf_str_tab[] = {
	{ "debug_msg", "test_msg", &var_cfg_debug_msg },

	/* 邮件通知相关 */

	{ "smtpd_addr", "mail.inc365.com:25", &var_cfg_smtpd_addr },
	{ "mail_from", "acl_notify@acl_master.com", &var_cfg_mail_from },
	{ "mail_to", "", &var_cfg_mail_to },
	{ "mail_cc", "", &var_cfg_mail_cc },
	{ "mail_bcc", "", &var_cfg_mail_bcc },
	{ "auth_user", "", &var_cfg_auth_user },
	{ "auth_pass", "", &var_cfg_auth_pass },
	{ "warn_mail", "warning_letter.tmpl", &var_cfg_warn_mail },
	{ "stmp_helo", "test.com", &var_cfg_smtp_helo },

	{ "sms_addr", "", &var_cfg_sms_addr },
	{ "host_ip", "", &var_cfg_host_ip },

	{ 0, 0, 0 }
};

int  var_cfg_debug_enable;
int  var_cfg_keep_alive;

ACL_CFG_BOOL_TABLE var_conf_bool_tab[] = {
	{ "debug_enable", 1, &var_cfg_debug_enable },
	{ "keep_alive", 1, &var_cfg_keep_alive },

	{ 0, 0, 0 }
};

int  var_cfg_io_timeout;
int  var_cfg_smtp_notify_cache_timeout;
int  var_cfg_sms_notify_cache_timeout;
int  var_cfg_work_week_min;
int  var_cfg_work_week_max;
int  var_cfg_work_hour_min;
int  var_cfg_work_hour_max;

ACL_CFG_INT_TABLE var_conf_int_tab[] = {
	{ "io_timeout", 120, &var_cfg_io_timeout, 0, 0 },
	{ "smtp_notify_cache_timeout", 60, &var_cfg_smtp_notify_cache_timeout, 0, 0 },
	{ "sms_notify_cache_timeout", 600, &var_cfg_sms_notify_cache_timeout, 0, 0 },
	{ "work_week_min", 1, &var_cfg_work_week_min, 0, 0 },
	{ "work_week_max", 1, &var_cfg_work_week_max, 0, 0 },
	{ "work_hour_min", 9, &var_cfg_work_hour_min, 0, 0 },
	{ "work_hour_max", 18, &var_cfg_work_hour_max, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

ACL_ARGV *var_recipients = NULL;
ACL_ARGV *var_ccs = NULL;
ACL_ARGV *var_bccs = NULL;

void service_var_init()
{
	var_recipients = acl_argv_split(var_cfg_mail_to, ",; \t");
	var_ccs = acl_argv_split(var_cfg_mail_cc, ",; \t");
	var_bccs = acl_argv_split(var_cfg_mail_bcc, ",; \t");
}

void service_var_end()
{
	if (var_recipients)
		acl_argv_free(var_recipients);
}
