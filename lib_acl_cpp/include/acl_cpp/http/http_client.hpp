#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

struct HTTP_HDR;
struct HTTP_HDR_RES;
struct HTTP_RES;
struct HTTP_HDR_REQ;
struct HTTP_REQ;

namespace acl {

class string;
class zlib_stream;
class socket_stream;
class ostream;
class istream;
class http_header;

/**
 * 该类的用处：1、当 HTTP 客户端向服务器请求数据时；2、当 HTTP 服务端接收
 * 到 HTTP 客户端连接时创建一个对应的 HTTP 客户端流对象
 * 该客户端流对象可以支持长连接
 */
class ACL_CPP_API http_client
{
public:
	/**
	 * 缺省的构造函数，使用此构造函数创建的 HTTP 客户端对象，需要显示地
	 * 调用 http_client::open 来打开数据流
	 */
	http_client(void);

	/**
	 * 根据已经连接成功的连接流对象创建 HTTP 客户端对象，但需要注意的是，
	 * 当该 http_client 对象销毁时，传入的 client 流对象并不会被销毁，需
	 * 要应用自己销毁，否则会造成资源泄露
	 * @param client {socket_stream*} HTTP 连接流对象，可以是请求端的流，
	 *  也可以是响应端的流；在本类对象被销毁时该流对象并不会被销毁，所以
	 *  用户需自行释放之
	 * @param is_request {bool} 是请求端还是响应端的客户端流
	 * @param unzip {bool} 当用来读取服务器的响应数据时，如果服务器返回的
	 *  数据体为压缩数据时，该参数控制在调用下面的函数时是否自动解压缩:
	 *  read_body(string&, bool, int*)
	 */
	http_client(socket_stream* client, bool is_request = false,
		bool unzip = true);

	virtual ~http_client(void);

	/**
	 * 在支持长连接的多次请求中，可以手工调用此函数清除中间的数据对象，
	 * 当然这不是必须的，因为在多次调用 read_head 时，read_head 会自动
	 * 调用 reset 来清除上次请求过程中的是间对象
	 */
	void reset(void);

	/**
	 * 连接远程 HTTP 服务器
	 * @param addr {const char*} 服务器地址，格式：IP:PORT 或 DOMAIN:PORT
	 * @param conn_timeout {int} 连接超时时间(秒)
	 * @param rw_timeout {int} 读写超时时间(秒)
	 * @param unzip {bool} 当服务器返回的数据体为压缩数据时是否自动解压缩
	 * @return {bool} 连接是否成功
	 */
	bool open(const char* addr, int conn_timeout = 60, int rw_timeout = 60,
		bool unzip = true);

	/**
	 * 写 HTTP 请求头数据至输出流中
	 * @param header {http_header&}
	 * @return {bool} 写头部数据是否成功
	 */
	bool write_head(const http_header& header);

	/**
	 * 发送 HTTP 数据体，可以循环调用此函数，当在第一次调用 write 函数写入
	 * HTTP 头时设置了 chunked 传输方式，则内部自动采用 chunked 传输方式; 
	 * 另外，在使用 chunked 方式传输数据时，应该最后再调用一次本函数，且参
	 * 数均设为 0 表示数据结束
	 * @param data {const void*} 数据地址
	 * @param len {size_t} data 数据长度
	 * @return {bool} 发送是否成功，如果返回 false 表示连接中断
	 */
	bool write_body(const void* data, size_t len);

	/**
	 * 当调用 http_client(socket_stream*, bool) 构造函数创建
	 * 或用 http_client(void) 构建同时调用 open 打开数据流时
	 * 可以调用本函数获得输出数据流句柄
	 * @return {ostream&} 返回输出流的引用，如果该流并不存在，
	 *  则内部自动会产生断言，提示使用者应先将流打开
	 */
	ostream& get_ostream(void) const;

	/**
	 * 当调用 http_client(socket_stream*, bool) 构造函数创建
	 * 或用 http_client(void) 构建同时调用 open 打开数据流时
	 * 可以调用本函数获得输入数据流句柄
	 * @return {istream&} 返回输入流的引用，如果该流并不存在，
	 *  则内部自动会产生断言，提示使用者应先将流打开
	 */
	istream& get_istream(void) const;

	/**
	 * 当调用 http_client(socket_stream*, bool) 构造函数创建
	 * 或用 http_client(void) 构建同时调用 open 打开数据流时
	 * 可以调用本函数获得数据流句柄
	 * @return {socket_stream&} 返回流的引用，如果该流并不存在，
	 *  则内部自动会产生断言，提示使用者应先将流打开
	 */
	socket_stream& get_stream(void) const;

