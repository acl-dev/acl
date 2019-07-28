#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/diff_manager.hpp"
#include "acl_cpp/stdlib/diff_object.hpp"
#endif

namespace acl
{

diff_object::diff_object(diff_manager& manager)
: dbuf_obj(&manager.get_dbuf())
, manager_(manager)
{
}

} // namespace acl
