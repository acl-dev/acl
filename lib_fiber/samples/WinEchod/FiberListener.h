#pragma once
#include "fiber/lib_fiber.hpp"

class CListener : public acl::fiber
{
public:
	CListener(acl::server_socket& listener);

protected:
	// @override
	void run(void);

private:
	~CListener(void);

	acl::server_socket& m_listener;
};