	/**
	 * 从 HTTP 服务器读取响应头数据或从 HTTP 客户端读取请求数据，
	 * 在长连接的多次请求中，后续的请求会自动清除上次的中间数据对象
	 * @return {bool} 是否成功
	 */
	bool read_head(void);

	/**
	 * 获得 HTTP 请求的数据体或响应的数据体长度
	 * @return {int64) 返回值若为 -1 则表明 HTTP 头不存在或没有长度字段
	 */
#if defined(_WIN32) || defined(_WIN64)
	__int64 body_length(void) const;
#else
	long long int body_length(void) const;
#endif

	/**
	 * 当该对象为请求端流对象时，该函数将获得请求头中的长度起始地址及结束地址
	 * @param range_from {long long int&} 偏移起始位置
	 * @param range_to {long long int&} 偏移结束位置
	 * @return {bool} 若出错或非分段请求数据则返回 false；
	 *  若是分段请求则返回 true，同时给 range_from 和 range_to 赋值
	 *  注：range_from/range_to 下标从 0 开始
	 *  数据格式：
	 *  Range: bytes={range_from}-{range_to} 或
	 *  Range: bytes={range_from}-
	 */
#if defined(_WIN32) || defined(_WIN64)
	bool request_range(__int64& range_from, __int64& range_to);
#else
	bool request_range(long long int& range_from, long long int& range_to);
#endif

	/**
	 * 当该对象为响应端流对象时，该函数将获得响应头中的长度起始地址及结束地址
	 * @param range_from {long long int&} 偏移起始位置
	 * @param range_to {long long int&} 偏移结束位置
	 * @param total {long long int} 存放总长度
	 * @return {bool} 若出错或非分段响应数据则返回 false；
	 *  若是分段响应则返回 true，同时给 range_from 和 range_to 赋值
	 *  注：range_from/range_to 下标从 0 开始
	 *  数据格式：
	 *  Content-Range: bytes {range_from}-{range_to}/{total_length}
	 *  如：Content-Range: bytes 2250000-11665200/11665201
	 */ 
#if defined(_WIN32) || defined(_WIN64)
	bool response_range(__int64& range_from, __int64& range_to,
		__int64& total);
#else
	bool response_range(long long int& range_from,
		long long int& range_to, long long int& total);
#endif

	/**
	 * HTTP 数据流(请求流或响应流是否允许保持长连接)
	 * @return {bool}
	 */
	bool keep_alive(void) const;

	/**
	 * 获得 HTTP 请求头或响应头中某个字段名的字段值
	 * @param name {const char*} 字段名
	 * @return {const char*} 字段值，为空时表示不存在
	 */
	const char* header_value(const char* name) const;

	/**
	 * 禁止 HTTP 请求/响应头中的某些字段
	 * @param name {const char*} 字段名
	 */
	void header_disable(const char* name);

	/**
	 * 将 HTTP 头中的某个字段进行替换
	 * @param name {const char*} HTTP 头的字段名，如：Content-Length，
	 *   该字段不区分大小写
	 * @param value {const char*} 该头部字段的值
	 * @param force {bool} 如果该头部字段不存在是否需要强制添加
	 * @return {bool} 返回 false 表示输入出错，或头部字段名不存在且参数
	 *  force_add 为 false
	 */
	bool header_update(const char* name, const char* value,
		bool force_add = true);

	/**
	 * 将 HTTP 头中的某个字段中包含某个字符串的源字符串进行替换, 可以
	 * 支持多次匹配替换
	 * @param name {const char*} HTTP 头的字段名，如：Content-Length，
	 *   该字段不区分大小写
	 * @param match {const char*} 字段值中匹配的字符串
	 * @param to {const char*} 替换成的目标字符串值
	 * @param case_sensitive {bool} 在查找替换时是否区分大小写
	 * @return {int} 匹配替换的次数，0 表示未做任何替换，< 0 表示出错
	 */
	int header_update(const char* name, const char* match,
		const char* to, bool case_sensitive = false);

	/**
	 * 获得 HTTP 服务器返回的 HTTP 响应状态：
	 * 1xx, 2xx, 3xx, 4xx, 5xx
	 * @return {int} 若返回值为 -1 则表示出错，或该会话过程
	 *  不是向 HTTP 服务器请求数据过程
	 */
	int response_status(void) const;

	/**
	 * 获得 HTTP 客户端请求的 HOST 字段值
	 * @return {const char*} 返回 NULL 表示不存在该字段
	 */
	const char* request_host(void) const;

