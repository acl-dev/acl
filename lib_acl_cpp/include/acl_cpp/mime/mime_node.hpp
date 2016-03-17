#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <stdlib.h>
#include <map>
#include "acl_cpp/stdlib/string.hpp"

struct MIME_NODE;

namespace acl {

class pipe_manager;
class ostream;
class ifstream;

class ACL_CPP_API mime_node
{
public:
	/**
	 * 构造函数
	 * @param emailFile {const char*} 存储邮件内容的源文件，可以
	 *  为空，但当为空时在调用 save_body 函数时，则不能指定源文件
	 * @param node {const MIME_NODE*} 邮件中的某个结点对象
	 * @param enableDecode {bool} 当邮件内容为 base64/qp 等编码格式
	 *  时是否需要自动进行解码
	 * @param toCharset {const char*} 缺省的目标字符集，如果目标
	 *  字符集与源字符集不同，则进行字符集转换
	 * @param off {off_t} 邮件内容在整个数据中的起始位置中附加的
	 *  相对偏移量，以便于用户可以在邮件内容前面加自己的私有数据
	 */
	mime_node(const char* emailFile, const MIME_NODE* node,
		bool enableDecode = true, const char* toCharset = "gb2312",
		off_t off = 0);
	virtual ~mime_node();

	/**
	 * 获得 MIME 结点中 Content-Type 值中的 name 字段值
	 * @return {const char*} 如果为空则表示没有该字段值
	 */
	const char* get_name() const
	{
		if (m_name.empty())
			return (NULL);
		return (m_name.c_str());
	}

	/**
	 * 获得 Content-Type 中的主类型，如: Content-Type: image/jpeg, 则本
	 * 函数返回 MIME_CTYPE_IMAGE (在 mime_define.hpp 中定义)
	 * @return {int} 返回 mime_define.hpp 中定义的 MIME_CTYPE_XXX
	 */
	int get_ctype(void) const
	{
		return (m_ctype);
	}

	/**
	 * 获得 Content-Type 中的从类型，如: Content-Type: image/jpeg, 则本
	 * 函数返回 MIME_STYPE_JPEG (在 mime_define.hpp 中定义)
	 * @return {int} 返回 mime_define.hpp 中定义的 MIME_STYPE_XXX
	 */
	int get_stype() const
	{
		return (m_stype);
	}

	/**
	 * 获得 Content-Type 中的主类型，以字符串方式表示
	 * @return {const char*} 返回 "" 表示不存在
	 */
	const char* get_ctype_s(void) const;

	/**
	 * 获得 Content-Type 中的从类型，以字符串方式表示
	 * @return {const char*} 返回 "" 表示不存在
	 */
	const char* get_stype_s(void) const;

	/**
	 * 获得传输编码类型 (对应于 Content-Transfer-Encoding)
	 * @return {int} 返回 mime_define.hpp 中定义的 MIME_ENC_XXX
	 */
	int get_encoding() const
	{
		return (m_encoding);
	}

	/**
	 * 获得结点字符集字符串(对应于 Content-Type 中的 charset 字段)
	 * @return {const char*} 为空则表示没有该字段
	 */
	const char* get_charset() const
	{
		return (m_charset);
	}

	/**
	 * 获得目标字符集, 由用户在构造函数中传入
	 * @return {const char*} 为空则表示用户未设置
	 */
	const char* get_toCharset() const
	{
		if (m_toCharset[0])
			return (m_toCharset);
		else
			return (NULL);
	}

	/**
	 * 获得本结点在邮件中的起始偏移量
	 * @return {off_t}
	 */
	off_t get_bodyBegin() const
	{
		return (m_bodyBegin);
	}

	/**
	 * 获得本结点在邮件中的结束偏移量
	 * @return {off_t}
	 */
	off_t get_bodyEnd() const
	{
		return (m_bodyEnd);
	}

	/**
	 * 获得本结点头部中某个字段的值
	 * @param name {const char*} 字段名, 如: Content-Type
	 * @return {const char*} 为空则表示不存在
	 */
	const char* header_value(const char* name) const;

	/**
	 * 取得该结点的所有头部字段集合
	 * @return {const std::map<string, string>&}
	 */
	const std::map<string, string>& get_headers() const;

	/**
	 * 转储本结点内容于指定的管道流中
	 * @param out {pipe_manager&}
	 * @return {bool} 是否成功
	 */
	bool save(pipe_manager& out) const;

