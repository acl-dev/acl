#ifndef	__LIB_HTTP_INCLUDE_H__
#define	__LIB_HTTP_INCLUDE_H__

#include "lib_http_status.h"
#include "lib_http_struct.h"

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------- 通用 HTTP 头操作函数 --------------------------*/
/* in http_hdr.c */

/**
 * 生成一个通用HTTP协议头的结构对象
 * @param size {size_t} 所需分配内存大小, 等于 HTTP_HDR_REQ 或 HTTP_HDR_RES 的尺寸
 * @return {HTTP_HDR*} !NULL: 返回一个HTTP_HDR结构指针; NULL: 出错.
 */
HTTP_API HTTP_HDR *http_hdr_new(size_t size);

/**
 * 从源HTTP通用头拷贝内部成员变量至一个新的HTTP通用头结构中
 * @param src {const HTTP_HDR*} 源HTTP通用头对象，不能为空
 * @param dst {HTTP_HDR*} 目的HTTP通用头对象，不能为空
 */
HTTP_API void http_hdr_clone(const HTTP_HDR *src, HTTP_HDR *dst);

/**
 * 释放一个HTTP_HDR结构内存
 * @param hh {HTTP_HDR*} 类型的数据指针，不能为空
 */
HTTP_API void http_hdr_free(HTTP_HDR *hh);

/**
 * 重置一个HTTP通用头的状态，释放内部成员变量，主要用于keep-alive的长连接多次请求
 * @param hh {HTTP_HDR*} HTTP通用头类型的数据指针，不能为空
 */
HTTP_API void http_hdr_reset(HTTP_HDR *hh);

/**
 * 向 HTTP_HDR 头中增加一个条目
 * @param hh {HTTP_HDR*} 通用头类型的数据指针，不能为空
 * @param entry {HTTP_HDR_ENTRY*} HTTP头条目结构指针, 不能为空
 */
HTTP_API void http_hdr_append_entry(HTTP_HDR *hh, HTTP_HDR_ENTRY *entry);

/**
 * 分析所给数据, 解析出协议, 主/次版本号，并将结果存在通用HTTP头结构内
 * @param hh {HTTP_HDR*} 类型的数据指针，不能为空
 * @param data {const char*} 数据格式须为: HTTP/1.0
 * @return {int} 0: OK; < 0: error.
 */
HTTP_API int http_hdr_parse_version(HTTP_HDR *hh, const char *data);

/**
 * 分析所有的通用HTTP协议头并存储在 hh 结构中
 * @param hh {HTTP_HDR*} 通用HTTP头类型的数据指针，不能为空
 * @return {int} 0: ok; < 0: error
 */
HTTP_API int http_hdr_parse(HTTP_HDR *hh);

/**
 * 由传入的 name, value 对产生一个 HTTP_HDR_ENTRY 对象
 * @param name {const char*} 变量名
 * @param value {const char*} 变量值
 * @return {HTTP_HDR_ENTRY*} 如果为空则是因为输入参数有误
 */
HTTP_API HTTP_HDR_ENTRY *http_hdr_entry_build(const char *name, const char *value);

/**
 * 根据传入的一行数据进行分析, 生成一个 HTTP_HDR_ENTRY
 * @param data {const char*} HTTP 协议头中的一行数据, 如: Content-Length: 200
 * @return {HTTP_HDR_ENTRY*} !NULL: ok; NULL: err.
 */
HTTP_API HTTP_HDR_ENTRY *http_hdr_entry_new(const char *data);
HTTP_API HTTP_HDR_ENTRY *http_hdr_entry_head(char *data);
HTTP_API HTTP_HDR_ENTRY *http_hdr_entry_new2(char *data);

/**
 * 获取一个 HTTP_HDR_ENTRY 条目
 * @param hh {HTTP_HDR*} 通用HTTP头类型的数据指针，不能为空
 * @param name {const char*} 该 HTTP_HDR_ENTRY 条目的标识名, 不能为空. 如: Content-Length.
 * @return ret {HTTP_HDR_ENTRY *} ret != NULL: ok; ret == NULL: 出错或不存在.
 */
HTTP_API HTTP_HDR_ENTRY *http_hdr_entry(const HTTP_HDR *hh, const char *name);

/**
 * 获取HTTP协议头里某个实体头的变量值，如某个实体头为：Host: www.test.com
 * 要获得 Host 变量的值，调用该函数后便可以取得 www.test.com
 * @param hh {HTTP_HDR*} 通用HTTP头类型的数据指针，不能为空
 * @param name {const char*} 该 HTTP_HDR_ENTRY 条目的标识名, 不能为空. 如: Content-Length
 * @return ret {char*} ret != NULL: ok; ret == NULL: 出错或不存在.
 */
HTTP_API char *http_hdr_entry_value(const HTTP_HDR *hh, const char *name);

