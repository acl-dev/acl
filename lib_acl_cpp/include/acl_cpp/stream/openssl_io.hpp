#pragma once
#include "../acl_cpp_define.hpp"
#include "sslbase_io.hpp"

struct ACL_VSTREAM;
typedef struct ssl_st SSL;

namespace acl {

class openssl_conf;

class ACL_CPP_API openssl_io : public sslbase_io {
public:
	openssl_io(openssl_conf& conf, bool server_side, bool nblock = false);

	/*
	 * @override stream_hook
	 */
	void destroy(void);

	/**
	 * @override sslbase_io
	 * @return {bool}
	 */
	bool handshake(void);

protected:
	~openssl_io(void);

	// 实现 stream_hook 类的虚方法

	// @override stream_hook
	bool open(ACL_VSTREAM* s);

	// @override stream_hook
	bool on_close(bool alive);

	// @override stream_hook
	int read(void* buf, size_t len);

	// @override stream_hook
	int send(const void* buf, size_t len);

private:
	openssl_conf& conf_;
	SSL* ssl_;
};

} // namespace acl
