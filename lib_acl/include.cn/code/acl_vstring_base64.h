#ifndef ACL_VSTRING_BASE64_CODE_INCLUDE_H
#define ACL_VSTRING_BASE64_CODE_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include "../stdlib/acl_vstring.h"

/**
 * BASE64 编码函数
 * @param vp {ACL_VSTRING*} 存储编码后结果
 * @param in {const char*} 源数据
 * @param len {int} in 源数据的长度
 * @return {ACL_VSTRING*} 与 vp 相同
 */
ACL_API ACL_VSTRING *acl_vstring_base64_encode(ACL_VSTRING *vp,
	const char *in, int len);

/**
 * BASE64 解码函数
 * @param vp {ACL_VSTRING*} 存储解码后结果
 * @param in {const char*} 编码后的数据
 * @param len {int} in 数据长度
 * @return {ACL_VSTRING*} NULL: 解码失败; !=NULL: 解码成功且与 vp 相同地址
 */
ACL_API ACL_VSTRING *acl_vstring_base64_decode(ACL_VSTRING *vp,
	const char *in, int len);

#ifdef  __cplusplus
}
#endif

#endif
