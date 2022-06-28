#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include <string.h>

#include "stdlib/acl_msg.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_token_tree.h"
#include "thread/acl_pthread.h"
#include "code/acl_xmlcode.h"

#endif

#include <stdio.h>
#include "uni2utf8.h"

static const char *__charmap[] = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, "&quot;", NULL, NULL, NULL, "&amp;", "&apos;",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, "&lt;", NULL, "&gt;", NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

int acl_xml_encode(const char *in, ACL_VSTRING *out)
{
	const unsigned char *ptr = (const unsigned char*) in;
	int  n = 0;
	size_t len = strlen(in);

	len += len / 2;

	ACL_VSTRING_SPACE(out, (int) len);

	while (*ptr) {
		if (__charmap[*ptr] != NULL) {
			acl_vstring_strcat(out, __charmap[*ptr]);
			n++;
		} else {
			ACL_VSTRING_ADDCH(out, *ptr);
		}
		ptr++;
	}

	ACL_VSTRING_TERMINATE(out);
	return n;
}

typedef struct {
	const char *str;
	const char *txt;
	size_t  len;
} XML_SPEC;

static const XML_SPEC __tab[] = {
	{ "<", "&lt;", sizeof("&lt;") - 1 },
	{ ">", "&gt;", sizeof("&gt;") - 1 },
	{ "&", "&amp;", sizeof("&amp;") - 1 },
	{ "\'", "&apos;", sizeof("&apos;") - 1 },
	{ "\"", "&quot;", sizeof("&quot;") - 1 },
	{ "\302\251", "&copy;", sizeof("&copy;") - 1 },
	{ "\302\256", "&reg;", sizeof("&reg;") - 1 },
	{ " ", "&nbsp;", sizeof("&nbsp;") - 1},
	{ 0, 0, 0 }
};

static ACL_TOKEN *__token_tree = NULL;

#ifndef HAVE_NO_ATEXIT
static void xml_decode_free(void)
{
	if (__token_tree) {
		acl_token_tree_destroy(__token_tree);
		__token_tree = NULL;
	}
}
#endif

static void xml_decode_init(void)
{
	int   i;

	__token_tree = acl_token_new();

	for (i = 0; __tab[i].str != 0; i++)
		acl_token_tree_add(__token_tree, __tab[i].txt,
			ACL_TOKEN_F_STOP, &__tab[i]);

#ifndef HAVE_NO_ATEXIT
	/* 进程退出时调用 html_decode_free 释放内存资源 */
	atexit(xml_decode_free);
#endif
}

static acl_pthread_once_t __token_once = ACL_PTHREAD_ONCE_INIT;

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
			int buflen = uni2utf8(n, buf, sizeof(buf));

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

	return in;
}

int acl_xml_decode(const char *in, ACL_VSTRING *out)
{
	int   n = 0, len = (int) strlen(in);
	const char *ptr = in, *pre;
	const ACL_TOKEN *token;
	const XML_SPEC *spec;

	ACL_VSTRING_SPACE(out, len);

	acl_pthread_once(&__token_once, xml_decode_init);
	if (__token_tree == NULL)
		acl_msg_fatal("__token_tree null");

	while (*ptr != 0) {
		pre = ptr;
		token = acl_token_tree_match(__token_tree, &ptr, NULL, NULL);
		if (token == NULL) {
			pre = markup_unescape(pre, out);
			len = (int) (ptr - pre);
			if (len > 0)
				acl_vstring_memcat(out, pre, len);
			break;
		}

		spec = (const XML_SPEC*) token->ctx;
		acl_assert(spec != NULL);

		len = (int) (ptr - pre - spec->len);
		if (len > 0)
			acl_vstring_memcat(out, pre, len);
		acl_vstring_strcat(out, spec->str);
		n++;
	}

	ACL_VSTRING_TERMINATE(out);
	return n;
}

/*--------------------------------------------------------------------------*/

#ifndef ACL_CLIENT_ONLY

