#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef  ACL_WINDOWS
#include <io.h>
#include <stdarg.h>
#include <errno.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <time.h>

#endif

#ifdef	ACL_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <signal.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_sys_patch.h"

#endif

#ifdef ACL_WINDOWS

ACL_FILE_HANDLE acl_file_open(const char *filepath, int flags, int mode)
{
	ACL_FILE_HANDLE fh;
	DWORD fileaccess = 0, fileshare = 0, filecreate = 0, fileattr = 0;

        /* decode the access flags */
	switch (flags & (O_RDONLY | O_WRONLY | O_RDWR)) {
	case O_RDONLY:         /* read access */
		fileaccess = GENERIC_READ;
		break;
	case O_WRONLY:         /* write access */
		fileaccess = GENERIC_WRITE;
		if ((flags & O_APPEND) != 0)
			fileaccess |= FILE_APPEND_DATA;
		break;
	case O_RDWR:           /* read and write access */
		fileaccess = GENERIC_READ | GENERIC_WRITE;
		if ((flags & O_APPEND) != 0)
			fileaccess = GENERIC_READ | FILE_APPEND_DATA;
		break;
	default:                /* error, bad flags */
		acl_set_error(ERROR_INVALID_PARAMETER);
		return ACL_FILE_INVALID;
	}

        /* decode open/create method flags */
	switch (flags & (O_CREAT | O_EXCL | O_TRUNC)) {
	case 0:
	case O_EXCL:            /* ignore EXCL w/o CREAT */
		filecreate = OPEN_EXISTING;
		break;
	case O_CREAT:
		filecreate = OPEN_ALWAYS;
		break;
	case O_CREAT | O_EXCL:
	case O_CREAT | O_TRUNC | O_EXCL:
		filecreate = CREATE_NEW;
		break;
	case O_TRUNC:
	case O_TRUNC | O_EXCL:  /* ignore EXCL w/o CREAT */
		filecreate = TRUNCATE_EXISTING;
		break;
	case O_CREAT | O_TRUNC:
		filecreate = CREATE_ALWAYS;
		break;
	default:
		/* this can't happen ... all cases are covered */
		acl_set_error(ERROR_INVALID_PARAMETER);
		return ACL_FILE_INVALID;
	}

	fileshare |= FILE_SHARE_READ | FILE_SHARE_WRITE;
	fileattr = FILE_ATTRIBUTE_NORMAL;

	fh = CreateFile(filepath, fileaccess, fileshare, NULL,
			filecreate, fileattr, NULL);
	return fh;
}

int acl_file_close(ACL_FILE_HANDLE fh)
{
	return CloseHandle(fh) ? 0 : -1;
}

acl_off_t acl_lseek(ACL_FILE_HANDLE fh, acl_off_t offset, int whence)
{
	const char *myname = "acl_lseek";
	LARGE_INTEGER li;
	DWORD method;

	if (whence == SEEK_CUR)
		method = FILE_CURRENT;
	else if (whence == SEEK_SET)
		method = FILE_BEGIN;
	else if (whence == SEEK_END)
		method = FILE_END;
	else {
		acl_msg_error("%s(%d): invalid whence(%d)",
			myname, __LINE__, whence);
		return -1;
	}

	li.QuadPart = offset;
	li.LowPart = SetFilePointer(fh, li.LowPart, &li.HighPart, method);

	if (li.LowPart == 0xFFFFFFFF && acl_last_error() != NO_ERROR) {
		li.QuadPart = -1;
	}

	return li.QuadPart;
}

int acl_file_read(ACL_FILE_HANDLE fh, void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *fp acl_unused,
	void *arg acl_unused)
{
	DWORD nRead = 0;

	if (!ReadFile(fh, buf, (DWORD) size, &nRead, NULL))
		return ACL_VSTREAM_EOF;

	return nRead;
}

int acl_file_write(ACL_FILE_HANDLE fh, const void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *fp acl_unused,
	void *arg acl_unused)
{
	DWORD nWritten = 0;

	if (!WriteFile(fh, buf, (DWORD) size, &nWritten, NULL))
		return ACL_VSTREAM_EOF;

	return nWritten;
}

