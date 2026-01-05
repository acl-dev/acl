#pragma once
#include "fiber_cpp_define.hpp"
#include <cstdlib>
#include <cassert>
#include <list>

struct ACL_FIBER_SEM;

namespace acl {

/**
 * fiber_sem_attr_t - Semaphore attribute flags
 * 
 * Defines the behavior mode for fiber semaphores.
 */
typedef enum {
	fiber_sem_t_sync  = 0,    // Synchronous mode
	fiber_sem_t_async = 0x01  // Asynchronous mode (default, more efficient)
} fiber_sem_attr_t;

/**
 * fiber_sem - A semaphore for fiber synchronization (SINGLE-THREAD ONLY)
 * 
 * ============================================================================
 * CRITICAL RESTRICTION: SINGLE-THREAD USE ONLY
 * ============================================================================
 * 
 * WARNING: fiber_sem can ONLY be used within a SINGLE THREAD!
 * - All wait() and post() calls must be from the SAME thread
 * - Cannot be used for cross-thread synchronization
 * - Designed for fiber-to-fiber synchronization within one thread
 * 
 * For cross-thread synchronization, use:
 * - fiber_tbox / fiber_tbox2 (thread-safe mailboxes)
 * - Standard thread synchronization primitives (std::mutex, etc.)
 * 
 * ============================================================================
 * 
 * This class implements a counting semaphore for coordinating multiple fibers
 * within the same thread. It provides a lightweight synchronization mechanism
 * optimized for fiber-based concurrency.
 * 
 * Key Features:
 * - Counting semaphore: Tracks available resources
 * - Blocking wait: Fibers block until resources are available
 * - Non-blocking trywait: Check availability without blocking
 * - Timeout support: Wait with timeout
 * - Async/sync modes: Different performance characteristics
 * 
 * Common Use Cases:
 * - Resource pooling (limiting concurrent access)
 * - Producer-consumer coordination (within same thread)
 * - Rate limiting
 * - Implementing other synchronization primitives
 * 
 * Thread Safety:
 * - NOT thread-safe: Must be used within a single thread only
 * - Safe for multiple fibers within the same thread
 * - Do NOT call wait()/post() from different threads
 */
class FIBER_CPP_API fiber_sem {
public:
	/**
	 * Constructor - Create a semaphore with specified initial count
	 * 
	 * @param max Initial count of the semaphore (number of available resources)
	 *            Can be 0 (no resources initially available)
	 * @param attr Semaphore attributes:
	 *             - fiber_sem_t_async: Asynchronous mode (default, more efficient)
	 *             - fiber_sem_t_sync: Synchronous mode
	 * 
	 * Example:
	 * ```cpp
	 * // Semaphore with 5 resources, async mode
	 * acl::fiber_sem sem(5);
	 * 
	 * // Semaphore with 0 resources (must post before wait succeeds)
	 * acl::fiber_sem sem2(0);
	 * ```
	 */
	explicit fiber_sem(size_t max, fiber_sem_attr_t attr = fiber_sem_t_async);
	
	/**
	 * Constructor - Create a semaphore with buffer size
	 * 
	 * @param max Initial count of the semaphore
	 * @param buf Buffer size for internal queue
	 */
	explicit fiber_sem(size_t max, size_t buf);
	
	/**
	 * Destructor - Destroys the semaphore
	 * 
	 * Warning: Destroying a semaphore while fibers are waiting on it
	 * results in undefined behavior.
	 */
	~fiber_sem();

	/**
	 * wait - Wait for the semaphore (decrement count)
	 * 
	 * Decrements the semaphore count. If the count is zero, the calling
	 * fiber blocks until the count becomes positive (via post()).
	 * 
	 * IMPORTANT: Must be called from the SAME thread as post()!
	 * 
	 * @param ms Timeout in milliseconds:
	 *           - ms < 0: Wait forever until semaphore is available (default: -1)
	 *           - ms == 0: Non-blocking (same as trywait())
	 *           - ms > 0: Wait up to ms milliseconds
	 * 
	 * @return int Returns:
	 *         - 0: Successfully acquired the semaphore
	 *         - -1: Timeout occurred (when ms >= 0)
	 * 
	 * Example:
	 * ```cpp
	 * acl::fiber_sem sem(3);  // 3 resources available
	 * 
	 * // Blocking wait
	 * if (sem.wait() == 0) {
	 *     // Got resource, do work
	 *     // ...
	 *     sem.post();  // Release resource
	 * }
	 * 
	 * // Wait with timeout
	 * if (sem.wait(5000) == 0) {  // Wait up to 5 seconds
	 *     // Got resource
	 *     sem.post();
	 * } else {
	 *     printf("Timeout\n");
	 * }
	 * ```
	 */
	int wait(int ms = -1);
	
