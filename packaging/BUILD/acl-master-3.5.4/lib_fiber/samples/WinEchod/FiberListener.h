#pragma once
#include "fiber/lib_fiber.hpp"

class CFiberListener : public acl::fiber
{
public:
	CFiberListener(acl::server_socket& listener);

protected:
	// @override
	void run(void);

private:
	acl::server_socket& m_listener;
	~CFiberListener(void);
};