/**
 * 将 HTTP 头中的某个字段进行替换, 该功能起初主要是为了实现 keep-alive 字段的替换
 * @param hh {HTTP_HDR*} 通用HTTP头类型的数据指针，不能为空
 * @param name {const char*} 该 HTTP_HDR_ENTRY 条目的标识名, 不能为空. 如: Content-Length
 * @param value {const char*} 该 name 字段所对应的新的值
 * @param force {int} 如果所替换的字段在原始HTTP请求里不存在, 则强行产生新的 entry 字段并
 *  填至该请求头, 当该值为非0值时进行强行添加, 否则若该name在请求里不存在则不添加.
 * @return {int} 0 表示替换成功; < 0 表示输入参数出错或该 name 字段在该HTTP请求头里不存在
 */
HTTP_API int http_hdr_entry_replace(HTTP_HDR *hh, const char *name, const char *value, int force);

/**
 * 将 HTTP 头中的某个字段中包含某个字符串的源字符串进行替换, 可以支持多次匹配替换
 * @param hh {HTTP_HDR*} 通用HTTP头类型的数据指针，不能为空
 * @param name {const char*} 该 HTTP_HDR_ENTRY 条目的标识名, 不能为空. 如: Cookie
 * @param from {const char*} 替换时的源字符串
 * @param to {const char*} 替换时的目标字符串
 * @param ignore_case {int} 在查找替换时是否忽略源字符串的大小写
 * @return {int} 0: 表示未做任何替换, > 0: 表示替换的次数
 */
HTTP_API int http_hdr_entry_replace2(HTTP_HDR *hh, const char *name,
        const char *from, const char *to, int ignore_case);

/**
 * 禁止HTTP协议头中的某项
 * @param hh {HTTP_HDR* } 通用HTTP头类型的数据指针，不能为空
 * @param name {const char*} 该 HTTP_HDR_ENTRY 条目的标识名, 不能为空. 如: Content-Length
 */
HTTP_API void http_hdr_entry_off(HTTP_HDR *hh, const char *name);

/**
 * 调试输出HTTP协议头部数据，调试类接口
 * @param hh {HTTP_HDR*} 通用HTTP头类型的数据指针，不能为空
 * @param msg {const char*} 用户希望与头部信息一起输出的自定义信息, 可以为空
 */
HTTP_API void http_hdr_print(const HTTP_HDR *hh, const char *msg);

/**
 * 调试输出HTTP协议头部数据，调试类接口
 * @param fp {ACL_VSTREAM*} 某个流指针，输出结果将会定向至该数据流(可以为网络流或文件流)
 * @param hh {HTTP_HDR*} 通用HTTP头类型的数据指针，不能为空
 * @param msg {const char*} 用户希望与头部信息一起输出的自定义信息, 可以为空
*/
HTTP_API void http_hdr_fprint(ACL_VSTREAM *fp, const HTTP_HDR *hh, const char *msg);

/**
 * 调试输出HTTP协议头部数据，调试类接口
 * @param bf {ACL_VSTRING*} 输出结果将会定向至该缓冲区
 * @param hh {HTTP_HDR*} 通用HTTP头类型的数据指针，不能为空
 * @param msg {const char*} 用户希望与头部信息一起输出的自定义信息, 可以为空
*/
HTTP_API void http_hdr_sprint(ACL_VSTRING *bf, const HTTP_HDR *hh, const char *msg);

/*-------------------------------- HTTP 请求头操作函数 -----------------------*/
/* in http_hdr_req.c */

/**
 * 设置标志位，针对 HTTP 请求的 URI 中的 ? 问号被转义(即被转成 %3F)的请求是否做兼容性处理
 * @param onoff {int} 为非 0 值时表示做兼容性处理，内部缺省值为 1
 */
HTTP_API void http_uri_correct(int onoff);

/**
 * 分配一个请求的HTTP协议头对象
 * @return {HTTP_HDR_REQ*} HTTP请求头对象
 */
HTTP_API HTTP_HDR_REQ *http_hdr_req_new(void);

/**
 * 根据请求的URL，请求的方法，HTTP版本创建一个HTTP请求头对象
 * @param url {const char*} 请求的URL，必须是完整的URL，如：
 *  http://www.test.com/path/proc?name=value
 *  http://www.test.com/path/proc
 *  http://www.test.com/
 * @param method {const char*} HTTP请求方法，必须为如下之一：
 *  GET, POST, CONNECT, HEAD, 且要注意必须都为大写
 * @param version {const char *} HTTP版本，必须为如下之一：
 *  HTTP/1.0, HTTP/1.1
 * @return {HTTP_HDR_REQ*} HTTP请求头对象
 */
HTTP_API HTTP_HDR_REQ *http_hdr_req_create(const char *url,
		const char *method, const char *version);