	/**
	 * trywait - Try to acquire the semaphore without blocking
	 * 
	 * Attempts to decrement the semaphore count. If the count is zero,
	 * returns immediately with failure (does not block).
	 * 
	 * @return int Returns:
	 *         - 0: Successfully acquired the semaphore
	 *         - -1: Semaphore not available (count is zero)
	 * 
	 * Example:
	 * ```cpp
	 * acl::fiber_sem sem(5);
	 * 
	 * if (sem.trywait() == 0) {
	 *     // Got resource without blocking
	 *     // Do work...
	 *     sem.post();
	 * } else {
	 *     // Resource not available, do something else
	 * }
	 * ```
	 */
	int trywait();
	
	/**
	 * post - Release the semaphore (increment count)
	 * 
	 * Increments the semaphore count. If any fibers are waiting on wait(),
	 * one of them will be unblocked.
	 * 
	 * IMPORTANT: Must be called from the SAME thread as wait()!
	 * 
	 * @return int Returns:
	 *         - 0: Successfully posted
	 *         - -1: Error occurred
	 * 
	 * Example:
	 * ```cpp
	 * acl::fiber_sem sem(0);  // Start with 0 resources
	 * 
	 * // Producer fiber
	 * go [&sem] {
	 *     // Produce resource
	 *     sem.post();  // Signal availability
	 * };
	 * 
	 * // Consumer fiber
	 * go [&sem] {
	 *     sem.wait();  // Wait for resource
	 *     // Consume resource
	 * };
	 * ```
	 */
	int post();

	/**
	 * num - Get the current semaphore count
	 * 
	 * Returns the current count of available resources.
	 * 
	 * Note: This value may change immediately after calling this method
	 * if other fibers are waiting/posting. Use for monitoring/debugging,
	 * not for synchronization logic.
	 * 
	 * @return size_t The current semaphore count
	 * 
	 * Example:
	 * ```cpp
	 * printf("Available resources: %zu\n", sem.num());
	 * ```
	 */
	size_t num() const;

private:
	ACL_FIBER_SEM* sem_;  // Underlying C semaphore structure
	
	// Disable copy construction and assignment
	fiber_sem(const fiber_sem&);
	const fiber_sem& operator=(const fiber_sem&);
};

/**
 * fiber_sem_guard - RAII guard for automatic semaphore management
 * 
 * This class provides automatic semaphore acquisition and release using
 * RAII (Resource Acquisition Is Initialization) pattern. The semaphore
 * is acquired in the constructor and automatically released in the destructor.
 * 
 * Benefits:
 * - Exception-safe: Semaphore is released even if exceptions occur
 * - Prevents forgetting to call post()
 * - Cleaner code with automatic cleanup
 * 
 * Example:
 * ```cpp
 * acl::fiber_sem sem(3);
 * 
 * void process() {
 *     acl::fiber_sem_guard guard(sem);  // Acquires semaphore
 *     
 *     // Do work...
 *     // If exception occurs, semaphore is still released
 *     
 *     // Semaphore automatically released when guard goes out of scope
 * }
 * ```
 */
class FIBER_CPP_API fiber_sem_guard {
public:
	/**
	 * Constructor - Acquires the semaphore
	 * 
	 * Calls sem.wait() to acquire the semaphore. Blocks if not available.
	 * 
	 * @param sem The semaphore to guard
	 */
	explicit fiber_sem_guard(fiber_sem& sem) : sem_(sem) {
		(void) sem_.wait();
	}

	/**
	 * Destructor - Releases the semaphore
	 * 
	 * Automatically calls sem.post() to release the semaphore.
	 */
	~fiber_sem_guard() {
		sem_.post();
	}

private:
	fiber_sem& sem_;  // Reference to the guarded semaphore

