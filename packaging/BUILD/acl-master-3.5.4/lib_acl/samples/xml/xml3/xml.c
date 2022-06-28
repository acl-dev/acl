#include <sys/mman.h>
#include <sys/stat.h>
#include "lib_acl.h"

static const char *__data1 =
  "<?xml version=\"1.0\"?>\r\n"
  "<?xml-stylesheet type=\"text/xsl\"\r\n"
  "\thref=\"http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl\"?>\r\n"
  "\t<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\"\r\n"
  "\t\"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\" [\r\n"
  "	<!ENTITY xmllint \"<command>xmllint</command>\">\r\n"
  "]>\r\n"
  "<root name='root1' id='root_id_1'>\r\n"
  "  <user name='user11\"' value='zsx11' id='id11'> user zsx11 </user>\r\n"
  "  <user name='user12' value='zsx12' id='id12'> user zsx12 \r\n"
  "    <age year='1972'>my age</age>\r\n"
  "    <other>\r\n"
  "      <email name='zsxxsz@263.net'/>"
  "      <phone>"
  "        <mobile number='111111'> mobile number </mobile>"
  "        <office number='111111'> office number </office>"
  "      </phone>"
  "    </other>"
  "  </user>\r\n"
  "  <user name='user13' value='zsx13' id='id13'> user zsx13 </user>\r\n"
  "</root>\r\n"
  "<root name='root2' id='root_id_2'>\r\n"
  "  <user name='user21' value='zsx21' id='id21'> user zsx21 </user>\r\n"
  "  <user name='user22' value='zsx22' id='id22'> user zsx22 \r\n"
  "    <!-- date should be the date of the latest change or the release version -->\r\n"
  "    <age year='1972'>my age</age>\r\n"
  "  </user>\r\n"
  "  <user name='user23' value='zsx23' id='id23'> user zsx23 </user>\r\n"
  "</root>\r\n"
  "<root name = 'root3' id = 'root_id_3'>\r\n"
  "  <user name = 'user31' value = 'zsx31' id = 'id31'> user zsx31 </user>\r\n"
  "  <user name = 'user32' value = 'zsx32' id = 'id32'> user zsx32 </user>\r\n"
  "  <user name = 'user33' value = 'zsx33' id = 'id33'> user zsx33 \r\n"
  "    <age year = '1978' month = '12' day = '11'> bao bao </age>\r\n"
  "  </user>\r\n"
  "  <!-- still a bit buggy output, will talk to docbook-xsl upstream to fix this -->\r\n"
  "  <!-- <releaseinfo>This is release 0.5 of the xmllint Manual.</releaseinfo> -->\r\n"
  "  <!-- <edition>0.5</edition> -->\r\n"
  "  <user name = 'user34' value = 'zsx34' id = 'id34'> user zsx34 </user>\r\n"
  "</root>\r\n";

static const char *__data2 =
  "<?xml version=\"1.0\"?>\r\n"
  "<?xml-stylesheet type=\"text/xsl\"\r\n"
  "	href=\"http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl\"?>\r\n"
  "<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\"\r\n"
  "  \"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\" [\r\n"
  "  <!ENTITY xmllint \"<command>xmllint</command>\">\r\n"
  "]>\r\n"
  "<root>test\r\n"
  "  <!-- <edition> - <!--0.5--> - </edition> -->\r\n"
  "  <user>zsx\r\n"
  "    <age>38</age>\r\n"
  "  </user>\r\n"
  "</root>\r\n"
  "<!-- <edition><!-- 0.5 --></edition> -->\r\n"
  "<!-- <edition>0.5</edition> -->\r\n"
  "<!-- <edition> -- 0.5 -- </edition> -->\r\n"
  "<root name='root' id='root_id'>test</root>\r\n";

static const char *__data3 = \
  "<root id='tt' >hello <root2> hi </root2></root><br/>\r\n";

