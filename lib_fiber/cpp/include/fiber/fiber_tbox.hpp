#pragma once
#include "fiber_cpp_define.hpp"
#include <list>
#include <cstdlib>
#include "fiber.hpp"
#include "fiber_mutex.hpp"
#include "fiber_cond.hpp"

namespace acl {

/**
 * fiber_tbox - A thread-safe mailbox for message passing between fibers/threads
 * 
 * Used for message communication between coroutines, threads, and between
 * coroutines and threads. Implemented through fiber condition variables and
 * fiber mutex for efficient synchronization.
 * 
 * Key Features:
 * - Thread-safe: Can be used across multiple fibers and threads
 * - Blocking operations: pop() blocks until a message is available
 * - Timeout support: Optional timeout for pop() operations
 * - Null message support: Can transfer null pointers as valid messages
 * - Automatic cleanup: Optional automatic deletion of unconsumed messages
 * - FIFO ordering: Messages are delivered in first-in-first-out order
 * - Batch operations: Can pop multiple messages at once
 * 
 * Use Cases:
 * - Producer-consumer patterns
 * - Task queues for fiber pools
 * - Inter-fiber communication
 * - Cross-thread message passing
 * - Event notification systems
 * 
 * Template Parameter:
 * @tparam T The type of objects to be transferred (must be pointer type)
 * 
 * Basic Sample:
 *
 * class myobj {
 * public:
 *     myobj() {}
 *     ~myobj() {}
 *
 *     void test() { printf("hello world\r\n"); }
 * };
 *
 * acl::fiber_tbox<myobj> mailbox;
 *
 * void producer_fiber() {
 *     myobj* o = new myobj;
 *     mailbox.push(o);  // Send message
 * }
 *
 * void consumer_fiber() {
 *     myobj* o = mailbox.pop();  // Receive message (blocks if empty)
 *     o->test();
 *     delete o;
 * }
 * 
 * Thread Safety:
 * - Safe to call push() from multiple fibers/threads concurrently
 * - Safe to call pop() from multiple fibers/threads concurrently
 * - Internal synchronization using fiber_mutex and fiber_cond
 */

// The base box<T> defined in acl_cpp/stdlib/box.hpp, so you must include
// box.hpp first before including fiber_tbox.hpp
template<typename T>
class fiber_tbox : public box<T> {
public:
	/**
	 * Constructor - Create a new fiber mailbox
	 * 
	 * @param free_obj {bool} If true, automatically delete unconsumed
	 *  message objects when fiber_tbox is destroyed. Set to false if
	 *  messages are managed externally or are not dynamically allocated.
	 *  Default: true
	 * 
	 * Example:
	 * ```cpp
	 * // Mailbox will delete unconsumed messages on destruction
	 * acl::fiber_tbox<MyClass> box1(true);
	 * 
	 * // Mailbox will NOT delete unconsumed messages
	 * acl::fiber_tbox<MyClass> box2(false);
	 * ```
	 */
	explicit fiber_tbox(bool free_obj = true) : size_(0), free_obj_(free_obj) {}

	/**
	 * Destructor - Destroys the mailbox and optionally cleans up messages
	 * 
	 * If free_obj was set to true in the constructor, all unconsumed
	 * messages in the queue will be deleted. Otherwise, messages are
	 * left as-is (potential memory leak if not handled externally).
	 */
	~fiber_tbox() {
		clear(free_obj_);
	}

	/**
	 * clear - Remove all unconsumed messages from the mailbox
	 * 
	 * Clean up the unconsumed message objects in the message queue.
	 * 
	 * @param free_obj {bool} If true, delete all message objects in the
	 *  queue before clearing. If false, just remove them from the queue
	 *  without deleting (caller is responsible for memory management).
	 *  Default: false
	 * 
	 * Warning: Calling clear(true) on messages that are not dynamically
	 * allocated or are managed elsewhere will cause undefined behavior.
	 * 
	 * Example:
	 * ```cpp
	 * fiber_tbox<MyClass> box;
	 * // ... push some messages ...
	 * box.clear(true);  // Delete all unconsumed messages
	 * ```
	 */
	void clear(bool free_obj = false) {
		if (free_obj) {
			for (typename std::list<T*>::iterator it =
				tbox_.begin(); it != tbox_.end(); ++it) {

				delete *it;
			}
		}
		tbox_.clear();
	}