	// Disable copy construction and assignment
	fiber_sem_guard(const fiber_sem_guard&);
	const fiber_sem_guard& operator=(const fiber_sem_guard&);
};

/**
 * fiber_sbox - A semaphore-based mailbox for passing pointers (SINGLE-THREAD ONLY)
 * 
 * ============================================================================
 * CRITICAL RESTRICTION: SINGLE-THREAD USE ONLY
 * ============================================================================
 * 
 * WARNING: fiber_sbox can ONLY be used within a SINGLE THREAD!
 * - Uses fiber_sem internally, which is single-thread only
 * - For cross-thread communication, use fiber_tbox instead
 * 
 * ============================================================================
 * 
 * This class combines a semaphore with a queue to create a mailbox for
 * passing pointers between fibers within the same thread. It uses fiber_sem
 * for efficient blocking/waking of fibers.
 * 
 * Differences from fiber_tbox:
 * - fiber_sbox: Uses fiber_sem (single-thread, more efficient within thread)
 * - fiber_tbox: Uses fiber_mutex + fiber_cond (thread-safe, cross-thread capable)
 * 
 * Template Parameter:
 * @tparam T The type of objects whose pointers will be transferred
 * 
 * The base box<T> defined in acl_cpp/stdlib/box.hpp, so you must include
 * box.hpp first before including fiber_sem.hpp
 */
template<typename T>
class fiber_sbox : public box<T> {
public:
	/**
	 * Constructor - Create a semaphore-based mailbox
	 * 
	 * @param free_obj If true, automatically delete unconsumed messages
	 *                 on destruction. Default: true
	 * @param async If true, use async semaphore mode (more efficient).
	 *              Default: true
	 */
	explicit fiber_sbox(bool free_obj = true, bool async = true)
	: sem_(0, async ? fiber_sem_t_async : fiber_sem_t_sync)
	, free_obj_(free_obj) {}

	/**
	 * Constructor - Create a mailbox with buffer size
	 * 
	 * @param buf Buffer size for the internal semaphore
	 * @param free_obj If true, automatically delete unconsumed messages
	 */
	explicit fiber_sbox(size_t buf, bool free_obj = true)
	: sem_(0, buf)
	, free_obj_(free_obj) {}

	/**
	 * Destructor - Destroys the mailbox and optionally cleans up messages
	 */
	~fiber_sbox() { clear(free_obj_); }

	/**
	 * push - Send a pointer to the mailbox
	 * 
	 * @param t Pointer to the object to send
	 * @param dummy Unused parameter (for interface compatibility)
	 * @return bool Always returns true
	 * @override
	 */
	bool push(T* t, bool dummy = false) {
		(void) dummy;
		sbox_.push_back(t);
		sem_.post();  // Signal that a message is available
		return true;
	}

	/**
	 * pop - Receive a pointer from the mailbox
	 * 
	 * @param ms Timeout in milliseconds (-1 = wait forever)
	 * @param found Optional output parameter indicating if a message was retrieved
	 * @return T* The retrieved pointer, or NULL on timeout
	 * @override
	 */
	T* pop(int ms, bool* found = NULL) {
		if (sem_.wait(ms) < 0) {
			if (found) {
				*found = false;
			}
			return NULL;
		}

		T* t = sbox_.front();
		sbox_.pop_front();
		if (found) {
			*found = true;
		}
		return t;
	}

	/**
	 * pop - Receive multiple pointers from the mailbox (batch operation)
	 * 
	 * @param out Output vector to store retrieved pointers
	 * @param max Maximum number of messages to retrieve (0 = unlimited)
	 * @param ms Timeout in milliseconds for the first message
	 * @return size_t Number of messages retrieved
	 * @override
	 */
	size_t pop(std::vector<T*>& out, size_t max, int ms) {
		size_t n = 0;
		while (true) {
			if (sem_.wait(ms) < 0) {
				return n;
			}

			T* t = sbox_.front();
			sbox_.pop_front();
			out.push_back(t);
			n++;
			if (max > 0 && n >= max) {
				return n;
			}
			ms = 0;  // No timeout for subsequent messages
		}
	}

	/**
	 * pop - Old interface for compatibility
	 * 
	 * @param found Optional output parameter
	 * @return T* The retrieved pointer
	 */
	T* pop(bool* found = NULL) {
		return pop(-1, found);
	}

	/**
	 * has_null - Check if NULL pointers are supported
	 * 
	 * @return bool Always returns true
	 * @override
	 */
	bool has_null() const {
		return true;
	}

	/**
	 * size - Get the current number of messages in the mailbox
	 * 
	 * @return size_t Number of pending messages
	 * @override
	 */
	size_t size() const {
		return sem_.num();
	}

private:
	fiber_sem     sem_;       // Semaphore for blocking/waking fibers
	std::list<T*> sbox_;      // Queue for storing message pointers
	bool          free_obj_;  // Whether to delete messages on destruction

