#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "unit_test/acl_unit_test.h"

#endif

AUT_CMD_TOKEN *aut_line_peer_token(const AUT_LINE *test_line)
{
	const char *myname = "aut_line_peer_token";
	AUT_LINE *test_peer;

	if (test_line == NULL || test_line->arg_inner == NULL)
		aut_log_fatal("%s: input error", myname);

	test_peer = aut_line_peer(test_line);
	if (test_peer == NULL)
		return (NULL);

	return ((AUT_CMD_TOKEN *) test_peer->arg_inner);
}

AUT_LINE *aut_line_peer(const AUT_LINE *test_line)
{
	const char *myname = "aut_line_peer";
	AUT_LINE *test_peer;
	AUT_CMD_TOKEN *token, *token_peer;

	if (test_line == NULL || test_line->arg_inner == NULL)
		aut_log_fatal("%s(%d)->%s: input error",
				__FILE__, __LINE__, myname);

	/* 从 acl_test_line 中取出以前注册的参数 */
	token = (AUT_CMD_TOKEN *) test_line->arg_inner;

	/* 取得相配对的的指针 */
	test_peer = token->peer;
	if (test_peer == NULL || test_peer->arg_inner == NULL) {
		aut_log_fatal("%s(%d)->%s: peers null, cmd_name=%s, err=%s",
			__FILE__, __LINE__,
			myname,	test_line,
			test_peer == NULL ? "test_line null": "arg_inner null");
	}

	/* 从 acl_test_line 中取出以前注册的参数 */
	token_peer = (AUT_CMD_TOKEN *) test_peer->arg_inner;

	/* 比较 begin 与 end 是否是一对 */
	if (token_peer->peer != test_line)
		aut_log_fatal("%s(%d)->%s: not peers",
				__FILE__, __LINE__, myname);

	return (test_peer);
}

