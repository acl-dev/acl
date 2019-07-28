#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include "noncopyable.hpp"
#include "string.hpp"

namespace acl {

/**
 * 双向输入输出缓冲管道流, 该流不仅可接收输入数据，同时还可输出所
 * 输入的数据，纯虚基类，子类需要实现三个接口函数
 */
class ACL_CPP_API pipe_stream : public noncopyable
{
public:
	pipe_stream() {}
	virtual ~pipe_stream() {}

	/**
	 * 数据输入输出接口
	 * @param in {const char*} 输入数据的地址
	 * @param len {size_t} 输入数据长度
	 * @param out {string*} 存储输出结果缓冲区，不能为空
	 * @param max {size_t} 希望接收到输出结果的长度限制，如果为0则
	 *  表示没有限制，输出结果都存储在 out 缓冲区中
	 * @return {int} 输出数据的长度，如果 < 0 则表示出错
	 */
	virtual int push_pop(const char* in, size_t len,
		string* out, size_t max = 0) = 0;

	/**
	 * 最后处理的输出数据接口
	 * @param out {string*} 存储输出结果缓冲区，不能为空
	 * @param max {size_t} 希望接收到输出结果的长度限制，如果为0则
	 *  表示没有限制，输出结果都存储在 out 缓冲区中
	 * @return {int} 输出数据的长度，如果 < 0 则表示出错
	 */
	virtual int pop_end(string* out, size_t max = 0) = 0;

	/**
	 * 清空内部缓冲区
	 */
	virtual void clear() {}
};

/**
 * 字符串处理双向管理流
 */
class ACL_CPP_API pipe_string : public pipe_stream
{
public:
	pipe_string();
	pipe_string(string& s);
	virtual ~pipe_string();

	// pipe_stream 基类中的四个虚函数
	virtual int push_pop(const char* in, size_t len,
		string* out, size_t max = 0);
	virtual int pop_end(string* out, size_t max = 0);
	virtual void clear()
	{
		m_pBuf->clear();
		m_pos = 0;
	}

	string& get_buf() const
	{
		return (*m_pBuf);
	}

	char* c_str() const
	{
		return (m_pBuf->c_str());
	}

	size_t length() const
	{
		return (m_pBuf->length());
	}

	bool empty() const
	{
		return (m_pBuf->empty());
	}

private:
	string* m_pBuf;
	string* m_pSavedBufPtr;
	size_t  m_pos;
};

/**
 * 管道流管理器，该类管理所有的管理流，将数据依次传递给所有管理流的
 * 输入接口，同时从所有管道流的输出接口中获得数据然后再将数据传递给
 * 下一个管道流的输入接口，以此类推，直到最后一个管道流
 */
class ACL_CPP_API pipe_manager : public noncopyable
{
public:
	pipe_manager();
	~pipe_manager();

	/**
	 * 以尾部添加的方式注册新的管道流处理器
	 * @param stream {pipe_stream*} 管道流处理器对象
	 * @return {bool} 如果该管道流处理器对象已经存在则返回 false
	 */
	bool push_back(pipe_stream* stream);

	/**
	 * 以头部添加的方式注册新的管道流处理器
	 * @param stream {pipe_stream*} 管道流处理器对象
	 * @return {bool} 如果该管道流处理器对象已经存在则返回 false
	 */
	bool push_front(pipe_stream* stream);

	/**
	 * 应用向管道流管理器添加新数据，由该管理器依次传递给所有已注册管道流
	 * 处理器，同时从已注册管道流处理器接收处理结果，依次传递给下一个
	 * @param src {const char*} 待处理的数据地址
	 * @param len {size_t} src 数据长度
	 * @param out {pipe_stream*} 如果非空，则该管道处理器将是最后一个只接收
	 *  输入而不进行输出的管道处理器
	 * @return {bool} 是否有错误发生
	 */
	bool update(const char* src, size_t len, pipe_stream* out = NULL);

	/**
	 * 最后必须调用一次该函数，以使有些管道的缓冲区里的数据可以一次性地
	 * 刷新至最后的管道中
	 * @param out {pipe_stream*} 如果非空，则该管道处理器将是最后一个只接收
	 *  输入而不进行输出的管道处理器
	 * @return {bool} 是否有错误发生
	 */
	bool update_end(pipe_stream* out = NULL);

	pipe_manager& operator<<(const string&);
	pipe_manager& operator<<(const string*);
	pipe_manager& operator<<(const char*);
#if defined(_WIN32) || defined(_WIN64)
	pipe_manager& operator<<(__int64);
	pipe_manager& operator<<(unsigned __int64);
#else
	pipe_manager& operator<<(long long int);
	pipe_manager& operator<<(unsigned long long int);
#endif
	pipe_manager& operator<<(long);
	pipe_manager& operator<<(unsigned long);
	pipe_manager& operator<<(int);
	pipe_manager& operator<<(unsigned int);
	pipe_manager& operator<<(short);
	pipe_manager& operator<<(unsigned short);
	pipe_manager& operator<<(char);
	pipe_manager& operator<<(unsigned char);

	char* c_str() const;
	size_t length() const;
	void clear();

private:
	std::list<pipe_stream*> m_streams;
	string* m_pBuf1, *m_pBuf2;
	pipe_string* m_pPipeStream;
};

} // namespace acl
