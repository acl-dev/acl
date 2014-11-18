#pragma once
#include <set>
#include "acl_cpp/stream/aio_handle.hpp"

class CAioTimer : public acl::aio_timer_callback
{
public:
	CAioTimer(acl::aio_handle* handle, int id, bool keep);
	~CAioTimer();

protected:
	// »ùÀà´¿Ðéº¯Êý
	virtual void timer_callback(unsigned int id);
	virtual void destroy(void);
private:
	acl::aio_handle* handle_;
	int   id_;
	bool  inited_;
	std::set<int> tt_;
};

class CMemory
{
public:
	CMemory(void)
	{
		printf(">>in CMemory\n");
	}

	~CMemory(void)
	{
		printf(">> in ~CMemory\n");
	}

protected:
private:
};