#pragma once

//////////////////////////////////////////////////////////////////////////

// 纯虚类，子类须实现该类中的纯虚接口
class ping_callback
{
public:
	ping_callback() {}
	virtual ~ping_callback() {}

	virtual void ping_report(size_t total, size_t curr, size_t nerr) = 0;
	virtual void ping_finish(const char* dbpath) = 0;
};

//////////////////////////////////////////////////////////////////////////

struct PING_PKT
{
	double rtt_;
	int ttl_;
	int bytes_;
	unsigned short seq_;
	char status_;
};

class ping;

class host_status
{
public:
	host_status(ping& p, const char* ip);
	~host_status();

	ping& get_ping() const
	{
		return ping_;
	}

	const std::vector<PING_PKT*>& get_pkt_list() const
	{
		return pkt_list_;
	}

	const char* get_ip() const
	{
		return ip_;
	}

	int get_sent() const
	{
		return sent_;
	}

	int get_received() const
	{
		return received_;
	}

	int get_lost() const
	{
		return lost_;
	}

	double get_loss() const
	{
		return loss_;
	}

	double get_minimum() const
	{
		return minimum_;
	}

	double get_maximum() const
	{
		return maximum_;
	}

	double get_average() const
	{
		return average_;
	}

	void add_status(const ICMP_PKT_STATUS* status);
	void set_statistics(const ICMP_STAT* status);
protected:
private:
	ping& ping_;
	char  ip_[32];
	int   sent_;
	int   received_;
	int   lost_;
	double loss_;
	double minimum_;
	double maximum_;
	double average_;

	std::vector<PING_PKT*> pkt_list_;
};

class ping : public acl::rpc_request
{
public:
	ping(const char* filepath, ping_callback* callback,
		int npkt, int delay, int timeout, int pkt_size);
protected:
	~ping();

	// 基类虚函数：子线程处理函数
	virtual void rpc_run();

	// 基类虚函数：主线程处理过程，收到子线程任务完成的消息
	virtual void rpc_onover();

	// 基类虚函数：主线程处理过程，收到子线程的通知消息
	virtual void rpc_wakeup(void* ctx);

protected:
private:
	acl::string filepath_;
	ping_callback* callback_;
	int   npkt_;
	int   delay_;
	int   timeout_;
	int   pkt_size_;
	std::vector<host_status*>* host_list_;
	size_t total_pkt_;
	size_t curr_pkt_;
	size_t error_pkt_;

	bool load_file();
	void ping_all();

private:
	static void ping_stat_response(ICMP_PKT_STATUS*, void*);
	static void ping_stat_timeout(ICMP_PKT_STATUS*, void*);
	static void ping_stat_unreach(ICMP_PKT_STATUS*, void*);
	static void ping_stat_finish(ICMP_HOST*, void*);
};
