#ifndef ACL_SCAN_DIR_INCLUDE_H
#define ACL_SCAN_DIR_INCLUDE_H

#include <sys/stat.h>

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
/**
 * Directory scanning type object.
 */
typedef struct ACL_SCAN_DIR ACL_SCAN_DIR;

/**
 * Callback function type used by directory scanning for user-defined operations.
 * @param scan {ACL_SCAN_DIR*} Directory scanner pointer
 * @param ctx {void*} User context pointer
 */
typedef int (*ACL_SCAN_DIR_FN)(ACL_SCAN_DIR *scan, void *ctx);

/**
 * During directory scanning, if an empty directory is encountered and the user
 * has set an automatic deletion callback, this callback function will notify
 * the user to delete the specified empty directory.
 * @param scan {ACL_SCAN_DIR*} Directory scanner pointer
 * @param ctx {void*} User context pointer
 */
typedef int (*ACL_SCAN_RMDIR_FN)(ACL_SCAN_DIR *scan, const char *path, void *ctx);

/**
 * Open a scan path and perform initial setup for subsequent acl_scan_dir operations.
 * @param path {const char*} Path string to open
 * @param recursive {int} Whether to recursively scan subdirectories
 * @return {ACL_SCAN_DIR*} NULL: Err; != NULL, OK
 */
ACL_API ACL_SCAN_DIR *acl_scan_dir_open(const char *path, int recursive);

/**
 * Open a scan path and perform initial setup for subsequent acl_scan_dir operations.
 * @param path {const char*} Path string to open
 * @param flags {unsigned} Flag bits, see ACL_SCAN_FLAG_XXX
 * @return {ACL_SCAN_DIR*} NULL: Err; != NULL, OK
 */
ACL_API ACL_SCAN_DIR *acl_scan_dir_open2(const char *path, unsigned flags);
#define ACL_SCAN_FLAG_RECURSIVE	(1 << 0)	/* Whether to recursively
						 *  scan */
#define ACL_SCAN_FLAG_RMDIR	(1 << 1)	/* Whether to automatically
						 *  delete empty
						 *  directories */

/**
 * Close the scanner.
 * @param scan {ACL_SCAN_DIR*} Scanner pointer
 */
ACL_API void acl_scan_dir_close(ACL_SCAN_DIR *scan);

/**
 * Reset the directory scanner's statistics and callback variables to 0.
 * @param scan {ACL_SCAN_DIR*} Scanner pointer
 */
ACL_API void acl_scan_dir_reset(ACL_SCAN_DIR *scan);

/**
 * Set callback functions for the scanner through this interface. The last parameter
 * must be a control flag set to ACL_SCAN_CTL_END to indicate the end of
 * the parameter list.
 * @param scan {ACL_SCAN_DIR*} Scanner pointer
 * @param name {int} First parameter name, ACL_SCAN_CTL_XXX
 */
ACL_API void acl_scan_dir_ctl(ACL_SCAN_DIR *scan, int name, ...);
#define ACL_SCAN_CTL_END	0  /**< Control end flag */
#define ACL_SCAN_CTL_FN		1  /**< Set ACL_SCAN_DIR_FN flag */
#define ACL_SCAN_CTL_CTX	2  /**< Set user context */
#define ACL_SCAN_CTL_RMDIR_FN	3  /**< Set delete directory callback function */

/**
 * Get the current absolute path (different from the path passed to
 * acl_scan_dir_open at program startup).
 * @param scan {ACL_SCAN_DIR*} Scanner pointer
 * @return {const char*} Absolute path, == NULL: err; != NULL, OK
 */
ACL_API const char *acl_scan_dir_path(ACL_SCAN_DIR *scan);

/**
 * Current scanned file name (without path). If scanning a directory, returns "\0".
 * @param scan {ACL_SCAN_DIR*} Scanner pointer
 * @return {const char*} File name
 */
ACL_API const char *acl_scan_dir_file(ACL_SCAN_DIR *scan);

/**
 * Total number of directories scanned so far.
 * @param scan {ACL_SCAN_DIR*}
 * @return {unsigned} Total number of directories
 */
ACL_API unsigned acl_scan_dir_ndirs(ACL_SCAN_DIR *scan);

/**
 * Total number of files scanned so far.
 * @param scan {ACL_SCAN_DIR*}
 * @return {unsigned} Total number of files
 */
ACL_API unsigned acl_scan_dir_nfiles(ACL_SCAN_DIR *scan);

/**
 * Total size of files scanned so far.
 * @param scan {ACL_SCAN_DIR*}
 * @return {acl_int64} -1: Error; >= 0: Ok
 */
ACL_API acl_int64 acl_scan_dir_nsize(ACL_SCAN_DIR *scan);

/**
 * Get the file or directory information currently scanned, similar to the
 * standard stat() function.
 * @param scan {ACL_SCAN_DIR*} Scanner pointer
 * @param sbuf {struct acl_stat*} Buffer pointer
 * @return {int} 0: Ok, -1: Error
 */
ACL_API int acl_scan_stat(ACL_SCAN_DIR *scan, struct acl_stat *sbuf);

/**
 * Get the information of the directory currently being scanned. This API is
 * similar to acl_scan_stat.
 * @param scan {ACL_SCAN_DIR*} Scanner pointer
 * @param sbuf {struct acl_stat*} Buffer pointer
 * @return {int} 0: Ok, -1: Error
 */
ACL_API int acl_scan_dir_stat(ACL_SCAN_DIR *scan, struct acl_stat *sbuf);