	/**
	 * push - Send a message object to the mailbox
	 * 
	 * Adds a message to the end of the queue and notifies one waiting
	 * consumer (if any). This operation is thread-safe and can be called
	 * from multiple fibers/threads concurrently.
	 * 
	 * @param t {T*} The object pointer to be sent to the queue. Can be
	 *  NULL if null messages are intentional.
	 * @param notify_first {bool} Controls the order of notification and unlock:
	 *  - true: Notify first, then unlock (safer for short-lived mailboxes)
	 *  - false: Unlock first, then notify (more efficient for long-lived mailboxes)
	 *  
	 *  Detailed explanation:
	 *  - When true: Ensures the consumer is notified while the lock is held,
	 *    preventing race conditions if the mailbox is destroyed immediately
	 *    after pop() returns. Use this when the mailbox lifetime is short.
	 *  - When false: Releases the lock before notifying, allowing better
	 *    concurrency. Use this when the mailbox has a long lifetime.
	 *  
	 *  Default: true (safer option)
	 * 
	 * @return {bool} Always returns true (aborts on internal errors)
	 * @override
	 * 
	 * Example:
	 * ```cpp
	 * fiber_tbox<Task> mailbox;
	 * 
	 * // Long-lived mailbox (better performance)
	 * mailbox.push(new Task(), false);
	 * 
	 * // Short-lived mailbox (safer)
	 * fiber_tbox<Result> temp_box;
	 * temp_box.push(result, true);
	 * Result* r = temp_box.pop();
	 * // temp_box destroyed here - notify_first=true prevents issues
	 * ```
	 */
	bool push(T* t, bool notify_first = true) {
		if (! mutex_.lock()) { abort(); }

		tbox_.push_back(t);
		size_++;

		if (notify_first) {
			// Notify while holding lock (safer for short-lived mailboxes)
			if (! cond_.notify()) { abort(); }
			if (! mutex_.unlock()) { abort(); }
		} else {
			// Unlock before notify (better performance for long-lived mailboxes)
			if (! mutex_.unlock()) { abort(); }
			if (! cond_.notify()) { abort(); }
		}

		return true;
	}

