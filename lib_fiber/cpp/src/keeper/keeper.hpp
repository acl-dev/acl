#pragma once
#include "fiber/fiber_tbox.hpp"

namespace acl {

class keeper_conn;

struct keeper_link {
	ACL_RING     me;
	keeper_conn* fb;
};

typedef enum {
	ASK_T_NULL,
	ASK_T_IDLE,
	ASK_T_STOP,
	ASK_T_CONN,
	ASK_T_CLOSE,
} ask_type_t;

typedef enum {
	KEEPER_T_READY,
	KEEPER_T_BUSY,
	KEEPER_T_IDLE,
} keeper_status_t;

class keeper_config
{
public:
	keeper_config(void)
	: conn_timeo(10)
	, rw_timeo(10)
	, conn_min(10)
	, conn_max(10)
	, conn_ttl(10)
	, pool_ttl(20) {}

	~keeper_config(void) {}

	int  conn_timeo;
	int  rw_timeo;
	int  conn_min;
	int  conn_max;
	int  conn_ttl;
	int  pool_ttl;
};

class ask_req
{
public:
	ask_req(ask_type_t type) : type_(type) {}

	ask_type_t get_type(void) const
	{
		return type_;
	}

	~ask_req(void) {}

private:
	ask_type_t type_;
};

class task_req
{
public:
	task_req(void)
	: hit_(false)
	, conn_cost_(1000)
	{
		gettimeofday(&stamp_, NULL);
	}

	~task_req(void) {}

	void set_addr(const char* addr)
	{
		addr_ = addr;
	}

	const char* get_addr(void) const
	{
		return addr_.c_str();
	}

	void set_hit(bool yes)
	{
		hit_ = yes;
	}

	bool is_hit(void) const
	{
		return hit_;
	}

	void set_stamp(void)
	{
		gettimeofday(&stamp_, NULL);
	}

	const struct timeval& get_stamp(void) const
	{
		return stamp_;
	}

	double get_cost(void) const
	{
		struct timeval curr;
		gettimeofday(&curr, NULL);
		return stamp_sub(curr, stamp_);
	}
	
	void set_conn_cost(double cost)
	{
		conn_cost_ = cost;
	}

	double get_conn_cost(void) const
	{
		return conn_cost_;
	}

public:
	socket_stream* pop(void)
	{
		return tbox_.pop();
	}

	void put(socket_stream* conn)
	{
		tbox_.push(conn);
	}

private:
	bool   hit_;
	string addr_;
	struct timeval stamp_;
	fiber_tbox<socket_stream> tbox_;
	double conn_cost_;
};

} // namespace acl