/**
 * 克隆一个HTTP请求头对象，但不复制其中的 chat_ctx, chat_free_ctx_fn
 * 两个成员变量
 * @param hdr_req {const HTTP_HDR_REQ*} HTTP请求头对象
 * @return {HTTP_HDR_REQ*} 克隆的HTTP请求头对象
 */
HTTP_API HTTP_HDR_REQ *http_hdr_req_clone(const HTTP_HDR_REQ* hdr_req);

/**
 * 根据上次HTTP请求头内容及重定向的URL产生一个新的HTTP请求头
 * @param hh {const HTTP_HDR_REQ*} 上次的HTTP请求头对象
 * @param url {const char *} 重定向的URL，如果有 http[s]:// 前缀，则认为
 *  是完整的URL，新的 Host 字段将由该URL中提取，否则则继承源HTTP请求头中
 *  的 Host 字段
 * @return {HTTP_HDR_REQ*} 新产生的重定向的HTTP请求头
 */
HTTP_API HTTP_HDR_REQ *http_hdr_req_rewrite(const HTTP_HDR_REQ *hh, const char *url);

/**
 * 根据HTTP请求头内容及重定向的URL重新设置该HTTP请求头的信息
 * @param hh {const HTTP_HDR_REQ*} 上次的HTTP请求头对象
 * @param url {const char *} 重定向的URL，如果有 http[s]:// 前缀，则认为
 *  是完整的URL，新的 Host 字段将由该URL中提取，否则则继承源HTTP请求头中
 *  的 Host 字段
 * @return {int} 0: ok; < 0: error
 */
HTTP_API int http_hdr_req_rewrite2(HTTP_HDR_REQ *hh, const char *url);

/**
 * 释放HTTP请求头对象
 * @param hh {HTTP_HDR_REQ*} HTTP请求头对象
 */
HTTP_API void http_hdr_req_free(HTTP_HDR_REQ *hh);

/**
 * 将HTTP请求头对象的成员变量释放并重新初始化
 * @param hh {HTTP_HDR_REQ*} HTTP请求头对象
 */
HTTP_API void http_hdr_req_reset(HTTP_HDR_REQ *hh);

/**
 * 分析HTTP协议头的cookies
 * @param hh {HTTP_HDR_REQ*} HTTP请求头类型的数据指针，不能为空
 * @return {int} 0: ok;  -1: err.
 */
HTTP_API int http_hdr_req_cookies_parse(HTTP_HDR_REQ *hh);

/**
 * 分析HTTP请求首行数据(如: GET /cgi-bin/test.cgi?name=value&name2=value2 HTTP/1.0)
 * 请求的方法(GET)-->hdr_request_method
 * URL数据分析结果(name=value)-->hdr_request_table
 * HTTP协议版本号(HTTP/1.0)-->hdr_request_proto
 * URL数据中的路径部分(/cgi-bin/test.cgi)-->hdr_request_url
 * @param hh {HTTP_HDR_REQ*} HTTP请求头类型的数据指针，不能为空
 * @return {int} 0: ok;  -1: err.
 */
HTTP_API int http_hdr_req_line_parse(HTTP_HDR_REQ *hh);

/**
 * 分析HTTP请求头协议数据, 其内部会调用 http_hdr_req_line_parse, http_hdr_req_cookies_parse
 * @param hh {HTTP_HDR_REQ*} HTTP请求头类型的数据指针，不能为空
 * @return {int} 0: ok;  -1: err.
 */
HTTP_API int http_hdr_req_parse(HTTP_HDR_REQ *hh);

/**
 * 分析HTTP请求头协议数据, 其内部会调用 http_hdr_req_line_parse, http_hdr_req_cookies_parse
 * 如果 parse_params 非 0 则分析HTTP请求 url 中的参数部分; 如果 parse_cookie 非 0 则分析
 * HTTP请求中的 cookie 内容
 * @param hh {HTTP_HDR_REQ*} HTTP请求头类型的数据指针，不能为空
 * @param parse_params {int} 是否分析请求 url 中的参数部分
 * @param parse_cookie {int} 是否分析请求中的 cookie 内容
 * @return {int} 0: ok;  -1: err.
 */
HTTP_API int http_hdr_req_parse3(HTTP_HDR_REQ *hh, int parse_params, int parse_cookie);

/**
 * 从HTTP请求头中获得某个cookie值
 * @param hh {HTTP_HDR_REQ*) HTTP请求头类型的数据指针，不能为空
 * @param name {const char*} 某个cookie的变量名, 不能为空
 * @return {const char*} !NULL: 该返回值即为所要求的cookie; NULL: 出错或所要求的cookie不存在
 */
HTTP_API const char *http_hdr_req_cookie_get(HTTP_HDR_REQ *hh, const char *name);

