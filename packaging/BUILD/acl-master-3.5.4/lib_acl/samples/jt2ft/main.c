#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test(void)
{
	ACL_FILE *fp = acl_fopen("ft.txt", "w+");
	const char *jt = "³¡´¡·¡¾¡À¡Ã¡Å¡É¡abcdefg0123456";
	char  buf[256], buf2[256];
	unsigned short *ptr;

	printf(">>>jt: %s, len: %d\n", jt, (int) strlen(jt));
	memset(buf, 0, sizeof(buf));
	acl_gbjt2ft(jt, strlen(jt), buf, sizeof(buf));
	printf(">>>ft: %s, len: %d\n", buf, (int) strlen(buf));
	ptr = (unsigned short*) buf;
	/*
	while (*ptr != 0) {
		snprintf(buf, sizeof(buf), "0x%x,", *ptr);
		acl_fwrite(buf, strlen(buf), 1, fp);
		ptr++;
	}
	*/
	acl_fwrite(buf, strlen(buf), 1, fp);
	acl_fclose(fp);

	fp = acl_fopen("ft.txt", "r");
	if (fp == NULL) {
		printf("open file ft.txt error(%s)\n", acl_last_serror());
		return;
	}

	memset(buf, 0, sizeof(buf));
	acl_fread(buf, strlen(jt), 1, fp);
	printf(">>>ft: %s, len: %d\n", buf, (int) strlen(buf));
	acl_gbft2jt(buf, strlen(buf), buf, (int) strlen(buf) - 1);
	printf(">>>jt: %s, len: %d\n", buf, (int) strlen(buf));
	acl_gbft2jt(buf, strlen(buf), buf2, (int) strlen(buf) - 1);
	printf(">>>jt: %s, len: %d\n", buf2, (int) strlen(buf2));
	acl_fclose(fp);
}

int main(void)
{
	test();

#ifdef ACL_MS_WINDOWS
	printf("enter any key to exit ...\n");
	getchar();
#endif
	return (0);
}
