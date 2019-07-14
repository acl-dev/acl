#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/hsocket/hserror.hpp"
#endif

#ifndef ACL_CLIENT_ONLY

namespace acl
{

static const char* dummy_unknown = "uknown error";

hserror::hserror(void)
{
}

hserror::~hserror(void)
{
}

const char* hserror::get_serror(int errnum)
{
	static struct {
		int  err;
		const char* msg;
	} err_list[] = {
		{ HS_ERR_INVALID_REPLY, "server reply invalid" },
		{ HS_ERR_EMPTY, "server reply empty" },
		{ HS_ERR_PARAMS, "params invalid" },
		{ HS_ERR_NOT_OPEN, "server's table not open" },
		{ HS_ERR_READ, "read from server error" },
		{ HS_ERR_WRITE, "write to server error" },
		{ HS_ERR_CONN, "connect server error" },
		{ HS_ERR_OK, "ok" },

		{ -1000, NULL}
	};

	for (int i = 0; err_list[i].msg != NULL; i++) {
		if (err_list[i].err == errnum) {
			return err_list[i].msg;
		}
	}
	return dummy_unknown;
}

}

#endif // ACL_CLIENT_ONLY
