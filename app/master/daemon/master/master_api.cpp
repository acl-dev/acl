/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Fri 16 Jun 2017 09:56:08 AM CST
 */

#include "stdafx.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

/* Application-specific. */

#include "master_params.h"
#include "master.h"
#include "master_api.h"

#define STR_SAME	!strcmp

ACL_MASTER_SERV *acl_master_lookup(const char *name)
{
	ACL_MASTER_SERV *serv;

	for (serv = acl_var_master_head; serv != 0; serv = serv->next) {
		if (STR_SAME(serv->name, name))
			return serv;
	}

	return NULL;
}

ACL_MASTER_SERV *acl_master_start(const char *filepath)
{
	ACL_MASTER_SERV *entry = acl_master_ent_load(filepath), *serv;

	if (entry == NULL) {
		acl_msg_error("%s(%d), %s: load %s error %s", __FILE__,
			__LINE__, __FUNCTION__, filepath, acl_last_serror());
		return NULL;
	}

	serv = acl_master_lookup(entry->name);
	if (serv != NULL) {
		acl_msg_error("%s(%d), %s: same service %s running",
			__FILE__, __LINE__, __FUNCTION__, entry->name);
		acl_master_ent_free(entry);
		return NULL;
	}
	
	entry->next = acl_var_master_head;
	acl_var_master_head = entry;
	acl_master_service_start(entry);
	return entry;
}

ACL_MASTER_SERV *acl_master_restart(const char *filepath)
{
	ACL_MASTER_SERV *entry = acl_master_ent_load(filepath);

	if (entry == NULL) {
		acl_msg_error("%s(%d), %s: load %s error %s", __FILE__,
			__LINE__, __FUNCTION__, filepath, acl_last_serror());
		return NULL;
	}

	(void) acl_master_stop(entry->name);
	acl_master_ent_free(entry);

	return acl_master_start(filepath);
}

/* stop one service according the master_service name in configure */

int     acl_master_stop(const char *name)
{
	ACL_MASTER_SERV *serv, **servp;

	for (servp = &acl_var_master_head; (serv = *servp) != 0;) {
		if (STR_SAME(serv->name, name)) {
			*servp = serv->next;
			acl_master_service_stop(serv);
			acl_master_ent_free(serv);
			return 0;
		}
	}

	acl_msg_warn("%s(%d), %s: service - %s not found",
		__FILE__, __LINE__, __FUNCTION__, name);
	return -1;
}

