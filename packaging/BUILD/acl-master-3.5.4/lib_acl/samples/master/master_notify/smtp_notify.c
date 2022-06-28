#include "lib_acl.h"
#include <string.h>
#include "lib_tpl.h"
#include "service_var.h"
#include "notify.h"

static char* get_warning_mail(const char *proc, ACL_ARGV *rcpts, int pid,
	const char *info, const char *ip)
{
	tpl_t *tpl;
	time_t now = time(NULL);
	struct tm local_time;
	char date[64], *buf;
	int  size;
	ACL_ITER iter;
	ACL_VSTRING *tos;

	/* 打开模板文件并创建模板对象 */
	tpl = tpl_alloc();
	if (tpl_load(tpl, var_cfg_warn_mail) != TPL_OK) {
		acl_msg_error("load %s error(%s)", var_cfg_warn_mail,
			acl_last_serror());
		return (NULL);
	}

	localtime_r(&now, &local_time);

	strftime(date, sizeof(date), "%Y/%m/%d %H:%M:%S", &local_time);

	tpl_set_field_fmt_global(tpl, "VAR_FROM", "%s", var_cfg_mail_from);

	tos = acl_vstring_alloc(256);
	acl_foreach(iter, var_recipients) {
		const char *ptr = (const char*) iter.data;

		if (ACL_VSTRING_LEN(tos) > 0)
			acl_vstring_strcat(tos, ",\r\n\t");
		acl_vstring_strcat(tos, ptr);
	}
	if (rcpts) {
		acl_foreach(iter, rcpts) {
			const char *ptr = (const char*) iter.data;

			if (ACL_VSTRING_LEN(tos) > 0)
				acl_vstring_strcat(tos, ",\r\n\t");
			acl_vstring_strcat(tos, ptr);
		}
	}
	tpl_set_field_fmt_global(tpl, "VAR_TOS", "%s", acl_vstring_str(tos));

	ACL_VSTRING_RESET(tos);
	acl_foreach(iter, var_ccs) {
		const char *ptr = (const char*) iter.data;

		if (ACL_VSTRING_LEN(tos) > 0)
			acl_vstring_strcat(tos, ",\r\n\t");
		acl_vstring_strcat(tos, ptr);
	}
	if (ACL_VSTRING_LEN(tos) > 0)
		tpl_set_field_fmt_global(tpl, "VAR_CCS",
				"Cc: %s\r\n", acl_vstring_str(tos));

	acl_vstring_free(tos);

	tpl_set_field_fmt_global(tpl, "VAR_ID", "%ld.%ld.localhost",
			acl_pthread_self(), getpid());
	tpl_set_field_fmt_global(tpl, "VAR_HOST", "%s", ip);
	tpl_set_field_fmt_global(tpl, "VAR_DATE", "%s", date);

	/* 进程相关信息 */
	tpl_set_field_fmt_global(tpl, "VAR_PROC", "%s", proc);
	tpl_set_field_fmt_global(tpl, "VAR_PID", "%d", pid);
	tpl_set_field_fmt_global(tpl, "VAR_INFO", "%s", info);

	size = tpl_length(tpl);
	if (size <= 0)
	{
		acl_msg_error("%s's size is 0!", var_cfg_warn_mail);
		return (NULL);
	}

	size += 1;

	buf = (char*) acl_mymalloc(size);
	tpl_get_content(tpl, buf);
	tpl_free(tpl);

	return (buf);
}

static int smtp_banner(ACL_VSTREAM *client)
{
	char  line[1024];
	int   ret;

	/* 读取欢迎信息 */
	ret = acl_vstream_gets_nonl(client, line, sizeof(line));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets banner error from %s",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr);
		return (-1);
	}

	return (0);
}

static int smtp_helo(ACL_VSTREAM *client)
{
	char  line[1024];
	int   ret;

	/* 发送 helo 信息 */
	ret = acl_vstream_fprintf(client, "helo %s\r\n", var_cfg_smtp_helo);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send helo to %s error(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr,
			acl_last_serror());
		return (-1);
	}

	/* 读取响应信息 */
	ret = acl_vstream_gets_nonl(client, line, sizeof(line));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets helo's reply from %s error(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr,
			acl_last_serror());
		return (-1);
	}

	return (0);
}

