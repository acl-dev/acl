/* smtp_client.cpp : 定义控制台应用程序的入口点 */

#include "lib_acl.h"
#include "lib_protocol.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

static int smtp_sender(void)
{
	SMTP_CLIENT* conn;
	char  addr[128], line[256];

	acl_printf("please enter smtp server addr: ");
	if (acl_gets_nonl(line, sizeof(line)) == NULL)
	{
		acl_puts("invalid smtp server addr");
		return -1;
	}

	if (strchr(line, ':') == NULL)
		snprintf(addr, sizeof(addr), "%s:25", line);
	else
		snprintf(addr, sizeof(addr), "%s", line);

	/* 连接 SMTP 服务器 */
	conn = smtp_open(addr, 60, 60, 1024);
	if (conn == NULL)
	{
		acl_printf("connect %s error %s\r\n", addr, acl_last_serror());
		return -1;
	}
	else
		acl_printf("connect smtpd(%s) ok\r\n", addr);

	/* 从 SMTP 服务器获得欢迎信息 */
	if (smtp_get_banner(conn) != 0)
	{
		acl_puts("get banner from server error");
		smtp_close(conn);
		return -1;
	}
	else
		acl_printf(">smtpd: %s\r\n", conn->buf);

	/* 向 SMTP 服务器发送 EHLO/HELO 命令 */
	if (smtp_greet(conn, "localhost", 0) != 0)
	{
		acl_printf("send ehlo cmd error: %s\r\n", conn->buf);
		smtp_close(conn);
		return -1;
	}
	else
		acl_printf(">smtpd: %s\r\n", conn->buf);

	/* 用户是否需要进行 SMTP 身份认证 */
	acl_printf("Do you want to auth login? n[y|n]: ");
	if (acl_gets_nonl(line, sizeof(line)) == NULL)
	{
		acl_puts("invalid input");
		smtp_close(conn);
		return -1;
	}

	/* 对用户身份进行 SMTP 身份认证 */
	else if (strcasecmp(line, "Y") == 0)
	{
		char user[128], pass[128];
		acl_printf("Please input user account: ");
		if (acl_gets_nonl(user, sizeof(user)) == NULL)
		{
			acl_puts("input invalid");
			smtp_close(conn);
			return -1;
		}

		acl_printf("Please input user password: ");
		if (acl_gets_nonl(pass, sizeof(pass)) == NULL)
		{
			acl_puts("input invalid");
			smtp_close(conn);
			return -1;
		}

		/* 开始进行 SMTP 身份认证 */
		if (smtp_auth(conn, user, pass) != 0)
		{
			acl_printf("smtp auth(%s, %s) error: %s, code: %d\r\n",
				user, pass, conn->buf, conn->smtp_code);
			smtp_close(conn);
			return -1;
		}
		else
			acl_printf(">smtpd: %s\r\n", conn->buf);
	}

	/* 获得发件人邮箱地址 */
	acl_printf("please input sender's email: ");
	if (acl_gets_nonl(line, sizeof(line)) == NULL)
	{
		acl_puts("invalid sender's email");
		smtp_close(conn);
		return -1;
	}

	/* 发送 MAIL FROM: 命令 */
	if (smtp_mail(conn, line) != 0)
	{
		acl_printf("smtp send MAIL FROM %s error\r\n", line);
		smtp_close(conn);
		return -1;
	}
	else
		acl_printf(">smtpd: %s\r\n", conn->buf);

	/* 发送 RCPT TO: 命令 */
	while (1)
	{
		acl_printf("please input mail recipients: ");
		if (acl_gets_nonl(line, sizeof(line)) == NULL)
		{
			acl_puts("invalid mail recipients");
			smtp_close(conn);
			return -1;
		}

		/* 发送 RCPT TO: 命令 */
		else if (smtp_rcpt(conn, line) != 0)
		{
			acl_printf("send RCPT TO: %s error: %s, code: %d\r\n",
				line, conn->buf, conn->smtp_code);
			smtp_close(conn);
			return -1;
		}
		else
			acl_printf(">smtpd: %s\r\n", conn->buf);

		acl_printf("Do you want to add another recipients? n[y|n]: ");
		if (acl_gets_nonl(line, sizeof(line)) == NULL)
		{
			acl_puts("input invalid");
			smtp_close(conn);
			return -1;
		}
		else if (strcasecmp(line, "y") != 0)
			break;
	}

	/* 发送 DATA: 命令 */
	if (smtp_data(conn) != 0)
	{
		acl_printf("send DATA error %s, code: %d\r\n",
			conn->buf, conn->smtp_code);
		smtp_close(conn);
		return -1;
	}
	else
		acl_printf(">smtpd: %s\r\n", conn->buf);

	/* 从终端接收用户的输入的邮件内容并发往 SMTP 服务器 */
	acl_puts("Please enter the email data below, end with \\r\\n.\\r\\n");

	while (1)
	{
		if (acl_gets_nonl(line, sizeof(line)) == NULL)
		{
			acl_puts("readline error");
			smtp_close(conn);
			return -1;
		}
		if (strcmp(line, ".") == 0)
			break;
		if (smtp_printf(conn, "%s\r\n", line) != 0)
		{
			acl_printf("send data to smtpd error, data: %s\r\n", line);
			smtp_close(conn);
			return -1;
		}
	}

	/* 发送 \r\n.\r\n 表示邮件数据发送完毕 */
	if (smtp_data_end(conn) != 0)
	{
		acl_printf("send . error: %s, code: %d\r\n",
			conn->buf, conn->smtp_code);
		smtp_close(conn);
		return -1;
	}
	else
		acl_printf(">smtpd: %s\r\n", conn->buf);

	/* 发送 QUIT 命令 */
	if (smtp_quit(conn) != 0)
	{
		acl_printf("smtp QUIT error: %s\r\n", conn->buf);
		smtp_close(conn);
		return -1;
	}
	else
		acl_printf(">smtpd: %s\r\n", conn->buf);

	smtp_close(conn);
	return 0;
}

int main(void)
{
	int   ret;

#ifdef WIN32
	acl_init();
#endif

	while (1)
	{
		char line[128];

		ret = smtp_sender();
		if (ret == -1)
			break;
		acl_printf("Do you want to send another email? n[y|n]: ");
		if (acl_gets_nonl(line, sizeof(line)) == NULL)
		{
			acl_puts("invalid input");
			break;
		}
		else if (strcasecmp(line, "y") != 0)
			break;
	}

#ifdef WIN32
	acl_vstream_printf("enter any key to exit\r\n");
	acl_vstream_getc(ACL_VSTREAM_IN);
#endif

	return ret;
}
