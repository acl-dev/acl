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
#elif defined(SMTP_DLL) /* || defined(_WINDLL) */
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
 * Connect to remote SMTP server
 * @param addr {const char*} SMTP server address, format: domain:port
 * @param conn_timeout {int} Connection timeout
 * @param rw_timeout {int} IO read/write timeout
 * @param line_limit {int} Max line length in SMTP session communication
 * @return {SMTP_CLIENT*} Returns non-NULL on success, NULL on failure
 */
SMTP_API SMTP_CLIENT *smtp_open(const char *addr, int conn_timeout,
	int rw_timeout, int line_limit);

/**
 * Close SMTP connection opened by smtp_open and free object
 * @param client {SMTP_CLIENT*} SMTP connection object
 */
SMTP_API void smtp_close(SMTP_CLIENT *client);

/**
 * Read welcome message from SMTP server
 * @param client {SMTP_CLIENT*} SMTP connection object
 * @return {int} 0 indicates success (SMTP_CLIENT::smtp_code indicates
 *  server code, SMTP_CLIENT::buf stores response data), otherwise indicates
 *  error and connection should be closed
 */
SMTP_API int smtp_get_banner(SMTP_CLIENT *client);

/**
 * Send HELO/EHLO command to SMTP server
 * @param client {SMTP_CLIENT*} SMTP connection object
 * @param name {const char*} Greeting info, usually hostname
 * @param ehlo {int} When non-zero, use EHLO; otherwise use HELO
 * @return {int} 0 indicates success (SMTP_CLIENT::smtp_code indicates
 *  server code, SMTP_CLIENT::buf stores response data), otherwise indicates
 *  error and connection should be closed
 */

SMTP_API int smtp_greet(SMTP_CLIENT *client, const char* name, int ehlo);

/**
 * Send HELO command to SMTP server
 * @param client {SMTP_CLIENT*} SMTP connection object
 * @param helo {const char*} Greeting info, usually hostname
 * @return {int} 0 indicates success (SMTP_CLIENT::smtp_code indicates
 *  server code, SMTP_CLIENT::buf stores response data), otherwise indicates
 *  error and connection should be closed
 */
SMTP_API int smtp_helo(SMTP_CLIENT *client, const char *helo);

/**
 * Send EHLO command to SMTP server
 * @param client {SMTP_CLIENT*} SMTP connection object
 * @param ehlo {const char*} Greeting info, usually hostname
 * @return {int} 0 indicates success (SMTP_CLIENT::smtp_code indicates
 *  server code, SMTP_CLIENT::buf stores response data), otherwise indicates
 *  error and connection should be closed
 */
SMTP_API int smtp_ehlo(SMTP_CLIENT *client, const char *ehlo);

/**
 * Send authentication info to SMTP server
 * @param client {SMTP_CLIENT*} SMTP connection object
 * @param user {const char*} SMTP email account
 * @param pass {const char*} SMTP email account password
 * @return {int} 0 indicates success (SMTP_CLIENT::smtp_code indicates
 *  server code, SMTP_CLIENT::buf stores response data), otherwise indicates
 *  error and connection should be closed
 */
SMTP_API int smtp_auth(SMTP_CLIENT *client, const char *user, const char *pass);

/**
 * Send MAIL FROM command to SMTP server
 * @param client {SMTP_CLIENT*} SMTP connection object
 * @param from {const char*} Sender address
 * @return {int} 0 indicates success (SMTP_CLIENT::smtp_code indicates
 *  server code, SMTP_CLIENT::buf stores response data), otherwise indicates
 *  error and connection should be closed
 */
SMTP_API int smtp_mail(SMTP_CLIENT *client, const char *from);

/**
 * Send RCPT TO command to SMTP server
 * @param client {SMTP_CLIENT*} SMTP connection object
 * @param to {const char*} Recipient address
 * @return {int} 0 indicates success (SMTP_CLIENT::smtp_code indicates
 *  server code, SMTP_CLIENT::buf stores response data), otherwise indicates
 *  error and connection should be closed
 */
SMTP_API int smtp_rcpt(SMTP_CLIENT *client, const char *to);

