#pragma once

#include "../master_fiber.hpp"
#include "../http_servlet.hpp"
#include "../go_fiber.hpp"

namespace acl {

/**
 * ============================================================================
 * Type Definitions - Callback function types for server lifecycle hooks
 * ============================================================================
 */

/**
 * proc_jail_t - Callback type for pre-jail initialization
 * Called before the process is jailed (chroot/setuid)
 */
typedef std::function<void()> proc_jail_t;

/**
 * proc_init_t - Callback type for process initialization
 * Called when a worker process starts
 */
typedef std::function<void()> proc_init_t;

/**
 * proc_exit_t - Callback type for process exit
 * Called when a worker process is about to exit
 */
typedef std::function<void()> proc_exit_t;

/**
 * proc_sighup_t - Callback type for SIGHUP signal handling
 * Called when the process receives a SIGHUP signal
 * @param acl::string& Configuration string or message
 * @return bool True to continue, false to stop
 */
typedef std::function<bool(acl::string&)> proc_sighup_t;

/**
 * thread_init_t - Callback type for thread initialization
 * Called when a worker thread starts
 */
typedef std::function<void()> thread_init_t;

/**
 * thread_accept_t - Callback type for connection acceptance
 * Called when a thread accepts a new client connection
 * @param acl::socket_stream& The accepted socket connection
 * @return bool True to process the connection, false to reject it
 */
typedef std::function<bool(acl::socket_stream&)> thread_accept_t;

/**
 * http_server_impl - Internal implementation class for HTTP server
 * 
 * This class provides the core implementation for the HTTP server,
 * inheriting from master_fiber to leverage fiber-based concurrency.
 * It handles:
 * - Session management (Redis or Memcache)
 * - HTTP request routing and handler dispatch
 * - Process and thread lifecycle management
 * - Connection acceptance and processing
 * 
 * This is an internal implementation class. Users should use the
 * http_server class instead, which provides a cleaner public API.
 */
class http_server_impl : public master_fiber {
public:
	/**
	 * Constructor - Initializes the HTTP server implementation
	 * 
	 * Sets up session management using either Redis or Memcache.
	 * If Redis is enabled, it creates a Redis cluster client for
	 * distributed session storage.
	 * 
	 * @param addr Session server address in format "ip|port" or "ip:port"
	 *             For Redis: "127.0.0.1|6379"
	 *             For Memcache: "127.0.0.1|11211"
	 * @param use_redis If true, use Redis for session management;
	 *                  If false, use Memcache (default: 127.0.0.1|11211)
	 */
	http_server_impl(const char* addr, bool use_redis) {
		if (use_redis) {
			redis_ = new redis_client_cluster;
			// Initialize Redis cluster: (passwd, addr, conn_timeout,
			//                            rw_timeout, max_conns)
			redis_->init(NULL, addr, 0, 10, 10);
			redis_->bind_thread(true);
		}
	}

	virtual ~http_server_impl(void) {}

protected:
	/**
	 * Service - Register a handler for a specific HTTP method and path
	 * 
	 * This method normalizes the path and stores the handler in the
	 * internal routing table. Path normalization includes:
	 * - Ensuring the path ends with '/'
	 * - Converting to lowercase for case-insensitive matching
	 * 
	 * @param type The HTTP method type (http_handler_get, http_handler_post, etc.)
	 * @param path The URL path to match (e.g., "/api/users")
	 * @param fn The handler function to execute when the path is matched
	 */
	void Service(int type, const char* path, http_handler_t fn) {
		if (type >= http_handler_get && type < http_handler_max
				&& path && *path) {

			// The path should look like "/xxx/" with
			// lowercase characters for normalized matching.

			acl::string buf(path);
			if (buf[buf.size() - 1] != '/') {
				buf += '/';
			}
			buf.lower();
			handlers_[type][buf] = std::move(fn);
		}
	}

protected:
	// ========================================================================
	// Member Variables
	// ========================================================================

	redis_client_cluster* redis_   = nullptr;  // Redis cluster client for session management
	proc_jail_t     proc_jail_     = nullptr;  // Pre-jail initialization callback
	proc_init_t     proc_init_     = nullptr;  // Process initialization callback
	proc_exit_t     proc_exit_     = nullptr;  // Process exit callback
	proc_sighup_t   proc_sighup_   = nullptr;  // SIGHUP signal handler callback
	thread_init_t   thread_init_   = nullptr;  // Thread initialization callback
	thread_accept_t thread_accept_ = nullptr;  // Connection acceptance callback
	http_handlers_t handlers_[http_handler_max];  // Route handlers for each HTTP method

	// ========================================================================
	// Overridden Methods from master_fiber
	// ========================================================================

	/**
	 * on_accept - Handle a new client connection
	 * 
	 * This method is called when a new client connection is accepted.
	 * It performs the following steps:
	 * 1. Calls the thread_accept_ callback if registered (for filtering)
	 * 2. Creates a session object (Redis or Memcache based)
	 * 3. Creates an HTTP servlet to handle the connection
	 * 4. Processes HTTP requests in a loop until connection closes
	 * 5. Cleans up the session object
	 * 
	 * @param conn The accepted socket connection stream
	 * 
	 * @override from master_fiber
	 */
	void on_accept(socket_stream& conn) {
		// Allow custom filtering/validation of the connection
		if (thread_accept_ && !thread_accept_(conn)) {
			return;
		}

		// Create session management object based on configuration
		acl::session* session;
		if (redis_) {
			session = new redis_session(*redis_, 0);
		} else {
			// Fallback to Memcache if Redis is not configured
			session = new memcache_session("127.0.0.1|11211");
		}

		// Create HTTP servlet to handle requests on this connection
		http_servlet servlet(handlers_, &conn, session);
		servlet.setLocalCharset("utf-8");

		// Process requests in a loop (supports HTTP keep-alive)
		while (servlet.doRun()) {}

		// Clean up session object
		delete session;
	}