int acl_file_writev(ACL_FILE_HANDLE fh, const struct iovec *vector,
	int count, int timeout acl_unused, ACL_VSTREAM *fp acl_unused,
	void *arg acl_unused)
{
	int   i, n;
	DWORD nWritten = 0;

	n = 0;
	for (i = 0; i < count; i++)	{
		if (!WriteFile(fh, vector[i].iov_base,
			(DWORD) vector[i].iov_len, &nWritten, NULL))
		{
			return ACL_VSTREAM_EOF;
		}
		else if (nWritten != vector[i].iov_len) {
			n += nWritten;
			break;
		}
		n += (int) vector[i].iov_len;
	}
	return n;
}

int acl_file_fflush(ACL_FILE_HANDLE fh, ACL_VSTREAM *fp acl_unused,
	void *arg acl_unused)
{
	if (FlushFileBuffers(fh))
		return 0;
	return -1;
}

acl_int64 acl_file_size(const char *filename)
{
	struct acl_stat sbuf;

	if (acl_stat(filename, &sbuf) == -1)
		return -1;
	return sbuf.st_size;
}

acl_int64 acl_file_fsize(ACL_FILE_HANDLE fh, ACL_VSTREAM *fp acl_unused,
	void *arg acl_unused)
{
	DWORD  nLow, nHigh;
	acl_int64 n;

	nLow = GetFileSize(fh, &nHigh);
	if (nLow == 0xFFFFFFFF)
		return -1;
	n = nHigh;
	return nLow + (n << 32);
}

/* this function comes from MS'S C Library */

