#include <sys/mman.h>
#include <sys/stat.h>
#include "rapidxml.hpp"
//#include "rapidxml_iterators.hpp"
//#include "rapidxml_utils.hpp"
//#include "rapidxml_print.hpp"
#include "lib_acl.h"

static char *mmap_addr(int fd, size_t len)
{
	char *ptr;

	if (fd == -1)
	{
		printf("open error %s\r\n", acl_last_serror());
		return NULL;
	}

	printf(">>fd: %d, len: %ld\r\n", fd, len);
	ptr = (char*) mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == NULL) {
		printf("mmap error %s\r\n", acl_last_serror());
		return NULL;
	}

	return ptr;
}

static void ummap_addr(char *addr, size_t len)
{
	munmap(addr, len);
}

static int parse_xml_file(const char *filepath)
{
	acl_int64 len;
	ACL_VSTREAM *fp = acl_vstream_fopen(filepath, O_RDWR, 0600, 8192);
	char *addr;
	rapidxml::xml_document<> doc;

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

	addr = mmap_addr(ACL_VSTREAM_FILE(fp), (size_t) len);
	acl_vstream_close(fp);
	// printf("addr: %s\r\n", addr);

	if (addr == NULL)
	{
		printf("mmap_addr error %s\r\n", acl_last_serror());
		return -1;
	}

	ACL_METER_TIME("-------------begin--------------");
	doc.parse<0>(addr);
	ACL_METER_TIME("-------------end--------------");

	ummap_addr(addr, len);

	printf("Enter any key to continue ...\r\n");
	getchar();

	return 0;
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help]\r\n"
		" -f xml_file\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch;
	char  filepath[256];

	filepath[0] = 0;

	while ((ch = getopt(argc, argv, "hf:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 'f':
			snprintf(filepath, sizeof(filepath), "%s", optarg);
			break;
		default:
			break;
		}
	}

	if (filepath[0] != 0)
		parse_xml_file(filepath);

	return 0;
}
