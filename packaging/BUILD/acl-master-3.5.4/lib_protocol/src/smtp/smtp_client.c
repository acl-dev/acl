#include "StdAfx.h"
#include "lib_acl.h"
#include <assert.h>
#include <string.h>
#include "smtp/smtp_client.h"

/* 创建 SMTP_CLIENT 对象并连接远程邮件服务器地址 */

SMTP_CLIENT *smtp_open(const char *addr, int conn_timeout,
	int rw_timeout, int line_limit)
{
	SMTP_CLIENT *client;
	ACL_VSTREAM *conn;

	conn = acl_vstream_connect(addr, ACL_BLOCKING, conn_timeout,
		rw_timeout, 4096);
	if (conn == NULL) {
		acl_msg_error("%s(%d): connect %s error(%s)",
			__FUNCTION__, __LINE__, addr,
			acl_last_serror());
		return NULL;
	}

	client = (SMTP_CLIENT*) acl_mycalloc(1, sizeof(SMTP_CLIENT));
	client->conn = conn;
	client->buf = (char*) acl_mymalloc(line_limit);
	client->size = line_limit;

	return client;
}

/* 释放 SMTP_CLIENT 资源 */

void smtp_close(SMTP_CLIENT *client)
{
	/* 此变量有可能被外部释放置空 */
	if (client->conn)
		acl_vstream_close(client->conn);
	acl_myfree(client->buf);
	acl_myfree(client);
}

/* 接收服务器的欢迎信息 */

int smtp_get_banner(SMTP_CLIENT *client)
{
	int   ret;
	char *ptr;

	client->smtp_code = 0;
	client->buf[0] = 0;

	/* 读取欢迎信息 */
	ret = acl_vstream_gets_nonl(client->conn, client->buf, client->size);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets banner error",
			__FUNCTION__, __LINE__);
		return -1;
	}

	ptr = strchr(client->buf, ' ');
	if (ptr == NULL)
		ptr = strchr(client->buf, '\t');
	if (ptr == NULL) {
		acl_msg_error("%s(%d): get banner(%s) invalid",
			__FUNCTION__, __LINE__, client->buf);
		return -1;
	}
	*ptr = 0;
	client->smtp_code = atoi(client->buf);
	*ptr = ' '; /* 恢复原始值 */
	if (client->smtp_code != 220) {
		acl_msg_error("%s(%d): get banner code(%d) error(%s)",
			__FUNCTION__, __LINE__, client->smtp_code, client->buf);
		return -1;
	}
	return 0;
}

int smtp_greet(SMTP_CLIENT *client, const char* name, int ehlo)
{
	if (ehlo)
		return smtp_ehlo(client, name);
	else
		return smtp_helo(client, name);
}

/* 向服务器发送 HELO 命令 */

int smtp_helo(SMTP_CLIENT *client, const char *helo)
{
	int   ret;
	char *ptr;

	client->smtp_code = 0;
	client->buf[0] = 0;

	/* 发送 helo 信息 */
	ret = acl_vstream_fprintf(client->conn, "HELO %s\r\n", helo);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send helo error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}

	/* 读取响应信息 */
	ret = acl_vstream_gets_nonl(client->conn, client->buf, client->size);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets helo's error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}

	ptr = strchr(client->buf, ' ');
	if (ptr == NULL)
		ptr = strchr(client->buf, '\t');
	if (ptr == NULL) {
		acl_msg_error("%s(%d): get helo reply error, line(%s)",
			__FUNCTION__, __LINE__, client->buf);
		return -1;
	}
	*ptr = 0;
	client->smtp_code = atoi(client->buf);
	*ptr = ' ';
	if (client->smtp_code != 250) {
		acl_msg_error("%s(%d): helo's reply code: %d, line(%s)",
			__FUNCTION__, __LINE__, client->smtp_code, client->buf);
		return -1;
	}
	return 0;
}

static void smtp_ehlo_flag(SMTP_CLIENT *client, ACL_ARGV *tokens)
{
	if (strcasecmp(tokens->argv[0], "PIPELINING") == 0) {
		client->flag |= SMTP_FLAG_PIPELINING;
	} else if (strcasecmp(tokens->argv[0], "AUTH") == 0) {
		client->flag |= SMTP_FLAG_AUTH;
	} else if (strcasecmp(tokens->argv[0], "8BITMIME") == 0) {
		client->flag |= SMTP_FLAG_8BITMIME;
	} else if (strcasecmp(tokens->argv[0], "DSN") == 0) {
		client->flag |= SMTP_FLAG_DSN;
	} else if (strcasecmp(tokens->argv[0], "ETRN") == 0) {
		client->flag |= SMTP_FLAG_ETRN;
	} else if (strcasecmp(tokens->argv[0], "SIZE") == 0) {
		if (tokens->argc >= 2)
			client->message_size_limit = atoi(tokens->argv[1]);
		if (client->message_size_limit > 0)
			client->flag |= SMTP_FLAG_SIZE;
	}
}

