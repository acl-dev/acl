#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include <vector>
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"
#include "../stream/socket_stream.hpp"
#include "../hsocket/hsproto.hpp"

#ifndef ACL_CLIENT_ONLY

struct ACL_ARGV;

namespace acl {

class hsrow;
class hstable;

class ACL_CPP_API hsclient : public noncopyable
{
public:
	/**
	 * Constructor
	 * @param addr {const char*} HandlerSocket server listening address on MySQL, format: ip:port
	 * @param cache_enable {bool} Whether to enable internal serialization cache functionality
	 * @param retry_enable {bool} Whether to retry when connection fails due to network reasons
	 *  Format: ip:port
	 */
	hsclient(const char* addr, bool cache_enable = true, bool retry_enable = true);
	~hsclient();

	/**
	 * Query database records matching field values.
	 * @param values {const char*[]} Matching field value array. The order of field values should match the order
	 *  of fields in the flds parameter of the open function.
	 * @param num {int} Length of values array. This value should correspond to the number of
	 *  fields in the flds array of the constructor.
	 * @param cond {const char*} Matching condition operator, default is:
	 *  = equal; >= greater than or equal; > greater than; < less than; <= less than or equal
	 * @param nlimit {int} Maximum number of query results, 0 means no limit on the number
	 * @param noffset {int} Starting position of query results (0 means starting from the first record)
	 * @return {const std::verctor<hsrow*>&} Returns results
	 */
	const std::vector<hsrow*>& get(const char* values[], int num,
		const char* cond = "=", int nlimit = 0, int noffset = 0);

	/**
	 * Query database records matching field values.
	 * @param first_value {const char*} Field value corresponding to the first field in the flds field set
	 *  of the constructor.
	 * @param ... {const char*} Variable parameter list. The last parameter must be NULL to indicate the end.
	 * @return {const std::verctor<hsrow*>&} Returns results
	 */
	const std::vector<hsrow*>& get(const char* first_value, ...)
		ACL_CPP_PRINTF(2, 3);

	/**
	 * Modify database records matching field values.
	 * @param values {const char*[]} Matching field value array. The order of field values should match the order
	 *  of fields in the flds parameter of the open function.
	 * @param num {int} Length of values array. This value should correspond to the number of
	 *  fields in the flds array of the constructor.
	 * @param to_values {cosnt *[]} Matching field new values. The order of field values should match the open function's
	 *  field order.
	 * @param to_num {int} Length of to_values array.
	 * @param cond {const char*} Matching condition operator, default is:
	 *  = equal; >= greater than or equal; > greater than; < less than; <= less than or equal
	 * @param nlimit {int} Maximum number of query results, 0 means no limit on the number
	 * @param noffset {int} Starting position of query results (0 means starting from the first record)
	 * @return {bool} Whether the operation was successful
	 */
	bool mod(const char* values[], int num,
		const char* to_values[], int to_num,
		const char* cond = "=", int nlimit = 0, int noffset = 0);

	/**
	 * Delete database records matching field values.
	 * @param values {const char*[]} Matching field value array. The order of field values should match the order
	 *  of fields in the flds parameter of the open function.
	 * @param num {int} Length of values array. This value should correspond to the number of
	 *  fields in the flds array of the constructor.
	 * @param cond {const char*} Matching condition operator, default is:
	 *  = equal; >= greater than or equal; > greater than; < less than; <= less than or equal
	 * @param nlimit {int} Number of records to delete, 0 means no limit
	 * @param noffset {int} Starting position of query results (0 means starting from the first record)
	 * @return {bool} Whether the operation was successful
	 */
	bool del(const char* values[], int num, const char* cond = "=",
		int nlimit = 0, int noffset = 0);

	/**
	 * Delete database records matching field values.
	 * @param first_value {const char*} Field value corresponding to the first field in the flds field set
	 *  of the constructor.
	 * @param ... {const char*} Variable parameter list. The last parameter must be NULL to indicate the end.
	 * @return {bool} Whether adding record was successful
	 */
	bool fmt_del(const char* first_value, ...) ACL_CPP_PRINTF(2, 3);

	/**
	 * Add database record.
	 * @param values {const char*[]} Matching field value array. The order of field values should match the constructor's
	 *  flds field order.
	 * @param num {int} Length of values array. This value should correspond to the number of
	 *  fields in the flds array of the constructor.
	 * @return {bool} Whether adding record was successful
	 */
	bool add(const char* values[], int num);

	/**
	 * Add database record using variable parameters.
	 * @param first_value {const char*} Field value corresponding to the first field in the flds field set
	 *  of the constructor.
	 * @param ... {const char*} Variable parameter list. The last parameter must be NULL to indicate the end.
	 * @return {bool} Whether adding record was successful
	 */
	bool fmt_add(const char* first_value, ...) ACL_CPP_PRINTF(2, 3);

	/**
	 * Set whether to enable debugging.
	 * @param on {bool} true means enable debugging, will output some intermediate information to the log.
	 */
	void debug_enable(bool on);

	/**
	 * Open database table.
	 * @param dbn {const char*} Database name
	 * @param tbl {const char*} Database table
	 * @param idx {const char*} Index field name
	 * @param flds {const char*} Field name set to open, format is
	 *  field names separated by separators ",; \t", e.g., user_id,user_name,user_mail
	 * @param auto_open {bool} Whether to automatically open if not opened
	 * @return {bool} true means table opened successfully, false means opening failed
	 */
	bool open_tbl(const char* dbn, const char* tbl,
		const char* idx, const char* flds, bool auto_open = true);

	/**
	 * Get connection address.
	 * @return {const char*} Never empty
	 */
	const char* get_addr() const;

	/**
	 * Get error code.
	 * @return {int}
	 */
	int  get_error() const;

	/**
	 * Get error message string.
	 * @param errnum {int} Error code obtained from get_error
	 * @return {const char*}
	 */
	const char* get_serror(int errnum) const;

	/**
	 * Get error message string from the last error.
	 * @return {const char*}
	 */
	const char* get_last_serror() const;

	/**
	 * Get the id number currently used by this hsclient object.
	 * @return {int}
	 */
	int get_id() const;
private:
	bool   debugOn_;
	char*  addr_;
	hsproto  proto_;
	bool   retry_enable_;
	int    id_max_;
	hstable* tbl_curr_;
	string buf_;

	// Network connection stream
	socket_stream stream_;
	std::map<string, hstable*> tables_;

	char   cond_def_[2];
	int    error_;
	const char* serror_;

	// Internal database table opening
	bool open_tbl(const char* dbn, const char* tbl,
		const char* idx, const char* flds, const char* key);

	// When reading and writing database operations fail, this function needs to be called
	// to close the connection stream and release resources.
	void close_stream();

	// Clear internally opened database table objects.
	void clear_tables();

	// Send database query command and get results.
	bool query(const char* oper, const char* values[], int num,
		const char* limit_offset, char mop,
		const char* to_values[], int to_num);
	bool chat();
};

} // namespace acl

#endif // ACL_CLIENT_ONLY

