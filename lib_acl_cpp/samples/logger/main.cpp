#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/acl_cpp_test.hpp"

int main(void)
{
	logger_open("test.log", "logger", "all:1");

	logger("logger opened!");
	acl::log::logger_test1();
	return (0);
}
