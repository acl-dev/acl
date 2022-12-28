#include "stdafx.h"
#include "test.h"

int main(int argc, char *argv[])
{
	int   ret, i;
	const char *conf = "test.cf";

	acl_lib_init();

	if (argc >= 2) {
		conf = argv[1];
	}

	printf("Begin load configure from %s\r\n", conf);

	ret = aut_cfg_parse(conf);
	if (ret < 0) {
		printf("aut_cfg_parse error, file=%s\n", argv[1]);
		return 1;
	}

	for (i = 0; __test_entry_tab[i].register_fn != NULL; i++) {
		__test_entry_tab[i].register_fn();
	}

	ret = aut_start();

	printf("-------------------------------------------------------\r\n");

	if (ret < 0) {
		printf("aut_start error(%d)\n", ret);
		return 1;
	} else {
		printf("aut_start ok\n");
		return 0;
	}
}

