#pragma once
#include "../acl_cpp_define.hpp"
#include "acl_cpp/stdlib/class_counter.hpp"
#include <string>

#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>

namespace acl {

template<typename T>
std::string get_type_name(const T& obj) {
	int status = 0;
	char* realname = abi::__cxa_demangle(typeid(obj).name(), NULL, NULL, &status);
	std::string result = (status == 0) ? realname : typeid(obj).name();
	free(realname);
	return result;
}

#else

template<typename T>
std::string get_type_name(const T& obj) {
	int status = 0;
	std::string result = typeid(obj).name();
	return result;
}

#endif

#define ACL_OBJ_INC() do {                                                    \
	std::string name = acl::get_type_name(*this);                         \
	acl::class_counter::get_instance().inc(name.c_str());                 \
} while (0)

#define ACL_OBJ_DEC() do {                                                    \
	std::string name = acl::get_type_name(*this);                         \
	acl::class_counter::get_instance().dec(name.c_str());                 \
} while (0)

} // namespace
