#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/mime/mime_head.hpp"

class mime_builder
{
public:
	mime_builder();
	~mime_builder();

	/**
	 * 获得邮件头
	 * @return {acl::mime_head&}
	 */
	acl::mime_head& primary_header(void)
	{
		return (header_);
	}

	/**
	 * 设置邮件正文内容，为纯文本格式
	 * @param src {const char*) 正文内容
	 * @param len {size_t} src 内容长度
	 * @return {mime_builder&}
	 */
	mime_builder& set_body_text(const char* src, size_t len);

	/**
	 * 设置邮件正文内容，为 HTML 格式
	 * @param src {const char*) 正文内容
	 * @param len {size_t} src 内容长度
	 * @return {mime_builder&}
	 */
	mime_builder& set_body_html(const char* src, size_t len);

	/**
	 * 添加附件
	 * @param filepath {const char*} 附件文件路径
	 * @return {mime_builder&}
	 */
	mime_builder& add_file(const char* filepath);

	/**
	 * 创建邮件内容，并转存至文件中
	 * @param to {const char*} 目标文件
	 * @return {bool}
	 */
	bool save_as(const char* to);

	/**
	 * 创建邮件内容，并转存至文件中
	 * @param fp {acl::ofstream&} 目标文件句柄
	 * @return {bool}
	 */
	bool save_as(acl::ofstream& fp);

private:
	acl::mime_head header_;
	char* body_text_;
	char* body_html_;
	std::vector<char*> attachs_;
	acl::string delimeter_;

	bool add_body(acl::ofstream& fp);
	bool add_boundary(acl::ofstream& fp, bool end = false);
	bool add_attach(acl::ofstream& fp, const char* filepath);
};
