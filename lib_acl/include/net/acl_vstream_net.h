#ifndef ACL_VSTREAM_NET_INCLUDE_H
#define ACL_VSTREAM_NET_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include "../stdlib/acl_vstream.h"

/**
 * 监听某个地址（对于UNIX，还可以监听域套接字）
 * @param addr {const char*} 监听地址,
 *  如：127.0.0.1:80 或域套接字(UNIX平台) 如：/tmp/test.sock，在 Linux 平台下，
 *  如果 addr 的首字母为 '@'，则当作 astract unix domain path.
 * @param qlen {int} 监听队列的长度
 * @param flag {unsigned} 监听标志位，参见：ACL_INET_FLAG_XXX
 * @param io_bufsize {int} 接收的新的客户端套接字的IO缓冲区大小
 * @param rw_timeout {int} 接收的新的客户端套接字的IO读写超时时间，单位为秒
 * @return {ACL_VSTREAM*} 监听流指针
 */
ACL_API ACL_VSTREAM *acl_vstream_listen_ex(const char *addr, int qlen,
		unsigned flag, int io_bufsize, int rw_timeout);

/**
 * 监听某个地址（对于UNIX，还可以监听域套接字）
 * @param addr {const char*} 监听地址
 *  如：127.0.0.1:80, 或域套接字, 如：/tmp/test.sock，当地址为 ip:0 时则监听
 *  端口号由操作系统自动分配
 * @param qlen {int} 监听队列的长度
 * @return {ACL_VSTREAM*} 监听流指针
 */
ACL_API ACL_VSTREAM *acl_vstream_listen(const char *addr, int qlen);

/**
 * 从监听流中接收一个客户端连接流
 * @param listen_stream {ACL_VSTREAM*} 监听流
 * @param client_stream {ACL_VSTREAM*} 可重复利用的 ACL_VSTREAM 结构，
 *  如果为空则内部产生一个新的 ACL_VSTREAM 流，否则复用该结构空间
 * @param ipbuf {char*} 如果不为空则用来存储客户端的IP地址
 * @param bsize {int} 如果 ipbuf 不为空，则表示 ipbuf 的空间大小
 * @return {ACL_VSTREAM*} 如果不为空则表示新接收的客户端流
 */
ACL_API ACL_VSTREAM *acl_vstream_accept_ex(ACL_VSTREAM *listen_stream,
		ACL_VSTREAM *client_stream, char *ipbuf, int bsize);

/**
 * 从监听流中接收一个客户端连接流
 * @param listen_stream {ACL_VSTREAM*} 监听流
 * @param ipbuf {char*} 如果不为空则用来存储客户端的IP地址
 * @param bsize {int} 如果 ipbuf 不为空，则表示 ipbuf 的空间大小
 * @return {ACL_VSTREAM*} 如果不为空则表示新接收的客户端流
 */
ACL_API ACL_VSTREAM *acl_vstream_accept(ACL_VSTREAM *listen_stream,
		char *ipbuf, int bsize);

/**
 * 远程连接服务器
 * @param addr {const char*} 服务器地址, 如果连接一个域套接口服务器(仅UNIX平台),
 *  域套接地址：/tmp/test.sock，对于 Linux 平台，如果首字母为 '@' 则表示连接本
 *  地的抽象UNIX域套接口(Linux abstract unix domain socket)；
 *  如果连接一个TCP服务器，则地址格式为:
 *  remote_addr[@local_ip], 如: www.sina.com|80@60.28.250.199, 意思是绑定
 *  本的网卡地址为: 60.28.250.199, 远程连接 www.sina.com 的 80 端口, 如果由OS
 *  自动绑定本地 IP 地址，则可以写为：www.sina.com:80
 * @param block_mode {int} 阻塞连接还是非阻塞连接，ACL_BLOCKING, ACL_NON_BLOCKING
 * @param conn_timeout {int} 连接超时时间(秒)
 * @param rw_timeout {int} 连接流成功后的读写超时时间，单位为秒
 * @param bufsize {int} 连接流成功后的缓冲区大小
 * @param errorp {int*} 如果不为空，则存储连接失败后的错误号
 * @return {ACL_VSTREAM*} 如果不为空，则表示连接成功后的数据流
 */
ACL_API ACL_VSTREAM *acl_vstream_connect_ex(const char *addr, int block_mode,
		int conn_timeout, int rw_timeout, int bufsize, int *errorp);

/**
 * 远程连接服务器
 * @param addr {const char*} 服务器地址，含义同上
 * @param block_mode {int} 阻塞连接还是非阻塞连接，ACL_BLOCKING, ACL_NON_BLOCKING
 * @param connect_timeout {int} 连接超时时间(秒)
 * @param rw_timeout {int} 连接流成功后的读写超时时间，单位为秒
 * @param rw_bufsize {int} 连接流成功后的缓冲区大小
 * @return {ACL_VSTREAM*} 如果不为空，则表示连接成功后的数据流
 */
ACL_API ACL_VSTREAM *acl_vstream_connect(const char *addr, int block_mode,
		int connect_timeout, int rw_timeout, int rw_bufsize);

/**
 * 针对 UDP 通信，该函数用来绑定本地 UDP 地址，如果绑定成功，则创建
 * ACL_VSTREAM 对象, 用户可以象调用 ACL_VSTREAM 对象的读写接口
 * @param addr {const char*} 本地 UDP 地址，格式：ip:port，可以输入地址 ip:0
 *  来让操作系统自动分配本地端口号，此外还支持在 UNIX 平台下绑定 UNIX 域套接口，
 *  UNIX 域套接口的地址格式为：{path}@udp，其中 {path} 为域套接口路径，@udp 为
 *  UDP 后缀
 * @param rw_timeout {int} 读写超时时间(秒)
 * @param flag {unsigned} 标志位
 * @return {ACL_VSTREAM*} 返回 NULL 表示绑定失败
 */
ACL_API ACL_VSTREAM *acl_vstream_bind(const char *addr, int rw_timeout, unsigned flag);

/**
 * 将网络流对象设置为 UDP IO 模式
 * @param stream {ACL_VSTREAM*}
 */
ACL_API void acl_vstream_set_udp_io(ACL_VSTREAM *stream);

#ifdef __cplusplus
}
#endif

#endif

