#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

namespace acl {

/**
 * Objects that need delayed release in connection pool can inherit this class,
 * and can call aio_handle:delay_free function
 * to delay release. Purpose: Prevent objects from being released prematurely
 * during recursive traversal list release.
 */
class ACL_CPP_API aio_delay_free : public noncopyable {
public:
	aio_delay_free();
	virtual ~aio_delay_free();

	/**
	 * Determine whether timer is in locked state. Objects in locked state's timer
	 * cannot be deleted, otherwise memory access error will occur.
	 * @return {bool} Whether it is in locked state. Objects in locked state
	 *  cannot be released immediately.
	 */
	bool locked() const;

	/**
	 * Set object to locked state. When object is in locked state, timer callback
	 * will not
	 * automatically release this object.
	 */
	void set_locked();

	/**
	 * Unset object's locked state.
	 */
	void unset_locked();

	/**
	 * Release callback function. Internally calls aio_timer_delay_free function to
	 * determine whether object needs delayed
	 * release. Subclasses can override this function.
	 */
	virtual void destroy() {}

private:
	bool locked_;
	bool locked_fixed_;
};

} // namespace acl
