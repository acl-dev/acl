#include "stdafx.h"
#include "redis_commands.h"

redis_commands::redis_commands(const char* addr, const char* passwd,
	int conn_timeout, int rw_timeout)
	: addr_(addr)
	, conn_timeout_(conn_timeout)
	, rw_timeout_(rw_timeout)
	, conn_(addr, conn_timeout, rw_timeout)
{
	conns_.set(addr_, conn_timeout_, rw_timeout_);
	conns_.set_all_slot(addr, 0);

	if (passwd && *passwd)
	{
		passwd_ = passwd;
		conn_.set_password(passwd);
		conns_.set_password("default", passwd);
	}
}

redis_commands::~redis_commands(void)
{
}

void redis_commands::help(void)
{
	printf("> keys pattern\r\n");
	printf("> hgetall parameter\r\n");
	printf("> remove pattern\r\n");
	printf("> type parameter ...\r\n");
	printf("> ttl parameter ...\r\n");
}

const std::map<acl::string, acl::redis_node*>* redis_commands::get_masters(
	acl::redis& redis)
{
	const std::map<acl::string, acl::redis_node*>* masters =
		redis.cluster_nodes();
	if (masters == NULL)
		printf("masters NULL\r\n");
	return masters;
}

void redis_commands::run(void)
{
	acl::string buf;
	acl::stdin_stream in;

	while (!in.eof())
	{
		printf("> ");
		fflush(stdout);

		if (in.gets(buf) == false)
			break;
		if (buf.equal("quit", false) || buf.equal("exit", false)
			|| buf.equal("q", false))
		{
			printf("Bye!\r\n");
			break;
		}

		if (buf.empty() || buf.equal("help", false))
		{
			help();
			continue;
		}

		std::vector<acl::string>& tokens = buf.split2(" \t");
		acl::string& cmd = tokens[0];
		cmd.lower();
		if (cmd == "date")
			show_date();
		else if (cmd == "keys")
			get_keys(tokens);
		else if (cmd == "hgetall")
			hgetall(tokens);
		else if (cmd == "remove" || cmd == "rm")
			pattern_remove(tokens);
		else if (cmd == "type")
			check_type(tokens);
		else if (cmd == "ttl")
			check_ttl(tokens);
		else
			help();
	}
}

void redis_commands::show_date(void)
{
	char buf[256];
	acl::rfc822 rfc;
	rfc.mkdate_cst(time(NULL), buf, sizeof(buf));
	printf("Date: %s\r\n", buf);
}

void redis_commands::get_keys(const std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2)
	{
		printf("> usage: keys parameter\r\n");
		return;
	}

	acl::redis redis(&conns_);
	const std::map<acl::string, acl::redis_node*>* masters =
		get_masters(redis);
	if (masters == NULL)
	{
		printf("no masters!\r\n");
		return;
	}

	int  n = 0;
	for (std::map<acl::string, acl::redis_node*>::const_iterator cit =
		masters->begin(); cit != masters->end(); ++cit)
	{
		n += get_keys(cit->second->get_addr(), tokens[1]);
	}

	printf("-----keys %s: total count: %d----\r\n", tokens[1].c_str(), n);
}

int redis_commands::get_keys(const char* addr, const char* pattern)
{
	if (addr == NULL || *addr == 0)
	{
		printf("addr NULL\r\nEnter any key to continue ...\r\n");
		getchar();
		return 0;
	}

	acl::redis_client conn(addr, conn_timeout_, rw_timeout_);
	if (passwd_.empty() == false)
		conn.set_password(passwd_);

	std::vector<acl::string> res;
	acl::redis_key redis(&conn);
	if (redis.keys_pattern(pattern, &res) <= 0)
		return 0;

	if (res.size() >= 40)
	{
		printf("Total: %d. Do you want to show them all in %s ? [y/n]: ",
			(int) res.size(), addr);
		fflush(stdout);

		acl::stdin_stream in;
		acl::string buf;
		if (in.gets(buf) == false || !buf.equal("y", false))
			return (int) res.size();
	}

	for (std::vector<acl::string>::const_iterator cit = res.begin();
		cit != res.end(); ++cit)
	{
		printf("%s\r\n", (*cit).c_str());
	}

	printf("--- Total: %d, addr: %s ---\r\n", (int) res.size(), addr);

	return (int) res.size();
}

