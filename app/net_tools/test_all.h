#pragma once

//////////////////////////////////////////////////////////////////////////

class test_callback
{
public:
	test_callback() {}
	virtual ~test_callback() {}

	virtual void test_report(const char* msg, unsigned nstep) = 0;
	virtual void test_store(const char* dbpath) = 0;
	virtual void test_finish() = 0;
};

//////////////////////////////////////////////////////////////////////////

class test_all;

class ping_result : public ping_callback
{
public:
	ping_result(test_all& test) : test_(test) {}
	~ping_result() {}
protected:
	virtual void ping_report(size_t total, size_t curr, size_t nerr);
	virtual void ping_finish(const char* dbpath);
private:
	test_all& test_;
};

class nslookup_result : public nslookup_callback
{
public:
	nslookup_result(test_all& test) : test_(test) {}
	~nslookup_result() {}
protected:
	virtual void nslookup_report(size_t total, size_t curr);
	virtual void nslookup_finish(const char* dbpath);
private:
	test_all& test_;
};

class smtp_result : public smtp_callback
{
public:
	smtp_result(test_all& test) : test_(test) {}
	~smtp_result() {}
protected:
	virtual void smtp_finish(const char* dbpath);
	virtual void smtp_report(const char* msg, size_t total,
		size_t curr, const SMTP_METER& meter);
private:
	test_all& test_;
};

class pop3_result : public pop3_callback
{
public:
	pop3_result(test_all& test) : test_(test) {}
	~pop3_result() {}
protected:
	virtual void pop3_finish(const char* dbpath);
	virtual void pop3_report(const char* msg, size_t total,
		size_t curr, const POP3_METER& meter);
private:
	test_all& test_;
};

//////////////////////////////////////////////////////////////////////////

class test_all
{
public:
	test_all(test_callback*);
	void start();
protected:
	~test_all();
public:
	void ping_report(size_t total, size_t curr, size_t nerr);
	void ping_finish(const char* dbpath);

	void nslookup_report(size_t total, size_t curr);
	void nslookup_finish(const char* dbpath);

	void smtp_finish(const char* dbpath);
	void smtp_report(const char* msg, size_t total,
		size_t curr, const SMTP_METER& meter);

	void pop3_finish(const char* dbpath);
	void pop3_report(const char* msg, size_t total,
		size_t curr, const POP3_METER& meter);
private:
	//friend class ping_result;
	//friend class nslookup_result;
	//friend class smtp_result;

	test_callback* callback_;
	void check_finish();
private:
	acl::string ip_file_;
	int ping_npkt_;
	int ping_delay_;
	int ping_timeout_;
	int ping_size_;
	bool ping_ok_;
	ping_result ping_result_;
private:
	acl::string domain_file_;
	acl::string dns_ip_;
	int dns_port_;
	int dns_timeout_;
	bool dns_ok_;
	nslookup_result ns_result_;
private:
	acl::string mail_user_;
	acl::string mail_pass_;
	int conn_timeout_;
	int rw_timeout_;
private:
	acl::string smtp_addr_;
	int smtp_port_;
	acl::string attach_;
	acl::string recipients_;
	bool smtp_ok_;
	smtp_result smtp_result_;
private:
	acl::string pop3_addr_;
	int pop3_port_;
	bool pop3_recv_all_;
	size_t pop3_recv_limit_;
	bool pop3_ok_;
	pop3_result pop3_result_;
	bool pop3_save_;
public:
	test_all& set_ip_file(const char* filename)
	{
		ip_file_ = filename;
		return *this;
	}
	test_all& set_ping_npkt(int ping_npkt)
	{
		ping_npkt_ = ping_npkt;
		return *this;
	}
	test_all& set_ping_delay(int ping_delay)
	{
		ping_delay_ = ping_delay;
		return *this;
	}
	test_all& set_ping_timeout(int ping_timeout)
	{
		ping_timeout_ = ping_timeout;
		return *this;
	}
	test_all& set_ping_size(int ping_size)
	{
		ping_size_ = ping_size;
		return *this;
	}
public:
	test_all& set_domain_file(const char* filename)
	{
		domain_file_ = filename;
		return *this;
	}
	test_all& set_dns_ip(const char* ip)
	{
		dns_ip_ = ip;
		return *this;
	}
	test_all& set_dns_port(int port)
	{
		dns_port_ = port;
		return *this;
	}
	test_all& set_dns_timeout(int timeout)
	{
		dns_timeout_ = timeout;
		return *this;
	}
public:
	test_all& set_attach(const char* attach)
	{
		attach_ = attach;
		return *this;
	}
	test_all& set_smtp_addr(const char* smtp_addr)
	{
		smtp_addr_ = smtp_addr;
		return *this;
	}
	test_all& set_smtp_port(int smtp_port)
	{
		smtp_port_ = smtp_port;
		return *this;
	}
	test_all& set_recipients(const char* recipients)
	{
		recipients_ = recipients;
		return *this;
	}
public:
	test_all& set_pop3_addr(const char* pop3_addr)
	{
		pop3_addr_ = pop3_addr;
		return *this;
	}
	test_all& set_pop3_port(int pop3_port)
	{
		pop3_port_ = pop3_port;
		return *this;
	}
	test_all& set_pop3_recv(int recv_limit)
	{
		if (recv_limit < 0)
		{
			pop3_recv_all_ = true;
			pop3_recv_limit_ = -1;
		}
		else if (recv_limit == 0)
		{
			pop3_recv_all_ = false;
			pop3_recv_limit_ = 0;
		}
		else
		{
			pop3_recv_all_ = false;
			pop3_recv_limit_ = recv_limit;
		}
		return *this;
	}
	test_all& set_pop3_save(bool on)
	{
		pop3_save_ = on;
		return *this;
	}
public:
	test_all& set_conn_timeout(int conn_timeout)
	{
		conn_timeout_ = conn_timeout;
		return *this;
	}
	test_all& set_rw_timeout(int rw_timeout)
	{
		rw_timeout_ = rw_timeout;
		return *this;
	}
	test_all& set_mail_user(const char* mail_user)
	{
		mail_user_ = mail_user;
		return *this;
	}
	test_all& set_mail_pass(const char* mail_pass)
	{
		mail_pass_ = mail_pass;
		return *this;
	}
};
