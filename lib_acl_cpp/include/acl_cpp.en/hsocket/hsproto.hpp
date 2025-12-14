#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <vector>

#ifndef ACL_CLIENT_ONLY

namespace acl {

class string;
class hsrow;

class ACL_CPP_API hsproto : public noncopyable
{
public:
	hsproto(bool cache_enable);
	~hsproto();

	/**
	 * Build database table opening protocol string.
	 * @param out {string&} Storage protocol string.
	 * @param id {int} Corresponding table ID number.
	 * @param dbn {const char*} Database name.
	 * @param tbl {const char*} Database table.
	 * @param idx {const char*} Index field name.
	 * @param flds {const char*} Field name set to open, format is
	 *  field names separated by separators ",; \t", e.g., user_id,user_name,user_mail
	 * @return {bool} Whether successful.
	 */
	static bool build_open(string& out, int id,
		const char* dbn, const char* tbl,
		const char* idx, const char* flds);

	/**
	 * Build query database record protocol string.
	 * @param out {string&} Storage protocol string.
	 * @param id {int} Corresponding table ID number.
	 * @param values {const char*[]} Matching field value array. The order of field values should match the order
	 *  of fields in the constructor.
	 * @param num {int} Length of values array. This value should correspond to the number of fields when creating the table.
	 * @param cond {const char*} Matching condition operator, default is:
	 *  = equal; >= greater than or equal; > greater than; < less than; <= less than or equal
	 * @param nlimit {int} Maximum number of query results, 0 means no limit on the number.
	 * @param noffset {int} Starting position of query results (0 means starting from the first record).
	 * @return {bool} Whether successful.
	 */
	static bool build_get(string& out, int id,
		const char* values[], int num,
		const char* cond = "=", int nlimit = 0, int noffset = 0);

	/**
	 * Build query database record protocol string.
	 * @param out {string&} Storage protocol string.
	 * @param id {int} Corresponding table ID number.
	 * @param nfld {int} Number of fields when creating the table.
	 * @param first_value {const char*} First parameter.
	 * @param ... {const char*} Variable parameter list. The last parameter must be NULL to indicate the end.
	 * @return {bool} Whether successful.
	 */
	static bool ACL_CPP_PRINTF(4, 5) build_get(string& out, int id,
		int nfld, const char* first_value, ...);

	/**
	 * Build modify database record protocol string.
	 * @param out {string&} Storage protocol string.
	 * @param id {int} Corresponding table ID number.
	 * @param values {const char*[]} Matching field value array. The order of field values should match the order
	 *  of fields in the constructor.
	 * @param num {int} Length of values array. This value should correspond to the number of fields when creating the table.
	 * @param to_values {cosnt *[]} Matching field new values. The order of field values should match the open function's
	 *  field order.
	 * @param to_num {int} Length of to_values array.
	 * @param cond {const char*} Matching condition operator, default is:
	 * @param nlimit {int} Maximum number of query results, 0 means no limit on the number.
	 * @param noffset {int} Starting position of query results (0 means starting from the first record).
	 * @return {bool} Whether successful.
	 */
	static bool build_mod(string& out, int id,
		const char* values[], int num,
		const char* to_values[], int to_num,
		const char* cond = "=", int nlimit = 0, int noffset = 0);

	/**
	 * Build delete database record protocol string.
	 * @param out {string&} Storage protocol string.
	 * @param id {int} Corresponding table ID number.
	 * @param values {const char*[]} Matching field value array. The order of field values should match the order
	 *  of fields in the constructor.
	 * @param num {int} Length of values array. This value should correspond to the number of fields when creating the table.
	 * @param cond {const char*} Matching condition operator, default is:
	 * @param nlimit {int} Maximum number of query results, 0 means no limit on the number.
	 * @param noffset {int} Starting position of query results (0 means starting from the first record).
	 * @return {bool} Whether successful.
	 */
	static bool build_del(string& out, int id, const char* values[],
		int num, const char* cond = "=",
		int nlimit = 0, int noffset = 0);

