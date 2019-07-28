#include "lib_acl.h"

#define STR	acl_vstring_str

static const char *__data1 =
	"<?xml version=\"1.0\"?>\r\n"
	"<?xml-stylesheet type=\"text/xsl\"\r\n"
	"\thref=\"http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl\"?>\r\n"
	"\t<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\"\r\n"
	"\t\"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\" [\r\n"
	"	<!ENTITY xmllint \"<command>xmllint</command>\">\r\n"
	"]>\r\n"
	"<root name='root1' id='root_id_1'>\r\n"
	"	<user name='user11\"' value='zsx11' id='id11'> user zsx11 </user>\r\n"
	"	<user name='user12' value='zsx12' id='id12'> user zsx12 \r\n"
	"		<age year='1972'>my age</age>\r\n"
	"		<other><email name='zsxxsz@263.net'/>"
	"			<phone>"
	"				<mobile number='111111'> mobile number </mobile>"
	"				<office number='111111'> mobile number </office>"
	"			</phone>"
	"		</other>"
	"	</user>\r\n"
	"	<user name='user13' value='zsx13' id='id13'> user zsx13 </user>\r\n"
	"</root>\r\n"
	"<root name='root2' id='root_id_2'>\r\n"
	"	<user name='user21' value='zsx21' id='id21'> user zsx21 </user>\r\n"
	"	<user name='user22' value='zsx22' id='id22'> user zsx22 \r\n"
	"		<!-- date should be the date of the latest change or the release version -->\r\n"
	"		<age year='1972'>my age</age>\r\n"
	"	</user>\r\n"
	"	<user name='user23' value='zsx23' id='id23'> user zsx23 </user>\r\n"
	"</root>\r\n"
	"<root name = 'root3' id = 'root_id_3'>\r\n"
	"	<user name = 'user31' value = 'zsx31' id = 'id31'> user zsx31 </user>\r\n"
	"	<user name = 'user32' value = 'zsx32' id = 'id32'> user zsx32 </user>\r\n"
	"	<user name = 'user33' value = 'zsx33' id = 'id33'> user zsx33 \r\n"
	"		<age year = '1978' month = '12' day = '11'> bao bao </age>\r\n"
	"	</user>\r\n"
	"	<!-- still a bit buggy output, will talk to docbook-xsl upstream to fix this -->\r\n"
	"	<!-- <releaseinfo>This is release 0.5 of the xmllint Manual.</releaseinfo> -->\r\n"
	"	<!-- <edition>0.5</edition> -->\r\n"
	"	<user name = 'user34' value = 'zsx34' id = 'id34'> user zsx34 </user>\r\n"
	"</root>\r\n";

static const char *__data2 =
	"<?xml version=\"1.0\"?>\r\n"
	"<?xml-stylesheet type=\"text/xsl\"\r\n"
	"	href=\"http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl\"?>\r\n"
	"<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\"\r\n"
	"	\"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\" [\r\n"
	"	<!ENTITY xmllint \"<command>xmllint</command>\">\r\n"
	"]>\r\n"
	"<root>test\r\n"
	"	<!-- <edition> - <!--0.5--> - </edition> -->\r\n"
	"	<user>zsx\r\n"
	"		<age>38</age>\r\n"
	"	</user>\r\n"
	"</root>\r\n"
	"<!-- <edition><!-- 0.5 --></edition> -->\r\n"
	"<!-- <edition>0.5</edition> -->\r\n"
	"<!-- <edition> -- 0.5 -- </edition> -->\r\n"
	"<root name='root' id='root_id'>test</root>\r\n";

static const char *__data3 = "<root id='tt' >hello <root2> hi </root2></root><br/>\r\n";

static const char* __data4 = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
	"<request action=\"get_location\" sid=\"YOU_CAN_GEN_SID\" user=\"admin@test.com\">\r\n"
	"	<tags1>\r\n"
	"		<module name=\"mail_ud_user\" />\r\n"
	"	</tags1>\r\n"
	"	<tags2>\r\n"
	"		<tags21>\r\n"
	"		<tags22>\r\n"
	"		<tags23>\r\n"
	"		<module name=\"mail_ud_user\" />\r\n"
	"	</tags2>\r\n"
	"	<tag3>\r\n"
	"		<module name=\"mail_ud_user\">\r\n"
	"	</tag3>\r\n"
	"	<tag4>\r\n"
	"		<module name=\"mail_ud_user\">\r\n"
	"	</tag4>\r\n"
	"</request>\r\n";

