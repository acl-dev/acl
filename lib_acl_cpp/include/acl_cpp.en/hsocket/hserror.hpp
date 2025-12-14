#pragma once
#include "../acl_cpp_define.hpp"

#ifndef ACL_CLIENT_ONLY

#define	HS_ERR_INVALID_REPLY	-7		// Server returned invalid data
#define	HS_ERR_EMPTY		-6		// Server returned empty data
#define	HS_ERR_PARAMS		-5		// Input parameter error
#define	HS_ERR_NOT_OPEN		-4		// Table not opened
#define	HS_ERR_READ		-3		// Read data failed
#define	HS_ERR_WRITE		-2		// Write data failed
#define	HS_ERR_CONN		-1		// Connection failed
#define	HS_ERR_OK		0		// Success

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
