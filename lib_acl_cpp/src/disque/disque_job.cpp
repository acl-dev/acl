#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/disque/disque_job.hpp"
#endif

namespace acl
{

disque_job::disque_job()
	: repl_(0)
	, ttl_(0)
	, ctime_(0)
	, delay_(0)
	, retry_(0)
{

}

disque_job::~disque_job()
{

}

bool disque_job::init(const redis_result& rr)
{
	size_t n;
	const redis_result** children = rr.get_children(&n);
	if (n == 0)
		return false;
	if (n % 2 != 0)
		return false;

	string name;
	for (size_t i = 0; i < n;)
	{
		const redis_result* r1 = children[i];
		i++;
		const redis_result* r2 = children[i];
		i++;
		if (r1->get_type() != REDIS_RESULT_STRING)
			continue;
		name.clear();
		r1->argv_to_string(name);
		if (name.empty())
			continue;

		redis_result_t type = r2->get_type();

#define EQ(x, y) !strcasecmp((x).c_str(), (y))
#define IS_NUMBER(x) (x) == REDIS_RESULT_INTEGER
#define IS_STRING(x) (x) == REDIS_RESULT_STRING
#define IS_ARRAY(x)  (x) == REDIS_RESULT_ARRAY

		if (EQ(name, "id") && IS_STRING(type))
			r2->argv_to_string(id_);
		else if (EQ(name, "queue") && IS_STRING(type))
			r2->argv_to_string(queue_);
		else if (EQ(name, "state") && IS_STRING(type))
			r2->argv_to_string(state_);
		else if (EQ(name, "repl") && IS_NUMBER(type))
			repl_ = r2->get_integer();
		else if (EQ(name, "ttl") && IS_NUMBER(type))
			ttl_ = r2->get_integer();
		else if (EQ(name, "ctime") && IS_NUMBER(type))
			ctime_ = r2->get_integer64();
		else if (EQ(name, "delay") && IS_NUMBER(type))
			delay_ = r2->get_integer();
		else if (EQ(name, "retry") && IS_NUMBER(type))
			retry_ = r2->get_integer();
		else if (EQ(name, "nodes-delivered") && IS_ARRAY(type))
			set_nodes_delivered(*r2);
		else if (EQ(name, "nodes-confirmed") && IS_ARRAY(type))
			set_nodes_confirmed(*r2);
		else if (EQ(name, "next-requeue-within") && IS_NUMBER(type))
			next_requeue_within_ = r2->get_integer();
		else if (EQ(name, "next-awake-within") && IS_NUMBER(type))
			next_awake_within_ = r2->get_integer();
		else if (EQ(name, "body") && IS_STRING(type))
			r2->argv_to_string(body_);
	}

	return true;
}

void disque_job::set_nodes_delivered(const redis_result& rr)
{
	set_nodes(rr, nodes_delivered_);
}

void disque_job::set_nodes_confirmed(const redis_result& rr)
{
	set_nodes(rr, nodes_confirmed_);
}

void disque_job::set_nodes(const redis_result& rr, std::vector<string>& out)
{
	size_t n;
	const redis_result** children = rr.get_children(&n);
	string id;
	for (size_t i = 0; i < n; i++)
	{
		if (children[0]->get_type() != REDIS_RESULT_STRING)
			continue;
		children[0]->argv_to_string(id);
		if (!id.empty())
		{
			out.push_back(id);
			id.clear();
		}
	}
}

void disque_job::set_id(const char* id)
{
	id_ = id;
}

void disque_job::set_queue(const char* name)
{
	queue_ = name;
}

void disque_job::set_body(const char* job, size_t len)
{
	body_.copy(job, len);
}

} // namespace acl
