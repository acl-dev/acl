#pragma once
#include "fiber_cpp_define.hpp"

struct ACL_FIBER_LOCK;
struct ACL_FIBER_RWLOCK;

namespace acl {

/**
 * fiber_lock - A lightweight mutex lock for fiber synchronization (SINGLE-THREAD ONLY)
 * 
 * ============================================================================
 * CRITICAL RESTRICTION: SINGLE-THREAD USE ONLY
 * ============================================================================
 * 
 * WARNING: fiber_lock can ONLY be used within a SINGLE THREAD!
 * - All lock() and unlock() calls must be from the SAME thread
 * - Cannot be used for cross-thread synchronization
 * - Designed for fiber-to-fiber synchronization within one thread
 * 
 * For cross-thread synchronization, use:
 * - fiber_mutex (thread-safe, cross-thread capable)
 * - Standard thread synchronization primitives (std::mutex, etc.)
 * 
 * ============================================================================
 * 
 * Mutex lock that can only be used for mutual exclusion between coroutines
 * (fibers) within the same thread.
 * 
 * Key Features:
 * - Lightweight: Optimized for single-thread fiber switching
 * - Fast: Lower overhead than fiber_mutex within same thread
 * - Simple: Basic lock/unlock semantics
 * - Trylock: Non-blocking lock attempt
 * 
 * Comparison with fiber_mutex:
 * - fiber_lock: Single-thread only, faster, simpler
 * - fiber_mutex: Cross-thread safe, more features (deadlock detection)
 * 
 * Use fiber_lock when:
 * - All fibers run in the same thread
 * - Maximum performance is needed
 * - Cross-thread synchronization is not required
 * 
 * Thread Safety:
 * - NOT thread-safe: Must be used within a single thread only
 * - Safe for multiple fibers within the same thread
 * - Do NOT call lock()/unlock() from different threads
 */
class FIBER_CPP_API fiber_lock {
public:
	/**
	 * Constructor - Create a new fiber lock
	 */
	fiber_lock();
	
	/**
	 * Destructor - Destroys the fiber lock
	 * 
	 * Warning: Destroying a lock while it is locked or while fibers
	 * are waiting on it results in undefined behavior.
	 */
	~fiber_lock();

	/**
	 * lock - Acquire the lock
	 * 
	 * Acquires the lock. If the lock is already held by another fiber
	 * in the same thread, the calling fiber blocks until the lock becomes
	 * available.
	 * 
	 * IMPORTANT: Must be called from the SAME thread as unlock()!
	 * 
	 * @return {bool} Returns true if successfully locked, false on error
	 * 
	 * Example:
	 * ```cpp
	 * acl::fiber_lock lock;
	 * int shared_data = 0;
	 * 
	 * void fiber_func() {
	 *     lock.lock();
	 *     shared_data++;  // Protected critical section
	 *     lock.unlock();
	 * }
	 * 
	 * // All fibers must run in the SAME thread
	 * go fiber_func;
	 * go fiber_func;
	 * acl::fiber::schedule();  // Single-threaded scheduler
	 * ```
	 */
	bool lock();

	/**
	 * trylock - Try to acquire the lock without blocking
	 * 
	 * Attempts to acquire the lock. If the lock is already held,
	 * returns immediately with failure (does not block).
	 * 
	 * IMPORTANT: Must be called from the SAME thread as the lock holder!
	 * 
	 * @return {bool} Returns:
	 *  - true: Successfully acquired the lock
	 *  - false: Lock is already held by another fiber
	 * 
	 * Example:
	 * ```cpp
	 * acl::fiber_lock lock;
	 * 
	 * void try_process() {
	 *     if (lock.trylock()) {
	 *         // Got the lock, do work
	 *         process_data();
	 *         lock.unlock();
	 *     } else {
	 *         // Couldn't get lock, do something else
	 *         do_other_work();
	 *     }
	 * }
	 * ```
	 */
	bool trylock();

