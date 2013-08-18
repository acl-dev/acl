#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#ifdef WIN32
#include <WinSock2.h>
#endif
#include "acl_cpp/stream/istream.hpp"
#include "acl_cpp/stream/ostream.hpp"

struct ACL_VSTREAM;

namespace acl {

class ACL_CPP_API socket_stream
	: public istream
	, public ostream
{
public:
	socket_stream();
	virtual ~socket_stream();

	/**
	 * 根据套接字打开的一个网络流
	 * @param fd 套接字
	 * @return {bool} 连接是否成功
	 */
#ifdef	WIN32
	bool open(SOCKET fd);
#else
	bool open(int fd);
#endif

	/**
	 * 根据 ACL_VSTREAM 流打开网络流
	 * @param vstream {ACL_VSTREAM*}
	 * @return {bool} 连接是否成功
	 */
	bool open(ACL_VSTREAM* vstream);

	/**
	 * 连接远程服务器并打开网络连接流
	 * @param addr {const char*} 远程服务器监听地址, 格式: IP:PORT,
	 *  对UNIX平台, 该地址还可以为域套接口地址, 如: /tmp/mysock
	 * @param conn_timeout {int} 连接超时时间(秒)
	 * @param rw_timeout {int} 读写超时时间(秒)
	 * @return {bool} 连接是否成功
	 */
	bool open(const char* addr, int conn_timeout, int rw_timeout);

	/**
	 * 关闭网络连接流
	 * @return {bool} 关闭是否成功
	 */
	bool close();

	/**
	 * 获得网络连接流的套接字连接句柄
	 * @return {ACL_SOCKET} 若出错，则返回 - 1(UNIX 平台)
	 *  或 INVALID_SOCKET(win32平台)
	 */
#ifdef WIN32
	SOCKET sock_handle() const;
#else
	int   sock_handle() const;
#endif

	/**
	 * 解绑套接字与流对象的绑定关系，同时将套接字返回给用户，即
	 * 将该套接字的管理权交给用户，本流对象在释放时不会关闭该套
	 * 接字，但用户接管该套接字后用完后必须将其关闭
	 * 解绑后除了 close/open 的调用有意义外，其它的调用(包括流对
	 * 象读写在内)都无意义
	 * @return {ACL_SOCKET} 返回 ACL_SOCKET_INVALID 表示该流对象
	 *  已经将套接字解绑
	 */
#ifdef WIN32
	SOCKET unbind_sock();
#else
	int    unbind_sock();
#endif

	/**
	 * 获得远程连接的地址
	 * @param full {bool} 是否获得完整地址，即：IP:PORT，如果该参数
	 *  为 false，则仅返回 IP，否则返回 IP:PORT
	 * @return {const char*} 远程连接地址，若返回值 == '\0' 则表示
	 *  无法获得远程连接地址
	 */
	const char* get_peer(bool full = false) const;

	/**
	 * 获得远程连接的 IP 地址
	 * @return {const char*} 远程连接地址，若返回值 == '\0' 则表示
	 *  无法获得远程连接地址
	 */
	const char* get_peer_ip() const;

	/**
	 * 获得连接的本地地址
	 * @param full {bool} 是否获得完整地址，即：IP:PORT，如果该参数
	 *  为 false，则仅返回 IP，否则返回 IP:PORT
	 * @return {const char*} 该连接的本地地址，若返回值 == "" 则表示
	 *  无法获得本地地址
	 */
	const char* get_local(bool full = false) const;

	/**
	 * 获得连接的本地 IP 地址
	 * @return {const char*} 该连接的本地地址，若返回值 == "" 则表示
	 *  无法获得本地地址
	 */
	const char* get_local_ip() const;
protected:
private:
	char  dummy_[1];
	char  peer_ip_[33];
	char  local_ip_[33];
	const char* get_ip(const char* addr, char* buf, size_t size);
};

} // namespace acl