void redis_commands::hgetall(const std::vector<acl::string>& tokens)
{
	std::vector<acl::string>::const_iterator cit = tokens.begin();
	++cit;
	
	for (; cit != tokens.end(); ++cit)
	{
		const char* key = (*cit).c_str();
		std::map<acl::string, acl::string> res;
		acl::redis cmd(&conns_);

		if (cmd.hgetall(key, res) == false)
		{
			printf("hgetall error: %s, key: %s\r\n",
				cmd.result_error(), key);
			return;
		}

		printf("key: %s\r\n", key);
		for (std::map<acl::string, acl::string>::const_iterator cit2
			= res.begin(); cit2 != res.end(); ++cit2)
		{
			printf("%s: %s\r\n", cit2->first.c_str(),
				cit2->second.c_str());
		}
		printf("-----------------------------------------------\r\n");
	}
}

void redis_commands::pattern_remove(const std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2)
	{
		printf("> usage: pattern_remove pattern\r\n");
		return;
	}

	const char* pattern = tokens[1].c_str();

	acl::redis redis(&conns_);
	const std::map<acl::string, acl::redis_node*>* masters =
		get_masters(redis);
	if (masters == NULL)
	{
		printf("no masters!\r\n");
		return;
	}

	acl::stdin_stream in;
	acl::string buf;

	int deleted = 0;
	std::vector<acl::string> res;

	for (std::map<acl::string, acl::redis_node*>::const_iterator cit =
		masters->begin(); cit != masters->end(); ++cit)
	{
		const char* addr = cit->second->get_addr();
		if (addr == NULL || *addr == 0)
		{
			printf("addr NULL, skip it\r\n");
			continue;
		}

		acl::redis_client conn(addr, conn_timeout_, rw_timeout_);
		if (!passwd_.empty())
			conn.set_password(passwd_);

		redis.clear(false);
		redis.keys_pattern(pattern, &res);

		printf("addr: %s, pattern: %s, total: %d\r\n",
			addr, pattern, (int) res.size());

		if (res.empty())
			continue;

		printf("Do you want to delete them all in %s ? [y/n]: ", addr);
		fflush(stdout);

		if (in.gets(buf) && buf.equal("y", false))
		{
			int ret = remove(res);
			if (ret > 0)
				deleted += ret;
		}
	}

	printf("pattern: %s, total: %d\r\n", pattern, deleted);
}

int redis_commands::remove(const std::vector<acl::string>& keys)
{
	acl::redis cmd(&conns_);

	int  deleted = 0, error = 0, notfound = 0;

	for (std::vector<acl::string>::const_iterator cit = keys.begin();
		cit != keys.end(); ++cit)
	{
		cmd.clear(false);
		int ret = cmd.del_one((*cit).c_str());
		if (ret < 0)
		{
			printf("del_one error: %s, key: %s\r\n",
				cmd.result_error(), (*cit).c_str());
			error++;
		}
		else if (ret == 0)
		{
			printf("not exist, key: %s\r\n", (*cit).c_str());
			notfound++;
		}
		else
		{
			printf("Delete ok, key: %s\r\n", (*cit).c_str());
			deleted++;
		}
	}

	printf("Remove over, deleted: %d, error: %d, not found: %d\r\n",
		deleted, error, notfound);
	return deleted;
}

void redis_commands::check_type(const std::vector<acl::string>& tokens)
{
	acl::redis cmd(&conns_);
	std::vector<acl::string>::const_iterator cit = tokens.begin();
	++cit;
	for (; cit != tokens.end(); ++cit)
	{
		cmd.clear(false);
		const char* key = (*cit).c_str();
		acl::redis_key_t type = cmd.type(key);
		switch (type)
		{
		case acl::REDIS_KEY_NONE:
			printf("%s: NONE\r\n", key);
			break;
		case acl::REDIS_KEY_STRING:
			printf("%s: STRING\r\n", key);
			break;
		case acl::REDIS_KEY_HASH:
			printf("%s: HASH\r\n", key);
			break;
		case acl::REDIS_KEY_LIST:
			printf("%s: LIST\r\n", key);
			break;
		case acl::REDIS_KEY_SET:
			printf("%s: SET\r\n", key);
			break;
		case acl::REDIS_KEY_ZSET:
			printf("%s: ZSET\r\n", key);
			break;
		default:
			printf("%s: UNKNOWN\r\n", key);
			break;
		}
	}
}

void redis_commands::check_ttl(const std::vector<acl::string>& tokens)
{
	acl::redis cmd(&conns_);
	std::vector<acl::string>::const_iterator cit = tokens.begin();
	++cit;
	for (; cit != tokens.end(); ++cit)
	{
		cmd.clear(false);
		const char* key = (*cit).c_str();
		int ttl = cmd.ttl(key);
		printf("%s: %d seconds\r\n", key, ttl);
	}
}
