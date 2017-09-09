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

#define SAME	!strcmp

ACL_MASTER_SERV *acl_master_lookup(const char *path)
{
	ACL_MASTER_SERV *entry = acl_master_ent_load(path), *serv;

	if (entry == NULL) {
		acl_msg_error("%s(%d), %s: load %s error %s", __FILE__,
			__LINE__, __FUNCTION__, path, acl_last_serror());
		return NULL;
	}

	serv = acl_master_ent_find(entry->name, entry->type);
	acl_master_ent_free(entry);
	return serv;
}

ACL_MASTER_SERV *acl_master_start(const char *path)
{
	ACL_MASTER_SERV *entry = acl_master_ent_load(path), *serv;

	if (entry == NULL) {
		acl_msg_error("%s(%d), %s: load %s error %s", __FILE__,
			__LINE__, __FUNCTION__, path, acl_last_serror());
		return NULL;
	}

	serv = acl_master_ent_find(entry->name, entry->type);
	if (serv != NULL) {
		acl_msg_error("%s(%d), %s: same service %s %d running",
			__FILE__, __LINE__, __FUNCTION__,
			entry->name, entry->type);
		acl_master_ent_free(entry);
		return NULL;
	}
	
	entry->next = acl_var_master_head;
	acl_var_master_head = entry;
	acl_master_service_start(entry);
	return entry;
}

ACL_MASTER_SERV *acl_master_restart(const char *path)
{
        ACL_MASTER_SERV *serv = acl_master_lookup(path);

        if (serv == NULL)
	        return acl_master_start(path);

        acl_master_service_restart(serv);
        return serv;
}

/* kill processes of service according the master_service name in configure */

int     acl_master_kill(const char *path)
{
	ACL_MASTER_SERV *serv = acl_master_lookup(path);
	ACL_MASTER_SERV *iter, **servp;

	if (serv == NULL) {
		acl_msg_error("%s(%d), %s: no service, path %s",
			__FILE__, __LINE__, __FUNCTION__, path);
		return -1;
	}

	for (servp = &acl_var_master_head; (iter = *servp) != 0;) {
		if (iter->type == serv->type && SAME(iter->name, serv->name)) {
			*servp = iter->next;
			acl_master_service_kill(iter);
			return 0;
		} else
			servp = &iter->next;
	}

	acl_msg_warn("%s(%d), %s: not found service - %s %d, path %s",
		__FILE__, __LINE__, __FUNCTION__, serv->name, serv->type, path);

	return -1;
}

/* stop one service according the master_service name in configure */

int     acl_master_stop(const char *path)
{
	ACL_MASTER_SERV *serv = acl_master_lookup(path);
	ACL_MASTER_SERV *iter, **servp;

	if (serv == NULL) {
		acl_msg_error("%s(%d), %s: no service, path %s",
			__FILE__, __LINE__, __FUNCTION__, path);
		return -1;
	}

	for (servp = &acl_var_master_head; (iter = *servp) != 0;) {
		if (iter->type == serv->type && SAME(iter->name, serv->name)) {
			*servp = iter->next;
                        // this service object will be freed after all
                        // children of which exited.
			acl_master_service_stop(iter);
			return 0;
		} else
			servp = &iter->next;
	}

	acl_msg_warn("%s(%d), %s: not found service - %s %d, path %s",
		__FILE__, __LINE__, __FUNCTION__, serv->name, serv->type, path);

	return -1;
}

int     acl_master_reload(const char *path, int *nchilden, int *nsignaled,
	SIGNAL_CALLBACK callback, void *ctx)
{
	ACL_MASTER_SERV *serv = acl_master_lookup(path);

	if (serv == NULL) {
		acl_msg_error("%s(%d), %s: no service for path %s",
			__FILE__, __LINE__, __FUNCTION__, path);
		return -1;
	}

	acl_master_sighup_children(serv, nchilden, nsignaled, callback, ctx);
	return 0;
}

void    acl_master_reload_clean(const char *path)
{
	ACL_MASTER_SERV *serv = acl_master_lookup(path);
	if (serv == NULL)
		return;

	ACL_RING_ITER    iter;
	ACL_MASTER_PROC *proc;
	acl_ring_foreach(iter, &serv->children) {
		proc = acl_ring_to_appl(iter.ptr, ACL_MASTER_PROC, me);
		acl_assert(proc);
		proc->signal_callback = NULL;
		proc->signal_ctx      = NULL;
	}
}
