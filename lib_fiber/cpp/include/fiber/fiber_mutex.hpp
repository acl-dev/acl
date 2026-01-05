#pragma once
#include <vector>
#include "fiber_cpp_define.hpp"
#include "fiber_mutex_stat.hpp"

struct ACL_FIBER_MUTEX;

namespace acl {

/**
 * fiber_mutex - A mutex lock for fiber and thread synchronization (CROSS-THREAD SAFE)
 * 
 * ============================================================================
 * CROSS-THREAD CAPABILITY: FULLY THREAD-SAFE
 * ============================================================================
 * 
 * IMPORTANT: fiber_mutex CAN be used across multiple threads!
 * - Safe for fibers within the same thread
 * - Safe for fibers across different threads
 * - Safe between threads and fibers
 * - Safe between pure threads
 * 
 * This is different from fiber_sem which is SINGLE-THREAD ONLY.
 * 
 * ============================================================================
 * 
 * The mutex lock can be used for mutual exclusion between:
 * - Coroutines (fibers) within the same thread
 * - Coroutines (fibers) of different threads
 * - Pure threads (without fibers)
 * - Coroutines and independent threads
 * 
 * Key Features:
 * - Thread-safe: Full cross-thread synchronization support
 * - Fiber-aware: Efficiently blocks fibers without blocking threads
 * - Deadlock detection: Built-in deadlock detection capabilities
 * - RAII support: Use with fiber_mutex_guard for automatic unlock
 * - Trylock support: Non-blocking lock attempt
 * 
 * Common Use Cases:
 * - Protecting shared data accessed by multiple fibers/threads
 * - Critical sections in concurrent code
 * - Implementing higher-level synchronization primitives
 * - Resource access serialization
 * 
 * Thread Safety:
 * - Fully thread-safe: Can be used across multiple threads
 * - Fiber-safe: Can be used across multiple fibers
 * - No restrictions on where lock/unlock are called from
 * 
 * Performance:
 * - Optimized for fiber context switching
 * - Minimal overhead when used within same thread
 * - Efficient cross-thread synchronization
 */
class FIBER_CPP_API fiber_mutex {
public:
	/**
	 * Constructor - Create a new fiber mutex
	 * 
	 * @param mutex {ACL_FIBER_MUTEX*} Optional external C mutex object:
	 *  - NULL (default): Creates an internal C mutex object automatically
	 *  - Non-NULL: Wraps the provided C mutex object
	 *  
	 *  Important: If you provide an external mutex object, you are responsible
	 *  for its lifetime management. The fiber_mutex destructor will NOT free
	 *  the external mutex object.
	 * 
	 * Example:
	 * ```cpp
	 * // Create with internal mutex (most common)
	 * acl::fiber_mutex mutex1;
	 * 
	 * // Create with external C mutex (advanced usage)
	 * ACL_FIBER_MUTEX* c_mutex = acl_fiber_mutex_create();
	 * acl::fiber_mutex mutex2(c_mutex);
	 * // ... use mutex2 ...
	 * acl_fiber_mutex_free(c_mutex);  // You must free it
	 * ```
	 */
	explicit fiber_mutex(ACL_FIBER_MUTEX* mutex = NULL);
	
	/**
	 * Destructor - Destroys the fiber mutex
	 * 
	 * If the mutex was created internally (default constructor), the internal
	 * mutex is freed. If an external mutex was provided, it is NOT freed.
	 * 
	 * Warning: Destroying a mutex while it is locked or while fibers/threads
	 * are waiting on it results in undefined behavior.
	 */
	~fiber_mutex();

