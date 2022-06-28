#pragma once

class check_sync
{
public:
	check_sync(void);
	~check_sync(void);

	void sio_check_pop3(acl::check_client& checker,
		acl::socket_stream& conn);
	void sio_check_http(acl::check_client& checker,
		acl::socket_stream& conn);
};

