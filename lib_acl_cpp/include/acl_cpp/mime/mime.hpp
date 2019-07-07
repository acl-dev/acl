#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <list>
#include <string>
#include "mime_head.hpp"

#if !defined(ACL_MIME_DISABLE)

struct MIME_STATE;

namespace acl {

class string;
class mime_node;
class mime_attach;
class mime_body;
class mime_image;
class ifstream;
class fstream;

class ACL_CPP_API mime : public noncopyable
{
public:
	mime(void);
	~mime(void);

	/////////////////////////////////////////////////////////////////////
	// 与邮件解析相关的函数

	/**
	 * 当用同一个MIME解析器对多封邮件解析时, 需要调用此函数清理之前
	 * 解析过程中产生的内存, 虽然多次调用该函数无害, 但为了不影响
	 * 效率, 最好在利用该解析器解析下一封邮件前调用该函数
	 */
	mime& reset(void);

	/**
	 * 调用者可以手工调用此函数以触发分析邮件头过程
	 */
	void primary_head_finish(void);

	/**
	 * 调用流式分析时用此函数判断邮件头是否解析完毕
	 * @return {bool} 是否邮件头解析完毕
	 */
	bool primary_head_ok(void) const;

	/**
	 * 开始进行流式解析过程, 该函数内部会自动调用 reset() 函数以重置解析
	 * 器状态
	 * @param path {const char*} 邮件文件路径名, 如果该参数为空, 则不能
	 *  获得邮件体数据, 也不能调用 save_xxx 相关的接口
	 */
	void update_begin(const char* path);

	/**
	 * 调用此函数进行流式方式解析邮件内容, 如果仅想解析邮件头, 则可以用此
	 * 接口解析完邮件头后调用 update_end() 接口即可, 如果想要解析完整的一
	 * 封邮件, 则需要不断地调用此函数直到本函数返回 true 表示 multipart 格式
	 * 的邮件解析完毕; 如果不是 multipart 格式邮件, 则此函数不可能会回返 true,
	 * 调用者需要自行判断邮件的结束位置
	 * @param data {const char*} 邮件数据(可能是邮件头也可能是邮件体, 并且
	 *  不必是完整的数据行)
	 * @param len {size_t} data 数据长度
	 * @return {bool} 针对 multipart 邮件, 返回 true 表示该封邮件结束完毕;
	 *  对于非 multipart 邮件, 该返回值永远为 false, 没有任何意义, 需要调用
	 *  者自己判断邮件的结束位置
	 * 注意: 调用完此函数后一定需要调用 update_end 函数通知解析器解析完毕
	 */
	bool update(const char* data, size_t len);

	/**
	 * 在采用流式解析结束后必须调用此函数
	 */
	void update_end(void);

	/**
	 * 调用此函数解析磁盘上的一封邮件
	 * @param file_path {const char*} 邮件文件路径
	 * @return {bool} 如果返回 false 说明源邮件文件无法打开
	 */
	bool parse(const char* file_path);

	/**
	 * 将邮件解析结果另存为另一个文件名
	 * @param out {ostream&} 目标流对象
	 * @return {bool} 是否成功
	 */
	bool save_as(ostream& out);

	/**
	 * 将邮件解析结果另存为另一个文件中
	 * @param file_path {const char*} 目标文件名
	 * @return {bool} 是否成功
	 */
	bool save_as(const char* file_path);

	/**
	 * 邮件解析完毕后，按客户显示的方式将解析结果保存于磁盘，
	 * 用户可以使用浏览器打开该 html 页面
	 * @param path {const char*} 页面保存路径
	 * @param filename {const char*} 目标文件名
	 * @param enableDecode {bool} 转储时是否自动进行解码
	 * @param toCharset {const char*} 目标字符集
	 * @param off {off_t} 调用者希望给邮件结点附加的相对偏移量
	 * @return {bool} 是否成功
	 */
	bool save_mail(const char* path, const char* filename,
		bool enableDecode = true, const char* toCharset = "gb2312",
		off_t off = 0);

