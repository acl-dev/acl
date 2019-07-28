#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

namespace acl {

class http_client;
class http_request;
class http_header;

class ACL_CPP_API http_download : public noncopyable
{
public:
	/**
	 * 构造函数
	 * @param url {const char*} 文件在服务器上的 url 地址
	 * @param addr {const char*} 非空时，设置服务器地址(格式为:
	 *  ip[|domain]:port，否则服务器地址从 url 中提取
	 */
	http_download(const char* url, const char* addr = NULL);
	virtual ~http_download();

	/**
	 * 在调用 run 之前可以通过本函数获得请求头对象，便于用户设置
	 * 自己的请求头字段(但 set_method/set_range 是内部自动设置的)
	 * @return {http_header*} 返回 NULL 表示输入的 URL 非法
	 */
	http_header* request_header() const;

	/**
	 * 调用此函数可以获得 http_request 对象，便于设置或查询请求头
	 * 或返回数据中的参数
	 * @return {http_request*} 返回 NULL 表示输入的 URL 非法
	 */
	http_request* request() const;

	/**
	 * 下载文件，当 range_from >= 0 且 range_to >= range_from 时自动
	 * 采用分段下载方式，否则采用全部下载方式
	 * @param range_from {acl_int64} 下载起始偏移位置，下标从 0 开始，
	 *  当该值 >= 0 且 range_to >= 本值时才采用分段下载方式
	 * @param range_to {acl_int64} 下载结束偏移位置
	 * @param req_body {const char*} 请求的数据体
	 * @param len {size_t} req_body 非空时指明其长度
	 * @return {bool} 下载是否成功，如果返回 true 则表示下载成功，否则
	 *  可能是输入参数非法，或 URL 不存在，或服务器不支持断点传输，或
	 *  在下载过程中子类返回 false 禁止继续下载
	 */
#if defined(_WIN32) || defined(_WIN64)
	bool get(__int64 range_from = -1, __int64 range_to = -1,
		const char* req_body = NULL, size_t len = 0);
#else
	bool get(long long int range_from = -1, long long int range_to = -1,
		const char* req_body = NULL, size_t len = 0);
#endif

	/**
	 * 重置内部请求状态
	 * @param url {const char*} 非空时则用此 URL 替代构造函数中输入的 URL,
	 *  否则依然使用构造函数中使用的 url
	 * @param addr {const char*} 非空时，设置服务器地址(格式为:
	 *  ip[|domain]:port，否则服务器地址从 url 中提取
	 * @return {bool} 返回 false 表示 url 非法
	 */
	bool reset(const char* url = NULL, const char* addr = NULL);

	/**
	 * 取得由构造函数或 reset 函数输入的 url
	 * @return {const char*} 返回 NULL 表示输入的 url 非法
	 */
	const char* get_url() const;

	/**
	 * 取得由构造函数或 reset 函数输入的 url 所得到的服务器地址，格式为：
	 * ip[|domain]:port
	 * @return {const char*} 返回 NULL 表示输入的 url 非法
	 */
	const char* get_addr() const;

protected:
	/**
	 * 当发送完 HTTP 请求数据后，读到 HTTP 服务器响应头后的回调函数
	 * @param conn {http_client*}
	 * @return {bool} 若子类返回 false 则停止继续下载
	 */
	virtual bool on_response(http_client* conn);

	/**
	 * 当得到服务器返回完整文件长度后的回调函数
	 * @param n {__int64} 完整文件长度
	 * @return {bool} 若子类返回 false 则停止继续下载
	 */
#if defined(_WIN32) || defined(_WIN64)
	virtual bool on_length(__int64 n);
#else
	virtual bool on_length(long long int n);
#endif

	/**
	 * 下载过程中，边下载边通知子类下载的数据及数据长度
	 * @param data {const void*} 下载的数据地址
	 * @param len {size_t} 下载的数据长度
	 * @return {bool} 若子类返回 false 则停止继续下载
	 */
	virtual bool on_save(const void* data, size_t len) = 0;

private:
	char* url_;
	char  addr_[128];
	http_request* req_;

	// 从头开始下载整个文件
	bool save_total(const char* body, size_t len);

	// 断点下载部分文件
#if defined(_WIN32) || defined(_WIN64)
	bool save_range(const char* body, size_t len,
		__int64 range_from, __int64 range_to);
#else
	bool save_range(const char* body, size_t len,
		long long int range_from, long long int range_to);
#endif

	// 开始下载
	bool save(http_request* req);
};

} // namespace acl
