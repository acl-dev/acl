#include "lib_acl.h"

static int echo_cmd(ACL_VSTREAM *client, const char *s)
{
	char  buf[1024];
	int   ret;

	if (acl_vstream_fprintf(client, "%s\r\n", s) == ACL_VSTREAM_EOF) {
		printf("send %s error\n", s);
		return (-1);
	}

	ret = acl_vstream_gets_nonl(client, buf, sizeof(buf));
	if (ret <= 0) {
		printf("get reply for %s error\n", s);
		return (-1);
	}
	printf(">>%s\n", s);
	printf("<<%s\n", buf);
	return (0);
}

int main(int argc, char *argv[])
{
	ACL_VSTREAM *client;
	const char *addr;
	char  buf[1024];
	int   ret, i = 0;

	if (argc != 2) {
		printf("usage: %s addr\n", argv[0]);
		return (0);
	}

	addr = argv[1];

	while (1) {
		i++;
		client = acl_vstream_connect(addr, ACL_BLOCKING, 0, 0, 4096);
		if (client == NULL) {
			printf("connect %s error(%s)\n", addr, acl_last_serror());
			return (1);
		}

		// get welcome banner
		ACL_METER_TIME("---begin get banner---");
		ret = acl_vstream_gets_nonl(client, buf, sizeof(buf));
		if (ret > 0) {
			printf("gets from %s: %s\n", addr, buf);
		} else {
			printf("gets welcome error\n");
			acl_vstream_close(client);
			break;
		}
		ACL_METER_TIME("---end get banner---");

		ACL_METER_TIME("---begin helo---");
		if (echo_cmd(client, "helo 126.com") < 0) {
			acl_vstream_close(client);
			break;
		}
		ACL_METER_TIME("---end helo ---");

		ACL_METER_TIME("---begin mail from---");
		if (echo_cmd(client, "mail from: <as@126.com>") < 0) {
			acl_vstream_close(client);
			break;
		}
		ACL_METER_TIME("---end mail from---");

		ACL_METER_TIME("---begin rcpt to---");
		//if (echo_cmd(client, "rcpt to: <admin@51iker.com>") < 0) {
		if (echo_cmd(client, "rcpt to: <admin@xgh.com>") < 0) {
			acl_vstream_close(client);
			break;
		}
		ACL_METER_TIME("---end rcpt to---");
		printf(">>>>----------------- i: %d----------------<<<\n", i);

#if 0
		ACL_METER_TIME("---begin data---");
		if (echo_cmd(client, "data") < 0) {
			acl_vstream_close(client);
			break;
		}

		ACL_METER_TIME("---end data---");

		ACL_METER_TIME("---begin .---");
		if (echo_cmd(client, ".") < 0) {
			acl_vstream_close(client);
			break;
		}

		ACL_METER_TIME("---end .---");

#elif 1
		ACL_METER_TIME("---begin rset---");
		if (echo_cmd(client, "rset") < 0) {
			acl_vstream_close(client);
			break;
		}
		ACL_METER_TIME("---end rset---");
#else
		ACL_METER_TIME("---begin quit---");
		if (echo_cmd(client, "quit") < 0) {
			acl_vstream_close(client);
			break;
		}
		ACL_METER_TIME("---end quit---");
#endif

		acl_vstream_close(client);
	}

	return (0);
}
