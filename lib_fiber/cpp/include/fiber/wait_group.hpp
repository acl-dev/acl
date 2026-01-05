#pragma once
#include "fiber_cpp_define.hpp"

//#include "acl_cpp/stdlib/atomic.hpp"

namespace acl {

template<typename T> class fiber_tbox;

/**
 * wait_group - A synchronization primitive for waiting on multiple fibers
 * 
 * This class is similar to Go's sync.WaitGroup. It allows one or more fibers
 * to wait for a collection of other fibers to complete their work.
 * 
 * Typical usage pattern:
 * 1. Call add(n) to set the counter to the number of fibers to wait for
 * 2. Each fiber calls done() when it completes its work
 * 3. Call wait() to block until the counter reaches zero
 * 
 * Thread-safe: Uses atomic operations for the internal counter.
 * 
 * Example:
 * ```cpp
 * acl::wait_group wg;
 * wg.add(3);  // Wait for 3 fibers
 * 
 * go [&] {
 *     // Do work...
 *     wg.done();
 * };
 * 
 * go [&] {
 *     // Do work...
 *     wg.done();
 * };
 * 
 * go [&] {
 *     // Do work...
 *     wg.done();
 * };
 * 
 * wg.wait();  // Block until all 3 fibers call done()
 * ```
 */
class FIBER_CPP_API wait_group {
public:
	/**
	 * Constructor - Creates a wait_group with counter initialized to 0
	 */
	wait_group();
	
	/**
	 * Destructor - Cleans up internal resources
	 */
	~wait_group();

	/**
	 * add - Increment the wait group counter
	 * 
	 * Adds n to the wait group counter. This should be called before
	 * starting the fibers you want to wait for.
	 * 
	 * @param n The number to add to the counter (can be negative to decrement)
	 * 
	 * Note: If the counter becomes negative, it's a programming error.
	 *       If the counter becomes zero, any waiting fibers are unblocked.
	 */
	void add(int n);
	
	/**
	 * done - Decrement the wait group counter by 1
	 * 
	 * This should be called by each fiber when it completes its work.
	 * Equivalent to add(-1).
	 * 
	 * When the counter reaches zero, all fibers blocked on wait() are unblocked.
	 */
	void done();
	
	/**
	 * wait - Block until the wait group counter reaches zero
	 * 
	 * This method blocks the calling fiber until the counter becomes zero.
	 * Multiple fibers can call wait() concurrently; they will all be unblocked
	 * when the counter reaches zero.
	 */
	void wait();

private:
	atomic_long state_;                 // Atomic counter for tracking pending operations
	fiber_tbox<unsigned long>* box_;    // Mailbox for fiber synchronization
};

} // namespace acl

/**
 * ============================================================================
 * USAGE PATTERNS
 * ============================================================================
 * 
 * Pattern 1: Wait for multiple parallel tasks
 * --------------------------------------------
 * void process_items(const std::vector<Item>& items) {
 *     acl::wait_group wg;
 *     wg.add(items.size());
 * 
 *     for (const auto& item : items) {
 *         go [&wg, item] {
 *             process(item);
 *             wg.done();
 *         };
 *     }
 * 
 *     wg.wait();  // Wait for all items to be processed
 *     printf("All items processed\n");
 * }
 * 
 * Pattern 2: Dynamic task addition
 * ---------------------------------
 * void worker_pool() {
 *     acl::wait_group wg;
 * 
 *     for (int i = 0; i < 10; i++) {
 *         wg.add(1);  // Add one task at a time
 *         go [&wg, i] {
 *             perform_task(i);
 *             wg.done();
 *         };
 *     }
 * 
 *     wg.wait();
 * }
 * 
 * Pattern 3: Nested wait groups
 * ------------------------------
 * void hierarchical_processing() {
 *     acl::wait_group outer_wg;
 *     outer_wg.add(3);
 * 
 *     for (int i = 0; i < 3; i++) {
 *         go [&outer_wg, i] {
 *             acl::wait_group inner_wg;
 *             inner_wg.add(5);
 * 
 *             for (int j = 0; j < 5; j++) {
 *                 go [&inner_wg, i, j] {
 *                     process_subtask(i, j);
 *                     inner_wg.done();
 *                 };
 *             }
 * 
 *             inner_wg.wait();  // Wait for all subtasks
 *             outer_wg.done();
 *         };
 *     }
 * 
 *     outer_wg.wait();  // Wait for all main tasks
 * }
 * 
 * Pattern 4: Error handling with wait groups
 * -------------------------------------------
 * bool process_with_errors(const std::vector<Task>& tasks) {
 *     acl::wait_group wg;
 *     std::atomic<bool> has_error(false);
 * 
 *     wg.add(tasks.size());
 *     for (const auto& task : tasks) {
 *         go [&wg, &has_error, task] {
 *             if (!execute(task)) {
 *                 has_error = true;
 *             }
 *             wg.done();
 *         };
 *     }
 * 
 *     wg.wait();
 *     return !has_error;
 * }
 * 
 * ============================================================================
 * IMPORTANT NOTES
 * ============================================================================
 * 
 * 1. Call add() BEFORE starting fibers:
 *    Good:
 *      wg.add(1);
 *      go [&] { ...; wg.done(); };
 *    
 *    Bad (race condition):
 *      go [&] { ...; wg.done(); };
 *      wg.add(1);  // May be too late!
 * 
 * 2. Match add() and done() calls:
 *    - Each add(n) must be matched by n calls to done()
 *    - Calling done() more times than add() is an error
 * 
 * 3. Reusing wait_group:
 *    - After wait() returns, the counter is zero
 *    - You can call add() again to reuse the wait_group
 * 
 * 4. Multiple waiters:
 *    - Multiple fibers can call wait() on the same wait_group
 *    - All will be unblocked when the counter reaches zero
 * 
 * 5. Performance:
 *    - Uses atomic operations for lock-free counter updates
 *    - Efficient for coordinating large numbers of fibers
 * 
 * ============================================================================
 * COMPARISON WITH OTHER SYNCHRONIZATION PRIMITIVES
 * ============================================================================
 * 
 * wait_group vs fiber_tbox:
 * - wait_group: Wait for N operations to complete (N-to-1)
 * - fiber_tbox: Message passing between fibers (1-to-1 or N-to-M)
 * 
 * wait_group vs fiber_event:
 * - wait_group: Counts down to zero, then unblocks
 * - fiber_event: Simple signal/wait mechanism (on/off state)
 * 
 * wait_group vs barrier:
 * - wait_group: One-time countdown (can be reused)
 * - barrier: Reusable synchronization point for fixed number of threads
 * 
 * ============================================================================
 */