static const char* __data4 = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
  "<request action=\"get_location\" sid=\"YOU_CAN_GEN_SID\" user=\"admin@test.com\">\r\n"
  "  <tags1>\r\n"
  "    <module name=\"mail_ud_user\" />\r\n"
  "  </tags1>\r\n"
  "  <tags2>\r\n"
  "    <tags21/>\r\n"
  "    <tags22 />\r\n"
  "    <tags23 />\r\n"
  "    <module name=\"mail_ud_user\" />\r\n"
  "  </tags2>\r\n"
  "  <tag3>\r\n"
  "    <module name=\"mail_ud_user\"></module>\r\n"
  "  </tag3>\r\n"
  "  <tag4>\r\n"
  "    <module name=\"mail_ud_user\"/>\r\n"
  "  </tag4>\r\n"
  "</request>\r\n"
  "<!-- <edition> -- 0.5 -- </edition> -->\r\n"
  "<!-- <edition> -- 0.5 -- </edition> -->\r\n"
  "<!-- <edition> -- 0.5 -- </edition> -->\r\n";

static const char* __data5 = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
  "<request action=\"get_location\" sid=\"YOU_CAN_GEN_SID\" user=\"admin@test.com\">\r\n"
  "  <tag3>\r\n"
  "    <module name=\"mail_ud_user\" />\r\n"
  "  </tag3>\r\n"
  "  <tag4>\r\n"
  "    <module name=\"mail_ud_user\" />\r\n"
  "  </tag4>\r\n"
  "</request>\r\n";

static const char* __data6 = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
  "<request action=\"get_location\" sid=\"YOU_CAN_GEN_SID\" user=\"admin@test.com\">\r\n"
  "  <tags2>\r\n"
  "    <tags21>\r\n"
  "    <tags22>\r\n"
  "    <tags23 />\r\n"
  "    <tags24 />\r\n"
  "    <tags25>\r\n"
  "    <tags26/>\r\n"
  "    <tags27>\r\n"
  "    <tags28/>\r\n"
  "    <tags29>\r\n"
  "    <tags30>\r\n"
  "  </tags2>\r\n"
  "</request>\r\n";

static const char* __data7 = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
  "<request action=\"get_location\" sid=\"YOU_CAN_GEN_SID\" user=\"admin@test.com\">\r\n"
  "  <tags2>\r\n"
  "   <tags22>\r\n"
  "   <tags23>\r\n"
  "   <tags24>\r\n"
  "   <tags25/>\r\n"
  "   <tags26/>\r\n"
  "   <tags27>\r\n"
  "   <tags28>\r\n"
  "   <tags29/>\r\n"
  "   <tags30>\r\n"
  "   <tags31>\r\n"
  "  </tags2>\r\n"
  "</request>\r\n";

 static const char* __data8 = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
 "<root> hello world! </root>\r\n";

static const char *__data9 =
  "<?xml version=\"1.0\"?>\r\n"
  "<?xml-stylesheet type=\"text/xsl\">\r\n";

static void parse_xml_benchmark(int once, int max, const char *data)
{
	int   i;
	size_t size = strlen(data) * 4;
	const char *mmap_file = "./local.map";
	ACL_XML2 *xml = acl_xml2_mmap_file(mmap_file, size, 1, NULL);

	acl_xml2_slash(xml, 1);

	ACL_METER_TIME("-------------bat begin--------------");
	for (i = 0; i < max; i++) {
		const char *ptr = data;

		if (once) {
			acl_xml2_parse(xml, ptr);
		} else {
			/* 每次仅输入一个字节来分析 xml 数据 */
			while (*ptr != 0) {
				char  ch2[2];

				ch2[0] = *ptr;
				ch2[1] = 0;
				acl_xml2_parse(xml, ch2);
				ptr++;
			}
		}
		/*
		if (i < 2 || i == 4) {
			printf("--------- dump xml --------------\n");
			acl_xml_dump(xml, ACL_VSTREAM_OUT);
			printf("--------- data src --------------\n");
			printf("%s", data);
			printf("--------- dump end --------------\n");
		}
		*/

		acl_xml2_reset(xml);
	}

	ACL_METER_TIME("-------------bat end--------------");
	acl_xml2_free(xml);

	printf("--------------benchmark_max: %d--------------\r\n", max);

	printf("Enter any key to continue ...\r\n");
	getchar();
}

