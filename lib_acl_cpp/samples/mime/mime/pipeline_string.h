#pragma once
#include "acl_cpp/stdlib/pipe_stream.hpp"

//class acl::string;

class pipeline_string : public acl::pipe_stream
{
public:
	pipeline_string();
	~pipeline_string();

protected:
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
		acl::string* out, size_t max = 0);

	/**
	* 最后处理的输出数据接口
	* @param out {string*} 存储输出结果缓冲区，不能为空
	* @param max {size_t} 希望接收到输出结果的长度限制，如果为0则
	*  表示没有限制，输出结果都存储在 out 缓冲区中
	* @return {int} 输出数据的长度，如果 < 0 则表示出错
	*/
	virtual int pop_end(acl::string* out, size_t max = 0);

	/**
	* 清空内部缓冲区
	*/
	virtual void clear();
private:
	bool strip_sp_;
};
