#pragma once
#include "fiber_cpp_define.hpp"

struct ACL_FIBER_COND;

namespace acl {

class fiber_mutex;

/**
 * fiber_cond - A condition variable for fiber synchronization
 * 
 * Conditional variables that can be used between coroutines, threads, and
 * between coroutines and threads.
 * 
 * This class provides a synchronization primitive that allows fibers to wait
 * for a condition to become true. It works similarly to pthread condition
 * variables but is optimized for fiber-based concurrency.
 * 
 * Key Features:
 * - Works across fibers within the same thread
 * - Works across different threads
 * - Works between fibers and threads
 * - Supports timeout-based waiting
 * - Compatible with fiber_mutex for proper synchronization
 * 
 * Typical Usage Pattern:
 * 1. Lock a fiber_mutex
 * 2. Check a condition (e.g., queue not empty)
 * 3. If condition is false, call wait() - this atomically unlocks the mutex
 *    and puts the fiber to sleep
 * 4. When notified, the fiber wakes up with the mutex locked again
 * 5. Re-check the condition (spurious wakeups are possible)
 * 6. Unlock the mutex when done
 * 
 * Thread Safety:
 * - Multiple fibers can wait on the same condition variable
 * - notify() can be called from any fiber or thread
 * - Must always be used with a fiber_mutex
 */
class FIBER_CPP_API fiber_cond {
public:
	/**
	 * Constructor - Creates a new condition variable
	 */
	fiber_cond();
	
	/**
	 * Destructor - Destroys the condition variable
	 * 
	 * Warning: Destroying a condition variable while fibers are waiting
	 * on it results in undefined behavior.
	 */
	~fiber_cond();

	/**
	 * wait - Wait for the condition variable to be signaled
	 * 
	 * Wait for the conditional variable to be available.
	 * 
	 * This method atomically:
	 * 1. Releases the mutex
	 * 2. Puts the calling fiber to sleep
	 * 3. Waits for a notification via notify()
	 * 4. Re-acquires the mutex before returning
	 * 
	 * Important Notes:
	 * - The mutex MUST be locked before calling wait()
	 * - The mutex will be locked again when wait() returns
	 * - Always check the condition in a loop (spurious wakeups possible)
	 * - If timeout occurs, the mutex is still re-acquired
	 * 
	 * @param mutex {fiber_mutex&} The mutex associated with this condition.
	 *              Must be locked by the calling fiber before wait() is called.
	 * @param timeout {int} The waiting timeout in milliseconds.
	 *                -1 means wait indefinitely (default)
	 *                0 means return immediately if not signaled
	 *                >0 means wait up to this many milliseconds
	 * @return {bool} Return true if notified/available, or false if timeout.
	 * 
	 * Example:
	 * ```cpp
	 * acl::fiber_mutex mutex;
	 * acl::fiber_cond cond;
	 * bool ready = false;
	 * 
	 * // Waiter fiber
	 * mutex.lock();
	 * while (!ready) {  // Always check condition in a loop
	 *     if (!cond.wait(mutex, 5000)) {  // 5 second timeout
	 *         printf("Timeout!\n");
	 *         break;
	 *     }
	 * }
	 * mutex.unlock();
	 * ```
	 */
	bool wait(fiber_mutex& mutex, int timeout = -1);

	/**
	 * notify - Wake up one waiting fiber
	 * 
	 * Wake up one waiter on the condition variable. If there are no
	 * waiters, return directly. The running behavior is similar to that
	 * of thread condition variables (pthread_cond_signal).
	 * 
	 * Behavior:
	 * - Wakes up ONE fiber waiting on this condition variable
	 * - If multiple fibers are waiting, only one is awakened
	 * - If no fibers are waiting, the call has no effect
	 * - The awakened fiber will re-acquire the mutex before continuing
	 * - Can be called with or without holding the associated mutex
	 * 
	 * Best Practice:
	 * - Call notify() while holding the mutex for predictable behavior
	 * - Or call notify() after releasing the mutex for better performance
	 * 
	 * @return {bool} Return true if successful or return false if error.
	 * 
	 * Example:
	 * ```cpp
	 * acl::fiber_mutex mutex;
	 * acl::fiber_cond cond;
	 * bool ready = false;
	 * 
	 * // Notifier fiber
	 * mutex.lock();
	 * ready = true;
	 * cond.notify();  // Wake up one waiter
	 * mutex.unlock();
	 * ```
	 * 
	 * Note: To wake up ALL waiting fibers, call notify() multiple times
	 * or use a broadcast mechanism if available in the underlying implementation.
	 */
	bool notify();

public:
	/**
	 * get_cond - Get the underlying C condition variable object
	 * 
	 * Return the C object conditional variable.
	 * 
	 * This method provides access to the underlying ACL_FIBER_COND structure
	 * for advanced use cases or interoperability with C code.
	 * 
	 * @return {ACL_FIBER_COND*} Pointer to the underlying condition variable
	 */
	ACL_FIBER_COND* get_cond() const {
		return cond_;
	}

private:
	ACL_FIBER_COND* cond_;  // Underlying C condition variable

