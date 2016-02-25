#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/mime/mime_define.hpp"
#include "acl_cpp/mime/mime_base64.hpp"
#include "acl_cpp/mime/mime_uucode.hpp"
#include "acl_cpp/mime/mime_xxcode.hpp"
#include "acl_cpp/mime/mime_quoted_printable.hpp"
#include "acl_cpp/mime/mime_code.hpp"
#endif

namespace acl {

#define INVALID		0xff
#ifndef UCHAR_MAX
#define UCHAR_MAX 0xff
#endif

mime_code::mime_code(bool addCrlf, bool addInvalid, const char* encoding_type)
: m_addCrLf(addCrlf)
, m_addInvalid(addInvalid)
, m_encoding(false)
, m_toTab(NULL)
, m_unTab(NULL)
, m_fillChar('=')
, m_pBuf(NULL)
{
	if (encoding_type)
		encoding_type_ = acl_mystrdup(encoding_type);
	else
		encoding_type_ = acl_mystrdup("unknown");
	reset();
}

mime_code::~mime_code()
{
	acl_myfree(encoding_type_);
	delete m_pBuf;
}

void mime_code::init(const unsigned char* toTab,
	const unsigned char* unTab, unsigned char fillChar)
{
	m_toTab = toTab;
	m_unTab = unTab;
	m_fillChar = fillChar;
}

void mime_code::reset()
{
	m_encodeCnt = 0;
	m_decodeCnt = 0;
}

void mime_code::add_crlf(bool on)
{
	m_addCrLf = on;
}

void mime_code::add_invalid(bool on)
{
	m_addInvalid = on;
}

void mime_code::encode_update(const char *src, int n, acl::string* out)
{
	int  i = 0;

	while (n > 0) {
		if (m_encodeCnt == (int) sizeof(m_encodeBuf)) {
			encode(out);
			m_encodeCnt = 0;
		}
		i = n;
		if (i > (int) sizeof(m_encodeBuf) - m_encodeCnt)
			i = (int) sizeof(m_encodeBuf) - m_encodeCnt;
		memcpy(m_encodeBuf + m_encodeCnt, src, i);
		m_encodeCnt += i;
		src += i;
		n -= i;
	}
}

void mime_code::encode_finish(acl::string* out)
{
	encode(out);
	m_encodeCnt = 0;
}

void mime_code::encode(acl::string *out)
{
	const unsigned char *cp;
	int     count;

	/*
	* Encode 3 -> 4.
	*/
	for (cp = (const unsigned char *) m_encodeBuf, count = m_encodeCnt;
		count > 0; count -= 3, cp += 3)
	{
		out->push_back((char) m_toTab[cp[0] >> 2]);
		if (count > 1) {
			out->push_back((char) m_toTab[(cp[0] & 0x3) << 4 | cp[1] >> 4]);
			if (count > 2) {
				out->push_back((char) m_toTab[(cp[1] & 0xf) << 2 | cp[2] >> 6]);
				out->push_back((char) m_toTab[cp[2] & 0x3f]);
			} else {
				out->push_back((char) m_toTab[(cp[1] & 0xf) << 2]);
				out->push_back(m_fillChar);
				break;
			}
		} else {
			out->push_back((char) m_toTab[(cp[0] & 0x3) << 4]);
			out->push_back(m_fillChar);
			out->push_back(m_fillChar);
			break;
		}
	}

	if (m_addCrLf) {
		out->push_back('\r');
		out->push_back('\n');
	}
}

void mime_code::decode_update(const char *src, int n, acl::string* out)
{
	int  i = 0;

	while (n > 0) {
		if (m_decodeCnt == (int) sizeof(m_decodeBuf)) {
			decode(out);
		}
		i = n;
		if (i > (int) sizeof(m_decodeBuf) - m_decodeCnt)
			i = (int) sizeof(m_decodeBuf) - m_decodeCnt;
		memcpy(m_decodeBuf + m_decodeCnt, src, i);
		src += i;
		n -= i;
		m_decodeCnt += i;
	}
}

void mime_code::decode_finish(acl::string* out)
{
	decode(out);

	/* 如果缓冲区内还有数据, 则因其不够4个字节而照原样拷贝 */

	if (m_addInvalid)
	{
		if (m_decodeCnt == 1) {
			out->push_back(m_decodeBuf[0]);
		} else if (m_decodeCnt == 2) {
			out->push_back(m_decodeBuf[0]);
			out->push_back(m_decodeBuf[1]);
		} else if (m_decodeCnt == 3) {
			out->push_back(m_decodeBuf[0]);
			out->push_back(m_decodeBuf[1]);
			out->push_back(m_decodeBuf[2]);
		}
	}
	m_decodeCnt = 0;
}

void mime_code::decode(acl::string* out)
{
	const unsigned char *cp;
	int     ch0, ch1, ch2, ch3;

	if (m_decodeCnt <= 0)
		return;

	/* 必须缓冲到 4 个字节才开始解析 */

	for (cp = (const unsigned char *) m_decodeBuf; m_decodeCnt >= 4;) {

		/* 跳过所有的回车换行符及补齐字节 '=' */

		if (*cp == '\r' || *cp == '\n' || *cp == m_fillChar) {
			cp++;
			m_decodeCnt--;
			continue;
		}

		/* 第一个字节 */

		if ((ch0 = m_unTab[*cp])== INVALID) {
			/* 如果非法, 则拷贝原字符 */
			if(m_addInvalid)
				out->push_back(*cp);
			cp++;
			m_decodeCnt--;
			continue;
		}
		cp++;
		m_decodeCnt--;

		/* 第二个字节 */

		if ((ch1 = m_unTab[*cp])== INVALID) {
			/* 如果非法, 则拷贝原字符 */
			if (m_addInvalid)
				out->push_back((char) (*cp));
			cp++;
			m_decodeCnt--;
			continue;
		}
		cp++;
		m_decodeCnt--;

		out->push_back((char) (ch0 << 2 | ch1 >> 4));

		/* 第三个字节 */

		ch2 = *cp;
		if (ch2 == m_fillChar) {
			cp++;
			m_decodeCnt--;
			continue;
		}

		if ((ch2 = m_unTab[ch2]) == INVALID) {
			/* 如果非法, 则拷贝原字符 */
			if (m_addInvalid)
				out->push_back((char) (*cp));
			cp++;
			m_decodeCnt--;
			continue;
		}
		cp++;
		m_decodeCnt--;

		out->push_back((char) (ch1 << 4 | ch2 >> 2));

		/* 第四个字节 */

		ch3 = *cp;
		if (ch3 == m_fillChar) {
			/* 第三个字节非 '=' 而第四个字节为 '=',
			* 则说明第四个字节为补齐用的
			*/
			cp++;
			m_decodeCnt--;
			continue;
		}
		if ((ch3 = m_unTab[ch3]) == INVALID) {
			/* 如果非法, 则拷贝原字符 */
			if (m_addInvalid)
				out->push_back((char) (*cp));
			cp++;
			m_decodeCnt--;
			continue;
		}
		cp++;
		m_decodeCnt--;

		out->push_back((char) (ch2 << 6 | ch3));
	}

	if (m_decodeCnt == 1) {
		m_decodeBuf[0] = *cp;
	} else if (m_decodeCnt == 2) {
		m_decodeBuf[0] = *cp;
		m_decodeBuf[1] = *(cp + 1);
	} else if (m_decodeCnt == 3) {
		m_decodeBuf[0] = *cp;
		m_decodeBuf[1] = *(cp + 1);
		m_decodeBuf[2] = *(cp + 2);
	}
}

void mime_code::create_decode_tab(const unsigned char *toTab,
	acl::string *out)
{
	unsigned char tab[255];
	char  buf[32];
	unsigned char *cp;
	int i, n = (int) strlen((const char*) toTab);

	memset(tab, 0xff, sizeof(tab));

	for (i = 0; i < n; i++) {
		tab[toTab[i]] = (unsigned char) i;
	}

	out->clear();
	for (i = 0, cp = tab; cp < tab + sizeof(tab); cp++) {
		if (i++ % 16 == 0) {
			out->append("\r\n");
		}
		safe_snprintf(buf, sizeof(buf), "%d, ", *cp);
		out->append((char*) buf);
	}
}

void mime_code::set_status(bool encoding)
{
	m_encoding = encoding;
}

int mime_code::push_pop(const char* in, size_t len,
	string* out, size_t max /* = 0 */)
{
	if (m_pBuf == NULL)
		m_pBuf = NEW acl::string(1024);

	if (in && len > 0)
	{
		if (m_encoding)
			encode_update(in, (int) len, m_pBuf);
		else
			decode_update(in, (int) len, m_pBuf);
	}

	if (out == NULL)
		return (0);

	len = m_pBuf->length();
	if (len == 0)
		return (0);

	size_t n;
	if (max > 0)
		n = max > len ? len : max;
	else
		n = len;

	out->append(m_pBuf->c_str(), n);

	if (len > n)
		m_pBuf->memmove(m_pBuf->c_str() + n, len - n);
	else
		m_pBuf->clear();

	return (int) (n);
}

int mime_code::pop_end(acl::string* out, size_t max /* = 0 */)
{
	if (m_pBuf == NULL)
	{
		logger_error("call push_pop first");
		return (-1);
	}
	if (m_encoding)
		encode_finish(m_pBuf);
	else
		decode_finish(m_pBuf);

	if (out == NULL)
	{
		m_pBuf->clear();
		return (0);
	}

	size_t n = m_pBuf->length();
	if (n == 0)
		return (0);
	if (max > 0 && n > max)
		n = max;
	out->append(m_pBuf->c_str(), n);
	m_pBuf->clear();
	return (int) (n);
}

void mime_code::clear()
{
	if (m_pBuf)
		m_pBuf->clear();
}

mime_code* mime_code::create(int encoding, bool warn_unsupport /* = true */)
{
	if(encoding == MIME_ENC_BASE64)
		return (NEW acl::mime_base64());
	else if (encoding == MIME_ENC_UUCODE)
		return (NEW acl::mime_uucode());
	else if (encoding == MIME_ENC_XXCODE)
		return (NEW acl::mime_xxcode());
	else if (encoding == MIME_ENC_QP)
		return (NEW acl::mime_quoted_printable());
	else
	{
		if (warn_unsupport)
			logger_warn("unknown encoding(%d)", encoding);
		return (NULL);
	}
}

} // namespace acl