	/**
	 * Build delete database record protocol string.
	 * @param out {string&} Storage protocol string.
	 * @param id {int} Corresponding table ID number.
	 * @param nfld {int} Number of fields when creating the table.
	 * @param first_value {const char*} First parameter.
	 * @param ... {const char*} Variable parameter list. The last parameter must be NULL to indicate the end.
	 * @return {bool} Whether successful.
	 */
	static bool ACL_CPP_PRINTF(4, 5) build_del(string& out, int id,
		int nfld, const char* first_value, ...);

	/**
	 * Build add database record protocol string.
	 * @param out {string&} Storage protocol string.
	 * @param id {int} Corresponding table ID number.
	 * @param values {const char*[]} Matching field value array. The order of field values should match the order
	 *  of fields in the constructor.
	 * @param num {int} Length of values array. This value should correspond to the number of fields when creating the table.
	 * @return {bool} Whether successful.
	 */
	static bool build_add(string& out, int id,
		const char* values[], int num);

	/**
	 * Build add database record protocol string.
	 * @param out {string&} Storage protocol string.
	 * @param id {int} Corresponding table ID number.
	 * @param nfld {int} Number of fields when creating the table.
	 * @param first_value {const char*} First parameter.
	 * @param ... {const char*} Variable parameter list. The last parameter must be NULL to indicate the end.
	 * @return {bool} Whether successful.
	 */
	static bool ACL_CPP_PRINTF(4, 5) build_add(string& out, int id,
		int nfld, const char* first_value, ...);

	/**
	 * Generic build database operation protocol string.
	 * @param out {string&} Storage protocol string.
	 * @param id {int} Corresponding table ID number.
	 * @param oper {const char*} Operation type. Corresponding operators are:
	 *  Add: +
	 *  Query: =, >, >=, <, <=
	 *  Modify: =, >, >=, <, <=
	 *  Delete: =, >, >=, <, <=
	 * @param values {const char*[]} Matching field value array. The order of field values should match the order
	 *  of fields in the constructor.
	 * @param num {int} Length of values array. This value should correspond to the number of fields when creating the table.
	 * @param limit_offset {const char*} Query range to be requested.
	 * @param mop {char} Operation type for delete/modify operations. Corresponding operations are:
	 *  D: delete, U: modify
	 * @param to_values {const char*[]} Target value pointer array.
	 * @param to_num {int} Length of to_values array.
	 */
	static void build_request(string& out, int id, const char* oper,
		const char* values[], int num,
		const char* limit_offset, char mop,
		const char* to_values[], int to_num);

	/**
	 * Parse database server response.
	 * @param nfld {int} Number of table elements opened.
	 * @param in {string&} Database server response data, tail should not contain "\r\n".
	 * @param errnum_out {int&} Store error code from operation process, see: hserror.hpp
	 * @param serror_out {const char*&} Store error message string from operation process.
	 * @return {bool} Whether operation was successful.
	 */
	bool parse_respond(int nfld, string& in,
                int& errnum_out, const char*& serror_out);

	/**
	 * When executing query operation, get query results through this method.
	 * @return {const std::vector<hsrow*>&}
	 */
	const std::vector<hsrow*>& get();

	/**
	 * When user needs to perform a second query operation, call this method to clear the last query result.
	 */
	void reset();
private:
	bool  debugOn_;
	bool  cache_enable_;
	//int   nfld_;
	int   ntoken_;
	char* buf_ptr_;

	// Query results
	std::vector<hsrow*> rows_;

	// Intermediate result cache. Intermediate results are serialized to ensure memory
	// safety. The intermediate results are serialized and cached to avoid
	// memory leaks.
	std::vector<hsrow*> rows_cache_;

	// Clear serialized result collection.
	void clear_cache();

	// Get next query result.
	hsrow* get_next_row();
};

}  // namespace acl

#endif // ACL_CLIENT_ONLY

