#pragma once
#include "fiber_cpp_define.hpp"

namespace acl {

class FIBER_CPP_API fiber_redis_pipeline : public redis_client_pipeline {
public:
	fiber_redis_pipeline(const char* addr);
	~fiber_redis_pipeline(void);

	// @override
	box<redis_pipeline_message>* create_box(void);
};

} // namespace acl
