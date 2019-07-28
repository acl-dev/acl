// mime.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "lib_acl.h"
#ifndef WIN32
#include <getopt.h>
#endif
#include <string>
#include <errno.h>
#include <string.h>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/ifstream.hpp"
#include "acl_cpp/stream/ofstream.hpp"
#include "acl_cpp/stdlib/charset_conv.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/mime/mime.hpp"
#include "acl_cpp/mime/mime_body.hpp"
#include "acl_cpp/mime/rfc2047.hpp"
#include "acl_cpp/mime/mime_attach.hpp"

#include "pipeline_string.h"

using namespace std;
using namespace acl;

static void header_out(const acl::mime* mime)
{
	printf("sender addr: %s\n", mime->sender().c_str());
	printf("from addr: %s\n", mime->from().c_str());
	printf("replyto addr: %s\n", mime->replyto().c_str());
	printf("returnpath addr: %s\n", mime->returnpath().c_str());

	const list<char*>& rcpts = mime->rcpt_list();
	list<char*>::const_iterator cit;

	for (cit = rcpts.begin(); cit != rcpts.end(); cit++)
	{
		const char* addr = *cit;
		printf("rcpt addr: %s\n", addr);
	}

	const list<char*>& tos = mime->to_list();
	for (cit = tos.begin(); cit != tos.end(); cit++)
	{
		const char* addr = *cit;
		printf("to addr: %s\n", addr);
	}

	const list<char*>& ccs = mime->cc_list();
	for (cit = ccs.begin(); cit != ccs.end(); cit++)
	{
		const char* addr = *cit;
		printf("cc addr: %s\n", addr);
	}

	const char* to = mime->header_value("To");
	acl::string out;
	if (to)
		rfc2047::decode(to, (int) strlen(to), &out);
	printf(">>>>To: %s, %s\n", to ? to : "null", out.c_str());
}

// 解析邮件并输出邮件头信息
static void test_mime_header(acl::mime& mime, const char* path)
{
	// 以下仅解析邮件头部分

	printf("\r\n");
	ACL_METER_TIME("---------------parse header begin--------------------");
	acl::ifstream fp;
	if (fp.open_read(path) == false) {
		printf("open %s error %s\n", path, strerror(errno));
		return;
	}
	acl::string buf;
	const char* ptr;
	size_t n;

	// 开始邮件解析过程
	mime.update_begin(path); // update_begin 内部会自动调用 reset()

	while (1) {
		if (fp.gets(buf, false) == false)
			break;
		if (buf == "\r\n" || buf == "\n")
			break;

		ptr = buf.c_str();

		n = buf.length();

		// 如果返回 true 表示头部解析完毕, 为了使该函数返回 true,
		// 必须保证调用此函数的最后一行数据为 "\r\n" 即空行

		(void) mime.update(ptr, n);
	}

	mime.update_end();

	header_out(&mime);
	ACL_METER_TIME("---------------parse header end  --------------------");
}

