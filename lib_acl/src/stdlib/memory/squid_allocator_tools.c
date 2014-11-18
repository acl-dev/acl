#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#include "squid_allocator.h"

void kb_incr(kb_t * k, unsigned int v)
{
	k->bytes += v;
	k->kb += (k->bytes >> 10);
	k->bytes &= 0x3FF;
	if (k->kb == 0) {
		/*
		 * If kb overflows and becomes negative then add powers of
		 * 2 until it becomes positive again.
		 */
		kb_t x;
		x.kb = 1 << 31;
		while (x.kb && ((k->kb + x.kb) == 0))
			x.kb <<= 1;
		k->kb += x.kb;
	}
}

void gb_flush(gb_t * g)
{
	g->gb += (g->bytes >> 30);
	g->bytes &= (1 << 30) - 1;
}

double gb_to_double(const gb_t * g)
{
	return ((double) g->gb) * ((double) (1 << 30)) + ((double) g->bytes);
}

const char *gb_to_str(const gb_t * g)
{
	/* it is often convenient to call gb_to_str
	 * several times for _one_ printf
	 */
#define max_cc_calls 5
	typedef char GbBuf[32];
	static GbBuf bufs[max_cc_calls];
	static int call_id = 0;
	double value = gb_to_double(g);
	char *buf = bufs[call_id++];
	if (call_id >= max_cc_calls)
		call_id = 0;
	/* select format */
	if (value < 1e9)
		snprintf(buf, sizeof(GbBuf), "%.2f MB", value / 1e6);
	else if (value < 1e12)
		snprintf(buf, sizeof(GbBuf), "%.2f GB", value / 1e9);
	else
		snprintf(buf, sizeof(GbBuf), "%.2f TB", value / 1e12);
	return buf;
}
