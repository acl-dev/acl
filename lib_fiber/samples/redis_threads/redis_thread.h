class redis_thread : public acl::thread
{
public:
	redis_thread(const char* addr, const char* passwd, int conn_timeout,
		int rw_timeout, int fibers_max, int stack_size,
		int oper_count, const char* cmd);
	redis_thread(acl::redis_client_cluster& cluster, int fibers_max,
		int stack_size, int oper_count, const char* cmd);

	~redis_thread(void);

private:
	acl::redis_client_cluster& get_cluster(void)
	{
		return *cluster_;
	}

	int get_fibers_max(void) const
	{
		return fibers_max_;
	}

	int get_oper_count(void) const
	{
		return oper_count_;
	}

	struct timeval& get_begin(void)
	{
		return begin_;
	}

	const acl::string& get_cmd(void) const {
		return cmd_;
	}

	void fiber_dec(int cnt);

protected:
	void *run(void);

private:
	acl::string addr_;
	acl::string passwd_;
	int conn_timeout_;
	int rw_timeout_;
	int fibers_max_;
	int fibers_cnt_;
	int stack_size_;
	int oper_count_;
	acl::string cmd_;

	struct timeval begin_;
	acl::redis_client_cluster* cluster_;
	acl::redis_client_cluster* cluster_internal_;

	static void fiber_redis(ACL_FIBER *fiber, void *ctx);
};
