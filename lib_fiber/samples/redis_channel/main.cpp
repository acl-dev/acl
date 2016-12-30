#include "stdafx.h"
//#include "stamp.h"

static int __fibers_count = 2;
static int __fibers_max   = 2;
static int __oper_count = 100;
//static struct timeval __begin, __end;
static ACL_FIBER **__workers = NULL;
static ACL_CHANNEL *__chan_exit = NULL;

static acl::redis_client_cluster __redis_cluster;

typedef struct {
	ACL_CHANNEL *chan;
	int   id;
	bool  busy;
	acl::string cmd;
} MYCHAN;

typedef struct {
	MYCHAN *chans;
	size_t  size;
	size_t  off;
} MYCHANS;

typedef struct {
	acl::string cmd;
	acl::string key;
	acl::string val;
	bool success;
} PKT;

static bool redis_set(ACL_FIBER& fiber, ACL_CHANNEL &chan, PKT& pkt)
{
	acl::redis cmd(&__redis_cluster);

	pkt.success = false;

	if (pkt.key.empty())
	{
		printf("%s(%d): fiber-%d: key empty!\r\n",
			__FUNCTION__, __LINE__, acl_fiber_id(&fiber));
		pkt.val = "key empty";
		acl_channel_sendp(&chan, &pkt);

		return false;
	}
	if (pkt.val.empty())
	{
		printf("%s(%d): fiber-%d: val empty\r\n",
			__FUNCTION__, __LINE__, acl_fiber_id(&fiber));
		pkt.val = "val empty";
		acl_channel_sendp(&chan, &pkt);

		return false;
	}

	if (cmd.set(pkt.key, pkt.val) == false)
	{
		printf("%s(%d): fiber-%d: set error, key: %s, val: %s\r\n",
			__FUNCTION__, __LINE__, acl_fiber_id(&fiber),
			pkt.key.c_str(), pkt.val.c_str());
		acl_channel_sendp(&chan, &pkt);

		return false;
	}

	pkt.success = true;
	if (acl_channel_sendp(&chan, &pkt) < 0)
	{
		printf("%s(%d): fiber-%d: acl_channel_sendp error, key %s\r\n",
			__FUNCTION__, __LINE__, acl_fiber_id(&fiber),
			pkt.key.c_str());

		return false;
	}

	return true;
}

static bool redis_get(ACL_FIBER& fiber, ACL_CHANNEL &chan, PKT &pkt)
{
	acl::redis cmd(&__redis_cluster);

	pkt.success = false;

	if (pkt.key.empty())
	{
		printf("fiber-%d: key empty!\r\n", acl_fiber_id(&fiber));
		pkt.val = "key empty";
		acl_channel_sendp(&chan, &pkt);

		return false;
	}

	if (cmd.get(pkt.key, pkt.val) == false)
	{
		printf("fiber-%d: get error, key: %s\r\n",
			acl_fiber_id(&fiber), pkt.key.c_str());
		pkt.val = "get error";
		acl_channel_sendp(&chan, &pkt);

		return false;
	}

	pkt.success = true;
	if (acl_channel_sendp(&chan, &pkt) < 0)
	{
		printf("fiber-%d: acl_channel_sendp error, key: %s\r\n",
			acl_fiber_id(&fiber), pkt.key.c_str());

		return false;
	}

	return true;
}

static bool redis_del(ACL_FIBER& fiber, ACL_CHANNEL &chan, PKT &pkt)
{
	acl::redis cmd(&__redis_cluster);

	pkt.success = false;

	if (pkt.key.empty())
	{
		printf("fiber-%d: key empty!\r\n", acl_fiber_id(&fiber));
		pkt.val = "key empty";
		acl_channel_sendp(&chan, &pkt);

		return false;
	}

	if (cmd.del_one(pkt.key) < 0)
	{
		printf("fiber-%d: del_one error, key: %s\r\n",
			acl_fiber_id(&fiber), pkt.key.c_str());
		pkt.val = "del error";
		acl_channel_sendp(&chan, &pkt);

		return false;
	}

	pkt.success = true;
	if (acl_channel_sendp(&chan, &pkt) < 0)
	{
		printf("fiber-%d: acl_channel_sendp error, key: %s\r\n",
			acl_fiber_id(&fiber), pkt.key.c_str());

		return false;
	}

	return true;
}

static void fiber_worker(ACL_FIBER *fiber, void *ctx)
{
	ACL_CHANNEL *chan = ((MYCHAN *) ctx)->chan;

	while (true)
	{
		PKT* pkt = (PKT *) acl_channel_recvp(chan);

		if (pkt == NULL)
		{
			printf("fiber-%d: acl_channel_recvp NULL\r\n",
				acl_fiber_id(fiber));
			break;
		}

		if (pkt->cmd.equal("set", false))
		{
			if (redis_set(*fiber, *chan, *pkt) == false)
			{
				printf("fiber-%d: redis_set error\r\n",
					acl_fiber_id(fiber));
				break;
			}
		}
		else if (pkt->cmd.equal("get", false))
		{
			if (redis_get(*fiber, *chan, *pkt) == false)
			{
				printf("fiber-%d: redis_get error\r\n",
					acl_fiber_id(fiber));
				break;
			}
		}
		else if (pkt->cmd.equal("del", false))
		{
			if (redis_del(*fiber, *chan, *pkt) == false)
			{
				printf("fiber-%d: redis_del error\r\n",
					acl_fiber_id(fiber));
				break;
			}
		}
		else
			printf("unknown cmd: %s\r\n", pkt->cmd.c_str());
	}
}

