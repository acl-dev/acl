#include "stdafx.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

/* Application-specific. */

#include "master_params.h"
#include "master.h"

/* acl_master_refresh - re-read configuration table */

void    acl_master_refresh(void)
{
	ACL_MASTER_SERV *serv, **servp;

	/*
	 * Mark all existing services.
	 */
	for (serv = acl_var_master_head; serv != 0; serv = serv->next)
		serv->flags |= ACL_MASTER_FLAG_MARK;

	/*
	 * Read each service's configuration file. The master_conf() routine
	 * unmarks services upon update. New services are born with the mark
	 * bit off. After this, anything with the mark bit on should be
	 * removed.
	 */
	acl_master_config();

	/*
	 * Delete all services that are still marked - they disappeared from
	 * the configuration file and are therefore no longer needed.
	 */
	for (servp = &acl_var_master_head; (serv = *servp) != 0;) {
		if ((serv->flags & ACL_MASTER_FLAG_MARK) != 0) {
			*servp = serv->next;
                        // serv will be freed after all of it children exited.
			acl_master_service_stop(serv);
		} else
			servp = &serv->next;
	}
}

/* acl_master_config - read config file */

void    acl_master_config(void)
{
	const char *myname = "acl_master_config";
	ACL_MASTER_SERV *entry;
	ACL_MASTER_SERV *serv;
	char *pathname;
	int   service_null = 1;

#define SWAP(type,a,b)	{ type temp = a; a = b; b = temp; }

	/* load main.cf configuration of master routine */
	pathname = acl_concatenate(acl_var_master_conf_dir, "/", "main.cf", 0);
	acl_master_params_load(pathname);
	acl_myfree(pathname);

	/* create the global acl_var_master_global_event */
	acl_master_service_init();

	/* create the global acl_var_master_child_table */
	acl_master_spawn_init();

	/* create IPC PIPE */
	acl_master_vars_init(acl_var_master_buf_size, acl_var_master_rw_timeout);

	/* set scanning path for services' configuration files */
	acl_set_master_service_path(acl_var_master_service_dir);

	/*
	 * A service is identified by its endpoint name AND by its transport
	 * type, not just by its name alone. The name is unique within its
	 * transport type. XXX Service privacy is encoded in the service name.
	 */
	acl_master_ent_begin();

	while ((entry = acl_master_ent_get()) != 0) {
		service_null = 0;
		if (acl_msg_verbose)
			acl_master_ent_print(entry);

		serv = acl_master_ent_find(entry->name, entry->type);

		/*
		 * Add a new service entry. We do not really care in what
		 * order the service entries are kept in memory.
		 */
		if (serv == 0) {
			entry->next  = acl_var_master_head;
			entry->start = (long) time(NULL);
			acl_var_master_head = entry;
			acl_master_service_start(entry);
			continue;
		}

		/*
		 * Update an existing service entry. Make the current
		 * generation of child processes commit suicide whenever
		 * it is convenient. The next generation of child processes
		 * will run with the new configuration settings.
		 */
		serv->flags &= ~ACL_MASTER_FLAG_MARK;
		if (entry->flags & ACL_MASTER_FLAG_CONDWAKE)
			serv->flags |= ACL_MASTER_FLAG_CONDWAKE;
		else
			serv->flags &= ~ACL_MASTER_FLAG_CONDWAKE;
		serv->wakeup_time = entry->wakeup_time;
		serv->max_proc = entry->max_proc;
		serv->prefork_proc = entry->prefork_proc;
		serv->throttle_delay = entry->throttle_delay;
		SWAP(char *, serv->path, entry->path);
		SWAP(char *, serv->notify_addr, entry->notify_addr);
		SWAP(char *, serv->notify_recipients, entry->notify_recipients);
		SWAP(ACL_ARGV *, serv->args, entry->args);
		acl_master_service_restart(serv);
		acl_master_ent_free(entry);
	}

	acl_master_ent_end();

	if (service_null)
		acl_msg_warn("%s(%d), %s: no service in dir %s%s", __FILE__,
			__LINE__, myname, acl_var_master_service_dir,
			acl_var_master_scan_subdir ? " and its subdir" : "");
}
