#ifndef	__LIB_GID_INCLUDE_H__
#define	__LIB_GID_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

/* 通讯协议方式 */
#define	GID_PROTO_CMDLINE	0	/* 命令行方式 */
#define	GID_PROTO_JSON		1	/* http 方式，数据格式为 json 格式 */
#define	GID_PROTO_XML		2	/* http 方式，数据格式为 xml 格式 */

/* 操作命令 */
#define	GID_CMD_NEXT	"new_gid"	/* 获取下一个唯一 gid */

/* 出错码 */

/* 客户端相关的错误码 */
#define	GID_OK			200	/* 正常 */
#define	GID_ERR_INIT		400	/* 库未初始化，应用应在程序初始化时调用初始化函数 gid_client_init */
#define	GID_ERR_CONN		401	/* 连接服务器失败 */
#define	GID_ERR_IO		402	/* 与服务器通信失败 */
#define	GID_ERR_PROTO		403	/* 协议格式错误 */
#define	GID_ERR_SERVER		404	/* 服务器内部出错 */

/* 服务端相关的错误码 */
#define GID_ERR_SID		500	/* 会话 ID 号不对 */
#define GID_ERR_OVERRIDE	501	/* 达到最大分配值 */
#define GID_ERR_SAVE		502	/* 存储至磁盘时出错 */

/**
 * 库初始化函数，使用者在程序启动后应该调用该函数初始化库
 * @param proto {int} 通信协议格式，参见上面：GID_PROTO_XXX
 * @param server_addr {const char*} gid 服务器地址，格式：ip:port 或
 *  domain:port 或 unix 域 /xxx/xxx/xxx
 */
void gid_client_init(int proto, const char *server_addr);

/**
 * 根据错误号获得错误描述
 * @param errnum {int} 错误号，参见上面：GID_ERR_XXX
 * @return {const char*} 错误描述信息
 */
const char *gid_client_serror(int errnum);

/**
 * 设置 HTTP 请求中的 URL，内部有缺省值，可不直接调用此函数
 * @param url {const char*} URL 字符串
 */
void gid_client_set_url(const char *url);

/**
 * 设置 HTTP 请求是否保持长连接，缺省情况下保持长连接
 * @param keepalive {int} 是否保持长连接
 */
void gid_client_set_keepalive(int keepalive);

/**
 * 在长连接情况下，如果连接中间意外中断，设置重试次数，缺省值为 1
 * @param nretry {int} 最大重试次数
 */
void gid_client_set_retry_limit(int nretry);

/**
 * 设置连接 gid 服务器的连接超时时间（秒），缺省值为 20 秒
 * @param timeout {int} 超时时间（秒）
 */
void gid_client_set_conn_timeout(int timeout);

/**
 * 设置网络通信的读写超时时间（秒），缺省值为 20 秒
 * @param timeout {int} 超时时间（秒）
 */
void gid_client_set_rw_timeout(int timeout);

/**
 * 获得下一个 gid 号
 * @param tag {const char*} 标识名称，如果为空，则内部缺省使用 default 标签，
 *  该值的格式为：tag_name[:sid]，其中的 tag_name 为真正的标识名，sid 为访问
 *  该标识对象的授权ID号，如果该值与服务端的 sid 不匹配，则禁止访问并返回错误，
 *  当访问一个新的标识对象并产生第一个 gid 值时，其中如果设置了 sid 则该 sid
 *  自动做为该标识对象的授权ID号，其它程序若想访问该标识对象的 gid 则必须提供
 *  该授权 ID 号
 * @param errnum {int*} 该指针非空时用来记录出错时的错误号
 * @return {long long int} 获得的下一个唯一 gid 号，如果该值 < 0 则表示出错
 */
long long int gid_next(const char *tag, int *errnum);

/* 如果获取 gid 的函数使用用户提供的连接描述符 */

/**
 * 采用命令行方式从服务端获取 gid 号
 * @param fd {int} 与服务器连接的套接字
 * @param tag {const char*} 标识名称，若为空则内部缺省使用default
 * @param errnum {int*} 该指针非空时用来记录出错时的错误号
 * @return {long long int} 获得的下一个唯一 gid 号， *  如果该值为 < 0 则表示出错
 */
long long int gid_cmdline_get(int fd, const char *tag, int *errnum);

/**
 * 采用 http 协议且数据格式为 json 格式，从服务端获取 gid 号
 * @param fd {int} 与服务器连接的套接字
 * @param tag {const char*} 标识名称，若为空则内部缺省使用default
 * @param errnum {int*} 该指针非空时用来记录出错时的错误号
 * @return {long long int} 获得的下一个唯一 gid 号，如果该值 < 0 则表示出错
 */
long long int gid_json_get(int fd, const char *tag, int *errnum);

/**
 * 采用 http 协议且数据格式为 xml 格式，从服务端获取 gid 号
 * @param fd {int} 与服务器连接的套接字
 * @param tag {const char*} 标识名称，若为空则内部缺省使用default
 * @param errnum {int*} 该指针非空时用来记录出错时的错误号
 * @return {long long int} 获得的下一个唯一 gid 号，如果该值 < 0 则表示出错
 */
long long int gid_xml_get(int fd, const char *tag, int *errnum);

#ifdef	__cplusplus
}
#endif

#endif
