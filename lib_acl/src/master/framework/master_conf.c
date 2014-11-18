/* System libraries. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX

#include <stdio.h>
#include <unistd.h>
#include <string.h>

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_argv.h"
#include "stdlib/acl_stringops.h"
#include "stdlib/acl_mymalloc.h"

/* Application-specific. */

#include "../master_params.h"
#include "master.h"

/* acl_master_refresh - re-read configuration table */

void    acl_master_refresh(void)
{
	ACL_MASTER_SERV *serv;
	ACL_MASTER_SERV **servp;

	/*
	 * Mark all existing services.
	 */
	for (serv = acl_var_master_head; serv != 0; serv = serv->next)
		serv->flags |= ACL_MASTER_FLAG_MARK;

	/*
	 * Read the master.cf configuration file. The master_conf() routine
	 * unmarks services upon update. New services are born with the mark
	 * bit off. After this, anything with the mark bit on should be
	 * removed.
	 */
	acl_master_config();

	/*
	 * Delete all services that are still marked - they disappeared from
	 * the configuration file and are therefore no longer needed.
	 */
	for (servp = &acl_var_master_head; (serv = *servp) != 0; /* void */ ) {
		if ((serv->flags & ACL_MASTER_FLAG_MARK) != 0) {
			*servp = serv->next;
			acl_master_stop_service(serv);
			acl_free_master_ent(serv);
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

#define STR_DIFF	strcmp
#define STR_SAME	!strcmp
#define SWAP(type,a,b)	{ type temp = a; a = b; b = temp; }

	pathname = acl_concatenate(acl_var_master_conf_dir,
			"/", "main.cf", (char *) 0);
	acl_master_params_load(pathname);
	acl_myfree(pathname);

	acl_master_vars_init(acl_var_master_buf_size, acl_var_master_rw_timeout);
	acl_set_master_service_path(acl_var_master_service_dir);

	/*
	 * A service is identified by its endpoint name AND by its transport
	 * type, not just by its name alone. The name is unique within its
	 * transport type. XXX Service privacy is encoded in the service name.
	 */
	acl_set_master_ent();
	while ((entry = acl_get_master_ent()) != 0) {
		if (acl_msg_verbose)
			acl_print_master_ent(entry);
		for (serv = acl_var_master_head; serv != 0; serv = serv->next) {
			if (STR_SAME(serv->name, entry->name)
				&& serv->type == entry->type)
			{
				break;
			}
		}

		service_null = 0;
		/*
		 * Add a new service entry. We do not really care in what
		 * order the service entries are kept in memory.
		 */
		if (serv == 0) {
			entry->next = acl_var_master_head;
			acl_var_master_head = entry;
			acl_master_start_service(entry);
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
		acl_master_restart_service(serv);
		acl_free_master_ent(entry);
	}
	acl_end_master_ent();

	if (service_null)
		acl_msg_fatal("%s(%d)->%s: no service file in dir %s%s can be used",
			__FILE__, __LINE__, myname, acl_var_master_service_dir,
			acl_var_master_scan_subdir ? " and its subdir" : "");
}
#endif /* ACL_UNIX */