	/**
	 * pop - Receive a message object from the mailbox
	 * 
	 * Retrieves and removes the first message from the queue. If the queue
	 * is empty, this method blocks until a message arrives or timeout occurs.
	 * 
	 * @param ms {int} Timeout in milliseconds:
	 *  - ms < 0: Wait forever until a message arrives (default: -1)
	 *  - ms == 0: Return immediately (non-blocking check)
	 *  - ms > 0: Wait up to ms milliseconds for a message
	 * 
	 * @param found {bool*} Optional output parameter to indicate whether
	 *  a message was actually retrieved. This is crucial for distinguishing
	 *  between:
	 *  - A valid NULL message (found=true, return=NULL)
	 *  - A timeout with no message (found=false, return=NULL)
	 *  If NULL, this parameter is ignored.
	 * 
	 * @return {T*} The retrieved message pointer, or NULL in these cases:
	 *  1. A valid NULL message was pushed (check found=true)
	 *  2. Timeout occurred with ms >= 0 (check found=false)
	 *  3. The fiber was killed (check found=false)
	 * 
	 * Return Value Interpretation:
	 * - Non-NULL: A valid message object was retrieved
	 * - NULL with found=true: A NULL message was intentionally pushed
	 * - NULL with found=false: Timeout or fiber killed, no message retrieved
	 * - NULL with ms=-1: A NULL message was retrieved (no timeout possible)
	 * 
	 * @override
	 * 
	 * Example 1: Blocking pop (wait forever)
	 * ```cpp
	 * fiber_tbox<Task> mailbox;
	 * Task* task = mailbox.pop();  // Blocks until message arrives
	 * if (task) {
	 *     task->execute();
	 *     delete task;
	 * }
	 * ```
	 * 
	 * Example 2: Pop with timeout
	 * ```cpp
	 * bool found;
	 * Task* task = mailbox.pop(5000, &found);  // Wait up to 5 seconds
	 * if (found) {
	 *     if (task) {
	 *         task->execute();
	 *         delete task;
	 *     } else {
	 *         printf("Received NULL message\n");
	 *     }
	 * } else {
	 *     printf("Timeout, no message received\n");
	 * }
	 * ```
	 * 
	 * Example 3: Non-blocking pop
	 * ```cpp
	 * bool found;
	 * Task* task = mailbox.pop(0, &found);  // Check immediately
	 * if (found && task) {
	 *     task->execute();
	 * }
	 * ```
	 */
	T* pop(int ms = -1, bool* found = NULL) {
		bool found_flag;

		if (! mutex_.lock()) { abort(); }

		while (true) {
			T* t = peek(found_flag);
			if (found_flag) {
				if (! mutex_.unlock()) { abort(); }
				if (found) {
					*found = found_flag;
				}
				return t;
			}

			// The calling order: wait should be called first
			// before checking wait_ms.
			if (! cond_.wait(mutex_, ms) && ms >= 0) {
				// Timeout occurred
				if (! mutex_.unlock()) { abort(); }
				if (found) {
					*found = false;
				}
				return NULL;
			}

			if (fiber::self_killed()) {
				// Fiber is being killed, abort waiting
				if (! mutex_.unlock()) { abort(); }
				if (found) {
					*found = false;
				}
				return NULL;
			}
		}
	}

	/**
	 * pop - Receive multiple message objects from the mailbox (batch operation)
	 * 
	 * Retrieves multiple messages from the queue in a single operation.
	 * This is more efficient than calling pop() multiple times when you
	 * need to process messages in batches.
	 * 
	 * Behavior:
	 * 1. If messages are available, retrieve up to 'max' messages immediately
	 * 2. If queue is empty, wait for at least one message (up to timeout)
	 * 3. Once at least one message is retrieved, get all available messages
	 *    up to 'max' without further waiting
	 * 
	 * @param out {std::vector<T*>&} Output vector to store retrieved messages.
	 *  Messages are appended to this vector (not cleared first).
	 * @param max {size_t} Maximum number of messages to retrieve:
	 *  - 0: Retrieve all available messages (no limit)
	 *  - >0: Retrieve at most this many messages
	 * @param ms {int} Timeout in milliseconds (same semantics as single pop):
	 *  - ms < 0: Wait forever for at least one message
	 *  - ms == 0: Return immediately with available messages
	 *  - ms > 0: Wait up to ms milliseconds for at least one message
	 * 
	 * @return {size_t} The number of messages retrieved (0 if timeout/killed)
	 * @override
	 * 
	 * Example 1: Batch processing
	 * ```cpp
	 * fiber_tbox<Task> mailbox;
	 * std::vector<Task*> tasks;
	 * 
	 * // Get up to 10 tasks, wait up to 1 second
	 * size_t n = mailbox.pop(tasks, 10, 1000);
	 * printf("Retrieved %zu tasks\n", n);
	 * 
	 * for (Task* task : tasks) {
	 *     task->execute();
	 *     delete task;
	 * }
	 * ```
	 * 
	 * Example 2: Drain all messages
	 * ```cpp
	 * std::vector<Task*> tasks;
	 * size_t n = mailbox.pop(tasks, 0, 0);  // Get all available, no wait
	 * printf("Drained %zu tasks\n", n);
	 * ```
	 */
	size_t pop(std::vector<T*>& out, size_t max, int ms) {
		size_t n = 0;
		bool found_flag;

		if (! mutex_.lock()) { abort(); }

		while (true) {
			T* t = peek(found_flag);
			if (found_flag) {
				out.push_back(t);
				n++;
				if (max > 0 && n >= max) {
					// Reached max limit, return
					if (! mutex_.unlock()) { abort(); }
					return n;
				}
				continue;  // Try to get more messages
			}

			if (n > 0) {
				// Got at least one message, return what we have
				if (! mutex_.unlock()) { abort(); }
				return n;
			}

			// No messages yet, wait for at least one
			if (! cond_.wait(mutex_, ms) && ms >= 0) {
				// Timeout occurred
				if (! mutex_.unlock()) { abort(); }
				return n;
			}

			if (fiber::self_killed()) {
				// Fiber is being killed
				if (! mutex_.unlock()) { abort(); }
				return n;
			}
		}
	}

