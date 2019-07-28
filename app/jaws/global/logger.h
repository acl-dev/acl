#ifndef	__LOGGER_INCLUDE_H__
#define	__LOGGER_INCLUDE_H__

#include "lib_acl.h"

#ifdef	__cplusplus
extern "C" {
#endif

void logger_set(const char *logger_name, ACL_AIO *aio);
void logger_end(void);

#ifdef	__cplusplus
}
#endif

#endif
