#include "stdafx.h"
#include "allow_list.h"

allow_list::allow_list()
{
	manager_allow_ = acl_iplink_create(10);
}

allow_list::~allow_list()
{
	acl_iplink_free(manager_allow_);
}

void allow_list::set_allow_manager(const char* white_list)
{
	if (strcasecmp(white_list, "all") == 0)
	{
		acl_iplink_insert(manager_allow_, "0.0.0.0", "255.255.255.255");
		return;
	}

	ACL_ARGV* tokens = acl_argv_split(white_list, ",; \t");
	ACL_ITER iter;

	acl_foreach(iter, tokens)
	{
		char* begin = (char* ) iter.data;
		char* end = strchr(begin, ':');
		if (end == NULL || *(end + 1)== 0)
		{
			logger_warn("invalid ip item: %s", begin);
			continue;
		}
		*end++ =  0;
		if (acl_iplink_insert(manager_allow_, begin, end) == NULL)
			logger_warn("invalid ip item: %s:%s", begin, end);
	}

	acl_argv_free(tokens);
}

bool allow_list::allow_manager(const char* ip)
{

	return acl_iplink_lookup_str(manager_allow_, ip) == NULL
		? false : true;
}