// 将邮件正文内容保存在磁盘上
// 将邮件正文内容保存在缓冲区中
// 采用管道流模式将邮件正文内容进行字符集转换，且保存在磁盘上
// 将邮件中的附件保存在磁盘上
// 将解析后的邮件再重新组合并保存在磁盘上
static void mime_test1(acl::mime& mime, const char* path, bool htmlFirst)
{
	mime.reset();

	acl::string buf;
	acl::ofstream fp_out;
	acl::mime_body* pBody = NULL;

	printf("\r\n>>> %s: path: %s\n", __FUNCTION__, path);

	ACL_METER_TIME("---------------parse begin --------------------");

	if (mime.parse(path) == false)
	{
		printf("mime parse %s error\n", path);
		return;
	}

	ACL_METER_TIME("---------------parse end  --------------------");

	const char* ctype = mime.get_ctype();
	bool is_text;
	if (!strcasecmp(ctype, "multipart") ||  !strcasecmp(ctype, "text"))
		is_text = true;
	else
		is_text = false;

	if (is_text)
		pBody = mime.get_body_node(htmlFirst, true, "iso-2022-jp");
	else
		pBody = mime.get_body_node(htmlFirst, false, NULL);

	//////////////////////////////////////////////////////////////////////

	// 将邮件正文内容保存在磁盘上

	printf("\r\n");

	ACL_METER_TIME("---------------save_body begin--------------------");

	header_out(&mime);
	mime.mime_debug("./var");

	printf(">>>ctype: %s, stype: %s\r\n",
		mime.get_ctype(), mime.get_stype());

	buf = "./var/";
	buf += path;
	mime.save_as(buf.c_str());

	if (pBody)
		pBody->save_body("./var/body_gb.txt");

	ACL_METER_TIME("---------------save_body end  --------------------");

	//////////////////////////////////////////////////////////////////////

	// 将邮件正文内容保存在缓冲区中

	if (pBody)
	{
		acl::string buf1;
		pBody->save_body(buf1);

		if (fp_out.open_write("./var/body_string.txt"))
		{
			fp_out.write(buf1.c_str(), buf1.length());
			fp_out.close();
		}
	}

	//////////////////////////////////////////////////////////////////////

	// 将邮件正文内容以 utf-8 存在磁盘上
	if (pBody && is_text)
	{
		printf("\r\n");
		ACL_METER_TIME("---------------save_body begin--------------------");
		acl::charset_conv jp2utf8;
		jp2utf8.update_begin("iso-2022-jp", "utf-8");

		acl::pipe_string pipe_out;
		pipeline_string pipeline;
		acl::pipe_manager manager;

		manager.push_front(&pipe_out);
		manager.push_front(&jp2utf8);
		manager.push_front(&pipeline);

		pBody->save(manager);

		ACL_METER_TIME("---------------save_body end1 --------------------");

		acl::string& sbuf = pipe_out.get_buf();

		fp_out.close();

		if (fp_out.open_write("./var/body_utf8.txt"))
		{
			fp_out.write(sbuf.c_str(), sbuf.length());
			fp_out.close();
		}

		ACL_METER_TIME("---------------save_body end2 --------------------");
	}

	//////////////////////////////////////////////////////////////////////

	// 采用管道流模式将邮件正文内容进行字符集转换，且保存在磁盘上

	if (pBody)
	{
		printf("\r\n");
		ACL_METER_TIME("---------------save_body begin--------------------");
		acl::charset_conv utf8ToGb, gbToBig5;
		utf8ToGb.update_begin("utf-8", "gbk");
		gbToBig5.update_begin("gbk", "big5");

		acl::pipe_string pipe_out;
		acl::pipe_manager manager;

		manager.push_front(&pipe_out);
		manager.push_front(&gbToBig5);
		manager.push_front(&utf8ToGb);

		pBody->save(manager);

		ACL_METER_TIME("---------------save_body end1 --------------------");

		acl::string& sbuf = pipe_out.get_buf();

		fp_out.close();

		if (fp_out.open_write("./var/body_big5.txt"))
		{
			fp_out.write(sbuf.c_str(), sbuf.length());
			fp_out.close();
		}

		ACL_METER_TIME("---------------save_body end2 --------------------");
	}

	//////////////////////////////////////////////////////////////////////

	// 将邮件中的附件保存在磁盘上

	printf("---------------------------------------------------------\r\n");
	printf(">>>> saving attach file now ...\r\n");
	const std::list<acl::mime_attach*>& attaches = mime.get_attachments();
	std::list<acl::mime_attach*>::const_iterator cit = attaches.begin();
	for (; cit != attaches.end(); cit++)
	{
		buf = "./var/";
		const char* filename = (*cit)->get_filename();
		if (filename != NULL)
			buf << filename;
		else
		{
			filename = (*cit)->header_value("Content-ID");
			if (filename == NULL || *filename == 0)
				continue;
			acl::string tmp(filename);
			tmp.strip("<>", true);
			buf << tmp;
		}

		acl::string attach_name;
		acl::rfc2047 rfc2047;
		rfc2047.reset(true);
		rfc2047.decode_update(buf, (int) buf.length());
		rfc2047.decode_finish("utf-8", &attach_name);

		printf(">>> attach file: |%s|, len: %d\n",
			attach_name.c_str(), (int) attach_name.length());

		(*cit)->save(attach_name.c_str());
	}

	printf(">>>> saved attach file ok ...\r\n");
	//////////////////////////////////////////////////////////////////////
	// 遍历所有节点
	printf("------------------------------------------------------\r\n");

	acl::string tmp;
	int i = 0;
	const std::list<acl::mime_node*>& nodes = mime.get_mime_nodes();
	for (std::list<acl::mime_node*>::const_iterator cit2 = nodes.begin();
		cit2 != nodes.end(); ++cit2)
	{
		printf("ctype: %s, stype: %s, begin: %ld, end: %ld\r\n",
			(*cit2)->get_ctype_s(), (*cit2)->get_stype_s(),
			(long) (*cit2)->get_bodyBegin(),
			(long) (*cit2)->get_bodyEnd());
		tmp.format("var/node-%d-body.txt", i++);
		(*cit2)->save(tmp);
		printf(">>>save to file: %s\r\n", tmp.c_str());
	}

	printf("------------------------------------------------------\r\n");

	//////////////////////////////////////////////////////////////////////

	// 将解析后的邮件再重新组合并保存在磁盘上

	mime.save_mail("./var", "test.html");
}