static int parse_xml_file(const char *filepath)
{
	int   n;
	acl_int64 len;
	char  buf[10240];
	ACL_VSTREAM *fp = acl_vstream_fopen(filepath, O_RDONLY, 0600, 8192);
	ACL_XML2 *xml;
	const char *mmap_file = "./local.map";

	if (fp == NULL) {
		printf("open %s error %s\r\n", filepath, acl_last_serror());
		return -1;
	}
	len = acl_vstream_fsize(fp);
	if (len <= 0) {
		printf("fsize %s error %s\r\n", filepath, acl_last_serror());
		acl_vstream_close(fp);
		return -1;
	}
	len *= 4;

	xml = acl_xml2_mmap_file(mmap_file, len, 1, NULL);

	ACL_METER_TIME("-------------begin--------------");
	while (1) {
		n = acl_vstream_read(fp, buf, sizeof(buf) - 1);
		if (n == ACL_VSTREAM_EOF)
			break;
		buf[n] = 0;
		acl_xml2_update(xml, buf);
	}
	ACL_METER_TIME("-------------end--------------");

	acl_vstream_close(fp);

	if (acl_xml2_is_complete(xml, "root"))
		printf("Xml is complete OK, filepath: %s\r\n", filepath);
	else
		printf("Xml is not complete, filepath: %s\r\n", filepath);
	acl_xml2_free(xml);

	printf("Enter any key to continue ...\r\n");
	getchar();

	return 0;
}

static void walk_xml(ACL_XML2* xml)
{
	ACL_ITER iter1;

	/* 从根结点开始遍历 xml 对象的所有结点 */

	printf("-------------- walk_xml -----------------------\r\n");

	acl_foreach(iter1, xml) {
		int  i;
		ACL_ITER iter2;
		ACL_XML2_NODE *node = (ACL_XML2_NODE*) iter1.data;

		for (i = 1; i < node->depth; i++)
			printf("\t");

		printf("%s(%d): tag->%s, size: %ld\n", __FUNCTION__, __LINE__,
			node->ltag, (long) node->ltag_size);

		/* 遍历 xml 结点的属性 */
		acl_foreach(iter2, node->attr_list) {
			ACL_XML2_ATTR *attr = (ACL_XML2_ATTR*) iter2.data;

			for (i = 1; i < node->depth + 1; i++)
				printf("\t");

			printf("%s(%d): attr->%s=\"%s\"\n", __FUNCTION__,
				__LINE__, attr->name, attr->value);
		}

		for (i = 1; i < node->depth + 1; i++)
			printf("\t");
		printf("%s(%d): text->%s, size: %ld\n", __FUNCTION__,
			__LINE__, node->text, (long) node->text_size);
	}

	printf("-------------- walk_xml end -------------------\r\n");

	printf("Enter any key to continue ...\r\n");
	getchar();
}

static void xml_node_attrs(ACL_XML2_NODE* node, int n)
{
	int i;
	ACL_ITER iter;

	acl_foreach(iter, node->attr_list) {
		ACL_XML2_ATTR *attr = (ACL_XML2_ATTR*) iter.data;

		for (i = 0; i < n; i++)
			printf("\t");

		printf("%s(%d): attr->%s(%ld, %ld)=\"%s(%ld, %ld)\"\n",
			__FUNCTION__, __LINE__, attr->name,
			(long) attr->name_size, (long) strlen(attr->name),
			attr->value, (long) attr->value_size,
			(long) strlen(attr->value));

		if ((size_t) attr->name_size != strlen(attr->name)) {
			printf("%s(%d): name_size invalie: %ld, %ld, %s\r\n",
				__FUNCTION__, __LINE__,
				(long) attr->name_size,
				(long) strlen(attr->name), attr->name);
			assert(0);
		}
		if ((size_t) attr->value_size != strlen(attr->value)) {
			printf("%s(%d): value_size invalie, value: %s, "
				"len: %ld, %ld\r\n", __FUNCTION__, __LINE__,
				attr->value, (long) attr->value_size,
				(long) strlen(attr->value));
			assert(0);
		}
	}
}

