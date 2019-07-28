#ifndef	__ACL_XMLCODE_INCLUDE_H__
#define	__ACL_XMLCODE_INCLUDE_H__

#include "../stdlib/acl_define.h"
#include "../stdlib/acl_vstring.h"

#ifdef	__cplusplus
extern "C" {
#endif

ACL_API int acl_xml_encode(const char *in, ACL_VSTRING *out);
ACL_API int acl_xml_decode(const char *in, ACL_VSTRING *out);

#ifndef ACL_CLIENT_ONLY

/**
 * xml 字符编码器
 * @param in {const char**} 源串地址的地址，函数返回后该地址记录未被处理的内容地址，
 *  如果输出缓冲区足够大，则该地址将指向源的尾部的后一个位置
 * @param ilen {size_t} 源内容的数据长度
 * @param out {char*} 输出缓冲区，用来存储转码后的结果
 * @param olen {size_t} 输出缓冲区的大小
 * @return {size_t} 转码后存储在输出缓冲区内的数据长度(该长度有可能大于源数据长度)，
 *  当输出缓冲区长度为：
 *  1) == 0 时，返回 0
 *  2) == 1 时，返回 0，且最后一个字节被置 '\0'
 *  3) > 1 时，最后一个字节被置 '\0'，返回值 > 0(不包含最后的 '\0')
 *  注：
 *  1) 函数返回后 in 的地址会发生改变，指向下一个待处理的地址
 *  2) 调用者在调用前应先保存 in 的地址，在 in 中未被处理的剩余的长度计算方式：
 *     ilen -= in_saved - in;
 *  3) 虽然当 olen > 0 时内部自动会给 out 的尾部置 '\0‘，但返回的数据长度
 *     不包括最后的 '\0'
 */
ACL_API size_t acl_xml_encode2(const char** in, size_t ilen,
		char* out, size_t olen);

ACL_API const char *acl_xml_decode2(const char *in, char **out, size_t *size);

#endif /* ACL_CLIENT_ONLY */

#ifdef	__cplusplus
}
#endif

#endif
