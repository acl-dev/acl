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

#include "stdlib/acl_vsprintf.h"
#include "stdlib/acl_mystring.h"

#endif

char *acl_lowercase(char *string)
{
	char *cp = string;

	if (string == NULL)
		return (NULL);

	while (*cp) {
		*cp = tolower(*cp);
		cp++;
	}

	return (string);
}

char *acl_lowercase2(char *string, size_t n)
{
	char *cp = string;

	if (string == NULL)
		return (NULL);

	while (*cp && n > 0) {
		*cp = tolower(*cp);
		cp++;
		n--;
	}

	return (string);
}

char *acl_lowercase3(const char *string, char *buf, size_t size)
{
	char *cp = buf;

	if (string == NULL || *string == 0 || buf == NULL)
		return(NULL);

	while (size > 1 && *string) {
		*cp++ = tolower(*string++);
		size--;
	}
	*cp = 0;

	return (buf);
}

char *acl_uppercase(char *string)
{
	char *cp = string;

	if (string == NULL)
		return (NULL);

	while (*cp) {
		*cp = toupper(*cp);
		cp++;
	}

	return (string);
}

char *acl_uppercase2(char *string, size_t n)
{
	char *cp = string;

	if (string == NULL)
		return (NULL);

	while (*cp && n > 0) {
		*cp = toupper(*cp);
		cp++;
		n--;
	}

	return (string);
}

char *acl_uppercase3(const char *string, char *buf, size_t size)
{
	char *cp = buf;

	if (string == NULL || *string == 0 || buf == NULL)
		return(NULL);

	while (size > 1 && *string) {
		*cp++ = toupper(*string++);
		size--;
	}
	*cp = 0;

	return (buf);
}

/* acl_mystrtok - safe tokenizer */

char *acl_mystrtok(char **src, const char *sep)
{
	char   *start = *src;
	char   *end;

	/*
	 * Skip over leading delimiters.
	 */
	start += strspn(start, sep);
	if (*start == 0) {
		*src = start;
		return (0);
	}

	/*
	 * Separate off one token.
	 */
	end = start + strcspn(start, sep);
	if (*end != 0)
		*end++ = 0;
	*src = end;
	return (start);
}

/* acl_mystrline */
char *acl_mystrline(char **src)
{
	char *start = *src;
	char *end = *src;
	int   squash = 0, nr = 0;

	if (start == NULL)
		return (NULL);

	while (*end) {
		switch (*end) {
		case '\\':
			squash = 1;
			break;
		case '\r':
			nr++;
			break;
		case '\n':
			if (squash == 0)
				goto TAG_LOOP_END;
			memmove(end - (squash + nr), end + 1, strlen(end + 1));
		default:
			squash = 0;
			nr = 0;
			break;
		}

		end++;
	}

TAG_LOOP_END:

	if (*end == '\n') {
		*(end - nr) = 0;
		*src = end + 1;  /* (*src) pointer to the next postion after '\n' */
	} else
		*src = 0;

	return (start);
}

char *acl_mystr_trim(char *str)
{
	size_t len;
	char *ptr = str;

	len = strlen(str);

	while (*ptr) {
		if (*ptr == ' ' || *ptr == '\t') {
			memmove(ptr, ptr + 1, len--);
		} else if (((*ptr) &0xff) == 0xa1 && ((*(ptr + 1)) & 0xff) == 0xa1) {
			/* 对于全角的空格为: '　', 即 0xa10xa1 */
			len--;
			memmove(ptr, ptr + 2, len--);
		} else {
			ptr++;
			len--;
		}
	}

	return (str);
}

int acl_mystr_strip(const char *haystack, const char *needle, char *buf, int bsize)
{
	const char *ptr_src;
	char *ptr_des, *ptr;
	int len, n, ncpy = 0;

	if (haystack == NULL || *haystack == 0 || needle == NULL
	    || *needle == 0 || buf == NULL || bsize <= 0)
		return(-1);

	ptr_src = haystack;
	ptr_des = buf;
	len     = strlen(needle);

	while(1) {
		ptr = strstr(ptr_src, needle);
		if (ptr == NULL) {
			n = strlen(ptr_src);
			if (bsize > n) {
				ACL_SAFE_STRNCPY(ptr_des, ptr_src, bsize);
				ncpy += n;
				*(ptr_des + n) = 0;
			}
			break;
		}
		n = ptr - ptr_src;
		if (bsize <= n)
			break;
		ACL_SAFE_STRNCPY(ptr_des, ptr_src, bsize);
		ncpy    += n;
		bsize   -= n;
		ptr_des += n;
		*ptr_des = 0;
		ptr_src += n + len;
	}

	return(ncpy);
}

