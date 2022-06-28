#pragma once
#include "../acl_cpp_define.hpp"

#ifndef ACL_CLIENT_ONLY

#define	HS_ERR_INVALID_REPLY	-7		// 服务器返回数据错误
#define	HS_ERR_EMPTY		-6		// 服务器返回数据为空
#define	HS_ERR_PARAMS		-5		// 输入参数错误
#define	HS_ERR_NOT_OPEN		-4		// 表未打开
#define	HS_ERR_READ		-3		// 读数据失败
#define	HS_ERR_WRITE		-2		// 写数据失败
#define	HS_ERR_CONN		-1		// 连接失败
#define	HS_ERR_OK		0		// 正确

namespace acl {

class ACL_CPP_API hserror
{
public:
	hserror();
	~hserror();

	static const char* get_serror(int errnum);
};

}  // namespace acl

#endif // ACL_CLIENT_ONLY
