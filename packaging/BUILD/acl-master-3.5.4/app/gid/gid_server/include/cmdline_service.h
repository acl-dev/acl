#ifndef	__PROTO_CMD_INCLUDE_H__
#define	__PROTO_CMD_INCLUDE_H__

#include "lib_acl.h"

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * 命令行方式的协议处理方式
 * @param client {ACL_VSTREAM*} 客户端流
 * @return {int} 0：表示正常，1：表示正常且保持长连接，-1：表示出错
 */
int cmdline_service(ACL_VSTREAM *client);

#ifdef	__cplusplus
}
#endif

#endif
