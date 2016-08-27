#include "stdafx.h"
#include "redis_util.h"
#include "redis_status.h"
#include "redis_commands.h"

#ifdef	HAS_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#define LIMIT	40

static const REDIS_CMD  __redis_cmds[] =
{
	{ "ALL",		"none",		"yes" },
	{ "BGREWRITEAOF",	"all",		"yes" },
	{ "BGSAVE",		"all",		"yes" },
	{ "CONFIG",		"all",		"yes" },
	{ "DBSIZE",		"master",	"yes" },
	{ "FLUSHALL",		"master",	"yes" },
	{ "FLUSHDB",		"master",	"yes" },
	{ "LASTSAVE",		"all",		"yes" },
	{ "MONITOR",		"master",	"yes" },
	{ "PSYNC",		"all",		"yes" },
	{ "SAVE",		"all",		"yes" },
	{ "SHUTDOWN",		"all",		"yes" },
	{ "SLOWLOG",		"all",		"yes" },
	{ "SYNC",		"all",		"yes" },
	{ "TIME",		"all",		"yes" },
	{ "KEYS",		"master",	"yes" },
	{ "SCAN",		"master",	"yes" },
	{ "",			"none",		"no"  },
};

redis_commands::redis_commands(const char* addr, const char* passwd,
	int conn_timeout, int rw_timeout, bool prefer_master,
	const char* cmds_file)
	: conn_timeout_(conn_timeout)
	, rw_timeout_(rw_timeout)
	, prefer_master_(prefer_master)
	, all_cmds_perm_("yes")
{
	if (passwd && *passwd)
		passwd_ = passwd;

	set_addr(addr, addr_);
	conns_ = NULL;

	init(cmds_file);
	create_cluster();
}

redis_commands::~redis_commands(void)
{
	delete conns_;
}

void redis_commands::init(const char* cmds_file)
{
	set_commands();

	if (cmds_file && *cmds_file)
	{
		acl::ifstream in;
		if (in.open_read(cmds_file) == false)
		{
			logger_error("load file %s error: %s",
				cmds_file, acl::last_serror());
			return;
		}

		load_commands(in);
	}

	show_commands();
}

void redis_commands::set_commands(void)
{
	for (size_t i = 0; !__redis_cmds[i].cmd.empty(); i++)
	{
		acl::string cmd(__redis_cmds[i].cmd);
		cmd.lower();

		std::map<acl::string, REDIS_CMD>::const_iterator cit =
			redis_cmds_.find(cmd);
		if (cit != redis_cmds_.end())
			continue;

		redis_cmds_[cmd] = __redis_cmds[i];
	}
}

void redis_commands::load_commands(acl::istream& in)
{
	acl::string line;
	size_t i = 0;

	while (!in.eof())
	{
		if (in.gets(line) == false)
			break;

		i++;
		line.trim_left_space().trim_right_space();
		if (line.empty() || line[0] == '#')
			continue;

		add_cmdline(line, i);
	}
}

void redis_commands::add_cmdline(acl::string& line, size_t i)
{
	std::vector<acl::string>& tokens = line.split2(" \t|:,;");
	if (tokens.size() < 3)
	{
		logger_warn("skip line(%d): %s", (int) i, line.c_str());
		return;
	}

	acl::string cmd(tokens[0]);
	cmd.upper();

	if (cmd == "ALL")
	{
		all_cmds_perm_ = tokens[2];
		all_cmds_perm_.lower();
		if (all_cmds_perm_ == "warn" || all_cmds_perm_ == "no")
			return;
		return;
	}

	REDIS_CMD redis_cmd;
	redis_cmd.cmd = cmd;
	redis_cmd.broadcast = tokens[1].lower();

	redis_cmd.perm = tokens[2];
	redis_cmd.perm.lower();
	if (redis_cmd.perm != "yes" && redis_cmd.perm != "warn")
		redis_cmd.perm = "no";

	redis_cmds_[cmd] = redis_cmd;
}