	/**
	 * has_null - Check if this mailbox supports NULL messages
	 * 
	 * fiber_tbox supports transferring NULL message objects.
	 * This allows NULL to be used as a valid signal/message.
	 * 
	 * @return {bool} Always returns true for fiber_tbox
	 * @override
	 */
	bool has_null() const {
		return true;
	}

	/**
	 * size - Get the current number of messages in the mailbox
	 * 
	 * Return the current number of messages in the message queue.
	 * 
	 * Note: This value may change immediately after calling this method
	 * if other fibers/threads are pushing or popping concurrently.
	 * Use this for monitoring/debugging, not for synchronization logic.
	 * 
	 * @return {size_t} The number of messages currently in the queue
	 * 
	 * Example:
	 * ```cpp
	 * printf("Mailbox has %zu pending messages\n", mailbox.size());
	 * ```
	 */
	size_t size() const {
		return size_;
	}

public:
	/**
	 * lock - Manually lock the internal mutex
	 * 
	 * Acquires the internal mutex. Use this for advanced scenarios where
	 * you need to perform multiple operations atomically. Always pair with
	 * unlock() and be careful to avoid deadlocks.
	 * 
	 * Warning: Improper use can cause deadlocks. Prefer using the normal
	 * push/pop methods which handle locking automatically.
	 */
	void lock() {
		if (! mutex_.lock()) {
			abort();
		}
	}

	/**
	 * unlock - Manually unlock the internal mutex
	 * 
	 * Releases the internal mutex previously acquired by lock().
	 * Must be called after lock() to avoid deadlocks.
	 */
	void unlock() {
		if (! mutex_.unlock()) {
			abort();
		}
	}

private:
	// Disable copy construction and assignment
	fiber_tbox(const fiber_tbox&) {}
	const fiber_tbox& operator=(const fiber_tbox&);

private:
	std::list<T*> tbox_;      // Internal queue for storing messages
	size_t        size_;      // Current number of messages in queue
	bool          free_obj_;  // Whether to delete messages on destruction
	fiber_mutex   mutex_;     // Mutex for thread-safe access
	fiber_cond    cond_;      // Condition variable for blocking/notification

	/**
	 * peek - Internal method to retrieve the first message without blocking
	 * 
	 * @param found_flag {bool&} Output parameter set to true if a message
	 *  was found, false if the queue is empty
	 * @return {T*} The first message in the queue, or NULL if empty
	 */
	T* peek(bool& found_flag) {
		typename std::list<T*>::iterator it = tbox_.begin();
		if (it == tbox_.end()) {
			found_flag = false;
			return NULL;
		}
		found_flag = true;
		size_--;
		T* t = *it;
		tbox_.erase(it);
		return t;
	}
};

} // namespace acl