static void walk_xml_node(ACL_XML2_NODE *node, int n)
{
	ACL_ITER iter;

	/* 遍历结点的子结点 */

	acl_foreach(iter, node) {
		int   i;
		ACL_XML2_NODE *child = (ACL_XML2_NODE*) iter.data;

		for (i = 0; i < n; i++)
			printf("\t");

		printf("%s(%d): tag->%s, size: %ld, %ld\n", __FUNCTION__,
			__LINE__, child->ltag, (long) child->ltag_size,
			(long) strlen(child->ltag));

		if ((size_t) child->ltag_size != strlen(child->ltag)) {
			printf("%s(%d): ltag_size invalid, ltag: %s, "
				"len: %ld, %ld\r\n", __FUNCTION__, __LINE__,
				child->ltag, (long) child->ltag_size,
				(long) strlen(child->ltag));
			assert(0);
		}

		xml_node_attrs(child, n + 1);

		for (i = 0; i < n + 1; i++)
			printf("\t");

		printf("%s(%d): text->%s, size: %ld, %ld\n", __FUNCTION__,
			__LINE__, child->text, (long) child->text_size,
			(long) strlen(child->text));
		if ((size_t) child->text_size != strlen(child->text)) {
			printf("%s(%d): text_size invalid: %ld, %ld, "
				"text: %s\r\n", __FUNCTION__, __LINE__,
				(long) child->text_size,
				(long) strlen(child->text), child->text);
			assert(0);
		}

		walk_xml_node(child, n + 1);
	}
}

static void list_xml_tags(ACL_XML2 *xml)
{
	ACL_ITER iter;

	printf("-------------- list xml's all tags --------------------\r\n");
	acl_foreach(iter, xml) {
		ACL_XML2_NODE *node = (ACL_XML2_NODE*) iter.data;
		printf("%s(%d): tag: %s\n", __FUNCTION__, __LINE__,
			node->ltag);
	}

	printf("-------------- list xml's all tags end ----------------\r\n");

	printf("Enter any key to continue ...\r\n");
	getchar();
}

static void test_getElementsByTagName(ACL_XML2 *xml, const char *tag)
{
	ACL_ARRAY *a;
	ACL_ITER iter;

	printf("--------- acl_xml2_getElementsByTagName ------------\n");

	a = acl_xml2_getElementsByTagName(xml, tag);
	if (a) {
		/* 遍历结果集 */
		acl_foreach(iter, a) {
			ACL_XML2_NODE *node = (ACL_XML2_NODE*) iter.data;
			printf("tag->%s, text: %s\n", node->ltag, node->text);
			xml_node_attrs(node, 1);
			walk_xml_node(node, 1);
		}

		/* 释放数组对象 */
		acl_xml2_free_array(a);
	}
	else
		printf(">>> not exist, tag: %s\r\n", tag);

	printf("--------- acl_xml2_getElementsByTagName end --------\n");

	printf("Enter any key to continue ...\r\n");
	getchar();
}

static ACL_XML2_NODE *test_getElementById(ACL_XML2 *xml, const char *id)
{
	ACL_ITER iter1;
	ACL_XML2_NODE *node = acl_xml2_getElementById(xml, id);

	printf("--------------- acl_xml2_getElementById ---------------\r\n");
	if (node == NULL) {
		printf(">>>id: %s not found\r\n", id);
		return NULL;
	}

	printf("tag-> %s, text: %s\n", node->ltag, node->text);

	/* 遍历该 xml 结点的属性 */
	acl_foreach(iter1, node->attr_list) {
		ACL_XML2_ATTR *attr = (ACL_XML2_ATTR*) iter1.data;
		printf("\tattr_name: %s, attr_value: %s\n",
			attr->name, attr->value);
	}

	printf("----------- the id2_2's next node is ------------\n");

	node = acl_xml2_node_next(node);

	if (node) {
		printf("-------------- walk node ----------------\n");

		/* 遍历该 xml 结点的属性 */
		acl_foreach(iter1, node->attr_list) {
			ACL_XML2_ATTR *attr = (ACL_XML2_ATTR*) iter1.data;
			printf("\tattr_name: %s, attr_value: %s\n",
				attr->name, attr->value);
		}

	} else
		printf("-------------- null node ----------------\n");

	printf("Enter any key to continue ...\r\n");
	getchar();

	return node;
}

