#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <list>
#include <string>
#include "../stdlib/dbuf_pool.hpp"
#include "../stdlib/string.hpp"
#include "../mime/mime_attach.hpp"
#include "http_type.hpp"

#if !defined(ACL_MIME_DISABLE)

struct MIME_STATE;
struct MIME_NODE;

namespace acl {

/**
 * HTTP mime node class, inheritance relationship:
 *   http_mime_node : mime_attach : mime_node
 * Common function functionality:
 * http_mime_node::get_mime_type Get type of this node
 * mime_node::get_name: Get name of this node
 * mime_attach::get_filename: When node is upload file type, this function
 *   gets filename of uploaded file
 * http_mime_node::get_value: When node is parameter type, this function gets
 *   parameter value
 */
class ACL_CPP_API http_mime_node : public mime_attach {
public:
	/**
	 * @param path {const char*} Original file storage path, cannot be empty
	 * @param node {MIME_NODE*} Corresponding MIME node, non-empty
	 * @param decodeIt {bool} Whether to decode MIME node's header data
	 *  or body data
	 * @param toCharset {const char*} Local character set
	 * @param off {long long} Offset data position
	 */
	http_mime_node(const char* path, const MIME_NODE* node,
		bool decodeIt = true, const char* toCharset = "gb2312",
		long long off = 0);
	~http_mime_node();

	/**
	 * Get type of this node
	 * @return {http_mime_t}
	 */
	http_mime_t get_mime_type() const;

	/**
	 * When type returned by get_mime_type is HTTP_MIME_PARAM, can
	 * call this function to get parameter value. Parameter name can be obtained
	 * through base class get_name()
	 * @return {const char*} Returns NULL indicates parameter does not exist
	 */
	const char* get_value() const;

private:
	http_mime_t mime_type_;
	std::string param_value_;

	void load_param(const char* path);
};

////////////////////////////////////////////////////////////////////////////////

/**
 * HTTP mime parser. This parser is a streaming parser. Users can input only
 * partial data to update function each time. When this function returns true,
 * it indicates parsing is complete and correct
 */
class ACL_CPP_API http_mime : public dbuf_obj {
public:
	/**
	 * Constructor
	 * @param boundary {const char*} Delimiter, cannot be empty
	 * @param local_charset {const char*} Local character set. When not empty,
	 * automatically converts
	 *  parameter content to local character set
	 */
	http_mime(const char* boundary, const char* local_charset  = "utf8");
	~http_mime();

	const char* get_boundary() const {
		return boundary_.c_str();
	}
	const char* get_local_charset() const {
		return local_charset_;
	}

	/**
	 * Set storage path for MIME data. After analyzing MIME data, if want to
	 * extract data from it, must provide storage location of original data of this
	 * MIME, otherwise
	 * cannot get corresponding data, i.e., save_xxx/get_nodes/get_node functions
	 * cannot
	 * be used normally
	 * @param path {const char*} File path name. If this parameter is empty, cannot
	 *  get body data, and cannot call save_xxx related interfaces
	 */
	void set_saved_path(const char* path);

	////////////////////////////////////////////////////////////////////////////

	/**
	 * Call this function to parse body content in streaming mode
	 * @param data {const char*} Body data (maybe be data header or body data,
	 *  and does not need to be complete data line)
	 * @param len {size_t} data data length
	 * @return {bool} For multipart data, returns true indicates parsing is
	 * complete;
	 *  For non-multipart files, this return value is always false, has no meaning,
	 *  need caller to judge end position of body data itself
	 * Note: After calling this function, must call update_end function to notify
	 * parser
	 * that parsing is complete
	 */
	bool update(const char* data, size_t len);

	/**
	 * Get all MIME nodes
	 * @return {const std::list<http_mimde_node*>&}
	 */
	const std::list<http_mime_node*>& get_nodes() const;

	/**
	 * Get HTTP MIME node based on variable name
	 * @param name {const char*} Variable name
	 * @return {const http_mime_node*} Returns NULL indicates node corresponding to
	 * variable name
	 *  does not exist
	 */
	const http_mime_node* get_node(const char* name) const;

	////////////////////////////////////////////////////////////////////////////

	http_mime& add_param(const char* name, const char* value);
	http_mime& add_file(const char* name, const char* filepath);

	bool save_to(const char* file_path);
	bool save_to(ostream& out);

	bool save_to(string& out);

private:
	std::string boundary_;
	string save_path_;
	long long off_;
	MIME_STATE* mime_state_;
	std::list<http_mime_node*> mime_nodes_; // For parsing
	char  local_charset_[32];
	bool  decode_on_;
	bool  parsed_;

private:
	typedef std::pair<std::string, std::string> pair_value;
	std::map<std::string, std::string> params_;
	std::map<std::string, pair_value>  files_;

	static bool save_param(ostream& out, const std::string& bound,
		const std::pair<std::string, std::string>& param);
	static bool save_file(ostream& out, const std::string& bound,
		const std::pair<std::string, pair_value>& file);

	static void save_param(string& out, const std::string& bound,
		const std::pair<std::string, std::string>& param);
	static bool save_file(string& out, const std::string& bound,
		const std::pair<std::string, pair_value>& file);

	static void get_ctype(const char* filename, std::string& ctype);
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)