static const char* __data5 = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
	"<request action=\"get_location\" sid=\"YOU_CAN_GEN_SID\" user=\"admin@test.com\">\r\n"
	"	<tag3>\r\n"
	"		<module name=\"mail_ud_user\" />\r\n"
	"	</tag3>\r\n"
	"	<tag4>\r\n"
	"		<module name=\"mail_ud_user\" />\r\n"
	"	</tag4>\r\n"
	"</request>\r\n";

static const char* __data6 = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
	"<request action=\"get_location\" sid=\"YOU_CAN_GEN_SID\" user=\"admin@test.com\">\r\n"
	"	<tags2>\r\n"
	"		<tags21>\r\n"
	"		<tags22>\r\n"
	"		<tags23 />\r\n"
	"		<tags24 />\r\n"
	"		<tags25>\r\n"
	"		<tags26/>\r\n"
	"		<tags27>\r\n"
	"		<tags28/>\r\n"
	"		<tags29>\r\n"
	"		<tags30>\r\n"
	"	</tags2>\r\n"
	"</request>\r\n";

static const char* __data7 = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
	"<request action=\"get_location\" sid=\"YOU_CAN_GEN_SID\" user=\"admin@test.com\">\r\n"
	"	<tags2>\r\n"
	"		<tags22>\r\n"
	"		<tags23>\r\n"
	"		<tags24>\r\n"
	"		<tags25/>\r\n"
	"		<tags26/>\r\n"
	"		<tags27>\r\n"
	"		<tags28>\r\n"
	"		<tags29/>\r\n"
	"		<tags30>\r\n"
	"		<tags31>\r\n"
	"	</tags2>\r\n"
	"</request>\r\n";

static void parse_xml_benchmark(int once, int max, const char *data)
{
	int   i;
	ACL_XML *xml = acl_xml_alloc();

	acl_xml_slash(xml, 1);

	ACL_METER_TIME("-------------bat begin--------------");
	for (i = 0; i < max; i++) {
		const char *ptr = data;

		if (once) {
			acl_xml_parse(xml, ptr);
		} else {
			/* 每次仅输入一个字节来分析 xml 数据 */
			while (*ptr != 0) {
				char  ch2[2];

				ch2[0] = *ptr;
				ch2[1] = 0;
				acl_xml_parse(xml, ch2);
				ptr++;
			}
		}
		if (i < 2 || i == 4) {
			printf("--------- dump xml --------------\n");
			acl_xml_dump(xml, ACL_VSTREAM_OUT);
			printf("--------- data src --------------\n");
			printf("%s", data);
			printf("--------- dump end --------------\n");
		}


		acl_xml_reset(xml);
	}
	ACL_METER_TIME("-------------bat end--------------");
	acl_xml_free(xml);
}

static void build_xml(ACL_XML *xml, const char *data)
{
	ACL_VSTRING *buf = acl_xml_build(xml, NULL);
	ACL_FILE *fp = acl_fopen("./build_xml.txt", "wb");

	acl_assert(fp);

	printf("--------------in build_xml -------------------\n");
	printf("%s\r\n", acl_vstring_str(buf));
	acl_fwrite(acl_vstring_str(buf), ACL_VSTRING_LEN(buf), 1, fp);
	printf("----------------------------------------------\n");
	printf("%s\r\n", data);
	acl_fclose(fp);
	printf("--------------end in build_xml----------------\n");
	acl_vstring_free(buf);
}

