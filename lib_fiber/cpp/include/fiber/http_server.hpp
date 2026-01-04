#pragma once

#include <string>
#include "detail/http_server_impl.hpp"

namespace acl {

/**
 * http_server - A high-performance HTTP server based on fiber coroutines
 * 
 * This class provides a convenient interface for building HTTP servers with
 * support for various HTTP methods and lifecycle hooks. It inherits from
 * http_server_impl and provides a fluent API for registering route handlers.
 * 
 * Features:
 * - Support for all standard HTTP methods (GET, POST, PUT, DELETE, etc.)
 * - WebSocket support
 * - Process and thread lifecycle hooks
 * - Redis integration for session management (optional)
 * - Fluent API design for easy configuration
 */
class http_server : public http_server_impl {
public:
	/**
	 * Constructor - Creates an HTTP server instance
	 * 
	 * @param session_addr Session management server address in format
	 *  "ip|port" or "ip:port"; Default: "127.0.0.1|6379"
	 * @param use_redis Whether to enable Redis for session management;
	 *  Default: true; If true, Redis will be used for session management.
	 */
	http_server(const char* session_addr = "127.0.0.1|6379", bool use_redis = true)
	: http_server_impl(session_addr, use_redis) {}
	
	~http_server() {}

public:
	// ========================================================================
	// HTTP Method Handlers - Register route handlers for different HTTP methods
	// ========================================================================

	/**
	 * Get - Register a handler for HTTP GET requests
	 * 
	 * @param path The URL path to match (e.g., "/api/users")
	 * @param fn The handler function to execute when the path is matched
	 * @return Reference to this http_server for method chaining
	 */
	http_server& Get(const char* path, http_handler_t fn) {
		this->Service(http_handler_get, path, fn);
		return *this;
	}

	/**
	 * Post - Register a handler for HTTP POST requests
	 * 
	 * @param path The URL path to match (e.g., "/api/users")
	 * @param fn The handler function to execute when the path is matched
	 * @return Reference to this http_server for method chaining
	 */
	http_server& Post(const char* path, http_handler_t fn) {
		this->Service(http_handler_post, path, fn);
		return *this;
	}

	/**
	 * Head - Register a handler for HTTP HEAD requests
	 * 
	 * @param path The URL path to match (e.g., "/api/status")
	 * @param fn The handler function to execute when the path is matched
	 * @return Reference to this http_server for method chaining
	 */
	http_server& Head(const char* path, http_handler_t fn) {
		this->Service(http_handler_head, path, fn);
		return *this;
	}

	/**
	 * Put - Register a handler for HTTP PUT requests
	 * 
	 * @param path The URL path to match (e.g., "/api/users/123")
	 * @param fn The handler function to execute when the path is matched
	 * @return Reference to this http_server for method chaining
	 */
	http_server& Put(const char* path, http_handler_t fn) {
		this->Service(http_handler_put, path, fn);
		return *this;
	}

	/**
	 * Patch - Register a handler for HTTP PATCH requests
	 * 
	 * @param path The URL path to match (e.g., "/api/users/123")
	 * @param fn The handler function to execute when the path is matched
	 * @return Reference to this http_server for method chaining
	 */
	http_server& Patch(const char* path, http_handler_t fn) {
		this->Service(http_handler_patch, path, fn);
		return *this;
	}

	/**
	 * Connect - Register a handler for HTTP CONNECT requests
	 * 
	 * @param path The URL path to match
	 * @param fn The handler function to execute when the path is matched
	 * @return Reference to this http_server for method chaining
	 */
	http_server& Connect(const char* path, http_handler_t fn) {
		this->Service(http_handler_connect, path, fn);
		return *this;
	}

	/**
	 * Purge - Register a handler for HTTP PURGE requests
	 * 
	 * @param path The URL path to match
	 * @param fn The handler function to execute when the path is matched
	 * @return Reference to this http_server for method chaining
	 */
	http_server& Purge(const char* path, http_handler_t fn) {
		this->Service(http_handler_purge, path, fn);
		return *this;
	}

	/**
	 * Delete - Register a handler for HTTP DELETE requests
	 * 
	 * @param path The URL path to match (e.g., "/api/users/123")
	 * @param fn The handler function to execute when the path is matched
	 * @return Reference to this http_server for method chaining
	 */
	http_server& Delete(const char* path, http_handler_t fn) {
		this->Service(http_handler_delete, path, fn);
		return *this;
	}

	/**
	 * Options - Register a handler for HTTP OPTIONS requests
	 * 
	 * @param path The URL path to match
	 * @param fn The handler function to execute when the path is matched
	 * @return Reference to this http_server for method chaining
	 */
	http_server& Options(const char* path, http_handler_t fn) {
		this->Service(http_handler_options, path, fn);
		return *this;
	}

	/**
	 * Propfind - Register a handler for HTTP PROPFIND requests (WebDAV)
	 * 
	 * @param path The URL path to match
	 * @param fn The handler function to execute when the path is matched
	 * @return Reference to this http_server for method chaining
	 */
	http_server& Propfind(const char* path, http_handler_t fn) {
		this->Service(http_handler_profind, path, fn);
		return *this;
	}

	/**
	 * Websocket - Register a handler for WebSocket upgrade requests
	 * 
	 * @param path The URL path to match (e.g., "/ws/chat")
	 * @param fn The handler function to execute for WebSocket connections
	 * @return Reference to this http_server for method chaining
	 */
	http_server& Websocket(const char* path, http_handler_t fn) {
		this->Service(http_handler_websocket, path, fn);
		return *this;
	}