static int smtp_auth(ACL_VSTREAM *client)
{
	char  line[1024];
	int   ret;
	ACL_ARGV *tokens = NULL;
	char *user = NULL, *pass = NULL;

#undef	RETURN
#define	RETURN(x) do {  \
	if (tokens)  \
		acl_argv_free(tokens);  \
	if (user)  \
		acl_myfree(user);  \
	if (pass)  \
		acl_myfree(pass);  \
	return (x);  \
} while (0)

	/* 发送认证命令 */

	ret = acl_vstream_fprintf(client, "AUTH LOGIN\r\n");
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send AUTH LOGIN to %s error(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr,
			acl_last_serror());
		RETURN (-1);
	}

	ret = acl_vstream_gets_nonl(client, line, sizeof(line));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets reply for AUTH LOGIN from %s error(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr,
			acl_last_serror());
		RETURN (-1);
	}

	tokens = acl_argv_split(line, "\t ");
	if (strcmp(tokens->argv[0], "334") != 0) {
		acl_msg_error("%s(%d): AUTH LOGIN failed from %s, line(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr, line);
		RETURN (-1);
	}

	/* 发送邮箱帐号 */

	user = (char*) acl_base64_encode(var_cfg_auth_user, strlen(var_cfg_auth_user));
	ret = acl_vstream_fprintf(client, "%s\r\n", user);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send user to %s error(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr,
			acl_last_serror());
		RETURN (-1);
	}

	ret = acl_vstream_gets_nonl(client, line, sizeof(line));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets from %s error(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr,
			acl_last_serror());
		RETURN (-1);
	}
	acl_argv_free(tokens);
	tokens = acl_argv_split(line, "\t ");
	if (strcmp(tokens->argv[0], "334") != 0) {
		acl_msg_error("%s(%d): AUTH LOGIN failed from %s, line(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr, line);
		RETURN (-1);
	}

	/* 发送 password */

	pass = (char*) acl_base64_encode(var_cfg_auth_pass, strlen(var_cfg_auth_pass));
	ret = acl_vstream_fprintf(client, "%s\r\n", pass);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send pass to %s error(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr,
			acl_last_serror());
		RETURN (-1);
	}
	ret = acl_vstream_gets_nonl(client, line, sizeof(line));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets pass's reply from %s error(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr,
			acl_last_serror());
		RETURN (-1);
	}
	acl_argv_free(tokens);
	tokens = acl_argv_split(line, "\t ");
	if (strcmp(tokens->argv[0], "235") != 0) {
		acl_msg_error("%s(%d): AUTH LOGIN failed from %s, line(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr, line);
		RETURN (-1);
	}

	RETURN (0);
}

static int smtp_from(ACL_VSTREAM *client)
{
	char  line[1024];
	int   ret;
	ACL_ARGV *tokens;

	/* 发送 mail from 信息 */
	ret = acl_vstream_fprintf(client, "mail from: <%s>\r\n", var_cfg_mail_from);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send mail from to %s error(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr,
			acl_last_serror());
		return (-1);
	}

	/* 读取响应信息 */
	ret = acl_vstream_gets_nonl(client, line, sizeof(line));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets mail from's reply from %s error(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr,
			acl_last_serror());
		return (-1);
	}

	tokens = acl_argv_split(line, "\t ");
	if (strcmp(tokens->argv[0], "250") != 0) {
		acl_msg_error("%s(%d): denied by server(%s), line(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr, line);
		acl_argv_free(tokens);
		return (-1);
	}

	acl_argv_free(tokens);
	return (0);
}

static int smtp_to(ACL_VSTREAM *client, const char *to)
{
	char  line[1024];
	int   ret;
	ACL_ARGV *tokens;

	ret = acl_vstream_fprintf(client, "rcpt to: <%s>\r\n", to);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send rcpt to %s error(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr,
			acl_last_serror());
		return (-1);
	}

	ret = acl_vstream_gets_nonl(client, line, sizeof(line));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets rcpt's reply from %s error(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr,
			acl_last_serror());
		return (-1);
	}

	tokens = acl_argv_split(line, "\t ");
	if (strcmp(tokens->argv[0], "250") != 0) {
		acl_msg_error("%s(%d): rcpt to %s error, line(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr, line);
		acl_argv_free(tokens);
		return (-1);
	}

	acl_argv_free(tokens);
	acl_msg_info(">>> send to: %s ok!", to);
	return (0);
}

static int smtp_tos(ACL_VSTREAM *client, ACL_ARGV *rcpts)
{
	ACL_ITER iter;
	int   nok = 0, ret;

	acl_foreach(iter, var_recipients) {
		const char *to = (const char*) iter.data;

		ret = smtp_to(client, to);
		if (ret == 0)
			nok++;
		else if (ACL_IF_VSTREAM_ERR(client))
			return (-1);
	}
	acl_foreach(iter, var_ccs) {
		const char *to = (const char*) iter.data;

		ret = smtp_to(client, to);
		if (ret == 0)
			nok++;
		else if (ACL_IF_VSTREAM_ERR(client))
			return (-1);
	}
	acl_foreach(iter, var_bccs) {
		const char *to = (const char*) iter.data;

		ret = smtp_to(client, to);
		if (ret == 0)
			nok++;
		else if (ACL_IF_VSTREAM_ERR(client))
			return (-1);
	}

	if (rcpts == NULL)
		return (nok);

	acl_foreach(iter, rcpts) {
		const char *to = (const char*) iter.data;

		ret = smtp_to(client, to);
		if (ret == 0)
			nok++;
		else if (ACL_IF_VSTREAM_ERR(client))
			return (-1);
	}

	return (nok);
}