static void parse_xml(int once, const char *data)
{
	ACL_XML *xml = acl_xml_alloc();
	const char *ptr;
	ACL_ITER iter1;
	int   i, total, left;
	ACL_ARRAY *a;
	ACL_XML_NODE *pnode;

	ptr = data;

	if (once) {
		/* 一次性地分析完整 xml 数据 */
		ACL_METER_TIME("-------------once begin--------------");
		acl_xml_parse(xml, ptr);
	} else {
		/* 每次仅输入一个字节来分析 xml 数据 */
		ACL_METER_TIME("-------------stream begin--------------");
		while (*ptr != 0) {
			char  ch2[2];

			ch2[0] = *ptr;
			ch2[1] = 0;
			acl_xml_parse(xml, ch2);
			ptr++;
		}
	}
	ACL_METER_TIME("-------------end--------------");
	printf("enter any key to continue ...\n");
	getchar();

	if (acl_xml_is_complete(xml, "root")) {
		printf(">> Yes, the xml complete\n");
	} else {
		printf(">> No, the xml not complete\n");
	}

	total = xml->node_cnt;

	/* 遍历根结点的一级子结点 */
	acl_foreach(iter1, xml->root) {
		ACL_ITER iter2;

		ACL_XML_NODE *node = (ACL_XML_NODE*) iter1.data;
		printf("tag> %s, text: %s\n", STR(node->ltag), STR(node->text));

		/* 遍历一级子结点的二级子结点 */
		acl_foreach(iter2, node) {
			ACL_ITER iter3;
			ACL_XML_NODE *node2 = (ACL_XML_NODE*) iter2.data;

			printf("\ttag> %s, text: %s\n", STR(node2->ltag), STR(node2->text));

			/* 遍历二级子结点的属性 */
			acl_foreach(iter3, node2->attr_list) {
				ACL_XML_ATTR *attr = (ACL_XML_ATTR*) iter3.data;
				printf("\t\tattr> %s: %s\n", STR(attr->name), STR(attr->value));
			}
		}
	}

	printf("----------------------------------------------------\n");

	/* 从根结点开始遍历 xml 对象的所有结点 */

	acl_foreach(iter1, xml) {
		ACL_ITER iter2;
		ACL_XML_NODE *node = (ACL_XML_NODE*) iter1.data;

		for (i = 1; i < node->depth; i++) {
			printf("\t");
		}

		printf("tag> %s, text: %s\n", STR(node->ltag), STR(node->text));

		/* 遍历 xml 结点的属性 */
		acl_foreach(iter2, node->attr_list) {
			ACL_XML_ATTR *attr = (ACL_XML_ATTR*) iter2.data;

			for (i = 1; i < node->depth; i++) {
				printf("\t");
			}

			printf("\tattr> %s: %s\n", STR(attr->name), STR(attr->value));
		}
	}

	/* 根据标签名获得 xml 结点集合 */

	printf("--------- acl_xml_getElementsByTagName ----------\n");
	a = acl_xml_getElementsByTagName(xml, "user");
	if (a) {
		/* 遍历结果集 */
		acl_foreach(iter1, a) {
			ACL_XML_NODE *node = (ACL_XML_NODE*) iter1.data;
			printf("tag> %s, text: %s\n", STR(node->ltag), STR(node->text));
		}
		/* 释放数组对象 */
		acl_xml_free_array(a);
	}


	/* 查询属性名为 name, 属性值为 user2_1 的所有 xml 结点的集合 */

	printf("--------- acl_xml_getElementsByName ------------\n");
	a = acl_xml_getElementsByName(xml, "user2_1");
	if (a) {
		/* 遍历结果集 */
		acl_foreach(iter1, a) {
			ACL_XML_NODE *node = (ACL_XML_NODE*) iter1.data;
			printf("tag> %s, text: %s\n", STR(node->ltag), STR(node->text));
		}
		/* 释放数组对象 */
		acl_xml_free_array(a);
	}

	/* 查询属性名为 id, 属性值为 id2_2 的所有 xml 结点集合 */
	printf("----------- acl_xml_getElementById -------------\n");
	pnode = acl_xml_getElementById(xml, "id2_2");
	if (pnode) {
		printf("tag> %s, text: %s\n", STR(pnode->ltag), STR(pnode->text));
		/* 遍历该 xml 结点的属性 */
		acl_foreach(iter1, pnode->attr_list) {
			ACL_XML_ATTR *attr = (ACL_XML_ATTR*) iter1.data;
			printf("\tattr_name: %s, attr_value: %s\n",
				STR(attr->name), STR(attr->value));
		}

		pnode = acl_xml_node_next(pnode);
		printf("----------------- the id2_2's next node is ---------------------\n");
		if (pnode) {
			printf("-------------- walk node -------------------\n");
			/* 遍历该 xml 结点的属性 */
			acl_foreach(iter1, pnode->attr_list) {
				ACL_XML_ATTR *attr = (ACL_XML_ATTR*) iter1.data;
				printf("\tattr_name: %s, attr_value: %s\n",
						STR(attr->name), STR(attr->value));
			}

		} else {
			printf("-------------- null node -------------------\n");
		}
	}

	pnode = acl_xml_getElementById(xml, "id12");
	if (pnode) {
		int   ndel = 0, node_cnt;

		/* 删除该结点及其子结点 */
		printf(">>>before delete %s, total: %d\n", STR(pnode->ltag), xml->node_cnt);
		ndel = acl_xml_node_delete(pnode);
		node_cnt = xml->node_cnt;
		printf(">>>after delete id12(%d deleted), total: %d\n", ndel, node_cnt);

		printf("Enter any key to continue ...\r\n");
		getchar();
	}

	acl_foreach(iter1, xml) {
		ACL_XML_NODE *node = (ACL_XML_NODE*) iter1.data;
		printf(">>tag: %s\n", STR(node->ltag));
	}

	pnode = acl_xml_getElementById(xml, "id12");
	if (pnode) {
		printf("-------------- walk %s node -------------------\n", STR(pnode->ltag));
		/* 遍历该 xml 结点的属性 */
		acl_foreach(iter1, pnode->attr_list) {
			ACL_XML_ATTR *attr = (ACL_XML_ATTR*) iter1.data;
			printf("\tattr_name: %s, attr_value: %s\n",
					STR(attr->name), STR(attr->value));
		}
	} else {
		printf("---- the id12 be deleted----\n");
	}

	build_xml(xml, data);

	/* 释放 xml 对象 */
	left = acl_xml_free(xml);
	printf("free all node ok, total(%d), left is: %d\n", total, left);
}

