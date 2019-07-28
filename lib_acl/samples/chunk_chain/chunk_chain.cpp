// .cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "lib_acl.h"

static char  buf[4096];

static void chain_add(ACL_CHAIN *chain, int from, int dlen)
{
	printf("add: from=%d, to=%d, dlen=%d\r\n", from, from + dlen, dlen);
	acl_chain_add(chain, buf, from, dlen);
	acl_chain_list(chain);
	printf("data dlen=%d, chunk dlen=%d\r\n\r\n", acl_chain_data_len(chain),
		acl_chain_chunk_data_len(chain));
}

int _tmain(int argc, _TCHAR* argv[])
{
	ACL_CHAIN *chain;
	int   i;

	for (i = 0; i < sizeof(buf); i++)
	{
		buf[i] = 'X';
	}

	chain = acl_chain_new(1024, 0);
	acl_chain_reset(chain, 10);
	chain_add(chain, 6, 3);
	chain_add(chain, 10, 100);
	chain_add(chain, 200, 100);
	chain_add(chain, 400, 100);
	chain_add(chain, 600, 100);
	chain_add(chain, 800, 100);
	chain_add(chain, 250, 149);
	chain_add(chain, 1100, 100);
	chain_add(chain, 1000, 100);
	chain_add(chain, 1201, 100);
	chain_add(chain, 1300, 100);
	chain_add(chain, 1, 1);
	chain_add(chain, 3, 2);
	chain_add(chain, 0, 2000);

	acl_chain_free(chain);

	getchar();
	return 0;
}
