#include "lib_acl.h"
#include "lib_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "probe.h"

char *var_cfg_file = NULL;
char *var_probe_procname = NULL;

static void init(void)
{
	acl_socket_init();
	probe_cfg_load();
	if (var_probe_logfile && *var_probe_logfile)
		acl_msg_open(var_probe_logfile, var_probe_procname);

/*	daemon(1, 1);
*/
}

static void usage(const char *progname)
{
	printf("usage: %s [option]\n"
		"\t-f configure_filename\n"
		"\t-h help\n",
		progname);
	exit (1);
}

int main(int argc, char *argv[])
{
	char  ch;

	var_probe_procname = acl_mystrdup(acl_safe_basename(argv[0]));

	while ((ch = getopt(argc, argv, "hf:")) > 0) {
		switch (ch) {
		case 'f':
			var_cfg_file = acl_mystrdup(optarg);
			break;
		case 'h':
			usage(argv[0]);
			break;
		default:
			usage(argv[0]);
			break;
		}
	}

	if (var_cfg_file == NULL)
		var_cfg_file = acl_mystrdup("test.cf");
	init();
	probe_run();
	acl_myfree(var_cfg_file);
	exit (0);
}
