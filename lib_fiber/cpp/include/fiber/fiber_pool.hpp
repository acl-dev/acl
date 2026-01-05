#pragma once
#include "fiber_cpp_define.hpp"

#if defined(USE_CPP11) || __cplusplus >= 201103L

#include <functional>
#include <memory>
#include <set>
#include "fiber.hpp"
#include "fiber_sem.hpp"

namespace acl {

/**
 * task_fn - Type alias for task functions executed in the fiber pool
 * 
 * @brief The task function type.
 * @note The task function should be a callable object, such as a lambda
 * function, a function pointer, or any object with operator().
 * The function takes no parameters and returns void.
 */
using task_fn = std::function<void(void)>;

/**
 * task_box - Container for managing a fiber and its task queue
 * 
 * @brief The task box class for holding tasks to be run by the fiber.
 * Each fiber in the pool has its own task_box that contains:
 * - A fiber instance that processes tasks
 * - A mailbox (fiber_sbox2) for receiving tasks
 * - Index information for pool management
 * - Idle state tracking
 * 
 * @tparam task_fn The type of the task function.
 */
template<class task_fn>
class task_box {
public:
	/**
	 * Constructor - Creates a task box with the given mailbox
	 * @param bx Pointer to the fiber mailbox for receiving tasks
	 */
	explicit task_box(fiber_sbox2<task_fn>* bx) : box(bx) {}
	
	/**
	 * Destructor - Cleans up the mailbox
	 */
	~task_box() { delete box; }

	std::shared_ptr<fiber> fb;       // The fiber that processes tasks from this box
	fiber_sbox2<task_fn> *box = nullptr;  // Mailbox for receiving tasks
	ssize_t idx  = -1;               // Index of this box in the pool's box array
	ssize_t idle = -1;               // Index in the idle boxes array (-1 if not idle)
};

class wait_group;

/**
 * fibers_set - Type alias for a set of fiber shared pointers
 * Uses owner_less for proper comparison of weak/shared pointers
 */
using fibers_set = std::set<std::shared_ptr<fiber>, std::owner_less<std::shared_ptr<fiber>>>;

/**
 * fiber_pool - A thread pool-like facility for managing and executing tasks in fibers
 * 
 * @brief The fiber pool class for running tasks in fibers. One fiber owns one
 * task box, and the task box is used to hold the tasks to be run by the fiber.
 * 
 * Key Features:
 * - Dynamic fiber scaling: Grows from min to max fibers based on workload
 * - Idle fiber management: Fibers can exit after idle timeout to save resources
 * - Task queuing: Each fiber has its own task queue (mailbox)
 * - Load balancing: Tasks are distributed across available fibers
 * - Backpressure: Optional yielding when task queues are full
 * 
 * Architecture:
 * - Each fiber has a dedicated task_box with a mailbox
 * - Tasks are pushed to fiber mailboxes in round-robin fashion
 * - Idle fibers are tracked separately for quick reuse
 * - Pool can grow dynamically up to max_fibers limit
 * 
 * Thread Safety:
 * - Can be created and used by single thread only
 * - Safe to call exec() from multiple fibers concurrently
 * - Internal state is managed using fiber-safe primitives
 */
class FIBER_CPP_API fiber_pool {
public:
	/**
	 * Constructor - Create a new fiber pool with specified configuration
	 * 
	 * @brief Construct a new fiber pool object.
	 * 
	 * The pool starts with 'min' fibers and can grow up to 'max' fibers
	 * based on workload. Fibers that remain idle for 'idle_ms' milliseconds
	 * will exit to conserve resources (if idle_ms >= 0).
	 * 
	 * @param min The minimum number of fibers in the pool (can be 0).
	 *            If 0, fibers are created on-demand.
	 * @param max The maximum number of fibers in the pool.
	 *            Must satisfy: when min == 0, max > min; when min > 0, max >= min.
	 * @param idle_ms The idle timeout in milliseconds before an idle fiber exits.
	 *                -1 means fibers never exit due to idle timeout.
	 *                0 means fibers exit immediately when idle.
	 *                >0 means fibers exit after being idle for this duration.
	 * @param box_buf The buffer size of each task box (mailbox capacity).
	 *                When a fiber's queue reaches this size, exec() may yield
	 *                to allow the fiber to process tasks. 0 means no yielding.
	 * @param stack_size The stack size in bytes for each fiber (default: 128KB).
	 * @param stack_share Whether fibers share stack memory (default: false).
	 *                    Shared stacks save memory but may have performance impact.
	 * 
	 * Example:
	 * ```cpp
	 * // Pool with 2-10 fibers, 60s idle timeout, 500 task buffer, 128KB stack
	 * acl::fiber_pool pool(2, 10, 60000, 500, 128000, false);
	 * ```
	 */
	fiber_pool(size_t min, size_t max, int idle_ms = -1, size_t box_buf = 500,
		size_t stack_size = 128000, bool stack_share = false);
	
