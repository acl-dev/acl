#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
/**
 *
 * System Library.
 *
 */
#include <stdio.h>
#include <stdlib.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/**
 *
 * Util Library.
 *
 */
#include "stdlib/acl_msg.h"

/**
 *
 * Application Specific.
 *
 */
#include "unit_test/acl_unit_test.h"

#endif

static char __log_pre[] = "unit_test";

int aut_log_open(const char *pathname)
{
	const char *myname = "aut_log_open";

	if (pathname == NULL || *pathname == 0) {
		printf("%s: invalid log_file name\n", myname);
		return (-1);
	}

	acl_msg_open(pathname, __log_pre);
	return (0);
}

void aut_log_info(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);

	if ((var_aut_log_level & VAR_AUT_LOG_PRINT) != 0) {
		vprintf(format, ap);
		printf("\n");
	}
	if ((var_aut_log_level & VAR_AUT_LOG_FPRINT) != 0)
		acl_msg_info2(format, ap);

	va_end(ap);
}

void aut_log_warn(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	if ((var_aut_log_level & VAR_AUT_LOG_PRINT) != 0) {
		vprintf(format, ap);
		printf("\n");
	}
	if ((var_aut_log_level & VAR_AUT_LOG_FPRINT) != 0)
		acl_msg_warn2(format, ap);
	va_end(ap);
}

void aut_log_error(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	if ((var_aut_log_level & VAR_AUT_LOG_PRINT) != 0) {
		vprintf(format, ap);
		printf("\n");
	}
	if ((var_aut_log_level & VAR_AUT_LOG_FPRINT) != 0)
		acl_msg_error2(format, ap);
	va_end(ap);
}

void aut_log_fatal(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	if ((var_aut_log_level & VAR_AUT_LOG_PRINT) != 0) {
		vprintf(format, ap);
		printf("\n");
        abort();
	}

	acl_msg_fatal2(format, ap);
	va_end(ap);
}

void aut_log_panic(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	if ((var_aut_log_level & VAR_AUT_LOG_PRINT) != 0) {
		vprintf(format, ap);
		printf("\n");
        abort();
	}
	acl_msg_panic2(format, ap);
	va_end(ap);
}