void redis_commands::show_commands(void)
{
#ifdef ACL_UNIX
	printf("\033[1;34;40m%-20s\033[0m"
		"\033[1;34;40m%-20s\033[0m"
		"\033[1;34;40m%-20s\033[0m\r\n",
#else
	printf("%-20s%-20s%-20s\r\n",
#endif
		"Command", "Broadcast", "Permission");

	for (std::map<acl::string, REDIS_CMD>::const_iterator cit =
		redis_cmds_.begin(); cit != redis_cmds_.end(); ++cit)
	{
#ifdef ACL_UNIX
		printf("\033[1;32;40m%-20s\033[0m"
			"\033[1;36;40m%-20s\033[0m"
			"\033[1;36;40m%-20s\033[0m\r\n",
#else
		printf("%-20s%-20s%-20s\r\n",
#endif
			cit->second.cmd.c_str(),
			cit->second.broadcast.c_str(),
			cit->second.perm.c_str());
	}
}

void redis_commands::set_addr(const char* in, acl::string& out)
{
	if (in == NULL || *in == 0)
		return;

	acl::string buf(in);
	std::vector<acl::string>& tokens = buf.split2(": \t");
	if (tokens.size() >= 2)
		out.format("%s:%s", tokens[0].c_str(), tokens[1].c_str());
}

void redis_commands::getline(acl::string& buf, const char* prompt /* = NULL */)
{
	if (prompt == NULL || *prompt == 0)
		prompt = "redis_builder> ";

#ifdef HAS_READLINE
	char* ptr = readline(prompt);
	if (ptr == NULL)
		exit (0);
	buf = ptr;
	buf.trim_right_line();
	if (!buf.empty() && !buf.equal("y", false) && !buf.equal("n", false))
		add_history(buf.c_str());
#else
	printf("%s", prompt);
	fflush(stdout);
	acl::stdin_stream in;
	if (in.gets(buf) == false)
		exit (0);
#endif
}

void redis_commands::create_cluster(void)
{
	while (addr_.empty())
	{
		const char* prompt = "please enter one redis addr: ";
		acl::string buf;
		getline(buf, prompt);
		if (buf.empty())
			continue;
		set_addr(buf, addr_);
	}

	conns_ = new acl::redis_client_cluster;
	conns_->set(addr_, conn_timeout_, rw_timeout_);
	conns_->set_all_slot(addr_, 0);

	if (!passwd_.empty())
		conns_->set_password("default", passwd_);
}

void redis_commands::help(void)
{
#ifdef ACL_UNIX
	printf("> \033[1;33;40mkeys\033[0m"
		" \033[1;36;40mpattern limit\033[0m\r\n");
	printf("> \033[1;33;40mscan\033[0m"
		" \033[1;36;40mpattern display_count ip:port\033[0m\r\n");
	printf("> \033[1;33;40mget\033[0m"
		" \033[1;36;40m[:limit] parameter ...\033[0m\r\n");
	printf("> \033[1;33;40mgetn\033[0m"
		" \033[1;36;40mparameter limit\033[0m\r\n");
	printf("> \033[1;33;40mremove\033[0m"
		" \033[1;36;40mpattern\033[0m\r\n");
	printf("> \033[1;33;40mtype\033[0m"
		" \033[1;36;40mparameter ...\033[0m\r\n");
	printf("> \033[1;33;40mttl\033[0m"
		" \033[1;36;40mparameter ...\033[0m\r\n");
	printf("> \033[1;33;40mserver\033[0m"
		" \033[1;36;40mredis_addr\033[0m\r\n");
	printf("> \033[1;33;40mdbsize\033[0m\r\n");
	printf("> \033[1;33;40mnodes\033[0m\r\n");
	printf("> \033[1;33;40mconfig set|get parameter\033[0m\r\n");
	printf("> \033[1;33;40mconfig rewrite\033[0m\r\n");
	printf("> \033[1;33;40mconfig resetstat\033[0m\r\n");
#else
	printf("> keys pattern limit\r\n");
	printf("> scan pattern display_count ip:port\r\n");
	printf("> get [:limit] parameter ...\r\n");
	printf("> getn parameter limit\r\n");
	printf("> remove pattern\r\n");
	printf("> type parameter ...\r\n");
	printf("> ttl parameter ...\r\n");
	printf("> server redis_addr\r\n");
	printf("> dbsize\r\n");
	printf("> nodes\r\n");
	printf("> config set|get parameter\r\n");
	printf("> config rewrite\r\n");
	printf("> config resetstat\r\n");
#endif
}

bool redis_commands::check(const char* command)
{
	if (all_cmds_perm_ == "no")
	{
		printf("All commands disable!\r\n");
		return false;
	}

	std::map<acl::string, REDIS_CMD>::const_iterator cit =
		redis_cmds_.find(command);

	acl::string info;
	acl::string perm;

	if (cit == redis_cmds_.end())
	{
		info = "BROADCAST";
		perm = "yes";
	}
	else
	{
		info = cit->second.broadcast;
		if (!info.equal("master", false) && !info.equal("slave", false))
			info = "SEND";
		perm = cit->second.perm;
	}

	if (all_cmds_perm_ == "warn" || perm == "warn")
	{
		acl::string buf;
		acl::string prompt;
		prompt.format("Do you want to %s DANGEROUS \"%s\""
			" command to all redis nodes ? [y/n]: ",
			info.c_str(), command);
		getline(buf, prompt);
		buf.lower();
		if (buf != "y" && buf != "yes")
		{
			printf("You discard \"%s\" command!\r\n", command);
			return false;
		}
	}

	return true;
}

void redis_commands::run(void)
{
	acl::string buf;

	while (true)
	{
		getline(buf);
		if (buf.equal("quit", false) || buf.equal("exit", false)
			|| buf.equal("q", false))
		{
			printf("Bye!\r\n");
			break;
		}

		if (buf.empty() || buf.equal("help", false)
			|| buf.equal("h", false))
		{
			help();
			continue;
		}

		std::vector<acl::string>& tokens = buf.split2(" \t", true);
		acl::string& cmd = tokens[0];
		cmd.upper();

		if (check(cmd) == false)
			continue;

		if (cmd == "DATE")
			show_date();
		else if (cmd == "SERVER")
			set_server(tokens);
		else if (cmd == "NODES")
			show_nodes();
		else if (cmd == "KEYS")
			get_keys(tokens);
		else if (cmd == "SCAN")
			scan_keys(tokens);
		else if (cmd == "GET")
			get(tokens);
		else if (cmd == "GETN")
			getn(tokens);
		else if (cmd == "REMOVE" || cmd == "RM")
			pattern_remove(tokens);
		else if (cmd == "TYPE")
			check_type(tokens);
		else if (cmd == "TTL")
			check_ttl(tokens);
		else if (cmd == "DBSIZE")
			get_dbsize(tokens);
		else if (cmd == "CONFIG")
			config(tokens);
#ifdef HAS_READLINE
		else if (cmd == "CLEAR" || cmd == "CL")
		{
			rl_clear_screen(0, 0);
			printf("\r\n");
		}
#endif
		else
			request(tokens);
	}
}

void redis_commands::set_server(const std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2)
	{
		printf("> usage: server ip:port\r\n");
		return;
	}

	acl::string buf(tokens[1]);
	if (tokens.size() >= 3)
		buf << " " << tokens[2];

	acl::string addr;
	set_addr(buf, addr);
	if (addr_ == addr)
	{
		printf("no change, redis server addr: %s\r\n", addr_.c_str());
		return;
	}

	printf("set redis server addr from %s to %s\r\n",
		addr_.c_str(), addr.c_str());
	addr_ = addr;

	delete conns_;

	create_cluster();
}

void redis_commands::show_nodes(void)
{
	acl::redis_client client(addr_, conn_timeout_, rw_timeout_);
	client.set_password(passwd_);
	acl::redis redis(&client);
	redis_status status(addr_, conn_timeout_, rw_timeout_, passwd_);
	status.show_nodes(redis);
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

	const char* pattern = tokens[1].c_str();
	int  max;
	if (tokens.size() >= 3)
	{
		max = atoi(tokens[2].c_str());
		if (max < 0)
			max = 10;
	}
	else
		max = 10;

	acl::redis redis(conns_);
	std::vector<acl::redis_node*> nodes;
	redis_util::get_nodes(redis, prefer_master_, nodes);

	int  n = 0;
	if (!nodes.empty())
	{
		for (std::vector<acl::redis_node*>::const_iterator
			cit = nodes.begin(); cit != nodes.end(); ++cit)
		{
			n += get_keys((*cit)->get_addr(), pattern, max);
		}
	}
	else
		n += get_keys(addr_, pattern, max);

	printf("-----keys %s: total count: %d----\r\n", tokens[1].c_str(), n);
}

int redis_commands::get_keys(const char* addr, const char* pattern, int max)
{
	if (addr == NULL || *addr == 0)
	{
		printf("addr NULL\r\nEnter any key to continue ...\r\n");
		getchar();
		return 0;
	}

	acl::redis_client conn(addr, conn_timeout_, rw_timeout_);
	if (!passwd_.empty())
		conn.set_password(passwd_);

	std::vector<acl::string> res;
	acl::redis_key redis(&conn);
	if (redis.keys_pattern(pattern, &res) <= 0)
		return 0;

	int n = 0;
	for (std::vector<acl::string>::const_iterator cit = res.begin();
		cit != res.end(); ++cit)
	{
		printf("%s\r\n", (*cit).c_str());
		n++;
		if (n >= max)
			break;
	}

	printf("--- Addr: %s, Total: %d, Limit: %d, Show: %d ---\r\n",
		addr, (int) res.size(), max, n);

	return (int) res.size();
}

void redis_commands::scan_keys(const std::vector<acl::string>& tokens)
{
	const char* pattern = NULL;
	size_t display_count = 10;
	acl::string addr;

	size_t size = tokens.size();
	if (size >= 2)
		pattern = tokens[1].c_str();
	else
		pattern = "*";

	if (size >= 3)
		display_count = (size_t) atoi(tokens[2].c_str());
	if (size >= 4)
		addr = tokens[3];

	if (!addr.empty())
	{
		scan(addr, pattern, display_count);
		return;
	}

	acl::redis redis(conns_);
	std::vector<acl::redis_node*> nodes;
	redis_util::get_nodes(redis, prefer_master_, nodes);

	int total = 0;
	for (std::vector<acl::redis_node*>::const_iterator cit
		= nodes.begin(); cit != nodes.end();)
	{
		int n = scan((*cit)->get_addr(), pattern, display_count);
		if (n > 0)
			total += n;

		printf("There are %d keys in %s.", n, (*cit)->get_addr());

		if (++cit == nodes.end())
		{
			printf("\r\n");
			break;
		}

		acl::string prompt;
		prompt.format(" Do you want to scan next %s? [y/n] ",
			(*cit)->get_addr());

		acl::string buf;
		getline(buf, prompt);
		if (!buf.empty() && !buf.equal("y", false))
			break;
	}

	printf("-----scan keys %s: total count: %d----\r\n",
		pattern ? pattern : "*", total);
}

int redis_commands::scan(const char* addr, const char* pattern,
	size_t display_count)
{
	acl::redis_client conn(addr, conn_timeout_, rw_timeout_);
	if (!passwd_.empty())
		conn.set_password(passwd_);
	acl::redis redis(&conn);

	size_t count = 10000;
	std::vector<acl::string> res;
	res.reserve(count);

	int cursor = 0, n = 0, i = 0;
	size_t n1 = 0;

	while (true)
	{
		cursor = redis.scan(cursor, res, pattern, &count);
		if (cursor < 0)
		{
			printf("scan error: %s\r\n", redis.result_error());
			break;
		}

		i++;
		n += res.size();

		if (display_count > 0 && n1++ < display_count)
		{
			size_t n2 = 0;
			for (std::vector<acl::string>::const_iterator cit
				= res.begin(); cit != res.end(); ++cit)
			{
				if (display_count > 0 && n2 >= display_count)
					break;
				get((*cit).c_str(), display_count);
				n2++;
			}
		}

		if (cursor == 0)
			break;
		redis.clear();
		res.clear();

		if (i > 0 && i % 10 == 0)
			acl_doze(100);
	}

	return n;
}

void redis_commands::getn(const std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2)
	{
		printf("> usage: getn key limit\r\n");
		return;
	}

	const char* key = tokens[1].c_str();

	int count;
	if (tokens.size() >= 3)
	{
		count = atoi(tokens[2].c_str());
		if (count < 0)
			count = 10;
	}
	else
		count = 10;

	get(key, count);
}

