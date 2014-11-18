#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include <map>

struct ACL_VSTREAM;

namespace acl {

class stream;

/**
 * 流对象 IO 注册回调类，子类需实现该类中的虚方法，子类对象通过 stream::setup_hook 注册，
 * 然后该子类对象中的 IO 过程作为 stream 类对象的底层 IO 过程被使用；
 * 如果不调用 stream::setup_hook 注册过程，则 stream 类对象的底层 IO 过程为默认过程
 * XXX： 本纯虚类被声明为堆对象类，建议子类也应该声明为堆对象类
 */
class ACL_CPP_API stream_hook
{
public:
	stream_hook() {}

	/**
	 * 读数据接口
	 * @param buf {void*} 读缓冲区地址，读到的数据将存放在该缓冲区中
	 * @param len {size_t} buf 缓冲区大小
	 * @return {int} 读到字节数，当返回值 < 0 时表示出错
	 */
	virtual int read(void* buf, size_t len) = 0;

	/**
	 * 发送数据接口
	 * @param buf {const void*} 发送缓冲区地址
	 * @param len {size_t} buf 缓冲区中数据的长度(必须 > 0)
	 * @return {int} 写入的数据长度，返回值 <０　时表示出错
	 */
	virtual int send(const void* buf, size_t len) = 0;

	/**
	 * 在 stream::setup_hook 内部将会调用 stream_hook::open 过程，以便于子类对象
	 * 用来初始化一些数据及会话
	 * @param s {stream*} 在 setup_hook 内部调用该方法将创建的流对象作为参数传入
	 * @return {bool} 如果子类实例返回 false，则 setup_hook 调用失败且会恢复原样
	 */
	virtual bool open(stream* s) = 0;

	/**
	 * 当 stream 流对象关闭前将会回调该函数以便于子类实例做一些善后工作
	 * @param alive {bool} 该连接是否依然正常
	 * @return {bool}
	 */
	virtual bool on_close(bool alive) { (void) alive; return true; }

	/**
	 * 当 stream 对象需要释放 stream_hook 子类对象时调用此方法
	 */
	virtual void destroy() {}

protected:
	virtual ~stream_hook() {}
};

class ACL_CPP_API stream
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
	ACL_VSTREAM* get_vstream() const;

	/**
	 * 解绑 ACL_VSTREAM 与流对象的绑定关系，同时将 ACL_VSTREAM 返回
	 * 给用户，即将该 ACL_VSTREAM的管理权交给用户，本流对象在释放时
	 * 不会关闭该 ACL_VSTREAM ，但用户接管该 ACL_VSTREAM 后用完后
	 * 必须将其关闭；解绑后除了 close/open 的调用有意义外，其它的调用
	 * (包括流对象读写在内)都无意义
	 * @return {ACL_VSTREAM} 返回 NULL 表示流对象已经将 ACL_VSTREAM 解绑
	 */
	ACL_VSTREAM* unbind();

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
	 * @param n {int} 超时时间(单位: 秒)
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
	stream_hook* get_hook() const;

	/**
	 * 删除当前注册的流读写对象并返回该对象，恢复缺省的读写过程
	 * @return {stream_hook*}
	 */
	stream_hook* remove_hook();

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
	bool eof_;
	bool opened_;

	void* default_ctx_;
	std::map<string, void*> ctx_table_;

private:
#ifdef WIN32
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
