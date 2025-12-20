#ifndef	ACL_TRACE_INCLUDE_H
#define	ACL_TRACE_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Save current process's stack trace to specified file.
 * @param filepath {const char*} Target file path
 */
void acl_trace_save(const char *filepath);

/**
 * Print current process's stack trace to log.
 */
void acl_trace_info(void);

#ifdef	__cplusplus
}
#endif

#endif
