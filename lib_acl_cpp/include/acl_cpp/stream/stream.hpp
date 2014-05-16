#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

struct ACL_VSTREAM;

namespace acl {

class ACL_CPP_API stream
{
public:
	stream(void);
	virtual ~stream(void) = 0;

	/**
	* 纯虚函数, 本类不能直接被实例化, 要求子类必须实现流关闭函数
	* @return {bool} true: 关闭成功; false: 关闭失败
	*/
	virtual bool close(void) = 0;

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
	 * 打开流对象，如果流已经打开，则不会重复打开
	 */
	void open_stream(void);

	/**
	 * 重新打开流对象，如果流已经打开则先释放流对象再打开
	 */
	void reopen_stream(void);

	/**
	 * 设置流的绑定对象
	 * @param ctx {void*}
	 */
	void set_ctx(void* ctx);

	/**
	 * 获得与流绑定的对象
	 * @return {void*}
	 */
	void* get_ctx() const;

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

protected:
	ACL_VSTREAM *stream_;
	bool eof_;
	bool opened_;

	void* ctx_;
};

} // namespace acl
