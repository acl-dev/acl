#pragma once
#include "../acl_cpp_define.hpp"

namespace acl {
class string;

ACL_CPP_API void escape(const char* in, size_t len, string& out);
ACL_CPP_API bool unescape(const char* in, size_t len, string& out);

} // namespace acl
