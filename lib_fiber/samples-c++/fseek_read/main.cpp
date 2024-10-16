#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>

class file_reader : public acl::fiber {
public:
	file_reader(const char* filepath, int offset, size_t length)
	: filepath_(filepath), offset_(offset), length_(length) {}

	~file_reader() {}

protected:
	// @override
	void run() {
		acl::ifstream reader;
		if (!reader.open_read(filepath_)) {
			printf("open_read %s error\r\n", filepath_.c_str());
			return;
		}
		printf("open_read %s ok\r\n", filepath_.c_str());

		if (reader.fseek(offset_, SEEK_SET) == -1) {
			printf("fseek %s error %s\r\n", filepath_.c_str(), acl::last_serror());
			return;
		}

		char buf[8192];
		if (length_ >= sizeof(buf)) {
			length_ = sizeof(buf);
		} else if (length_ == 0) {
			length_ = 10;
		}

		int ret = reader.read(buf, length_, false);
		if (ret <= 0) {
			printf("read from %s error\r\n", filepath_.c_str());
			return;
		}

		buf[ret] = 0;
		printf("read data: [%s]\r\n", buf);
	}

private:
	acl::string filepath_;
	int offset_;
	size_t length_;
};

static void usage(const char* procname) {
	printf("usage: %s -h [help] -f file_path -o offset -n length\r\n", procname);
}

int main(int argc, char* argv[]) {
	int ch, offset = 0, length = 20;
	acl::string filepath;

	while ((ch = getopt(argc, argv, "hf:o:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			filepath = optarg;
			break;
		case 'o':
			offset = atoi(optarg);
			break;
		case 'n':
			length = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	if (filepath.empty()) {
		usage(argv[0]);
		return 1;
	}

	file_reader reader(filepath, offset, (size_t) length);
	reader.start();

	acl::fiber::schedule();
	return 0;
}
