#include "stdafx.h"
#include "master_service.h"

////////////////////////////////////////////////////////////////////////////////
// 配置内容项

char *var_cfg_local_addr;
acl::master_str_tbl var_conf_str_tab[] = {
	{ "local_addr", "127.0.0.1:0", &var_cfg_local_addr },

	{ 0, 0, 0 }
};

int  var_cfg_use_threads;
acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "use_threads", 0, &var_cfg_use_threads },

	{ 0, 0, 0 }
};

int  var_cfg_int;
acl::master_int_tbl var_conf_int_tab[] = {
	{ "int", 120, &var_cfg_int, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

long long int  var_cfg_int64;
acl::master_int64_tbl var_conf_int64_tab[] = {
	{ "int64", 120, &var_cfg_int64, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

////////////////////////////////////////////////////////////////////////////////

static acl_pthread_key_t  stream_key;
static acl_pthread_once_t stream_once = ACL_PTHREAD_ONCE_INIT;

class echo_thread : public acl::thread
{
public:
	echo_thread(const char* peer, const char* data, size_t dlen)
	{
#if defined(WIN32) && _MSC_VER >= 1500
		_snprintf_s(peer_addr_, sizeof(peer_addr_), sizeof(peer_addr_), "%s", peer);
#else
		snprintf(peer_addr_, sizeof(peer_addr_), "%s", peer);
#endif
		buf_ = (char*) acl_mymemdup(data, dlen);
		len_ = (int) dlen;
	}

	~echo_thread()
	{
		acl_myfree(buf_);
	}

protected:
	virtual void* run()
	{
		// 回写需要先设置远程连接地址
		acl::socket_stream* conn = (acl::socket_stream*)
			acl_pthread_getspecific(stream_key);
		acl_assert(conn);
		conn->set_peer(peer_addr_);
		conn->write(buf_, len_);

		// 因为该对象是动态分配的，所以需要释放掉
		delete this;
		return NULL;
	}

private:
	char  peer_addr_[64];
	char* buf_;
	int   len_;
};

static void close_stream(void* arg)
{
	acl::socket_stream* conn = (acl::socket_stream*) arg;
	delete conn;
}

static void thread_init_once()
{
	acl_pthread_key_create(&stream_key, close_stream);
}

class mythread_pool : public acl::thread_pool
{
public:
	mythread_pool() {}
	~mythread_pool() {}

protected:
	virtual bool thread_on_init()
	{
		acl_pthread_once(&stream_once, thread_init_once);
		acl::socket_stream* conn = (acl::socket_stream*)
			acl_pthread_getspecific(stream_key);
		if (conn != NULL)
			return true;

		conn = new acl::socket_stream;
		if (conn->bind_udp(var_cfg_local_addr) == false)
		{
			logger_error("bind %s error %s", var_cfg_local_addr,
				acl::last_serror());
			delete conn;
			return false;
		}

		acl_pthread_setspecific(stream_key, conn);
		return true;
	}

	virtual void thread_on_exit()
	{
	}
};

////////////////////////////////////////////////////////////////////////////////

static acl::thread_pool *__threads = NULL;

master_service::master_service()
{
}

master_service::~master_service()
{
}

void master_service::on_read(acl::socket_stream* stream)
{
	int   n;
	char  buf[4096];

	// 从远程连接读一条记录
	if ((n = stream->read(buf, sizeof(buf), false)) == -1)
		return;

	if (0)
		logger("read from %s, %d bytes, local: %s",
			stream->get_peer(true), n, stream->get_local(true));

	// 如果采用单线程模式，则直接回写
	if (__threads == NULL)
	{
		stream->write(buf, n);
		return;
	}

	// 否则，采用多线程模式，将回写任务交给线程池处理

	echo_thread* thr = new echo_thread(stream->get_peer(true), buf, n);
	__threads->run(thr);
}

void master_service::proc_on_init()
{
	if (var_cfg_use_threads)
	{
		// 线程池模式下，需要创建线程池

		__threads = new mythread_pool;
		__threads->set_limit(100);
		__threads->start();
	}
	else
		__threads = NULL;
}

void master_service::proc_on_exit()
{
	if (__threads)
	{
		// 停止线程池并销毁

		__threads->stop();
		delete __threads;
	}
}
