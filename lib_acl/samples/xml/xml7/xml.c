#include <sys/mman.h>
#include <sys/stat.h>
#include "lib_acl.h"

static int parse_xml_file(const char *filepath)
{
	int   n;
	acl_int64 len;
	char  buf[10240];
	ACL_VSTREAM *in = acl_vstream_fopen(filepath, O_RDONLY, 0600, 8192);
	const char* outfile = "./out.xml";
	ACL_VSTREAM *out = acl_vstream_fopen(outfile, O_RDWR | O_CREAT | O_TRUNC, 0600, 8192);
	ACL_XML2 *xml;
	const char *mmap_file = "./local.map";
	const char* ptr;

	if (in == NULL) {
		printf("open %s error %s\r\n", filepath, acl_last_serror());
		return -1;
	}

	if (out == NULL)
	{
		printf("open %s error %s\r\n", outfile, acl_last_serror());
		acl_vstream_close(in);
		return -1;
	}

	len = acl_vstream_fsize(in);
	if (len <= 0) {
		printf("fsize %s error %s\r\n", filepath, acl_last_serror());
		acl_vstream_close(in);
		acl_vstream_close(out);
		return -1;
	}

	acl_vstream_printf(">>>file(%s)'s size: %lld\r\n", filepath, len);

	len *= 4;

	xml = acl_xml2_mmap_file(mmap_file, len, 10, NULL);

	len = 0;
	while (1) {
		n = acl_vstream_read(in, buf, sizeof(buf) - 1);
		if (n == ACL_VSTREAM_EOF)
			break;
		buf[n] = 0;
		acl_xml2_update(xml, buf);
		len += n;
	}

	acl_vstream_close(in);

	acl_vstream_printf(">>read size: %lld\r\n", len);

	ptr = acl_xml2_build(xml);
	if (ptr == NULL)
		printf("acl_xml2_build error\r\n");

	len = acl_vstring_end(xml->vbuf) - ptr;
	acl_vstream_printf(">>>build xml's size:%lld, strlen: %ld\r\n",
		len, (long) strlen(ptr));
	acl_vstream_printf(">>> ptr: {%s}\r\n", ptr);

	if (acl_vstream_writen(out, ptr, len) == ACL_VSTREAM_EOF) {
		printf("write error %s, len: %ld\r\n",
			acl_last_serror(), (long) len);
		return -1;
	}

	acl_vstream_close(out);
	acl_xml2_free(xml);

	return 0;
}

int main(int argc, char *argv[])
{
	unsigned char ch = -1;
	printf("unsigned char: %u\r\n", ch);
	if (argc < 2) {
		printf("usage: %s filepath\r\n", argv[0]);
		return 0;
	}

	acl_msg_stdout_enable(1);
	parse_xml_file(argv[1]);

	return 0;
}
