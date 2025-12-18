
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
#elif defined(HTTP_DLL) /* || defined(_WINDLL) */
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

/* Structure type definitions */
typedef struct HTTP_HDR HTTP_HDR;
typedef struct HTTP_HDR_REQ HTTP_HDR_REQ;
typedef struct HTTP_HDR_RES HTTP_HDR_RES;
typedef struct HTTP_REQ HTTP_REQ;
typedef struct HTTP_RES HTTP_RES;
typedef struct HTTP_HDR_ENTRY HTTP_HDR_ENTRY;

/* Callback type definitions */

/**
 * Callback function type for header reading process
 * @param status {int} HTTP_CHAT_XXX
 *    status:
 *      HTTP_CHAT_OK: Successfully read complete header
 *      HTTP_CHAT_ERR_TOO_MANY_LINES: Too many lines in header
 * @param arg {void*} Parameter for callback function
 * @return {int} If callback function returns -1, upper layer stops or
 *    aborts; otherwise upper layer continues
 */
typedef int  (*HTTP_HDR_NOTIFY)(int status, void *arg);

/**
 * Callback function type during request/response body reading
 * @param status {int} HTTP_CHAT_XXX
 *  status:
 *    HTTP_CHAT_OK: Received complete body, data stores last chunk,
 *      dlen indicates data length
 *    HTTP_CHAT_DATA: In chunked transfer mode, indicates partial data
 *      in each chunk; in non-chunked mode, indicates a portion of body
 *    HTTP_CHAT_CHUNK_HDR: Indicates a chunk header in chunked mode
 *    HTTP_CHAT_CHUNK_TRAILER: Last chunk header in chunked mode
 *    HTTP_CHAT_CHUNK_DATA_ENDL: Separator at end of each data chunk
 *    HTTP_CHAT_ERR_PROTO: Protocol error
 * @param data {char *} Start address of body data, may be NULL
 * @param dlen {int} Current data length
 * @return {int} If callback function returns -1, upper layer stops or
 *    aborts; otherwise upper layer continues
 */
typedef int  (*HTTP_BODY_NOTIFY)(int status, char *data, int dlen, void *arg);

/* Common status field definitions */
#define	HTTP_CHAT_OK                    0       /* Normal completion */
#define	HTTP_CHAT_CONTINUE              1       /* Internal use */
#define	HTTP_CHAT_DATA                  2       /* Partial data in body */
#define	HTTP_CHAT_CHUNK_HDR             3       /* Chunk header data */
#define HTTP_CHAT_CHUNK_DATA_ENDL       4       /* Separator in chunks */
#define	HTTP_CHAT_CHUNK_TRAILER         5       /* Last chunk header */
#define HTTP_CHAT_ERR_MIN               100     /* Minimum error value */
#define	HTTP_CHAT_ERR_IO                101     /* IO error */
#define	HTTP_CHAT_ERR_PROTO             102     /* Protocol error */
#define	HTTP_CHAT_ERR_TOO_MANY_LINES    103     /* Header too many lines */
#define HTTP_CHAT_ERR_MAX               1000    /* Error range */

/* Set flag bits */
#define	HTTP_CHAT_FLAG_BUFFED           0x0001

/* HTTP protocol header field definitions */
#define	HTTP_HDR_ENTRY_VIA              "via"   /* Prevent recursion */
#define	HTTP_HDR_ENTRY_FORWARD_FOR      "X-Forwarded-For"  /* Request hdr */

/* HTTP protocol request structure */
struct HTTP_REQ {
	HTTP_HDR_REQ *hdr_req;  /* From client */
	int  status;            /* Status, defined above: HTTP_STATUS_ */
	unsigned int flag;      /* Defined as: HTTP_CHAT_FLAG_XXX */
	void *ctx;
	void (*free_ctx)(void*);
};

struct HTTP_RES {
	HTTP_HDR_RES *hdr_res;  /* From client */
	int   read_cnt;
	int   status;           /* Status, defined above: HTTP_STATUS_ */
	unsigned int flag;      /* Defined as: HTTP_CHAT_FLAG_XXX */
	void *ctx;
	void (*free_ctx)(void*);
};

/* Name-value pair entry */
struct HTTP_HDR_ENTRY {
	char *name;
	char *value;
	int   off;
};

/* HTTP protocol header */

struct HTTP_HDR {
	/* Common members */
	char  proto[8];        /* Supported protocol: HTTP */
	struct {
		unsigned int major; /* Major version number */
		unsigned int minor; /* Minor version number */
	} version;

	http_off_t content_length; /* HTTP protocol body data length */

	/* Keep-alive: 0 -> no keep-alive, > 0 -> keep-alive, < 0 -> no field */
	short  keep_alive;
	short  chunked; /* This field is used for req/res body. For expansion */

	/* Internal variables */
	short  cur_lines;
	short  max_lines;
	short  valid_lines;
	short  status;
	short  keep_alive_count; /* Keep-alive counter */

	ACL_ARRAY  *entry_lnk;  /* Stores HTTP_HDR_ENTRY type elements */
	void *chat_ctx;
	void (*chat_free_ctx_fn)(void*);

	short  debug;            /* Debug flag for message header */
};

#define HDR_RESTORE(hdr_ptr, hdr_type, hdr_member) \
	((hdr_type *) (((char *) (hdr_ptr)) - offsetof(hdr_type, hdr_member)))

/* HTTP request header */
struct HTTP_HDR_REQ {
	HTTP_HDR hdr;       /* Common header, for common methods */

	int   port;         /* Server port number of the request */
	/* Request-specific members */
	char  method[32];   /* Request method: POST, GET, CONNECT */
	char  host[512];    /* Server domain or IP address */
	ACL_VSTRING *url_part; /*
                                * Stores backend part from request URL,
                                * e.g.:
                                * 1) http://test.com.cn/cgi-bin/test?name=value
                                *    => /cgi-bin/test?name=value
                                */
	ACL_VSTRING *url_path;  /*
                                 * Stores pure path from URL (without params),
                                 * e.g. for /path/test.cgi?name=value,
                                 * stores /path/test.cgi, remaining
                                 * params stored in url_params.
                                 */
	ACL_VSTRING *url_params; /* Stores parameters from URL */
	ACL_VSTRING *file_path;

	ACL_HTABLE *params_table; /* Stores various fields from URL params */
	ACL_HTABLE *cookies_table; /* Stores cookies */
	unsigned int flag;        /* Flag bits */
#define	HTTP_HDR_REQ_FLAG_PARSE_PARAMS	(1 << 0)
#define	HTTP_HDR_REQ_FLAG_PARSE_COOKIE	(1 << 1)
};

/* HTTP response header */

struct HTTP_HDR_RES {
	HTTP_HDR hdr;           /* Common header, for common methods */

	/* Response-specific members */
	int   reply_status;     /* Server response code: 100, 200, 404, etc */
};

#ifdef	__cplusplus
}
#endif

#endif


