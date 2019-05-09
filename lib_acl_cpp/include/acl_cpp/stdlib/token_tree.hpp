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
 * 256 叉匹配树中的节点对象，为纯私有类
 */
class ACL_CPP_API token_node : public noncopyable
{
public:
	/**
	 * 获得该节点对应的键值
	 * @return {const char*}
	 */
	const char* get_key(void) const;

	/**
	 * 获得该节点所绑定的对象地址
	 * @return {void*}
	 */
	void* get_ctx(void) const;

	/**
	 * 获得该节点所属的匹配树对象
	 * @return {token_tree*}
	 */
	token_tree* get_tree(void) const
	{
		return tree_;
	}

	/**
	 * 获得 C 版本的节点对象
	 * @return {ACL_TOKEN*}
	 */
	ACL_TOKEN* get_token(void) const
	{
		return me_;
	}

private:
	friend class token_tree;	// 仅允许 token_tree 构造/析构本类对象

	token_node(void);
	~token_node(void);

	void set_node(ACL_TOKEN* token, token_tree* tree);

private:
	ACL_TOKEN*  me_;
	token_tree* tree_;
	string      key_;
	bool        dirty_;
};

/**
 * 256 叉树最大匹配查找算法，该算法具有通用性及非常高的性能(比哈希性能还高)，
 * 通过将字符串映射到 256 叉树上进行匹配查找
 */
class ACL_CPP_API token_tree : public noncopyable
{
public:
	token_tree(void);
	~token_tree(void);

	/**
	 * 添加一个新的项
	 * @param key {const char*} 键值
	 * @param ctx {void*} 该 key 所绑定的对象，可以为空
	 * @return {bool} 添加是否成功，返回 false 表明相同 key 已存在
	 */
	bool insert(const char* key, void* ctx = NULL);

	/**
	 * 从匹配树中删除指定的 key 项
	 * @param key {const char*} 键值
	 * @return {void*} 返回添加时绑定的对象地址
	 */
	void* remove(const char* key);

	/**
	 * 根据键值精确查找匹配的节点
	 * @param key {const char*} 键值
	 * @return {const token_node*} 返回 NULL 表示未找到匹配项
	 */
	const token_node* find(const char* key);

	/**
	 * 按字符串最大匹配模式从匹配中查找与所给文本字符串相匹配的节点，同时
	 * 移动文本字符串的指针位置
	 * @param text {const char**} 要匹配查找的文本字符串，在匹配过程中，该
	 *  地址指针会被移动至下一位置
	 * @param delimiters {const char*} 非 NULL 时指定的截止符字符串，即查
	 *  找过程中只要遇到的字符在该截止字符串中，则返回本次查找的结果
	 * @param delimiters_tab {const char*} 非 NULL 时指定的截止符字符数组，
	 *  即查找过程中只要遇到的字符在该截止字符数组中，则返回本次查找的结果，该数组
	 *  必须由 create_delimiters_tab 创建，由 free_delimiters_tab 释放
	 * @return {token_node*} 返回 NULL 表示本次查找未找到匹配项，通过检查
	 *  *text 是否为 '\0' 表示是否匹配完毕目标文本字符串
	 *  注：当 delimiters 非空时优先使用 delimiters 做为截止符，否则再检查
	 *  delimiters_tab 是否非空，如果非空则使用其做为截止符
	 */
	const token_node* search(const char** text, const char* delimiters = NULL,
		const char* delimiters_tab = NULL);

	/**
	 * 创建截止符数组
	 * @param delimiters {const char*} 截止符字符串
	 * @return {char*} 根据截止符字符串创建的截止符数组
	 */
	static char* create_delimiters_tab(const char* delimiters);

	/**
	 * 释放由 create_delimiters_tab 创建的截止符数组
	 * @param delimiters_tab {char*}
	 */
	static void free_delimiters_tab(char* delimiters_tab);

	/**
	 * 遍历 256 匹配树时需先调用本方法获得第一个节点对象
	 * @return {token_node*}
	 */
	const token_node* first_node(void);

	/**
	 * 遍历 256 匹配树时需先调用本方法获得下一个节点对象
	 * @return {token_node*}
	 */
	const token_node* next_node(void);

	/**
	 * 获得 C 版本的 256 叉树对象
	 * @return {ACL_TOKEN*}
	 */
	ACL_TOKEN* get_tree(void) const
	{
		return tree_;
	}

private:
	ACL_TOKEN* tree_;
	ACL_ITER*  iter_;
	token_node node_;
};

} // namespace acl
