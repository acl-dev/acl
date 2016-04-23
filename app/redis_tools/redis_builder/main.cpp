#include "stdafx.h"
#include "redis_status.h"
#include "redis_monitor.h"
#include "redis_util.h"
#include "redis_builder.h"
#include "redis_reshard.h"
#include "redis_commands.h"
#include "redis_cmdump.h"

//////////////////////////////////////////////////////////////////////////

#ifdef	HAS_READLINE
#include <readline/readline.h>
#include <readline/history.h>

static void test_readline(void)
{
	while (true)
	{
		char* ptr = readline("> ");
		if (ptr == NULL || *ptr == 0)
			break;
		if (strcasecmp(ptr, "q") == 0)
		{
			printf("Bye!\r\n");
			break;
		}
		printf("%s", ptr);
		add_history(ptr);
	}
}
#endif

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		" -s redis_addr[ip:port]\r\n"
		" -a cmd[nodes|slots|create|add_node|del_node|node_id|reshard|hash_slot|status|dump|run]\r\n"
		" -p passwd\r\n"
		" -N new_node[ip:port]\r\n"
		" -S [add node as slave]\r\n"
		" -r replicas[default 0]\r\n"
		" -d [if just display the result for create command]\r\n"
		" -k key\r\n"
		" -T dump_to_file\r\n"
		" -A [dump_all_masters_cmds, default: no]\r\n"
		" -M [prefer using masters]\r\n"
		" -f configure_file\r\n"
		" -F redis_commands_file\r\n",
		procname);

	printf("\r\nabout command:\r\n"
		" nodes: display information about nodes of the cluster\r\n"
		" slots: display information about slots of the cluster\r\n"
		" create: build a cluster after all nodes started\r\n"
		" add_node: add a node specified by -N to another by -s\r\n"
		" del_node: remove a node specified by -N from anothrer by -s\r\n"
		" node_id: get the address of one node specified by -s\r\n"
		" reshard: resharding the slots in all the nodes of the cluster\r\n");

	printf("\r\nfor samples:\r\n"
		" %s -s 127.0.0.1:6379 -a create -f cluster.xml\r\n"
		" %s -s 127.0.0.1:6379 -a create -f cluster.xml p 123456\r\n"
		" %s -s 127.0.0.1:6379 -a create -f nodes4.xml -r 2\r\n"
		" %s -s 127.0.0.1:6379 -a nodes\r\n"
		" %s -s 127.0.0.1:6379 -a nodes -p 123456\r\n"
		" %s -s 127.0.0.1:6379 -a slots\r\n"
		" %s -s 127.0.0.1:6379 -a del_node -I node_id\r\n"
		" %s -s 127.0.0.1:6379 -a node_id\r\n"
		" %s -s 127.0.0.1:6379 -a reshard\r\n"
		" %s -a hash_slot -k key\r\n"
		" %s -s 127.0.0.1:6379 -a status\r\n"
		" %s -s 127.0.0.1:6379 -a dump -f dump.txt -A all -M\r\n"
		" %s -s 127.0.0.1:6379 -a add_node -N 127.0.0.1:6380 -S\r\n",
		procname, procname, procname, procname, procname, procname,
		procname, procname, procname, procname, procname, procname,
		procname);
}