/**
 * ============================================================================
 * USAGE EXAMPLES
 * ============================================================================
 * 
 * Example 1: Simple Producer-Consumer
 * ------------------------------------
 * #include "fiber/fiber_tbox.hpp"
 * #include "fiber/go_fiber.hpp"
 * 
 * struct Task {
 *     int id;
 *     std::string data;
 * };
 * 
 * acl::fiber_tbox<Task> mailbox;
 * 
 * void producer() {
 *     for (int i = 0; i < 10; i++) {
 *         Task* task = new Task{i, "data"};
 *         mailbox.push(task);
 *         printf("Produced task %d\n", i);
 *         acl::fiber::delay(100);
 *     }
 *     
 *     // Send termination signal (NULL message)
 *     mailbox.push(nullptr);
 * }
 * 
 * void consumer() {
 *     while (true) {
 *         Task* task = mailbox.pop();
 *         if (!task) {
 *             printf("Received termination signal\n");
 *             break;
 *         }
 *         
 *         printf("Consumed task %d: %s\n", task->id, task->data.c_str());
 *         delete task;
 *     }
 * }
 * 
 * int main() {
 *     go producer;
 *     go consumer;
 *     acl::fiber::schedule();
 *     return 0;
 * }
 * 
 * ============================================================================
 * Example 2: Multiple Producers and Consumers
 * ============================================================================
 * 
 * acl::fiber_tbox<Task> mailbox;
 * std::atomic<int> task_id(0);
 * 
 * void producer(int id) {
 *     for (int i = 0; i < 5; i++) {
 *         Task* task = new Task{task_id++, "producer " + std::to_string(id)};
 *         mailbox.push(task);
 *         acl::fiber::delay(rand() % 200);
 *     }
 * }
 * 
 * void consumer(int id) {
 *     while (true) {
 *         bool found;
 *         Task* task = mailbox.pop(5000, &found);  // 5 second timeout
 *         
 *         if (!found) {
 *             printf("Consumer %d: timeout\n", id);
 *             break;
 *         }
 *         
 *         if (task) {
 *             printf("Consumer %d: task %d from %s\n",
 *                    id, task->id, task->data.c_str());
 *             delete task;
 *         }
 *     }
 * }
 * 
 * int main() {
 *     // Start 3 producers and 2 consumers
 *     for (int i = 0; i < 3; i++) {
 *         go[i] { producer(i); };
 *     }
 *     for (int i = 0; i < 2; i++) {
 *         go[i] { consumer(i); };
 *     }
 *     
 *     acl::fiber::schedule();
 *     return 0;
 * }
 * 
 * ============================================================================
 * Example 3: Batch Processing
 * ============================================================================
 * 
 * acl::fiber_tbox<Task> mailbox;
 * 
 * void batch_producer() {
 *     for (int i = 0; i < 100; i++) {
 *         mailbox.push(new Task{i, "data"});
 *         if (i % 10 == 0) {
 *             acl::fiber::delay(50);  // Burst of 10, then pause
 *         }
 *     }
 *     mailbox.push(nullptr);  // Termination signal
 * }
 * 
 * void batch_consumer() {
 *     std::vector<Task*> batch;
 *     
 *     while (true) {
 *         batch.clear();
 *         
 *         // Get up to 20 tasks, wait up to 1 second
 *         size_t n = mailbox.pop(batch, 20, 1000);
 *         
 *         if (n == 0) {
 *             printf("No tasks received, exiting\n");
 *             break;
 *         }
 *         
 *         printf("Processing batch of %zu tasks\n", n);
 *         
 *         for (Task* task : batch) {
 *             if (!task) {
 *                 printf("Termination signal received\n");
 *                 return;
 *             }
 *             
 *             // Process task
 *             printf("Task %d\n", task->id);
 *             delete task;
 *         }
 *     }
 * }
 * 
 * int main() {
 *     go batch_producer;
 *     go batch_consumer;
 *     acl::fiber::schedule();
 *     return 0;
 * }
 * 
 * ============================================================================
 * Example 4: Request-Response Pattern
 * ============================================================================
 * 
 * struct Request {
 *     int id;
 *     acl::fiber_tbox<Response>* response_box;
 * };
 * 
 * struct Response {
 *     int id;
 *     std::string result;
 * };
 * 
 * acl::fiber_tbox<Request> request_box;
 * 
 * void server() {
 *     while (true) {
 *         Request* req = request_box.pop();
 *         if (!req) break;
 *         
 *         // Process request
 *         Response* resp = new Response{req->id, "processed"};
 *         
 *         // Send response back
 *         req->response_box->push(resp);
 *         delete req;
 *     }
 * }
 * 
 * void client(int id) {
 *     acl::fiber_tbox<Response> response_box;
 *     
 *     // Send request
 *     Request* req = new Request{id, &response_box};
 *     request_box.push(req);
 *     
 *     // Wait for response (with timeout)
 *     bool found;
 *     Response* resp = response_box.pop(5000, &found);
 *     
 *     if (found && resp) {
 *         printf("Client %d: got response: %s\n", id, resp->result.c_str());
 *         delete resp;
 *     } else {
 *         printf("Client %d: timeout\n", id);
 *     }
 * }
 * 
 * int main() {
 *     go server;
 *     
 *     for (int i = 0; i < 5; i++) {
 *         go[i] { client(i); };
 *     }
 *     
 *     acl::fiber::schedule();
 *     return 0;
 * }
 * 
 * ============================================================================
 * BEST PRACTICES
 * ============================================================================
 * 
 * 1. Memory Management:
 *    - Always delete messages after consuming them (unless managed externally)
 *    - Use free_obj=true in constructor for automatic cleanup
 *    - Consider using smart pointers for automatic memory management
 * 
 * 2. NULL Messages:
 *    - Use NULL as a termination signal in producer-consumer patterns
 *    - Always check the 'found' parameter when using timeouts
 *    - Document whether your protocol allows NULL messages
 * 
 * 3. Timeout Usage:
 *    - Use -1 for blocking operations (wait forever)
 *    - Use 0 for non-blocking checks
 *    - Use positive values for bounded waiting
 *    - Always check return value and 'found' parameter with timeouts
 * 
 * 4. notify_first Parameter:
 *    - Use true (default) for short-lived mailboxes
 *    - Use false for long-lived mailboxes (better performance)
 *    - When in doubt, use true (safer)
 * 
 * 5. Batch Operations:
 *    - Use pop(vector, max, ms) for high-throughput scenarios
 *    - Reduces context switches and lock contention
 *    - Set max=0 to drain all available messages
 * 
 * 6. Error Handling:
 *    - Check for NULL returns when using timeouts
 *    - Handle fiber termination (fiber::self_killed())
 *    - Be prepared for spurious wakeups
 * 
 * 7. Performance:
 *    - Minimize time spent holding locks
 *    - Use batch operations when processing multiple messages
 *    - Consider using multiple mailboxes for different priorities
 * 
 * 8. Deadlock Prevention:
 *    - Avoid holding locks when calling external code
 *    - Don't call pop() while holding another mailbox's lock
 *    - Use timeouts to detect potential deadlocks
 * 
 * ============================================================================
 * COMMON PATTERNS
 * ============================================================================
 * 
 * Pattern 1: Termination Signal
 * ------------------------------
 * // Producer
 * mailbox.push(nullptr);  // Signal termination
 * 
 * // Consumer
 * while (true) {
 *     T* msg = mailbox.pop();
 *     if (!msg) break;  // Termination detected
 *     process(msg);
 *     delete msg;
 * }
 * 
 * Pattern 2: Timeout with Retry
 * ------------------------------
 * int retries = 3;
 * bool found;
 * T* msg = nullptr;
 * 
 * while (retries-- > 0) {
 *     msg = mailbox.pop(1000, &found);
 *     if (found) break;
 *     printf("Timeout, retrying...\n");
 * }
 * 
 * Pattern 3: Non-blocking Check
 * ------------------------------
 * bool found;
 * T* msg = mailbox.pop(0, &found);
 * if (found && msg) {
 *     process(msg);
 * } else {
 *     // Do other work
 * }
 * 
 * Pattern 4: Work Stealing
 * -------------------------
 * // Multiple consumers try to steal work
 * void worker() {
 *     while (running) {
 *         bool found;
 *         Task* task = mailbox.pop(100, &found);
 *         if (found && task) {
 *             process(task);
 *         } else {
 *             // Try another mailbox or do other work
 *         }
 *     }
 * }
 * 
 * ============================================================================
 */