static ACL_XML2 *get_xml(int once, const char *data,
	const char* root, int multi_root)
{
	size_t size = strlen(data) * 4;
	ACL_VSTRING *vbuf = acl_vstring_alloc(size);
	ACL_XML2 *xml;
	const char *left;

	printf("--------------------- xml data ------------------------\r\n");
	printf("{%s}\r\n", data);
	printf("--------------------- xml data end --------------------\r\n");
	printf("Enter any key to continue ...\r\n");
	getchar();

	xml = acl_xml2_alloc(vbuf);
	acl_xml2_multi_root(xml, multi_root);
	acl_xml2_decode_enable(xml, 1);
	acl_xml2_slash(xml, 1);

	if (once) {
		/* 一次性地分析完整 xml 数据 */
		ACL_METER_TIME("-------------once begin--------------");
		left = acl_xml2_parse(xml, data);
	} else {
		/* 每次仅输入一个字节来分析 xml 数据 */
		ACL_METER_TIME("-------------stream begin--------------");
		while (*data != 0) {
			char  ch2[2];

			ch2[0] = *data;
			ch2[1] = 0;
			left = acl_xml2_parse(xml, ch2);
			data++;
		}
	}

	ACL_METER_TIME("-------------end--------------");
	printf("---------------- left data ----------------------------\r\n");
	printf("{%s}\r\n", left ? left : "empty");
	printf("---------------- left data end ------------------------\r\n");

	printf("Enter any key to continue ...\r\n");
	getchar();

	if (acl_xml2_is_complete(xml, root))
		printf(">> Yes, the xml complete\n");
	else
		printf(">> No, the xml not complete, root tag: %s\n", root);

	printf("Enter any key to continue ...\r\n");
	getchar();

	return xml;
}

static void parse_xml(int once, const char *data,
	const char* root, int multi_root)
{
	ACL_XML2 *xml = get_xml(once, data, root, multi_root);
	ACL_XML2_NODE *node;
	int total = xml->node_cnt, left;
	const char *ptr;

	/* 遍历所有 xml 节点 */
	walk_xml(xml);

	/* 递归遍历 root 节点的所有子节点 */
	printf("-------------------- walk root node -------------------\r\n");
	walk_xml_node(xml->root, 0);
	printf("-------------------- walk root node end ---------------\r\n");

	printf("Enter any key to continue ...\r\n");
	getchar();

	/* 根据标签名获得 xml 结点集合 */

	/* 查询属性名为 name, 属性值为 user 的所有 xml 结点的集合 */
	test_getElementsByTagName(xml, "user");

	/* 查询属性名为 name, 属性值为 user2_1 的所有 xml 结点的集合 */
	test_getElementsByTagName(xml, "user2_1");

	/* 查询属性名为 id, 属性值为 id2_2 的所有 xml 结点集合 */
	(void) test_getElementById(xml, "id2_2");

	/* 查询属性名为 id, 属性值为 id2_3 的所有 xml 结点集合 */
	node = test_getElementById(xml, "id2_3");
	if (node) {
		int   ndel = 0, node_cnt;

		/* 删除该结点及其子结点 */
		printf(">>>before delete %s, total: %d\n",
			node->ltag, xml->node_cnt);
		ndel = acl_xml2_node_delete(node);
		node_cnt = xml->node_cnt;
		printf(">>>after delete id2_3(%d deleted), total: %d\n",
			ndel, node_cnt);
	}
	node = test_getElementById(xml, "id2_3");

	list_xml_tags(xml);

	printf("----------------- build xml -------------------------\r\n");
	ptr = acl_xml2_build(xml);
	printf("%s, len: %ld, %ld\r\n", ptr, (long) strlen(ptr),
		(long) (acl_vstring_end(xml->vbuf) - ptr));
	if (strlen(ptr) != (size_t) (acl_vstring_end(xml->vbuf) - ptr)) {
		printf("%s(%d): invalid xml size: %ld, %ld\r\n", __FUNCTION__,
			__LINE__, (long) strlen(ptr),
			(long) (acl_vstring_end(xml->vbuf) - ptr));
		assert(0);
	}

	printf("----------------- build xml end ---------------------\r\n");

	/* 释放 xml 对象 */
	acl_vstring_free(xml->vbuf);
	left = acl_xml2_free(xml);

	printf("Free all node ok, total(%d), left is: %d\n", total, left);
	printf("Enter any key to continue ...\r\n");
	getchar();
}