void redis_commands::get(const std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2) // xxx
		return;

	std::vector<acl::string>::const_iterator cit = tokens.begin();
	++cit;

	int  max;
	const char* ptr = (*cit).c_str();
	if (*ptr == ':' && *(ptr + 1) != 0)
	{
		ptr++;
		max = atoi(ptr);
		if (max < 0)
			max = 10;
		++cit;
	}
	else
		max = 10;

	for (; cit != tokens.end(); ++cit)
	{
		const char* key = (*cit).c_str();
		get(key, max);
	}
}

void redis_commands::get(const char* key, int max)
{
	acl::redis cmd(conns_);
	acl::redis_key_t type = cmd.type(key);

	switch (type)
	{
		case acl::REDIS_KEY_NONE:
			break;
		case acl::REDIS_KEY_STRING:
			string_get(key);
			break;
		case acl::REDIS_KEY_HASH:
			hash_get(key, max);
			break;
		case acl::REDIS_KEY_LIST:
			list_get(key, max);
			break;
		case acl::REDIS_KEY_SET:
			set_get(key, max);
			break;
		case acl::REDIS_KEY_ZSET:
			zset_get(key, max);
			break;
		default:
			printf("%s: unknown type: %d\r\n", key, (int) type);
			break;
	}
}

void redis_commands::hash_get(const std::vector<acl::string>& tokens)
{
	if (tokens.empty())  // xxx
		return;

	std::vector<acl::string>::const_iterator cit = tokens.begin();
	for (++cit; cit != tokens.end(); ++cit)
	{
		hash_get((*cit).c_str(), 0);
		printf("-----------------------------------------------\r\n");
	}
}