	// Disable copy construction and assignment
	fiber_cond(const fiber_cond&);
	void operator=(const fiber_cond&);
};

} // namespace acl

/**
 * ============================================================================
 * USAGE EXAMPLES
 * ============================================================================
 * 
 * Example 1: Producer-Consumer Pattern
 * -------------------------------------
 * #include "fiber/fiber_cond.hpp"
 * #include "fiber/fiber_mutex.hpp"
 * #include "fiber/go_fiber.hpp"
 * #include <queue>
 * 
 * std::queue<int> queue;
 * acl::fiber_mutex mutex;
 * acl::fiber_cond cond;
 * bool done = false;
 * 
 * // Consumer fiber
 * void consumer() {
 *     while (true) {
 *         mutex.lock();
 *         
 *         // Wait for data or done signal
 *         while (queue.empty() && !done) {
 *             cond.wait(mutex);  // Atomically unlocks and waits
 *         }
 *         
 *         if (!queue.empty()) {
 *             int item = queue.front();
 *             queue.pop();
 *             mutex.unlock();
 *             
 *             printf("Consumed: %d\n", item);
 *         } else if (done) {
 *             mutex.unlock();
 *             break;  // Exit when done and queue is empty
 *         }
 *     }
 * }
 * 
 * // Producer fiber
 * void producer() {
 *     for (int i = 0; i < 10; i++) {
 *         mutex.lock();
 *         queue.push(i);
 *         printf("Produced: %d\n", i);
 *         cond.notify();  // Wake up a consumer
 *         mutex.unlock();
 *         
 *         acl::fiber::delay(100);  // Simulate work
 *     }
 *     
 *     // Signal completion
 *     mutex.lock();
 *     done = true;
 *     cond.notify();
 *     mutex.unlock();
 * }
 * 
 * int main() {
 *     go consumer;
 *     go producer;
 *     
 *     acl::fiber::schedule();
 *     return 0;
 * }
 * 
 * ============================================================================
 * Example 2: Event Notification with Timeout
 * ============================================================================
 * 
 * acl::fiber_mutex mutex;
 * acl::fiber_cond cond;
 * bool event_occurred = false;
 * 
 * // Waiter fiber
 * void waiter() {
 *     mutex.lock();
 *     
 *     // Wait up to 5 seconds for the event
 *     while (!event_occurred) {
 *         if (!cond.wait(mutex, 5000)) {
 *             printf("Timeout waiting for event\n");
 *             mutex.unlock();
 *             return;
 *         }
 *     }
 *     
 *     printf("Event occurred!\n");
 *     mutex.unlock();
 * }
 * 
 * // Event trigger fiber
 * void trigger() {
 *     acl::fiber::delay(2000);  // Wait 2 seconds
 *     
 *     mutex.lock();
 *     event_occurred = true;
 *     cond.notify();
 *     mutex.unlock();
 * }
 * 
 * int main() {
 *     go waiter;
 *     go trigger;
 *     
 *     acl::fiber::schedule();
 *     return 0;
 * }
 * 
 * ============================================================================
 * Example 3: Multiple Waiters
 * ============================================================================
 * 
 * acl::fiber_mutex mutex;
 * acl::fiber_cond cond;
 * int counter = 0;
 * const int TARGET = 10;
 * 
 * // Waiter fiber
 * void waiter(int id) {
 *     mutex.lock();
 *     
 *     while (counter < TARGET) {
 *         printf("Waiter %d: counter = %d, waiting...\n", id, counter);
 *         cond.wait(mutex);
 *     }
 *     
 *     printf("Waiter %d: target reached! counter = %d\n", id, counter);
 *     mutex.unlock();
 * }
 * 
 * // Incrementer fiber
 * void incrementer() {
 *     for (int i = 0; i < TARGET; i++) {
 *         acl::fiber::delay(500);
 *         
 *         mutex.lock();
 *         counter++;
 *         printf("Incremented counter to %d\n", counter);
 *         
 *         // Wake up all waiters when target is reached
 *         if (counter >= TARGET) {
 *             // Note: notify() wakes one waiter, call multiple times for all
 *             for (int j = 0; j < 3; j++) {
 *                 cond.notify();
 *             }
 *         } else {
 *             cond.notify();  // Wake up one waiter
 *         }
 *         
 *         mutex.unlock();
 *     }
 * }
 * 
 * int main() {
 *     go[&] { waiter(1); };
 *     go[&] { waiter(2); };
 *     go[&] { waiter(3); };
 *     go incrementer;
 *     
 *     acl::fiber::schedule();
 *     return 0;
 * }
 * 
 * ============================================================================
 * IMPORTANT NOTES
 * ============================================================================
 * 
 * 1. Always Use with fiber_mutex:
 *    - fiber_cond MUST be used with fiber_mutex
 *    - The mutex must be locked before calling wait()
 *    - The mutex is automatically unlocked during wait()
 *    - The mutex is automatically re-locked when wait() returns
 * 
 * 2. Check Condition in a Loop:
 *    Good:
 *      while (!condition) {
 *          cond.wait(mutex);
 *      }
 *    
 *    Bad:
 *      if (!condition) {
 *          cond.wait(mutex);  // Spurious wakeup not handled!
 *      }
 * 
 * 3. Spurious Wakeups:
 *    - wait() may return even when not notified
 *    - Always re-check the condition after wait() returns
 *    - This is standard behavior for condition variables
 * 
 * 4. Timeout Handling:
 *    - When timeout occurs, wait() returns false
 *    - The mutex is still re-acquired even on timeout
 *    - Always check the return value if using timeout
 * 
 * 5. Notification Semantics:
 *    - notify() wakes up ONE waiting fiber
 *    - If no fibers are waiting, notify() has no effect
 *    - Notifications are not queued
 *    - To wake all waiters, call notify() multiple times
 * 
 * 6. Lock Order:
 *    - Avoid deadlocks by always acquiring locks in the same order
 *    - Be careful with nested condition variables
 * 
 * 7. Performance Considerations:
 *    - Calling notify() while holding the mutex is safer but slower
 *    - Calling notify() after releasing the mutex is faster
 *    - Choose based on your correctness and performance needs
 * 
 * 8. Cross-Thread Usage:
 *    - fiber_cond works across threads
 *    - Can notify from one thread and wait in another
 *    - Ensure proper memory synchronization
 * 
 * ============================================================================
 * COMPARISON WITH OTHER SYNCHRONIZATION PRIMITIVES
 * ============================================================================
 * 
 * fiber_cond vs fiber_event:
 * - fiber_cond: Requires mutex, supports complex conditions
 * - fiber_event: Simpler, no mutex required, binary state
 * 
 * fiber_cond vs fiber_tbox:
 * - fiber_cond: Synchronization primitive, no data transfer
 * - fiber_tbox: Message passing, transfers data between fibers
 * 
 * fiber_cond vs wait_group:
 * - fiber_cond: General-purpose condition waiting
 * - wait_group: Specialized for counting task completion
 * 
 * fiber_cond vs channel:
 * - fiber_cond: Low-level synchronization primitive
 * - channel: Higher-level communication mechanism
 * 
 * ============================================================================
 * COMMON PATTERNS
 * ============================================================================
 * 
 * Pattern 1: Wait for State Change
 * ---------------------------------
 * mutex.lock();
 * while (state != DESIRED_STATE) {
 *     cond.wait(mutex);
 * }
 * // State is now DESIRED_STATE
 * mutex.unlock();
 * 
 * Pattern 2: Notify After State Change
 * -------------------------------------
 * mutex.lock();
 * state = NEW_STATE;
 * cond.notify();
 * mutex.unlock();
 * 
 * Pattern 3: Timed Wait
 * ---------------------
 * mutex.lock();
 * while (!condition) {
 *     if (!cond.wait(mutex, timeout_ms)) {
 *         // Timeout occurred
 *         mutex.unlock();
 *         return false;
 *     }
 * }
 * mutex.unlock();
 * return true;
 * 
 * Pattern 4: Predicate Function
 * ------------------------------
 * auto wait_for = [&](auto predicate) {
 *     mutex.lock();
 *     while (!predicate()) {
 *         cond.wait(mutex);
 *     }
 *     mutex.unlock();
 * };
 * 
 * wait_for([]() { return queue.size() > 0; });
 * 
 * ============================================================================
 */