	/**
	 * 转储本结点内容于指定的管道流中
	 * @param out {pipe_manager&}
	 * @param src {const char*} 邮件内容的起始地址，如果为空指针，
	 *  则从构造函数中所提供的 emailFile 的文件中提取邮件内容
	 * @param len {int} 邮件内容的数据长度，如果为0，则从构造
	 *  函数中所提供的 emailFile 的文件中提取邮件内容
	 * @return {bool} 是否成功
	 */
	bool save(pipe_manager& out, const char* src, int len) const;

	/**
	 * 转储本结点内容于指定的输出流中
	 * @param out {ostream&} 流出流
	 * @param src {const char*} 邮件内容的起始地址，如果为空指针，
	 *  则从构造函数中所提供的 emailFile 的文件中提取邮件内容
	 * @param len {int} 邮件内容的数据长度，如果为0，则从构造
	 *  函数中所提供的 emailFile 的文件中提取邮件内容
	 * @return {bool} 是否成功
	 */
	bool save(ostream& out, const char* src = NULL, int len = 0) const;

	/**
	 * 转储本结点内容于指定的文件中
	 * @param outFile {const char*} 目标文件名
	 * @param src {const char*} 邮件内容的起始地址，如果为空指针，
	 *  则从构造函数中所提供的 emailFile 的文件中提取邮件内容
	 * @param len {int} 邮件内容的数据长度，如果为0，则从构造
	 *  函数中所提供的 emailFile 的文件中提取邮件内容
	 * @return {bool} 是否成功
	 */
	bool save(const char* outFile, const char* src = NULL, int len = 0) const;

	/**
	 * 转储本结点内容于缓冲区中
	 * @param out {string&} 缓冲区
	 * @param src {const char*} 邮件内容的起始地址，如果为空指针，
	 *  则从构造函数中所提供的 emailFile 的文件中提取邮件内容
	 * @param len {int} 邮件内容的数据长度，如果为0，则从构造
	 *  函数中所提供的 emailFile 的文件中提取邮件内容
	 * @return {bool} 是否成功
	 */
	bool save(string& out, const char* src, int len) const;

	/**
	 * 获得本结点对应的父结点对象
	 * @return {mime_node*} 为空则表示本结点没有父结点(则说明
	 *  本结点为邮件的根结点); 否则则返回的父结点需要在用完后
	 *  delete 掉以释放相应内存
	 */
	mime_node* get_parent() const;

	/**
	 * 判断本结点是否有父结点
	 * @return {bool} true 则表示有父结点, 否则表示没有
	 */
	bool has_parent() const;

	/**
	 * 获得父结点的主类型 (MIME_CTYPE_XXX), 如果为 MIME_CTYPE_OTHER
	 * 则说明父结点不存在或父结点的主类型未知
	 * @return {int} MIME_CTYPE_XXX
	 */
	int parent_ctype(void) const;
	const char* parent_ctype_s(void) const;

	/**
	 * 获得父结点的从类型 (MIME_STYPE_XXX), 如果为 MIME_STYPE_OTHER
	 * 则说明父结点不存在或父结点的从类型未知
	 * @return {int} MIME_STYPE_XXX
	 */
	int parent_stype(void) const;
	const char* parent_stype_s(void) const;

	/**
	 * 获得父结点的编码类型 (MIME_ENC_XXX), 如果返回值为 MIME_ENC_OTHER
	 * 则说明父结点不存在或父结点的编码类型未知
	 * @return {int} MIME_ENC_XXX
	 */
	int parent_encoding() const;

	/**
	 * 获得父结点的字符集类型, 如果返回值为空则说明父结点不存在或父结点
	 * 中没有字符集类型
	 * @return {const char*}
	 */
	char* parent_charset() const;

	/**
	 * 获得父结点的数据体起始偏移量
	 * @return {off_t} 返回值为 -1 表示父结点不存在
	 */
	off_t parent_bodyBegin() const;

	/**
	 * 获得父结点的数据体结束偏移量
	 * @return {off_t} 返回值为 -1 表示父结点不存在
	 */
	off_t parent_bodyEnd() const;

	/**
	 * 获得父结点头部中某个字段名对应的字段值, 如: Content-Type
	 * @param name {const char*} 字段名
	 * @return {const char*} 字段值, 返回空则说明父结点不存在
	 *  或父结点头部中不存在该字段
	 */
	const char* parent_header_value(const char* name) const;

protected:
	bool  m_enableDecode;
	string m_name;
	string m_emailFile;
	int   m_ctype;		// mime_define.hpp
	int   m_stype;		// mime_define.hpp
	int   m_encoding;	// mime_define.hpp
	char  m_charset[32];
	char  m_toCharset[32];
	off_t m_bodyBegin;
	off_t m_bodyEnd;
	std::map<string, string>* m_headers_;
	const MIME_NODE* m_pMimeNode;
	mime_node* m_pParent;
};

} // namespace acl
