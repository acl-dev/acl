#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"

#if !defined(ACL_MIME_DISABLE)

namespace acl {

class mime_code;
class ostream;

/**
 * 撰写邮件时，此类用于创建与邮件附件相关的功能
 */
class ACL_CPP_API mail_attach
{
public:
	/**
	 * 将一个普通文件打包进邮件时的构造函数
	 * @param filepath {const char*} 附件文件存储路径（含文件名）
	 * @param content_type {const char*} 附件文件类型
	 * @param charset {const char*} 若为纯文件，此参数表明纯文本的字符集
	 */
	mail_attach(const char* filepath, const char* content_type,
		const char* charset);
	~mail_attach();

	/**
	 * 设置附件的文件名，内部会自动对文件名用 rfc2047 格式进行编码
	 * @param name {const char*} 非空字符串
	 * @param charset {const char*} 该参数指定字符集，当非 NULL 时，则内
	 *  部自动使用 rfc2047 格式对文件名进行编码，否则内部直接存储输入名称
	 * @return {mail_attach&}
	 */
	mail_attach& set_filename(const char* name, const char* charset = NULL);

	/**
	 * 当邮件中的数据体为 multipart/relative 类型时，调用此函数设置其中的
	 * html 正文中 cid 标识符
	 * @param id {const char*} cid 标识符
	 * @return {mail_attach&}
	 */
	mail_attach& set_content_id(const char* id);

	/**
	 * 获得构造函数传入的附件文件路径
	 * @return {const char*}
	 */
	const char* get_filepath() const
	{
		return filepath_.c_str();
	}

	/**
	 * 获得附件的文件名部分经 rfc2047 编码后名称
	 * @return {const char*}
	 */
	const char* get_filename() const
	{
		return filename_.c_str();
	}

	/**
	 * 获得构造函数传入的文件类型
	 * @return {const char*}
	 */
	const char* get_content_type() const
	{
		return ctype_.c_str();
	}

	/**
	 * 获得由 set_content_id 设置的该附件的 cid 标识符
	 * @return {const char*}
	 */
	const char* get_content_id() const
	{
		return cid_.c_str();
	}

	/**
	 * 将附件内容采用传入的编码器进行编码后存入内存缓冲区
	 * @param coder {mime_code*} 编码器（base64/qp等）
	 * @param out {string&} 存储结果，采用 append 方式
	 * @return {bool} 编码过程是否成功
	 */
	bool save_to(mime_code* coder, string& out);

	/**
	 * 将附件内容采用传入的编码器进行编码后存入输出流中
	 * @param coder {mime_code*} 编码器（base64/qp等）
	 * @param out {out&} 存储结果
	 * @return {bool} 编码过程是否成功
	 */
	bool save_to(mime_code* coder, ostream& out);

	/**
	 * 创建该附件在 MIME 邮件中的文件头信息
	 * @param transfer_encoding {const char*} 编码方式
	 * @param out {string&} 存储结果，采用 append 方式
	 */
	void build_header(const char* transfer_encoding, string& out);

private:
	string filepath_;
	string filename_;
	string ctype_;
	string cid_;
	string charset_;

	bool rfc2047_encode(const char* name, const char* charset, string& out);
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
