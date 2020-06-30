#pragma once

class http_service
{
public:
	http_service(void) {}
	~http_service(void) {}

public:
	// Register all Http handlers with the http url path

	http_service& Get(const char* path, http_handler_t fn)
	{
		Service(http_handler_get, path, fn);
		return *this;
	}

	http_service& Post(const char* path, http_handler_t fn)
	{
		Service(http_handler_post, path, fn);
		return *this;
	}

	http_service& Head(const char* path, http_handler_t fn)
	{
		Service(http_handler_head, path, fn);
		return *this;
	}

	http_service& Put(const char* path, http_handler_t fn)
	{
		Service(http_handler_put, path, fn);
		return *this;
	}

	http_service& Patch(const char* path, http_handler_t fn)
	{
		Service(http_handler_patch, path, fn);
		return *this;
	}

	http_service& Connect(const char* path, http_handler_t fn)
	{
		Service(http_handler_connect, path, fn);
		return *this;
	}

	http_service& Purge(const char* path, http_handler_t fn)
	{
		Service(http_handler_purge, path, fn);
		return *this;
	}

	http_service& Delete(const char* path, http_handler_t fn)
	{
		Service(http_handler_delete, path, fn);
		return *this;
	}

	http_service& Options(const char* path, http_handler_t fn)
	{
		Service(http_handler_options, path, fn);
		return *this;
	}

	http_service& Propfind(const char* path, http_handler_t fn)
	{
		Service(http_handler_profind, path, fn);
		return *this;
	}

	http_service& Websocket(const char* path, http_handler_t fn)
	{
		Service(http_handler_websocket, path, fn);
		return *this;
	}

	http_service& Unknown(const char* path, http_handler_t fn)
	{
		Service(http_handler_unknown, path, fn);
		return *this;
	}

	http_service& Error(const char* path, http_handler_t fn)
	{
		Service(http_handler_error, path, fn);
		return *this;
	}

public:
	http_handlers_t* get_handlers(void)
	{
		return handlers_;
	}

private:
	http_handlers_t handlers_[http_handler_max];

	void Service(int type, const char* path, http_handler_t fn)
	{
		if (type >= http_handler_get && type < http_handler_max
				&& path && *path) {

			// The path should lookup like as "/xxx/" with
			// lower charactors.

			acl::string buf(path);
			if (buf[buf.size() - 1] != '/') {
				buf += '/';
			}
			buf.lower();
			handlers_[type][buf] = fn;
		}
	}
};
