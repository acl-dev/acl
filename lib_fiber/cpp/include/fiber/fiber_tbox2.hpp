#pragma once
#include "fiber_cpp_define.hpp"
#include <list>
#include <vector>
#include <cstdlib>
#include "fiber.hpp"
#include "fiber_mutex.hpp"
#include "fiber_cond.hpp"

namespace acl {

/**
 * Used for message communication between coroutines, threads, and between
 * coroutines, implemented through coroutine condition variables and
 * coroutine lock.
 *
 * Sample:
 *
 * class myobj {
 * public:
 *     myobj() {}
 *     ~myobj() {}
 *
 *     void test() { printf("hello world\r\n"); }
 * };
 *
 * acl::fiber_tbox2<myobj> tbox;
 *
 * void thread_producer() {
 *     myobj o;
 *     tbox.push(o);
 * }
 *
 * void thread_consumer() {
 *     myobj o;

 *     if (tbox.pop(o)) {
 *         o.test();
 *     }
 * }
 */

// The fiber_tbox2 has an object copying process in push/pop which is suitable
// for transfering the object managed by std::shared_ptr.

// The base box2<T> defined in acl_cpp/stdlib/box.hpp, so you must include
// box.hpp first before including fiber_tbox.hpp
template<typename T>
class fiber_tbox2 : public box2<T> {
public:
	/**
	 * Constructor - Create a new fiber mailbox with value semantics
	 * 
	 * Initializes the mailbox with an initial capacity of 10,000 elements.
	 * The capacity will automatically grow by 10,000 elements when needed.
	 * 
	 * Note: Unlike fiber_tbox, fiber_tbox2 does not have a free_obj parameter
	 * because objects are managed by value semantics (automatic cleanup).
	 */
	fiber_tbox2() : capacity_(10000) , off_curr_(0) , off_next_(0) {
		box_ = new T[capacity_];
	}

	/**
	 * Destructor - Destroys the mailbox and all contained objects
	 * 
	 * All unconsumed messages are automatically destroyed through their
	 * destructors. No manual cleanup is needed.
	 */
	~fiber_tbox2() { delete []box_; }

	/**
	 * clear - Remove all unconsumed messages from the mailbox
	 * 
	 * Clean up unconsumed messages in the message queue.
	 * Resets the internal offsets, effectively clearing the queue.
	 * 
	 * Note: Objects are not explicitly destroyed here; they will be
	 * overwritten when new messages are pushed. The destructors will
	 * be called when the mailbox is destroyed or when objects are
	 * overwritten.
	 * 
	 * Example:
	 * ```cpp
	 * fiber_tbox2<Task> box;
	 * // ... push some messages ...
	 * box.clear();  // Clear all unconsumed messages
	 * ```
	 */
	void clear() {
		off_curr_ = off_next_ = 0;
	}