// 将位于文件中的邮件内容进行解析，并将邮件体数据转储于另一个内存缓冲中
static void mime_test2(acl::mime& mime, const char* path)
{
	// 以下仅解析邮件头部分

	printf("\r\n");
	ACL_METER_TIME("---------------parse mail begin--------------------");
	acl::ifstream fp;
	if (fp.open_read(path) == false) {
		printf("open %s error %s\n", path, strerror(errno));
		return;
	}
	acl::string buf;
	const char* ptr;
	size_t n;

	// 开始邮件解析过程
	mime.update_begin(path);

	while (1) {
		if (fp.gets(buf, false) == false)
			break;
		ptr = buf.c_str();
		n = buf.length();

		// 如果返回 true 表示头部解析完毕, 为了使该函数返回 true,
		// 必须保证调用此函数的最后一行数据为 "\r\n" 即空行

		//printf(">>>>%s", ptr);
		if (mime.update(ptr, n) == true)
		{
			printf(">>> parse over, last line: %s\n", ptr);
			break;
		}
		buf.clear();
	}

	// 必须调用 update_end
	mime.update_end();

	acl::mime_body* pBody;
	pBody = mime.get_body_node(false);
	if (pBody)
	{
		acl::string buf2;
		pBody->save_body(buf2);
		printf(">>>>>>>body: %s\n", buf2.c_str());
	}

	header_out(&mime);
	ACL_METER_TIME("---------------parse mail end  --------------------");
}

// 将位于内存中的邮件内容进行解析，并将邮件体数据转储于另一个内存缓冲中
static void mime_test3(acl::mime& mime, const char* path)
{
	// 以下仅解析邮件头部分

	printf("\r\n");
	ACL_METER_TIME("---------------parse mail begin--------------------");
	acl::string buf;

	if (acl::ifstream::load(path, &buf) == false)
	{
		printf("load %s error %s\n", path, strerror(errno));
		return;
	}

	// 开始邮件解析过程
	mime.reset();
	if (mime.update(buf.c_str(), buf.length()) != true)
	{
		printf("mime parse error\r\n");
		return;
	}

	// 必须调用 update_end
	mime.update_end();

	acl::mime_body* pBody;
	pBody = mime.get_body_node(false);
	if (pBody)
	{
		acl::string out;
		if (pBody->save_body(out, buf.c_str(), (ssize_t) buf.length()) == false)
			printf(">>>>save_body to buffer error\r\n");
		else
			printf(">>>>>>>body: %s\n", out.c_str());
	}
	else
		printf(">>>> no body\r\n");

	header_out(&mime);
	ACL_METER_TIME("---------------parse mail end  --------------------");
}