	// Disable copy construction and assignment
	fiber_sbox(const fiber_sbox&);
	const fiber_sbox& operator=(const fiber_sbox&);

public:
	/**
	 * clear - Remove all unconsumed messages from the mailbox
	 * 
	 * @param free_obj If true, delete all message objects before clearing
	 */
	void clear(bool free_obj = false) {
		if (free_obj) {
			for (typename std::list<T*>::iterator it =
				sbox_.begin(); it != sbox_.end(); ++it) {

				delete *it;
			}
		}
		sbox_.clear();
	}
};

/**
 * fiber_sbox2 - A semaphore-based mailbox for passing objects by value (SINGLE-THREAD ONLY)
 * 
 * ============================================================================
 * CRITICAL RESTRICTION: SINGLE-THREAD USE ONLY
 * ============================================================================
 * 
 * WARNING: fiber_sbox2 can ONLY be used within a SINGLE THREAD!
 * - Uses fiber_sem internally, which is single-thread only
 * - For cross-thread communication, use fiber_tbox2 instead
 * 
 * ============================================================================
 * 
 * This class is similar to fiber_sbox but transfers objects by value instead
 * of pointers. It combines a semaphore with a circular buffer for efficient
 * message passing between fibers within the same thread.
 * 
 * Differences from fiber_tbox2:
 * - fiber_sbox2: Uses fiber_sem (single-thread, more efficient within thread)
 * - fiber_tbox2: Uses fiber_mutex + fiber_cond (thread-safe, cross-thread capable)
 * 
 * Template Parameter:
 * @tparam T The type of objects to be transferred by value
 */
template<typename T>
class fiber_sbox2 : public box2<T> {
public:
	/**
	 * Constructor - Create a semaphore-based mailbox for value types
	 * 
	 * @param async If true, use async semaphore mode (more efficient).
	 *              Default: true
	 */
	explicit fiber_sbox2(bool async = true)
	: sem_(0, async ? fiber_sem_t_async : fiber_sem_t_sync)
	, capacity_(1000)
	, off_curr_(0)
	, off_next_(0)
	{
		box_ = new T[capacity_];
	}

	/**
	 * Constructor - Create a mailbox with buffer size
	 * 
	 * @param buf Buffer size for the internal semaphore
	 */
	explicit fiber_sbox2(size_t buf)
	: sem_(0, buf)
	, capacity_(1000)
	, off_curr_(0)
	, off_next_(0)
	{
		box_ = new T[capacity_];
	}

	/**
	 * Destructor - Destroys the mailbox and all contained objects
	 */
	~fiber_sbox2() { delete []box_; }

	/**
	 * push - Send an object to the mailbox by value
	 * 
	 * @param t The object to send (copied or moved)
	 * @param dummy Unused parameter (for interface compatibility)
	 * @return bool Always returns true
	 * @override
	 */
	bool push(T t, bool dummy = false) {
		(void) dummy;

		// Check if buffer is full and needs compaction or growth
		if (off_next_ == capacity_) {
			if (off_curr_ >= 1000) {
				// Compact: Move unconsumed messages to the front
#if 1
				size_t n = 0;
				for (size_t i = off_curr_; i < off_next_; i++) {
					box_[n++] = box_[i];
				}
#else
				memmove(box_, box_ + off_curr_,
					(off_next_ - off_curr_) * sizeof(T*));
#endif

				off_next_ -= off_curr_;
				off_curr_ = 0;
			} else {
				// Grow the buffer by 10,000 elements
				size_t capacity = capacity_ + 10000;
				T* box = new T[capacity];
				for (size_t i = 0; i < capacity_; i++) {
#if __cplusplus >= 201103L || defined(USE_CPP11)
					box[i] = std::move(box_[i]);
#else
					box[i] = box_[i];
#endif
				}
				delete []box_;
				box_ = box;
				capacity_ = capacity;
			}
		}
		box_[off_next_++] = t;
		sem_.post();  // Signal that a message is available
		return true;
	}