/* 向服务器发送 EHLO 命令 */

int smtp_ehlo(SMTP_CLIENT *client, const char *ehlo)
{
	int   ret;
	char *ptr;
	ACL_ARGV *tokens;

	client->buf[0] = 0;
	client->smtp_code = 0;

	ret = acl_vstream_fprintf(client->conn, "EHLO %s\r\n", ehlo);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): set EHLO error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}

	while (1) {
		ret = acl_vstream_gets_nonl(client->conn, client->buf, client->size);
		if (ret == ACL_VSTREAM_EOF) {
			acl_msg_error("%s(%d): get EHLO's reply error(%s)",
				__FUNCTION__, __LINE__, acl_last_serror());
			return -1;
		} else if (ret < 3) {
			acl_msg_warn("%s(%d): EHLO's reply(%s) tool short",
				__FUNCTION__, __LINE__, client->buf);
			continue;
		}

		if (strncmp(client->buf, "250", 3) != 0) {
			ret = client->buf[3];
			client->buf[3] = 0;
			client->smtp_code = atoi(client->buf);
			client->buf[3] = ret;

			acl_msg_error("%s(%d): EHLO's reply(%s) code(%d) error",
				__FUNCTION__, __LINE__, client->buf, ret);
			return -1;
		} else
			client->smtp_code = 250;

		if (ret == 3)
			break;

		ptr = client->buf + 4;
		tokens = acl_argv_split(ptr, " =");
		smtp_ehlo_flag(client, tokens);
		acl_argv_free(tokens);

		if (client->buf[3] == ' ')
			break;
	}

	return 0;
}

/* 向服务器发送身份认证信息 */

int smtp_auth(SMTP_CLIENT *client, const char *user, const char *pass)
{
	int   ret;
	ACL_ARGV *tokens = NULL;
	char *user_encoded = NULL, *pass_encoded = NULL;

#undef	RETURN
#define	RETURN(x) do {  \
	if (tokens)  \
		acl_argv_free(tokens);  \
	if (user_encoded)  \
		acl_myfree(user_encoded);  \
	if (pass_encoded)  \
		acl_myfree(pass_encoded);  \
	return (x);  \
} while (0)

	client->smtp_code = 0;
	client->buf[0] = 0;

	/* 发送认证命令 */

	ret = acl_vstream_fprintf(client->conn, "AUTH LOGIN\r\n");
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send AUTH LOGIN to error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		RETURN (-1);
	}

	ret = acl_vstream_gets_nonl(client->conn, client->buf, client->size);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets AUTH LOGIN's reply error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		RETURN (-1);
	}

	tokens = acl_argv_split(client->buf, "\t ");
	client->smtp_code = atoi(tokens->argv[0]);
	if (client->smtp_code != 334) {
		acl_msg_error("%s(%d): AUTH LOGIN failed, line(%s)",
			__FUNCTION__, __LINE__, client->buf);
		RETURN (-1);
	}

	/* 发送邮箱帐号 */

	user_encoded = (char*) acl_base64_encode(user, (int) strlen(user));
	ret = acl_vstream_fprintf(client->conn, "%s\r\n", user_encoded);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send user error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		RETURN (-1);
	}

	ret = acl_vstream_gets_nonl(client->conn, client->buf, client->size);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): auth gets error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		RETURN (-1);
	}
	acl_argv_free(tokens);
	tokens = acl_argv_split(client->buf, "\t ");
	client->smtp_code = atoi(tokens->argv[0]);
	if (client->smtp_code != 334) {
		acl_msg_error("%s(%d): AUTH LOGIN failed, line(%s)",
			__FUNCTION__, __LINE__, client->buf);
		RETURN (-1);
	}

	/* 发送 password */

	pass_encoded = (char*) acl_base64_encode(pass, (int) strlen(pass));
	ret = acl_vstream_fprintf(client->conn, "%s\r\n", pass_encoded);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send pass error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		RETURN (-1);
	}
	ret = acl_vstream_gets_nonl(client->conn, client->buf, client->size);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): auth gets pass's reply error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		RETURN (-1);
	}
	acl_argv_free(tokens);
	tokens = acl_argv_split(client->buf, "\t ");
	client->smtp_code = atoi(tokens->argv[0]);
	if (client->smtp_code != 235) {
		acl_msg_error("%s(%d): AUTH LOGIN failed, line(%s)",
			__FUNCTION__, __LINE__, client->buf);
		RETURN (-1);
	}

	RETURN (0);
}

