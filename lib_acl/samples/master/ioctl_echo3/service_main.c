#include "lib_acl.h"
#include "service_var.h"
#include "service_main.h"

typedef struct
{
	int   n;
	char  s[128];
} MY_CTX;

/* 初始化函数 */
void service_init(void *init_ctx acl_unused)
{
	const char *myname = "service_init";

	acl_msg_info("%s: init ok ...", myname);
}

/* 进程退出前调用的函数 */
void service_exit(void *arg acl_unused)
{
	const char *myname = "service_exit";

	acl_msg_info("%s: exit now ...", myname);
}

static void on_close(ACL_VSTREAM *client, void *arg)
{
	MY_CTX *ctx = (MY_CTX*) arg;
	acl_msg_info("stream close now, n: %d, rw_timeout: %d",
		ctx->n, client->rw_timeout);
	acl_myfree(ctx);
}

int service_on_accept(void *arg acl_unused, ACL_VSTREAM *client)
{
	MY_CTX *ctx = (MY_CTX*) client->context;

	acl_vstream_fprintf(client, "hello, you're welcome!\r\n");
	ctx = (MY_CTX*) acl_mycalloc(1, sizeof(MY_CTX));
	snprintf(ctx->s, sizeof(ctx->s), "hello world!");
	ctx->n = 1;
	client->context = ctx;
	acl_vstream_add_close_handle(client, on_close, ctx);
	return (0);
}

/* 协议处理函数入口 */
int service_main(void *run_ctx acl_unused, ACL_VSTREAM *client)
{
	const char *myname = "service_main";
	int   ret, ready;
	ACL_VSTRING *buf = acl_vstring_alloc(1024);

	acl_msg_info(">>>one coonected!, rw_timeout: %d", client->rw_timeout);

	while (1) {
		ready = 0;
		ACL_VSTRING_RESET(buf);
		ret = acl_vstream_gets_peek(client, buf, &ready);
		if (ret == ACL_VSTREAM_EOF) {
			if (var_cfg_debug_enable)
				acl_msg_info("%s: close client now, (%s), timeout: %d",
					myname, var_cfg_debug_msg, client->rw_timeout);
			acl_vstring_free(buf);
			return (-1);  /* 返回负值以使框架内部关闭 client 数据流 */
		} else if (!ready)
			break;

		if (acl_vstream_writen(client, acl_vstring_str(buf), ret) == ACL_VSTREAM_EOF) {
			if (var_cfg_debug_enable)
				acl_msg_info("%s: write to client error, close now(%s)",
					myname, var_cfg_debug_msg);
			acl_vstring_free(buf);
			return (-1);  /* 返回负值以使框架内部关闭 client 数据流 */
		}
	}
	acl_vstring_free(buf);

	if (var_cfg_keep_alive) {
		if (var_cfg_debug_enable)
			acl_msg_info("%s: keep alive, wait client...", myname);
		return (0);  /* 返回 0 以使框架内部自动监听该数据流从而保持长连接 */
	} else {
		/* 可以在此处返回 =1, 使框架内部自动关闭 client 数据流,
		 * 也可以在此处直接关闭 client 数据流，同时返回 1 告诉框架
		 * 该流已经被用户关闭了不必再关心该 client 数据流.
		 */
		acl_vstream_close(client);
		return (1);
	}
}
