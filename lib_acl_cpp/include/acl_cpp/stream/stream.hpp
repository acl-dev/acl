#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"
#include <map>

struct ACL_VSTREAM;

namespace acl {

class stream_hook;
class dbuf_pool;

class ACL_CPP_API stream : public noncopyable
{
public:
	stream(void);
	virtual ~stream(void) = 0;

	/**
	 * 调用本函数关闭流连接
	 * @return {bool} true: 关闭成功; false: 关闭失败
	 */
	bool close(void);

	/**
	* 判断流是否已经结束
	* @return {bool} true: 流已经结束; false: 流未结束
	*/
	bool eof(void) const;

	/**
	 * 清除流结束标志位，即将 eof_ 标志位置为 false
	 */
	void clear_eof(void);

	/**
	* 当前流是否处理打开状态
	* @return {bool} true: 流已经打开; false: 流未打开
	*/
	bool opened(void) const;

	/**
	 * 获得当前流的 ACL_VSTREAM 流对象
	 * @return {ACL_VSTREAM*}
	 */
	ACL_VSTREAM* get_vstream(void) const;

	/**
	 * 解绑 ACL_VSTREAM 与流对象的绑定关系，同时将 ACL_VSTREAM 返回
	 * 给用户，即将该 ACL_VSTREAM的管理权交给用户，本流对象在释放时
	 * 不会关闭该 ACL_VSTREAM ，但用户接管该 ACL_VSTREAM 后用完后
	 * 必须将其关闭；解绑后除了 close/open 的调用有意义外，其它的调用
	 * (包括流对象读写在内)都无意义
	 * @return {ACL_VSTREAM} 返回 NULL 表示流对象已经将 ACL_VSTREAM 解绑
	 */
	ACL_VSTREAM* unbind(void);

	/**
	 * 设置流的绑定对象
	 * @param ctx {void*}
	 * @param key {const char* } 标识该 ctx 的键
	 * @param replace {bool} 当对应的 KEY 存在时是否允许覆盖
	 * @return {bool} 当 replace 为 false 且 key 已经存在时则返回 false
	 */
	bool set_ctx(void* ctx, const char* key = NULL, bool replace = true);

	/**
	 * 获得与流绑定的对象
	 * @param key {const char* key} 非空时使用该 key 查询对应的 ctx 对象，
	 *  否则返回缺省的 ctx 对象
	 * @return {void*}
	 */
	void* get_ctx(const char* key = NULL) const;

	/**
	 * 删除流中绑定的对象
	 * @param key {const char*} 非空时删除对应该 key 的 ctx 对象，否则删除
	 *  缺省的 ctx 对象
	 * @return {void*} 当对象不存在时返回 NULL，成功删除后返回该对象
	 */
	void* del_ctx(const char* key = NULL);

	/**
	 * 设置流的读写超时时间
	 * @param n {int} 超时时间(单位: 秒)，该值 > 0 则启用超时检测过程，否则将会
	 *  一直阻塞直到可读或出错
	 */
	void set_rw_timeout(int n);

	/**
	 * 获得当前流的读写超时时间
	 * @return {int} 获得流的读写超时时间(秒)
	 */
	int get_rw_timeout(void) const;

	/**
	 * 注册读写流对象，内部会自动调用 hook->open 过程，如果成功，则返回之前注册的对象
	 * (可能为NULL)，若失败则返回与输入参数相同的指针，应用可以通过判断返回值与输入值
	 * 是否相同来判断注册流对象是否成功
	 * xxx: 在调用此方法前必须保证流连接已经创建
	 * @param hook {stream_hook*} 非空对象指针
	 * @return {stream_hook*} 返回值与输入值不同则表示成功
	 */
	stream_hook* setup_hook(stream_hook* hook);

	/**
	 * 获得当前注册的流读写对象
	 * @return {stream_hook*}
	 */
	stream_hook* get_hook(void) const;

	/**
	 * 删除当前注册的流读写对象并返回该对象，恢复缺省的读写过程
	 * @return {stream_hook*}
	 */
	stream_hook* remove_hook(void);

public:
	/**
	 * 因为 stream 的生命周期较长，使用者使用 stream 对象中的内部缓存区可以
	 * 适当减少缓存区的频繁创建与释放
	 * @return {string&}
	 */
	string& get_buf(void);

	/**
	 * 获得与 stream 生命周期相同的 dbuf 内存分配器
	 * @return {dbuf_pool&}
	 */
	dbuf_pool& get_dbuf(void);

protected:
	/**
	 * 打开流对象，如果流已经打开，则不会重复打开
	 */
	void open_stream(bool is_file = false);

	/**
	 * 重新打开流对象，如果流已经打开则先释放流对象再打开
	 */
	void reopen_stream(bool is_file = false);

protected:
	stream_hook* hook_;
	ACL_VSTREAM *stream_;
	string* buf_;
	dbuf_pool* dbuf_;

	void* default_ctx_;
	std::map<string, void*>* ctx_table_;

	bool eof_;
	bool opened_;

private:
#if defined(_WIN32) || defined(_WIN64)
	static int read_hook(SOCKET fd, void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
	static int send_hook(SOCKET fd, const void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);

	static int fread_hook(HANDLE fd, void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
	static int fsend_hook(HANDLE fd, const void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
#else
	static int read_hook(int fd, void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
	static int send_hook(int fd, const void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);

	static int fread_hook(int fd, void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
	static int fsend_hook(int fd, const void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
#endif
};

} // namespace acl
