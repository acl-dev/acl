#include <string>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"
#include <vector>

static void build_html(void)
{
	acl::mail_message message("gbk");

	message.set_from("zsxxsz@263.net")
		.set_sender("zsx1@263.net")
		.set_reply_to("zsx2@263.net")
		.add_to("\"郑树新1\" <zsx1@sina.com>")
//		.add_to("\"郑树新1\" <zsx1@sina.com>; \"郑树新2\" <zsx2@sina.com>; <zsx3@sina.com>")
//		.add_cc("\"郑树新3\" <zsx1@163.com>; \"郑树新4\" <zsx2@163.com>")
		.set_subject("主题：中国人民银行！")
		.add_header("X-Forward-For", "<zsx@263.net>");

	const char* html = "<html><body><B><font color='red'>中国人民银行 HTML 格式</font></B></body></html>";
	acl::mail_body body("gbk");
	body.set_html(html, strlen(html));
	message.set_body(body);

	const char* filepath = "./html.eml";
	if (message.save_to(filepath) == false)
		printf("compose %s error: %s\r\n", filepath, acl::last_serror());
	else
		printf("compose %s ok\r\n", filepath);
}

static void build_plain(void)
{
	acl::mail_message message("gbk");

	message.set_from("zsxxsz@263.net")
		.set_sender("zsx1@263.net")
		.set_reply_to("zsx2@263.net")
		.add_to("\"郑树新1\" <zsx1@sina.com>; \"郑树新2\" <zsx2@sina.com>")
		.add_cc("\"郑树新3\" <zsx1@163.com>; \"郑树新4\" <zsx2@163.com>")
		.set_subject("主题：中国人民银行！");

	const char* plain = "中国人民银行 TEXT 格式";
	acl::mail_body body("gbk");
	body.set_plain(plain, strlen(plain));
	message.set_body(body);

	const char* filepath = "./plain.eml";
	if (message.save_to(filepath) == false)
		printf("compose %s error: %s\r\n", filepath, acl::last_serror());
	else
		printf("compose %s ok\r\n", filepath);
}

static void build_alternative(void)
{
	acl::mail_message message("gbk");

	message.set_from("zsxxsz@263.net", "郑树新")
		.set_sender("zsx1@263.net")
		.set_reply_to("zsx2@263.net")
		.add_to("\"郑树新1\" <zsx1@sina.com>; \"郑树新2\" <zsx2@sina.com>")
		.add_cc("\"郑树新3\" <zsx1@163.com>; \"郑树新4\" <zsx2@163.com>")
		.set_subject("主题：中国人民银行！");
	message.add_attachment("main.cpp", "text/plain")
		.add_attachment("Makefile", "text/plain");

	const char* plain = "中国人民银行 TEXT 格式";
	const char* html = "<html><body><B><font color='red'>中国人民银行 HTML 格式</font></B></body></html>";
	acl::mail_body body("gbk");
	body.set_alternative(html, strlen(html), plain, strlen(plain));
	message.set_body(body);

	const char* filepath = "./alternative.eml";
	if (message.save_to(filepath) == false)
		printf("compose %s error: %s\r\n", filepath, acl::last_serror());
	else
		printf("compose %s ok\r\n", filepath);
}

