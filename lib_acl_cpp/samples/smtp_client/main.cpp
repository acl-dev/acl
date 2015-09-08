// md5.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

static acl::polarssl_conf ssl_conf;

static bool send_mail(const char* addr, const char* sender, const char* pass,
	const char* recipients, bool use_ssl)
{
	acl::smtp_client conn(addr, 60, 60);

	if (use_ssl)
		conn.set_ssl(&ssl_conf);

	conn.set_auth(sender, pass)
		.set_from(sender)
		.add_recipients(recipients);

	if (conn.send_envelope() == false)
	{
		printf("send envelope error: %d, %s\r\n",
			conn.get_smtp_code(), conn.get_smtp_status());
		return false;
	}

	if (conn.data_begin() == false)
	{
		printf("send data begin error: %d, %s\r\n",
			conn.get_smtp_code(), conn.get_smtp_status());
	}

	const char* data = "From: <zsxxsz@263.net>\r\n"
		"To: zsxxsz@263.net\r\n"
		"Subject: hello, world!\r\n"
		"\r\n"
		"hello world!\r\n";

	if (conn.write(data, strlen(data)) == false)
	{
		printf("send data error: %d, %s\r\n",
			conn.get_smtp_code(), conn.get_smtp_status());
		return false;
	}

	if (conn.data_end() == false)
	{
		printf("send data end error: %d, %s\r\n",
			conn.get_smtp_code(), conn.get_smtp_status());
		return false;
	}

	printf("sendmail ok, from: %s, to: %s, code: %d, status: %s\r\n",
		sender, recipients, conn.get_smtp_code(),
		conn.get_smtp_status());

	if (conn.quit() == false)
	{
		printf("send quit error: %d, %s\r\n",
			conn.get_smtp_code(), conn.get_smtp_status());
		return false;
	}

	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"\t-s smtp_server_addr\r\n"
		"\t-e [if use ssl]\r\n"
		"\t-u sender\r\n"
		"\t-p sender_pass\r\n"
		"\t-t recipients[sample: to1@xxx.com; to2@xxx.com\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int   ch;
	bool  use_ssl = false;
	acl::string addr("smtp.263.net:25");
	acl::string sender("zsxxsz@263.net"), pass, recipients;

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hs:u:p:t:e")) > 0)
	{
		switch (ch)
		{
		case  'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'u':
			sender = optarg;
			break;
		case 'p':
			pass = optarg;
			break;
		case 't':
			recipients = optarg;
			break;
		case 'e':
			use_ssl = true;
			break;
		default:
			break;
		}
	}

	if (addr.empty() || sender.empty() || pass.empty() || recipients.empty())
	{
		usage(argv[0]);
		return 1;
	}

	(void) send_mail(addr.c_str(), sender.c_str(), pass.c_str(),
		recipients.c_str(), use_ssl);


#if defined(_WIN32) || defined(_WIN64)
	getchar();
#endif
	return 0;
}
