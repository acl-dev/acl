#pragma once
#include "../acl_cpp_define.hpp"
#include "mime_node.hpp"

#if !defined(ACL_MIME_DISABLE)

struct MIME_NODE;

namespace acl {

class pipe_manager;
class ostream;
class pipe_string;
class string;

class ACL_CPP_API mime_body : public mime_node
{
public:
	/**
	 * 构造函数
	 * @param emailFile {const char*} 存储邮件内容的源文件，可以
	 *  为空，但当为空时在调用 save_body 函数时，则不能指定源文件
	 * @param node {const MIME_NODE*} 邮件中的某个结点对象
	 * @param htmlFirst {bool} 是否在提取内容时优先提取 HTML 数据
	 * @param enableDecode {bool} 当邮件内容为 base64/qp 等编译格式
	 *  时是否需要自动进行解码
	 * @param toCharset {const char*} 缺省的目标字符集，如果目标
	 *  字符集与源字符集不同，则进行字符集转换
	 * @param off {off_t} 邮件内容在整个数据中的起始位置中附加的
	 *  相对偏移量，以便于用户可以在邮件内容前面加自己的私有数据
	 */
	mime_body(const char* emailFile, const MIME_NODE* node,
		bool htmlFirst = true, bool enableDecode = true,
		const char* toCharset = "gb2312", off_t off = 0)
		: mime_node(emailFile, node, enableDecode, toCharset, off)
		, m_htmlFirst(htmlFirst)
	{
	}

	~mime_body(void) {}

	/**
	 * 设置是否仅提取 HTML 数据, 如果为 true 则优先提取 HTML 数据,
	 * 当不存在 HTML 数据时才会提取纯文本数据; 如果为 false 则优先
	 * 提取纯文本数据, 如果仅有 HTML 数据时则会从该 HTML 数据中抽
	 * 取出纯文本数据
	 * @param htmlFirst {bool}
	 */
	void set_status(bool htmlFirst)
	{
		m_htmlFirst = htmlFirst;
	}

	/**
	 * 转储邮件正文内容于管道流中
	 * @param out {pipe_manager&} 管道流管理器
	 * @param src {const char*} 邮件内容的起始地址，如果为空指针，
	 *  则从构造函数中所提供的 emailFile 的文件中提取邮件内容
	 * @param len {int} 邮件内容的数据长度，如果为0，则从构造
	 *  函数中所提供的 emailFile 的文件中提取邮件内容
	 * @return {bool} 是否成功
	 */
	bool save_body(pipe_manager& out, const char* src = NULL,
		int len = 0);

	/**
	 * 转储邮件正文内容于输出流中
	 * @param out {ostream&} 输出流
	 * @param src {const char*} 邮件内容的起始地址，如果为空指针，
	 *  则从构造函数中所提供的 emailFile 的文件中提取邮件内容
	 * @param len {int} 邮件内容的数据长度，如果为0，则从构造
	 *  函数中所提供的 emailFile 的文件中提取邮件内容
	 * @return {bool} 是否成功
	 */
	bool save_body(ostream& out, const char* src = NULL,
		int len = 0);

	/**
	 * 转储邮件正文内容于目标文件中
	 * @param file_path {const char*} 目标文件名
	 * @param src {const char*} 邮件内容的起始地址，如果为空指针，
	 *  则从构造函数中所提供的 emailFile 的文件中提取邮件内容
	 * @param len {int} 邮件内容的数据长度，如果为0，则从构造
	 *  函数中所提供的 emailFile 的文件中提取邮件内容
	 * @return {bool} 是否成功
	 */
	bool save_body(const char* file_path, const char* src = NULL,
		int len = 0);

	/**
	 * 转储邮件正文内容于管道缓冲区内
	 * @param out {pipe_string&} 管道缓冲区
	 * @param src {const char*} 邮件内容的起始地址，如果为空指针，
	 *  则从构造函数中所提供的 emailFile 的文件中提取邮件内容
	 * @param len {int} 邮件内容的数据长度，如果为0，则从构造
	 *  函数中所提供的 emailFile 的文件中提取邮件内容
	 * @return {bool} 是否成功
	 */
	bool save_body(pipe_string& out, const char* src = NULL,
		int len = 0);

	/**
	 * 转储邮件正文内容于缓冲区中
	 * @param out {string&} 缓冲区
	 * @param src {const char*} 邮件内容的起始地址，如果为空指针，
	 *  则从构造函数中所提供的 emailFile 的文件中提取邮件内容
	 * @param len {int} 邮件内容的数据长度，如果为0，则从构造
	 *  函数中所提供的 emailFile 的文件中提取邮件内容
	 * @return {bool} 是否成功
	 */
	bool save_body(string& out, const char* src = NULL,
		int len = 0);

	/**
	 * 判断结点头部类型中的从类型是否 MIME_STYPE_HTML 类型
	 * @return {bool}
	 */
	bool html_stype(void) const;

private:
	bool m_htmlFirst;
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