/**
 * Send DATA command to SMTP server
 * @param client {SMTP_CLIENT*} SMTP connection object
 * @return {int} 0 indicates success (SMTP_CLIENT::smtp_code indicates
 *  server code, SMTP_CLIENT::buf stores response data), otherwise indicates
 *  error and connection should be closed
 */
SMTP_API int smtp_data(SMTP_CLIENT *client);

/**
 * Send email body data to SMTP server. Loop calling this function until
 * all data sent
 * @param client {SMTP_CLIENT*} SMTP connection object
 * @param src {const char*} Email data in MIME format
 * @param len {size_t} src data length
 * @return {int} 0 indicates success (SMTP_CLIENT::smtp_code indicates
 *  server code, SMTP_CLIENT::buf stores response data), otherwise indicates
 *  error and connection should be closed
 */
SMTP_API int smtp_send(SMTP_CLIENT *client, const char* src, size_t len);

/**
 * Send email body data to SMTP server. Loop calling this function until
 * all data sent
 * @param client {SMTP_CLIENT*} SMTP connection object
 * @param fmt {const char*} Format string
 * @param ... Parameters
 * @return {int} 0 indicates success (SMTP_CLIENT::smtp_code indicates
 *  server code, SMTP_CLIENT::buf stores response data), otherwise indicates
 *  error and connection should be closed
 */
SMTP_API int smtp_printf(SMTP_CLIENT *client, const char* fmt, ...);

/**
 * After sending all email data, call this to notify SMTP server that
 * email sending is complete
 * @param client {SMTP_CLIENT*} SMTP connection object
 * @return {int} 0 indicates success (SMTP_CLIENT::smtp_code indicates
 *  server code, SMTP_CLIENT::buf stores response data), otherwise indicates
 *  error and connection should be closed
 */
SMTP_API int smtp_data_end(SMTP_CLIENT *client);

/**
 * Send email file at specified path to SMTP server
 * @param client {SMTP_CLIENT*} SMTP connection object
 * @param filepath {const char*} Email file path
 * @return {int} 0 indicates success (SMTP_CLIENT::smtp_code indicates
 *  server code, SMTP_CLIENT::buf stores response data), otherwise indicates
 *  error and connection should be closed
 */
SMTP_API int smtp_send_file(SMTP_CLIENT *client, const char *filepath);

/**
 * Send email content from stream to SMTP server
 * @param client {SMTP_CLIENT*} SMTP connection object
 * @param in {ACL_VSTREAM*} Email file stream
 * @return {int} 0 indicates success (SMTP_CLIENT::smtp_code indicates
 *  server code, SMTP_CLIENT::buf stores response data), otherwise indicates
 *  error and connection should be closed
 */
SMTP_API int smtp_send_stream(SMTP_CLIENT *client, ACL_VSTREAM *in);

/**
 * Send quit (QUIT) command to SMTP server
 * @param client {SMTP_CLIENT*} SMTP connection object
 * @return {int} 0 indicates success (SMTP_CLIENT::smtp_code indicates
 *  server code, SMTP_CLIENT::buf stores response data), otherwise indicates
 *  error and connection should be closed
 */
SMTP_API int smtp_quit(SMTP_CLIENT *client);

/**
 * Send NOOP command to SMTP server
 * @param client {SMTP_CLIENT*} SMTP connection object
 * @return {int} 0 indicates success (SMTP_CLIENT::smtp_code indicates
 *  server code, SMTP_CLIENT::buf stores response data), otherwise indicates
 *  error and connection should be closed
 */
SMTP_API int smtp_noop(SMTP_CLIENT *client);

/**
 * Send RSET command to SMTP server
 * @param client {SMTP_CLIENT*} SMTP connection object
 * @return {int} 0 indicates success (SMTP_CLIENT::smtp_code indicates
 *  server code, SMTP_CLIENT::buf stores response data), otherwise indicates
 *  error and connection should be closed
 */
SMTP_API int smtp_rset(SMTP_CLIENT *client);

#ifdef __cplusplus
}
#endif

#endif

