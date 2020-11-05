#ifndef	__SMTP_CLIENT_INCLUDE_H__
#define	__SMTP_CLIENT_INCLUDE_H__

/* #include "lib_acl.h" */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SMTP_LIB
# ifndef SMTP_API
#  define SMTP_API
# endif
#elif defined(SMTP_DLL) // || defined(_WINDLL)
# if defined(SMTP_EXPORTS) || defined(protocol_EXPORTS)
#  ifndef SMTP_API
#   define SMTP_API __declspec(dllexport)
#  endif
# elif !defined(SMTP_API)
#  define SMTP_API __declspec(dllimport)
# endif
#elif !defined(SMTP_API)
# define SMTP_API
#endif

typedef struct SMTP_CLIENT {
	ACL_VSTREAM *conn;
	int   smtp_code;
	char* buf;
	int   size;
	unsigned int flag;
#define SMTP_FLAG_PIPELINING	(1 << 0)
#define SMTP_FLAG_AUTH          (1 << 1)
#define SMTP_FLAG_8BITMIME      (1 << 2)
#define SMTP_FLAG_DSN           (1 << 3)
#define SMTP_FLAG_VRFY          (1 << 4)
#define SMTP_FLAG_ETRN          (1 << 5)
#define SMTP_FLAG_SIZE          (1 << 6)
	int   message_size_limit;
} SMTP_CLIENT;

/**
 * 远程连接 SMTP 服务器
 * @param addr {const char*} SMTP 服务器地址，格式：domain:port
 * @param conn_timeout {int} 连接超时时间
 * @param rw_timeout {int} IO读写超时时间
 * @param line_limit {int} SMTP 会话过程中每行的最大长度限制
 * @return {SMTP_CLIENT*} 连接成功返回非空值，否则返回 NULL
 */
SMTP_API SMTP_CLIENT *smtp_open(const char *addr, int conn_timeout,
	int rw_timeout, int line_limit);

/**
 * 关闭由 smtp_open 打开的 SMTP 连接并释放对象
 * @param client {SMTP_CLIENT*} SMTP 连接对象
 */
SMTP_API void smtp_close(SMTP_CLIENT *client);

/**
 * 获得 SMTP 服务器的欢迎信息
 * @param client {SMTP_CLIENT*} SMTP 连接对象
 * @return {int} 0 表示成功(SMTP_CLIENT::smtp_code 表示返回码，
 *  SMTP_CLIENT::buf 存储响应内容)，否则表示出错，应该关闭连接对象
 */
SMTP_API int smtp_get_banner(SMTP_CLIENT *client);

/**
 * 向 SMTP 服务器发送 HELO/EHLO 命令
 * @param client {SMTP_CLIENT*} SMTP 连接对象
 * @param name {const char*} 握手信息，一般用域名
 * @param ehlo {int} 非 0 时使用 EHLO，否则使用 HELO
 * @return {int} 0 表示成功(SMTP_CLIENT::smtp_code 表示返回码，
 *  SMTP_CLIENT::buf 存储响应内容)，否则表示出错，应该关闭连接对象
 */

SMTP_API int smtp_greet(SMTP_CLIENT *client, const char* name, int ehlo);

/**
 * 向 SMTP 服务器发送 HELO 命令
 * @param client {SMTP_CLIENT*} SMTP 连接对象
 * @param helo {const char*} 握手信息，一般用域名
 * @return {int} 0 表示成功(SMTP_CLIENT::smtp_code 表示返回码，
 *  SMTP_CLIENT::buf 存储响应内容)，否则表示出错，应该关闭连接对象
 */
SMTP_API int smtp_helo(SMTP_CLIENT *client, const char *helo);

/**
 * 向 SMTP 服务器发送 EHLO 命令
 * @param client {SMTP_CLIENT*} SMTP 连接对象
 * @param ehlo {const char*} 握手信息，一般用域名
 * @return {int} 0 表示成功(SMTP_CLIENT::smtp_code 表示返回码，
 *  SMTP_CLIENT::buf 存储响应内容)，否则表示出错，应该关闭连接对象
 */
SMTP_API int smtp_ehlo(SMTP_CLIENT *client, const char *ehlo);

/**
 * 向 SMTP 服务器发送验证信息
 * @param client {SMTP_CLIENT*} SMTP 连接对象
 * @param user {const char*} SMTP 邮件账号
 * @param pass {const char*} SMTP 邮件账号密码
 * @return {int} 0 表示成功(SMTP_CLIENT::smtp_code 表示返回码，
 *  SMTP_CLIENT::buf 存储响应内容)，否则表示出错，应该关闭连接对象
 */
SMTP_API int smtp_auth(SMTP_CLIENT *client, const char *user, const char *pass);

/**
 * 向 SMTP 服务器发送 MAIL FROM 命令
 * @param client {SMTP_CLIENT*} SMTP 连接对象
 * @param from {const char*} 发送者邮箱
 * @return {int} 0 表示成功(SMTP_CLIENT::smtp_code 表示返回码，
 *  SMTP_CLIENT::buf 存储响应内容)，否则表示出错，应该关闭连接对象
 */
