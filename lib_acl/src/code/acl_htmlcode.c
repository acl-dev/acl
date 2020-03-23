#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stdlib/acl_msg.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_token_tree.h"
#include "thread/acl_pthread.h"
#include "code/acl_htmlcode.h"

#endif

#ifndef ACL_CLIENT_ONLY

#include "uni2utf8.h"
#include "html_charset.h"

int acl_html_encode(const char *in, ACL_VSTRING *out)
{
	int  n = 0;
	const unsigned char *ptr = (const unsigned char*) in;
	unsigned int k;

	while (*ptr) {
		k = (unsigned char) (*ptr);
		if (k >= 128)
			ACL_VSTRING_ADDCH(out, k);
		else if (html_charmap[k] != NULL) {
			acl_vstring_strcat(out, html_charmap[k]);
			n++;
		} else 
			ACL_VSTRING_ADDCH(out, k);
		ptr++;
	}

	ACL_VSTRING_TERMINATE(out);
	return (n);
}

static ACL_TOKEN *__decode_token_tree = NULL;

#ifndef HAVE_NO_ATEXIT
static void html_decode_free(void)
{
	if (__decode_token_tree) {
		acl_token_tree_destroy(__decode_token_tree);
		__decode_token_tree = NULL;
	}
}
#endif

static void html_decode_init(void)
{
	size_t i, n = sizeof(html_tab) / sizeof(html_tab[0]);

	__decode_token_tree = acl_token_new();

	/* 暂且不兼容全角空格等字符 */

	for (i = 0; i < n; i++) {
#if 0
		if (acl_token_tree_word_match(__decode_token_tree,
			html_tab[i].txt) != NULL)
		{
			continue;
		}
#endif
		acl_token_tree_add(__decode_token_tree, html_tab[i].txt,
			ACL_TOKEN_F_STOP, &html_tab[i]);
	}

#ifndef HAVE_NO_ATEXIT
	/* 进程退出时调用 html_decode_free 释放内存资源 */
	atexit(html_decode_free);
#endif
}

static acl_pthread_once_t __decode_token_once = ACL_PTHREAD_ONCE_INIT;

static const char* markup_unescape(const char* in, ACL_VSTRING* out)
{
	unsigned int   n;
	char  temp[2], buf[7];

	while (*in != 0) {
		if (*in == '&' && *(in + 1) == '#'
			&& (sscanf(in, "&#%u%1[;]", &n, temp) == 2
			    || sscanf(in, "&#x%x%1[;]", &n, temp) == 2)
			&& n != 0)
		{
			int buflen = uni2utf8((unsigned int) n,
				buf, sizeof(buf));
			buf[buflen] = '\0';
			acl_vstring_strcat(out, buf);

			n = *(in + 2) == 'x' ? 3 : 2;
			while (isxdigit(in[n]))
				n++;
			if(in[n] == ';')
				n++;
			in += n;
		} else {
			ACL_VSTRING_ADDCH(out, (unsigned char) (*in));
			in++;
		}
	}

	return (in);
}

int acl_html_decode(const char *in, ACL_VSTRING *out)
{
	int   n = 0, len;
	const char *ptr = in, *pre;
	const ACL_TOKEN *token;
	const HTML_SPEC *spec;

	acl_pthread_once(&__decode_token_once, html_decode_init);
	if (__decode_token_tree == NULL)
		acl_msg_fatal("__decode_token_tree null");

	while (*ptr != 0) {
		pre = ptr;
		token = acl_token_tree_match(__decode_token_tree,
				&ptr, NULL, NULL);
		if (token == NULL) {
			pre = markup_unescape(pre, out);
			len = (int) (ptr - pre);
			if (len > 0)
				acl_vstring_memcat(out, pre, len);
			break;
		}
		spec = (const HTML_SPEC*) token->ctx;
		acl_assert(spec != NULL);

		len = (int) (ptr - pre - spec->len);
		if (len > 0)
			acl_vstring_memcat(out, pre, len);
		if (spec->ch > 255)
			acl_vstring_memcat(out, (char*) &spec->ch, sizeof(spec->ch));
		else
			ACL_VSTRING_ADDCH(out, (unsigned char) spec->ch);
		n++;
	}

	ACL_VSTRING_TERMINATE(out);
	return (n);
}

#endif /* ACL_CLIENT_ONLY */
