#include "stdafx.hpp"
#include "fiber/fiber.hpp"
#include "fiber/fiber_mutex.hpp"

namespace acl {

fiber_mutex::fiber_mutex(ACL_FIBER_MUTEX *mutex /* NULL */)
{
	if (mutex) {
		mutex_ = mutex;
		mutex_internal_ = NULL;
	} else {
		mutex_internal_ = mutex_ = acl_fiber_mutex_create(0);
	}
}

fiber_mutex::~fiber_mutex(void)
{
	if (mutex_internal_) {
		acl_fiber_mutex_free(mutex_internal_);
	}
}

bool fiber_mutex::lock(void)
{
	acl_fiber_mutex_lock(mutex_);
	return true;
}

bool fiber_mutex::trylock(void)
{
	return acl_fiber_mutex_trylock(mutex_) == 0 ? true : false;
}

bool fiber_mutex::unlock(void)
{
	acl_fiber_mutex_unlock(mutex_);
	return true;
}

bool fiber_mutex::deadlock(fiber_mutex_stats& out)
{
	ACL_FIBER_MUTEX_STATS *stats = acl_fiber_mutex_deadlock();

	if (stats == NULL) {
		return false;
	}

	for (size_t i = 0; i < stats->count; i++) {
		fiber_mutex_stat stat;
		stat.fb = new fiber(stats->stats[i].fiber);
		stat.waiting = stats->stats[i].waiting;

		for (size_t j = 0; j < stats->stats[i].count; j++) {
			ACL_FIBER_MUTEX* mutex = stats->stats[i].holding[j];
			stat.holding.push_back(mutex);
		}

		out.stats.push_back(stat);
	}

	acl_fiber_mutex_stats_free(stats);
	return true;
}

} // namespace acl