	/**
	 * lock - Acquire the mutex lock
	 * 
	 * Acquires the mutex lock. If the mutex is already locked by another
	 * fiber or thread, the calling fiber blocks until the mutex becomes
	 * available.
	 * 
	 * Thread Safety: Can be called from any fiber or thread.
	 * 
	 * Behavior:
	 * - If mutex is unlocked: Acquires immediately and returns
	 * - If mutex is locked: Blocks the calling fiber until available
	 * - Fiber-aware: Blocks the fiber, not the thread
	 * - Reentrant: NOT reentrant (same fiber cannot lock twice)
	 * 
	 * @return {bool} Returns true if successfully locked, false on error
	 * 
	 * Example:
	 * ```cpp
	 * acl::fiber_mutex mutex;
	 * int shared_counter = 0;
	 * 
	 * void increment() {
	 *     mutex.lock();
	 *     shared_counter++;  // Protected critical section
	 *     mutex.unlock();
	 * }
	 * 
	 * // Can be called from multiple fibers or threads
	 * go increment;  // Fiber 1
	 * go increment;  // Fiber 2
	 * std::thread(increment).detach();  // Thread
	 * ```
	 */
	bool lock();

	/**
	 * trylock - Try to acquire the mutex without blocking
	 * 
	 * Attempts to acquire the mutex lock. If the mutex is already locked,
	 * returns immediately with failure (does not block).
	 * 
	 * Thread Safety: Can be called from any fiber or thread.
	 * 
	 * @return {bool} Returns:
	 *  - true: Successfully acquired the mutex
	 *  - false: Mutex is already locked by another fiber/thread
	 * 
	 * Example:
	 * ```cpp
	 * acl::fiber_mutex mutex;
	 * 
	 * void try_process() {
	 *     if (mutex.trylock()) {
	 *         // Got the lock, do critical work
	 *         process_data();
	 *         mutex.unlock();
	 *     } else {
	 *         // Couldn't get lock, do something else
	 *         do_other_work();
	 *     }
	 * }
	 * ```
	 */
	bool trylock();

	/**
	 * unlock - Release the mutex lock
	 * 
	 * Releases the mutex lock and wakes up one waiting fiber or thread
	 * (if any are waiting).
	 * 
	 * Thread Safety: Can be called from any fiber or thread.
	 * 
	 * Important: Must be called by the same fiber/thread that acquired
	 * the lock. Unlocking from a different fiber/thread results in
	 * undefined behavior.
	 * 
	 * @return {bool} Returns true if successfully unlocked, false on error
	 * 
	 * Example:
	 * ```cpp
	 * acl::fiber_mutex mutex;
	 * 
	 * void critical_section() {
	 *     mutex.lock();
	 *     try {
	 *         // Critical work...
	 *         mutex.unlock();
	 *     } catch (...) {
	 *         mutex.unlock();  // Must unlock even on exception
	 *         throw;
	 *     }
	 * }
	 * 
	 * // Better: Use RAII guard
	 * void critical_section_safe() {
	 *     acl::fiber_mutex_guard guard(mutex);
	 *     // Critical work...
	 *     // Automatically unlocked when guard goes out of scope
	 * }
	 * ```
	 */
	bool unlock();

public:
	/**
	 * get_mutex - Get the underlying C mutex object
	 * 
	 * Returns the underlying ACL_FIBER_MUTEX structure for advanced use
	 * cases or interoperability with C code.
	 * 
	 * @return {ACL_FIBER_MUTEX*} Pointer to the underlying C mutex object
	 */
	ACL_FIBER_MUTEX* get_mutex() const {
		return mutex_;
	}

	/**
	 * deadlock - Detect deadlock state in the system
	 * 
	 * Static method that checks for deadlocks among all fiber mutexes
	 * in the system. This is a diagnostic tool for debugging deadlock
	 * issues in complex concurrent programs.
	 * 
	 * @param out {fiber_mutex_stats&} Output parameter that receives
	 *            detailed information about detected deadlocks, including:
	 *            - Which mutexes are involved
	 *            - Which fibers/threads are waiting
	 *            - Lock acquisition order
	 * 
	 * @return {bool} Returns:
	 *  - true: Deadlock detected (check 'out' for details)
	 *  - false: No deadlock detected
	 * 
	 * Example:
	 * ```cpp
	 * acl::fiber_mutex_stats stats;
	 * if (acl::fiber_mutex::deadlock(stats)) {
	 *     printf("Deadlock detected!\n");
	 *     // Analyze stats for deadlock details
	 * }
	 * ```
	 */
	static bool deadlock(fiber_mutex_stats& out);