void redis_commands::hash_get(const char* key, size_t max)
{
	std::map<acl::string, acl::string> res;
	acl::redis cmd(conns_);

	if (cmd.hgetall(key, res) == false)
	{
		printf("hgetall error: %s, key: [%s]\r\n",
			cmd.result_error(), key);
		return;
	}

	size_t n = 0, count = res.size();
	printf("HASH KEY: %s, COUNT: %d, MAX: %d\r\n",
		key, (int) count, (int) max);

	for (std::map<acl::string, acl::string>::const_iterator cit2
		= res.begin(); cit2 != res.end(); ++cit2)
	{
		printf("[%s]: [%s]\r\n", cit2->first.c_str(),
			cit2->second.c_str());
		n++;
		if (max > 0 && n >= max)
			break;
	}
	printf("HASH KEY: %s, COUNT: %d, MAX: %d, SHOW: %d\r\n",
		key, (int) count, (int) max, (int) n);
}

void redis_commands::string_get(const std::vector<acl::string>& tokens)
{
	if (tokens.empty())  // xxx
		return;

	std::vector<acl::string>::const_iterator cit = tokens.begin();
	for (++cit; cit != tokens.end(); ++cit)
	{
		string_get((*cit).c_str());
		printf("-----------------------------------------------\r\n");
	}
}

