#include "stdafx.h"
#include "redis_status.h"
#include "redis_util.h"
#include "redis_builder.h"
#include "redis_reshard.h"

//////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s redis_addr[ip:port]\r\n"
		"-a cmd[nodes|slots|create|add_node|del_node|node_id|reshard]\r\n"
		"-N new_node[ip:port]\r\n"
		"-S [add node as slave]\r\n"
		"-r replicas[default 0]\r\n"
		"-f configure_file\r\n",
		procname);

	printf("\r\nfor samples:\r\n"
		"%s -s 127.0.0.1:6379 -a create -f cluster.xml\r\n"
		"%s -s 127.0.0.1:6379 -a create -f nodes4.xml -r 2\r\n"
		"%s -s 127.0.0.1:6379 -a nodes\r\n"
		"%s -s 127.0.0.1:6379 -a slots\r\n"
		"%s -s 127.0.0.1:6379 -a del_node -I node_id\r\n"
		"%s -s 127.0.0.1:6379 -a node_id\r\n"
		"%s -s 127.0.0.1:6379 -a reshard\r\n"
		"%s -s 127.0.0.1:6379 -a add_node -N 127.0.0.1:6380 -S\r\n",
		procname, procname, procname, procname,
		procname, procname, procname, procname);
}

int main(int argc, char* argv[])
{
	// initiation the acl library
	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	int  ch;
	size_t replicas = 0;
	bool add_slave = false;
	acl::string addr, cmd, conf, new_addr, node_id;

	while ((ch = getopt(argc, argv, "hs:a:f:N:SI:r:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'a':
			cmd = optarg;
			break;
		case 'f':
			conf = optarg;
			break;
		case 'N':
			new_addr = optarg;
			break;
		case 'S':
			add_slave = true;
			break;
		case 'I':
			node_id = optarg;
			break;
		case 'r':
			replicas = (size_t) atoi(optarg);
			break;
		default:
			break;
		}
	}

	int conn_timeout = 10, rw_timeout = 120;
	acl::redis_client client(addr, conn_timeout, rw_timeout);
	acl::redis redis(&client);

	if (cmd == "nodes")
	{
		if (addr.empty())
		{
			printf("usage: %s -s ip:port -a nodes\r\n", argv[0]);
			goto END;
		}
		redis_status status(addr, conn_timeout, rw_timeout);
		status.show_nodes(redis);
	}
	else if (cmd == "slots")
	{
		if (addr.empty())
		{
			printf("usage: %s -a ip:port -a slots\r\n", argv[0]);
			goto END;
		}
		redis_status status(addr, conn_timeout, rw_timeout);
		status.show_slots(redis);
	}
	else if (cmd == "create")
	{
		if (conf.empty())
		{
			printf("usage: %s -a create -f cluster.xml\r\n", argv[0]);
			goto END;
		}
		redis_builder builder;
		builder.build(conf.c_str(), replicas);
	}
	else if (cmd == "add_node")
	{
		if (addr.empty() || new_addr.empty())
		{
			printf("usage: %s -s ip:port -a add_node -N ip:port -S\r\n", argv[0]);
			goto END;
		}
		redis_builder builder;
		builder.add_node(addr, new_addr, add_slave);
	}
	else if (cmd == "del_node")
	{
		if (addr.empty() || node_id.empty())
		{
			printf("usage: %s -s ip:port -a del_node -I nod_id\r\n", argv[0]);
			goto END;
		}
		redis_builder builder;
		builder.del_node(addr, node_id);
	}
	else if (cmd == "node_id")
	{
		if (addr.empty())
		{
			printf("usage: %s -s ip:port -a node_id\r\n", argv[0]);
			goto END;
		}
		node_id.clear();
		if (redis_util::get_node_id(addr, node_id) == false)
			printf("can't get node id, addr: %s\r\n", addr.c_str());
		else
			printf("addr: %s, node_id: %s\r\n", addr.c_str(),
				node_id.c_str());
	}
	else if (cmd == "reshard")
	{
		if (addr.empty())
		{
			printf("usage: %s -s ip:port -a reshard\r\n", argv[0]);
			goto END;
		}

		redis_reshard reshard(addr);
		reshard.run();
	}
	else
		printf("unknown cmd: %s\r\n", cmd.c_str());

END:

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
