#include "stdafx.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

/* Application-specific. */

#include "master_params.h"
#include "master.h"
#include "master_api.h"

#define SAME	!strcmp

static bool setup_callback(const char *service, ACL_MASTER_SERV *serv,
	STATUS_CALLBACK callback, void *ctx)
{
	if (callback != NULL && serv->callback != NULL) {
		acl_msg_warn("%s(%d), %s: another callback is set!",
			__FILE__, __LINE__, service);
		return false;
	}

	serv->callback = callback;
	serv->ctx      = ctx;

	return true;
}

ACL_MASTER_SERV *acl_master_lookup(const char *path)
{
	return acl_master_ent_find(path);
}

static int check_command(ACL_MASTER_SERV *entry, const char *ext)
{
	char *path;

	/* if ext name not NULL, then add it as the extern name of command */
	if (ext && *ext)
		path = acl_concatenate(entry->path, ext, NULL);
	else
		path = acl_concatenate(entry->path, entry->cmdext, NULL);

	if (access(path, F_OK) != 0) {
		acl_msg_error("%s(%d), %s: command %s can't be executed, %s",
			__FILE__, __LINE__, __FUNCTION__, path,
			acl_last_serror());
		return -1;
	}

	acl_myfree(entry->path);
	entry->path = path;

	acl_msg_info("service command path=%s", path);

	/* reset argv be used to transfer execvp */
	acl_argv_set(entry->args, 0, path);

	return 0;
}

ACL_MASTER_SERV *acl_master_start(const char *path, int *nchilden,
	int *nsignaled, STATUS_CALLBACK callback, void *ctx, const char *ext)
{
	ACL_MASTER_SERV *entry = acl_master_lookup(path);

	if (entry != NULL) {
		acl_msg_error("%s(%d), %s: same service %s running",
			__FILE__, __LINE__, __FUNCTION__, path);
		return NULL;
	}

	entry = acl_master_ent_load(path);
	if (entry == NULL) {
		acl_msg_error("%s(%d), %s: load %s error %s", __FILE__,
			__LINE__, __FUNCTION__, path, acl_last_serror());
		return NULL;
	}

	if (check_command(entry, ext) < 0) {
		acl_msg_error("%s(%d), %s: can't start service %s, %s",
			__FILE__, __LINE__, __FUNCTION__, entry->path, path);
		acl_master_ent_free(entry);
		return NULL;
	}

	if (acl_master_service_start(entry) < 0) {
		acl_msg_error("%s(%d), %s: start %s error",
			__FILE__, __LINE__, __FUNCTION__, path);
		acl_master_ent_free(entry);
		return NULL;
	}

	if (nchilden)
		*nchilden = entry->prefork_proc;
	if (nsignaled)
		*nsignaled = entry->prefork_proc;

	(void) setup_callback(__FUNCTION__, entry, callback, ctx);
	entry->next = acl_var_master_head;
	acl_var_master_head = entry;

	return entry;
}

ACL_MASTER_SERV *acl_master_restart(const char *path, int *nchilden,
	int *nsignaled, STATUS_CALLBACK callback, void *ctx, const char *ext)
{
        ACL_MASTER_SERV *serv = acl_master_lookup(path);

        if (serv == NULL)
		return acl_master_start(path, nchilden, nsignaled,
				callback, ctx, ext);

	ACL_MASTER_SERV *entry = acl_master_ent_load(path);
	if (entry == NULL) {
		acl_msg_error("%s(%d), %s: service load %s error %s", __FILE__,
			__LINE__, __FUNCTION__, path, acl_last_serror());
		return NULL;
	}

	if (check_command(entry, ext) < 0) {
		acl_msg_error("%s(%d), %s: can't restart service %s, %s",
			__FILE__, __LINE__, __FUNCTION__, serv->path, path);
		return NULL;
	}

	acl_master_refresh_service(entry);
	return serv;
}

/* kill processes of service according the master_service name in configure */

int acl_master_kill(const char *path)
{
	ACL_MASTER_SERV *serv = acl_master_lookup(path);
	ACL_MASTER_SERV *iter, **servp;

	if (serv == NULL) {
		acl_msg_error("%s(%d), %s: no service, path %s",
			__FILE__, __LINE__, __FUNCTION__, path);
		return -1;
	}

	for (servp = &acl_var_master_head; (iter = *servp) != 0;) {
		if (SAME(iter->conf, path)) {
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

int acl_master_stop(const char *path)
{
	ACL_MASTER_SERV *serv = acl_master_lookup(path);
	ACL_MASTER_SERV *iter, **servp;

	if (serv == NULL) {
		acl_msg_error("%s(%d), %s: no service, path %s",
			__FILE__, __LINE__, __FUNCTION__, path);
		return -1;
	}

	for (servp = &acl_var_master_head; (iter = *servp) != 0;) {
		if (SAME(iter->conf, path)) {
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

int acl_master_reload(const char *path, int *nchilden, int *nsignaled,
	STATUS_CALLBACK callback, void *ctx)
{
	ACL_MASTER_SERV *serv = acl_master_lookup(path);

	if (serv == NULL) {
		acl_msg_error("%s(%d), %s: no service for path %s",
			__FILE__, __LINE__, __FUNCTION__, path);
		return -1;
	}

	if (nchilden)
		*nchilden = (int) acl_ring_size(&serv->children);

	(void) setup_callback(__FUNCTION__, serv, callback, ctx);

	acl_master_sighup_children(serv, nsignaled);

	return 0;
}

void acl_master_callback_clean(const char *path)
{
	ACL_MASTER_SERV *serv = acl_master_lookup(path);

	if (serv) {
		serv->callback = NULL;
		serv->ctx      = NULL;
	}
}
