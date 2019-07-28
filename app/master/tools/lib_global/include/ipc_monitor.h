class ipc_monitor : public acl::thread
{
public:
	ipc_monitor(acl::tcp_ipc& ipc, int ttl, bool& service_exit);
	~ipc_monitor(void) {}

private:
	// @override
	void* run(void);

	void check_idle(void);

private:
	acl::tcp_ipc& ipc_;
	int ttl_;
	bool& service_exit_;
};