static void parse_xml_file(const char *filepath, int once)
{
	char *data = acl_vstream_loadfile(filepath);
	ACL_VSTREAM *fp;
	char *ptr;
	ACL_XML *xml;
	struct timeval  begin, end;

	if (data == NULL)
		return;

	gettimeofday(&begin, NULL);

	/* 创建 xml 对象 */
	xml = acl_xml_alloc();

	ptr = data;

	if (once) {
		/* 一次性地分析完整 xml 数据 */
		acl_xml_parse(xml, ptr);
	} else {
		/* 每次仅输入一个字节来分析 xml 数据 */
		while (*ptr) {
			char  ch2[2];

			ch2[0] = *ptr;
			ch2[1] = 0;
			acl_xml_parse(xml, ch2);
			ptr++;
		}
	}

	gettimeofday(&end, NULL);

	printf("------ok, time: %ld seconds, %ld microseconds -------\r\n",
		(long) end.tv_sec - (long) begin.tv_sec,
		(long) end.tv_usec - (long) begin.tv_usec);


	fp = acl_vstream_fopen("dump.txt", O_RDWR | O_CREAT | O_TRUNC, 0600, 4096);

	/* 将 xml 对象转储至指定流中 */
	acl_xml_dump(xml, fp);

	acl_vstream_fclose(fp);
	acl_xml_free(xml);
	acl_myfree(data);
}

static void build_xml2(void)
{
	ACL_XML *xml = acl_xml_alloc();
	ACL_XML_NODE *node1, *node2, *node3;
	ACL_VSTRING *buf;

	node1 = acl_xml_create_node(xml, "users", "text1");
	acl_xml_node_add_child(xml->root, node1);
	(void) acl_xml_node_add_attr(node1, "name", "users list");

	node2 = acl_xml_create_node(xml, "user", "text11");
	acl_xml_node_add_child(node1, node2);
	acl_xml_node_add_attrs(node2, "name", "user11", "value", "zsx11", NULL);

	node3 = acl_xml_create_node(xml, "age", "text111");
	acl_xml_node_add_child(node2, node3);
	acl_xml_node_add_attrs(node3, "name", "user111", "value", "zsx111", NULL);

	node2 = acl_xml_create_node(xml, "user", "text2");
	acl_xml_node_add_child(node1, node2);
	acl_xml_node_add_attrs(node2, "name", "value2", "value", "zsx2", NULL);

	node2 = acl_xml_create_node(xml, "user", "text3");
	acl_xml_node_add_child(node1, node2);
	acl_xml_node_add_attrs(node2, "name", "value3", "value", "zsx3", NULL);

	buf = acl_xml_build(xml, NULL);
	printf("--------------------xml string-------------------\r\n");
	printf("%s\n", acl_vstring_str(buf));
	acl_vstring_free(buf);

	acl_xml_free(xml);
}