	/**
	 * pop - Receive an object from the mailbox by value
	 * 
	 * @param t Output parameter to receive the object
	 * @param ms Timeout in milliseconds (-1 = wait forever)
	 * @return bool True if a message was retrieved, false on timeout
	 * @override
	 */
	bool pop(T& t, int ms = -1) {
		if (sem_.wait(ms) < 0) {
			return false;
		}

#if __cplusplus >= 201103L || defined(USE_CPP11)
		t = std::move(box_[off_curr_++]);
#else
		t = box_[off_curr_++];
#endif
		if (off_curr_ == off_next_) {
			if (off_curr_ > 0) {
				off_curr_ = off_next_ = 0;
			}
		}
		return true;
	}

	/**
	 * pop - Receive multiple objects from the mailbox (batch operation)
	 * 
	 * @param out Output vector to store retrieved objects
	 * @param max Maximum number of messages to retrieve (0 = unlimited)
	 * @param ms Timeout in milliseconds for the first message
	 * @return size_t Number of messages retrieved
	 * @override
	 */
	size_t pop(std::vector<T>& out, size_t max, int ms) {
		size_t n = 0;
		while (true) {
			if (sem_.wait(ms) < 0) {
				return n;
			}

#if __cplusplus >= 201103L || defined(USE_CPP11)
			out.emplace_back(std::move(box_[off_curr_++]));
#else
			out.push_back(box_[off_curr_++]);
#endif
			n++;
			if (off_curr_ == off_next_) {
				if (off_curr_ > 0) {
					off_curr_ = off_next_ = 0;
				}
			}
			if (max > 0 && n >= max) {
				return n;
			}
			ms = 0;  // No timeout for subsequent messages
		}
	}

	/**
	 * size - Get the current number of messages in the mailbox
	 * 
	 * @return size_t Number of pending messages
	 * @override
	 */
	size_t size() const {
		return off_next_ - off_curr_;
	}

	/**
	 * has_null - Check if NULL messages are supported
	 * 
	 * @return bool Always returns true for interface compatibility
	 * @override
	 */
	bool has_null() const {
		return true;
	}

private:
	fiber_sem    sem_;        // Semaphore for blocking/waking fibers
	T*           box_;        // Internal array for storing messages
	size_t       capacity_;   // Current capacity of the array
	size_t       off_curr_;   // Offset of the first unconsumed message
	size_t       off_next_;   // Offset where the next message will be stored

	// Disable copy construction and assignment
	fiber_sbox2(const fiber_sbox2&);
	const fiber_sbox2& operator=(const fiber_sbox2&);
};

} // namespace acl