int acl_fstat(ACL_FILE_HANDLE fh, struct acl_stat *buf)
{
	int isdev;          /* 0 for a file, 1 for a device */
	int retval = 0;     /* assume good return */
	BY_HANDLE_FILE_INFORMATION bhfi;
	FILETIME LocalFTime;
	SYSTEMTIME SystemTime;
	struct tm t;

	isdev = GetFileType(fh) & ~FILE_TYPE_REMOTE;
	if (isdev != FILE_TYPE_DISK) {
		if (isdev == FILE_TYPE_CHAR || isdev == FILE_TYPE_PIPE) {
			if (isdev == FILE_TYPE_CHAR)
				buf->st_mode = _S_IFCHR;
			else
				buf->st_mode = _S_IFIFO;

			buf->st_rdev = buf->st_dev = 0;
			buf->st_nlink = 1;
			buf->st_uid = buf->st_gid = buf->st_ino = 0;
			buf->st_atime = buf->st_mtime = buf->st_ctime = 0;
			if (isdev == FILE_TYPE_CHAR) {
				buf->st_size = 0;
			} else {
				unsigned long ulAvail;
				int rc;

				rc = PeekNamedPipe(fh, NULL, 0, NULL,
					&ulAvail, NULL);
				if (rc) {
					buf->st_size = (_off_t) ulAvail;
				} else {
					buf->st_size = (_off_t) 0;
				}
			}

			goto done;
		} else if (isdev == FILE_TYPE_UNKNOWN) {
			retval = -1;
			goto done;      /* join common return code */
		} else {
			retval = -1;
			goto done;
		}
	}

	/* set the common fields
	 */
	buf->st_ino = buf->st_uid = buf->st_gid = buf->st_mode = 0;
	buf->st_nlink = 1;

	/* use the file handle to get all the info about the file */
	if (!GetFileInformationByHandle(fh, &bhfi)) {
		retval = -1;
		goto done;
	}

	if (bhfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
		buf->st_mode |= (_S_IREAD + (_S_IREAD >> 3) + (_S_IREAD >> 6));
	else
		buf->st_mode |= ((_S_IREAD|_S_IWRITE)
			+ ((_S_IREAD|_S_IWRITE) >> 3)
			+ ((_S_IREAD|_S_IWRITE) >> 6));

	/* set file date fields */
	if (!FileTimeToLocalFileTime(&(bhfi.ftLastWriteTime), &LocalFTime)
		|| !FileTimeToSystemTime(&LocalFTime, &SystemTime ))
	{
		retval = -1;
		goto done;
	}

	t.tm_year = SystemTime.wYear - 1900;
	t.tm_mon = SystemTime.wMonth - 1;
	t.tm_mday = SystemTime.wDay;
	t.tm_hour = SystemTime.wHour;
	t.tm_min = SystemTime.wMinute;
	t.tm_sec = SystemTime.wSecond;
	buf->st_mtime = mktime(&t);

	if (bhfi.ftLastAccessTime.dwLowDateTime
		||bhfi.ftLastAccessTime.dwHighDateTime)
	{

		if (!FileTimeToLocalFileTime(
			&(bhfi.ftLastAccessTime), &LocalFTime )
			|| !FileTimeToSystemTime(&LocalFTime, &SystemTime))
		{
			retval = -1;
			goto done;
		}

		t.tm_year = SystemTime.wYear - 1900;
		t.tm_mon = SystemTime.wMonth - 1;
		t.tm_mday = SystemTime.wDay;
		t.tm_hour = SystemTime.wHour;
		t.tm_min = SystemTime.wMinute;
		t.tm_sec = SystemTime.wSecond;
		buf->st_atime = mktime(&t);
	}
	else
		buf->st_atime = buf->st_mtime;

	if (bhfi.ftCreationTime.dwLowDateTime
		|| bhfi.ftCreationTime.dwHighDateTime)
	{
		if (!FileTimeToLocalFileTime(&(bhfi.ftCreationTime), &LocalFTime)
			|| !FileTimeToSystemTime(&LocalFTime, &SystemTime))
		{
			retval = -1;
			goto done;
		}

		t.tm_year = SystemTime.wYear - 1900;
		t.tm_mon = SystemTime.wMonth - 1;
		t.tm_mday = SystemTime.wDay;
		t.tm_hour = SystemTime.wHour;
		t.tm_min = SystemTime.wMinute;
		t.tm_sec = SystemTime.wSecond;
		buf->st_ctime = mktime(&t);
	}
	else
		buf->st_ctime = buf->st_mtime;

	buf->st_size = ((__int64)(bhfi.nFileSizeHigh)) * (0x100000000i64)
					+ (__int64)(bhfi.nFileSizeLow);

	buf->st_mode |= _S_IFREG;

	/* On DOS, this field contains the drive number, but
	 * the drive number is not available on this platform.
	 * Also, for UNC network names, there is no drive number.
	 */
	buf->st_rdev = buf->st_dev = 0;

/* Common return code */
done:
	return retval;
}

#elif defined(ACL_UNIX)

ACL_FILE_HANDLE acl_file_open(const char *filepath, int flags, int mode)
{
	return open(filepath, flags, mode);
}

int acl_file_close(ACL_FILE_HANDLE fh)
{
	return close(fh);
}

acl_off_t acl_lseek(ACL_FILE_HANDLE fh, acl_off_t offset, int whence)
{
#if	defined(ACL_LINUX) || defined(ACL_SUNOS5)
# if    defined(MINGW)
	return lseek(fh, offset, whence);
# else
	return lseek64(fh, offset, whence);
# endif
#else
	return lseek(fh, offset, whence);
#endif
}

int acl_file_read(ACL_FILE_HANDLE fh, void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *fp acl_unused,
	void *arg acl_unused)
{
	return read(fh, buf, size);
}

int acl_file_write(ACL_FILE_HANDLE fh, const void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *fp acl_unused,
	void *arg acl_unused)
{
	return write(fh, buf, size);
}

int acl_file_writev(ACL_FILE_HANDLE fh, const struct iovec *vector, int count,
	int timeout acl_unused, ACL_VSTREAM *fp acl_unused,
	void *arg acl_unused)
{
	return writev(fh, vector, count);
}

int acl_file_fflush(ACL_FILE_HANDLE fh, ACL_VSTREAM *fp acl_unused,
	void *arg acl_unused)
{
	return fsync(fh);
}

acl_int64 acl_file_size(const char *filename)
{
	struct acl_stat sbuf;

	if (acl_stat(filename, &sbuf) == -1)
		return -1;
	return sbuf.st_size;
}

acl_int64 acl_file_fsize(ACL_FILE_HANDLE fh, ACL_VSTREAM *fp acl_unused,
	void *arg acl_unused)
{
	struct acl_stat sbuf;

	if (acl_fstat(fh, &sbuf) == -1)
		return -1;
	return sbuf.st_size;
}

#else
# error "unknown OS type"
#endif
