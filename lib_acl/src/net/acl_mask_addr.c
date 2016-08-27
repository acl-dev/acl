/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <limits.h>			/* CHAR_BIT */

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "net/acl_mask_addr.h"

#endif

/* acl_mask_addr - mask off a variable-length address */

void acl_mask_addr(unsigned char *addr_bytes,
	unsigned addr_byte_count, unsigned network_bits)
{
	unsigned char *p;

	if (network_bits > addr_byte_count * CHAR_BIT)
		acl_msg_panic("mask_addr: address byte count %d"
				" too small for bit count %d", 
				addr_byte_count, network_bits);

	p = addr_bytes + network_bits / CHAR_BIT;
	network_bits %= CHAR_BIT;

	/* "using unsigned" is just avoiding gcc6.1 warning */
	if (network_bits != 0)
		*p++ &= (unsigned char) ~0 << (unsigned)
			(CHAR_BIT - network_bits);

	while (p < addr_bytes + addr_byte_count)
		*p++ = 0;
}

