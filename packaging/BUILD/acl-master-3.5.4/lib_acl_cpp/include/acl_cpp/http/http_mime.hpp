#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <list>
#include "../stdlib/dbuf_pool.hpp"
#include "../stdlib/string.hpp"
#include "../mime/mime_attach.hpp"
#include "http_type.hpp"

#if !defined(ACL_MIME_DISABLE)

struct MIME_STATE;
struct MIME_NODE;

namespace acl {

/**
 * http mime 结点类，继承关系：
 *   http_mime_node : mime_attach : mime_node
 * 常用函数功能：
 * http_mime_node::get_mime_type 获得该结点的类型
 * mime_node::get_name: 获得该结点的名称
 * mime_attach::get_filename: 当结点为上传文件类型时，此函数
 *   获得上传文件的文件名
 * http_mime_node::get_value: 当结点为参数类型时，此函数获得
 *   参数值
 */
class ACL_CPP_API http_mime_node : public mime_attach
{
public:
	/**
	 * @param path {const char*} 原始文件存放路径，不能为空
	 * @param node {MIME_NODE*} 对应的 MIME 结点，非空
	 * @param decodeIt {bool} 是否对 MIME 结点的头部数据
	 *  或数据体数据进行解码
	 * @param toCharset {const char*} 本机的字符集
	 * @param off {off_t} 偏移数据位置
	 */
	http_mime_node(const char* path, const MIME_NODE* node,
		bool decodeIt = true, const char* toCharset = "gb2312",
		off_t off = 0);
	~http_mime_node(void);

	/**
	 * 获得该结点的类型
	 * @return {http_mime_t}
	 */
	http_mime_t get_mime_type(void) const;

	/**
	 * 当 get_mime_type 返回的类型为 HTTP_MIME_PARAM 时，可以
	 * 调用此函数获得参数值；参数名可以通过基类的 get_name() 获得
	 * @return {const char*} 返回 NULL 表示参数不存在
	 */
	const char* get_value(void) const;

protected:

private:
	http_mime_t mime_type_;
	char* param_value_;

	void load_param(const char* path);
};

//////////////////////////////////////////////////////////////////////////

/**
 * http mime 解析器，该解析器为流式解析器，用户在使用时可以每次仅输入
 * 部分数据给 update 函数，当该函数返回 true 时表示解析完成且解析正确
 */
class ACL_CPP_API http_mime : public dbuf_obj
{
public:
	/**
	 * 构建函数
	 * @param boundary {const char*} 分隔符，不能为空
	 * @param local_charset {const char*} 本地字符集，非空时会自动将
	 *  参数内容转为本地字符集
	 */
	http_mime(const char* boundary, const char* local_charset  = "gb2312");
	~http_mime(void);

	/**
	 * 设置 MIME 数据的存储路径，当分析完 MIME 数据后，如果想要
	 * 从中提取数据，则必须给出该 MIME 的原始数据的存储位置，否则
	 * 无法获得相应数据，即 save_xxx/get_nodes/get_node 函数均无法
	 * 正常使用
	 * @param path {const char*} 文件路径名, 如果该参数为空, 则不能
	 *  获得数据体数据, 也不能调用 save_xxx 相关的接口
	 */
	void set_saved_path(const char* path);

	/**
	 * 调用此函数进行流式方式解析数据体内容
	 * @param data {const char*} 数据体(可能是数据头也可能是数据体, 
	 *  并且不必是完整的数据行)
	 * @param len {size_t} data 数据长度
	 * @return {bool} 针对 multipart 数据, 返回 true 表示解析完毕;
	 *  对于非 multipart 文件, 该返回值永远为 false, 没有任何意义, 
	 *  需要调用者自己判断数据体的结束位置
	 * 注意: 调用完此函数后一定需要调用 update_end 函数通知解析器
	 * 解析完毕
	 */
	bool update(const char* data, size_t len);

	/**
	 * 获得所有的 MIME 结点
	 * @return {const std::list<http_mimde_node*>&}
	 */
	const std::list<http_mime_node*>& get_nodes(void) const;

	/**
	 * 根据变量名取得 HTTP MIME 结点
	 * @param name {const char*} 变量名
	 * @return {const http_mime_node*} 返回空则说明对应变量名的结点
	 *  不存在
	 */
	const http_mime_node* get_node(const char* name) const;

private:
	string boundary_;
	string save_path_;
	off_t off_;
	MIME_STATE* mime_state_;
	std::list<http_mime_node*> mime_nodes_;
	char  local_charset_[32];
	bool  decode_on_;
	bool  parsed_;
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