static int smtp_data(ACL_VSTREAM *client, const char *proc,
	ACL_ARGV *rcpts, int pid, const char *info, const char *ip)
{
	char  line[1024];
	ACL_ARGV *tokens;
	int   ret;
	char *buf;

	ret = acl_vstream_fprintf(client, "data\r\n");
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send data to %s error(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr,
			acl_last_serror());
		return (-1);
	}

	ret = acl_vstream_gets_nonl(client, line, sizeof(line));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets data's reply from %s error(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr,
			acl_last_serror());
		return (-1);
	}

	tokens = acl_argv_split(line, "\t ");
	if (strcmp(tokens->argv[0], "354") != 0) {
		acl_msg_error("%s(%d): denied from %s, line(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr, line);
		acl_argv_free(tokens);
		return (-1);
	}
	acl_argv_free(tokens);

	buf = get_warning_mail(proc, rcpts, pid, info, ip);
	if (buf == NULL) {
		acl_msg_error("%s(%d): get_warning_mail error",
			__FUNCTION__, __LINE__);
		return (-1);
	}
	ret = acl_vstream_writen(client, buf, strlen(buf));
	acl_myfree(buf);

	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send mail error(%s) to %s",
			__FUNCTION__, __LINE__, acl_last_serror(),
			var_cfg_smtpd_addr);
		return (-1);
	}
	ret = acl_vstream_fprintf(client, "\r\n.\r\n");
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send mail eof error(%s) to %s",
			__FUNCTION__, __LINE__, acl_last_serror(),
			var_cfg_smtpd_addr);
		return (-1);
	}
	
	ret = acl_vstream_gets(client, line, sizeof(line));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets mail eof's reply error(%s) from %s",
			__FUNCTION__, __LINE__, acl_last_serror(),
			var_cfg_smtpd_addr);
		return (-1);
	}

	tokens = acl_argv_split(line, "\t ");
	if (strcmp(tokens->argv[0], "250") != 0) {
		acl_msg_error("%s(%d): send mail error to %s, line: %s",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr, line);
		acl_argv_free(tokens);
		return (-1);
	}
	acl_argv_free(tokens);
	return (0);
}

static int smtp_sendmail(const char *proc, ACL_ARGV *rcpts,
	int pid, const char *info)
{
	ACL_VSTREAM *client = NULL;
	char  ip[64], *ptr;

#undef	RETURN
#define	RETURN(x) do {  \
	if (client)  \
		acl_vstream_close(client);  \
	return (x);  \
} while (0)

	client = acl_vstream_connect(var_cfg_smtpd_addr, ACL_BLOCKING,
			30, 30, 4096);
	if (client == NULL) {
		acl_msg_error("%s(%d): connect %s error(%s)",
			__FUNCTION__, __LINE__, var_cfg_smtpd_addr,
			acl_last_serror());
		RETURN (-1);
	}

	if (var_cfg_host_ip && *var_cfg_host_ip) {
		snprintf(ip, sizeof(ip), "%s", var_cfg_host_ip);
	} else if (acl_getsockname(ACL_VSTREAM_SOCK(client), ip, sizeof(ip)) < 0) {
		acl_msg_warn("%s(%d): get local ip error: %s",
			__FUNCTION__, __LINE__, acl_last_serror());
		snprintf(ip, sizeof(ip), "127.0.0.1");
	} else {
		ptr = strchr(ip, ':');
		if (ptr)
			*ptr = 0;
	}

	acl_msg_info("use ip: %s", var_cfg_host_ip);

	if (smtp_banner(client) < 0)
		RETURN (-1);
	if (smtp_helo(client) < 0)
		RETURN (-1);
	if (*var_cfg_auth_user != 0 && *var_cfg_auth_pass != 0) {
		if (smtp_auth(client) < 0)
			RETURN (-1);
	}
	if (smtp_from(client) < 0)
		RETURN (-1);
	if (smtp_tos(client, rcpts) < 0)
		RETURN (-1);
	if (smtp_data(client, proc, rcpts, pid, info, ip) < 0)
		RETURN (-1);
	acl_msg_info("send mail ok!");
	RETURN (0);
}

int smtp_notify(const char *proc, ACL_ARGV *rcpts,
	int pid, const char *info)
{
	if (*var_cfg_smtpd_addr == 0 || *var_cfg_warn_mail == 0
		|| var_recipients == NULL)
	{
		acl_msg_info("smtp_notify not sent!, smtpd_addr: %s, warn_mail: %s",
			var_cfg_smtpd_addr, var_cfg_warn_mail);
		return (0);
	}

	return (smtp_sendmail(proc, rcpts, pid, info));
}