	/**
	 * 获得 HTTP 客户端请求的 PORT 端口号
	 * @return {int} 返回 -1 表示不存在
	 */
	int request_port(void) const;

	/**
	 * 获得 HTTP 客户端请求的 HTTP 方法：GET, POST, CONNECT
	 * @return {const char*} 返回值为空表示不存在
	 */
	const char* request_method(void) const;

	/**
	 * 获得 HTTP 客户端请求的 URL 中除去 HTTP://domain 后的内容
	 * 如：对于 http://test.com.cn/cgi-bin/test?name=value，则该
	 * 函数应该返回：/cgi-bin/test?name=value
	 * @return {const char*} 返回 NULL 表示不存在
	 */
	const char* request_url(void) const;

	/**
	 * 获得 HTTP 客户端请求的 URL 中的相对路径(不包含主机部分)，
	 * 如：对于 http://test.com.cn/cgi-bin/test?name=value，则该
	 * 函数应该返回：/path/test.cgi
	 * @return {const char*} 返回 NULL 表示不存在
	 */
	const char* request_path(void) const;

	/**
	 * 获得 HTTP 客户端请求的 URL 中的所有参数，如：
	 * http://test.com.cn/cgi-bin/test?name=value，则该函数应该返回：
	 * name=value
	 * @return {const char*} 返回 NULL 表示不存在
	 */
	const char* request_params(void) const;

	/**
	 * 获得 HTTP 客户端请求的 URL 中指定的参数值，如：
	 * http://test.com.cn/cgi-bin/test?name=value，则通过该函数可以
	 * 获得 name 参数的值为 value
	 * @param name {const char*} 参数名
	 * @return {const char*} 参数值，返回 NULL 表示不存在
	 */
	const char* request_param(const char* name) const;

	/**
	 * 获得 HTTP 客户端请求头中的 cookie 值
	 * @param name {const char*} cookie 名
	 * @return {const char*} cookie 值，返回 NULL 则表示不存在
	 */
	const char* request_cookie(const char* name) const;

	/**
	 * 从 HTTP 服务器读取响应体数据或从 HTTP 客户端读取请求体数据，
	 * 此函数将对收到的数据内容进行解压操作
	 * @param out {string&} 存储数据体的缓冲区
	 * @param clean {bool} 在接收数据前是否自动清空 buf 缓冲区
	 * @param real_size {int*} 若该指针非空，则记录真正读到的数据长度，
	 *  通过该指针返回的数据值永远 >= 0
	 * @return {int} 返回值含义如下：(应用需要通过 body_finish 函数和
	 *       disconnected 函数来判断数据体是否读完或连接是否关闭)
	 *  > 0: 表示已经读到的数据，并且数据还未读完
	 *  == 0: 有两种原因会返回 0，当数据读完时返回 0，可调用 body_finish
	 *        函数判断是否已经读完 HTTP 响应数据；当读到压缩数据的尾部时，
	 *        因压缩数据的8字节尾部数据是控制字段，所以不做为数据体返回，
	 *        此时也会返回 0；
	 *        还可以通过 disconnected() 函数判断连接是否已经被关闭
	 *        如果数据读完且连接半未关闭，则可以继续保持长连接
	 *  < 0: 表示连接关闭
	 * 注：read_body 的两个函数不能混用；
	 *     当为解压缩数据时，则返回的值为解压缩后的数据长度
	 */
	int read_body(string& out, bool clean = true, int* real_size = NULL);
	
	/**
	 * 从 HTTP 服务器读取响应体数据或从 HTTP 客户端读取请求体数据，
	 * 该函数不能对数据进行解压
	 * @param buf {char*} 存储数据体的缓冲区，不能为空
	 * @param size {size_t} buf 缓冲区长度
	 * @return {int} 返回值含义如下：
	 *  > 0: 表示已经读到的数据，并且数据还未读完
	 *  == 0: 表示已经读完 HTTP 响应体数据，但连接并未关闭
	 *  < 0: 表示连接关闭
	 */
	int read_body(char* buf, size_t size);

