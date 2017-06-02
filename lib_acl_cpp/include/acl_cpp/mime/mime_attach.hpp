#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "mime_node.hpp"

namespace acl {

class ACL_CPP_API mime_attach : public mime_node
{
public:
	mime_attach(const char* emailFile, const MIME_NODE* node,
		bool enableDecode = true, const char* toCharset = "gb2312",
		off_t off = 0);

	virtual ~mime_attach();

	/**
	 * 获得附件的文件名
	 * @return {const char*} 返回值为 NULL 则说明没有找到文件名
	 */
	const char* get_filename() const;

private:
	string m_filename;
};

} // namespace acl