	/**
	 * unlock - Release the lock
	 * 
	 * Releases the lock and wakes up one waiting fiber (if any).
	 * 
	 * IMPORTANT: Must be called from the SAME thread as lock()!
	 * 
	 * @return {bool} Returns true if successfully unlocked, false on error
	 * 
	 * Example:
	 * ```cpp
	 * acl::fiber_lock lock;
	 * 
	 * void critical_section() {
	 *     lock.lock();
	 *     // Critical work...
	 *     lock.unlock();
	 * }
	 * ```
	 */
	bool unlock();

private:
	ACL_FIBER_LOCK* lock_;  // Underlying C lock structure

	// Disable copy construction and assignment
	fiber_lock(const fiber_lock&);
	void operator=(const fiber_lock&);
};

/**
 * fiber_rwlock - A read-write lock for fiber synchronization (SINGLE-THREAD ONLY)
 * 
 * ============================================================================
 * CRITICAL RESTRICTION: SINGLE-THREAD USE ONLY
 * ============================================================================
 * 
 * WARNING: fiber_rwlock can ONLY be used within a SINGLE THREAD!
 * - All rlock(), wlock(), runlock(), wunlock() calls must be from the SAME thread
 * - Cannot be used for cross-thread synchronization
 * - Designed for fiber-to-fiber synchronization within one thread
 * 
 * For cross-thread read-write synchronization, use:
 * - std::shared_mutex (C++17)
 * - pthread_rwlock_t
 * - Other thread-safe read-write lock implementations
 * 
 * ============================================================================
 * 
 * Read/write lock that can only be used for mutual exclusion between
 * coroutines (fibers) within the same thread.
 * 
 * Key Features:
 * - Multiple readers: Multiple fibers can hold read locks simultaneously
 * - Exclusive writer: Only one fiber can hold write lock at a time
 * - Writer priority: Writers block new readers
 * - Lightweight: Optimized for single-thread fiber switching
 * 
 * Read-Write Lock Semantics:
 * - Read lock: Shared access, multiple readers allowed
 * - Write lock: Exclusive access, no readers or writers allowed
 * - Readers block writers, writers block everyone
 * 
 * Use fiber_rwlock when:
 * - Read operations are much more frequent than writes
 * - All fibers run in the same thread
 * - Need to maximize read concurrency
 * 
 * Thread Safety:
 * - NOT thread-safe: Must be used within a single thread only
 * - Safe for multiple fibers within the same thread
 * - Do NOT call lock/unlock methods from different threads
 */
class FIBER_CPP_API fiber_rwlock {
public:
	/**
	 * Constructor - Create a new read-write lock
	 */
	fiber_rwlock();
	
	/**
	 * Destructor - Destroys the read-write lock
	 * 
	 * Warning: Destroying a lock while it is locked or while fibers
	 * are waiting on it results in undefined behavior.
	 */
	~fiber_rwlock();

	/**
	 * rlock - Acquire a read lock (shared)
	 * 
	 * Acquires a read lock. Multiple fibers can hold read locks
	 * simultaneously. If a write lock is held, the calling fiber
	 * blocks until the write lock is released.
	 * 
	 * IMPORTANT: Must be called from the SAME thread as runlock()!
	 * 
	 * Example:
	 * ```cpp
	 * acl::fiber_rwlock rwlock;
	 * std::vector<int> shared_data;
	 * 
	 * void reader() {
	 *     rwlock.rlock();
	 *     // Multiple readers can access simultaneously
	 *     for (int val : shared_data) {
	 *         printf("%d ", val);
	 *     }
	 *     rwlock.runlock();
	 * }
	 * ```
	 */
	void rlock();

	/**
	 * tryrlock - Try to acquire a read lock without blocking
	 * 
	 * Attempts to acquire a read lock. If a write lock is held,
	 * returns immediately with failure (does not block).
	 * 
	 * @return {bool} Returns:
	 *  - true: Successfully acquired the read lock
	 *  - false: Write lock is held by another fiber
	 * 
	 * Example:
	 * ```cpp
	 * if (rwlock.tryrlock()) {
	 *     // Got read lock
	 *     read_data();
	 *     rwlock.runlock();
	 * } else {
	 *     // Write lock held, do something else
	 * }
	 * ```
	 */
	bool tryrlock();