/**
 * 从HTTP请求头中取得HTTP请求的方法, 如: POST, GET, CONNECT
 * @param hh {const HTTP_HDR_REQ*} HTTP请求头类型的数据指针，不能为空
 * @return {const char*} 返回请示方法. NULL: error; !NULL: OK.
 */
HTTP_API const char *http_hdr_req_method(const HTTP_HDR_REQ *hh);

/**
 * 从HTTP请求头中获取请求URL中某个请求字段的数据, 
 * 如取: /cgi-bin/test.cgi?n1=v1&n2=v2 中的 n2的值v2
 * @param hh {const HTTP_HDR_REQ*} HTTP请求头类型的数据指针，不能为空
 * @param name {const char*} 请求参数中的变量名
 * @return {const char*} !NULL: ok, 返回变量值的内存指针;  NULL: 出错，或请求的变量名不存在.
 */
HTTP_API const char *http_hdr_req_param(const HTTP_HDR_REQ *hh, const char *name);

/**
 * 从HTTP请求头中获取请求行中的访问路径部分, 不包含主机名但包含参数.
 * 如原请求行数据为:
 *   GET /cgi-bin/test.cgi?n1=v1&n2=v2 HTTP/1.1
 *  or
 *   GET http://www.test.com[:80]/cgi-bin/test.cgi?n1=v1&n2=v2 HTTP/1.1
 * 则分析后的结果数据为:
 *   /cgi-bin/test.cgi?n1=v1&n2=v2
 * @param hh {const HTTP_HDR_REQ*} HTTP请求头类型的数据指针，不能为空
 * @return {const char*} 请示的URL. !NULL: OK; NULL: error.
 */
HTTP_API const char *http_hdr_req_url_part(const HTTP_HDR_REQ *hh);

/**
 * 从HTTP请求头中获取请求行中的访问路径部分, 不包含主机名及参数.
 * 如原请求行数据为:
 *   GET /cgi-bin/test.cgi?n1=v1&n2=v2 HTTP/1.1
 *  or
 *   GET http://www.test.com[:80]/cgi-bin/test.cgi?n1=v1&n2=v2 HTTP/1.1
 * 则分析后的结果数据为:
 *   /cgi-bin/test.cgi
 * @param hh {const HTTP_HDR_REQ*} HTTP请求头类型的数据指针，不能为空
 * @return {const char*} 请示的URL. !NULL: OK; NULL: error.
 */
HTTP_API const char *http_hdr_req_url_path(const HTTP_HDR_REQ *hh);

/**
 * 从HTTP请求协议头中获得服务器的主机IP或域名，格式为：IP|domain[:PORT]
 * 如: 192.168.0.22:80, or www.test.com:8088
 * @param hh {const HTTP_HDR_REQ*} HTTP请求头类型的数据指针，不能为空
 * @return {const char*} 返回用户请示的主机名. !NULL: ok; NULL: error.
 */
HTTP_API const char *http_hdr_req_host(const HTTP_HDR_REQ *hh);

/**
 * 从HTTP请求头协议中获得完整的URL请求字符串
 * 如原HTTP请求头为:
 * GET /cgi-bin/test.cgi?n1=v1&n2=v2 HTTP/1.1
 * HOST: www.test.com
 * 则经该函数后则返回:
 * http://www.test.com/cgi-bin/test.cgi?n1=v1&n2=v2
 * @param hh {const HTTP_HDR_REQ*} HTTP请求头类型的数据指针，不能为空
 * @return {const char*} 请示的URL. !NULL: OK; NULL: error.
 * @example:
 *  void test(HTTP_HDR_REQ *hh)
 *  {
 *    const char *url = http_hdr_req_url(hh);
 *    printf(">>> url: %s\r\n", url ? url : "null");
 *  }
 *  注意, 因为 http_hdr_req_url 内部使用到了一个线程局部静态变量内存区, 所以
 *  不可如下使用，否则会使返回的数据发生重叠.
 *  void test(HTTP_HDR_REQ *hh1, HTTP_HDR_REQ *hh2)
 *  {
 *    const char *url1 = http_hdr_req_url(hh1);
 *    const char *url2 = http_hdr_req_url(hh2);
 *    printf(">>> url1: %s, url2: %s\n", url1, url2);
 *  }
 *  因为 url1, url2 实际上都是指向的同一内存区, 所以最终的结果将是 url1, url2
 *  内容相同. 如遇此类情形, 应该如下操作:
 *  void test(HTTP_HDR_REQ *hh1, HTTP_HDR_REQ *hh2)
 *  {
 *    const char *ptr;
 *    static char dummy[1];
 *    char *url1 = dummy, *url2 = dummy;
 *    ptr = http_hdr_req_url(hh1);
 *    if (ptr)
 *      url1 = acl_mystrdup(ptr);
 *    ptr = http_hdr_req_url(hh2);
 *    if (ptr)
 *      url2 = acl_mystrdup(ptr);
 *    printf(">>> url1: %s, url2: %s\n", url1, url2);
 *    if (url1 != dummy)
 *      acl_myfree(url1);
 *    if (url2 != dummy)
 *      acl_myfree(url2);
 *  }
 */
