#ifndef __DIR_SYS_PATCH_INCLUDE_H__
#define __DIR_SYS_PATCH_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ACL_MS_WINDOWS
#include <direct.h>
#define __S_ISTYPE(mode, mask)  (((mode) & _S_IFMT) == (mask))
#ifndef	S_ISDIR
# define S_ISDIR(mode)    __S_ISTYPE((mode), _S_IFDIR)
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif

