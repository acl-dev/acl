#include "app_main.h"
#include "service_main.h"

int main(int argc, char *argv[])
{
	app_main(argc, argv, service_main, NULL,
		APP_CTL_INIT_FN, service_init,
		/* APP_CTL_INIT_CTX, NULL, */
		APP_CTL_EXIT_FN, service_exit,
		/* APP_CTL_EXIT_CTX, NULL, */
		APP_CTL_CFG_BOOL, service_conf_bool_tab,
		APP_CTL_CFG_INT, service_conf_int_tab,
		APP_CTL_CFG_STR, service_conf_str_tab,
		APP_CTL_END);

	exit (0);
}

