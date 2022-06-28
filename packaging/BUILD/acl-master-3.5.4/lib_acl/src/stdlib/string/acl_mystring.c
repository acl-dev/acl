#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_vsprintf.h"
#include "stdlib/acl_mystring.h"

#endif

char *acl_lowercase(char *s)
{
	char *cp = s;

	if (s == NULL)
		return NULL;

	while (*cp) {
		*cp = tolower(*cp);
		cp++;
	}

	return s;
}

char *acl_lowercase2(char *s, size_t n)
{
	char *cp = s;

	if (s == NULL)
		return NULL;

	while (*cp && n > 0) {
		*cp = tolower(*cp);
		cp++;
		n--;
	}

	return s;
}

char *acl_lowercase3(const char *s, char *buf, size_t size)
{
	char *cp = buf;

	if (s == NULL || *s == 0 || buf == NULL)
		return NULL;

	while (size > 1 && *s) {
		*cp++ = tolower(*s++);
		size--;
	}
	*cp = 0;

	return buf;
}

char *acl_uppercase(char *s)
{
	char *cp = s;

	if (s == NULL)
		return NULL;

	while (*cp) {
		*cp = toupper(*cp);
		cp++;
	}

	return s;
}

char *acl_uppercase2(char *s, size_t n)
{
	char *cp = s;

	if (s == NULL)
		return (NULL);

	while (*cp && n > 0) {
		*cp = toupper(*cp);
		cp++;
		n--;
	}

	return s;
}

char *acl_uppercase3(const char *s, char *buf, size_t size)
{
	char *cp = buf;

	if (s == NULL || *s == 0 || buf == NULL)
		return(NULL);

	while (size > 1 && *s) {
		*cp++ = toupper(*s++);
		size--;
	}
	*cp = 0;

	return buf;
}

/* acl_strtok - safe tokenizer */

char *acl_strtok(char **src, const char *sep)
{
	char   *start = *src;
	char   *end;

	/*
	 * Skip over leading delimiters.
	 */
	start += strspn(start, sep);
	if (*start == 0) {
		*src = start;
		return 0;
	}

	/*
	 * Separate off one token.
	 */
	end = start + strcspn(start, sep);
	if (*end != 0)
		*end++ = 0;
	*src = end;
	return start;
}

char *acl_strline(char **src)
{
	char *start = *src;
	char *end = *src;
	int   backslash = 0, nr = 0;

	if (start == NULL)
		return (NULL);

	while (*end) {
		switch (*end) {
		case '\\':
			backslash = 1;
			break;
		case '\r':
			nr++;
			break;
		case '\n':
			if (backslash == 0)
				goto TAG_END;
			memmove(end - (backslash + nr), end + 1, strlen(end + 1));
			backslash = 0;
			nr = 0;
			break;
		default:
			backslash = 0;
			nr = 0;
			break;
		}

		end++;
	}

TAG_END:

	if (*end == '\n') {
		*(end - nr) = 0;

		/* (*src) pointer to the next postion after '\n' */
		*src = end + 1;
	} else
		*src = 0;

	return start;
}

char *acl_strtrim(char *str)
{
	size_t len;
	char *ptr = str;

	len = strlen(str);

	while (*ptr) {
		if (*ptr == ' ' || *ptr == '\t') {
			memmove(ptr, ptr + 1, len--);
		} else if (((*ptr) &0xff) == 0xa1
			&& ((*(ptr + 1)) & 0xff) == 0xa1)
		{
			/* 对于全角的空格为: '　', 即 0xa10xa1 */
			len--;
			memmove(ptr, ptr + 2, len--);
		} else {
			ptr++;
			len--;
		}
	}

	return str;
}

int acl_strstrip(const char *haystack, const char *needle,
	char *buf, int bsize)
{
	const char *src;
	char *des, *ptr;
	int len, n, ncpy = 0;

	if (haystack == NULL || *haystack == 0 || needle == NULL
	    || *needle == 0 || buf == NULL || bsize <= 0)
		return -1;

	src = haystack;
	des = buf;
	len = (int) strlen(needle);

	while(1) {
		ptr = strstr(src, needle);
		if (ptr == NULL) {
			n = (int) strlen(src);
			if (bsize > n) {
				ACL_SAFE_STRNCPY(des, src, bsize);
				ncpy += n;
				*(des + n) = 0;
			}
			break;
		}
		n = (int) (ptr - src);
		if (bsize <= n)
			break;
		ACL_SAFE_STRNCPY(des, src, bsize);
		ncpy    += n;
		bsize   -= n;
		des += n;
		*des = 0;
		src += n + len;
	}

	return ncpy;
}

