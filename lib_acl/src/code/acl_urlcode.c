#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <ctype.h>
#include <string.h>

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "code/acl_urlcode.h"

#endif

static unsigned char hex_enc_table[] = "0123456789ABCDEF";

char *acl_url_encode(const char *str)
{
	const char *myname = "acl_url_encode";
	register int i, j, len, tmp_len;
	unsigned char *tmp;

	len = strlen(str);
	tmp_len = len;
	tmp = (unsigned char*) acl_mymalloc(len+1);
	if (tmp == NULL)
		acl_msg_fatal("%s(%d): malloc error", myname, __LINE__);

	for (i = 0, j = 0; i < len; i++, j++) {
		tmp[j] = (unsigned char)str[i];
		if (tmp[j] == ' ')
			tmp[j] = '+';
		else if (!isalnum(tmp[j]) && strchr("_-.", tmp[j]) == NULL) {
			tmp_len += 3;
			tmp = acl_myrealloc(tmp, tmp_len);
			if (!tmp)
				acl_msg_fatal("%s(%d): realloc error", myname, __LINE__);

			tmp[j++] = '%';
			tmp[j++] = hex_enc_table[(unsigned char)str[i] >> 4];
			tmp[j] = hex_enc_table[(unsigned char)str[i] & 0x0F];
		}
	}

	tmp[j] = '\0';

	return ((char*) tmp);
}

static unsigned char hex_dec_table[256] = {
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0,  0,  0,  0,  0,  0,
	0, 10, 11, 12, 13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0, 10, 11, 12, 13, 13, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

char *acl_url_decode(const char *str)
{
	const char *myname = "acl_url_decode";
	char *tmp;
	register int i, len, pos = 0;

	len = strlen(str);
	tmp = (char *) acl_mymalloc(len + 1);
	if (tmp == NULL)
		acl_msg_fatal("%s(%d): malloc error", myname, __LINE__);

	for (i = 0; i < len; i++) {
		/* If we found a '%' character, then the next two are the character
		 * hexa code. Converting a hexadecimal code to their decimal is easy:
		 * The first character needs to be multiplied by 16 ( << 4 ), and the
		 * another one we just get the value from hextable variable
		 */
		if ((str[i] == '%') && isalnum(str[i+1]) && isalnum(str[i+2])) {
			tmp[pos] = (hex_dec_table[(unsigned char) str[i+1]] << 4)
				+ hex_dec_table[(unsigned char) str[i+2]];
			i += 2;
		} else if (str[i] == '+')
			tmp[pos] = ' ';
		else
			tmp[pos] = str[i];

		pos++;
	}

	tmp[pos] = '\0';

	return (tmp);
}
