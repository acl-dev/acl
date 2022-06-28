#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <string.h>
#include "stdlib/acl_mymalloc.h"
#include "code/acl_gbcode.h"

#endif

#ifndef ACL_CLIENT_ONLY

#include "gb_jt2ft.h"
#include "gb_ft2jt.h"

static void gbtransfer(const unsigned short chartab[], const char *data, size_t dlen,
	char *buf, size_t size)
{
	const unsigned char *ptr_jt = (const unsigned char*) data;
	unsigned char *ptr_ft = (unsigned char*) buf;
	unsigned short n;

	while (dlen > 0 && size > 0) {
		if (*ptr_jt > 0x80) {
			n = chartab[*((const unsigned short*) ptr_jt)];
			if (n == 0xffff) {
				*ptr_ft++ = *ptr_jt++;
				dlen--;
				size--;
				if (dlen == 0 || size == 0)
					break;
				*ptr_ft++ = *ptr_jt++;
				dlen--;
				size--;
			} else if (size == 1) {
				*ptr_ft++ = *ptr_jt++;
				break;
			} else {
				*((unsigned short*) ptr_ft) = n;
				ptr_ft += 2;
				size -= 2;
				ptr_jt += 2;
				dlen -= 2;
			}
		} else {
			*ptr_ft++ = *ptr_jt++;
			dlen--;
			size--;
		}
	}
}

void acl_gbjt2ft(const char *data, size_t dlen, char *buf, size_t size)
{
	gbtransfer(__jt2ft_tab, data, dlen, buf, size);
}

void acl_gbft2jt(const char *data, size_t dlen, char *buf, size_t size)
{
	gbtransfer(__ft2jt_tab, data, dlen, buf, size);
}

#endif /* ACL_CLIENT_ONLY */
