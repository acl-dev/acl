#pragma once

class check_async : public acl::aio_callback
{
public:
	check_async(acl::check_client& checker);

protected:
	// 重载父类 aio_callback 中的虚函数

	/**
	 * 客户端流的读成功回调过程
	 * @param data {char*} 读到的数据地址
	 * @param len {int} 读到的数据长度
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流
	 */
	bool read_callback(char* data, int len);

	/**
	 * 客户端流的超时回调过程
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流
	 */
	bool timeout_callback();

	/**
	 * 客户端流的超时回调过程
	 */
	void close_callback();

private:
	acl::check_client& checker_;

	// 析构函数声明为私有方法，从而要求该对象在创建时是堆对象
	~check_async(void);
};
