#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

//////////////////////////////////////////////////////////////////////////////

static bool get_line(const char* path, std::string& out) {                      
	if (NULL == path) {                                                          
		return false;                                                            
	}                                                                            

#if 0
	acl::string tmp;
	acl::ifstream::load(path, tmp);
	//out = tmp.c_str();
	return true;
#endif

	std::ifstream ifs(path, std::ifstream::in);                                  
	if (!ifs.is_open()) {  // 文件打开失败                                       
		return false;                                                            
	}                                                                            

	while (!ifs.eof()) {                                                         
		std::string line;                                                        
		getline(ifs, line);  // 防止unix格式文件行后只有\n                       
		if (!line.empty()) {                                                     
			out = line;                                                          
		}                                                                        
	}                                                                            
	printf(">>>close file: %s\n", path);
	ifs.close();                                                                 
	printf(">>>close file: %s ok\n", path);
	return true;                                                                 
}

int main() {
	acl::acl_cpp_init();
	acl::log::stdout_open(true);
	acl::fiber::stdout_open(true);

	acl::server_socket ss;

	go[&ss] {
		if (!ss.open("127.0.0.1:8899")) {
			printf("open error\n");
			return 0;
		}

		printf("listen fd: %d\n", ss.sock_handle());

		while (true) {
			acl::socket_stream* conn = ss.accept();
			if (conn == nullptr) {
				continue;
			}

			go[conn] {
				printf("accept one fd=%d\n", conn->sock_handle());
				acl::string buf;
				while (!conn->eof()) {
					if (!conn->read_wait(5000)) {
						printf("read wait %d timeout: %s\r\n",
							conn->sock_handle(), acl::last_serror());
						break;
					}
					if (conn->gets(buf)) {
						conn->puts(buf);
					}
				}

				delete conn;
			};
		}
	};

	go[] {
		std::string buf;
		if (get_line("main.cpp", buf)) {
			printf("\r\n");
			printf("line: %s\n", buf.c_str());
		} else {
			printf("get line error\n");
		}
	};

	acl::fiber::schedule();
	return 0;
}