void redis_commands::string_get(const char* key)
{
	acl::string buf;
	acl::redis cmd(conns_);

	if (cmd.get(key, buf) == false)
	{
		printf("get error: %s, key: [%s]\r\n", cmd.result_error(), key);
		return;
	}

	printf("STRING KEY: [%s], VALUE: [%s]\r\n", key, buf.c_str());
}

void redis_commands::list_get(const std::vector<acl::string>& tokens)
{
	if (tokens.empty())  // xxx
		return;

	std::vector<acl::string>::const_iterator cit = tokens.begin();
	for (++cit; cit != tokens.end(); ++cit)
	{
		list_get((*cit).c_str(), 0);
		printf("-----------------------------------------------\r\n");
	}
}

void redis_commands::list_get(const char* key, size_t max)
{
	acl::string buf;
	acl::redis cmd(conns_);

	int len = cmd.llen(key), count = len;
	if (len < 0)
	{
		printf("llen error: %s, key: [%s]\r\n",
			cmd.result_error(), key);
		return;
	}
	if (len > LIMIT)
	{
		acl::string prompt;
		prompt.format("Do you show all %d elements for key %s ? [y/n] ",
			len, key);

		getline(buf, prompt);
		if (!buf.equal("y", false))
			return;
	}

	if (max > 0 && (size_t) len > max)
		len = (int) max;

	printf("LIST KEY: [%s], COUNT: %d, MAX: %d, SHOW: %d\r\n",
		key, count, (int) max, len);

	for (int i = 0; i < len; i++)
	{
		buf.clear();
		cmd.clear(false);
		if (cmd.lindex(key, i, buf) == false)
		{
			printf("lindex error: %s, key: %s, idx: %d\r\n",
				cmd.result_error(), key, i);
			return;
		}
		printf("[%s]\r\n", buf.c_str());
	}

	printf("LIST KEY: [%s], COUNT: %d, MAX: %d, SHOW: %d\r\n",
		key, count, (int) max, len);
}

void redis_commands::set_get(const std::vector<acl::string>& tokens)
{
	if (tokens.empty())  // xxx
		return;

	std::vector<acl::string>::const_iterator cit = tokens.begin();
	for (++cit; cit != tokens.end(); ++cit)
	{
		set_get((*cit).c_str(), 0);
		printf("-----------------------------------------------\r\n");
	}
}

void redis_commands::set_get(const char* key, size_t max)
{
	acl::string buf;
	acl::redis cmd(conns_);
	int len = cmd.scard(key), count = len;
	if (len < 0)
	{
		printf("scard error: %s, key: %s\r\n", cmd.result_error(), key);
		return;
	}
	if (len > LIMIT)
	{
		acl::string prompt;
		prompt.format("Do you show all %d elements for key %s ? [y/n] ",
			len, key);

		getline(buf, prompt);
		if (!buf.equal("y", false))
			return;
	}

	if (max > 0 && max > (size_t) len)
		len = (int) max;

	printf("SET KEY: [%s], COUNT: %d\r\n", key, len);

	for (int i = 0; i < len; i++)
	{
		buf.clear();
		cmd.clear(false);
		if (cmd.spop(key, buf) == false)
		{
			printf("spop error: %s, key: [%s], idx: %d\r\n",
				cmd.result_error(), key, i);
			return;
		}
		printf("[%s]\r\n", buf.c_str());
	}

	printf("SET KEY: [%s], COUNT: %d, MAX: %d, SHOW: %d\r\n",
		key, count, (int) max, len);
}

