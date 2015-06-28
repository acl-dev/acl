#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>  /* just for EOF */
#include <fcntl.h>
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_file.h"

#endif

ACL_FILE *acl_fopen(const char *filename, const char *mode)
{
	ACL_FILE *fp;
	ACL_VSTREAM *stream;
	int   oflags = 0, whileflag;
#ifdef	ACL_WINDOWS
	int   commodeset = 0, scanset = 0;
#endif

	/* Skip leading spaces */
	while (*mode == ' ') {
		mode++;
	}

	/* First mode character must be 'r', 'w', or 'a'. */

	switch (*mode) {
	case 'r':
		oflags = O_RDONLY;
		break;
	case 'w':
		oflags = O_WRONLY | O_CREAT | O_TRUNC;
		break;
	case 'a':
		oflags = O_WRONLY | O_CREAT | O_APPEND;
		break;
	default:
		errno = EINVAL;
		acl_msg_error("Invalid file open mode");
		return NULL;
	}

	/* There can be up to three more optional mode characters:
	 * (1) A single '+' character,
	 * (2) One of 't' and 'b' and
	 * (3) One of 'c' and 'n'.
	 */

	whileflag = 1;

	while (*++mode && whileflag) {
		switch (*mode) {
		case ' ':
			/* skip spaces */
			break;
		case '+':
			if (oflags & O_RDWR)
				whileflag = 0;
			else {
				oflags |= O_RDWR;
				oflags &= ~(O_RDONLY | O_WRONLY);
			}
			break;
		case 'b':
#ifdef	ACL_WINDOWS
			if (oflags & (O_TEXT | O_BINARY))
				whileflag = 0;
			else
				oflags |= O_BINARY;
#endif
			break;
#ifdef	ACL_WINDOWS
		case 't':
			if (oflags & (O_TEXT | O_BINARY))
				whileflag = 0;
			else
				oflags |= O_TEXT;
			break;
		case ('c'):
			if (commodeset)
				whileflag = 0;
			else
				commodeset = 1;
			break;
		case 'n':
			if (commodeset)
				whileflag = 0;
			else
				commodeset = 1;
			break;
		case 'S':
			if (scanset)
				whileflag = 0;
			else {
				scanset = 1;
				oflags |= O_SEQUENTIAL;
			}
			break;
		case 'R':
			if (scanset)
				whileflag = 0;
			else {
				scanset = 1;
				oflags |= O_RANDOM;
			}
			break;
#if 0
		case 'T':
			if (oflags & O_SHORT_LIVED)
				whileflag = 0;
			else
				oflags |= O_SHORT_LIVED;
			break;
#endif
		case 'D':
			if (oflags & O_TEMPORARY)
				whileflag = 0;
			else
				oflags |= O_TEMPORARY;
			break;
		case 'N':
			oflags |= O_NOINHERIT;
			break;
#endif  /* ACL_WINDOWS */
		default:
			errno = EINVAL;
			acl_msg_error("Invalid file open mode");
			return NULL;
		}
	}

	stream = acl_vstream_fopen(filename, oflags, 0644, 4096);
	if (stream == NULL)
		return NULL;

	fp = (ACL_FILE*) acl_mymalloc(sizeof(ACL_FILE));
	fp->fp = stream;
	fp->status = 0;
	fp->errnum = 0;
	return fp;
}

void acl_clearerr(ACL_FILE *fp)
{
	fp->status = 0;
	fp->errnum = 0;
}

int acl_fclose(ACL_FILE *fp)
{
	int   ret = acl_vstream_fclose(fp->fp);

	acl_myfree(fp);
	if (ret == ACL_VSTREAM_EOF || ret < 0)
		return EOF;
	return (0);
}

int acl_feof(ACL_FILE *fp)
{
	return (fp->status & ACL_FILE_EOF) != 0;
}

size_t acl_fread(void *buf, size_t size, size_t nitems, ACL_FILE *fp)
{
	int   ret;

	if (size == 0 || nitems == 0)
		return 0;

	fp->status &= ~ACL_FILE_EOF;
	fp->errnum = 0;

	ret = acl_vstream_readn(fp->fp, buf, size * nitems);
	if (ret == ACL_VSTREAM_EOF) {
		fp->status |= ACL_FILE_EOF;
		return EOF;
	}

	ret /= (int) size;
	if (ret == (int) nitems)
		return (nitems);
	else if (ret == 0) {
		fp->status |= ACL_FILE_EOF;
		return EOF;
	} else
		return ret;
}

char *acl_fgets(char *buf, int size, ACL_FILE *fp)
{
	int   ret;

	fp->status &= ~ACL_FILE_EOF;
	fp->errnum = 0;

	ret = acl_vstream_gets(fp->fp, buf, size);
	if (ret == ACL_VSTREAM_EOF) {
		fp->status |= ACL_FILE_EOF;
		return NULL;
	}
	return buf;
}