SMTP_API int smtp_mail(SMTP_CLIENT *client, const char *from);

/**
 * 向 SMTP 服务器发送 RCPT TO 命令
 * @param client {SMTP_CLIENT*} SMTP 连接对象
 * @param to {const char*} 接收者邮箱
 * @return {int} 0 表示成功(SMTP_CLIENT::smtp_code 表示返回码，
 *  SMTP_CLIENT::buf 存储响应内容)，否则表示出错，应该关闭连接对象
 */
SMTP_API int smtp_rcpt(SMTP_CLIENT *client, const char *to);

/**
 * 向 SMTP 服务器发送 DATA 命令
 * @param client {SMTP_CLIENT*} SMTP 连接对象
 * @return {int} 0 表示成功(SMTP_CLIENT::smtp_code 表示返回码，
 *  SMTP_CLIENT::buf 存储响应内容)，否则表示出错，应该关闭连接对象
 */
SMTP_API int smtp_data(SMTP_CLIENT *client);

/**
 * 向 SMTP 服务器发送邮件体内容，可以循环调用本函数直至数据发送完毕
 * @param client {SMTP_CLIENT*} SMTP 连接对象
 * @param src {const char*} 遵守邮件 MIME 编码格式的邮件体内容
 * @param len {size_t} src 数据长度
 * @return {int} 0 表示成功(SMTP_CLIENT::smtp_code 表示返回码，
 *  SMTP_CLIENT::buf 存储响应内容)，否则表示出错，应该关闭连接对象
 */
SMTP_API int smtp_send(SMTP_CLIENT *client, const char* src, size_t len);

/**
 * 向 SMTP 服务器发送邮件体内容，可以循环调用本函数直至数据发送完毕
 * @param client {SMTP_CLIENT*} SMTP 连接对象
 * @param fmt {const char*} 格式字符串
 * @param ... 变参
 * @return {int} 0 表示成功(SMTP_CLIENT::smtp_code 表示返回码，
 *  SMTP_CLIENT::buf 存储响应内容)，否则表示出错，应该关闭连接对象
 */
SMTP_API int smtp_printf(SMTP_CLIENT *client, const char* fmt, ...);

/**
 * 发送完邮件内容后调用本函数告诉 SMTP 服务器邮件数据完毕
 * @param client {SMTP_CLIENT*} SMTP 连接对象
 * @return {int} 0 表示成功(SMTP_CLIENT::smtp_code 表示返回码，
 *  SMTP_CLIENT::buf 存储响应内容)，否则表示出错，应该关闭连接对象
 */
SMTP_API int smtp_data_end(SMTP_CLIENT *client);

/**
 * 向 SMTP 服务器发送指定件路径的邮件文件
 * @param client {SMTP_CLIENT*} SMTP 连接对象
 * @param filepath {const char*} 邮件文件路径
 * @return {int} 0 表示成功(SMTP_CLIENT::smtp_code 表示返回码，
 *  SMTP_CLIENT::buf 存储响应内容)，否则表示出错，应该关闭连接对象
 */
SMTP_API int smtp_send_file(SMTP_CLIENT *client, const char *filepath);

/**
 * 向 SMTP 服务器发送给定文件流的邮件内容
 * @param client {SMTP_CLIENT*} SMTP 连接对象
 * @param in {ACL_VSTREAM*} 邮件文件输入流
 * @return {int} 0 表示成功(SMTP_CLIENT::smtp_code 表示返回码，
 *  SMTP_CLIENT::buf 存储响应内容)，否则表示出错，应该关闭连接对象
 */
SMTP_API int smtp_send_stream(SMTP_CLIENT *client, ACL_VSTREAM *in);

/**
 * 向 SMTP 服务器发送退出(QUIT)命令
 * @param client {SMTP_CLIENT*} SMTP 连接对象
 * @return {int} 0 表示成功(SMTP_CLIENT::smtp_code 表示返回码，
 *  SMTP_CLIENT::buf 存储响应内容)，否则表示出错，应该关闭连接对象
 */
SMTP_API int smtp_quit(SMTP_CLIENT *client);

/**
 * 向 SMTP 服务器发送 NOOP 命令
 * @param client {SMTP_CLIENT*} SMTP 连接对象
 * @return {int} 0 表示成功(SMTP_CLIENT::smtp_code 表示返回码，
 *  SMTP_CLIENT::buf 存储响应内容)，否则表示出错，应该关闭连接对象
 */
SMTP_API int smtp_noop(SMTP_CLIENT *client);

/**
 * 向 SMTP 服务器发送 RSET 命令
 * @param client {SMTP_CLIENT*} SMTP 连接对象
 * @return {int} 0 表示成功(SMTP_CLIENT::smtp_code 表示返回码，
 *  SMTP_CLIENT::buf 存储响应内容)，否则表示出错，应该关闭连接对象
 */
SMTP_API int smtp_rset(SMTP_CLIENT *client);

#ifdef __cplusplus
}
#endif

#endif