	/**
	 * 获得邮件正文节点
	 * @param htmlFirst {bool} 优先获得HTML格式的文本；否则优先获得
	 *  纯文本，且如果只有HTML文本则转换为纯文本
	 * @param enableDecode {bool} 转储时是否对原文进行解码
	 * @param toCharset {const char*} 目标字符集
	 * @param off {off_t} 调用者希望给邮件体结点附加的相对偏移量
	 * @return {mime_body*} 若未找到正文内容则返回 NULL
	 */
	mime_body* get_body_node(bool htmlFirst, bool enableDecode = true,
                const char* toCharset = "gb2312", off_t off = 0);

	/**
	 * 获得 text/plain 格式的正文节点
	 * @param enableDecode {bool} 转储时是否对原文进行解码
	 * @param toCharset {const char*} 目标字符集
	 * @param off {off_t} 调用者希望给邮件体结点附加的相对偏移量
	 * @return {mime_body*} 若未找到 plain 格式的正文内容则返回 NULL
	 */
	mime_body* get_plain_body(bool enableDecode = true,
		const char* toCharset = "gb2312", off_t off = 0);

	/**
	 * 获得 text/html 格式的正文节点
	 * @param enableDecode {bool} 转储时是否对原文进行解码
	 * @param toCharset {const char*} 目标字符集
	 * @param off {off_t} 调用者希望给邮件体结点附加的相对偏移量
	 * @return {mime_body*} 若未找到 html 格式的正文内容则返回 NULL
	 */
	mime_body* get_html_body(bool enableDecode = true,
		const char* toCharset = "gb2312", off_t off = 0);

	/**
	 * 获得所有的 mime 节点列表
	 * @param enableDecode {bool} 转储时是否自动进行解码
	 * @param toCharset {const char*} 目标字符集
	 * @param off {off_t} 调用者希望给邮件结点附加的相对偏移量
	 * @return {const std::list<mime_node*>&}
	 */
	const std::list<mime_node*>& get_mime_nodes(bool enableDecode = true,
		const char* toCharset = "gb2312", off_t off = 0);

	/**
	 * 获得附件列表
	 * @param enableDecode {bool} 转储时是否自动进行解码
	 * @param toCharset {const char*} 目标字符集
	 * @param off {off_t} 调用者希望给邮件结点附加的相对偏移量
	 * @param all {bool} 提取所有包括 message/application/image 在内的所有节点
	 * @return {const std::list<mime_attach*>&}
	 */
	const std::list<mime_attach*>& get_attachments(bool enableDecode = true,
		const char* toCharset = "gb2312", off_t off = 0, bool all = true);

	/**
	 * 获得图片列表
	 * @param enableDecode {bool} 转储时是否自动进行解码
	 * @param toCharset {const char*} 目标字符集
	 * @param off {off_t} 调用者希望给邮件结点附加的相对偏移量
	 * @return {const std::list<mime_image*>&}
	 */
	const std::list<mime_image*>& get_images(bool enableDecode = true,
		const char* toCharset = "gb2312", off_t off = 0);
	mime_image* get_image(const char* cld, bool enableDecode = true,
		const char* toCharset = "gb2312", off_t off = 0);

	/**
	 * 调试MIME解析结果
	 * @param save_path {const char*} 存储 MIME 解析结果的路径
	 * @param decode {bool} 是否对原文进行解码
	 */
	void mime_debug(const char* save_path, bool decode = true);

	/////////////////////////////////////////////////////////////////////
        // 和邮件头相关的函数

	/**
	 * 设置发件人
	 * @param addr {const char*} 邮件地址
	 * @return {mime&}
	 */
	mime& set_sender(const char* addr)
	{
		m_primaryHeader.set_returnpath(addr);
		return *this;
	}

