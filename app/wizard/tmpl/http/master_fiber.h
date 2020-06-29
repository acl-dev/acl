#pragma once

class http_service : public acl::master_fiber
{
public:
	http_service(void);
	~http_service(void);

public:
	// Register all Http handlers with the http url path

	http_service& Get(const char* path, http_handler_t fn);
	http_service& Post(const char* path, http_handler_t fn);
	http_service& Head(const char* path, http_handler_t fn);
	http_service& Put(const char* path, http_handler_t fn);
	http_service& Patch(const char* path, http_handler_t fn);
	http_service& Connect(const char* path, http_handler_t fn);
	http_service& Purge(const char* path, http_handler_t fn);
	http_service& Delete(const char* path, http_handler_t fn);
	http_service& Options(const char* path, http_handler_t fn);
	http_service& Propfind(const char* path, http_handler_t fn);
	http_service& Websocket(const char* path, http_handler_t fn);
	http_service& Unknown(const char* path, http_handler_t fn);
	http_service& Error(const char* path, http_handler_t fn);

protected:
	// @override
	void on_accept(acl::socket_stream& conn);

	// @override
	void proc_pre_jail(void);

	// @override
	void proc_on_listen(acl::server_socket& ss);

	// @override
	void proc_on_init(void);

	// @override
	void proc_on_exit(void);

	// @override
	bool proc_on_sighup(acl::string&);

private:
	http_handlers_t handlers_[http_handler_max];

	void Service(int type, const char* path, http_handler_t fn);
};