size_t acl_xml_encode2(const char **in, size_t ilen, char *out, size_t olen)
{
	size_t olen_saved = olen;

	if (olen == 0)
		return 0;

	while (ilen > 0 && olen > 1) {
		unsigned char ch = (unsigned char)(**in);
		if (__charmap[ch] != NULL) {
			size_t n = strlen(__charmap[ch]);
			if (olen < n)
				break;
			memcpy(out, __charmap[ch], n);
			out += n;
			olen -= n;
		} else {
			*out = (char) ch;
			out++;
			olen--;
		}

		(*in)++;
		ilen--;
	}

	*out = '\0';
	return olen_saved - olen;
}

#if 0
const char *acl_xml_encode2(const char *in, size_t ilen,
	char **out, size_t *olen)
{
	const unsigned char *ptr = (const unsigned char*) in;
	const char *s;

	if (*olen == 0)
		return in;

	*olen -= 1;  /* reserve space for '\0' */

	while (ilen > 0 && *olen > 0) {
		if ((s = __charmap[*ptr]) != NULL) {
			size_t n = strlen(s);
			if (*olen < n)
				break;
			memcpy(*out, s, n);
			*out += n;
			*olen -= n;
		} else if (*olen > 0) {
			**out = *ptr;
			*out += 1;
			*olen -= 1;
		} else
			break;
		ptr++;
		ilen--;
	}

	**out = '\0';
	return (const char*) ptr;
}
#endif

/* return the left char of in, > 0 when the out buf is not enough */

static size_t copy_buf(char **out, size_t *olen, const char *in, size_t ilen)
{
	while (ilen > 0 && *olen > 1) {
		**out = *in++;
		*out += 1;
		ilen--;
		(*olen)--;
	}

	return ilen;
}

static const char *markup_unescape2(const char *in, char **out, size_t *size)
{
	unsigned int   n;
	char  temp[2], buf[7];

	while (*in != 0) {
		if (*in == '&' && *(in + 1) == '#'
			&& (sscanf(in, "&#%u%1[;]", &n, temp) == 2
			|| sscanf(in, "&#x%x%1[;]", &n, temp) == 2)
			&& n != 0)
		{
			size_t buflen = uni2utf8(n, buf, sizeof(buf));

			if (copy_buf(out, size, buf, buflen) > 0)
				break;

			n = *(in + 2) == 'x' ? 3 : 2;
			while (isxdigit(in[n]))
				n++;

			if(in[n] == ';')
				n++;
			in += n;
		} else {
			if (*size <= 0)
				break;
			**out = *in++;
			*out += 1;
			*size -= 1;
		}
	}

	return in;
}

const char *acl_xml_decode2(const char *in, char **out, size_t *size)
{
	size_t len;
	const char *ptr = in, *pre;
	const ACL_TOKEN *token;
	const XML_SPEC *spec;

	if (*size == 0)
		return in;

	*size -= 1;  /* reserve space for '\0' */

	acl_pthread_once(&__token_once, xml_decode_init);
	if (__token_tree == NULL)
		acl_msg_fatal("__token_tree null");

	while (*ptr != 0) {
		pre = ptr;
		token = acl_token_tree_match(__token_tree, &ptr, NULL, NULL);
		if (token == NULL) {
			pre = markup_unescape2(pre, out, size);
			if (*size == 0)
				break;

			if (ptr > pre)
				(void) copy_buf(out, size, pre, ptr - pre);
			break;
		}

		spec = (const XML_SPEC*) token->ctx;
		acl_assert(spec != NULL);

		len = ptr - pre - spec->len;
		if (len > 0)
			copy_buf(out, size, pre, len);

		if (*size == 0)
			break;

		(void) copy_buf(out, size, spec->str, strlen(spec->str));
		if (*size == 0)
			break;
	}

	**out = '\0';
	*out += 1;
	return ptr;
}

#endif /* ACL_CLIENT_ONLY */
