#pragma once

struct ACL_DBUF_POOL;

namespace acl
{

class dbuf_pool
{
public:
	dbuf_pool();
	~dbuf_pool();

	void* dbuf_alloc(size_t len);
	void* dbuf_calloc(size_t len);
	char* dbuf_strdup(const char* s);
	void* dbuf_memdup(const void* s, size_t len);

private:
	ACL_DBUF_POOL* pool_;
};

} // namespace acl
