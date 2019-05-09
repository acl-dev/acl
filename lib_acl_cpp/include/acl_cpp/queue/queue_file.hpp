#pragma once
#include <time.h>
#include "../stdlib/string.hpp"
#include "../stdlib/locker.hpp"
#include "../stdlib/noncopyable.hpp"
#include "queue_manager.hpp"

#ifndef MAXPATH255
#define MAXPATH255 255
#endif

namespace acl {

class fstream;

class ACL_CPP_API queue_file : public noncopyable
{
public:
	queue_file();

	/**
	 * 获得文件流指针
	 * @return {acl::fstream*} 文件流指针, 如果为 NULL 则说明文件还未打开
	 */
	fstream* get_fstream(void) const;

	/**
	 * 获得文件创建时间
	 * @return {time_t}, 返回自 1970 年以来的秒数, 如果返回值为 (time_t) -1,
	 *  则表示出错
	 */
	time_t get_ctime(void) const;

	/**
	 * 向文件中写数据
	 * @param data {const void*} 数据地址
	 * @param len {size} 数据长度
	 * @return {bool} 写数据是否成功
	 */
	bool write(const void* data, size_t len);
	int format(const char* fmt, ...) ACL_CPP_PRINTF(2, 3);
	int vformat(const char* fmt, va_list ap);

	/**
	 * 从文件中读取数据
	 * @param buf {void*} 缓存地址
	 * @param len {size_t} buf 大小
	 * @return {int} 读取的数据长度, -1: 表示读结束或读失败或输入参数错误,
	 *  应该关闭该文件对象, > 0: 表示成功
	 */
	int read(void* buf, size_t len);

	/**
	 * 取得本队列文件的键值, 该值即是队列文件的部分文件名(不含路径,
	 * 扩展名)
	 * @return {const char*} 队列文件键值
	 */
	const char* key(void) const
	{
		return m_partName;
	}

	/**
	 * 获得队列文件的访问全路径
	 * @return {const char*}
	 */
	const char* get_filePath(void) const
	{
		return m_filePath.c_str();
	}

	/**
	 * 获得队列文件的根路径部分(不含队列目录)
	 * @return {const char*}
	 */
	const char* get_home(void) const
	{
		return m_home;
	}

	/**
	 * 获得该队列文件的队列名
	 * @return {const char*} 队列名称
	 */
	const char* get_queueName(void) const
	{
		return m_queueName;
	}

	/**
	 * 获得队列子目录
	 * @return {const char*} 队列子目录名
	 */
	const char* get_queueSub(void) const
	{
		return m_queueSub;
	}

	/**
	 * 获得该队列文件的扩展名
	 * @return {const char*} 扩展名称
	 */
	const char* get_extName(void) const
	{
		return m_extName;
	}

	/**
	 * 获得已经写入的数据大小尺寸
	 * @return {size_t}
	 */
	size_t get_fileSize() const
	{
		return nwriten_;
	}

private:
	friend class queue_manager;

	~queue_file();

	/**
	 * 创建新的队列文件, 创建完毕后会自动创建该文件的加锁对象
	 * 可以直接调用该文件的 lock()/unlock()
	 * @param home {const char*} 队列文件所在根路径
	 * @param queueName {const char*} 队列名
	 * @param extName {const char*} 队列文件扩展名
	 * @param width {unsigned} 队列二级目录的个数
	 * @return {bool} 创建新队列文件是否成功, 如果返回 false 则说明
	 *  输入的 path 或 extName 非法
	 */
	bool create(const char* home, const char* queueName,
		const char* extName, unsigned width);

	/**
	 * 打开已经存在的队列文件, 打开后会自动创建该文件的加锁对象,
	 * 可以直接调用该文件的 lock()/unlock()
	 * @param filePath {const char*} 队列文件路径
	 * @return {bool} 打开队列文件是否成功
	 */
	bool open(const char* filePath);
	bool open(const char* home, const char* queueName, const char* queueSub,
		const char* partName, const char* extName);

	/**
	 * 关闭当前文件句柄
	 */
	void close();

	/**
	 * 从磁盘上删除本队列文件
	 * @return {bool} 删除是否成功
	 */
	bool remove();

	/**
	 * 将队列文件从当前队列中移至目标队列中
	 * @param queueName {const char*} 目标队列名称
	 * @param extName {const char*} 目标扩展名称
	 * @return {bool} 移动文件是否成功
	 */
	bool move_file(const char* queueName, const char* extName);

	/**
	 * 设置队列名
	 * @param queueName {const char*} 队列名称
	 */
	void set_queueName(const char* queueName);

	/**
	 * 设置队列文件的扩展名
	 */
	void set_extName(const char* extName);

	/**
	 * 对当前队列文件对象加锁(包括互斥锁及文件锁)
	 * @return {bool} 加锁是否成功
	 */
	bool lock(void);

	/**
	 * 对当前队列文件对象解锁(包括互斥锁及文件锁)
	 * @return {bool} 解锁是否成功
	 */
	bool unlock(void);

private:
	// 文件流对象
	fstream* m_fp;

	// 队列文件相对于队列根目录的全路径名
	string m_filePath;

	// 队列文件的根路径
	char  m_home[MAXPATH255];

	// 队列名称
	char  m_queueName[32];

	// 队列下的子目录
	char  m_queueSub[32];

	// 队列文件名, 不包括路径, 也不包括文件的扩展名
	char  m_partName[MAXPATH255];

	// 队列文件的扩展名
	char  m_extName[32];

	// 加锁对象
	locker m_locker;

	// 当前文件是否已经被加锁了
	bool  m_bLocked;

	// 文件锁是否已经打开了
	bool  m_bLockerOpened;

	// 已经写入的文件尺寸大小
	size_t nwriten_;
};

} // namespace acl