	/**
	 * Destructor - Stops the pool and cleans up all resources
	 */
	~fiber_pool();

	// Disable copy construction and assignment
	fiber_pool(const fiber_pool&) = delete;
	fiber_pool& operator=(const fiber_pool&) = delete;

	/**
	 * exec - Submit a task to be executed by a fiber in the pool
	 * 
	 * @brief Execute a task in the fiber pool.
	 * 
	 * This method submits a task to the pool for asynchronous execution.
	 * The task selection strategy is:
	 * 1. If there are idle fibers, reuse one
	 * 2. If pool hasn't reached max size, create a new fiber
	 * 3. Otherwise, round-robin to existing fibers
	 * 
	 * The method may yield if the selected fiber's task queue is full
	 * (when box_buf > 0), providing backpressure control.
	 * 
	 * @tparam Fn The type of the callable object (function, lambda, etc.)
	 * @tparam Args The types of the arguments to pass to the function
	 * @param fn The function to be run by one fiber of the fiber pool.
	 * @param args The arguments to be passed to the function (forwarded perfectly).
	 * 
	 * Example:
	 * ```cpp
	 * pool.exec([]() { printf("Hello\n"); });
	 * pool.exec([](int x) { printf("%d\n", x); }, 42);
	 * pool.exec(my_function, arg1, arg2);
	 * ```
	 */
	template<class Fn, class ...Args>
	void exec(Fn&& fn, Args&&... args) {
		// Bind the function with its arguments
		auto obj = std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...);
		task_box<task_fn>* box;
		
		// Task assignment strategy
		if (box_idle_ > 0) {
			// Reuse an idle fiber
			box = boxes_idle_[box_idle_ - 1];
		} else if (box_count_ < box_max_) {
			// Create a new fiber if under max limit
			fiber_create(1);
			box = boxes_[box_next_++ % box_count_];
		} else {
			// Round-robin to existing fibers
			box = boxes_[box_next_++ % box_count_];
		}

		// Push task to the selected fiber's mailbox
		box->box->push(obj, true);
		
		// Backpressure: yield if the mailbox is getting full
		if (box_buf_ > 0 && box->box->size() >= (size_t) box_buf_) {
			fiber::yield();
		}
	}

	/**
	 * stop - Gracefully stop the fiber pool
	 * 
	 * @brief Stop the fiber pool.
	 * 
	 * This method signals all fibers in the pool to stop processing tasks.
	 * It waits for all currently executing tasks to complete before returning.
	 * After calling stop(), the pool cannot be reused.
	 * 
	 * Note: This is a blocking call that waits for all fibers to finish.
	 */
	void stop();

public:
	// ========================================================================
	// Getter Methods - Query pool configuration and status
	// ========================================================================

	/**
	 * get_box_min - Get the minimum number of fibers configured for the pool
	 * 
	 * @brief Get the minimum size of the fiber pool.
	 * @return size_t The minimum number of fibers (as configured in constructor)
	 */
	size_t get_box_min() const {
		return box_min_;
	}

	/**
	 * get_box_max - Get the maximum number of fibers allowed in the pool
	 * 
	 * @brief Get the maximum size of the fiber pool.
	 * @return size_t The maximum number of fibers (as configured in constructor)
	 */
	size_t get_box_max() const {
		return box_max_;
	}

	/**
	 * get_box_count - Get the current number of active fibers in the pool
	 * 
	 * @brief Get the count of fibers in the pool.
	 * @return size_t The current number of fibers (between min and max)
	 */
	size_t get_box_count() const {
		return box_count_;
	}