	/**
	 * 设置发件人: From: zhengshuxin@51iker.com
	 * @param addr {const char*} 邮件地址
	 * @return {mime&}
	 */
	mime& set_from(const char* addr)
	{
		m_primaryHeader.set_from(addr);
		return *this;
	}

	/**
	 * 设置邮件回地址: Reply-To: zhengshuxin@51iker.com
	 * @param addr {const char*} 邮件地址
	 * @return {mime&}
	 */
	mime& set_replyto(const char* addr)
	{
		m_primaryHeader.set_replyto(addr);
		return *this;
	}

	/**
	 * 设置邮件反复地址 Return-Path: <zhengshuxin@51iker.com>
	 * @param addr {const char*} 邮件地址
	 * @return {mime&}
	 */
	mime& set_returnpath(const char* addr)
	{
		m_primaryHeader.set_returnpath(addr);
		return *this;
	}

	/**
	 * 设置邮件主题: Subject: test
	 * @param s {const char*} 邮件主题
	 * @return {mime&}
	 */
	mime& set_subject(const char* s)
	{
		m_primaryHeader.set_subject(s);
		return *this;
	}

	/**
	 * 添加邮件接收人: To: <zhengshuxin@51iker.com>
	 * @param addr {const char*} 邮件地址
	 * @return {mime&}
	 */
	mime& add_to(const char* addr)
	{
		m_primaryHeader.add_to(addr);
		return *this;
	}

	/**
	 * 添加邮件抄送者: CC: <zhengshuxin@51iker.com>
	 * @param addr {const char* addr} 邮件地址
	 * @return {mime&}
	 */
	mime& add_cc(const char* addr)
	{
		m_primaryHeader.add_cc(addr);
		return *this;
	}

	/**
	 * 添加邮件密送者: BCC: <zhengshuxin@51iker.com>
	 * @param addr {const char* addr} 邮件地址
	 * @return {mime&}
	 */
	mime& add_bcc(const char* addr)
	{
		m_primaryHeader.add_bcc(addr);
		return *this;
	}

	/**
	 * 添加邮件接收者: CC: <zhengshuxin@51iker.com>
	 * @param addr {const char* addr} 邮件地址
	 * @return {mime&}
	 */
	mime& add_rcpt(const char* addr)
	{
		m_primaryHeader.add_rcpt(addr);
		return *this;
	}

	/**
	 * 添加邮件头的字段
	 * @param name {const char*} 字段名
	 * @param value {const char*} 字段值
	 * @return {mime&}
	 */
	mime& add_header(const char* name, const char* value)
	{
		m_primaryHeader.add_header(name, value);
		return *this;
	}

	/**
	 * 设置邮件头的内容类型: Content-Type: text/plain
	 * @param ctype {const char*} 主类型
	 * @param stype {const char*} 子类型
	 * @return {mime&}
	 */
	mime& set_type(const char* ctype, const char* stype)
	{
		m_primaryHeader.set_type(ctype, stype);
		return *this;
	}

	/**
	 * 设置邮件头部的分隔符
	 * @param s {const char*} 分隔串
	 * @return {mime&}
	 */
	mime& set_boundary(const char* s)
	{
		m_primaryHeader.set_boundary(s);
		return *this;
	}

	/**
	 * 获得发件人
	 * @return {const string&} 如果返回对象的内容为空
	 *  (调用 string::empty()) 则表示没有此字段
	 */
	const string& sender(void) const
	{
		return m_primaryHeader.sender();
	}

	/**
	 * 获得发件人
	 * @return {const string&} 如果返回对象的内容为空
	 *  (调用 string::empty()) 则表示没有此字段
	 */
	const string& from(void) const
	{
		return m_primaryHeader.from();
	}

	/**
	 * 获得回复邮件地址
	 * @return {const string&} 如果返回对象的内容为空
	 *  (调用 string::empty()) 则表示没有此字段
	 */
	const string& replyto(void) const
	{
		return m_primaryHeader.replyto();
	}

