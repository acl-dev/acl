class http_thread : public acl::thread
{
public:
	http_thread(const char* addr, int conn_timeout, int rw_timeout,
		int fibers_max, int stack_size, int oper_count);
	http_thread(acl::http_request_manager& cluster,
		int fibers_max, int stack_size, int oper_count);

	~http_thread(void);

private:
	acl::connect_manager& get_cluster(void)
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

	void fiber_dec(int cnt);

protected:
	void *run(void);

	static double stamp_sub(const struct timeval *from,
		const struct timeval *sub_by);

private:
	acl::string addr_;
	int conn_timeout_;
	int rw_timeout_;
	int fibers_max_;
	int fibers_cnt_;
	int stack_size_;
	int oper_count_;
	struct timeval begin_;
	acl::connect_manager* cluster_;
	acl::connect_manager* cluster_internal_;

	static void fiber_http(ACL_FIBER *fiber, void *ctx);
};
