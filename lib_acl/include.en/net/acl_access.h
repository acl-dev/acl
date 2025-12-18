#ifndef	ACL_ACCESS_INCLUDE_H
#define	ACL_ACCESS_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_stdlib.h"

/**
 * Add IP address ranges to the access list.
 * @param data IP address range string. Format:
 *  10.0.0.1:10.0.250.1, 192.168.0.1:192.168.0.255
 * @param sep1 Separator between each IP address range, such as "," separator
 * @param sep2 Separator between high address and low address in
 *  each IP address range, such as ":" separator
 * @return Operation result. 0: success; < 0: failure
 * Note: This function is not thread-safe
 */
ACL_API int acl_access_add(const char *data, const char *sep1, const char *sep2);

/**
 * Read IP address string from configuration file and
 * automatically add IP address ranges to the list.
 * @param xcp Successfully parsed configuration file parser
 * @param name Variable name in xcp that stores the IP address string
 * @return Whether addition was successful. 0: success; < 0: failure.
 * Note: This function is not thread-safe
 */
ACL_API int acl_access_cfg(ACL_XINETD_CFG_PARSER *xcp, const char *name);

/**
 * Users can set their own log recording function. If this
 * function is not called, the library will automatically use the
 * default in aclMsg.c.
 * @param log_fn User's own log recording function.
 * Note: This function is not thread-safe
 */
ACL_API void acl_access_setup_logfn(void (*log_fn)(const char *fmt, ...));

/**
 * Check whether the given IP address is in the allowed access IP address list.
 * @param ip Format: 192.168.0.1
 * @return Whether it is in the allowed access list, != 0: yes; == 0: no.
 */
ACL_API int acl_access_permit(const char *ip);

/**
 * Print actual address range information.
 */
ACL_API void acl_access_debug(void);

#ifdef	__cplusplus
}
#endif

#endif