/* 向服务器发送 MAIL FROM 命令 */

int smtp_mail(SMTP_CLIENT *client, const char *from)
{
	int   ret;
	ACL_ARGV *tokens;

	client->smtp_code = 0;
	client->buf[0] = 0;

	/* 发送 mail from 信息 */
	ret = acl_vstream_fprintf(client->conn, "MAIL FROM: <%s>\r\n", from);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send mail from error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}


	/* 读取响应信息 */
	ret = acl_vstream_gets_nonl(client->conn, client->buf, client->size);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets mail from's reply error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return (-1);
	}

	tokens = acl_argv_split(client->buf, "\t ");
	client->smtp_code = atoi(tokens->argv[0]);
	if (client->smtp_code != 250) {
		acl_msg_error("%s(%d): mail from error(%d), line(%s)",
			__FUNCTION__, __LINE__, client->smtp_code, client->buf);
		acl_argv_free(tokens);
		return -1;
	}

	acl_argv_free(tokens);
	return 0;
}

/* 向服务器发送一个 RCPT TO 命令 */

int smtp_rcpt(SMTP_CLIENT *client, const char *to)
{
	int   ret;
	ACL_ARGV *tokens;

	client->smtp_code = 0;
	client->buf[0] = 0;

	ret = acl_vstream_fprintf(client->conn, "RCPT TO: <%s>\r\n", to);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send rcpt error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}

	ret = acl_vstream_gets_nonl(client->conn, client->buf, client->size);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets rcpt's reply error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}

	tokens = acl_argv_split(client->buf, "\t ");
	client->smtp_code = atoi(tokens->argv[0]);
	if (client->smtp_code != 250) {
		acl_msg_error("%s(%d): rcpt's reply error(%d), line(%s)",
			__FUNCTION__, __LINE__, client->smtp_code, client->buf);
		acl_argv_free(tokens);
		return -1;
	}

	acl_argv_free(tokens);
	return 0;
}

/* 发送 DATA 命令 */

int smtp_data(SMTP_CLIENT *client)
{
	ACL_ARGV *tokens;
	int   ret;

	client->smtp_code = 0;
	client->buf[0] = 0;

	ret = acl_vstream_fprintf(client->conn, "DATA\r\n");
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send data error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}

	ret = acl_vstream_gets_nonl(client->conn, client->buf, client->size);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets data's reply error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}

	tokens = acl_argv_split(client->buf, "\t ");
	client->smtp_code = atoi(tokens->argv[0]);
	if (client->smtp_code != 354) {
		acl_msg_error("%s(%d): data denied, error(%d), line(%s)",
			__FUNCTION__, __LINE__, client->smtp_code, client->buf);
		acl_argv_free(tokens);
		return -1;
	}
	acl_argv_free(tokens);

	return 0;
}

/* 发送  \r\n.\r\n */

int smtp_data_end(SMTP_CLIENT *client)
{
	int   ret;
	ACL_ARGV *tokens;

	client->smtp_code = 0;
	client->buf[0] = 0;

	ret = acl_vstream_fprintf(client->conn, "\r\n.\r\n");
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send mail eof error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}
	
	ret = acl_vstream_gets_nonl(client->conn, client->buf, client->size);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets mail eof's reply error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}

	tokens = acl_argv_split(client->buf, "\t ");
	client->smtp_code = atoi(tokens->argv[0]);
	if (client->smtp_code != 250) {
		acl_msg_error("%s(%d): send mail error(%d), line: %s",
			__FUNCTION__, __LINE__, client->smtp_code, client->buf);
		acl_argv_free(tokens);
		return -1;
	}

	acl_argv_free(tokens);
	return 0;
}

int smtp_send(SMTP_CLIENT *client, const char* src, size_t len)
{
	if (acl_vstream_writen(client->conn, src, len) == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): write data error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}

	return 0;
}

int smtp_printf(SMTP_CLIENT *client, const char* fmt, ...)
{
	va_list ap;
	int   ret;

	va_start(ap, fmt);
	ret = acl_vstream_vfprintf(client->conn, fmt, ap);
	va_end(ap);

	return ret == ACL_VSTREAM_EOF ? -1 : 0;
}

