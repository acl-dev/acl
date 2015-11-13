#include "stdafx.h"
#include "util.h"
#include <vector>
#include "session.h"

class test_thread : public acl::thread
{
public:
	test_thread(acl::redis_client_cluster& cluster,
			size_t max_threads, int count)
		: cluster_(cluster), max_threads_(max_threads), count_(count)
	{
		sess_ = new acl::redis_session(cluster_, max_threads_);
	}

	~test_thread()
	{
		delete sess_;
	}

protected:
	void* run()
	{
		acl::string name;
		char value[128];

		memset(value, 'X', sizeof(value));
		value[sizeof(value) - 1] = 0;

		printf(">>> count: %d\r\n", count_);

		struct timeval begin;
		gettimeofday(&begin, NULL);

		for (int i = 0; i < count_; i++)
		{
			name.format("tid_%lu_%d", thread_id(), i);
			if (sess_->set(name.c_str(), value) == false)
			{
				printf("set error, name: %s\r\n", name.c_str());
				break;
			}

			const char* ptr = sess_->get(name.c_str());
			if (ptr == NULL || *ptr == 0)
			{
				printf("get error, name: %s\r\n", name.c_str());
				break;
			}

			if (strcasecmp(ptr, value) != 0)
			{
				printf("invalid result\r\n");
				break;
			}

			if (sess_->del(name.c_str()) == false)
			{
				printf("del error, name: %s\r\n", name.c_str());
				break;
			}

			if (i < 10)
				printf("test name: %s ok\r\n", name.c_str());
			if (i % 1000 == 0)
			{
				char info[128];
				acl::safe_snprintf(info, sizeof(info),
					"benchmark: %d", i);
				acl::meter_time(__FILE__, __LINE__, info);
			}
		}

		struct timeval end;
		gettimeofday(&end, NULL);

		double inter = util::stamp_sub(&end, &begin);
		printf("total: %d, spent: %0.2f ms, speed: %0.2f\r\n",
			count_, inter, (count_ * 1000) /(inter > 0 ? inter : 1));

		return NULL;
	}

private:
	acl::redis_client_cluster& cluster_;
	acl::redis_session* sess_;
	size_t max_threads_;
	int count_;
};

bool test_redis_session(const char* addr, int n, int max_threads)
{
	int conn_timeout = 10, rw_timeout = 10;
	acl::redis_client_cluster cluster;
	cluster.set(addr, max_threads, conn_timeout, rw_timeout);

	std::vector<test_thread*> threads;
	for (int i = 0; i < max_threads; i++)
	{
		test_thread* thread = new test_thread(cluster, max_threads, n);
		threads.push_back(thread);
		thread->set_detachable(false);
		thread->start();
	}

	for (std::vector<test_thread*>::iterator it = threads.begin();
		it != threads.end(); ++it)
	{
		(*it)->wait();
		delete (*it);
	}

	return true;
}

void test_session_string(const char* addr)
{
	acl::session_string s1;
	s1.copy(addr, strlen(addr));
	s1.todo_ = acl::TODO_DEL;

	printf("s1: addr: %s, todo: %d\r\n", s1.c_str(), s1.todo_);

	acl::session_string s2 = s1;
	printf("structor copy --> s2: addr: %s, todo: %d\r\n", s2.c_str(), s2.todo_);

	s1.todo_ = acl::TODO_NUL;
	s2 = s1;
	printf("assign copy --> s2: addr: %s, todo: %d\r\n", s2.c_str(), s2.todo_);

	s2.todo_ = acl::TODO_SET;
	acl::session_string s3(s2);
	printf("structor copy --> s3: addr: %s, todo: %d\r\n", s3.c_str(), s3.todo_);
}

bool test_redis_session_attrs(const char* addr, int n)
{
	std::map<acl::string, acl::session_string> attrs;
	acl::session_string value, name;

	for (int i = 0; i < n; i++)
	{
		name.format("name_%d", i);
		value.format("value_%d", i);

		attrs[name] = value;
	}

	acl::redis_client_cluster cluster;
	cluster.set(addr, 1, 10, 10);

	acl::redis_session sess(cluster, 1);
	if (sess.set_attrs(attrs) == false)
	{
		printf("set_attrs error\r\n");
		return false;
	}

	printf("set_attrs ok\r\n");

	attrs.clear();
	if (sess.get_attrs(attrs) == false)
	{
		printf("get_attrs error\r\n");
		return false;
	}

	printf("get_attrs ok, n: %d\r\n", (int) attrs.size());

	std::map<acl::string, acl::session_string>::const_iterator cit;
	for (cit = attrs.begin(); cit != attrs.end(); ++cit)
		printf("%s=%s\r\n", cit->first.c_str(), cit->second.c_str());

	return true;
}
