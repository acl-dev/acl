#include "lib_acl.h"
#include <assert.h>
#include "service_main.h"
#include "service_var.h"

static void service_test(void)
{
	const char *addr = "127.0.0.1:8885";
	ACL_VSTREAM *sstream = acl_vstream_listen(addr, 32), *client;
	int   ret;

	assert(sstream != NULL);

	acl_xinetd_params_int_table(NULL, var_conf_int_tab);
	acl_xinetd_params_str_table(NULL, var_conf_str_tab);
	acl_xinetd_params_bool_table(NULL, var_conf_bool_tab);

	printf("listen %s ok\n", addr);

	while (1) {
		client = acl_vstream_accept(sstream, NULL, 0);
		if (client == NULL) {
			printf("accept error: %s\n", acl_last_serror());
			break;
		}

		while (1) {
			ret = service_main(NULL, client);
			if (ret < 0) {
				acl_vstream_close(client);
				break;
			}
			if (ret > 0)
				break;
		}
	}

	acl_vstream_close(sstream);
}

int main(int argc, char *argv[])
{
	if (argc == 2 && strcasecmp(argv[1], "test") == 0) {
		service_test();
	} else {
		acl_threads_server_main(argc, argv, service_main, NULL,
				ACL_APP_CTL_ON_ACCEPT, service_on_accept,
				ACL_APP_CTL_INIT_FN, service_init,
				ACL_APP_CTL_EXIT_FN, service_exit,
				ACL_APP_CTL_CFG_BOOL, var_conf_bool_tab,
				ACL_APP_CTL_CFG_INT, var_conf_int_tab,
				ACL_APP_CTL_CFG_STR, var_conf_str_tab,
				ACL_APP_CTL_END);
	}
	return (0);
}
