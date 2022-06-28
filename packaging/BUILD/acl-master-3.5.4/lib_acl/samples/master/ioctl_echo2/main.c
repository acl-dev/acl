#include "lib_acl.h"
#include "service_main.h"

int main(int argc, char *argv[])
{
	acl_ioctl_app_main(argc, argv, service_main, NULL,
		ACL_APP_CTL_INIT_FN, service_init,
		/* ACL_APP_CTL_INIT_CTX, NULL, */
		ACL_APP_CTL_EXIT_FN, service_exit,
		/* ACL_APP_CTL_EXIT_CTX, NULL, */
		ACL_APP_CTL_CFG_BOOL, service_conf_bool_tab,
		ACL_APP_CTL_CFG_INT, service_conf_int_tab,
		ACL_APP_CTL_CFG_STR, service_conf_str_tab,
		ACL_APP_CTL_END);

	exit (0);
}