void redis_commands::zset_get(const std::vector<acl::string>& tokens)
{
	if (tokens.empty())  // xxx
		return;

	std::vector<acl::string>::const_iterator cit = tokens.begin();
	for (++cit; cit != tokens.end(); ++cit)
	{
		zset_get((*cit).c_str(), 0);
		printf("-----------------------------------------------\r\n");
	}
}

void redis_commands::zset_get(const char* key, size_t max)
{
	acl::string buf;
	acl::redis cmd(conns_);
	int len = cmd.zcard(key), count = len;
	if (len < 0)
	{
		printf("zcard error: %s, key: [%s]\r\n", cmd.result_error(), key);
		return;
	}
	if (len > LIMIT)
	{
		acl::string prompt;
		prompt.format("Do you show all %d elements for key %s ? [y/n] ",
			len, key);

		getline(buf, prompt);
		if (!buf.equal("y", false))
			return;
	}

	if (max > 0 && max > (size_t) len)
		len = (int) max;

	std::vector<acl::string> res;
	printf("ZSET KEY: [%s], COUNT: %d, MAX: %d, SHOW: %d\r\n",
		key, count, (int) max, len);

	for (int i = 0; i < len; i++)
	{
		buf.clear();
		cmd.clear(false);
		res.clear();
		int ret = cmd.zrange(key, i, i + 1, &res);
		if (ret < 0)
		{
			printf("zrange error: %s, key: [%s], idx: %d\r\n",
				cmd.result_error(), key, i);
			return;
		}

		if (res.empty())
			continue;

		for (std::vector<acl::string>::const_iterator cit
			= res.begin(); cit != res.end(); ++cit)
		{
			printf("[%s]\r\n", (*cit).c_str());
		}
	}

	printf("ZSET KEY: [%s], COUNT: %d, MAX: %d, SHOW: %d\r\n",
		key, count, (int) max, len);
}

void redis_commands::pattern_remove(const std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2)
	{
		printf("> usage: pattern_remove pattern\r\n");
		return;
	}

	const char* pattern = tokens[1].c_str();

	acl::redis redis(conns_);
	const std::map<acl::string, acl::redis_node*>* masters =
		redis_util::get_masters(redis);

	int deleted = 0;

	if (masters != NULL)
	{
		for (std::map<acl::string, acl::redis_node*>::const_iterator
			cit = masters->begin(); cit != masters->end(); ++cit)
		{
			const char* addr = cit->second->get_addr();
			if (addr == NULL || *addr == 0)
			{
				printf("addr NULL, skip it\r\n");
				continue;
			}

			deleted += pattern_remove(addr, pattern);
		}
	}
	else
		deleted += pattern_remove(addr_, pattern);

	printf("removed pattern: [%s], total: %d\r\n", pattern, deleted);
}

int redis_commands::pattern_remove(const char* addr, const char* pattern)
{
	acl::string buf;
	std::vector<acl::string> res;

	//acl::redis_client conn(addr, conn_timeout_, rw_timeout_);
	//if (!passwd_.empty())
	//	conn.set_password(passwd_);

	acl::redis redis(conns_);
	redis.keys_pattern(pattern, &res);

	printf("addr: %s, pattern: [%s], total: %d\r\n",
		addr, pattern, (int) res.size());

	if (res.empty())
		return 0;

	acl::string prompt;
	prompt.format("Do you want to delete them all in %s ? [y/n]: ", addr);

	getline(buf, prompt);
	if (buf.equal("y", false))
	{
		int ret = remove(res);
		if (ret > 0)
			return ret;
	}

	return 0;
}

int redis_commands::remove(const std::vector<acl::string>& keys)
{
	acl::redis cmd(conns_);

	int  deleted = 0, error = 0, notfound = 0;

	for (std::vector<acl::string>::const_iterator cit = keys.begin();
		cit != keys.end(); ++cit)
	{
		cmd.clear(false);
		int ret = cmd.del_one((*cit).c_str());
		if (ret < 0)
		{
			printf("del_one error: %s, key: [%s]\r\n",
				cmd.result_error(), (*cit).c_str());
			error++;
		}
		else if (ret == 0)
		{
			printf("not exist, key: [%s]\r\n", (*cit).c_str());
			notfound++;
		}
		else
		{
			printf("Delete ok, key: [%s]\r\n", (*cit).c_str());
			deleted++;
		}
	}

	printf("Remove over, deleted: %d, error: %d, not found: %d\r\n",
		deleted, error, notfound);
	return deleted;
}