HTTP_API const char *http_hdr_req_url(const HTTP_HDR_REQ *hh);

/**
 * 分析HTTP请求头中的 Range 字段
 * @param hdr_req {HTTP_HDR_REQ*} 请求HTTP协议头, 不能为空
 * @param range_from {http_off_t*} 存储偏移起始位置
 * @param range_to {http_off_t*} 存储偏移结束位置
 * 注： * {range_from}, {range_to} 下标从0开始
 * 请求的 Range 格式:
 *   Range: bytes={range_from}-, bytes={range_from}-{range_to}
 */
HTTP_API int http_hdr_req_range(const HTTP_HDR_REQ *hdr_req,
	http_off_t *range_from, http_off_t *range_to);

/*---------------------------- HTTP 响应头操作函数 ---------------------------*/
/* in http_hdr_res.c */

/**
 * 分析HTTP响应头中的状态行
 *@param hh {HTTP_HDR_RES*} HTTP响应头类型的数据指针，不能为空
 *@param dbuf {const char*} 状态行数据, 如: HTTP/1.0 200 OK，不能为空
 *@return {int} 0: ok;  < 0: error，分析结果存储在 hh 结构中
 */
HTTP_API int http_hdr_res_status_parse(HTTP_HDR_RES *hh, const char *dbuf);

/**
 * 创建一个新的HTTP响应头
 * @return {HTTP_HDR_RES*}
 */
HTTP_API HTTP_HDR_RES *http_hdr_res_new(void);

/**
 * 克隆一个HTTP响应头
 * @param hdr_res {const HTTP_HDR_RES*} 源HTTP响应头
 * @return {HTTP_HDR_RES *} 新产生的HTTP响应头
 */
HTTP_API HTTP_HDR_RES *http_hdr_res_clone(const HTTP_HDR_RES *hdr_res);

/**
 * 释放一个HTTP响应头
 * @param hh {HTTP_HDR_RES*} HTTP响应头
 */
HTTP_API void http_hdr_res_free(HTTP_HDR_RES *hh);

/**
 * 向HTTP响应头重新初始化并释放其中的成员变量
 * @param hh {HTTP_HDR_RES*} HTTP响应头
 */
HTTP_API void http_hdr_res_reset(HTTP_HDR_RES *hh);

/**
 * 分析HTTP响应头里的数据，并存储分析结果
 * @param hdr_res {HTTP_HDR_RES*} HTTP响应头
 */
HTTP_API int http_hdr_res_parse(HTTP_HDR_RES *hdr_res);

/**
 * 分析HTTP响应头中的 Range 字段
 * @param hdr_res {HTTP_HDR_RES*} 响应HTTP协议头, 不能为空
 * @param range_from {http_off_t*} 存储偏移起始位置, 不能为空
 * @param range_to {http_off_t*} 存储偏移结束位置, 不能为空
 * @param total_length {http_off_t*} 整个数据文件的总长度, 可为空
 * @return {int} 返回 0 表示成功，-1 表示失败
 * 注： * {range_from}, {range_to} 下标从0开始
 * 响应的 Range 格式:
 *   Content-Range: bytes {range_from}-{range_to}/{total_length}
 */
HTTP_API int http_hdr_res_range(const HTTP_HDR_RES *hdr_res,
	http_off_t *range_from, http_off_t *range_to, http_off_t *total_length);

/* in http_rfc1123.c */

/**
 * 将时间值转换成RFC1123所要求的格式
 * @param buf {char*} 存储空间
 * @param size {size_t} buf 的空间大小
 * @param t {time_t} 时间值
 */
HTTP_API const char *http_mkrfc1123(char *buf, size_t size, time_t t);

/*----------------------- HTTP 异步读操作函数 --------------------------------*/
/* in http_chat_async.c */

/**
 * 异步获取一个HTTP REQUEST协议头，数据结果存储在hdr中, 当取得一个完整的HTTP头或
 * 出错时调用用户的注册函数 notify
 * @param hdr {HTTP_HDR_REQ*} HTTP请求头类型结构指针，不能为空
 * @param astream {ACL_ASTREAM*} 与客户端连接的数据流, 不能为空
 * @param notify {HTTP_HDR_NOTIFY} 当HTTP协议头读完或出错时调用的用户的注册函数
 * @param arg {void*} notify 调用时的一个参数
 * @param timeout {int} 接收数据过程中的读超时时间
 */
HTTP_API void http_hdr_req_get_async(HTTP_HDR_REQ *hdr, ACL_ASTREAM *astream,
		HTTP_HDR_NOTIFY notify, void *arg, int timeout);

