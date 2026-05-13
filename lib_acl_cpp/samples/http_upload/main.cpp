// http_servlet.cpp : 定义控制台应用程序的入口点。
//
#include <assert.h>
#include <getopt.h>
#include "acl_cpp/lib_acl.hpp"

//////////////////////////////////////////////////////////////////////////

class http_request_test {
public:
	http_request_test(const char* server_addr) {
		server_addr_= server_addr;
	}

	~http_request_test() {}

	bool run(const std::vector<std::string>& files,
		 const std::map<std::string, std::string>& params) {
		const char* boundary = "================xxxxxx113333333";
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

		const char* local_file = "./tmp.mime";

		// 创建  HTTP 请求客户端
		acl::http_request req(server_addr_);

		// 添加 HTTP 请求头字段
		acl::http_header& hdr = req.request_header();  // 请求头对象的引用
		hdr.set_url("/upload");

		// 发送 HTTP MIME 请求数据
		if (req.upload(mime, local_file) == false) {
			logger_error("upload %s to %s error", local_file,
				server_addr_.c_str());
			return false;
		}

		acl::string body;
		if (req.get_body(body) == false) {
			logger_error("get http body error");
			return false;
		}

		return true;
	}

private:
	acl::string server_addr_;	// web 服务器地址
};

//////////////////////////////////////////////////////////////////////////

static void usage(const char* procname) {
	printf("usage: %s -h[help]\r\n", procname);
	printf("options:\r\n");
	printf("\t-f upload_file\r\n");
}

int main(int argc, char* argv[]) {
	int   ch;
	acl::string server_addr("127.0.0.1:8888");

	std::vector<std::string> files;

	while ((ch = getopt(argc, argv, "hs:f:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			files.push_back(optarg);
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
	http_request_test request(server_addr);
	request.run(files, params);
	return 0;
}