	/**
	 * deadlock_show - Detect and display deadlock information
	 * 
	 * Static method that detects deadlocks and prints detailed information
	 * to standard output, including:
	 * - All fibers/threads involved in the deadlock
	 * - Stack traces of deadlocked fibers
	 * - Mutex acquisition chains
	 * 
	 * This is useful for debugging deadlock issues during development.
	 * 
	 * Example:
	 * ```cpp
	 * // Call periodically or when system appears hung
	 * acl::fiber_mutex::deadlock_show();
	 * ```
	 */
	static void deadlock_show();

private:
	ACL_FIBER_MUTEX* mutex_;           // Pointer to the actual mutex (internal or external)
	ACL_FIBER_MUTEX* mutex_internal_;  // Internal mutex (NULL if using external)

	// Disable copy construction and assignment
	fiber_mutex(const fiber_mutex&);
	void operator=(const fiber_mutex&);
};

/**
 * fiber_mutex_guard - RAII guard for automatic mutex management
 * 
 * This class provides automatic mutex acquisition and release using
 * RAII (Resource Acquisition Is Initialization) pattern. The mutex
 * is locked in the constructor and automatically unlocked in the destructor.
 * 
 * Benefits:
 * - Exception-safe: Mutex is unlocked even if exceptions occur
 * - Prevents forgetting to unlock
 * - Cleaner, more maintainable code
 * - Reduces risk of deadlocks from forgotten unlocks
 * 
 * Thread Safety: Can be used from any fiber or thread (same as fiber_mutex).
 * 
 * Example:
 * ```cpp
 * acl::fiber_mutex mutex;
 * int shared_data = 0;
 * 
 * void update_data(int value) {
 *     acl::fiber_mutex_guard guard(mutex);  // Locks mutex
 *     
 *     // Critical section - mutex is held
 *     shared_data = value;
 *     
 *     // If exception occurs here, mutex is still unlocked
 *     if (value < 0) {
 *         throw std::runtime_error("Invalid value");
 *     }
 *     
 *     // Mutex automatically unlocked when guard goes out of scope
 * }
 * ```
 */
class FIBER_CPP_API fiber_mutex_guard {
public:
	/**
	 * Constructor - Acquires the mutex lock
	 * 
	 * Locks the provided mutex. Blocks if the mutex is already locked.
	 * 
	 * @param mutex The mutex to guard
	 */
	explicit fiber_mutex_guard(fiber_mutex& mutex) : mutex_(mutex) {
		mutex_.lock();
	}

	/**
	 * Destructor - Releases the mutex lock
	 * 
	 * Automatically unlocks the mutex when the guard goes out of scope.
	 */
	~fiber_mutex_guard() {
		mutex_.unlock();
	}

private:
	fiber_mutex& mutex_;  // Reference to the guarded mutex
};

} // namespace acl