int acl_strtrunc_byln(char *str)
{
	if (str == NULL)
		return -1;

	while (*str) {
		if (*str == '\r' || *str == '\n') {
			*str = 0;
			break;
		}
		str++;
	}

	return 0;
}

/*--------------------------------------------------------------------------
 * 返回指针的当前位置, 并且删除路径中多余的 '/'(for unix) or '\\'(for windows)
 * 并且返回的当前指针所存储的字符为 '\0', 而到数第二个字符有可能为 '/' or '\\',
 * 也有可能不为 //'/' or '\\', 但在结果集中绝不会出现连续的 '/' or '\\'
 */
static char *path_str_strip(const char *psrc, char *pbuf, int sizeb)
{
	const   char *ptr = psrc;
	char    *obj;
	int     n;

	if (ptr == NULL || *ptr == 0 || pbuf == NULL || sizeb <= 0)
		return NULL;

	obj = pbuf;
	n   = sizeb;

	while (*ptr && n > 0) {
		/* skip any useless '/'(in unix) or '\\'(in windows) */
		if (*ptr == PATH_SEP_C && *(ptr + 1) == PATH_SEP_C)
		    ;
		else {
			*obj++ = *ptr;
			n--;
		}

		ptr++;
	}

	if (n <= 0)      /* 说明所给的缓冲区空间不够大 */
		return NULL;

	/* 必须保证最后一个字符是以 '\0' 结束 */
	*obj = 0;

	return obj;
}
/*----------------------------------------------------------------------------
 * 保证结果类似于如下形式:
 * /home/avwall/test.txt
 */
int acl_file_path_correct(const char *psrc_file_path, char *pbuf, int sizeb)
{
	char    *ptr;

	ptr = path_str_strip(psrc_file_path, pbuf, sizeb);
	if (ptr == NULL)
		return -1;
	return 0;
}
/*----------------------------------------------------------------------------
 * 保证路径名经过此函数后都为如下格式:
 * 源:   /home/avwall/, /home//////avwall/, /home/avwall, /////home/avwall///
 *       /home/avwall////, /home///avwall///, ///home///avwall///
 * 结果: /home/avwall/
 */
int acl_dir_correct(const char *psrc_dir, char *pbuf, int sizeb)
{
	char    *ptr;

	/* 删除连续的 '/'(unix) or '\\'(windows) */
	ptr = path_str_strip(psrc_dir, pbuf, sizeb);

	/* 该函数若返回的结果不为空, 则 *ptr 定为 '\0' */
	if (ptr == NULL)
		return -1;

	/* 为了保证最后一个字符肯定为 '/'(unix) or '\\'(windows), 需做如下处理 */

	if (*(ptr - 1) != PATH_SEP_C) {
		if (ptr >= pbuf + sizeb) /* 说明所给的内存空间不够 */
			return -1;
		*ptr++ = PATH_SEP_C;
		*ptr = 0;
	}
	return 0;
}

int acl_dir_getpath(const char *pathname, char *pbuf, int bsize)
{
	char *ptr;
	int   n;

	if (pathname == NULL || pbuf == NULL || bsize <= 0)
		return -1;
	
	n = acl_file_path_correct(pathname, pbuf, bsize);
	if (n < 0)
		return -1;
	ptr = strrchr(pbuf, PATH_SEP_C);
	if (ptr != NULL)
		*ptr = 0;
	if (ptr == pbuf) { /* such as "/tmp.txt", I'll left "/" */
		if (bsize >= 2)
			*(ptr + 1) = 0;
	}

	return 0;
}

/**
 * strnlen - Find the length of a length-limited string
 * @s: The string to be sized
 * @count: The maximum number of bytes to search
 */
size_t acl_strnlen(const char * s, size_t count)
{
        const char *sc;

        for (sc = s; count-- && *sc != '\0'; ++sc)
                /* nothing */;
        return sc - s;
}

long long acl_atoll(const char *s)
{
	long long num = 0;
	int neg = 0;

	while (isspace(*s))
		s++;

	if (*s == '-') {	
		neg = 1;
		s++;
	}

	while (isdigit(*s)) {
		num = 10 * num + (*s - '0');
		s++;
	}

	if (neg)
		num = -num;
	return num;
}

#ifdef ACL_WINDOWS

acl_uint64 acl_atoui64(const char *str)
{
#if 0
	return (acl_uint64) _atoi64(str);
#else
	return (acl_uint64) acl_atoll(str);
#endif
}

acl_int64 acl_atoi64(const char *str)
{
#if 0
	return _atoi64(str);
#else
	return (acl_int64) acl_atoll(str);
#endif
}

const char *acl_ui64toa(acl_uint64 value, char *buf, size_t size)
{
	if (size < 21)
		return NULL;
	return _ui64toa(value, buf, 10);
}

