#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"
#include "../http/http_ctype.hpp"

#if !defined(ACL_MIME_DISABLE)

namespace acl {

class mime_code;
class mail_attach;

/**
 * 邮件正文构建类
 */
class ACL_CPP_API mail_body : public noncopyable
{
public:
	/**
	 * 构造函数
	 * @param charset {const char*} 正文的字符集
	 * @param encoding {const char*} 正文的编码格式
	 */
	mail_body(const char* charset = "utf-8",
		const char* encoding = "base64");
	~mail_body();

	/**
	 * 获得正文折内容类型
	 * @return {const string&}
	 */
	const string& get_content_type() const
	{
		return content_type_;
	}

	/**
	 * 获得正文内容类型对象
	 * @return {const http_ctype&}
	 */
	const http_ctype& get_ctype() const
	{
		return ctype_;
	}

	/**
	 * 设置邮件正文为 TEXT/HTML 格式
	 * @param html {const char*} HTML 数据
	 * @param len {size_t} html 数据长度(虽然 html 是字符串格式，但提供
	 *  数据长度有利于调用更灵活高效，内部不再重新通过 strlen 计算长度)
	 * @return {mail_body&}
	 */
	mail_body& set_html(const char* html, size_t len);

	/**
	 * 设置邮件正文为 TEXT/PLAIN 格式
	 * @param plain {const char*} TEXT 数据
	 * @param len {size_t} plain 数据长度(虽然 plain 是文本格式，但提供
	 *  数据长度有利于调用更灵活高效，内部不再重新通过 strlen 计算长度)
	 * @return {mail_body&}
	 */
	mail_body& set_plain(const char* plain, size_t len);

	/**
	 * 当邮件内容为 multipart/alternative 格式时调用此函数设置相应类型的
	 * 正文内容
	 * @param html {const char*} 正文中的 HTML 数据(非空)
	 * @param hlen {size_t} html 数据长度(>0)
	 * @param plain {const char*} 正文中的 TEXT 数据(非空)
	 * @param plen {size_t} plain 数据长度(>0)
	 * @return {mail_body&}
	 */
	mail_body& set_alternative(const char* html, size_t hlen,
		const char* plain, size_t plen);

	/**
	 * 当邮件正文内容为 multipart/relative 格式时调用此函数设置正文内容
	 * @param html {const char*} 正文中的 HTML 数据(非空)
	 * @param hlen {size_t} html 数据长度(>0)
	 * @param plain {const char*} 正文中的 plain 数据(非空)
	 * @param plen {size_t} plain 数据长度(>0)
	 * @param attachments {const std::vector<mail_attach*>&} 存放
	 *  与 html 中的 cid 相关的图片等附件对象
	 * @return {mail_body&}
	 */
	mail_body& set_relative(const char* html, size_t hlen,
		const char* plain, size_t plen,
		const std::vector<mail_attach*>& attachments);

	/**
	 * 获得 set_html 函数设置的 html/plain 数据
	 * @param len {size_t} 存放数据长度结果
	 * @return {const char*}
	 */
	const char* get_html(size_t& len) const
	{
		len = hlen_;
		return html_;
	}

	/**
	 * 获得 set_plain 函数设置的 plain/plain 数据
	 * @param len {size_t} 存放数据长度结果
	 * @return {const char*}
	 */
	const char* get_plain(size_t& len) const
	{
		len = plen_;
		return plain_;
	}

	/**
	 * 获得 set_attachments 函数设置的附件集合
	 * @return {const std::vector<mail_attach*>*}
	 */
	const std::vector<mail_attach*>* get_attachments() const
	{
		return attachments_;
	}

	/**
	 * 构造邮件正文并将结果追加于给定的输出流中
	 * @param out {ostream&} 输出流对象
	 * @return {bool} 操作是否成功
	 */
	bool save_to(ostream& out) const;

	/**
	 * 构造邮件正文并将结果追加于给定的缓冲区中
	 * @param out {string&} 存储结果
	 * @return {bool} 操作是否成功
	 */
	bool save_to(string& out) const;

	/**
	 * text/html 格式的邮件正文构造过程，并将结果追加于给定的缓冲区中
	 * @param in {const char*} 输入的 html 格式数据
	 * @param len {size_t} in 的数据长度
	 * @param out {string&} 以数据追加方式存储结果
	 * @return {bool} 操作是否成功
	 */
	bool save_html(const char* in, size_t len, string& out) const;

	/**
	 * text/plain 格式的邮件正文构造过程，并将结果追加于给定的缓冲区中
	 * @param in {const char*} 输入的 plain 格式数据
	 * @param len {size_t} in 的数据长度
	 * @param out {string&} 以数据追加方式存储结果
	 * @return {bool} 操作是否成功
	 */
	bool save_plain(const char* in, size_t len, string& out) const;

	/**
	 * multipart/relative 格式的邮件正文构造过程，并将结果追加于给定的缓冲区中
	 * @param html {const char*} 输入的 html 格式数据
	 * @param hlen {size_t} html 的数据长度
	 * @param plain {const char*} 正文中的 TEXT 数据(非空)
	 * @param plen {size_t} plain 数据长度(>0)
	 * @param attachments {const std::vector<mail_attach*>&} 存放
	 *  与 html 中的 cid 相关的图片等附件对象
	 * @param out {string&} 以数据追加方式存储结果
	 * @return {bool} 操作是否成功
	 */
	bool save_relative(const char* html, size_t hlen,
		const char* plain, size_t plen,
		const std::vector<mail_attach*>& attachments,
		string& out) const;

	/**
	 * multipart/alternative 格式的邮件正文构造过程，并将结果追加于给定的缓冲区中
	 * @param html {const char*} 输入的 html 格式数据
	 * @param hlen {size_t} html 的数据长度
	 * @param plain {const char*} 正文中的 TEXT 数据(非空)
	 * @param plen {size_t} plain 数据长度(>0)
	 * @param out {string&} 以数据追加方式存储结果
	 * @return {bool} 操作是否成功
	 */
	bool save_alternative(const char* html, size_t hlen,
		const char* plain, size_t plen, string& out) const;

private:
	string  charset_;
	string  content_type_;
	string  transfer_encoding_;
	mime_code* coder_;
	string  boundary_;
	http_ctype ctype_;
	int     mime_stype_;

	const char* html_;
	size_t hlen_;
	const char* plain_;
	size_t plen_;
	const std::vector<mail_attach*>* attachments_;

	bool build(const char* in, size_t len, const char* content_type,
		const char* charset, mime_code& coder, string& out) const;
	bool build_html(const char* in, size_t len,
		const char* charset, string& out) const;
	bool build_plain(const char* in, size_t len,
		const char* charset, string& out) const;

	void set_content_type(const char* content_type);
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