void redis_commands::check_type(const std::vector<acl::string>& tokens)
{
	acl::redis cmd(conns_);
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
			printf("[%s]: NONE\r\n", key);
			break;
		case acl::REDIS_KEY_STRING:
			printf("[%s]: STRING\r\n", key);
			break;
		case acl::REDIS_KEY_HASH:
			printf("[%s]: HASH\r\n", key);
			break;
		case acl::REDIS_KEY_LIST:
			printf("[%s]: LIST\r\n", key);
			break;
		case acl::REDIS_KEY_SET:
			printf("[%s]: SET\r\n", key);
			break;
		case acl::REDIS_KEY_ZSET:
			printf("[%s]: ZSET\r\n", key);
			break;
		default:
			printf("[%s]: UNKNOWN\r\n", key);
			break;
		}
	}
}

void redis_commands::check_ttl(const std::vector<acl::string>& tokens)
{
	acl::redis cmd(conns_);
	std::vector<acl::string>::const_iterator cit = tokens.begin();
	++cit;
	for (; cit != tokens.end(); ++cit)
	{
		cmd.clear(false);
		const char* key = (*cit).c_str();
		int ttl = cmd.ttl(key);
		printf("[%s]: %d seconds\r\n", key, ttl);
	}
}

void redis_commands::get_dbsize(const std::vector<acl::string>&)
{
	acl::redis redis(conns_);
	std::vector<acl::redis_node*> nodes;
	redis_util::get_nodes(redis, prefer_master_, nodes);

	int total = 0;

	if (!nodes.empty())
	{
		for (std::vector<acl::redis_node*>::const_iterator
			cit = nodes.begin(); cit != nodes.end(); ++cit)
		{
			const char* addr = (*cit)->get_addr();
			if (addr == NULL || *addr == 0)
			{
				printf("addr NULL\r\n");
				continue;
			}

			acl::redis_client conn(addr, conn_timeout_, rw_timeout_);
			if (!passwd_.empty())
				conn.set_password(passwd_);
			acl::redis cmd(&conn);
			int n = cmd.dbsize();
			printf("----- ADDR: %s, DBSIZE: %d -----\r\n", addr, n);
			if (n > 0)
				total += n;
		}
	}
	else
	{
		acl::redis_client conn(addr_, conn_timeout_, rw_timeout_);
		if (!passwd_.empty())
			conn.set_password(passwd_);
		acl::redis cmd(&conn);
		int n = cmd.dbsize();
		printf("----- ADDR: %s, DBSIZE: %d -----\r\n", addr_.c_str(), n);
		if (n > 0)
			total += n;
	}

	printf("---- Total DBSIZE: %d -----\r\n", total);
}

void redis_commands::request(const std::vector<acl::string>& tokens)
{
	acl::string cmd(tokens[0]);
	cmd.upper();
	std::map<acl::string, REDIS_CMD>::const_iterator cit =
		redis_cmds_.find(cmd);
	if (cit == redis_cmds_.end())
	{
		request_one(tokens);
		return;
	}

	if (cit->second.broadcast.equal("all", false))
		request_all(tokens);
	else if (cit->second.broadcast.equal("master", false))
		request_masters(tokens);
	else if (cit->second.broadcast.equal("slave", false))
		request_slaves(tokens);
	else
		request_one(tokens);
}

void redis_commands::request_one(const std::vector<acl::string>& tokens)
{
	acl::redis redis(conns_);
	const acl::redis_result* result = redis.request(tokens);
	if (result == NULL)
	{
		printf("request error: %s, addr: %s\r\n",
			redis.result_error(), redis.get_client_addr());
		show_request(tokens);
		printf("\r\n");
		return;
	}

	show_result(*result, redis.get_client_addr());
}

void redis_commands::request_all(const std::vector<acl::string>& tokens)
{
	acl::redis_client client(addr_, conn_timeout_, rw_timeout_);
	client.set_password(passwd_);
	acl::redis redis(&client);
	std::vector<const acl::redis_node*> nodes;

	redis_util::get_all_nodes(redis, nodes);
	if (nodes.empty())
	{
		logger_error("no node of the cluster: %s", addr_.c_str());
		return;
	}

	for (std::vector<const acl::redis_node*>::const_iterator cit
		= nodes.begin(); cit != nodes.end(); ++cit)
	{
		const char* addr = (*cit)->get_addr();
		if (addr == NULL || *addr == 0)
		{
			logger_error("addr NULL");
			continue;
		}

		request_one(addr, tokens);
	}
}

void redis_commands::request_masters(const std::vector<acl::string>& tokens)
{
	acl::redis redis(conns_);
	const std::map<acl::string, acl::redis_node*>* masters =
		redis_util::get_masters(redis);
	if (masters == NULL)
	{
		logger_error("no masters node of the cluster");
		return;
	}

	for (std::map<acl::string, acl::redis_node*>::const_iterator cit
		= masters->begin(); cit != masters->end(); ++cit)
	{
		const char* addr = cit->second->get_addr();
		if (addr == NULL || *addr == 0)
		{
			logger_error("addr NULL");
			continue;
		}

		request_one(addr, tokens);
	}
}