/**
 * 异步获取一个HTTP RESPOND协议头，数据结果存储在hdr中, 当取得一个完整的HTTP头或
 * 出错时调用用户的注册函数 notify
 * @param hdr {HTTP_HDR_REQ*} HTTP响应头类型结构指针，不能为空
 * @param astream {ACL_ASTREAM*} 与服务端连接的数据流, 不能为空
 * @param notify {HTTP_HDR_NOTIFY} 当HTTP协议头读完或出错时调用的用户的注册函数
 * @param arg {void*} notify 调用时的一个参数
 * @param timeout {int} 接收数据过程中的读超时时间
 */
HTTP_API void http_hdr_res_get_async(HTTP_HDR_RES *hdr, ACL_ASTREAM *astream,
		HTTP_HDR_NOTIFY notify, void *arg, int timeout);

/**
 * 异步从客户端读取请求的BODY协议体, 在接收过程中边接收连回调用户的 notify
 * 回调函数, 如果 notify 返回小于 0 的值, 则认为出错且不再继续接收数据
 * @param request {HTTP_REQ*} HTTP请求体类型指针, 不能为空, 且 request->hdr 为空
 * @param astream {ACL_ASTREAM*} 与客户端连接的数据流, 不能为空
 * @param notify {HTTP_BODY_NOTIFY} 接收客户端数据过程中回调的用户的注册函数
 * @param arg {void*} notify 调用时的一个参数
 * @param timeout {int} 接收数据过程中的读超时时间
 */
HTTP_API void http_req_body_get_async(HTTP_REQ *request, ACL_ASTREAM *astream,
		 HTTP_BODY_NOTIFY notify, void *arg, int timeout);
/*
 * 异步从服务器端读取响应数据的BODY协议体, 在接收过程中连接收连回调用户的
 * notify 回调函数, 如果 notify 返回小于 0 的值, 则认为出错且不再继续接收数据
 * @param respond {HTTP_RES*} HTTP响应体类型指针, 不能为空,且 respond->hdr 不为空
 * @param astream {ACL_ASTREAM*} 与服务端连接的数据流, 不能为空
 * @param notify {HTTP_BODY_NOTIFY} 接收服务端数据过程中回调的用户的注册函数
 * @param arg {void*} notify 调用时的一个参数
 * @param timeout {int} 接收数据过程中的读超时时间
 */
HTTP_API void http_res_body_get_async(HTTP_RES *respond, ACL_ASTREAM *astream,
		HTTP_BODY_NOTIFY notify, void *arg, int timeout);

/*----------------------- HTTP 同步读操作函数 --------------------------------*/
/* in http_chat_sync.c */

/**
* 同步获取一个HTTP REQUEST协议头，数据结果存储在hdr中, 当取得一个完整的HTTP头或
* 出错时调用用户的注册函数 notify
* @param hdr {HTTP_HDR_REQ*} HTTP请求头类型结构指针，不能为空
* @param stream {ACL_VSTREAM*} 与客户端连接的数据流, 不能为空
* @param timeout {int} 接收数据过程中的读超时时间
* @return {int} 0: 成功; < 0: 失败
*/
HTTP_API int http_hdr_req_get_sync(HTTP_HDR_REQ *hdr,
		 ACL_VSTREAM *stream, int timeout);

/**
 * 同步获取一个HTTP RESPOND协议头，数据结果存储在hdr中, 当取得一个完整的HTTP头或
 * 出错时调用用户的注册函数 notify
 * @param hdr {HTTP_HDR_REQ*} HTTP响应头类型结构指针，不能为空
 * @param stream {ACL_VSTREAM*} 与服务端连接的数据流, 不能为空
 * @param timeout {int} 接收数据过程中的读超时时间
 * @return {int} 0: 成功; < 0: 失败
 */
HTTP_API int http_hdr_res_get_sync(HTTP_HDR_RES *hdr,
		ACL_VSTREAM *stream, int timeout);

/**
 * 同步从客户端读取请求的BODY协议体
 * @param request {HTTP_REQ*} HTTP请求体类型指针, 不能为空, 且 request->hdr 为空
 * @param stream {ACL_VSTREAM*} 与客户端连接的数据流, 不能为空
 * @param buf {void *} 存储结果的内容空间
 * @param size {int} buf 的空间大小
 * @return ret {http_off_t} 本次读到的HTTP请求体的内容
 *             0: 表示读完了HTTP数据体内容，但并不代表数据流已经关闭;
 *             < 0: 表示读出错，流关闭或出错;
 *             > 0: 表示未读完，目前读到ret 个字节的数据
 */
HTTP_API http_off_t http_req_body_get_sync(HTTP_REQ *request, ACL_VSTREAM *stream,
		void *buf, int size);
#define http_req_body_get_sync2	http_req_body_get_sync

