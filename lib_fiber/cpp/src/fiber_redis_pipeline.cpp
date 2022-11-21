#include "stdafx.hpp"
#include "fiber/fiber_tbox.hpp"
#include "fiber/fiber_redis_pipeline.hpp"

namespace acl {

fiber_redis_pipeline::fiber_redis_pipeline(const char* addr)
: redis_client_pipeline(addr)
{
}

fiber_redis_pipeline::~fiber_redis_pipeline(void) {}

box<redis_pipeline_message>* fiber_redis_pipeline::create_box(void)
{
	return new fiber_tbox<redis_pipeline_message>(false);
}

} // namespace acl