/**
 * ============================================================================
 * USAGE EXAMPLES
 * ============================================================================
 * 
 * Example 1: Basic Mutex Usage (Cross-Thread)
 * --------------------------------------------
 * #include "fiber/fiber_mutex.hpp"
 * #include "fiber/go_fiber.hpp"
 * #include <thread>
 * 
 * acl::fiber_mutex mutex;
 * int shared_counter = 0;
 * 
 * void increment_from_fiber() {
 *     for (int i = 0; i < 1000; i++) {
 *         mutex.lock();
 *         shared_counter++;
 *         mutex.unlock();
 *     }
 * }
 * 
 * void increment_from_thread() {
 *     for (int i = 0; i < 1000; i++) {
 *         mutex.lock();
 *         shared_counter++;
 *         mutex.unlock();
 *     }
 * }
 * 
 * int main() {
 *     // Start fibers in main thread
 *     go increment_from_fiber;
 *     go increment_from_fiber;
 *     
 *     // Start regular threads
 *     std::thread t1(increment_from_thread);
 *     std::thread t2(increment_from_thread);
 *     
 *     // Wait for fibers
 *     acl::fiber::schedule();
 *     
 *     // Wait for threads
 *     t1.join();
 *     t2.join();
 *     
 *     printf("Final counter: %d\n", shared_counter);  // Should be 4000
 *     return 0;
 * }
 * 
 * ============================================================================
 * Example 2: RAII Guard (Recommended Pattern)
 * ============================================================================
 * 
 * acl::fiber_mutex mutex;
 * std::vector<int> shared_data;
 * 
 * void add_data(int value) {
 *     acl::fiber_mutex_guard guard(mutex);  // Automatic lock
 *     
 *     shared_data.push_back(value);
 *     
 *     if (value < 0) {
 *         throw std::runtime_error("Negative value");
 *         // Mutex is still unlocked even with exception!
 *     }
 *     
 *     // Mutex automatically unlocked here
 * }
 * 
 * void process_data() {
 *     acl::fiber_mutex_guard guard(mutex);
 *     
 *     for (int val : shared_data) {
 *         printf("%d ", val);
 *     }
 *     printf("\n");
 * }
 * 
 * ============================================================================
 * Example 3: Trylock Pattern
 * ============================================================================
 * 
 * acl::fiber_mutex mutex;
 * 
 * void try_process() {
 *     if (mutex.trylock()) {
 *         // Got the lock, do critical work
 *         printf("Processing...\n");
 *         expensive_operation();
 *         mutex.unlock();
 *     } else {
 *         // Couldn't get lock, do something else
 *         printf("Busy, doing other work\n");
 *         do_other_work();
 *     }
 * }
 * 
 * ============================================================================
 * Example 4: Producer-Consumer with Cross-Thread Mutex
 * ============================================================================
 * 
 * acl::fiber_mutex mutex;
 * std::queue<int> queue;
 * bool done = false;
 * 
 * void producer() {
 *     for (int i = 0; i < 100; i++) {
 *         acl::fiber_mutex_guard guard(mutex);
 *         queue.push(i);
 *         printf("Produced: %d\n", i);
 *     }
 *     
 *     acl::fiber_mutex_guard guard(mutex);
 *     done = true;
 * }
 * 
 * void consumer() {
 *     while (true) {
 *         acl::fiber_mutex_guard guard(mutex);
 *         
 *         if (!queue.empty()) {
 *             int item = queue.front();
 *             queue.pop();
 *             printf("Consumed: %d\n", item);
 *         } else if (done) {
 *             break;
 *         }
 *     }
 * }
 * 
 * int main() {
 *     // Producer in fiber
 *     go producer;
 *     
 *     // Consumer in separate thread
 *     std::thread consumer_thread(consumer);
 *     
 *     acl::fiber::schedule();
 *     consumer_thread.join();
 *     
 *     return 0;
 * }
 * 
 * ============================================================================
 * Example 5: Deadlock Detection
 * ============================================================================
 * 
 * acl::fiber_mutex mutex1, mutex2;
 * 
 * void fiber1() {
 *     mutex1.lock();
 *     acl::fiber::delay(100);
 *     mutex2.lock();  // Potential deadlock
 *     // ...
 *     mutex2.unlock();
 *     mutex1.unlock();
 * }
 * 
 * void fiber2() {
 *     mutex2.lock();
 *     acl::fiber::delay(100);
 *     mutex1.lock();  // Potential deadlock
 *     // ...
 *     mutex1.unlock();
 *     mutex2.unlock();
 * }
 * 
 * void monitor() {
 *     acl::fiber::delay(1000);
 *     
 *     // Check for deadlocks
 *     acl::fiber_mutex_stats stats;
 *     if (acl::fiber_mutex::deadlock(stats)) {
 *         printf("Deadlock detected!\n");
 *         // Or use deadlock_show() for detailed output
 *         acl::fiber_mutex::deadlock_show();
 *     }
 * }
 * 
 * int main() {
 *     go fiber1;
 *     go fiber2;
 *     go monitor;
 *     
 *     acl::fiber::schedule();
 *     return 0;
 * }
 * 
 * ============================================================================
 * BEST PRACTICES
 * ============================================================================
 * 
 * 1. Always Use RAII Guard:
 *    Good:
 *      acl::fiber_mutex_guard guard(mutex);
 *      // Critical section
 *    
 *    Bad:
 *      mutex.lock();
 *      // Critical section
 *      mutex.unlock();  // Easy to forget, especially with exceptions
 * 
 * 2. Keep Critical Sections Short:
 *    - Minimize time holding the lock
 *    - Move non-critical work outside the lock
 *    - Reduces contention and improves performance
 * 
 * 3. Avoid Nested Locks:
 *    - Can lead to deadlocks
 *    - If necessary, always acquire in the same order
 *    - Use deadlock detection during development
 * 
 * 4. Use Trylock for Non-Critical Operations:
 *    - Allows fallback behavior when lock is busy
 *    - Prevents blocking on non-essential operations
 *    - Good for performance-sensitive code
 * 
 * 5. Deadlock Prevention:
 *    - Always acquire locks in the same order
 *    - Use timeouts when possible
 *    - Keep lock hierarchies simple
 *    - Use deadlock detection tools during development
 * 
 * 6. Cross-Thread Considerations:
 *    - fiber_mutex works across threads (unlike fiber_sem)
 *    - Safe to lock in one thread and unlock in another (not recommended)
 *    - Prefer locking and unlocking in the same fiber/thread
 * 
 * 7. Performance Tips:
 *    - Use trylock for optimistic locking
 *    - Minimize lock contention by reducing critical section size
 *    - Consider lock-free data structures for high-contention scenarios
 *    - Profile to identify lock bottlenecks
 * 
 * 8. Error Handling:
 *    - Always check return values of lock/unlock
 *    - Use RAII to ensure unlock even with exceptions
 *    - Handle lock failures gracefully
 * 
 * ============================================================================
 * COMPARISON: fiber_mutex vs fiber_sem
 * ============================================================================
 * 
 * +----------------------+----------------------+----------------------+
 * | Feature              | fiber_mutex          | fiber_sem            |
 * +----------------------+----------------------+----------------------+
 * | Thread Safety        | CROSS-THREAD SAFE    | SINGLE-THREAD ONLY   |
 * | Use Case             | Mutual exclusion     | Counting semaphore   |
 * | Cross-thread         | YES                  | NO                   |
 * | Fibers across threads| YES                  | NO                   |
 * | Deadlock detection   | YES                  | NO                   |
 * | RAII guard           | YES                  | YES                  |
 * | Trylock              | YES                  | YES (trywait)        |
 * | Reentrant            | NO                   | NO                   |
 * | Performance (same)   | Good                 | Better               |
 * | Performance (cross)  | Good                 | N/A (not supported)  |
 * +----------------------+----------------------+----------------------+
 * 
 * When to use fiber_mutex:
 * - Need cross-thread synchronization
 * - Protecting shared data accessed from multiple threads
 * - Mutual exclusion between fibers and threads
 * - Need deadlock detection
 * 
 * When to use fiber_sem:
 * - Single-threaded fiber coordination only
 * - Need counting semaphore semantics
 * - Maximum performance within single thread
 * - Resource pooling within one thread
 * 
 * ============================================================================
 * COMMON PATTERNS
 * ============================================================================
 * 
 * Pattern 1: Simple Critical Section
 * -----------------------------------
 * acl::fiber_mutex mutex;
 * {
 *     acl::fiber_mutex_guard guard(mutex);
 *     // Critical section
 * }
 * 
 * Pattern 2: Conditional Locking
 * -------------------------------
 * if (mutex.trylock()) {
 *     // Got lock, do work
 *     mutex.unlock();
 * } else {
 *     // Couldn't get lock, do alternative
 * }
 * 
 * Pattern 3: Scoped Locking
 * -------------------------
 * void process() {
 *     acl::fiber_mutex_guard guard(mutex);
 *     // Entire function is critical section
 * }
 * 
 * Pattern 4: Multiple Locks (Ordered)
 * ------------------------------------
 * // Always lock in the same order to prevent deadlock
 * mutex1.lock();
 * mutex2.lock();
 * // Critical section
 * mutex2.unlock();
 * mutex1.unlock();
 * 
 * ============================================================================
 */