	/**
	 * get_box_idle - Get the number of idle fibers currently waiting for tasks
	 * 
	 * @brief Get the idle count of idle fibers in the fiber pool.
	 * @return size_t The number of fibers currently idle (waiting for work)
	 */
	size_t get_box_idle() const {
		return box_idle_;
	}

	/**
	 * get_box_buf - Get the task buffer size for each fiber's mailbox
	 * 
	 * @brief Get the box buffer size of holding tasks.
	 * @return size_t The mailbox buffer size (as configured in constructor)
	 */
	size_t get_box_buf() const {
		return box_buf_;
	}

private:
	// ========================================================================
	// Private Member Variables
	// ========================================================================

	wait_group* wg_;          // Wait group for synchronizing fiber shutdown
	int    idle_ms_;          // Idle timeout in milliseconds (-1 = no timeout)
	size_t box_buf_;          // Buffer size for each task box mailbox
	size_t stack_size_;       // Stack size for each fiber
	bool   stack_share_;      // Whether fibers share stack memory

	size_t box_min_;          // Minimum number of fibers
	size_t box_max_;          // Maximum number of fibers

	size_t box_count_  = 0;   // Current number of active fibers
	size_t box_next_   = 0;   // Next box index for round-robin task assignment
	ssize_t box_idle_  = 0;   // Number of idle fibers

	task_box<task_fn> **boxes_;       // Array of all task boxes
	task_box<task_fn> **boxes_idle_;  // Array of idle task boxes (for quick lookup)
	fibers_set fibers_;               // Set of all fiber shared pointers

	// ========================================================================
	// Private Methods
	// ========================================================================

	/**
	 * fiber_create - Create a specified number of new fibers
	 * @param count Number of fibers to create
	 */
	void fiber_create(size_t count);
	
	/**
	 * fiber_run - Entry point for fiber execution
	 * @param box The task box associated with this fiber
	 */
	void fiber_run(task_box<task_fn>* box);
	
	/**
	 * running - Main loop for processing tasks from the task box
	 * @param box The task box to process tasks from
	 */
	void running(task_box<task_fn>* box);
};