static void test1(void)
{
	const char* data = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
	"<?xml-stylesheet type=\"text/xsl\"\r\n"
	"\thref=\"http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl\"?>\r\n"
	"\t<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\"\r\n"
	"\t\"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\" [\r\n"
	"	<!ENTITY xmllint \"<command>xmllint</command>\">\r\n"
	"]>\r\n"
	"<root name1 = \"\\\"value1\\\"\" name2 = \"val\\ue2\" name3 = \"v\\al'u\'e3\" />\r\n";
	ACL_XML *xml = acl_xml_alloc();
	ACL_ITER node_it, attr_it;
	ACL_XML_NODE *node;
	const char *encoding, *type, *href;

	printf("data: %s\r\n", data);
	acl_xml_update(xml, data);
	acl_foreach(node_it, xml) {
		ACL_XML_NODE *tmp = (ACL_XML_NODE*) node_it.data;
		printf("tag: %s\r\n", STR(tmp->ltag));
		acl_foreach(attr_it, tmp->attr_list) {
			ACL_XML_ATTR *attr = (ACL_XML_ATTR*) attr_it.data;
			printf("\tattr_name: %s, attr_value: %s\r\n",
				STR(attr->name), STR(attr->value));
		}
	}

	printf("------------------------------------------------------\r\n");

	encoding = acl_xml_getEncoding(xml);
	type = acl_xml_getType(xml);
	node = acl_xml_getElementMeta(xml, "xml-stylesheet");
	if (node)
		href = acl_xml_getElementAttrVal(node, "href");
	else
		href = NULL;
	printf("xml encoding: %s, type: %s, href: %s\r\n",
		encoding ? encoding : "null", type ? type : "null",
		href ? href : "null");
	acl_xml_free(xml);
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help]"
		" -f {xml_file}\n"
		" -s[parse once]\n"
		" -M[use mempool]\n"
		" -b[benchmark] -m benchmark_max\n"
		" -p[print] data1|data2|data3|data4|data5|data6|data7\n"
		" -d[which data] data1|data2|data3|data4|data5|data6|data7\n",
		procname);
}

#ifdef WIN32
#define snprintf _snprintf
#endif

int main(int argc, char *argv[])
{
	int   ch, once = 0, bench_max = 10000;
	char  filepath[256];
	int   benchmark = 0, use_mempool = 0;
	const char *data = __data1;

	if (0) {
		test1(); getchar(); exit(0);
	}

	snprintf(filepath, sizeof(filepath), "xmlcatalog_man.xml");

	while ((ch = getopt(argc, argv, "hf:sbm:Mp:d:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 'f':
			snprintf(filepath, sizeof(filepath), "%s", optarg);
			break;
		case 's':
			once = 1;
			break;
		case 'b':
			benchmark = 1;
			break;
		case 'M':
			use_mempool = 1;
			break;
		case 'm':
			bench_max = atoi(optarg);
			break;
		case 'd':
			if (strcasecmp(optarg, "data2") == 0)
				data = __data2;
			else if (strcasecmp(optarg, "data3") == 0)
				data = __data3;
			else if (strcasecmp(optarg, "data4") == 0)
				data = __data4;
			else if (strcasecmp(optarg, "data5") == 0)
				data = __data5;
			else if (strcasecmp(optarg, "data6") == 0)
				data = __data6;
			else if (strcasecmp(optarg, "data7") == 0)
				data = __data7;
			break;
		case 'p':
			if (strcasecmp(optarg, "data1") == 0)
				printf("%s\n", __data1);
			else if (strcasecmp(optarg, "data2") == 0)
				printf("%s\n", __data2);
			else if (strcasecmp(optarg, "data3") == 0)
				printf("%s\n", __data3);
			else if (strcasecmp(optarg, "data4") == 0)
				printf("%s\n", __data4);
			else if (strcasecmp(optarg, "data5") == 0)
				printf("%s\n", __data5);
			else if (strcasecmp(optarg, "data6") == 0)
				printf("%s\n", __data6);
			else if (strcasecmp(optarg, "data7") == 0)
				printf("%s\n", __data7);
			return (0);
		default:
			break;
		}
	}

	if (use_mempool)
		acl_mem_slice_init(8, 1024, 100000,
				ACL_SLICE_FLAG_GC2 |
				ACL_SLICE_FLAG_RTGC_OFF |
				ACL_SLICE_FLAG_LP64_ALIGN);
	if (benchmark) {
		parse_xml_benchmark(once, bench_max, data);
		return (0);
	}

	parse_xml(once, data);
	parse_xml_file(filepath, once);
	build_xml2();

#ifdef	ACL_MS_WINDOWS
	printf("ok, enter any key to exit ...\n");
	getchar();
#endif
	return 0;
}
