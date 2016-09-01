#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <ctype.h>
#include <string.h>

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_dbuf_pool.h"
#include "stdlib/acl_msg.h"
#include "code/acl_urlcode.h"

#endif

static unsigned char enc_tab[] = "0123456789ABCDEF";

char *acl_url_encode(const char *str, ACL_DBUF_POOL *dbuf)
{
	int i, j, len, tmp_len;
	unsigned char *tmp;

	len = (int) strlen(str);
	tmp_len = len;

	if (dbuf != NULL)
		tmp = (unsigned char*) acl_dbuf_pool_alloc(dbuf, len + 1);
	else
		tmp = (unsigned char*) acl_mymalloc(len + 1);

	for (i = 0, j = 0; i < len; i++, j++) {
		tmp[j] = (unsigned char) str[i];
		if (tmp[j] == ' ')
			tmp[j] = '+';
		else if (!isalnum(tmp[j]) && strchr("_-.", tmp[j]) == NULL) {
			tmp_len += 3;
			if (dbuf != NULL) {
				unsigned char *t = (unsigned char*) 
					acl_dbuf_pool_alloc(dbuf, tmp_len);
				if (j > 0)
					memcpy(t, tmp, j);
				tmp = t;
			} else
				tmp = acl_myrealloc(tmp, tmp_len);

			tmp[j++] = '%';
			tmp[j++] = enc_tab[(unsigned char)str[i] >> 4];
			tmp[j] = enc_tab[(unsigned char)str[i] & 0x0F];
		}
	}

	tmp[j] = '\0';
	return (char*) tmp;
}

static unsigned char dec_tab[256] = {
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0,  0,  0,  0,  0,  0,
	0, 10, 11, 12, 13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0, 10, 11, 12, 13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,
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

char *acl_url_decode(const char *str, ACL_DBUF_POOL *dbuf)
{
	char *tmp;
	int i, len, pos = 0;

	len = (int) strlen(str);
	if (dbuf != NULL)
		tmp = (char*) acl_dbuf_pool_alloc(dbuf, len + 1);
	else
		tmp = (char*) acl_mymalloc(len + 1);

	for (i = 0; i < len; i++) {
		/* If we found a '%' character, then the next two are
		 * the character hexa code. Converting a hexadecimal
		 * code to their decimal is easy: The first character
		 * needs to be multiplied by 16 ( << 4 ), and the
		 * another one we just get the value from hextable variable
		 */
		if (str[i] == '+')
			tmp[pos] = ' ';
		else if (str[i] != '%')
			tmp[pos] = str[i];
		else if (i + 2 >= len) {  /* check boundary */
			tmp[pos++] = '%';  /* keep it */
			if (++i >= len)
				break;
			tmp[pos] = str[i];
			break;
		} else if (isalnum(str[i + 1]) && isalnum(str[i + 2])) {
			tmp[pos] = (dec_tab[(unsigned char) str[i + 1]] << 4)
				+ dec_tab[(unsigned char) str[i + 2]];
			i += 2;
		} else
			tmp[pos] = str[i];

		pos++;
	}

	tmp[pos] = '\0';
	return tmp;
}
