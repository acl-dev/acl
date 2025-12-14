#pragma once
#include "../acl_cpp_define.hpp"
#include "noncopyable.hpp"
#include "string.hpp"
#include <list>

struct ACL_TOKEN;
struct ACL_ITER;

namespace acl {

class token_tree;

/**
 * Node object in 256-way matching tree, pure private class
 */
class ACL_CPP_API token_node : public noncopyable {
public:
	/**
	 * Get key value corresponding to this node
	 * @return {const char*}
	 */
	const char* get_key() const;

	/**
	 * Get object address bound to this node
	 * @return {void*}
	 */
	void* get_ctx() const;

	/**
	 * Get matching tree object this node belongs to
	 * @return {token_tree*}
	 */
	token_tree* get_tree() const {
		return tree_;
	}

	/**
	 * Get C version node object
	 * @return {ACL_TOKEN*}
	 */
	ACL_TOKEN* get_token() const {
		return me_;
	}

private:
	friend class token_tree;	// Only token_tree is allowed to construct/destruct objects of this class

	token_node();
	~token_node();

	void set_node(ACL_TOKEN* token, token_tree* tree);

private:
	ACL_TOKEN*  me_;
	token_tree* tree_;
	string      key_;
	bool        dirty_;
};

/**
 * 256-way tree maximum matching search algorithm. This algorithm is universal
 * and has very high performance (higher than hash performance).
 * Performs matching search by mapping strings to 256-way tree
 */
class ACL_CPP_API token_tree : public noncopyable {
public:
	token_tree();
	~token_tree();

	/**
	 * Add a new item
	 * @param key {const char*} Key value
	 * @param ctx {void*} Object bound to this key, can be empty
	 * @return {bool} Whether add was successful. Returns false indicates same key
	 * already exists
	 */
	bool insert(const char* key, void* ctx = NULL);

	/**
	 * Delete specified key item from matching tree
	 * @param key {const char*} Key value
	 * @return {void*} Returns object address bound when adding
	 */
	void* remove(const char* key);

	/**
	 * Find matching node exactly based on key value
	 * @param key {const char*} Key value
	 * @return {const token_node*} Returns NULL indicates matching item not found
	 */
	const token_node* find(const char* key);

	/**
	 * Search matching tree in maximum string matching mode to find node matching
	 * given text string, and
	 * move text string pointer position
	 * @param text {const char**} Text string to match and search. During matching
	 * process, this
	 *  address pointer will be moved to next position
	 * @param delimiters {const char*} When not NULL, specifies delimiter string,
	 * i.e., during
	 * search process, as long as encountered character is in this delimiter
	 * string, returns result of this search
	 * @param delimiters_tab {const char*} When not NULL, specifies delimiter
	 * character array,
	 * i.e., during search process, as long as encountered character is in this
	 * delimiter character array, returns result of this search. This array
	 * must be created by create_delimiters_tab and released by free_delimiters_tab
	 * @return {token_node*} Returns NULL indicates this search did not find
	 * matching item. Check
	 * whether *text is '\0' to determine if target text string matching is
	 * complete
	 * Note: When delimiters is not empty, delimiters is used as delimiter first,
	 * otherwise check
	 *  whether delimiters_tab is not empty, if not empty, use it as delimiter
	 */
	const token_node* search(const char** text, const char* delimiters = NULL,
		const char* delimiters_tab = NULL);

	/**
	 * Create delimiter array
	 * @param delimiters {const char*} Delimiter string
	 * @return {char*} Delimiter array created based on delimiter string
	 */
	static char* create_delimiters_tab(const char* delimiters);

	/**
	 * Release delimiter array created by create_delimiters_tab
	 * @param delimiters_tab {char*}
	 */
	static void free_delimiters_tab(char* delimiters_tab);

	/**
	 * When traversing 256 matching tree, need to call this method first to get
	 * first node object
	 * @return {token_node*}
	 */
	const token_node* first_node();

	/**
	 * When traversing 256 matching tree, need to call this method to get next node
	 * object
	 * @return {token_node*}
	 */
	const token_node* next_node();

	/**
	 * Get C version 256-way tree object
	 * @return {ACL_TOKEN*}
	 */
	ACL_TOKEN* get_tree() const {
		return tree_;
	}

private:
	ACL_TOKEN* tree_;
	ACL_ITER*  iter_;
	token_node node_;
};

} // namespace acl