int main(int argc, char* argv[])
{
	// initiation the acl library
	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	int  ch;
	size_t replicas = 0;
	bool add_slave = false, just_display = false;
	acl::string addr, cmd, conf, new_addr, node_id, key, passwd;
	bool dump_all = false, prefer_master = false;
	acl::string filepath("dump.txt"), cmds_file;

	while ((ch = getopt(argc, argv, "hs:a:f:N:SI:r:dk:p:T:AMF:")) > 0)
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
		case 'd':
			just_display = true;
			break;
		case 'k':
			key = optarg;
			break;
		case 'p':
			passwd = optarg;
			break;
		case 'T':
			filepath = optarg;
			break;
		case 'A':
			dump_all = true;
			break;
		case 'M':
			prefer_master = true;
			break;
		case 'F':
			cmds_file = optarg;
			break;
		default:
			break;
		}
	}

	int conn_timeout = 10, rw_timeout = 120;

	if (cmd == "hash_slot")
	{
		if (key.empty())
			printf("usage: %s -a hash_slot -k key\r\n", argv[0]);
		else
		{
			size_t max_slot = 16384;
			unsigned short n = acl_hash_crc16(key.c_str(), key.length());
			unsigned short slot = n %  max_slot;
			printf("key: %s, slot: %d\r\n", key.c_str(), (int) slot);
		}
	}
	else if (cmd == "nodes")
	{
		if (addr.empty())
			printf("usage: %s -s ip:port -a nodes\r\n", argv[0]);
		else {
			acl::redis_client client(addr, conn_timeout, rw_timeout);
			client.set_password(passwd);
			acl::redis redis(&client);
			redis_status status(addr, conn_timeout, rw_timeout, passwd);
			status.show_nodes(redis);
		}
	}
	else if (cmd == "slots")
	{
		if (addr.empty())
			printf("usage: %s -a ip:port -a slots\r\n", argv[0]);
		else
		{
			acl::redis_client client(addr, conn_timeout, rw_timeout);
			client.set_password(passwd);
			acl::redis redis(&client);
			redis_status status(addr, conn_timeout, rw_timeout, passwd);
			status.show_slots(redis);
		}
	}
	else if (cmd == "create")
	{
		if (conf.empty())
			printf("usage: %s -a create -f cluster.xml\r\n", argv[0]);
		else
		{
			redis_builder builder(passwd);
			builder.build(conf.c_str(), replicas, just_display);
		}
	}
	else if (cmd == "add_node")
	{
		if (addr.empty() || new_addr.empty())
			printf("usage: %s -s ip:port -a add_node -N ip:port -S\r\n", argv[0]);
		else
		{
			redis_builder builder(passwd);
			builder.add_node(addr, new_addr, add_slave);
		}
	}
	else if (cmd == "del_node")
	{
		if (addr.empty() || node_id.empty())
			printf("usage: %s -s ip:port -a del_node -I nod_id\r\n", argv[0]);
		else
		{
			redis_builder builder(passwd);
			builder.del_node(addr, node_id);
		}
	}
	else if (cmd == "node_id")
	{
		if (addr.empty())
			printf("usage: %s -s ip:port -a node_id\r\n", argv[0]);
		else
		{
			node_id.clear();
			if (redis_util::get_node_id(addr, node_id, passwd) == false)
				printf("can't get node id, addr: %s\r\n",
					addr.c_str());
			else
				printf("addr: %s, node_id: %s\r\n",
					addr.c_str(), node_id.c_str());
		}
	}
	else if (cmd == "reshard")
	{
		if (addr.empty())
			printf("usage: %s -s ip:port -a reshard\r\n", argv[0]);
		else
		{
			redis_reshard reshard(addr, passwd);
			reshard.run();
		}
	}
	else if (cmd == "status")
	{
		if (addr.empty())
			printf("usage: %s -s ip:port -a status\r\n", argv[0]);
		else
		{
			redis_monitor monitor(addr, conn_timeout, rw_timeout,
				passwd, prefer_master);
			monitor.status();
		}
	}
	else if (cmd == "dump")
	{
		if (addr.empty())
			printf("usage: %s -s ip:port -a monitor -f dump.txt\r\n", argv[0]);
		else
		{
			redis_cmdump dump(addr, conn_timeout, rw_timeout,
				passwd, prefer_master);
			dump.saveto(filepath, dump_all);
		}
	}
	else if (cmd == "run" || cmd.empty())
	{
		acl::string path;
		const char* ptr = acl_process_path();
		if (ptr && *ptr)
		{
			path.dirname(ptr);
			filepath.format("%s/redis_commands.txt", path.c_str());
			acl::ifstream in;
			if (in.open_read(filepath))
			{
				in.close();
				cmds_file = filepath;
			}
		}
		redis_commands cmds(addr, passwd, conn_timeout,
			rw_timeout, prefer_master, cmds_file);
		cmds.run();
	}
#ifdef	HAS_READLINE
	else if (cmd == "readline")
		test_readline();
#endif
	else
		printf("unknown cmd: %s\r\n", cmd.c_str());

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