/**
 * ============================================================================
 * USAGE EXAMPLES
 * ============================================================================
 * 
 * Example 1: Resource Pool with fiber_sem
 * ----------------------------------------
 * #include "fiber/fiber_sem.hpp"
 * #include "fiber/go_fiber.hpp"
 * 
 * acl::fiber_sem pool_sem(3);  // 3 resources available
 * 
 * void worker(int id) {
 *     printf("Worker %d: waiting for resource\n", id);
 *     
 *     if (pool_sem.wait() == 0) {
 *         printf("Worker %d: got resource\n", id);
 *         
 *         // Use resource
 *         acl::fiber::delay(1000);
 *         
 *         printf("Worker %d: releasing resource\n", id);
 *         pool_sem.post();
 *     }
 * }
 * 
 * int main() {
 *     // Start 10 workers, but only 3 can run concurrently
 *     for (int i = 0; i < 10; i++) {
 *         go[i] { worker(i); };
 *     }
 *     
 *     acl::fiber::schedule();
 *     return 0;
 * }
 * 
 * ============================================================================
 * Example 2: Producer-Consumer with fiber_sbox (SINGLE-THREAD)
 * ============================================================================
 * 
 * struct Task {
 *     int id;
 *     std::string data;
 * };
 * 
 * acl::fiber_sbox<Task> mailbox;
 * 
 * void producer() {
 *     for (int i = 0; i < 10; i++) {
 *         Task* task = new Task{i, "data"};
 *         mailbox.push(task);
 *         printf("Produced task %d\n", i);
 *         acl::fiber::delay(100);
 *     }
 *     mailbox.push(nullptr);  // Termination signal
 * }
 * 
 * void consumer() {
 *     while (true) {
 *         Task* task = mailbox.pop();
 *         if (!task) {
 *             printf("Termination signal received\n");
 *             break;
 *         }
 *         
 *         printf("Consumed task %d\n", task->id);
 *         delete task;
 *     }
 * }
 * 
 * int main() {
 *     // IMPORTANT: Both fibers run in the SAME thread
 *     go producer;
 *     go consumer;
 *     
 *     acl::fiber::schedule();  // Single-threaded fiber scheduler
 *     return 0;
 * }
 * 
 * ============================================================================
 * Example 3: RAII Guard with fiber_sem_guard
 * ============================================================================
 * 
 * acl::fiber_sem resource_sem(5);
 * 
 * void process_with_guard() {
 *     // Semaphore automatically acquired
 *     acl::fiber_sem_guard guard(resource_sem);
 *     
 *     // Do work...
 *     printf("Processing...\n");
 *     acl::fiber::delay(500);
 *     
 *     // Semaphore automatically released when guard goes out of scope
 *     // Even if exceptions occur!
 * }
 * 
 * ============================================================================
 * Example 4: Rate Limiting with fiber_sem
 * ============================================================================
 * 
 * acl::fiber_sem rate_limiter(10);  // 10 requests per batch
 * 
 * void make_request(int id) {
 *     if (rate_limiter.trywait() == 0) {
 *         printf("Request %d: processing\n", id);
 *         // Process request...
 *         acl::fiber::delay(100);
 *         rate_limiter.post();
 *     } else {
 *         printf("Request %d: rate limited, waiting\n", id);
 *         rate_limiter.wait();  // Block until available
 *         printf("Request %d: processing\n", id);
 *         // Process request...
 *         rate_limiter.post();
 *     }
 * }
 * 
 * ============================================================================
 * IMPORTANT NOTES
 * ============================================================================
 * 
 * 1. SINGLE-THREAD RESTRICTION:
 *    - fiber_sem, fiber_sbox, fiber_sbox2 are SINGLE-THREAD ONLY
 *    - All wait() and post() calls must be from the SAME thread
 *    - DO NOT use across multiple threads
 * 
 * 2. For Cross-Thread Communication:
 *    - Use fiber_tbox (for pointers, thread-safe)
 *    - Use fiber_tbox2 (for values, thread-safe)
 *    - Use standard thread primitives (std::mutex, std::condition_variable)
 * 
 * 3. When to Use fiber_sem vs fiber_tbox:
 *    Within Single Thread:
 *      - fiber_sem/fiber_sbox: More efficient, lower overhead
 *      - fiber_tbox: Also works, but slightly more overhead
 *    
 *    Across Threads:
 *      - fiber_sem/fiber_sbox: CANNOT be used (undefined behavior)
 *      - fiber_tbox: MUST be used (thread-safe)
 * 
 * 4. Semaphore Patterns:
 *    - Resource Pool: Initialize with N resources, wait/post around usage
 *    - Signaling: Initialize with 0, post to signal, wait to receive
 *    - Rate Limiting: Initialize with limit, trywait for non-blocking check
 * 
 * 5. RAII Guard Benefits:
 *    - Exception-safe resource management
 *    - Prevents forgetting to release semaphore
 *    - Cleaner, more maintainable code
 * 
 * 6. Performance:
 *    - fiber_sem is optimized for single-thread fiber switching
 *    - Async mode (default) is more efficient than sync mode
 *    - Lower overhead than mutex-based synchronization within same thread
 * 
 * ============================================================================
 * COMPARISON TABLE
 * ============================================================================
 * 
 * +------------------+------------------+------------------+------------------+
 * | Feature          | fiber_sem        | fiber_sbox       | fiber_tbox       |
 * +------------------+------------------+------------------+------------------+
 * | Thread Safety    | Single-thread    | Single-thread    | Multi-thread     |
 * | Use Case         | Counting sem     | Mailbox (ptr)    | Mailbox (ptr)    |
 * | Synchronization  | fiber_sem        | fiber_sem        | mutex + cond     |
 * | Performance      | Fastest          | Fast             | Good             |
 * | Cross-thread     | NO               | NO               | YES              |
 * | Overhead         | Lowest           | Low              | Medium           |
 * +------------------+------------------+------------------+------------------+
 * 
 * +------------------+------------------+------------------+
 * | Feature          | fiber_sbox2      | fiber_tbox2      |
 * +------------------+------------------+------------------+
 * | Thread Safety    | Single-thread    | Multi-thread     |
 * | Use Case         | Mailbox (value)  | Mailbox (value)  |
 * | Synchronization  | fiber_sem        | mutex + cond     |
 * | Performance      | Fast             | Good             |
 * | Cross-thread     | NO               | YES              |
 * | Overhead         | Low              | Medium           |
 * +------------------+------------------+------------------+
 * 
 * ============================================================================
 */
