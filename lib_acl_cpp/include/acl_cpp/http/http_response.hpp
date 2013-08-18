#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/http/http_header.hpp"

namespace acl {

class http_client;
class http_pipe;
class socket_stream;
class xml;
class json;

class ACL_CPP_API http_response
{
public:
	/**
	 * 构造函数：通过该构造函数传入的 socket_stream 流对象并
	 * 不会被关闭，需要调用者自己关闭
	 * @param client {socket_stream*} 数据连接流，非空
	 * 注：该类实例在长连接时可以被多次使用，但一定得注意使用
	 * 顺序：get_body->response
	 */
	http_response(socket_stream* client);
	virtual ~http_response(void);

	//////////////////////////////////////////////////////////////////////////
	// 与读取请求数据相关的方法

	/**
	 * 读取 HTTP 请求客户端的 HTTP 请求头，在调用本方法后才可以调用 get_body
	 * @return {bool} 是否成功
	 */
	bool read_header();

	/**
	 * 读取 xml 格式的 HTTP 请求数据体，调用完此函数后应该调用
	 * response 给客户端返回响应数据；该函数每次被调用时，内部的
	 * 对象会被初始化，所以该函数可以在多个会话中被多次调用
	 * @param out {xml&} 将 HTTP 请求体数据存储于该 xml 对象中
	 * @param to_charset {const char*} 当该项非空，内部自动
	 *  将数据转成该字符集存储于 xml 对象中
	 * @return {bool} 是否正常
	 *  注：必须首先调用 read_header 后才能调用本函数
	 *      当请求数据体特别大时不应用此函数，以免内存耗光
	 */
	bool get_body(xml& out, const char* to_charset = NULL);

	/**
	 * 读取 json 格式的 HTTP 请求数据体，调用完此函数后应该调用
	 * response 给客户端返回响应数据；该函数每次被调用时，内部的
	 * 对象会被初始化，所以该函数可以在多个会话中被多次调用
	 * @param out {json&} 将 HTTP 请求体数据存储于该 json 对象中
	 * @param to_charset {const char*} 当该项非空，内部自动
	 *  将数据转成该字符集存储于 json 对象中
	 * @return {bool} 是否正常
	 *  注：必须首先调用 read_header 后才能调用本函数
	 *      当请求数据体特别大时不应用此函数，以免内存耗光
	 */
	bool get_body(json& out, const char* to_charset = NULL);

	/*
	 * 读取 HTTP 全部请求体数据并存储于输入的缓冲区中
	 * @param out {string&} 存储请求数据体
	 * @param to_charset {const char*} 当该项非空，内部自动
	 *  将数据转成该字符集存储于 out 对象中
	 * 注：当请求数据体特别大时不应用此函数，以免内存耗光
	 *     必须首先调用 read_header 后才能调用本函数
	 */
	bool get_body(string& out, const char* to_charset = NULL);

	/*
	 * 读取 HTTP 请求体数据并存储于输入的缓冲区中，可以循环
	 * 调用本函数，直至数据读完了，
	 * @param buf {char*} 存储部分请求体数据
	 * @param size {size_t} buf 缓冲区大小
	 * @return {int} 返回值 == 0 表示正常读完毕，< 0 表示客户端
	 *  关闭连接，> 0 表示已经读到的数据，用户应该一直读数据直到
	 *  返回值 <= 0 为止
	 *  注：该函数读到的是原始 HTTP 数据体数据，不做解压和字符集
	 *      解码，用户自己根据需要进行处理
	 *      必须首先调用 read_header 后才能调用本函数
	 */
	int get_body(char* buf, size_t size);

	//////////////////////////////////////////////////////////////////////////
	// 与数据响应相关的方法函数

	/**
	 * 获得 HTTP 响应头对象，然后在返回的 HTTP 响应头对象中添加
	 * 自己的响应头字段或 http_header::reset()重置响应头状态，
	 * 参考：http_header 类
	 * @return {http_header&}
	 */
	http_header& response_header(void);

	/**
	 * 向 HTTP 客户端发送响应数据，在调用本函数前，必须先调用
	 * get_body 获得 HTTP 客户端的请求数据
	 * @param body {const char*} 数据体地址
	 * @param len {size_t} body 非空时表明 body 数据长度
	 * @param status {int} HTTP 响应码：1xx, 2xx, 3xx, 4xx, 5xx
	 * @param keep_alive {bool} 是否与客户端保持长连接
	 * @return {bool} 发送是否成功
	 */
	bool response(const char* body, size_t len,
		int status = 200, bool keep_alive = true);

	//////////////////////////////////////////////////////////////////////////

	/**
	 * 获得 http_client HTTP 连接流，可以通过返回的对象获得
	 * 客户端请求头的部分数据，参考：http_client 类
	 * @return {http_client*} 当返回空时表示流出错了
	 */
	http_client* get_client(void) const;

	/**
	 * 关闭 HTTP 连接流
	 */
	void close(void);
protected:
private:
	bool debug_;
	bool header_ok_;
	http_client* client_;
	http_header  header_;
	http_pipe* get_pipe(const char* to_charset);
};

} // namespace acl