	/**
	 * 获得回复邮件地址
	 * @return {const string&} 如果返回对象的内容为空
	 *  (调用 string::empty()) 则表示没有此字段
	 */
	const string& returnpath(void) const
	{
		return m_primaryHeader.returnpath();
	}

	/**
	 * 获得邮件主题
	 * @return {const string&} 如果返回对象的内容为空
	 *  (调用 string::empty()) 则表示没有此字段
	 */
	const string& subject(void) const
	{
		return m_primaryHeader.subject();
	}

	/**
	 * 获得收件人列表: To: xxx@xxx.com
	 * @return {const std::list<char*>&) 如果返回对象的内容为空
	 *  (调用 std::list<char*>::empty()) 则表示没有此字段
	 */
	const std::list<char*>& to_list(void) const
	{
		return m_primaryHeader.to_list();
	}

	/**
	 * 获得抄送人列表: To: xxx@xxx.com
	 * @return {const std::list<char*>&) 如果返回对象的内容为空
	 *  (调用 std::list<char*>::empty()) 则表示没有此字段
	 */
	const std::list<char*>& cc_list(void) const
	{
		return m_primaryHeader.cc_list();
	}

	/**
	 * 获得暗送人列表: To: xxx@xxx.com
	 * @return {const std::list<char*>&) 如果返回对象的内容为空
	 *  (调用 std::list<char*>::empty()) 则表示没有此字段
	 */
	const std::list<char*>& bcc_list(void) const
	{
		return m_primaryHeader.bcc_list();
	}

	/**
	 * 获得收件人列表:
	 * To: xxx@xxx.xxx, CC: xxx@xxx.xxx, BCC: xxx@xxx.xxx
	 * @return {const std::list<char*>&) 如果返回对象的内容为空
	 *  (调用 std::list<char*>::empty()) 则表示没有此字段
	 */
	const std::list<char*>& rcpt_list(void) const
	{
		return m_primaryHeader.rcpt_list();
	}

	/**
	 * 获得邮件头的各个字段列表
	 * @return {const std::list<HEADER*>&)
	 */
	const std::list<HEADER*>& header_list(void) const
	{
		return m_primaryHeader.header_list();
	}	

	/**
	 * 查询邮件头对应字段名的字段值
	 * @param name {const char*} 字段名
	 * @return {const char*} 字段值, 为空时表示不存在
	 */
	const char* header_value(const char* name) const
	{
		return m_primaryHeader.header_value(name);
	}

	/**
	 * 查询邮件头对应字段名的字段值集合
	 * @param name {const char*} 字段名
	 * @param values {std::list<const char*>*} 存储对应的结果集
	 * @return {int} 字段值集合的个数
	 */
	int header_values(const char* name, std::list<const char*>* values) const
	{
		return m_primaryHeader.header_values(name, values);
	}

	/**
	 * 获得邮件头中关于 Content-Type: text/html 中的 text 字段
	 * @return {const char*} 永远返回非空值
	 */
	const char* get_ctype(void) const
	{
		return m_primaryHeader.get_ctype();
	}

	/**
	 * 获得邮件头中关于 Content-Type: text/html 中的 html 字段
	 * @return {const char*} 永远返回非空值
	 */
	const char* get_stype(void) const
	{
		return m_primaryHeader.get_stype();
	}

	/**
	 * 获得邮件头
	 * @return {const mime_head&}
	 */
	const mime_head& primary_header(void) const
	{
		return m_primaryHeader;
	}

private:
	mime_head m_primaryHeader;

	MIME_STATE* m_pMimeState;
	bool m_bPrimaryHeadFinish;
	char* m_pFilePath;
	mime_body* m_pBody;
	std::list<mime_node*>* m_pNodes;
	std::list<mime_attach*>* m_pAttaches;
	std::list<mime_image*>* m_pImages;
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