/**
 * 同步从服务端读取响应的BODY协议体
 * @param respond {HTTP_RES*} HTTP响应体类型指针, 不能为空, 且 respond->hdr 为空
 * @param stream {ACL_VSTREAM*} 与客户端连接的数据流, 不能为空
 * @param buf {void *} 存储结果的内容空间
 * @param size {int} buf 的空间大小
 * @return ret {http_off_t} 本次读到的HTTP响应体的内容
 *             0: 表示读完了HTTP数据体内容，但并不代表数据流已经关闭;
 *             < 0: 表示读出错，流关闭或出错;
 *             > 0: 表示未读完，目前读到ret 个字节的数据
 */
HTTP_API http_off_t http_res_body_get_sync(HTTP_RES *respond, ACL_VSTREAM *stream,
		void *buf, int size);
#define http_res_body_get_sync2	http_res_body_get_sync

/**
 * 设置请求协议的控制标志位
 * @param request {HTTP_REQ*} HTTP请求体类型指针, 不能为空, 且 request->hdr 为空
 * @param name {int} 第一个标志位，当最后一个标志位为 HTTP_CHAT_SYNC_CTL_END 时
 *  表示结束
 */
HTTP_API void http_chat_sync_reqctl(HTTP_REQ *request, int name, ...);

/**
 * 设置响应协议的控制标志位
 * @param respond {HTTP_RES*} HTTP响应体类型指针, 不能为空, 且 respond->hdr 为空
 * @param name {int} 第一个标志位，当最后一个标志位为 HTTP_CHAT_SYNC_CTL_END 时
 *  表示结束
 */
HTTP_API void http_chat_sync_resctl(HTTP_RES *respond, int name, ...);
#define	HTTP_CHAT_SYNC_CTL_END      0  /**< 结束标志位 */
#define	HTTP_CHAT_CTL_BUFF_ONOFF    1  /**< 是否打开数据接收时的预缓冲策略 */

/*------------------------ HTTP 请求体构造及释放函数  ------------------------*/
/* in http_req.c */

/**
 * 根据HTTP请求头分配一个请求体对象
 * @param hdr_req {HTTP_HDR_REQ*} 请求头对象
 * @return {HTTP_REQ*} 请求体对象
 */
HTTP_API HTTP_REQ *http_req_new(HTTP_HDR_REQ *hdr_req);

/**
 * 释放请求体对象
 * @param request {HTTP_REQ*} 请求体对象
 */
HTTP_API void http_req_free(HTTP_REQ *request);

/*------------------------ HTTP 响应体构造及释放函数  ------------------------*/
/* in http_res.c */

/**
* 根据HTTP响应头分配一个响应体对象
* @param hdr_res {HTTP_HDR_RES*} 响应头对象
* @return {HTTP_RES*} 响应体对象
*/
HTTP_API HTTP_RES *http_res_new(HTTP_HDR_RES *hdr_res);

/**
 * 释放响应体对象
 * @param respond {HTTP_RES*} 响应体对象
 */
HTTP_API void http_res_free(HTTP_RES *respond);

/*------------------------------ HTTP 头构造函数 -----------------------------*/
/* in http_hdr_build.c */

/**
 * 向通用HTTP头中添加数据
 * @param hdr {HTTP_HDR*} 通用HTTP头对象
 * @param name {const char*} 变量名，如 Accept-Encoding: deflate, gzip 中的 Accept-Encoding
 * @param value {const char*} 变量值，如 Accept-Encoding: deflate, gzip 中的 deflate, gzip
 */
HTTP_API void http_hdr_put_str(HTTP_HDR *hdr, const char *name, const char *value);

/**
 * 向通用HTTP头中添加数据
 * @param hdr {HTTP_HDR*} 通用HTTP头对象
 * @param name {const char*} 变量名，如 Content-Length: 1024 中的 Conteng-Length
 * @param value {const int} 变量值，如 Content-Length: 1024 中的 1024
 */
HTTP_API void http_hdr_put_int(HTTP_HDR *hdr, const char *name, int value);

/**
 * 向通用HTTP头中添加数据
 * @param hdr {HTTP_HDR*} 通用HTTP头对象
 * @param name {const char*} 变量名，如 Accept-Encoding: deflate, gzip 中的 Accept-Encoding
 * @param fmt {const char*} 变参格式字符串
 */
# if defined(WIN32) || defined(WIN64)
HTTP_API void http_hdr_put_fmt(HTTP_HDR *hdr, const char *name, const char *fmt, ...);
#else
HTTP_API void __attribute__((format(printf,3,4)))
	http_hdr_put_fmt(HTTP_HDR *hdr, const char *name, const char *fmt, ...);
#endif

/**
 * 向通用HTTP头中添加时间数据
 * @param hdr {HTTP_HDR*} 通用HTTP头对象
 * @param name {const char*} 变量名
 * @param t {time_t} 时间值
 */
