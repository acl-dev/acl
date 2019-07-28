#pragma once
#include "../acl_cpp_define.hpp"
#include "mime_code.hpp"

#if !defined(ACL_MIME_DISABLE)

namespace acl {

class string;

class ACL_CPP_API mime_quoted_printable : public mime_code
{
public:
	/**
	 * 构造函数
	 * @param addCrlf {bool} 非流式编码时是否在末尾添加 "\r\n"
	 * @param addInvalid {bool} 流式解码时是否遇到非法字符是否原样拷贝
	 */
	mime_quoted_printable(bool addCrlf = false, bool addInvalid = false);
	~mime_quoted_printable(void);

	// 基类的虚函数重载

	/* 流式编码函数，使用方法： encode_update->encode_update->...->encode_finish */

	/**
	 * 流式编码函数
	 * @param src {const char*} 原始数据
	 * @param n {int} src 数据长度
	 * @param out {string*} 存储编码结果，可以通过 out->empty()
	 *  来测试 out 中是否有结果数据，用 out->length() 获得 out 中结果
	 *  数据的长度，注意当用完 out 中的结果数据后一定要调用 out->clear()
	 *  来清空用过的结果数据；也可以多次调用该函数后，在最后调用
	 *  encode_finish 后一次性取出所有数据
	 */
	void encode_update(const char *src, int n, string* out);

	/**
	 * 流式编码结束函数，调用该函数后，取出最后的结果数据
	 * @param out {string*} 存储编码结果，可以通过 out->empty()
	 *  来测试 out 中是否有结果数据，用 out->length() 获得 out 中结果
	 *  数据的长度，注意当用完 out 中的结果数据后一定要调用 out->clear()
	 *  来清空用过的结果数据
	 */
	void encode_finish(string* out);

	/* 流式解码函数，使用方法： decode_update->decode_update->...->decode_finish */

	/**
	 * 流式解码函数
	 * @param src {const char*} 原始数据
	 * @param n {int} src 数据长度
	 * @param out {string*} 存储解码结果，可以通过 out->empty()
	 *  来测试 out 中是否有结果数据，用 out->length() 获得 out 中结果
	 *  数据的长度，注意当用完 out 中的结果数据后一定要调用 out->clear()
	 *  来清空用过的结果数据；也可以多次调用该函数后，在最后调用
	 *  decode_finish 后一次性取出所有数据
	 */
	void decode_update(const char *src, int n, string* out);

	/**
	 * 流式解码结束函数，调用该函数后，取出最后的结果数据
	 * @param out {string*} 存储解码结果，可以通过 out->empty()
	 *  来测试 out 中是否有结果数据，用 out->length() 获得 out 中结果
	 *  数据的长度，注意当用完 out 中的结果数据后一定要调用 out->clear()
	 *  来清空用过的结果数据
	 */
	void decode_finish(string* out);

	/**
	 * 静态编码函数，直接将输入数据进行编码同时存入用户缓冲区
	 * 用户缓冲区
	 * @param in {const char*} 输入数据地址
	 * @param n {int} 输入数据长度
	 * @param out {string*} 存储结果的缓冲区
	 */
	static void encode(const char* in, int n, string* out);

	/**
	 * 静态解码函数，直接将输入数据进行解析并存入用户缓冲区
	 * @param in {const char*} 输入数据地址
	 * @param n {int} 数据长度
	 * @param out {string*} 存储解析结果
	 */
	static void decode(const char* in, int n, string* out);

	/**
	 * 重置编、解码器状态
	 */
	void reset(void);

	/**
	 * 设置在编码结束时是否添加 "\r\n"
	 * @param on {bool}
	 */
	void add_crlf(bool on);

	/**
	 * 设置在解码过程中是否拷贝非法字符
	 * @param on {bool}
	 */
	void add_invalid(bool on);

protected:
private:
	void encode(string* out);
	void decode(string* out);

	bool hex_decode(unsigned char first, unsigned char second,
		unsigned int *result);

	char  m_encodeBuf[72];
	int   m_encodeCnt;
	char  m_decodeBuf[144];
	int   m_decodeCnt;
	bool  m_addCrLf;
	bool  m_addInvalid;
};

}  // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
