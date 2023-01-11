#include "stdafx.hpp"
#include "fiber/fiber.hpp"
#include "fiber/fiber_mutex.hpp"
#include "fiber/fiber_mutex_stat.hpp"

namespace acl {

fiber_mutex_stats::~fiber_mutex_stats(void)
{
	for (std::vector<fiber_mutex_stat>::iterator it = stats.begin();
		it != stats.end(); ++it) {
		delete (*it).fb;
	}
}

} // namespace acl