static void test1(void)
{
	const char* data = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
	"<?xml-stylesheet type=\"text/xsl\"\r\n"
	"\thref=\"http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl\"?>\r\n"
	"<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\"\r\n"
	"\t\"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\" [\r\n"
	"	<!ENTITY xmllint \"<command>xmllint</command>\">\r\n"
	"]>\r\n"
	"<root name1 = \"value1\" name2 = \"val\\ue2\" name3 = \"v\\al'ue3\">hello world!</root>\r\n";
	ACL_XML2 *xml;
	ACL_XML2_NODE *node;
	const char *encoding, *type, *href;
	size_t size = strlen(data) * 3;
	ACL_VSTRING *buf = acl_vstring_alloc(size);
	const char *ptr;

	xml = acl_xml2_alloc(buf);

	printf("------------------------------------------------------\r\n");

	printf("%s\r\n", data);

	acl_xml2_update(xml, data);

	printf("--------------- walk_xml_node ------------------------\r\n");

	walk_xml_node(xml->root, 0);

	printf("--------------- walk_xml_node end --------------------\r\n");

	printf("Enter any key to continue ...\r\n");
	getchar();

	printf("----------------- lookup meta data -------------------\r\n");
	encoding = acl_xml2_getEncoding(xml);
	type = acl_xml2_getType(xml);
	node = acl_xml2_getElementMeta(xml, "xml-stylesheet");
	if (node)
		href = acl_xml2_getElementAttrVal(node, "href");
	else
		href = NULL;

	printf("xml encoding: %s\r\n", encoding ? encoding : "null");
	printf("xml type: %s\r\n", type ? type : "null");
	printf("xml href: %s\r\n", href ? href : "null");

	printf("----------------- lookup meta data end ---------------\r\n");

	printf("Enter any key to continue ...\r\n");
	getchar();
	test_getElementsByTagName(xml, "root");

	printf("----------------- build xml -------------------------\r\n");
	ptr = acl_xml2_build(xml);
	printf("%s\r\n", ptr);
	printf("----------------- build xml end ---------------------\r\n");
	acl_vstring_free(buf);
	acl_xml2_free(xml);
}