/**
 * Whether directory scanning is complete.
 * @param scan {ACL_SCAN_DIR*} Scanner pointer
 * @return {int} 0: indicates not finished; !=0: indicates finished
 */
ACL_API int acl_scan_dir_end(ACL_SCAN_DIR *scan);

/**
 * Push a path that needs to be scanned onto the stack.
 * @param scan {ACL_SCAN_DIR*} Scanner pointer
 * @param path {const char*} Path to push onto the stack
 * @return {int} 0: OK; -1: Err
 */
ACL_API int acl_scan_dir_push(ACL_SCAN_DIR *scan, const char *path);

/**
 * Pop the next path.
 * @param scan {ACL_SCAN_DIR*} Scanner pointer
 * @return {ACL_SCAN_DIR*} Returns the next object in the stack, == NULL: empty; != NULL, OK
 */
ACL_API ACL_SCAN_DIR *acl_scan_dir_pop(ACL_SCAN_DIR *scan);

/**
 * Get the next directory or file name from scan's current path. Note: this function
 * internally supports recursive scanning, and the recursive parameter in
 * acl_scan_dir_open is effective for this function.
 *  1) ".." and "." are automatically skipped
 *  2) Returns only the name, not the path. The path can be obtained via acl_scan_dir_path
 * @param scan {ACL_SCAN_DIR*} Scanner pointer
 * @return {const char*} Directory name or file name, != NULL: OK; == NULL, scanning finished
 */
ACL_API const char *acl_scan_dir_next(ACL_SCAN_DIR *scan);

/**
 * Get the next file name (without path, path can be obtained via acl_scan_dir_path).
 * This function internally supports recursive directory scanning, and the recursive
 * parameter in acl_scan_dir_open is effective for this function.
 * @param scan {ACL_SCAN_DIR*} Scanner pointer
 * @return {const char*} Returns the next scanned file name: !NULL, OK; NULL
 *  scanning finished, should stop scanning
 */
ACL_API const char *acl_scan_dir_next_file(ACL_SCAN_DIR *scan);

/**
 * Get the next directory name (without path, path can be obtained via acl_scan_dir_path).
 * This function internally supports recursive directory scanning, and the recursive
 * parameter in acl_scan_dir_open is effective for this function.
 * @param scan {ACL_SCAN_DIR*} Scanner pointer
 * @return {const char*} Returns the next scanned directory name: !NULL, OK;
 *  NULL scanning finished, should stop scanning
 */
ACL_API const char *acl_scan_dir_next_dir(ACL_SCAN_DIR *scan);

/**
 * Get the next directory or file name (without path, path can be obtained
 * via acl_scan_dir_path).
 * This function internally supports recursive directory scanning, and the
 * recursive parameter in acl_scan_dir_open is effective for this function.
 * @param scan {ACL_SCAN_DIR*} Scanner pointer
 * @param is_file {int*} When returning, the address stores a value indicating whether
 *  the scanned item is a file. If true, it's a file; otherwise, it's a directory.
 * @return {const char*} Returns the next scanned directory or file name:
 *  !NULL, OK; NULL scanning finished, should stop scanning
 */
ACL_API const char *acl_scan_dir_next_name(ACL_SCAN_DIR *scan, int *is_file);

/**
 * Get the total disk space size occupied by the current directory (in bytes).
 * This function internally supports recursive directory scanning, and the recursive
 * parameter in acl_scan_dir_open is effective for this function.
 * @param scan {ACL_SCAN_DIR*} Directory scanner when scanning directories
 * @param nfile {int*} Number of files scanned and recorded
 * @param ndir {int*} Number of directories scanned and recorded
 * @return {acl_int64} -1: Error; >= 0: Ok
 */
ACL_API acl_int64 acl_scan_dir_size2(ACL_SCAN_DIR *scan, int *nfile, int *ndir);

/**
 * Get the total disk space size occupied by the current directory (in bytes).
 * @param pathname {const char*} Directory path string
 * @param recursive {int} Whether to recursively scan subdirectories under the directory
 * @param nfile {int*} Number of files scanned and recorded
 * @param ndir {int*} Number of directories scanned and recorded
 * @return {acl_int64} -1: Error, >= 0: Ok
 */
ACL_API acl_int64 acl_scan_dir_size(const char *pathname, int recursive,
		int *nfile, int *ndir);

/**
 * Delete all files and directories under the specified path.
 * @param nfile {int*} Number of files scanned and recorded
 * @param ndir {int*} Number of directories scanned and recorded
 * This function internally supports recursive directory scanning, and the
 * recursive parameter in acl_scan_dir_open is effective for this function.
 * @param scan {ACL_SCAN_DIR*} Directory scanner when scanning directories
 * @return {acl_int64} >= 0: Total size of files and directories actually
 *  deleted (bytes); < 0: error.
 */
ACL_API acl_int64 acl_scan_dir_rm2(ACL_SCAN_DIR *scan, int *nfile, int *ndir);

/**
 * Delete all files and directories under the specified path.
 * @param pathname {const char*} Path string
 * @param recursive {int} Whether to recursively delete subdirectories and
 *  files under subdirectories
 * @param ndir {int*} If not NULL, the process will store the total number
 *  of directories deleted in *ndir
 * @param nfile {int*} If not NULL, the process will store the total number
 *  of files deleted in *nfile
 * @return {acl_int64} >= 0: Total size of files and directories actually
 *  deleted (bytes); < 0: error.
 */
ACL_API acl_int64 acl_scan_dir_rm(const char *pathname, int recursive,
		int *ndir, int *nfile);

#ifdef  __cplusplus
}
#endif

#endif