int acl_mystr_truncate_byln(char *str_src)
{
	if (str_src == NULL)
		return (-1);

	while (*str_src) {
		if (*str_src == '\r' || *str_src == '\n') {
			*str_src = 0;
			break;
		}
		str_src++;
	}

	return (0);
}

/*---------------------------------------------------------------------------- 
 * 返回指针的当前位置, 并且删除路径中多余的 '/'(for unix) or '\\'(for windows)
 * 并且返回的当前指针所存储的字符为 '\0', 而到数第二个字符有可能为 '/' or '\\',
 * 也有可能不为 //'/' or '\\', 但在结果集中绝不会出现连续的 '/' or '\\'
 */
static char *path_str_strip(const char *psrc, char *pbuf, int sizeb)
{
	const   char *ptr_src = psrc;
	char    *ptr_obj;
	int     n;

	if (ptr_src == NULL || *ptr_src == 0 || pbuf == NULL || sizeb <= 0)
		return(NULL);

	ptr_obj = pbuf;
	n       = sizeb;

	while (*ptr_src && n > 0) {
		if (*ptr_src == PATH_SEP_C
		    && *(ptr_src + 1) == PATH_SEP_C)
			; /* skip any useless '/'(in unix) or '\\'(in windows) */
		else {
			*ptr_obj++ = *ptr_src;
			n--;
		}

		ptr_src++;
	}

	if (n <= 0)      /* 说明所给的缓冲区空间不够大 */
		return (NULL);

	/* 必须保证最后一个字符是以 '\0' 结束 */
	*ptr_obj = 0;

	return (ptr_obj);
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
		return (-1);
	return (0);
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
		return(-1);

	/* 为了保证最后一个字符肯定为 '/'(unix) or '\\'(windows), 需做如下处理 */

	if (*(ptr - 1) != PATH_SEP_C) {
		if (ptr >= pbuf + sizeb) /* 说明所给的内存空间不够 */
			return(-1);
		*ptr++ = PATH_SEP_C;
		*ptr = 0;
	}
	return(0);
}

int acl_dir_getpath(const char *pathname, char *pbuf, int bsize)
{
	char *ptr;
	int   n;

	if (pathname == NULL || pbuf == NULL || bsize <= 0)
		return (-1);
	
	n = acl_file_path_correct(pathname, pbuf, bsize);
	if (n < 0)
		return (-1);
	ptr = strrchr(pbuf, PATH_SEP_C);
	if (ptr != NULL)
		*ptr = 0;
	if (ptr == pbuf) { /* such as "/tmp.txt", I'll left "/" */
		if (bsize >= 2)
			*(ptr + 1) = 0;
	}

	return (0);
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
        return (sc - s);
}

#ifdef WIN32

acl_uint64 acl_atoui64(const char *str)
{
	return ((acl_uint64) _atoi64(str));
}

acl_int64 acl_atoi64(const char *str)
{
	return (_atoi64(str));
}

const char *acl_ui64toa(acl_uint64 value, char *buf, size_t size)
{
	if (size < 21)
		return (NULL);
	return (_ui64toa(value, buf, 10));
}

const char *acl_i64toa(acl_int64 value, char *buf, size_t size)
{
	if (size < 21)
		return (NULL);
	return (_i64toa(value, buf, 10));
}

#elif defined(ACL_UNIX)

acl_uint64 acl_atoui64(const char *str)
{
	return ((acl_uint64) strtoull(str, NULL, 10));
}

acl_int64 acl_atoi64(const char *str)
{
	return ((acl_int64) strtoull(str, NULL, 10));
}

const char *acl_ui64toa(acl_uint64 value, char *buf, size_t size)
{
	if (size < 21)
		return (NULL);

	snprintf(buf, size, "%llu", value);
	return (buf);
}

const char *acl_i64toa(acl_int64 value, char *buf, size_t size)
{
	if (size < 21)
		return (NULL);

	snprintf(buf, size, "%lld", value);
	return (buf);
}

#endif

static void x64toa(acl_uint64 val, char *buf, size_t size, unsigned radix, int is_neg)
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
	} while (firstdig < p); /* repeat until halfway */
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