char *acl_fgets_nonl(char *buf, int size, ACL_FILE *fp)
{
	int   ret;

	fp->status &= ~ACL_FILE_EOF;
	fp->errnum = 0;

	ret = acl_vstream_gets_nonl(fp->fp, buf, size);
	if (ret == ACL_VSTREAM_EOF) {
		fp->status |= ACL_FILE_EOF;
		return NULL;
	}
	return buf;
}

int acl_fgetc(ACL_FILE *fp)
{
	int   ret = ACL_VSTREAM_GETC(fp->fp);

	fp->status &= ~ACL_FILE_EOF;
	fp->errnum = 0;

	if (ret == ACL_VSTREAM_EOF) {
		fp->status |= ACL_FILE_EOF;
		return EOF;
	}
	return ret;
}

char *acl_gets(char *buf, size_t size)
{
	return acl_vstream_gets(ACL_VSTREAM_IN, buf, size)
		== ACL_VSTREAM_EOF ? NULL : buf;
}

char *acl_gets_nonl(char *buf, size_t size)
{
	return acl_vstream_gets_nonl(ACL_VSTREAM_IN, buf, size)
		== ACL_VSTREAM_EOF ? NULL : buf;
}

int acl_getchar()
{
	return acl_vstream_getc(ACL_VSTREAM_IN);
}

int acl_fprintf(ACL_FILE *fp, const char *fmt, ...)
{
	va_list ap;
	int   ret;

	va_start(ap, fmt);
	ret  = acl_vfprintf(fp, fmt, ap);
	va_end(ap);

	return ret;
}

int acl_vfprintf(ACL_FILE *fp, const char *fmt, va_list ap)
{
	int   ret;

	fp->status &= ~ACL_FILE_EOF;
	fp->errnum = 0;

	ret = acl_vstream_vfprintf(fp->fp, fmt, ap);
	if (ret == ACL_VSTREAM_EOF) {
		fp->status |= ACL_FILE_EOF;
		return EOF;
	}

	return ret;
}

size_t acl_fwrite(const void *ptr, size_t size, size_t nitems, ACL_FILE *fp)
{
	int   ret;

	if (size == 0 || nitems == 0)
		return 0;

	fp->status &= ~ACL_FILE_EOF;
	fp->errnum = 0;

	ret = acl_vstream_writen(fp->fp, ptr, size * nitems);
	if (ret == ACL_VSTREAM_EOF) {
		fp->status |= ACL_FILE_EOF;
		return EOF;
	}

	return nitems;
}

int acl_printf(const char *fmt, ...)
{
	va_list ap;
	int   ret;

	va_start(ap, fmt);
	ret  = acl_vprintf(fmt, ap);
	va_end(ap);

	return ret;
}

int acl_vprintf(const char *fmt, va_list ap)
{
	int   ret;

	ret = acl_vstream_vfprintf(ACL_VSTREAM_OUT, fmt, ap);
	if (ret == ACL_VSTREAM_EOF)
		return EOF;
	return ret;
}

int acl_fputs(const char *s, ACL_FILE *fp)
{
	int   ret;

	fp->status &= ~ACL_FILE_EOF;
	fp->errnum = 0;

	ret = acl_vstream_fputs(s, fp->fp);
	if (ret == ACL_VSTREAM_EOF) {
		fp->status |= ACL_FILE_EOF;
		return EOF;
	}

	return ret;
}

int acl_putc(int c, ACL_FILE *fp)
{
	int   ret;
	unsigned char ch = (unsigned char) c;

	fp->status &= ~ACL_FILE_EOF;
	fp->errnum = 0;

	ret = ACL_VSTREAM_PUTC(ch, fp->fp);
	if (ret == ACL_VSTREAM_EOF) {
		fp->status |= ACL_FILE_EOF;
		return EOF;
	}
	if (acl_vstream_fflush(ACL_VSTREAM_OUT) == ACL_VSTREAM_EOF) {
		fp->status |= ACL_FILE_EOF;
		return EOF;
	}
	return ret;
}

int acl_puts(const char *s)
{
	int   ret;

	ret = acl_vstream_fputs(s, ACL_VSTREAM_OUT);
	if (ret == ACL_VSTREAM_EOF)
		return EOF;
	return ret;
}

int acl_putchar(int c)
{
	int   ret;
	unsigned char ch = (unsigned char) c;

	ret = ACL_VSTREAM_PUTC(ch, ACL_VSTREAM_OUT);
	if (ret == ACL_VSTREAM_EOF)
		return EOF;
	if (acl_vstream_fflush(ACL_VSTREAM_OUT) == ACL_VSTREAM_EOF)
		return EOF;
	return ret;
}

acl_off_t acl_fseek(ACL_FILE *fp, acl_off_t offset, int whence)
{
	return acl_vstream_fseek(fp->fp, offset, whence);
}

acl_off_t acl_ftell(ACL_FILE *fp)
{
	return acl_vstream_ftell(fp->fp);
}