	/**
	 * 从 HTTP 服务器响应数据或客户端请求数据中读取一行数据，此函数内部将
	 * 会对原始数据进行解压操作；可以循环调用此函数直到该函数返回 false
	 * 或 body_finish() 返回 true 为止；当该函数返回 false 时表示连接已经
	 * 关闭，当返回 true 时表示读到了一行数据，此时可以通过判断
	 * body_finish() 返回值来判断是否已经读完了数据体
	 * @param out {string&} 存储数据体的缓冲区，在该函数内部不会自动清理该
	 *  缓冲区，用户可在调用该函数前自行清理该缓冲区(可调用:out.clear())
	 * @param nonl {bool} 读取一行数据时是否自动去掉尾部的 "\r\n" 或 "\n"
	 * @param size {size_t*} 当读到完整的一行数据时存放该行数据的长度，
	 *  当读到一个空行且 nonl 为 true 时，则该值为 0
	 * @return {bool} 是否读到了一行数据，当该函数返回 false 时表示读完毕
	 *  或读出错，且没有读到完整的一行数据；如果返回 true 表示读到了一行
	 *  数据，当读到一个空行时该函数也会返回 true，只是 *size = 0
	 */
	bool body_gets(string& out, bool nonl = true, size_t* size = NULL);

	/**
	 * 判断是否已经读完 HTTP 响应数据体
	 * @return {bool}
	 */
	bool body_finish(void) const;

	/**
	 * 判断网络连接是否已经关闭
	 * @return {bool}
	 */
	bool disconnected(void) const;

	/**
	 * 取得通过 read_head 读到的 HTTP 响应头对象，且当传入缓冲区
	 * 非空时，将 HTTP 响应头数据拷贝至缓冲区
	 * @param buf {string*} 非空时用来存储 HTTP 响应头数据
	 * @return {const HTTP_HDR_RES*} HTTP 响应头对象，如果为空，则说明
	 *  未读到响应头数据
	 */
	HTTP_HDR_RES* get_respond_head(string* buf);

	/**
	 * 取得通过 read_head 读到的 HTTP 请求头对象，且当传入缓冲区
	 * 非空时，将 HTTP 请求头数据拷贝至缓冲区
	 * @param buf {string*} 非空时用来存储 HTTP 请求头数据
	 * @return {const HTTP_HDR_REQ*} HTTP 请求头对象，如果为空，则说明
	 *  未读到请求头数据
	 */
	HTTP_HDR_REQ* get_request_head(string* buf);

	/**
	 * 输出服务器返回的 HTTP 响应头信息至标准输出
	 * @param prompt {const char*} 若非空则随同 HTTP 头信息一起输出
	 */
	void print_header(const char* prompt = NULL);

	/**
	 * 输出服务器返回的 HTTP 响应头信息至输出流中
	 * @param out {ostream&} 输出流，可以是文件流，也可以是网络流
	 * @param prompt {const char*} 若非空则随同 HTTP 头信息一起输出
	 */
	void fprint_header(ostream& out, const char* prompt = NULL);

	/**
	 * 输出服务器返回的 HTTP 响应头信息至缓冲区中
	 * @param out {string&} 存储结果的数据缓冲区
	 * @param prompt {const char*} 若非空则随同 HTTP 头信息一起输出
	 */
	void sprint_header(string& out, const char* prompt = NULL);

private:
	socket_stream* stream_;     // HTTP 数据流
	bool stream_fixed_;         // 是否允许释放 stream_ 流对象

	HTTP_HDR_RES* hdr_res_;     // HTTP 头响应对象
	struct HTTP_RES* res_;      // HTTP 响应对象
	HTTP_HDR_REQ* hdr_req_;     // HTTP 头请求对象
	struct HTTP_REQ* req_;      // HTTP 请求对象
	bool unzip_;                // 是否对压缩数据进行解压缩
	zlib_stream* zstream_;      // 解压对象
	bool is_request_;           // 是否是客户请求端
	int  gzip_header_left_;     // gzip 头剩余的长度
	int  last_ret_;             // 数据读完后记录最后的返回值
	bool head_sent_;            // 头部数据是否已经发送完毕
	bool body_finish_;          // 是否已经读完 HTTP 响应体数据
	bool disconnected_;         // 网络连接是否已经关闭
	bool chunked_transfer_;     // 是否为 chunked 传输模式
	unsigned gzip_crc32_;       // gzip 压缩数据时的检验值
	unsigned gzip_total_in_;    // gzip 压缩前的总数据长度      
	string* buf_;               // 内部缓冲区，用在按行读等操作中

	bool read_request_head(void);
	bool read_response_head(void);
	int  read_request_body(char* buf, size_t size);
	int  read_response_body(char* buf, size_t size);
	int  read_request_body(string& out, bool clean, int* real_size);
	int  read_response_body(string& out, bool clean, int* real_size);

	HTTP_HDR* get_http_hdr() const;

public:
	bool write_chunk(ostream& out, const void* data, size_t len);
	bool write_chunk_trailer(ostream& out);

	bool write_gzip(ostream& out, const void* data, size_t len);
	bool write_gzip_trailer(ostream& out);
};

}  // namespace acl