HTTP_API void http_hdr_put_time(HTTP_HDR *hdr, const char *name, time_t t);

/**
 * 根据HTTP请求头的字段来设置是否与服务端保持长连接, 结果存储于HTTP响应头中
 * @param req {const HTTP_HDR_REQ*} HTTP请求头
 * @param res {HTTP_HDR_RES*} HTTP响应头，存储分析结果
 */
HTTP_API int http_hdr_set_keepalive(const HTTP_HDR_REQ *req, HTTP_HDR_RES *res);

/**
 * 用返回状态(1xx, 2xx, 3xx, 4xx, 5xx) 初始化一个HTTP响应头
 * @param hdr_res {HTTP_HDR_RES*} HTTP响应头，存储分析结果
 * @param status {int} 状态号，nxx(1xx, 2xx, 3xx, 4xx, 5xx)
 */
HTTP_API void http_hdr_res_init(HTTP_HDR_RES *hdr_res, int status);

/**
 * 用返回状态(nxx)生成一个HTTP响应头
 * @param status {int} 状态号，nxx(1xx, 2xx, 3xx, 4xx, 5xx)
 * @return {HTTP_HDR_RES*} 生成的HTTP响应头
 */
HTTP_API HTTP_HDR_RES *http_hdr_res_static(int status);

/**
* 用返回状态(nxx)生成一个HTTP响应头
* @param status {int} 状态号，nxx(4xx, 5xx)
* @return {HTTP_HDR_RES*} 生成的HTTP响应头
*/
HTTP_API HTTP_HDR_RES *http_hdr_res_error(int status);

/**
 * 根据HTTP通用头生成头的完整内容于BUF中
 * @param hdr {const HTTP_HDR*} 通用HTTP头
 * @param strbuf {ACL_VSTRING*} 存储结果的缓冲区
 */
HTTP_API void http_hdr_build(const HTTP_HDR *hdr, ACL_VSTRING *strbuf);

/**
 * 根据HTTP请求头生成请求头内容于BUF中
 * @param hdr_req {const HTTP_HDR_REQ*} HTTP请求头
 * @param strbuf {ACL_VSTRING*} 存储结果的缓冲区
 */
HTTP_API void http_hdr_build_request(const HTTP_HDR_REQ *hdr_req, ACL_VSTRING *strbuf);

/*----------------------------- HTTP 响应状态信息函数 ------------------------*/
/* in http_status.c */

/**
 * 根据HTTP响应号(nxx)返回该值所代表的字符串
 * @param status {int} 状态号，nxx(1xx, 2xx, 3xx, 4xx, 5xx)
 * @return {const char*} 响应号所对应的字符串表示
 */
HTTP_API const char *http_status_line(int status);

/*---------------------------- HTTP HTML 模板操作函数 ------------------------*/
/* in http_tmpl.c */

/**
 * 装载HTTP响应代码的HTML模板
 * @param tmpl_path {const char*} HTML模板文件所在的路径
 */
HTTP_API void http_tmpl_load(const char *tmpl_path);

/**
 * 读取对应HTTP响应状态码的模板信息
 * @param status {int} HTTP 状态响应码
 * @return {const ACL_VSTRING*} 对应HTTP响应状态码的模板信息
 */
HTTP_API const ACL_VSTRING *http_tmpl_get(int status);

/**
 * 读取对应HTTP响应状态码的标题提示信息
 * @param status {int} HTTP 状态响应码
 * @return {const char*} 对应HTTP响应状态码的标题提示信息
 */
HTTP_API const char *http_tmpl_title(int status);

/**
 * 获得相应HTTP响应状态码的模板提示信息的长度大小
 * @param status {int} HTTP 状态响应码
 * @return {int} 模板提示信息的长度大小
 */
HTTP_API int http_tmpl_size(int status);

/*---------------------------- HTTP HTML 模板初始化函数 ----------------------*/
/* in http_init.c */

/**
 * 初始化HTTP应用协议
 * @param tmpl_path {const char*} 模板信息文件的存放路径
 */
HTTP_API void http_init(const char *tmpl_path);

/**
 * 是否自动缓冲被释放的 HTTP 头对象，从而使其内存可以重复使用, 该函数在程序初始化
 * 时只能被调用一次
 * @param max {int} 当该值 > 0 时便自动启用 HTTP 头对象缓冲功能
 */
HTTP_API void http_hdr_cache(int max);

/**
 * 设置在进行 HTTP 协议体数据传输时的缓冲区大小
 * @param size {http_off_t} 缓冲区大小
 */
HTTP_API void http_buf_size_set(http_off_t size);

/**
 * 获得进行 HTTP 协议体数据传输时的缓冲区大小
 * @return {http_off_t} 缓冲区大小
 */
HTTP_API http_off_t http_buf_size_get(void);

#ifdef	__cplusplus
}
#endif

#endif