	/**
	 * Unknown - Register a handler for unknown/unsupported HTTP methods
	 * 
	 * @param path The URL path to match
	 * @param fn The handler function to execute for unknown methods
	 * @return Reference to this http_server for method chaining
	 */
	http_server& Unknown(const char* path, http_handler_t fn) {
		this->Service(http_handler_unknown, path, fn);
		return *this;
	}

	/**
	 * Error - Register a handler for error responses
	 * 
	 * @param path The URL path to match
	 * @param fn The handler function to execute for error handling
	 * @return Reference to this http_server for method chaining
	 */
	http_server& Error(const char* path, http_handler_t fn) {
		this->Service(http_handler_error, path, fn);
		return *this;
	}

public:
	// ========================================================================
	// Lifecycle Hooks - Register callbacks for process and thread lifecycle events
	// ========================================================================

	/**
	 * before_proc_jail - Register a callback to be called before process jail
	 * 
	 * This hook is called before the process is jailed (chroot/setuid).
	 * Use this to perform any initialization that requires elevated privileges.
	 * 
	 * @param fn The callback function to execute before jailing
	 * @return Reference to this http_server for method chaining
	 */
	http_server& before_proc_jail(proc_jail_t fn) {
		this->proc_jail_ = fn;
		return *this;
	}

	/**
	 * on_proc_init - Register a callback for process initialization
	 * 
	 * This hook is called when a worker process is initialized.
	 * Use this to set up process-level resources (e.g., database connections).
	 * 
	 * @param fn The callback function to execute on process initialization
	 * @return Reference to this http_server for method chaining
	 */
	http_server& on_proc_init(proc_init_t fn) {
		this->proc_init_ = fn;
		return *this;
	}

	/**
	 * on_proc_exit - Register a callback for process exit
	 * 
	 * This hook is called when a worker process is about to exit.
	 * Use this to clean up process-level resources.
	 * 
	 * @param fn The callback function to execute on process exit
	 * @return Reference to this http_server for method chaining
	 */
	http_server& on_proc_exit(proc_exit_t fn) {
		this->proc_exit_ = fn;
		return *this;
	}

	/**
	 * on_proc_sighup - Register a callback for SIGHUP signal
	 * 
	 * This hook is called when the process receives a SIGHUP signal.
	 * Use this to reload configuration or perform other signal-triggered actions.
	 * 
	 * @param fn The callback function to execute on SIGHUP
	 * @return Reference to this http_server for method chaining
	 */
	http_server& on_proc_sighup(proc_sighup_t fn) {
		this->proc_sighup_ = fn;
		return *this;
	}

	/**
	 * on_thread_init - Register a callback for thread initialization
	 * 
	 * This hook is called when a worker thread is initialized.
	 * Use this to set up thread-local resources.
	 * 
	 * @param fn The callback function to execute on thread initialization
	 * @return Reference to this http_server for method chaining
	 */
	http_server& on_thread_init(thread_init_t fn) {
		this->thread_init_ = fn;
		return *this;
	}

	/**
	 * on_thread_accept - Register a callback for connection acceptance
	 * 
	 * This hook is called when a thread accepts a new client connection.
	 * Use this to perform per-connection initialization or filtering.
	 * 
	 * @param fn The callback function to execute on connection acceptance
	 * @return Reference to this http_server for method chaining
	 */
	http_server& on_thread_accept(thread_accept_t fn) {
		this->thread_accept_ = fn;
		return *this;
	}
};

} // namespace acl

/**
 * ============================================================================
 * USAGE EXAMPLE
 * ============================================================================
 * 
 * #include "fiber/http_server.hpp"
 * 
 * int main() {
 *     acl::http_server server("127.0.0.1|6379", true);
 * 
 *     // Register route handlers using fluent API
 *     server.Get("/api/users", [](acl::HttpRequest& req, acl::HttpResponse& res) {
 *         std::string buf("{\"users\": [\"user1\", \"user2\"]}");
 *         res.setContentLength(buf.size());
 *         res.setStatus(200)
 *            .setContentType("application/json")
 *            .write(buf.c_str(), buf.size());
 *         return true;
 *     })
 *     .Post("/api/users", [](acl::HttpRequest& req, acl::HttpResponse& res) {
 *         // Handle POST request
 *         std::string buf("User created");
 *         res.setContentLength(buf.size());
 *         res.setStatus(201)
 *            .setContentType("text/plain")
 *            .write(buf.c_str(), buf.size());
 *         return true;
 *     })
 *     .Delete("/api/users/:id", [](acl::HttpRequest& req, acl::HttpResponse& res) {
 *         // Handle DELETE request
 *         std::string buf("User deleted");
 *         res.setContentLength(buf.size());
 *         res.setStatus(204)
 *            .setContentType("text/plain")
 *            .write(buf.c_str(), buf.size());
 *         return true;
 *     });
 * 
 *     // Register lifecycle hooks
 *     server.on_proc_init([](void* ctx) {
 *         printf("Process initialized\n");
 *     })
 *     .on_thread_init([](void* ctx) {
 *         printf("Thread initialized\n");
 *     });
 * 
 *     // Start the server
 *     server.run_alone("0.0.0.0:8080");
 * 
 *     return 0;
 * }
 * 
 * ============================================================================
 * KEY FEATURES
 * ============================================================================
 * 
 * 1. Fluent API - Chain method calls for clean, readable code
 * 2. Fiber-based - High concurrency with low memory overhead
 * 3. Full HTTP support - All standard HTTP methods supported
 * 4. WebSocket support - Built-in WebSocket upgrade handling
 * 5. Lifecycle hooks - Fine-grained control over server lifecycle
 * 6. Redis integration - Optional session management with Redis
 * 7. Process management - Multi-process architecture for scalability
 */