void redis_commands::request_slaves(const std::vector<acl::string>& tokens)
{
	acl::redis redis(conns_);
	std::vector<const acl::redis_node*> slaves;
	redis_util::get_slaves(redis, slaves);
	if (slaves.empty())
	{
		logger_error("no slaves node of the cluster");
		return;
	}

	for (std::vector<const acl::redis_node*>::const_iterator cit
		= slaves.begin(); cit != slaves.end(); ++cit)
	{
		const char* addr = (*cit)->get_addr();
		if (addr == NULL || *addr == 0)
		{
			logger_error("addr NULL");
			continue;
		}

		request_one(addr, tokens);
	}
}

bool redis_commands::show_result(const acl::redis_result& result,
	const char* addr)
{
	acl::string buf;
	size_t size;
	const acl::redis_result** children;
	acl::redis_result_t type = result.get_type();

	switch (type)
	{
	case acl::REDIS_RESULT_NIL:
		if (addr && *addr)
			printf("%s-->", addr);
		printf("[nil]\r\n");
		break;
	case acl::REDIS_RESULT_ERROR:
		if (addr && *addr)
			printf("%s-->", addr);
		printf("-%s\r\n", result.get_error());
		return false;
	case acl::REDIS_RESULT_STATUS:
		if (addr && *addr)
			printf("%s-->", addr);
		printf("+%s\r\n", result.get_status());
		break;
	case acl::REDIS_RESULT_INTEGER:
		if (addr && *addr)
			printf("%s-->", addr);
		printf(":%lld\r\n", result.get_integer64());
		break;
	case acl::REDIS_RESULT_STRING:
		buf.clear();
		result.argv_to_string(buf);
		if (!buf.empty())
			printf("$%d\r\n%s\r\n", (int) buf.size(), buf.c_str());
		break;
	case acl::REDIS_RESULT_ARRAY:
		children = result.get_children(&size);
		if (size > 0)
			printf("*%d\r\n", (int) size);
		for (size_t i = 0; i < size; i++)
		{
			const acl::redis_result* rr = children[i];
			acl_assert(rr != NULL);
			show_result(*rr, addr);
		}
		break;
	case acl::REDIS_RESULT_UNKOWN:
	default:
		if (addr && *addr)
			printf("%s-->", addr);
		printf("unknown type: %d\r\n", (int) type);
		return false;
	}

	return true;
}

void redis_commands::show_request(const std::vector<acl::string>& tokens)
{
	for (std::vector<acl::string>::const_iterator cit =
			tokens.begin(); cit != tokens.end(); ++cit)
	{
		if (cit == tokens.begin())
			printf("%s", (*cit).c_str());
		else
			printf(" \"%s\"", (*cit).c_str());
	}
}

void redis_commands::config_usage(void)
{
	logger_error("> usage: config get parameter");
	logger_error("> usage: config set parameter");
	logger_error("> usage: config rewrite");
	logger_error("> usage: config resetstat");
}

void redis_commands::config(const std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2)
	{
		config_usage();
		return;
	}

	acl::redis_client client(addr_, conn_timeout_, rw_timeout_);
	client.set_password(passwd_);
	acl::redis redis(&client);
	std::vector<const acl::redis_node*> nodes;

	redis_util::get_all_nodes(redis, nodes);
	if (nodes.empty())
	{
		logger_error("no node of the cluster: %s", addr_.c_str());
		return;
	}

	for (std::vector<const acl::redis_node*>::const_iterator cit
		= nodes.begin(); cit != nodes.end(); ++cit)
	{
		const char* addr = (*cit)->get_addr();
		if (addr == NULL || *addr == 0)
		{
			logger_error("addr NULL");
			continue;
		}

		config(addr, tokens);
	}
}

void redis_commands::config(const char* addr,
	const std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2)
	{
		config_usage();
		return;
	}
	request_one(addr, tokens);
}

void redis_commands::request_one(const char* addr,
	const std::vector<acl::string>& tokens)
{
	acl::redis_client client(addr, conn_timeout_, rw_timeout_);
	client.set_password(passwd_);
	acl::redis redis(&client);
	const acl::redis_result* result = redis.request(tokens);
	if (result == NULL)
	{
		printf("request error: %s\r\n", redis.result_error());
		show_request(tokens);
		printf("\r\n");
	}
	else
	{
		if (show_result(*result, addr) == false)
		{
			printf("request error\r\n");
			show_request(tokens);
			printf("\r\n");
		}
	}
}
