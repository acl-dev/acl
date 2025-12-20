#ifndef ACL_GETOPT_INCLUDE_H
#define ACL_GETOPT_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"

extern ACL_API int   acl_optind;
extern ACL_API char *acl_optarg;

ACL_API void acl_getopt_init(void);
ACL_API int acl_getopt(int argc, char *argv[], const char *opts);

#if defined(_WIN32) || defined(_WIN64)
# define optind acl_optind
# define optarg acl_optarg
# define getopt_init acl_getopt_init
# define getopt acl_getopt
#endif

#ifdef __cplusplus
}
#endif
#endif

