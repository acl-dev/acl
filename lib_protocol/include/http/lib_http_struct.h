
#ifndef	__LIB_HTTP_STRUCT_INCLUDE_H__
#define	__LIB_HTTP_STRUCT_INCLUDE_H__

/* #include "lib_acl.h" */

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef HTTP_LIB
# ifndef HTTP_API
#  define HTTP_API
# endif
#elif defined(HTTP_DLL) // || defined(_WINDLL)
# if defined(HTTP_EXPORTS) || defined(protocol_EXPORTS)
#  ifndef HTTP_API
#   define HTTP_API __declspec(dllexport)
#  endif
# elif !defined(HTTP_API)
#  define HTTP_API __declspec(dllimport)
# endif
#elif !defined(HTTP_API)
# define HTTP_API
#endif

typedef	acl_int64	http_off_t;

/* 结构类型定义 */
typedef struct HTTP_HDR HTTP_HDR;
typedef struct HTTP_HDR_REQ HTTP_HDR_REQ;
typedef struct HTTP_HDR_RES HTTP_HDR_RES;
typedef struct HTTP_REQ HTTP_REQ;
typedef struct HTTP_RES HTTP_RES;
typedef struct HTTP_HDR_ENTRY HTTP_HDR_ENTRY;

/* 函数类型定义 */

/**
 * 数据头过程中的回回调函数类型定义
 * @param status {int} HTTP_CHAT_XXX
 *    status:
 *      HTTP_CHAT_OK: 读到完整的数据头
 *      HTTP_CHAT_ERR_TOO_MANY_LINES: 数据头中的行数太多
 * @param arg {void*} 回调函数的参数
 * @return {int} 该回调函数如果返回值为 -1 则上级调用者便结束; 若返回 0 
 *    上级调用者继续
 */
typedef int  (*HTTP_HDR_NOTIFY)(int status, void *arg);

/**
 * 数据体请求过程中的回调函数类型定义
 * @param status {int} HTTP_CHAT_XXX
 *  status:
 *    HTTP_CHAT_OK: 已经读完整个数据体，且 data 代表最后一部分数据, dlen 表示
 *      data 的数据长度
 *    HTTP_CHAT_DATA: 当为块传输方式时，表示每个数据块中的数据体中的部分数据；
 *      当非块传输方式时，表示整个数据体的一部分数据
 *    HTTP_CHAT_CHUNK_HDR: 表示块传输方式中的某个数据块的头数据
 *    HTTP_CHAT_CHUNK_TRAILER: 表示块传输方式中的最后一个数据块的头数据
 *    HTTP_CHAT_CHUNK_DATA_ENDL: 表示块传输方式中每块数据中最后的分隔行数据
 *    HTTP_CHAT_ERR_PROTO: 表示协议出错
 * @param data {char *} 所读到的数据开始地址，永远不为空
 * @param dlen {int} 表示当前 data 数据长度
 * @return {int} 该回调函数如果返回值为 -1 则上级调用者便结束; 若返回 0 
 *    上级调用者继续
 */
typedef int  (*HTTP_BODY_NOTIFY)(int status, char *data, int dlen, void *arg);

/* 通信过程状态字定义 */
#define	HTTP_CHAT_OK                    0       /**< 读完了整个数据 */
#define	HTTP_CHAT_CONTINUE              1       /**< 内部用 */
#define	HTTP_CHAT_DATA                  2       /**< 数据体中的部分数据 */
#define	HTTP_CHAT_CHUNK_HDR             3       /**< 块数据头中的数据 */
#define HTTP_CHAT_CHUNK_DATA_ENDL       4       /**< 块数据体中的分隔行数据 */
#define	HTTP_CHAT_CHUNK_TRAILER         5       /**< 最后一个数据块的头部分数据 */
#define HTTP_CHAT_ERR_MIN               100     /**< 做为错误值的最小值 */
#define	HTTP_CHAT_ERR_IO                101     /**< IO出错 */
#define	HTTP_CHAT_ERR_PROTO             102     /**< 请求数据或响应数据的协议出错 */
#define	HTTP_CHAT_ERR_TOO_MANY_LINES    103     /**< 数据头太多行 */
#define HTTP_CHAT_ERR_MAX               1000    /**< 最大错误范围 */

/* 设置的标志位 */
#define	HTTP_CHAT_FLAG_BUFFED           0x0001

