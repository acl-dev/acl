#ifndef ACL_HEX_CODE_INCLUDE_H
#define ACL_HEX_CODE_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_vstring.h"

/**
 * 将二进制数据进行编码，一个字节转换成两个字节后，从而转为文本字符串
 * @param buf {ACL_VSTRING*} 存储转换结果
 * @param ptr {const char*} 二进制数据
 * @param len {int} ptr 数据的长度
 * @return {ACL_VSTRING*} 如果转换成功，则与 buf 相同
 */
ACL_API ACL_VSTRING *acl_hex_encode(ACL_VSTRING *buf, const char *ptr, int len);

/**
 * 将编码后的数据进行解码
 * @param buf {ACL_VSTRING*} 存储转换结果
 * @param ptr {const char*} 编码数据
 * @param len {int} ptr 数据长度
 * @return {ACL_VSTRING*} 如果解码成功，则与 buf 相同, 否则返回 NULL
 */
ACL_API ACL_VSTRING *acl_hex_decode(ACL_VSTRING *buf, const char *ptr, int len);

#ifdef  __cplusplus
}
#endif

#endif

