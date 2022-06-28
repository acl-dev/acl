#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/hsocket/hstable.hpp"
#endif

#ifndef ACL_CLIENT_ONLY

namespace acl
{

hstable::hstable(int id, const char* dbn, const char* tbl,
	const char* idx, const char* flds)
{
	id_   = id;
	dbn_  = acl_mystrdup(dbn);
	tbl_  = acl_mystrdup(tbl);
	idx_  = acl_mystrdup(idx);
	flds_ = acl_mystrdup(flds);

	ACL_ARGV *tokens = acl_argv_split(flds, ",; \t");
	nfld_ = tokens->argc;
	acl_argv_free(tokens);
	values_ = (char**) acl_mycalloc(nfld_, sizeof(char*));
}

hstable::~hstable(void)
{
	acl_myfree(dbn_);
	acl_myfree(tbl_);
	acl_myfree(idx_);
	acl_myfree(flds_);
	acl_myfree(values_);
}

}  // namespace acl

#endif // ACL_CLIENT_ONLY