/* HTTP 协议头部字段的定义 */
#define	HTTP_HDR_ENTRY_VIA              "via"   /**< HTTP 头添加字段，防止递归请求 */
#define	HTTP_HDR_ENTRY_FORWARD_FOR      "X-Forwarded-For"  /**< HTTP 请求头添加字段 */

/* HTTP 协议请求结构 */
struct HTTP_REQ {
	HTTP_HDR_REQ *hdr_req;  /**< 与 client 相关 */
	int  status;            /**< 是否出错, defined above: HTTP_STATUS_ */
	unsigned int flag;      /**< defined as: HTTP_CHAT_FLAG_XXX */
	void *ctx;
	void (*free_ctx)(void*);
};

struct HTTP_RES {
	HTTP_HDR_RES *hdr_res;  /**< 与 client 相关 */
	int   read_cnt;
	int   status;           /**< 是否出错, defined above: HTTP_STATUS_ */
	unsigned int flag;      /**< defined as: HTTP_CHAT_FLAG_XXX */
	void *ctx;
	void (*free_ctx)(void*);
};

/* name-value 格式的条目 */
struct HTTP_HDR_ENTRY {
	char *name;
	char *value;
	int   off;
};

/* HTTP 协议头 */

struct HTTP_HDR {
	/* 通用实体 */
	char  proto[32];        /**< 支持的协议: HTTP */
	struct {
		unsigned int major; /**< 主版本号 */
		unsigned int minor; /**< 次版本号 */
	} version;

	/**< 是否保持长连接: 0 -> 不保持，> 0 -> 保持，< 0 -> 没有该字段 */
	int   keep_alive;
	http_off_t   content_length; /**< HTTP协议体数据长度 */
	int   chunked;          /**
                                 * 该字段本来对HTTP协议响应有意义,
                                 * 为了将来的扩展, 故定义于此
                                 */

	/* 内部变量 */
	int   cur_lines;
	int   max_lines;
	int   valid_lines;
	int   status;
	int   keep_alive_count; /**< 处理次数 */

	ACL_ARRAY  *entry_lnk;  /**< 存储着 HTTP_HDR_ENTRY 类型的元素 */
	void *chat_ctx;
	void (*chat_free_ctx_fn)(void*);

	int   debug;            /**< 调试信息头的标志位 */
};

#define HDR_RESTORE(hdr_ptr, hdr_type, hdr_member) \
	((hdr_type *) (((char *) (hdr_ptr)) - offsetof(hdr_type, hdr_member)))

/* HTTP 请求头 */
struct HTTP_HDR_REQ {
	HTTP_HDR hdr;       /**< 包裹了通用的HDR头, 便于通用分析 */

	int   port;         /**< 所请求的服务端的服务端口号 */
	/* 请求实体 */
	char  method[32];   /**< 请求方法: POST, GET, CONNECT */
	char  host[512];    /**< 所请示的主机的域名或IP地址 */
	ACL_VSTRING *url_part; /**
                                * 存储着请求行 URL 中的后半部分,
                                * 如:
                                * 1) http://test.com.cn/cgi-bin/test?name=value
                                *    => /cgi-bin/test?name=value
                                */
	ACL_VSTRING *url_path;  /**
                                 * 存储着请求行 URL 中的相对路径发(不包含主机部分),
                                 * 如对于 /path/test.cgi?name=value,
                                 * 仅存储 /path/test.cgi, 剩余的
                                 * 参数部分则由 url_params 存储.
                                 */
	ACL_VSTRING *url_params; /**< 存储着 URL 中的参数部分 */
	ACL_VSTRING *file_path;

	ACL_HTABLE *params_table; /**< 存储着 URL 请求行的各个字段的数据 */
	ACL_HTABLE *cookies_table; /**< 存储着的 cookie 项 */
	unsigned int flag;        /**< 标志位 */
#define	HTTP_HDR_REQ_FLAG_PARSE_PARAMS	(1 << 0)
#define	HTTP_HDR_REQ_FLAG_PARSE_COOKIE	(1 << 1)
};

/* HTTP 响应头 */

struct HTTP_HDR_RES {
	HTTP_HDR hdr;           /**< 包裹了通用的HDR头, 便于通用分析 */

	/* 响应实体 */
	int   reply_status;     /**< 服务器的响应代码，如: 100, 200, 404, 304, 500 */
};

#ifdef	__cplusplus
}
#endif

#endif

