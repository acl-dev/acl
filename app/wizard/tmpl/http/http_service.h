#pragma once

class http_service
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
	http_service& Default(http_default_handler_t fn);

public:
	bool doService(int type, HttpRequest& req, HttpResponse& res);

public:
	http_handlers_t* get_handlers(void)
	{
		return handlers_;
	}

private:
	http_handlers_t handlers_[http_handler_max];
	http_default_handler_t  handler_default_;

	void Service(int type, const char* path, http_handler_t fn);
};