	/**
	 * push - Send a message object to the mailbox by value
	 * 
	 * Adds a message to the end of the queue by copying or moving (C++11+)
	 * the object. The mailbox automatically grows if capacity is reached.
	 * 
	 * Growth Strategy:
	 * 1. If buffer is full and consumed space >= 10,000: Compact the buffer
	 * 2. Otherwise: Grow capacity by 10,000 elements
	 * 
	 * @param t {T} The object to be transferred BY VALUE. In C++11+, the
	 *  object is moved if possible (using std::move), otherwise copied.
	 *  The original object can be safely destroyed after push() returns.
	 * 
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
	 * Example 1: Value types
	 * ```cpp
	 * fiber_tbox2<int> mailbox;
	 * mailbox.push(42);  // Integer is copied
	 * ```
	 * 
	 * Example 2: Smart pointers (recommended)
	 * ```cpp
	 * fiber_tbox2<std::shared_ptr<Task>> mailbox;
	 * auto task = std::make_shared<Task>();
	 * mailbox.push(task);  // Shared pointer copied, ref count++
	 * // task can still be used here
	 * ```
	 * 
	 * Example 3: Move semantics (C++11+)
	 * ```cpp
	 * fiber_tbox2<std::string> mailbox;
	 * std::string msg = "Hello";
	 * mailbox.push(std::move(msg));  // String is moved (efficient)
	 * // msg is now in moved-from state
	 * ```
	 */
	bool push(T t, bool notify_first = true) {
		if (! mutex_.lock()) { abort(); }

		// Check if buffer is full
		if (off_next_ == capacity_) {
			// Try to compact the buffer first if there's enough consumed space
			if (off_curr_ >= 10000) {
				// Compact: Move unconsumed messages to the front
#if 1
				size_t n = 0;
				for (size_t i = off_curr_; i < off_next_; i++) {
					box_[n++] = box_[i];
				}
#else
				// Alternative: memmove (only safe for POD types)
				memmove(box_, box_ + off_curr_,
					(off_next_ - off_curr_) * sizeof(T));
#endif

				off_next_ -= off_curr_;
				off_curr_ = 0;
			} else {
				// Grow the buffer by 10,000 elements
				size_t capacity = capacity_ + 10000;
				T* box = new T[capacity];
				for (size_t i = 0; i < capacity_; i++) {
#if __cplusplus >= 201103L || defined(USE_CPP11)
					box[i] = std::move(box_[i]);  // Move existing objects
#else
					box[i] = box_[i];  // Copy existing objects
#endif
				}
				delete []box_;
				box_ = box;
				capacity_ = capacity;
			}
		}

		// Store the message (move in C++11+, copy otherwise)
#if __cplusplus >= 201103L || defined(USE_CPP11)
		box_[off_next_++] = std::move(t);
#else
		box_[off_next_++] = t;
#endif

		// Notify waiting consumers
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
	 * pop - Receive a message object from the mailbox by value
	 * 
	 * Retrieves and removes the first message from the queue by copying or
	 * moving (C++11+) it into the provided reference. If the queue is empty,
	 * this method blocks until a message arrives or timeout occurs.
	 * 
	 * @param t {T&} Output parameter that will receive the message object.
	 *  In C++11+, the object is moved if possible (using std::move),
	 *  otherwise copied. The object in the mailbox is consumed.
	 * 
	 * @param ms {int} Timeout in milliseconds:
	 *  - ms < 0: Wait forever until a message arrives (default: -1)
	 *  - ms == 0: Return immediately (non-blocking check)
	 *  - ms > 0: Wait up to ms milliseconds for a message
	 * 
	 * @return {bool} Returns true if a message was retrieved, false if:
	 *  - Timeout occurred (when ms >= 0)
	 *  - The fiber was killed
	 * 
	 * Note: Unlike fiber_tbox::pop(), this method does not need a 'found'
	 * parameter because the return value unambiguously indicates success.
	 * There are no "null messages" in fiber_tbox2.
	 * 
	 * @override
	 * 
	 * Example 1: Blocking pop (wait forever)
	 * ```cpp
	 * fiber_tbox2<Task> mailbox;
	 * Task task;
	 * if (mailbox.pop(task)) {  // Blocks until message arrives
	 *     task.execute();
	 * }
	 * ```
	 * 
	 * Example 2: Pop with timeout
	 * ```cpp
	 * Task task;
	 * if (mailbox.pop(task, 5000)) {  // Wait up to 5 seconds
	 *     task.execute();
	 * } else {
	 *     printf("Timeout, no message received\n");
	 * }
	 * ```
	 * 
	 * Example 3: Non-blocking pop
	 * ```cpp
	 * Task task;
	 * if (mailbox.pop(task, 0)) {  // Check immediately
	 *     task.execute();
	 * } else {
	 *     // Do other work
	 * }
	 * ```
	 * 
	 * Example 4: Smart pointers
	 * ```cpp
	 * fiber_tbox2<std::shared_ptr<Task>> mailbox;
	 * std::shared_ptr<Task> task;
	 * if (mailbox.pop(task)) {
	 *     task->execute();
	 *     // Automatic cleanup when task goes out of scope
	 * }
	 * ```
	 */
	bool pop(T& t, int ms = -1) {
		if (! mutex_.lock()) { abort(); }

		while (true) {
			if (peek_obj(t)) {
				if (! mutex_.unlock()) { abort(); }
				return true;
			}

			// Wait for a message to arrive
			if (!cond_.wait(mutex_, ms) && ms >= 0) {
				// Timeout occurred
				if (! mutex_.unlock()) { abort(); }
				return false;
			}

			if (fiber::self_killed()) {
				// Fiber is being killed, abort waiting
				if (! mutex_.unlock()) { abort(); }
				return false;
			}
		}
	}

	/**
	 * pop - Receive multiple message objects from the mailbox (batch operation)
	 * 
	 * Retrieves multiple messages from the queue in a single operation by
	 * copying or moving (C++11+) them into the provided vector. This is more
	 * efficient than calling pop() multiple times when you need to process
	 * messages in batches.
	 * 
	 * Behavior:
	 * 1. If messages are available, retrieve up to 'max' messages immediately
	 * 2. If queue is empty, wait for at least one message (up to timeout)
	 * 3. Once at least one message is retrieved, get all available messages
	 *    up to 'max' without further waiting
	 * 
	 * @param out {std::vector<T>&} Output vector to store retrieved messages.
	 *  Messages are appended to this vector (not cleared first).
	 *  Objects are copied or moved (C++11+) into the vector.
	 * 
	 * @param max {size_t} Maximum number of messages to retrieve:
	 *  - 0: Retrieve all available messages (no limit)
	 *  - >0: Retrieve at most this many messages
	 * 
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
	 * fiber_tbox2<Task> mailbox;
	 * std::vector<Task> tasks;
	 * 
	 * // Get up to 10 tasks, wait up to 1 second
	 * size_t n = mailbox.pop(tasks, 10, 1000);
	 * printf("Retrieved %zu tasks\n", n);
	 * 
	 * for (Task& task : tasks) {
	 *     task.execute();
	 * }
	 * ```
	 * 
	 * Example 2: Drain all messages
	 * ```cpp
	 * std::vector<Task> tasks;
	 * size_t n = mailbox.pop(tasks, 0, 0);  // Get all available, no wait
	 * printf("Drained %zu tasks\n", n);
	 * ```
	 */
	size_t pop(std::vector<T>& out, size_t max, int ms) {
		size_t n = 0;

		if (! mutex_.lock()) { abort(); }

		while (true) {
			T t;
			if (peek_obj(t)) {
				out.push_back(t);  // Copy or move into vector
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
	 * size - Get the current number of messages in the mailbox
	 * 
	 * Return the current number of messages in the message queue.
	 * 
	 * Note: This value may change immediately after calling this method
	 * if other fibers/threads are pushing or popping concurrently.
	 * Use this for monitoring/debugging, not for synchronization logic.
	 * 
	 * @return {size_t} The number of messages currently in the queue
	 * @override
	 * 
	 * Example:
	 * ```cpp
	 * printf("Mailbox has %zu pending messages\n", mailbox.size());
	 * ```
	 */
	size_t size() const {
		return off_next_ - off_curr_;
	}

	/**
	 * has_null - Check if this mailbox supports NULL messages
	 * 
	 * Note: This returns true for compatibility with the box2<T> interface,
	 * but fiber_tbox2 doesn't actually support NULL messages in the same
	 * way as fiber_tbox. Since fiber_tbox2 transfers objects by value,
	 * there's no concept of a "null pointer" message. However, the type T
	 * itself could be a nullable type (e.g., std::shared_ptr<T> can be null).
	 * 
	 * @return {bool} Always returns true for interface compatibility
	 * @override
	 */
	bool has_null() const {
		return true;
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
		if (! mutex_.lock()) { abort(); }
	}

	/**
	 * unlock - Manually unlock the internal mutex
	 * 
	 * Releases the internal mutex previously acquired by lock().
	 * Must be called after lock() to avoid deadlocks.
	 */
	void unlock() {
		if (! mutex_.unlock()) { abort(); }
	}

private:
	// Disable copy construction and assignment
	fiber_tbox2(const fiber_tbox2&) {}
	const fiber_tbox2& operator=(const fiber_tbox2&);

private:
	T*           box_;        // Internal array for storing messages
	size_t       capacity_;   // Current capacity of the array
	size_t       off_curr_;   // Offset of the first unconsumed message
	size_t       off_next_;   // Offset where the next message will be stored
	fiber_mutex  mutex_;      // Mutex for thread-safe access
	fiber_cond   cond_;       // Condition variable for blocking/notification

	/**
	 * peek_obj - Internal method to retrieve the first message without blocking
	 * 
	 * @param t {T&} Output parameter that will receive the message object
	 * @return {bool} Returns true if a message was retrieved, false if empty
	 */
	bool peek_obj(T& t) {
		if (off_curr_ == off_next_) {
			// Queue is empty
			if (off_curr_ > 0) {
				// Reset offsets to start of buffer
				off_curr_ = off_next_ = 0;
			}
			return false;
		}

		// Move or copy the object out of the buffer
#if __cplusplus >= 201103L || defined(USE_CPP11)
		t = std::move(box_[off_curr_++]);  // Move for efficiency
#else
		t = box_[off_curr_++];  // Copy in C++03
#endif
		return true;
	}
};

} // namespace acl

/**
 * ============================================================================
 * COMPARISON: fiber_tbox vs fiber_tbox2
 * ============================================================================
 * 
 * +------------------+-------------------------+---------------------------+
 * | Feature          | fiber_tbox<T>           | fiber_tbox2<T>            |
 * +------------------+-------------------------+---------------------------+
 * | Transfer Method  | Pointer (T*)            | Value (T)                 |
 * | Memory Semantics | Manual (new/delete)     | Automatic (RAII)          |
 * | Copying          | No copy (pointer only)  | Copy or move object       |
 * | NULL Messages    | Yes (nullptr)           | No (but T can be nullable)|
 * | Storage          | std::list<T*>           | T[] array                 |
 * | Capacity         | Unlimited (dynamic)     | Dynamic (grows by 10K)    |
 * | Memory Overhead  | List nodes + pointers   | Array + unused slots      |
 * | Best For         | Large objects           | Small objects/smart ptrs  |
 * |                  | Polymorphic types       | Value types               |
 * |                  | Manual control          | Automatic management      |
 * | Move Semantics   | N/A (just pointer)      | Yes (C++11+)              |
 * | Smart Pointers   | Possible but awkward    | Ideal use case            |
 * +------------------+-------------------------+---------------------------+
 * 
 * ============================================================================
 * WHEN TO USE fiber_tbox:
 * ============================================================================
 * 
 * 1. Large objects where copying is expensive
 * 2. Polymorphic types (base class pointers)
 * 3. When you need explicit ownership transfer
 * 4. When NULL is a meaningful signal
 * 5. When you want zero-copy semantics
 * 
 * Example:
 * ```cpp
 * fiber_tbox<LargeObject> box;
 * box.push(new LargeObject());  // No copy, just pointer
 * LargeObject* obj = box.pop();
 * delete obj;  // Manual cleanup
 * ```
 * 
 * ============================================================================
 * WHEN TO USE fiber_tbox2:
 * ============================================================================
 * 
 * 1. Smart pointers (std::shared_ptr, std::unique_ptr)
 * 2. Small value types (int, struct, etc.)
 * 3. When you want automatic memory management
 * 4. When move semantics are available (C++11+)
 * 5. When you want simpler ownership semantics
 * 
 * Example:
 * ```cpp
 * fiber_tbox2<std::shared_ptr<Task>> box;
 * box.push(std::make_shared<Task>());  // Ref count++
 * std::shared_ptr<Task> task;
 * box.pop(task);  // Automatic cleanup
 * ```
 * 
 * ============================================================================
 * USAGE EXAMPLES
 * ============================================================================
 * 
 * Example 1: Smart Pointers (Recommended Pattern)
 * ------------------------------------------------
 * #include "fiber/fiber_tbox2.hpp"
 * #include "fiber/go_fiber.hpp"
 * #include <memory>
 * 
 * struct Task {
 *     int id;
 *     std::string data;
 *     void execute() { printf("Task %d: %s\n", id, data.c_str()); }
 * };
 * 
 * using TaskPtr = std::shared_ptr<Task>;
 * acl::fiber_tbox2<TaskPtr> mailbox;
 * 
 * void producer() {
 *     for (int i = 0; i < 10; i++) {
 *         auto task = std::make_shared<Task>(Task{i, "data"});
 *         mailbox.push(task);
 *         printf("Produced task %d\n", i);
 *         acl::fiber::delay(100);
 *     }
 *     // Send termination signal (null shared_ptr)
 *     mailbox.push(nullptr);
 * }
 * 
 * void consumer() {
 *     while (true) {
 *         TaskPtr task;
 *         if (mailbox.pop(task)) {
 *             if (!task) {
 *                 printf("Termination signal received\n");
 *                 break;
 *             }
 *             task->execute();
 *             // Automatic cleanup when task goes out of scope
 *         }
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
 * Example 2: Value Types
 * ============================================================================
 * 
 * struct Point {
 *     int x, y;
 * };
 * 
 * acl::fiber_tbox2<Point> mailbox;
 * 
 * void producer() {
 *     for (int i = 0; i < 5; i++) {
 *         Point p{i, i * 2};
 *         mailbox.push(p);  // Object is copied/moved
 *     }
 * }
 * 
 * void consumer() {
 *     for (int i = 0; i < 5; i++) {
 *         Point p;
 *         if (mailbox.pop(p, 1000)) {  // 1 second timeout
 *             printf("Point(%d, %d)\n", p.x, p.y);
 *         }
 *     }
 * }
 * 
 * ============================================================================
 * Example 3: Move-Only Types (C++11+)
 * ============================================================================
 * 
 * acl::fiber_tbox2<std::unique_ptr<Task>> mailbox;
 * 
 * void producer() {
 *     for (int i = 0; i < 5; i++) {
 *         auto task = std::make_unique<Task>(Task{i, "data"});
 *         mailbox.push(std::move(task));  // Must use std::move
 *         // task is now nullptr
 *     }
 * }
 * 
 * void consumer() {
 *     for (int i = 0; i < 5; i++) {
 *         std::unique_ptr<Task> task;
 *         if (mailbox.pop(task)) {
 *             task->execute();
 *             // Automatic cleanup when task goes out of scope
 *         }
 *     }
 * }
 * 
 * ============================================================================
 * Example 4: Batch Processing with Smart Pointers
 * ============================================================================
 * 
 * using TaskPtr = std::shared_ptr<Task>;
 * acl::fiber_tbox2<TaskPtr> mailbox;
 * 
 * void batch_consumer() {
 *     std::vector<TaskPtr> batch;
 *     
 *     while (true) {
 *         batch.clear();
 *         
 *         // Get up to 20 tasks, wait up to 1 second
 *         size_t n = mailbox.pop(batch, 20, 1000);
 *         
 *         if (n == 0) break;  // Timeout
 *         
 *         printf("Processing batch of %zu tasks\n", n);
 *         for (auto& task : batch) {
 *             if (task) {
 *                 task->execute();
 *             }
 *         }
 *     }
 * }
 * 
 * ============================================================================
 * BEST PRACTICES
 * ============================================================================
 * 
 * 1. Prefer Smart Pointers:
 *    - Use std::shared_ptr for shared ownership
 *    - Use std::unique_ptr for exclusive ownership
 *    - Automatic memory management, no leaks
 * 
 * 2. Move Semantics (C++11+):
 *    - Use std::move() for move-only types
 *    - Compiler automatically moves when possible
 *    - Reduces copying overhead
 * 
 * 3. Type Selection:
 *    - Small types (<= 64 bytes): fiber_tbox2 is efficient
 *    - Large types (> 64 bytes): Consider fiber_tbox with pointers
 *    - Smart pointers: Always use fiber_tbox2
 * 
 * 4. Capacity Management:
 *    - Initial capacity: 10,000 elements
 *    - Growth: +10,000 elements when full
 *    - Compaction: When consumed >= 10,000
 *    - Monitor size() if memory is constrained
 * 
 * 5. Performance:
 *    - Move semantics (C++11+) avoid unnecessary copies
 *    - Batch operations reduce lock contention
 *    - Compaction reuses memory efficiently
 * 
 * 6. Termination Signals:
 *    - For smart pointers: push(nullptr)
 *    - For value types: Use a special sentinel value
 *    - Or use a separate flag/channel
 * 
 * 7. Error Handling:
 *    - Check return value of pop() for timeout
 *    - Handle fiber termination gracefully
 *    - Ensure proper cleanup in destructors
 * 
 * 8. Thread Safety:
 *    - All operations are thread-safe
 *    - Can be used across fibers and threads
 *    - No external synchronization needed
 * 
 * ============================================================================
 */