	/**
	 * proc_pre_jail - Called before process jail (chroot/setuid)
	 * 
	 * This hook is invoked before the process is jailed for security.
	 * Use this to perform any initialization that requires elevated
	 * privileges (e.g., opening privileged ports, accessing restricted files).
	 * 
	 * @override from master_fiber
	 */
	void proc_pre_jail(void) {
		if (proc_jail_) {
			proc_jail_();
		}
	}

	/**
	 * proc_on_init - Called when a worker process initializes
	 * 
	 * This hook is invoked after a worker process is forked and initialized.
	 * Use this to set up process-level resources such as:
	 * - Database connection pools
	 * - Logging systems
	 * - Cache initialization
	 * 
	 * @override from master_fiber
	 */
	void proc_on_init(void) {
		if (proc_init_) {
			proc_init_();
		}
	}

	/**
	 * proc_on_exit - Called when a worker process is about to exit
	 * 
	 * This hook is invoked before a worker process terminates.
	 * Use this to clean up process-level resources such as:
	 * - Closing database connections
	 * - Flushing logs
	 * - Releasing shared memory
	 * 
	 * @override from master_fiber
	 */
	void proc_on_exit(void) {
		if (proc_exit_) {
			proc_exit_();
		}
	}

	/**
	 * proc_on_sighup - Called when the process receives SIGHUP signal
	 * 
	 * This hook is invoked when the process receives a SIGHUP signal,
	 * typically used for configuration reload without restarting.
	 * 
	 * @param s Configuration string or message passed with the signal
	 * @return bool True to continue running, false to stop the process
	 * 
	 * @override from master_fiber
	 */
	bool proc_on_sighup(acl::string& s) {
		if (proc_sighup_) {
			return proc_sighup_(s);
		}
		return true;
	}

	/**
	 * thread_on_init - Called when a worker thread initializes
	 * 
	 * This hook is invoked when a worker thread starts.
	 * Use this to set up thread-local resources such as:
	 * - Thread-local storage
	 * - Per-thread caches
	 * - Thread-specific connections
	 * 
	 * @override from master_fiber
	 */
	void thread_on_init(void) {
		if (thread_init_) {
			thread_init_();
		}
	}
};

} // namespace acl

/**
 * ============================================================================
 * ARCHITECTURE OVERVIEW
 * ============================================================================
 * 
 * The http_server_impl class provides the internal implementation for the
 * HTTP server framework. It follows a multi-process, multi-thread architecture
 * with fiber-based concurrency:
 * 
 * 1. PROCESS MODEL:
 *    - Master process manages worker processes
 *    - Worker processes handle client connections
 *    - Process lifecycle hooks for initialization and cleanup
 * 
 * 2. THREAD MODEL:
 *    - Each worker process runs multiple threads
 *    - Threads accept and process connections concurrently
 *    - Thread lifecycle hooks for per-thread setup
 * 
 * 3. FIBER MODEL:
 *    - Each connection is handled by a fiber (coroutine)
 *    - Fibers provide lightweight concurrency
 *    - Thousands of concurrent connections with low memory overhead
 * 
 * 4. SESSION MANAGEMENT:
 *    - Redis: Distributed session storage for multi-server deployments
 *    - Memcache: Fallback session storage for single-server setups
 * 
 * 5. REQUEST PROCESSING FLOW:
 *    a. Thread accepts connection (on_accept)
 *    b. Optional connection filtering (thread_accept_ callback)
 *    c. Session object created (Redis or Memcache)
 *    d. HTTP servlet processes requests in a loop
 *    e. Handlers dispatched based on method and path
 *    f. Session cleaned up when connection closes
 * 
 * 6. LIFECYCLE HOOKS EXECUTION ORDER:
 *    Master Process:
 *      1. proc_pre_jail()  - Before security jail
 *      2. Fork worker processes
 *    
 *    Worker Process:
 *      3. proc_on_init()   - Process initialization
 *      4. Spawn worker threads
 *      5. thread_on_init() - Thread initialization (per thread)
 *      6. on_accept()      - Connection handling (per connection)
 *      7. proc_on_exit()   - Process cleanup (on shutdown)
 *    
 *    Signal Handling:
 *      - proc_on_sighup()  - Configuration reload (on SIGHUP)
 * 
 * ============================================================================
 * DESIGN PATTERNS
 * ============================================================================
 * 
 * 1. Template Method Pattern:
 *    - master_fiber defines the framework
 *    - http_server_impl overrides specific hooks
 * 
 * 2. Strategy Pattern:
 *    - Pluggable session management (Redis vs Memcache)
 *    - Customizable lifecycle callbacks
 * 
 * 3. Chain of Responsibility:
 *    - Request flows through: accept -> filter -> session -> servlet -> handler
 * 
 * ============================================================================
 */
