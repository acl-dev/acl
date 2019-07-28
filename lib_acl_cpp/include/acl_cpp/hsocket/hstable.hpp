#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl {

class ACL_CPP_API hstable : public noncopyable
{
public:
	hstable(int id, const char* dbn, const char* tbl,
		const char* idx, const char* flds);
	~hstable();

private:
	friend class hsclient;
	int    id_;
	char*  dbn_;
	char*  tbl_;
	char*  idx_;
	char*  flds_;
	int    nfld_;
	char** values_;
};

}  // namespace acl

#endif // ACL_CLIENT_ONLY
