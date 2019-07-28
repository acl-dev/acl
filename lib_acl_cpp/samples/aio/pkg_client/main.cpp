#include <iostream>
#include <assert.h>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

static int   __timeout = 10;

typedef enum
{
	STATUS_T_HDR,
	STATUS_T_DAT,
} status_t;

// 数据头
struct DAT_HDR
{
	int  len;		// 数据体长度
	char cmd[64];		// 命令字
};

class mythread : public acl::thread
{
public:
	mythread(const char* addr, int count, int length)
	: addr_(addr)
	, count_(count)
	, length_(length)
	{
		req_dat_ = (char*) malloc(length);
		res_dat_ = (char*) malloc(length);
		res_max_ = length;
		memset(req_dat_, 'X', length_);
	}

	~mythread(void)
	{
		free(req_dat_);
		free(res_dat_);
	}

protected:
	// @override
	void* run(void)
	{
		acl::socket_stream conn;

		// 连接服务器
		if (!conn.open(addr_.c_str(), __timeout, __timeout)) {
			printf("connect %s error\r\n", addr_.c_str());
			return NULL;
		}

		for (int i = 0; i < count_; i++) {
			if (!handle_pkg(conn, i)) {
				break;
			}
		}

		return NULL;
	}

private:
	bool handle_pkg(acl::socket_stream& conn, int i)
	{
		DAT_HDR req_hdr;
		req_hdr.len = htonl(length_);
		ACL_SAFE_STRNCPY(req_hdr.cmd, "SEND", sizeof(req_hdr.cmd));

		// 先写数据头
		if (conn.write(&req_hdr, sizeof(req_hdr)) == -1) {
			printf("write hdr to server failed\r\n");
			return false;
		}

		// 再写数据体
		if (conn.write(req_dat_, length_) == -1) {
			printf("write dat to server failed\r\n");
			return false;
		}

		//////////////////////////////////////////////////////////////

		// 先读响应头
		DAT_HDR res;
		if (conn.read(&res, sizeof(DAT_HDR)) == -1) {
			printf("read DAT_HDR error\r\n");
			return false;
		}

		// 将响应体数据长度转为主机字节序
		res.len = ntohl(res.len);
		if (res.len <= 0) {
			printf("invalid length: %d\r\n", res.len);
			return false;
		}

		// 检测是否需要重新分配读响应数据的内存
		if (res.len + 1 >= res_max_) {
			res_max_ = res.len + 1;
			res_dat_ = (char*) realloc(res_dat_, res_max_);
		}

		// 再读响应体
		if (conn.read(res_dat_, res.len) == -1) {
			printf("read data error, len: %d\r\n", res.len);
			return false;
		}

		res_dat_[res.len] = 0;
		if (i < 10) {
			printf("thread: %lu, cmd: %s, dat: %s, len: %d\r\n",
				(unsigned long) thread_id(), res.cmd,
				res_dat_, res.len);
		}

		return true;
	}

private:
	acl::string addr_;
	int   count_;
	int   length_;
	char* req_dat_;
	char* res_dat_;
	int   res_max_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"	-s ip:port[default: 127.0.0.1:1900]\r\n"
		"	-t timeout[default: 10]\r\n"
		"	-n count[default: 100]\r\n"
		"	-l length[default: 1024]\r\n"
		"	-c max_threads[default: 10]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, count = 100, max_threads = 10, length = 1024;
	acl::string addr("127.0.0.1:1900");

	while ((ch = getopt(argc, argv, "n:c:s:ht:l:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			count = atoi(optarg);
			break;
		case 'c':
			max_threads = atoi(optarg);
			break;
		case 's':
			addr = optarg;
			break;
		case 't':
			__timeout = atoi(optarg);
			break;
		case 'l':
			length = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	std::vector<mythread*> threads;
	for (int i = 0; i < max_threads; i++) {
		mythread* thread = new mythread(addr, count, length);
		thread->set_detachable(false);
		threads.push_back(thread);
		thread->start();
	}

	for (std::vector<mythread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {
		if (!(*it)->wait()) {
			printf("thread wait error: %s\r\n", acl::last_serror());
			break;
		}

		delete *it;
	}

	return 0;
}
