#ifndef ACL_DIR_INCLUDE_H
#define ACL_DIR_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"

#if defined(_WIN32) || defined(_WIN64)

#if !defined(_UNICODE)

/**
 * dirent structure returned by readdir().
 */
struct dirent {
	char d_name[260];
};

/**
 * DIR type returned by opendir().  The members of this structure
 * must not be accessed by application programs.
 */
typedef struct {
    HANDLE        _d_hdir;              /**< directory handle */
    char         *_d_dirname;           /**< directory name */
    unsigned      _d_magic;             /**< magic cookie for verifying handle */
    unsigned      _d_nfiles;            /**< no. of files remaining in buf */
    char          _d_buf[sizeof(WIN32_FIND_DATA)];  /**< buffer for a single file */
} DIR;

/**
 * Prototypes.
 */
ACL_API DIR *opendir(const char *dirname);
ACL_API struct dirent *readdir(DIR *dir);
ACL_API int closedir(DIR *dir);
ACL_API void rewinddir(DIR *dir);

/* @: directory functions */

#define _topendir       opendir
#define _treaddir       readdir
#define _trewinddir     rewinddir
#define _tclosedir      closedir
#define _tDIR           DIR
#define _tdirent        dirent

#else  /* _UNICODE */

/* wdirent structure returned by wreaddir().
*/
struct wdirent {
	wchar_t d_name[260];
};

typedef struct {
    unsigned long _d_hdir;              /**< directory handle */
    wchar_t      *_d_dirname;           /**< directory name */
    unsigned      _d_magic;             /**< magic cookie for verifying handle */
    unsigned      _d_nfiles;            /**< no. of files remaining in buf */
    char          _d_buf[sizeof(WIN32_FIND_DATA)];  /**< buffer for a single file */
} wDIR;

ACL_API wDIR *wopendir(const wchar_t *dirname);
ACL_API struct wdirent *wreaddir(wDIR *dir);
ACL_API int wclosedir (wDIR *dir);
ACL_API void wrewinddir(wDIR *dir);

/* @: directory functions */

#define _topendir       wopendir
#define _treaddir       wreaddir
#define _trewinddir     wrewinddir
#define _tclosedir      wclosedir
#define _tDIR           wDIR
#define _tdirent        wdirent

#endif	/* _UNICODE */

#endif /* _WIN32 */

#ifdef __cplusplus
}
#endif

#endif

