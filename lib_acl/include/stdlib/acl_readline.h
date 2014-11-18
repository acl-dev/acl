#ifndef	ACL_READLINE_INCLUDE_H
#define	ACL_READLINE_INCLUDE_H

#include "acl_vstream.h"
#include "acl_vstring.h"

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * 从数据流中读取一个逻辑行数据. 空行将被忽略，如果一行的非空格起始字符为 "#" 则
 * 该行也被忽略；一个逻辑行的首个字符必须是非空格、非 "#" 字符，如果该行的后续行
 * 以空格或TAB开始，则该后续属于此逻辑行
 * @param buf {ACL_VSTRING*} 存储结果的缓冲区，不能为空
 * @param fp {ACL_VSTREAM*} 数据流句柄，不能为空
 * @param lineno {int} 如果非空，则记录该逻辑行在流中的真实行号
 * @return {ACL_VSTRING*} 如果未读到逻辑行，则返回空，否则返回输入 buf 的相同地址
 */
ACL_API ACL_VSTRING *acl_readlline(ACL_VSTRING *buf, ACL_VSTREAM *fp, int *lineno);

#ifdef	__cplusplus
}
#endif

#endif
