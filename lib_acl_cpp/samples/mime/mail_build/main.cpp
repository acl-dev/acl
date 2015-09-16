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
		.add_to("\"郑树新1\" <zsx1@sina.com>; \"郑树新2\" <zsx2@sina.com>")
		.add_cc("\"郑树新3\" <zsx1@163.com>; \"郑树新4\" <zsx2@163.com>")
		.set_subject("主题：中国人民银行！")
		.add_header("X-Forward-For", "<zsx@263.net>");

	const char* html = "<html><body>中国人民银行 HTML 格式</body></html>";
	acl::mail_body body("gbk");
	body.set_html(html, strlen(html));
	message.set_body(body);

	const char* filepath = "./html.eml";
	if (message.save_to(filepath) == false)
		printf("compose %s error: %s\r\n", filepath, acl::last_serror());
	else
		printf("compose %s ok\r\n", filepath);
}

static void build_text(void)
{
	acl::mail_message message("gbk");

	message.set_from("zsxxsz@263.net")
		.set_sender("zsx1@263.net")
		.set_reply_to("zsx2@263.net")
		.add_to("\"郑树新1\" <zsx1@sina.com>; \"郑树新2\" <zsx2@sina.com>")
		.add_cc("\"郑树新3\" <zsx1@163.com>; \"郑树新4\" <zsx2@163.com>")
		.set_subject("主题：中国人民银行！");

	const char* text = "中国人民银行 TEXT 格式";
	acl::mail_body body("gbk");
	body.set_text(text, strlen(text));
	message.set_body(body);

	const char* filepath = "./text.eml";
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

	const char* text = "中国人民银行 TEXT 格式";
	const char* html = "<html><body>中国人民银行 HTML 格式</body></html>";
	acl::mail_body body("gbk");
	body.set_alternative(html, strlen(html), text, strlen(text));
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

	const char* text_file = "./var/text.txt";
	const char* html_file = "./var/html.txt";
	acl::string text, html;
	if (acl::ifstream::load(text_file, &text) == false)
	{
		printf("load %s error %s\r\n", text_file, acl::last_serror());
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
		acl::mail_attach("./var/img/spacer.gif", "image/gif", "gbk");
	attach->set_content_id("__0@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/img/tl.jpg", "image/jpg", "gbk");
	attach->set_content_id("__1@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/img/t_bg.jpg", "image/jpg", "gbk");
	attach->set_content_id("__2@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/img/tr.jpg", "image/jpg", "gbk");
	attach->set_content_id("__3@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/img/m_bgl.jpg", "image/jpg", "gbk");
	attach->set_content_id("__4@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/img/m.jpg", "image/jpg", "gbk");
	attach->set_content_id("__5@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/img/m_bgr.jpg", "image/jpg", "gbk");
	attach->set_content_id("__6@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/img/dl.jpg", "image/jpg", "gbk");
	attach->set_content_id("__7@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/img/d_bg.jpg", "image/jpg", "gbk");
	attach->set_content_id("__8@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/img/dr.jpg", "image/jpg", "gbk");
	attach->set_content_id("__9@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/img/t_ml.jpg", "image/jpg", "gbk");
	attach->set_content_id("__10@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/img/m_tl.jpg", "image/jpg", "gbk");
	attach->set_content_id("__11@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/img/m_dr.jpg", "image/jpg", "gbk");
	attach->set_content_id("__12@Foxmail.net");
	attachments.push_back(attach);

	attach = new(dbuf->dbuf_alloc(sizeof(acl::mail_attach)))
		acl::mail_attach("./var/img/d_mr.jpg", "image/jpg", "gbk");
	attach->set_content_id("__13@Foxmail.net");
	attachments.push_back(attach);

	/////////////////////////////////////////////////////////////////////

	acl::mail_body body("gbk");
	body.set_relative(html.c_str(), html.size(),
		text.c_str(), text.size(), attachments);
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

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"-t mime_type[1: html, 2: text, 3: alternative, 4: relative\r\n",
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
		build_text();
	else if (mime_type == 3)
		build_alternative();
	else if (mime_type == 4)
		build_relative();
	else
	{
		build_html();
		build_text();
		build_alternative();
		build_relative();
	}

#if defined(_WIN32) || defined(_WIN64)
	printf("Enter any key to exit...\r\n");
	getchar();
#endif

	return (0);
}
