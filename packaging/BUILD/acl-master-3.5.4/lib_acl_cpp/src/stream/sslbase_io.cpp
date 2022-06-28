#include "acl_stdafx.hpp"

#ifndef ACL_PREPARE_COMPILE
# include "acl_cpp/stdlib/snprintf.hpp"
# include "acl_cpp/stdlib/log.hpp"
# include "acl_cpp/stream/sslbase_conf.hpp"
# include "acl_cpp/stream/sslbase_io.hpp"
#endif

namespace acl
{

sslbase_io::sslbase_io(sslbase_conf& conf, bool server_side,
	bool nblock /* = false */)
: base_conf_(conf)
, server_side_(server_side)
, nblock_(nblock)
, handshake_ok_(false)
, stream_(NULL)
{
	refers_ = NEW atomic_long(0);
}

sslbase_io::~sslbase_io(void)
{
	delete refers_;
}

void sslbase_io::set_non_blocking(bool yes)
{
	// 此处仅设置非阻塞 IO 标志位，至于套接字是否被设置了非阻塞模式
	// 由应用自己来决定

	nblock_ = yes;
}

void sslbase_io::set_sni_host(const char *host)
{
	if (host && *host) {
		sni_host_ = host;
	}
}

} // namespace acl
