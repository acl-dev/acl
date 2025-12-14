#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include "../stream/socket_stream.hpp"
#include <vector>

#if !defined(ACL_MIME_DISABLE)

struct SMTP_CLIENT;

namespace acl {

class istream;
class sslbase_conf;
class mail_message;

/**
 * SMTP email sending client class. This class object can be used to send emails and supports functions such as authentication.
 */
class ACL_CPP_API smtp_client : public noncopyable
{
public:
	/**
	 * Constructor
	 * @param addr {const char*} SMTP email server address, format: IP:PORT
	 *  or domain:port
	 * @param conn_timeout {int} Timeout for connecting to the server (seconds)
	 * @param rw_timeout {int} Network I/O timeout (seconds)
	 */
	smtp_client(const char* addr, int conn_timeout = 60,
		int rw_timeout = 60);
	~smtp_client();

	/**
	 * Call this function to send email data to the email server. This function will first call send_envelop
	 * to send the envelope. When email or message.get_email() is not empty, it will call the email sending
	 * process; otherwise (i.e., both email and message.get_email() are NULL), it will only send
	 * the envelope.
	 * @param message {const mail_messsage&} Email-related information, must be constructed in advance.
	 * @param email {const char*} When not empty, this file will be used as the email body data to send.
	 * @return {bool} Whether the sending was successful.
	 *  Note: If email is NULL and messsage.get_email() is also NULL, then this
	 *      function only sends the SMTP envelope part. The user also needs to call:
	 *      data_begin-->write|format|vformat|send_file-->data_end
	 *      process to send the email body data.
	 */
	bool send(const mail_message& message, const char* email = NULL);

	/**
	 * In the SMTP session phase, only send the email envelope part data. After the application calls this function successfully,
	 * it also needs to call:
	 * 1. data_begin: Start sending email body command
	 * 2. write/format/vformat/send_file: Send email data
	 * 3. data_end: Indicate that sending email body data is complete
	 * @param message {const mail_message&} Mail message object constructed for sending email.
	 * @return {bool} Whether successful.
	 *  Note: This function is a combination of the open/auth_login/mail_from/rcpt_to envelope sending process.
	 */
	bool send_envelope(const mail_message& message);

	/**
	 * Set SSL data transmission mode.
	 * @param ssl_conf {sslbase_conf*} When not empty, specifies to use SSL transmission mode.
	 * @return {smtp_client&}
	 */
	smtp_client& set_ssl(sslbase_conf* ssl_conf);

	/**
	 * Get the status code returned by the server in the last SMTP interaction process.
	 * @return {int}
	 */
	int get_code() const;

	/**
	 * Get the status information returned by the server in the last SMTP interaction process.
	 * @return {const char*}
	 */
	const char* get_status() const;

	/////////////////////////////////////////////////////////////////////

	/**
	 * Send email body data. This function can be called in a loop, but the data content must be in strict email format.
	 * @param data {const char*} Email content
	 * @param len {size_t} Email data length of data
	 * @return {bool} Whether the command operation was successful.
	 *  Note: Before the first call to this function, it must be ensured that the SMTP envelope has been successfully sent.
	 */
	bool write(const char* data, size_t len);

	/**
	 * Send email body data. This function can be called in a loop, but the data content must be in strict email format.
	 * @param fmt {const char*} Variable parameter format
	 * @return {bool} Whether the command operation was successful.
	 *  Note: Before the first call to this function, it must be ensured that the SMTP envelope has been successfully sent.
	 */
	bool format(const char* fmt, ...);

	/**
	 * Send email body data. This function can be called in a loop, but the data content must be in strict email format.
	 * @param fmt {const char*} Variable parameter format
	 * @param ap {va_list}
	 * @return {bool} Whether the command operation was successful.
	 *  Note: Before the first call to this function, it must be ensured that the SMTP envelope has been successfully sent.
	 */
	bool vformat(const char* fmt, va_list ap);

	/////////////////////////////////////////////////////////////////////

	/**
	 * Connect to remote SMTP server.
	 * @return {bool} Whether the connection was successful. If you want to use SSL mode, you need to call
	 *  set_ssl to set SSL communication mode after the class object is initialized.
	 */
	bool open();

	/**
	 * Actively close the connection with the SMTP server.
	 */
	void close();

	/**
	 * After the first successful connection, this function needs to be called to get the welcome message from the SMTP server.
	 * @return {bool} Whether successful.
	 */
	bool get_banner();

	/**
	 * After successfully calling get_banner, call this function to send HELO/HELO command to the SMTP server.
	 * @return {bool} Whether successful.
	 */
	bool greet();

	/**
	 * After successfully calling greet, call this function to send authentication command to the SMTP server.
	 * @param user {const char*} User account, non-empty string.
	 * @param pass {const char*} User account password, non-empty string.
	 * @return {bool} Whether successful.
	 */
	bool auth_login(const char* user, const char* pass);

	/**
	 * After successfully calling auth_login (if there is no authentication, it can be after successfully calling greet),
	 * call this function to send MAIL FROM command to the SMTP server.
	 * @param from {const char*} Sender's email address.
	 * @return {bool} Whether successful.
	 */
	bool mail_from(const char* from);

	/**
	 * After successfully calling mail_from, call this function to send RCPT TO command to the SMTP server,
	 * specifying a recipient. This function can be called multiple times to send emails to multiple recipients.
	 * @param to {const char*} Recipient's email address.
	 * @return {bool} Whether successful.
	 */
	bool rcpt_to(const char* to);

	/**
	 * After successfully calling rcpt_to or send_envelope, call this function to send
	 * DATA command to the SMTP server, indicating the start of sending email data.
	 * @return {bool} Whether the command operation was successful.
	 *  Note: Before calling this function, it must be ensured that the SMTP envelope has been successfully sent.
	 */
	bool data_begin();

	/**
	 * After successfully calling data_begin, call this function to send a complete email to the SMTP server.
	 * The path where the email is stored on disk needs to be provided.
	 * @param filepath {const char*} Email file path.
	 * @return {bool} Whether the command operation was successful.
	 *  Note: Before calling this function, it must be ensured that the SMTP envelope has been successfully sent.
	 */
	bool send_email(const char* filepath);

	/**
	 * After the email is sent (e.g., calling: send_email), this function must be called last to tell the SMTP
	 * email server that sending data is complete.
	 * @return {bool} Whether the command operation was successful.
	 */
	bool data_end();

	/**
	 * Disconnect from the email server.
	 * @return {bool} Whether the command operation was successful.
	 */
	bool quit();

	/**
	 * NOOP command.
	 * @return {bool} Whether the command operation was successful.
	 */
	bool noop();

	/**
	 * Reset the connection state with the email server.
	 * @return {bool} Whether the command operation was successful.
	 */
	bool reset();

	/**
	 * Get the connection stream object between this and the SMTP server. This function can only be called after open succeeds.
	 * @return {socket_stream&}
	 */
	socket_stream& get_stream(void)
	{
		return stream_;
	}

private:
	sslbase_conf* ssl_conf_;
	char* addr_;
	int   conn_timeout_;
	int   rw_timeout_;
	SMTP_CLIENT* client_;
	socket_stream stream_;
	bool  ehlo_;
	bool  reuse_;

	bool to_recipients(const std::vector<rfc822_addr*>& recipients);
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)

