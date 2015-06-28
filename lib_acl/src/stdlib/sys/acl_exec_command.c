#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#ifdef	ACL_UNIX
#include <unistd.h>
#elif defined(ACL_WINDOWS)
#include <process.h>
#endif

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_argv.h"
#include "stdlib/acl_exec_command.h"

#endif

/* Application-specific. */

#define SPACE_TAB	" \t"

/* exec_command - exec command */

void acl_exec_command(const char *command)
{
	ACL_ARGV   *argv;

	/*
	 * Character filter. In this particular case, we allow space and tab in
	 * addition to the regular character set.
	 */
	static char ok_chars[] = "1234567890!@%-_=+:,./"
				  "abcdefghijklmnopqrstuvwxyz"
				  "ABCDEFGHIJKLMNOPQRSTUVWXYZ" SPACE_TAB;

	/*
	 * See if this command contains any shell magic characters.
	 */
	if (command[strspn(command, ok_chars)] == 0) {

		/*
		 * No shell meta characters found, so we can try to avoid the overhead
		 * of running a shell. Just split the command on whitespace and exec
		 * the result directly.
		 */
		argv = acl_argv_split(command, SPACE_TAB);
#ifdef ACL_UNIX
		(void) execvp(argv->argv[0], argv->argv);
#elif defined(ACL_WINDOWS)
		(void) _execvp(argv->argv[0], argv->argv);
#endif

		/*
		 * Auch. Perhaps they're using some shell built-in command.
		 */
		if (errno != ENOENT || strchr(argv->argv[0], '/') != 0)
			acl_msg_fatal("execvp %s: %s", argv->argv[0], acl_last_serror());

		/*
		 * Not really necessary, but...
		 */
		acl_argv_free(argv);
	}

	/*
	 * Pass the command to a shell.
	 */
#ifdef ACL_UNIX
	(void) execl(ACL_PATH_BSHELL, "sh", "-c", command, (char *) 0);
	acl_msg_fatal("execl %s: %s", ACL_PATH_BSHELL, acl_last_serror());
#elif defined(ACL_WINDOWS)
	(void) _execl(command, command, _execl, NULL);
	acl_msg_fatal("execl %s: %s", command, acl_last_serror());
#endif
}
