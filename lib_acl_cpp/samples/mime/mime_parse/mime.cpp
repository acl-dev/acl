#include "stdafx.h"

static void mime_test(acl::mime& mime, const char* path, bool htmlFirst)
{
	mime.reset();

	acl::string buf;
	acl::ofstream fp_out;
	acl::mime_body* pBody;

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
		pBody = mime.get_body_node(htmlFirst, true, "gbk");
	else
		pBody = mime.get_body_node(htmlFirst, false, NULL);

	if (pBody)
		pBody->save_body("./var/body.txt");
	else
		printf("-----no body data node------\r\n");

	ACL_METER_TIME("---------------save_body end  --------------------");

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
		if (filename == NULL)
			continue;
		buf << filename;

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

//////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{

	printf("usage: %s [options]\r\n"
		" -h [help]\r\n"
		" -s [html first, or text first]\r\n"
		" -f mail_file\r\n", procname);
}

int main(int argc, char* argv[])
{
	char  ch;
	bool  htmlFirst = false;
	acl::string path("test.eml");
	acl::log::stdout_open(true);

	while ((ch = (char) getopt(argc, argv, "hsf:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return (0);
		case 's':
			htmlFirst = true;
			break;
		case 'f':
			path = optarg;
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);
	acl::mime mime;
	mime_test(mime, path.c_str(), htmlFirst);

	return 0;
}
