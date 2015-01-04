#pragma once
#include <list>

#define OSS_SANE_FREE(x) do \
{ \
	if ((x) != NULL && pool_ == NULL) { \
		acl_myfree((x)); \
	} \
	(x) = NULL; \
} while (0)

#define OSS_SANE_DUP(from, to) do \
{ \
	if ((to) != NULL) \
	{ \
		if (pool_ == NULL) \
			acl_myfree((to)); \
		(to) = NULL; \
	} \
	if ((from) != NULL && *(from) != 0) \
	{ \
		if (pool_ != NULL) \
			(to) = pool_->dbuf_strdup((from)); \
		else \
			(to) = acl_mystrdup((from)); \
	} \
} while (0)

#define OSS_FREE_LIST(x) do \
{ \
	std::list<char*>::iterator it = (x).begin(); \
	for (; it != (x).end(); ++it) \
	{ \
		if (pool_ == NULL) \
			acl_myfree((*it)); \
	} \
	(x).clear(); \
} while (0)

#define OSS_COPY_LIST(from, to) do \
{ \
	std::list<string>::const_iterator cit = (from).begin(); \
	if (pool_ != NULL) \
	{ \
		for (; cit != (from).end(); ++cit) \
			(to).push_back(pool_->dbuf_strdup((*cit).c_str())); \
	} \
	else \
	{ \
		for (; cit != (from).end(); ++cit) \
			(to).push_back(acl_mystrdup((*cit).c_str())); \
	} \
} while (0)