/**
 * ============================================================================
 * USAGE EXAMPLES
 * ============================================================================
 * 
 * Example 1: Basic Usage with Lambda Functions
 * ---------------------------------------------
 * void mytest(acl::wait_group& wg, int i) {
 *    printf("Task %d is running\n", i);
 *    wg.done();
 * }
 *
 * int main(int argc, char* argv[]) {
 *    // Create pool: min=1, max=20, idle_timeout=60ms, buffer=500,
 *    //              stack=64KB, shared_stack=false
 *    acl::fiber_pool pool(1, 20, 60, 500, 64000, false);
 *    acl::wait_group wg;
 *    int i = 0;
 *
 *    // Method 1: Lambda with capture by value
 *    wg.add(1);
 *    pool.exec([&wg, i]() {
 *        printf("Task %d is running\n", i);
 *        wg.done();
 *    });
 *    i++;
 *
 *    // Method 2: Lambda with parameters
 *    wg.add(1);
 *    pool.exec([&wg](int i) {
 *  	  printf("Task %d is running\n", i);
 *  	  wg.done();
 *    }, i);
 *    i++;
 *
 *    // Method 3: Regular function with std::ref for reference parameters
 *    wg.add(1);
 *    pool.exec(mytest, std::ref(wg), i);
 *
 *    // Wait for all tasks to complete, then stop the pool
 *    go[&wg, &pool] {
 *        wg.wait();
 *        pool.stop();
 *    };
 *
 *    // Start the fiber scheduler
 *    acl::fiber::schedule();
 *    return 0;
 * }
 * 
 * ============================================================================
 * Example 2: Processing a Large Number of Tasks
 * ============================================================================
 * 
 * void process_item(int item_id) {
 *     // Simulate some work
 *     printf("Processing item %d\n", item_id);
 *     acl::fiber::delay(10);  // 10ms delay
 * }
 * 
 * int main() {
 *     // Pool with 5-50 fibers, 5 second idle timeout
 *     acl::fiber_pool pool(5, 50, 5000, 1000, 128000, false);
 *     acl::wait_group wg;
 * 
 *     // Submit 1000 tasks
 *     const int num_tasks = 1000;
 *     wg.add(num_tasks);
 * 
 *     for (int i = 0; i < num_tasks; i++) {
 *         pool.exec([&wg, i]() {
 *             process_item(i);
 *             wg.done();
 *         });
 *     }
 * 
 *     // Wait for completion
 *    go[&wg, &pool] {
 *        wg.wait();
 *        pool.stop();
 *    };
 * 
 *    acl::fiber::schedule();
 *    return 0;
 * }
 * 
 * ============================================================================
 * Example 3: Dynamic Task Generation
 * ============================================================================
 * 
 * int main() {
 *     acl::fiber_pool pool(2, 10, -1, 500, 128000, false);
 *     std::atomic<int> active_tasks(0);
 * 
 *     // Producer fiber
 *     go[&pool, &active_tasks] {
 *         for (int i = 0; i < 100; i++) {
 *             active_tasks++;
 *             pool.exec([i, &active_tasks]() {
 *                 printf("Task %d executing\n", i);
 *                 acl::fiber::delay(100);
 *                 active_tasks--;
 *             });
 *             acl::fiber::delay(50);  // Throttle task generation
 *         }
 *     };
 * 
 *     // Monitor fiber
 *     go[&pool, &active_tasks] {
 *         while (active_tasks > 0) {
 *             printf("Active tasks: %d, Pool size: %zu, Idle: %zu\n",
 *                    active_tasks.load(),
 *                    pool.get_box_count(),
 *                    pool.get_box_idle());
 *             acl::fiber::delay(1000);
 *         }
 *         pool.stop();
 *     };
 * 
 *     acl::fiber::schedule();
 *     return 0;
 * }
 * 
 * ============================================================================
 * BEST PRACTICES
 * ============================================================================
 * 
 * 1. Pool Sizing:
 *    - Set min to expected baseline load
 *    - Set max to handle peak load
 *    - Leave headroom for unexpected spikes
 * 
 * 2. Idle Timeout:
 *    - Use -1 for stable workloads (no timeout)
 *    - Use positive value for variable workloads
 *    - Balance between responsiveness and resource usage
 * 
 * 3. Buffer Size:
 *    - Larger buffer = more queued tasks per fiber
 *    - Smaller buffer = better backpressure control
 *    - Set to 0 to disable yielding
 * 
 * 4. Stack Size:
 *    - Default 128KB is suitable for most tasks
 *    - Increase for deep recursion or large local variables
 *    - Decrease for memory-constrained environments
 * 
 * 5. Shared Stack:
 *    - Use false (default) for better performance
 *    - Use true only when memory is extremely limited
 *    - Shared stacks have context switch overhead
 * 
 * 6. Task Granularity:
 *    - Tasks should be neither too small nor too large
 *    - Too small: overhead dominates
 *    - Too large: poor load balancing
 *    - Aim for tasks that take 10-100ms
 * 
 * 7. Error Handling:
 *    - Always use wait_group to track task completion
 *    - Handle exceptions within tasks
 *    - Use atomic variables for error flags
 * 
 * 8. Shutdown:
 *    - Always call stop() before program exit
 *    - Use wait_group to ensure all tasks complete
 *    - stop() is blocking and waits for fibers
 * 
 * ============================================================================
 * PERFORMANCE CONSIDERATIONS
 * ============================================================================
 * 
 * 1. Pool Growth Strategy:
 *    - Pool grows one fiber at a time when needed
 *    - Idle fibers are reused before creating new ones
 *    - Round-robin distribution when pool is at max
 * 
 * 2. Memory Usage:
 *    - Each fiber: stack_size bytes (default 128KB)
 *    - Each task_box: ~100 bytes + mailbox overhead
 *    - Total memory ≈ max_fibers * (stack_size + mailbox_overhead)
 * 
 * 3. Latency:
 *    - Task submission: O(1) - very fast
 *    - Task start: Depends on fiber availability
 *    - Backpressure: Yields when mailbox is full
 * 
 * 4. Throughput:
 *    - Limited by number of fibers and task duration
 *    - Optimal when tasks are I/O bound
 *    - CPU-bound tasks may need fewer fibers
 * 
 * ============================================================================
 */
} // namespace acl

#endif // __cplusplus >= 201103L
