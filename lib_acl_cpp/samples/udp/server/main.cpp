#include "stdafx.h"
#include "udp_sock.h"
#include "udp_pkt.h"

static void mio(udp_sock& sock, int inter, bool echo)
{
#define PKT_CNT 20
#define BUF_LEN 1024

	udp_pkts upkts(PKT_CNT);
	int nread = 0, total_read = 0, n = 0, nwrite = 0;

	while (true)
	{
		int ret = sock.recv(upkts);
		if (ret <= 0)
		{
			printf("read error %s, ret: %d\r\n",
				acl::last_serror(), ret);
			break;
		}

		nread++;
		total_read += ret;

		if (echo)
		{
			ret = sock.send(upkts);
			if (ret <= 0)
			{
				printf("send error %s, ret: %d\r\n",
					acl::last_serror(), ret);
				break;
			}
			nwrite++;
		}

		if (++n % inter == 0) {
			char buf[256];

			snprintf(buf, sizeof(buf),
				"curr: %d, total: %d, ret: %d, dlen: %lu, "
				"nread: %d, nwrite: %d, ip: %s, port: %d",
				n, total_read, ret,
				upkts[0]->get_dlen(),
				nread, nwrite,
				upkts[0]->get_ip(),
				upkts[0]->get_port());
			ACL_METER_TIME(buf);
		}

	}
}

int main(void)
{
	const char* addr = "127.0.0.1:8888";
	udp_sock sock;
	if (sock.server_open(addr) == false)
	{
		printf("server_open %s error %s\r\n",
			addr, acl::last_serror());
		return 1;
	}

	mio(sock, 50000, true);

	return 0;
}