static int __display = 0;

static void fiber_result(ACL_FIBER *fiber, void *ctx)
{
	MYCHANS *mychans  = (MYCHANS *) ctx;
	MYCHAN  *mychan   = &mychans->chans[mychans->off++];
	ACL_CHANNEL *chan = mychan->chan;
	PKT pkt;

	if (mychans->off == mychans->size)
		mychans->off = 0;

	pkt.cmd = mychan->cmd;

	for (int i = 0; i < __oper_count; i++)
	{
		pkt.key.format("key-%d-%d", acl_fiber_id(fiber), i);
		pkt.val.format("val-%d-%d", acl_fiber_id(fiber), i);

		if (acl_channel_sendp(chan, &pkt) < 0)
		{
			printf("%s(%d): fiber-%d: acl_channel_sendp error, key = %s\r\n",
				__FUNCTION__, __LINE__, acl_fiber_id(fiber),
				pkt.key.c_str());
			break;
		}

		PKT* res = (PKT *) acl_channel_recvp(chan);
		if (res == NULL)
		{
			printf("%s(%d): fiber-%d: acl_channel_recvp error, key = %s\r\n",
				__FUNCTION__, __LINE__, acl_fiber_id(fiber),
				pkt.key.c_str());
			break;
		}

		//assert(res == &pkt);

		if (!res->success)
		{
			printf("%s(%d): fiber-%d: cmd = %s, key = %s, failed\r\n",
				__FUNCTION__, __LINE__, acl_fiber_id(fiber),
				pkt.cmd.c_str(), pkt.key.c_str());
			continue;
		}

		if (++__display >= 10)
			continue;

		if (pkt.cmd.equal("get", false))
			printf("fiber-%d: cmd = %s, key = %s, val = %s\r\n",
				acl_fiber_id(fiber), pkt.cmd.c_str(),
				pkt.key.c_str(), res->val.c_str());
		else
			printf("fiber-%d: cmd = %s, key = %s\r\n",
				acl_fiber_id(fiber), pkt.cmd.c_str(),
				pkt.key.c_str());
	}

	if (--__fibers_count == 0)
	{
		printf("---All fibers are over!---\r\n");
		unsigned long n = 100;
		acl_channel_sendul(__chan_exit, n);
	}
}

static void fiber_wait(ACL_FIBER *, void *ctx)
{
	ACL_CHANNEL *chan = (ACL_CHANNEL *) ctx;
	unsigned long n = acl_channel_recvul(chan);
	
	printf("----fiber-%d: get n: %lu---\r\n", acl_fiber_self(), n);

	for (int i = 0; __workers[i] != NULL; i++)
	{
		printf("kill fiber-%d\r\n", acl_fiber_id(__workers[i]));
		acl_fiber_kill(__workers[i]);
	}

	printf("---- fiber schedul stopping now ----\r\n");
	acl_fiber_schedule_stop();
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s redis_addr\r\n"
		" -a command[set|get|del]\r\n"
		" -n operation_count\r\n"
		" -c fibers count\r\n"
		" -w workers count\r\n"
		" -t connect timoeut\r\n"
		" -r rw_timeout\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, conn_timeout = 0, rw_timeout = 0, nworkers = 10;
	acl::string addr("127.0.0.1:6379"), cmd("set");

	while ((ch = getopt(argc, argv, "hs:n:c:r:t:w:a:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'a':
			cmd = optarg;
			break;
		case 'w':
			nworkers = atoi(optarg);
			break;
		case 's':
			addr = optarg;
			break;
		case 'n':
			__oper_count = atoi(optarg);
			break;
		case 'c':
			__fibers_max = atoi(optarg);
			break;
		case 'r':
			rw_timeout = atoi(optarg);
			break;
		case 't':
			conn_timeout = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();

	__redis_cluster.set(addr.c_str(), 0, conn_timeout, rw_timeout);

	//gettimeofday(&__begin, NULL);

	if (nworkers > __fibers_max)
		nworkers = __fibers_max;

	nworkers = __fibers_max;

	MYCHANS mychans;
	mychans.size  = nworkers;
	mychans.off   = 0;
	mychans.chans = new MYCHAN[nworkers];

	for (int i = 0; i < nworkers; i++)
	{
		mychans.chans[i].chan = acl_channel_create(sizeof(void*), 1000);
		mychans.chans[i].cmd  = cmd;
	}

	__workers = new ACL_FIBER*[nworkers + 1];
	for (int i = 0; i < nworkers; i++)
		__workers[i] =
			acl_fiber_create(fiber_worker, &mychans.chans[i], 32000);

	__workers[nworkers] = NULL;

	__fibers_count = nworkers;

	for (int i = 0; i < __fibers_max; i++)
		(void) acl_fiber_create(fiber_result, &mychans, 32000);

	__chan_exit = acl_channel_create(sizeof(unsigned long), 1000);
	acl_fiber_create(fiber_wait, __chan_exit, 32000);

	acl_fiber_schedule();

	for (int i = 0; i < nworkers; i++)
		acl_channel_free(mychans.chans[i].chan);

	delete [] mychans.chans;
	acl_channel_free(__chan_exit);

	delete [] __workers;

	return 0;
}