	/**
	 * runlock - Release a read lock
	 * 
	 * Releases a read lock. If this is the last reader and writers
	 * are waiting, one writer will be unblocked.
	 * 
	 * IMPORTANT: Must be called from the SAME thread as rlock()!
	 */
	void runlock();

	/**
	 * wlock - Acquire a write lock (exclusive)
	 * 
	 * Acquires a write lock. Only one fiber can hold a write lock
	 * at a time, and no readers are allowed. If any locks (read or
	 * write) are held, the calling fiber blocks until all locks are
	 * released.
	 * 
	 * IMPORTANT: Must be called from the SAME thread as wunlock()!
	 * 
	 * Example:
	 * ```cpp
	 * acl::fiber_rwlock rwlock;
	 * std::vector<int> shared_data;
	 * 
	 * void writer() {
	 *     rwlock.wlock();
	 *     // Exclusive access - no readers or other writers
	 *     shared_data.push_back(42);
	 *     rwlock.wunlock();
	 * }
	 * ```
	 */
	void wlock();

	/**
	 * trywlock - Try to acquire a write lock without blocking
	 * 
	 * Attempts to acquire a write lock. If any locks (read or write)
	 * are held, returns immediately with failure (does not block).
	 * 
	 * @return {bool} Returns:
	 *  - true: Successfully acquired the write lock
	 *  - false: Lock is held by another fiber (reader or writer)
	 * 
	 * Example:
	 * ```cpp
	 * if (rwlock.trywlock()) {
	 *     // Got write lock
	 *     modify_data();
	 *     rwlock.wunlock();
	 * } else {
	 *     // Lock held, defer write
	 *     queue_write_operation();
	 * }
	 * ```
	 */
	bool trywlock();

	/**
	 * wunlock - Release a write lock
	 * 
	 * Releases a write lock. Waiting readers or writers will be
	 * unblocked (typically writers have priority).
	 * 
	 * IMPORTANT: Must be called from the SAME thread as wlock()!
	 */
	void wunlock();

private:
	ACL_FIBER_RWLOCK* rwlk_;  // Underlying C read-write lock structure

	// Disable copy construction and assignment
	fiber_rwlock(const fiber_rwlock&);
	void operator=(const fiber_rwlock&);
};

} // namespace acl

