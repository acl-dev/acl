#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/acl_cpp_test.hpp"

int main(void)
{
	acl::log::stdout_open(true);
	logger_open("test.log", "logger", "101->DEBUG_TEST1:1");

	logger("logger opened!");
	acl::log::logger_test1();
	return (0);
}
