
#include <stdio.h>
#include <stdlib.h>
#include "lib_acl.h"

#include "test_main.h"

int main(int argc, char *argv[])
{
	int   ret, i;

	acl_lib_init();

	if (argc != 2) {
		printf("usage: %s conf_file\n", argv[0]);
		exit (1);
	}

	ret = aut_cfg_parse(argv[1]);
	if (ret < 0) {
		printf("aut_cfg_parse error, file=%s\n", argv[1]);
		exit (1);
	}

	for (i = 0; __test_entry_tab[i].register_fn; i++)
		__test_entry_tab[i].register_fn();

	ret = aut_start();
	if (ret < 0) {
		printf("aut_start error(%d)\n", ret);
		exit (1);
	} else {
		printf("aut_start ok\n");
		exit (0);
	}
}