/**
 * ============================================================================
 * USAGE EXAMPLES
 * ============================================================================
 * 
 * Example 1: Basic fiber_lock Usage (Single-Thread)
 * --------------------------------------------------
 * #include "fiber/fiber_lock.hpp"
 * #include "fiber/go_fiber.hpp"
 * 
 * acl::fiber_lock lock;
 * int counter = 0;
 * 
 * void increment() {
 *     for (int i = 0; i < 1000; i++) {
 *         lock.lock();
 *         counter++;
 *         lock.unlock();
 *     }
 * }
 * 
 * int main() {
 *     // All fibers run in the SAME thread
 *     go increment;
 *     go increment;
 *     go increment;
 *     
 *     acl::fiber::schedule();  // Single-threaded scheduler
 *     printf("Counter: %d\n", counter);  // Should be 3000
 *     return 0;
 * }
 * 
 * ============================================================================
 * Example 2: Read-Write Lock Pattern
 * ============================================================================
 * 
 * acl::fiber_rwlock rwlock;
 * std::vector<int> shared_data;
 * 
 * void reader(int id) {
 *     for (int i = 0; i < 5; i++) {
 *         rwlock.rlock();
 *         printf("Reader %d: ", id);
 *         for (int val : shared_data) {
 *             printf("%d ", val);
 *         }
 *         printf("\n");
 *         rwlock.runlock();
 *         
 *         acl::fiber::delay(100);
 *     }
 * }
 * 
 * void writer(int id) {
 *     for (int i = 0; i < 3; i++) {
 *         rwlock.wlock();
 *         shared_data.push_back(id * 10 + i);
 *         printf("Writer %d: added %d\n", id, id * 10 + i);
 *         rwlock.wunlock();
 *         
 *         acl::fiber::delay(200);
 *     }
 * }
 * 
 * int main() {
 *     // Start multiple readers and writers (all in same thread)
 *     go[&] { reader(1); };
 *     go[&] { reader(2); };
 *     go[&] { reader(3); };
 *     go[&] { writer(1); };
 *     go[&] { writer(2); };
 *     
 *     acl::fiber::schedule();
 *     return 0;
 * }
 * 
 * ============================================================================
 * Example 3: Trylock Pattern
 * ============================================================================
 * 
 * acl::fiber_lock lock;
 * int busy_count = 0;
 * 
 * void try_process(int id) {
 *     if (lock.trylock()) {
 *         printf("Fiber %d: got lock\n", id);
 *         // Do work
 *         acl::fiber::delay(100);
 *         lock.unlock();
 *     } else {
 *         printf("Fiber %d: lock busy\n", id);
 *         busy_count++;
 *     }
 * }
 * 
 * int main() {
 *     for (int i = 0; i < 10; i++) {
 *         go[i] { try_process(i); };
 *     }
 *     
 *     acl::fiber::schedule();
 *     printf("Busy count: %d\n", busy_count);
 *     return 0;
 * }
 * 
 * ============================================================================
 * Example 4: Read-Heavy Workload
 * ============================================================================
 * 
 * acl::fiber_rwlock rwlock;
 * std::map<std::string, int> cache;
 * 
 * int read_cache(const std::string& key) {
 *     rwlock.rlock();
 *     auto it = cache.find(key);
 *     int value = (it != cache.end()) ? it->second : -1;
 *     rwlock.runlock();
 *     return value;
 * }
 * 
 * void write_cache(const std::string& key, int value) {
 *     rwlock.wlock();
 *     cache[key] = value;
 *     rwlock.wunlock();
 * }
 * 
 * void reader_fiber() {
 *     for (int i = 0; i < 100; i++) {
 *         int val = read_cache("key1");
 *         printf("Read: %d\n", val);
 *         acl::fiber::delay(10);
 *     }
 * }
 * 
 * void writer_fiber() {
 *     for (int i = 0; i < 10; i++) {
 *         write_cache("key1", i);
 *         printf("Wrote: %d\n", i);
 *         acl::fiber::delay(100);
 *     }
 * }
 * 
 * int main() {
 *     // Many readers, few writers
 *     for (int i = 0; i < 10; i++) {
 *         go reader_fiber;
 *     }
 *     go writer_fiber;
 *     
 *     acl::fiber::schedule();
 *     return 0;
 * }
 * 
 * ============================================================================
 * BEST PRACTICES
 * ============================================================================
 * 
 * 1. Single-Thread Restriction:
 *    - NEVER use fiber_lock or fiber_rwlock across multiple threads
 *    - All fibers must run in the same thread
 *    - Use fiber_mutex for cross-thread synchronization
 * 
 * 2. Choose the Right Lock:
 *    Same thread, simple mutex:
 *      - Use fiber_lock (fastest)
 *    
 *    Same thread, read-heavy:
 *      - Use fiber_rwlock (allows concurrent reads)
 *    
 *    Cross-thread:
 *      - Use fiber_mutex (thread-safe)
 * 
 * 3. Keep Critical Sections Short:
 *    - Minimize time holding the lock
 *    - Move non-critical work outside the lock
 *    - Reduces contention between fibers
 * 
 * 4. Use Trylock for Non-Critical Operations:
 *    - Allows fallback behavior when lock is busy
 *    - Prevents blocking on non-essential operations
 *    - Good for performance-sensitive code
 * 
 * 5. Read-Write Lock Guidelines:
 *    - Use when reads >> writes (10:1 or higher ratio)
 *    - Readers can run concurrently
 *    - Writers block all other access
 *    - Consider simple lock if writes are frequent
 * 
 * 6. Avoid Deadlocks:
 *    - Always acquire locks in the same order
 *    - Don't hold lock while waiting for another resource
 *    - Keep lock hierarchies simple
 * 
 * 7. Error Handling:
 *    - Always check return values of lock operations
 *    - Ensure unlock is called even with exceptions
 *    - Consider RAII wrappers for automatic unlock
 * 
 * 8. Performance:
 *    - fiber_lock is faster than fiber_mutex within same thread
 *    - fiber_rwlock is best for read-heavy workloads
 *    - Profile to identify lock bottlenecks
 * 
 * ============================================================================
 * COMPARISON TABLE
 * ============================================================================
 * 
 * +------------------+------------------+------------------+------------------+
 * | Feature          | fiber_lock       | fiber_rwlock     | fiber_mutex      |
 * +------------------+------------------+------------------+------------------+
 * | Thread Safety    | Single-thread    | Single-thread    | Cross-thread     |
 * | Lock Type        | Exclusive        | Read/Write       | Exclusive        |
 * | Multiple readers | NO               | YES              | NO               |
 * | Performance      | Fastest          | Fast (reads)     | Good             |
 * | Cross-thread     | NO               | NO               | YES              |
 * | Deadlock detect  | NO               | NO               | YES              |
 * | Use case         | Simple mutex     | Read-heavy       | Cross-thread     |
 * | Overhead         | Lowest           | Low              | Medium           |
 * +------------------+------------------+------------------+------------------+
 * 
 * When to use fiber_lock:
 * - Single-threaded fiber coordination
 * - Simple mutual exclusion
 * - Maximum performance needed
 * - No read/write distinction
 * 
 * When to use fiber_rwlock:
 * - Single-threaded fiber coordination
 * - Read operations >> write operations
 * - Want concurrent reads
 * - Performance is important
 * 
 * When to use fiber_mutex:
 * - Need cross-thread synchronization
 * - Fibers run in multiple threads
 * - Need deadlock detection
 * - Thread-safety is required
 * 
 * ============================================================================
 * COMMON PATTERNS
 * ============================================================================
 * 
 * Pattern 1: Simple Critical Section (fiber_lock)
 * ------------------------------------------------
 * lock.lock();
 * // Critical section
 * lock.unlock();
 * 
 * Pattern 2: Read-Heavy Access (fiber_rwlock)
 * --------------------------------------------
 * // Many readers
 * rwlock.rlock();
 * read_data();
 * rwlock.runlock();
 * 
 * // Occasional writer
 * rwlock.wlock();
 * write_data();
 * rwlock.wunlock();
 * 
 * Pattern 3: Conditional Locking
 * -------------------------------
 * if (lock.trylock()) {
 *     // Got lock, do work
 *     lock.unlock();
 * } else {
 *     // Couldn't get lock, do alternative
 * }
 * 
 * Pattern 4: Read-Modify-Write
 * -----------------------------
 * // Read first
 * rwlock.rlock();
 * bool need_update = check_condition();
 * rwlock.runlock();
 * 
 * // Write if needed
 * if (need_update) {
 *     rwlock.wlock();
 *     // Double-check after acquiring write lock
 *     if (check_condition()) {
 *         modify_data();
 *     }
 *     rwlock.wunlock();
 * }
 * 
 * ============================================================================
 * IMPORTANT WARNINGS
 * ============================================================================
 * 
 * 1. NEVER use fiber_lock or fiber_rwlock across threads:
 *    BAD:
 *      std::thread t1([&] { lock.lock();  ...; lock.unlock(); });
 *      std::thread t2([&] { lock.lock();  ...; lock.unlock(); });
 *    
 *    GOOD:
 *      go [&] { lock.lock(); ...; lock.unlock(); };
 *      go [&] { lock.lock(); ...; lock.unlock(); };
 *      acl::fiber::schedule();  // Single thread
 * 
 * 2. Don't mix fiber_lock with cross-thread operations:
 *    - Use fiber_mutex if any cross-thread access is needed
 *    - Mixing can cause undefined behavior
 * 
 * 3. Read-write lock upgrade not supported:
 *    - Cannot upgrade read lock to write lock
 *    - Must release read lock, then acquire write lock
 *    - Upgrading can cause deadlocks
 * 
 * 4. Not reentrant:
 *    - Same fiber cannot lock twice
 *    - Will cause deadlock
 *    - Keep lock hierarchies simple
 * 
 * ============================================================================
 */
