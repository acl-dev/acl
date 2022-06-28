#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <list>

#if !defined(ACL_MIME_DISABLE)

namespace acl {

class string;
class mime_code;

struct rfc2047_entry 
{
	string* pData;		// 数据内容
	string* pCharset;	// 字符集
	char  coding;		// 编码格式，B 表示 BASE64, Q 表示 QP
};

class ACL_CPP_API rfc2047 : public noncopyable
{
public:
	/**
	 * 构造函数
	 * @param strip_sp {bool} 在解码过程中是否去掉回车换行符及每行开头的
	 *  空格及TAB
	 * @param addCrlf {bool} 在编码过程中当数据比较长时是否自动添加 "\r\n"
	 */
	rfc2047(bool strip_sp = true, bool addCrlf = true);
	~rfc2047(void);

	/**
	 * 流式解析数据, 可以循环调用此函数, 每次添加部分数据
	 * 直至添加完毕
	 * @param in {const char*} 输入源字符串
	 * @param n {int} in 输入串的长度
	 */
	void decode_update(const char* in, int n);

	/**
	 * 将 rfc2047 解析结果转换成指定的字符集字符串, 如果不能
	 * 正确转换则保留源串内容
	 * @param to_charset {const char*} 目标字符集
	 * @param out {string*} 存储转换结果
	 * @param addInvalid {bool} 当为 true 时，则转码过程中如果遇到了
	 *  非法字符集，则直接拷贝，否则则跳过，默认情况下是直接拷贝
	 * @return {bool} 转换是否成功
	 */
	bool decode_finish(const char* to_charset, string* out,
		bool addInvalid = true);

	/**
	 * rfc2047 编码过程中添加数据
	 * @param in {const char*} 输入数据
	 * @param n {int} in 数据长度
	 * @param out {string*} 存储编码结果
	 * @param charset {const char*} 输入数据的字符集类型
	 * @param coding {char} 编码类型，支持的编码类型有:
	 *   B: base64, Q: quoted_printable
	 * @return {bool} 检查输入参数是否正确且编码是否成功
	 */
	bool encode_update(const char* in, int n, string* out,
		const char* charset = "gb2312", char coding = 'B');

	/**
	 * 将 encode_update 添加的数据进行编码后存储于用户指定缓冲区
	 * @param  out {string*} 存储编码结果的用户缓冲区
	 * @return {bool} 是否成功
	 */
	bool encode_finish(string* out);

	/**
	 * 静态编码器
	 * @param in {const char*} 输入数据地址
	 * @param n {int} 数据长度
	 * @param out {string*} 存储编码结果的缓冲区
	 * @param charset {const char*} 输入数据的字符集
	 * @param coding {char} 编码类型，支持的编码类型有:
	 *   B: base64, Q: quoted_printable
	 * @param addCrlf {bool} 在编码过程中当数据比较长时是否自动添加 "\r\n"
	 * @return {bool} 编码是否成功
	 */
	static bool encode(const char* in, int n, string* out,
		const char* charset = "gb2312", char coding = 'B',
		bool addCrlf = true);

	/**
	 * 静态解码器
	 * @param in {const char*} 输入数据地址
	 * @param n {int} 数据长度
	 * @param out {string*} 存储解码结果的缓冲区
	 * @param to_charset {const char*} 目标字符集
	 * @param strip_sp {bool} 是否去掉回车换行符及每行开头的空格及TAB
	 * @param addInvalid {bool} 当为 true 时，则转码过程中如果遇到了
	 *  非法字符集，则直接拷贝，否则则跳过，默认情况下是直接拷贝
	 * @return {bool} 解码是否成功
	 */
	static bool decode(const char* in, int n, string* out,
		const char* to_charset = "gb2312", bool strip_sp = false,
		bool addInvalid = true);

	/**
	 * 将解析结果以链表的形式给出
	 * @return {const std::list<rfc2047_entry*>&}
	 */
	const std::list<rfc2047_entry*>& get_list(void) const;

	/**
	 * 重置解析器状态后, 该解析器可再次使用
	 * @param strip_sp {bool} 是否去掉回车换行符及每行开头的空格及TAB
	 */
	void reset(bool strip_sp = true);

	/**
	 * 调试输出解析结果
	 */
	void debug_rfc2047(void) const;

private:
	std::list<rfc2047_entry*> m_List;
	rfc2047_entry* m_pCurrentEntry;
	mime_code* m_coder;
	int   m_status;
	bool  m_stripSp;
	bool  m_addCrlf;
	char  m_lastCh;

public:
	// 以下函数仅内部使用

	int status_next(const char* s, int n);
	int status_data(const char* s, int n);
	int status_charset(const char* s, int n);
	int status_coding(const char* s, int n);
	int status_equal_question(const char* s, int n);
	int status_question_first(const char* s, int n);
	int status_question_second(const char* s, int n);
	int status_question_equal(const char* s, int n);
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