// 带偏移量的MIME解析过程，并将邮件体数据转储于另一个文件中
static void mime_test4(acl::mime& mime, const char* path)
{
	mime.reset();
	acl::ifstream in;
	if (in.open_read(path) == false)
	{
		printf("open %s error %s\n", path, strerror(errno));
		return;
	}
	acl::string buf;
	size_t off = 0;

	// 先读文件头, 并略过文件头
	while (true)
	{
		if (in.gets(buf, false) == false)
			break;

		off += buf.length();

		if (buf == "\n" || buf == "\r\n")
		{
			buf.clear();
			break;
		}
		buf.clear();
	}

	// 开始解析邮件头及邮件体部分

	mime.update_begin(path);
	// 开始读邮件
	while (true)
	{
		if (in.gets(buf, false) == false)
			break;
		mime.update(buf.c_str(), buf.length());
		buf.clear();
	}
	mime.update_end();

	printf("\n-----------------------------------------------------\n\n");
	acl::mime_body* pBody = mime.get_body_node(false, true, "gb2312", (off_t) off);
	if (pBody)
	{
		acl::string buf2;
		pBody->save_body(buf2);
		printf(">>>>>>>body(%d): %s\n", (int) off, buf2.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{

	printf("usage: %s [options]\r\n"
		" -h [help]\r\n"
		" -s [html first, or text first]\r\n"
		" -t test0/test1/test2/test3/test4\r\n"
		" -f mail_file\r\n", procname);
	
	printf("test0: 解析邮件并输出邮件头信息\r\n"
		"test1: 将邮件正文内容保存在磁盘上\r\n"
		"\t将邮件正文内容保存在缓冲区中\r\n"
		"\t采用管道流模式将邮件正文内容进行字符集转换，且保存在磁盘上\r\n"
		"\t将邮件中的附件保存在磁盘上\r\n"
		"\t将解析后的邮件再重新组合并保存在磁盘上\r\n");
	printf("test2: 将位于文件中的邮件内容进行解析，并将邮件体数据转储于另一个内存缓冲中\r\n");
	printf("test3: 将位于内存中的邮件内容进行解析，并将邮件体数据转储于另一个内存缓冲中\r\n");
	printf("test4: 带偏移量的MIME解析过程，并将邮件体数据转储于另一个文件中(例子：test16.eml)\r\n");
}

int main(int argc, char* argv[])
{
	char  ch;
	bool  htmlFirst = false;
	acl::string path("test.eml");
	acl::string cmd("test1");
	acl::log::stdout_open(true);

	while ((ch = (char) getopt(argc, argv, "hst:f:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return (0);
		case 's':
			htmlFirst = true;
			break;
		case 't':
			cmd = optarg;
			break;
		case 'f':
			path = optarg;
			break;
		default:
			break;
		}
	}

	logger_open("test.log", "mime", "all:1");

	acl::mime mime;

	//////////////////////////////////////////////////////////////////////

	if (cmd == "test0")
		test_mime_header(mime, path.c_str());
	else if (cmd == "test1")
		mime_test1(mime, path.c_str(), htmlFirst);
	else if (cmd == "test2")
		mime_test2(mime, path.c_str());
	else if (cmd == "test3")
		mime_test3(mime, path.c_str());
	else if (cmd == "test4")
		mime_test4(mime, path.c_str());
	else
		printf(">>> unknown cmd: %s\n", cmd.c_str());

	//////////////////////////////////////////////////////////////////////

	printf("enter any key to exit\r\n");
	logger_close();
	getchar();
	return 0;
}