static void build_relative(void)
{
	acl::mail_message message("gbk");

	message.set_from("zsxxsz@263.net", "郑树新")
		.set_sender("zsx1@263.net")
		.set_reply_to("zsx2@263.net")
		.add_to("\"郑树新1\" <zsx1@sina.com>; \"郑树新2\" <zsx2@sina.com>")
		.add_cc("\"郑树新3\" <zsx1@163.com>; \"郑树新4\" <zsx2@163.com>")
		.set_subject("主题：中国人民银行！");

	const char* plain_file = "./var/plain.txt";
	const char* html_file = "./var/html.txt";
	acl::string plain, html;
	if (acl::ifstream::load(plain_file, &plain) == false)
	{
		printf("load %s error %s\r\n", plain_file, acl::last_serror());
		return;
	}
	if (acl::ifstream::load(html_file, &html) == false)
	{
		printf("load %s error %s\r\n", html_file, acl::last_serror());
		return;
	}

	/////////////////////////////////////////////////////////////////////

	acl::dbuf_pool* dbuf = new acl::dbuf_pool;
	std::vector<acl::mail_attach*> attachments;
	acl::mail_attach* attach;

	attach = new((char*) dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/spacer.gif", "image/gif", "gbk");
	attach->set_content_id("__0@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/tl.jpg", "image/jpg", "gbk");
	attach->set_content_id("__1@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/t_bg.jpg", "image/jpg", "gbk");
	attach->set_content_id("__2@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/tr.jpg", "image/jpg", "gbk");
	attach->set_content_id("__3@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/m_bgl.jpg", "image/jpg", "gbk");
	attach->set_content_id("__4@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/m.jpg", "image/jpg", "gbk");
	attach->set_content_id("__5@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/m_bgr.jpg", "image/jpg", "gbk");
	attach->set_content_id("__6@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/dl.jpg", "image/jpg", "gbk");
	attach->set_content_id("__7@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/d_bg.jpg", "image/jpg", "gbk");
	attach->set_content_id("__8@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/dr.jpg", "image/jpg", "gbk");
	attach->set_content_id("__9@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/t_ml.jpg", "image/jpg", "gbk");
	attach->set_content_id("__10@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/m_tl.jpg", "image/jpg", "gbk");
	attach->set_content_id("__11@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/m_dr.jpg", "image/jpg", "gbk");
	attach->set_content_id("__12@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/d_mr.jpg", "image/jpg", "gbk");
	attach->set_content_id("__13@Foxmail.net");
	attachments.push_back(attach);

	/////////////////////////////////////////////////////////////////////

	acl::mail_body body("gbk");
	body.set_relative(html.c_str(), html.size(),
		plain.c_str(), plain.size(), attachments);
	message.set_body(body);

	const char* filepath = "./relative.eml";
	if (message.save_to(filepath) == false)
		printf("compose %s error: %s\r\n", filepath, acl::last_serror());
	else
		printf("compose %s ok\r\n", filepath);

	// 调用所有动态对象的析构函数
	std::vector<acl::mail_attach*>::iterator it = attachments.begin();
	for (; it != attachments.end(); ++it)
		(*it)->~mail_attach();

	// 一次性释放前面动态分配的内存
	dbuf->destroy();
}

static void build_mixed_relative(void)
{
	acl::mail_message message("gbk");

	message.set_from("zsxxsz@263.net", "郑树新")
		.set_sender("zsx1@263.net")
		.set_reply_to("zsx2@263.net")
		.add_to("\"郑树新1\" <zsx1@sina.com>; \"郑树新2\" <zsx2@sina.com>")
		.add_cc("\"郑树新3\" <zsx1@163.com>; \"郑树新4\" <zsx2@163.com>")
		.set_subject("主题：中国人民银行！");

	message.add_attachment("main.cpp", "text/plain")
		.add_attachment("Makefile", "text/plain");

	const char* plain_file = "./var/email1/plain.txt";
	const char* html_file = "./var/email1/html.txt";
	acl::string plain, html;
	if (acl::ifstream::load(plain_file, &plain) == false)
	{
		printf("load %s error %s\r\n", plain_file, acl::last_serror());
		return;
	}
	if (acl::ifstream::load(html_file, &html) == false)
	{
		printf("load %s error %s\r\n", html_file, acl::last_serror());
		return;
	}

	/////////////////////////////////////////////////////////////////////

	acl::dbuf_pool* dbuf = new acl::dbuf_pool;
	std::vector<acl::mail_attach*> attachments;
	acl::mail_attach* attach;

	attach = new((char*) dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/spacer.gif", "image/gif", "gbk");
	attach->set_content_id("__0@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/tl.jpg", "image/jpg", "gbk");
	attach->set_content_id("__1@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/t_bg.jpg", "image/jpg", "gbk");
	attach->set_content_id("__2@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/tr.jpg", "image/jpg", "gbk");
	attach->set_content_id("__3@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/m_bgl.jpg", "image/jpg", "gbk");
	attach->set_content_id("__4@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/m.jpg", "image/jpg", "gbk");
	attach->set_content_id("__5@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/m_bgr.jpg", "image/jpg", "gbk");
	attach->set_content_id("__6@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/dl.jpg", "image/jpg", "gbk");
	attach->set_content_id("__7@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/d_bg.jpg", "image/jpg", "gbk");
	attach->set_content_id("__8@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/dr.jpg", "image/jpg", "gbk");
	attach->set_content_id("__9@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/t_ml.jpg", "image/jpg", "gbk");
	attach->set_content_id("__10@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/m_tl.jpg", "image/jpg", "gbk");
	attach->set_content_id("__11@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/m_dr.jpg", "image/jpg", "gbk");
	attach->set_content_id("__12@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/email1/img/d_mr.jpg", "image/jpg", "gbk");
	attach->set_content_id("__13@Foxmail.net");
	attachments.push_back(attach);

	/////////////////////////////////////////////////////////////////////

	acl::mail_body body("gbk");
	body.set_relative(html.c_str(), html.size(),
		plain.c_str(), plain.size(), attachments);
	message.set_body(body);

	const char* filepath = "./mixed_relative.eml";
	if (message.save_to(filepath) == false)
		printf("compose %s error: %s\r\n", filepath, acl::last_serror());
	else
		printf("compose %s ok\r\n", filepath);

	// 调用所有动态对象的析构函数
	std::vector<acl::mail_attach*>::iterator it = attachments.begin();
	for (; it != attachments.end(); ++it)
		(*it)->~mail_attach();

	// 一次性释放前面动态分配的内存
	dbuf->destroy();
}

static void build_mixed_relative2(void)
{
	acl::mail_message message("gbk");

	message.set_from("zsxxsz@263.net", "郑树新")
		.set_sender("zsx1@263.net")
		.set_reply_to("zsx2@263.net")
		.add_to("\"郑树新1\" <zsx1@sina.com>; \"郑树新2\" <zsx2@sina.com>")
		.add_cc("\"郑树新3\" <zsx1@163.com>; \"郑树新4\" <zsx2@163.com>")
		.set_subject("主题：中国人民银行！");

	message.add_attachment("main.cpp", "text/plain")
		.add_attachment("var/email2/architecture.pptx", "application/ms-pptx");

	const char* plain_file = "./var/email2/plain.txt";
	const char* html_file = "./var/email2/html.txt";
	acl::string plain, html;
	if (acl::ifstream::load(plain_file, &plain) == false)
	{
		printf("load %s error %s\r\n", plain_file, acl::last_serror());
		return;
	}
	if (acl::ifstream::load(html_file, &html) == false)
	{
		printf("load %s error %s\r\n", html_file, acl::last_serror());
		return;
	}

	/////////////////////////////////////////////////////////////////////

	std::vector<acl::mail_attach*> attachments;
	acl::mail_attach* attach = new acl::mail_attach("./var/email2/img/q.JPG", "image/jpg", "gbk");
	attach->set_content_id("111CF2FD1A884FF9BD0A2B735ED773F3@ikerf954f96714");
	attachments.push_back(attach);

	/////////////////////////////////////////////////////////////////////

	acl::mail_body body("gbk");
	body.set_relative(html.c_str(), html.size(),
		plain.c_str(), plain.size(), attachments);
	message.set_body(body);

	const char* filepath = "./mixed_relative2.eml";
	if (message.save_to(filepath) == false)
		printf("compose %s error: %s\r\n", filepath, acl::last_serror());
	else
		printf("compose %s ok\r\n", filepath);

	delete attach;
}

static void build_mixed(void)
{
	acl::mail_message message("gbk");

	message.set_from("zsxxsz@263.net", "郑树新")
		.set_sender("zsx1@263.net")
		.set_reply_to("zsx2@263.net")
		.add_to("\"郑树新1\" <zsx1@sina.com>; \"郑树新2\" <zsx2@sina.com>")
		.add_cc("\"郑树新3\" <zsx1@163.com>; \"郑树新4\" <zsx2@163.com>")
		.set_subject("主题：中国人民银行！");

	message.add_attachment("main.cpp", "text/plain")
		.add_attachment("var/email2/architecture.pptx", "application/ms-pptx");

	const char* filepath = "./mixed.eml";
	if (message.save_to(filepath) == false)
		printf("compose %s error: %s\r\n", filepath, acl::last_serror());
	else
		printf("compose %s ok\r\n", filepath);
}

static void build_mixed_html(void)
{
	acl::mail_message message("gbk");

	message.set_from("zsxxsz@263.net", "郑树新")
		.set_sender("zsx1@263.net")
		.set_reply_to("zsx2@263.net")
		.add_to("\"郑树新1\" <zsx1@sina.com>; \"郑树新2\" <zsx2@sina.com>")
		.add_cc("\"郑树新3\" <zsx1@163.com>; \"郑树新4\" <zsx2@163.com>")
		.set_subject("主题：中国人民银行！");

	message.add_attachment("main.cpp", "text/plain")
		.add_attachment("var/email2/architecture.pptx", "application/ms-pptx");

	const char* html_file = "./var/html.txt";
	acl::string html;
	if (acl::ifstream::load(html_file, &html) == false)
	{
		printf("load %s error %s\r\n", html_file, acl::last_serror());
		return;
	}

	/////////////////////////////////////////////////////////////////////

	acl::mail_body body("gbk");
	body.set_html(html.c_str(), html.size());
	message.set_body(body);

	const char* filepath = "./mixed_html.eml";
	if (message.save_to(filepath) == false)
		printf("compose %s error: %s\r\n", filepath, acl::last_serror());
	else
		printf("compose %s ok\r\n", filepath);
}

static void build_mixed_plain(void)
{
	acl::mail_message message("gbk");

	message.set_from("zsxxsz@263.net", "郑树新")
		.set_sender("zsx1@263.net")
		.set_reply_to("zsx2@263.net")
		.add_to("\"郑树新1\" <zsx1@sina.com>; \"郑树新2\" <zsx2@sina.com>")
		.add_cc("\"郑树新3\" <zsx1@163.com>; \"郑树新4\" <zsx2@163.com>")
		.set_subject("主题：中国人民银行！");

	message.add_attachment("main.cpp", "text/plain")
		.add_attachment("var/email2/architecture.pptx", "application/ms-pptx");

	const char* plain_file = "./var/plain.txt";
	acl::string plain;
	if (acl::ifstream::load(plain_file, &plain) == false)
	{
		printf("load %s error %s\r\n", plain_file, acl::last_serror());
		return;
	}

	/////////////////////////////////////////////////////////////////////

	acl::mail_body body("gbk");
	body.set_html(plain.c_str(), plain.size());
	message.set_body(body);

	const char* filepath = "./mixed_plain.eml";
	if (message.save_to(filepath) == false)
		printf("compose %s error: %s\r\n", filepath, acl::last_serror());
	else
		printf("compose %s ok\r\n", filepath);
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"-t mime_type[1: html, 2: plain, 3: alternative, 4: relative, 5: mixed_relative, 6: mixed_relative2, 7: mixed, 8: mixed_html, 9: mixed_plain\r\n",
		procname);

#if defined(_WIN32) || defined(_WIN64)
	printf("Enter any key to exit...\r\n");
	getchar();
#endif
}

int main(int argc, char* argv[])
{
	int   ch;
	int   mime_type = 0;

	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "ht:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 1;
		case 't':
			mime_type = atoi(optarg);
			break;
		default:
			break;
		}
	}

	if (mime_type == 1)
		build_html();
	else if (mime_type == 2)
		build_plain();
	else if (mime_type == 3)
		build_alternative();
	else if (mime_type == 4)
		build_relative();
	else if (mime_type == 5)
		build_mixed_relative();
	else if (mime_type == 6)
		build_mixed_relative2();
	else if (mime_type == 7)
		build_mixed();
	else if (mime_type == 8)
		build_mixed_html();
	else if (mime_type == 9)
		build_mixed_plain();
	else
	{
		build_html();
		build_plain();
		build_mixed_html();
		build_mixed_plain();
		build_alternative();
		build_relative();
		build_mixed_relative();
		build_mixed_relative2();
		build_mixed();
	}

#if defined(_WIN32) || defined(_WIN64)
	printf("Enter any key to exit...\r\n");
	getchar();
#endif

	return (0);
}
