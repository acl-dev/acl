#pragma once
#include "../acl_cpp_define.hpp"

#include "string.hpp"
#include "pipe_stream.hpp"

struct ACL_VSTRING;

namespace acl {

class ACL_CPP_API charset_conv : public pipe_stream
{
public:
	charset_conv(void);
	~charset_conv(void);

	/**
	 * 设置是否允许将无效的字符集直接拷贝
	 * @param onoff {bool} 当为 true 时，则转码过程中如果遇到了
	 *  非法字符集，则直接拷贝，否则则跳过，默认情况下是直接拷贝
	 */
	void set_add_invalid(bool onoff);

	/**
	 * 转换函数
	 * @param fromCharset {const char*} 源字符集
	 * @param toCharset {const char*} 目标字符集
	 * @param in {const char*} 输入的源数据地址(非空)
	 * @param n {size_t} 输入源数据的长度(>0)
	 * @param out {string*} 存储转换结果
	 * @return {bool} 转换是否成功
	 */
	bool convert(const char* fromCharset, const char* toCharset,
		const char* in, size_t n, string* out);

	/**
	 * 如果转换失败, 该函数返回出错原因
	 * @return {const char*} 出错原因
	 */
	const char* serror(void) const;

	/**
	 * 重置转码状态, 该解析器便可重复使用, 但在再次使用前需要调用
	 * set(from, to) 设置源字符集与目标字符集
	 */
	void reset(void);

	/* 流式分析过程：update_begin->update->update ... ->update_finish */

	/**
	 * 初始化流式分析的相关参数
	 * @param fromCharset {const char*} 源字符集
	 * @param toCharset {const char*} 目标字符集
	 * @return {bool} 初始化是否成功
	 */
	bool update_begin(const char* fromCharset, const char* toCharset);

	/**
	 * 以流式方式进行字符集转换
	 * @param in {const char*} 源字符串
	 * @param len {size_t} in 字符串长度
	 * @param out {string*} 存储转换结果
	 * @return {bool} 当前转换过程是否成功
	 */
	bool update(const char* in, size_t len, string* out);

	/**
	 * 流式转换结束后需要调用此函数提取最后的转换结果
	 * @param out {string*} 存储转换结果
	 */
	void update_finish(string* out);

	/**
	 * 创建字符集转换器
	 * @param fromCharset {const char*} 源字符集
	 * @param toCharset {const char*} 目标字符集
	 * @return {charset_conv*} 如果输入参数非法，或源字符集
	 *  与目标字符集相同，或不支持两个字符集间的转换则返回NULL，
	 *  用完后需要调用 delete 删除
	 */
	static charset_conv* create(const char* fromCharset,
	                const char* toCharset);

	// pipe_stream 虚函数重载

	virtual int push_pop(const char* in, size_t len,
		string* out, size_t max = 0);
	virtual int pop_end(string* out, size_t max = 0);
	virtual void clear();

private:
	bool m_addInvalid;  // 如果遇到无效的字符集，是否直接拷贝
	string  m_errmsg;
	string* m_pBuf;
	char  m_fromCharset[32];
	char  m_toCharset[32];
	void* m_iconv;
	ACL_VSTRING* m_pInBuf;
	ACL_VSTRING* m_pOutBuf;
	const char* m_pUtf8Pre;
};

} // namespace acl
