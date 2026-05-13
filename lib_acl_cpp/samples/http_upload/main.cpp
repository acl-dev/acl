// http_servlet.cpp : 定义控制台应用程序的入口点。
//
#include <assert.h>
#include <getopt.h>
#include "acl_cpp/lib_acl.hpp"

//////////////////////////////////////////////////////////////////////////

class http_request_test {
public:
	http_request_test(const char* server_addr, bool debug_mode) {
		server_addr_ = server_addr;
		debug_mode_  = debug_mode;
	}

	~http_request_test() {}

	bool run(const std::vector<std::string>& files,
		 const std::map<std::string, std::string>& params) {
		const char* boundary = "----------------xxxxxx113333333";
		acl::http_mime mime(boundary);

		// 添加参数数据
		for (std::map<std::string, std::string>::const_iterator cit =
			  params.begin(); cit != params.end(); ++cit) {
			mime.add_param(cit->first.c_str(), cit->second.c_str());
		}

		int i = 0;
		acl::string name;
		// 添加文件信息
		for (std::vector<std::string>::const_iterator cit =
			  files.begin(); cit != files.end(); ++cit) {
			name.format("file-%d", i++);
			mime.add_file(name.c_str(), cit->c_str());
		}


		// 创建  HTTP 请求客户端
		acl::http_request req(server_addr_);

		// 添加 HTTP 请求头字段
		acl::http_header& hdr = req.request_header();  // 请求头对象的引用
		hdr.set_url("/upload");

#if 0
		// 当发送的数据比较大时，可以采用此方法，将数据先临时放在
		// 本地文件中，然后再发送
		// 发送 HTTP MIME 请求数据
		const char* tmpfile = "./local.file";
		if (req.upload(mime, tmpfile, debug_mode_) == false) {
			printf("upload to %s error\r\n", server_addr_.c_str());
			return false;
		}
#else
		// 当发送数据量比较小时，可以采用此方法，将数据放在内存中发送
		acl::string buff;
		// 发送 HTTP MIME 请求数据
		if (req.upload(mime, debug_mode_ ? &buff : NULL) == false) {
			printf("upload to %s error\r\n", server_addr_.c_str());
			return false;
		}

		if (debug_mode_) {
			printf("buff:\r\n%s\r\n", buff.c_str());
		}
#endif

		acl::string body;
		if (req.get_body(body) == false) {
			printf("get http body error\r\n");
			return false;
		}

		printf("http status: %d\r\n", req.http_status());
		printf("server response body:\r\n\r\n");
		printf("%s\r\n", body.c_str());
		return true;
	}

private:
	acl::string server_addr_;	// web 服务器地址
	bool debug_mode_;
};

//////////////////////////////////////////////////////////////////////////

static void usage(const char* procname) {
	printf("usage: %s -h[help]\r\n", procname);
	printf("options:\r\n");
	printf("\t-f upload_file\r\n");
	printf("\t-D [if in debug mode]\r\n");
}

int main(int argc, char* argv[]) {
	int   ch;
	acl::string server_addr("127.0.0.1:8888");
	bool debug_mode = false;

	std::vector<std::string> files;

	while ((ch = getopt(argc, argv, "hs:f:D")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			files.push_back(optarg);
			break;
		case 'D':
			debug_mode = true;
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	// 将日志输出至屏幕
	acl::log::stdout_open(true);

	std::map<std::string, std::string> params;
	params["name1"] = "value1";
	params["name2"] = "value2";
	params["name3"] = "value3";

	// 开始运行
	http_request_test request(server_addr, debug_mode);
	request.run(files, params);
	return 0;
}