/* 发送邮件内容 */

int smtp_send_stream(SMTP_CLIENT *client, ACL_VSTREAM *in)
{
	int   n = 0, ret;

	while (1) {
		ret = acl_vstream_read(in, client->buf, client->size);
		if (ret == ACL_VSTREAM_EOF)
			break;
		if (acl_vstream_writen(client->conn, client->buf, ret)
			== ACL_VSTREAM_EOF)
		{
			acl_msg_error("%s(%d): write data error(%s)",
				__FUNCTION__, __LINE__, acl_last_serror());
			return -1;
		}
		n += ret;
	}

	if (n == 0) {
		acl_msg_error("%s(%d): in stream is empty",
			__FUNCTION__, __LINE__);
		return -1;
	}
	return 0;
}

/* 向服务器发送邮件内容 */

int smtp_send_file(SMTP_CLIENT *client, const char* filepath)
{
	int   ret;
	ACL_VSTREAM *in = acl_vstream_fopen(filepath, O_RDONLY, 0600, 4096);

	if (in == NULL) {
		acl_msg_error("%s(%d): open %s error(%s)", __FUNCTION__,
			__LINE__, filepath, acl_last_serror());
		return -1;
	}
	ret = smtp_send_stream(client, in);
	acl_vstream_close(in);
	return ret;
}

/* 发送 QUIT 命令 */

int smtp_quit(SMTP_CLIENT *client)
{
	int   ret;
	ACL_ARGV *tokens;

	client->smtp_code = 0;
	client->buf[0] = 0;

	ret = acl_vstream_fprintf(client->conn, "QUIT\r\n");
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send quit cmd error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}

	ret = acl_vstream_gets_nonl(client->conn, client->buf, client->size);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets quit's reply error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}

	tokens = acl_argv_split(client->buf, "\t ");
	client->smtp_code = atoi(tokens->argv[0]);
	if (client->smtp_code != 221) {
		acl_msg_error("%s(%d): quit's reply: %d",
			__FUNCTION__, __LINE__, client->smtp_code);
		acl_argv_free(tokens);
		return -1;
	}
	acl_argv_free(tokens);
	return 0;
}

/* 发送 NOOP 命令 */

int smtp_noop(SMTP_CLIENT *client)
{
	int   ret;
	char *ptr;

	client->buf[0] = 0;
	client->smtp_code = 0;

	ret = acl_vstream_fprintf(client->conn, "NOOP\r\n");
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): write NOOP error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}

	ret = acl_vstream_gets_nonl(client->conn, client->buf, client->size);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): get NOOP's reply error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}

	ptr = strchr(client->buf, ' ');
	if (ptr == NULL)
		ptr = strchr(client->buf, '\t');
	if (ptr == NULL) {
		acl_msg_error("%s(%d): NOOP's reply(%s) invalid",
			__FUNCTION__, __LINE__, client->buf);
		return -1;
	}
	ret = *ptr;
	*ptr = 0;
	client->smtp_code = atoi(client->buf);
	*ptr = ret;
	if (client->smtp_code != 250) {
		acl_msg_error("%s(%d): NOOP's reply(%s) code(%d) error",
			__FUNCTION__, __LINE__, client->buf, client->smtp_code);
		return -1;
	}

	return 0;
}

/* 发送 RSET 命令 */

int smtp_rset(SMTP_CLIENT *client)
{
	int   ret;
	char *ptr;

	client->buf[0] = 0;
	client->smtp_code = 0;

	ret = acl_vstream_fprintf(client->conn, "RSET\r\n");
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): write RSET error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}
	ret = acl_vstream_gets_nonl(client->conn, client->buf, client->size);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): get RSET's reply error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}

	ptr = strchr(client->buf, ' ');
	if (ptr == NULL)
		ptr = strchr(client->buf, '\t');
	if (ptr == NULL) {
		acl_msg_error("%s(%d): RSET's reply(%s) invalid",
			__FUNCTION__, __LINE__, client->buf);
		return -1;
	}

	ret = *ptr;
	*ptr = 0;
	client->smtp_code = atoi(client->buf);
	*ptr = ret;

	if (client->smtp_code != 250) {
		acl_msg_error("%s(%d): RSET's reply(%s) code(%d) error",
			__FUNCTION__, __LINE__, client->buf, client->smtp_code);
		return -1;
	}

	return 0;
}
