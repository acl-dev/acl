#ifndef	ACL_BASE64_CODE_INCLUDE_H
#define	ACL_BASE64_CODE_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"

/**
 * BASE64 编码函数
 * @param plain_in {const char*} 输入的源内容数据
 * @param len {int} plain_in 的数据长度
 * @return {unsigned char*} BASE64编码后的数据，需用 acl_myfree 释放
 */
ACL_API unsigned char *acl_base64_encode(const char *plain_in, int len);

/**
 * BASE64 解码函数
 * @param code_in {const char*} 经BASE64编码后的数据
 * @param ppresult {char**} 如果解码成功，则存储解码结果，且不用时需用
 *  acl_myfree 来释放其内存空间
 * @return {int} -1: 表示解码失败且 *ppresult 指向NULL; >0: 表示解码后的数据内容
 *  长度，且 *ppresult 指向一新动态分配的内存区，内部存储解码结果，需用 acl_myfree
 *  释放 *ppresult 的动态内存
 */
ACL_API int acl_base64_decode(const char *code_in, char **ppresult);


#ifdef  __cplusplus
}
#endif

#endif