const char *acl_i64toa(acl_int64 value, char *buf, size_t size)
{
	if (size < 21)
		return NULL;
	return _i64toa(value, buf, 10);
}

#elif defined(ACL_UNIX)

acl_uint64 acl_atoui64(const char *str)
{
#ifdef MINGW
	return (acl_uint64) acl_atoll(str);
#else
	return (acl_uint64) strtoull(str, NULL, 10);
#endif
}

acl_int64 acl_atoi64(const char *str)
{
#ifdef MINGW
	return (acl_int64) acl_atoll(str);
#else
	return (acl_int64) strtoull(str, NULL, 10);
#endif
}

const char *acl_ui64toa(acl_uint64 value, char *buf, size_t size)
{
	if (size < 21)
		return NULL;

	snprintf(buf, size, "%llu", value);
	return buf;
}

const char *acl_i64toa(acl_int64 value, char *buf, size_t size)
{
	if (size < 21)
		return NULL;

	snprintf(buf, size, "%lld", value);
	return buf;
}

#endif

static void x64toa(acl_uint64 val, char *buf, size_t size,
	unsigned radix, int is_neg)
{
	char *p;                /* pointer to traverse string */
	char *firstdig;         /* pointer to first digit */
	char temp;              /* temp char */
	unsigned digval;        /* value of digit */

	p = buf;

	if (is_neg) {
		*p++ = '-';     /* negative, so output '-' and negate */
		val = (acl_uint64) (-(acl_int64) val);
	}

	firstdig = p;           /* save pointer to first digit */

	do {
		if (size-- <= 0)
			break;

		digval = (unsigned) (val % radix);
		val /= radix;   /* get next digit */

		/* convert to ascii and store */
		if (digval > 9)
			*p++ = (char) (digval - 10 + 'a');  /* a letter */
		else
			*p++ = (char) (digval + '0');       /* a digit */
	} while (val > 0);

	/* We now have the digit of the number in the buffer, but in reverse
	   order.  Thus we reverse them now. */

	*p-- = '\0';            /* terminate string; p points to last digit */

	do {
		temp = *p;
		*p = *firstdig;
		*firstdig = temp;   /* swap *p and *firstdig */
		--p;
		++firstdig;         /* advance to next two digits */
	} while (firstdig < p);     /* repeat until halfway */
}

/* Actual functions just call conversion helper with neg flag set correctly,
   and return pointer to buffer. */

const char *acl_i64toa_radix(acl_int64 val, char *buf, size_t size, int radix)
{
	x64toa((acl_uint64)val, buf, size, radix, (radix == 10 && val < 0));
	return buf;
}

const char *acl_ui64toa_radix(acl_uint64 val, char *buf, size_t size, int radix)
{
	x64toa(val, buf, size, radix, 0);
	return buf;
}

/*--------------------------------------------------------------------------*/

static int get_blank_line(const char* s, int n, ACL_LINE_STATE* state)
{
	/* 如果还未找到换行符，则继续 */

	if (state->last_lf == 0) {
		while (n > 0) {
			state->last_ch = *s;
			state->offset++;
			n--;
			if (state->last_ch == '\n') {
				state->last_lf = '\n';
				break;
			}
			s++;
		}

		return n;
	}

	/* 如果数据以换行开始， 说明当前的空行找到 */
	if (*s == '\n') {
		/* 上次数据为: \n\r 或 \n */

		state->offset++;
		state->finish = 1;
		state->last_ch = '\n';
		return n - 1;
	}

	if (*s == '\r') {
		state->offset++;
		if (state->last_ch == '\r') {
			/* XXX: 出现了 \n\r\r 现象 */
			state->last_lf = 0;
			return n - 1;
		}

		/* 返回, 以期待下一个字符为 '\n' */
		state->last_ch = '\r';
		return n - 1;
	}

	/* 清除 '\n' */
	state->last_lf = 0;

	return n;
}

int acl_find_blank_line(const char *s, int n, ACL_LINE_STATE *state)
{
	int   ret;

	while (n > 0) {
		ret = get_blank_line(s, n, state);
		if (state->finish)
			return ret;
		if (ret == 0)
			return 0;
		s += n - ret;
		n = ret;
	}

	return  0;
}

ACL_LINE_STATE *acl_line_state_alloc(void)
{
	ACL_LINE_STATE *state = (ACL_LINE_STATE*)
		acl_mycalloc(1, sizeof(ACL_LINE_STATE));

	return state;
}

void acl_line_state_free(ACL_LINE_STATE *state)
{
	acl_myfree(state);
}

ACL_LINE_STATE *acl_line_state_reset(ACL_LINE_STATE *state, int offset)
{
	memset(state, 0, sizeof(ACL_LINE_STATE));
	state->offset = offset;
	return state;
}
