#include "lib_acl.h"

/* 配置文件项 */

/* configure info */

/* TODO: you can add configure items here */

ACL_CONFIG_BOOL_TABLE __conf_bool_tab[] = {
	/* TODO: you can add configure variables of int type here */

	{ 0, 0, 0 },
};

ACL_CONFIG_INT_TABLE __conf_int_tab[] = {
	/* TODO: you can add configure variables of int type here */

	{ 0, 0, 0, 0, 0 },
};

ACL_CONFIG_STR_TABLE __conf_str_tab[] = {
	/* TODO: you can add configure variables of (char *) type here */

	{ 0, 0, 0 },
};

static void __pre_jail_init(void *ctx acl_unused)
{
}

static void __post_jail_init(void *ctx acl_unused)
{
}

static void service_exit(void *ctx acl_unused)
{
}

/* 协议处理函数入口 */
static void service_main(void *ctx acl_unused, ACL_VSTREAM *client)
{
	char  buf[256];
	int   ret;

	ACL_VSTREAM_SET_RWTIMO(client, 10);

	ret = acl_vstream_read(client, buf, sizeof(buf));
	if (ret == ACL_VSTREAM_EOF) {
		if (0)
			acl_msg_error("read error %s, local: %s, peer: %s",
				acl_last_serror(), ACL_VSTREAM_LOCAL(client),
				ACL_VSTREAM_PEER(client));
		return;
	}
	buf[ret] = 0;

	if (0)
		acl_msg_info("read: %s", buf);

	ret = acl_vstream_write(client, buf, strlen(buf));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("read error %s, local: %s, peer: %s",
			acl_last_serror(), ACL_VSTREAM_LOCAL(client),
			ACL_VSTREAM_PEER(client));
		return;
	}
}

int main(int argc, char *argv[])
{
	acl_udp_server_main(argc, argv, service_main,
			ACL_MASTER_SERVER_INT_TABLE, __conf_int_tab,
			ACL_MASTER_SERVER_STR_TABLE, __conf_str_tab,
			ACL_MASTER_SERVER_BOOL_TABLE, __conf_bool_tab,
			ACL_MASTER_SERVER_PRE_INIT, __pre_jail_init,
			ACL_MASTER_SERVER_POST_INIT, __post_jail_init,
			ACL_MASTER_SERVER_EXIT, service_exit,
			0);
	exit (0);
}