static void build_xml(void)
{
	size_t size = 2000;
	ACL_VSTRING *vbuf = acl_vstring_alloc(size);
	ACL_XML2 *xml = acl_xml2_alloc(vbuf);
	ACL_XML2_NODE *node1, *node2, *node3;
	const char *buf;
	const char* pp = "<users name=\"users list\">text1<user name=\"user11\" value=\"zsx11\">text11<age name=\"user111\" value=\"zsx111\">text111</age></user><user name=\"value2\" value=\"zsx2\">text2</user><user name=\"value3\" value=\"zsx3\">text3</user></users>";

	/*
	vbuf->maxlen = size;
	*/

	node1 = acl_xml2_create_node(xml, "users", "text1");
	acl_xml2_node_add_child(xml->root, node1);
	(void) acl_xml2_node_add_attr(node1, "name", "users list");

	node2 = acl_xml2_create_node(xml, "user", "text11");
	acl_xml2_node_add_child(node1, node2);
	acl_xml2_node_add_attrs(node2, "name", "user11", "value", "zsx11", NULL);

	node3 = acl_xml2_create_node(xml, "age", "text111");
	acl_xml2_node_add_child(node2, node3);
	acl_xml2_node_add_attrs(node3, "name", "user111", "value", "zsx111", NULL);

	node2 = acl_xml2_create_node(xml, "user", "text2");
	acl_xml2_node_add_child(node1, node2);
	acl_xml2_node_add_attrs(node2, "name", "value2", "value", "zsx2", NULL);

	node2 = acl_xml2_create_node(xml, "user", "text3");
	acl_xml2_node_add_child(node1, node2);
	acl_xml2_node_add_attrs(node2, "name", "value3", "value", "zsx3", NULL);

	printf("--------------------xml string-------------------\r\n");
	buf = acl_xml2_build(xml);
	printf("[%s]\n", buf);

	if (strlen(buf) != (size_t) (acl_vstring_end(xml->vbuf) - buf)) {
		printf("invalid length: %ld, %ld, %p\r\n", (long) strlen(buf),
			(long) (acl_vstring_end(xml->vbuf) - buf),
			acl_vstring_end(xml->vbuf));
		assert(0);
	}
	printf("length: %ld, %ld\r\n", (long) strlen(buf),
		(long) (acl_vstring_end(xml->vbuf) - buf));

	if (strcmp(pp, buf) == 0)
		printf(">>>>>>>>OK<<<<<<<<<<<\r\n");
	else {
		printf(">>>>>>>>Error<<<<<<<<\r\n");
		assert(0);
	}

	printf("--------------------xml string end---------------\r\n");


	acl_vstring_free(vbuf);
	acl_xml2_free(xml);

	printf("Enter any key to continue ...\r\n");
	getchar();
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help]\r\n"
		" -s[parse once]\r\n"
		" -b benchmark_max\r\n"
		" -B build_xml\r\n"
		" -t [if test one xml]\r\n"
		" -f xml_file\r\n"
		" -m[if enable  multiple root xml node, default: no]\r\n"
		" -p[print] data1|data2|data3|data4|data5|data6|data7\r\n"
		" -P [if parse one xml with one data]\r\n"
		" -d[parse] data1|data2|data3|data4|data5|data6|data7|data8|data9\r\n",
		procname);
}

#ifdef WIN32
#define snprintf _snprintf
#endif

int main(int argc, char *argv[])
{
	int   ch, once = 0, multi_root = 0, benchmark_max = 0;
	int   parse_one = 0, build = 0, test_one = 0;
	const char *data = __data1;
	const char* root = "root";
	char  filepath[256];

//	acl_msg_stdout_enable(1);

	filepath[0] = 0;

	while ((ch = getopt(argc, argv, "hsp:d:mb:f:PBt")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 't':
			test_one = 1;
			break;
		case 'm':
			multi_root = 1;
			break;
		case 's':
			once = 1;
			break;
		case 'b':
			benchmark_max = atoi(optarg);
			break;
		case 'P':
			parse_one = 1;
			break;
		case 'B':
			build = 1;
			break;
		case 'd':
			if (strcasecmp(optarg, "data2") == 0) {
				data = __data2;
				root = "root";
			} else if (strcasecmp(optarg, "data3") == 0) {
				data = __data3;
				root = "root";
			} else if (strcasecmp(optarg, "data4") == 0) {
				data = __data4;
				root = "request";
			} else if (strcasecmp(optarg, "data5") == 0) {
				data = __data5;
				root = "request";
			} else if (strcasecmp(optarg, "data6") == 0) {
				data = __data6;
				root = "request";
			} else if (strcasecmp(optarg, "data7") == 0) {
				data = __data7;
				root = "request";
			} else if (strcasecmp(optarg, "data8") == 0) {
				data = __data8;
				root = "root";
			} else if (strcasecmp(optarg, "data9") == 0) {
				data = __data9;
				root = "root";
			}
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
			else if (strcasecmp(optarg, "data8") == 0)
				printf("%s\n", __data8);
			else if (strcasecmp(optarg, "data9") == 0)
				printf("%s\n", __data9);
			return (0);
		case 'f':
			snprintf(filepath, sizeof(filepath), "%s", optarg);
			break;
		default:
			break;
		}
	}

	if (benchmark_max > 0)
		parse_xml_benchmark(once, benchmark_max, data);

	if (test_one)
	{
		test1();

		printf("Enter any key to continue ...\r\n");
		getchar();
	}


	if (parse_one)
		parse_xml(once, data, root, multi_root);

	if (filepath[0] != 0)
		parse_xml_file(filepath);

	if (build)
		build_xml();

	printf("----OK, ENTER ANY KEY TO EXIT ----\n");
	getchar();
	return 0;
}
