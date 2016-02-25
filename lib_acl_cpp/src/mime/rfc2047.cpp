#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mime/mime_base64.hpp"
#include "acl_cpp/mime/mime_quoted_printable.hpp"
#include "acl_cpp/stdlib/charset_conv.hpp"
#include "acl_cpp/mime/rfc2047.hpp"
#endif

#define SCOPY(x, y) ACL_SAFE_STRNCPY((x), (y), sizeof(x))

namespace acl {

enum
{
	rfc2047_status_next,
	rfc2047_status_data,
	rfc2047_status_equal_question,
	rfc2047_status_charset,
	rfc2047_status_question_first,
	rfc2047_status_coding,
	rfc2047_status_question_second,
	rfc2047_status_question_equal
};

rfc2047::rfc2047(bool strip_sp /* = true */, bool addCrlf /* = true */)
	: m_pCurrentEntry(NULL)
	, m_coder(NULL)
	, m_status(rfc2047_status_next)
	, m_stripSp(strip_sp)
	, m_addCrlf(addCrlf)
	, m_lastCh(0)
{

}

rfc2047::~rfc2047()
{
	reset();
	delete m_coder;
}

void rfc2047::reset(bool strip_sp /* = true */)
{
	std::list<rfc2047_entry*>::iterator it, lt;
	
	it = lt = m_List.begin();
	for (; it != m_List.end(); it = lt)
	{
		lt++;
		delete (*it)->pData;
		delete (*it)->pCharset;
		delete (*it);
		m_List.pop_front();
	}
	m_pCurrentEntry = NULL;
	m_status = rfc2047_status_next;
	m_lastCh = 0;
	m_stripSp = strip_sp;
	delete m_coder;
	m_coder = NULL;
}

const std::list<rfc2047_entry*>& rfc2047::get_list() const
{
	return m_List;
}

void rfc2047::debug_rfc2047() const
{
	std::list<rfc2047_entry*>::const_iterator cit = m_List.begin();
	for (; cit != m_List.end(); ++cit)
	{
		printf(">>> debug_rfc2047: charset: %s, code: %c, data: %s\n",
			(*cit)->pCharset->c_str(),
			(*cit)->coding,
			(*cit)->pData->c_str());
	}
}

#define SKIP(s, n) \
{ \
	if (n > 0 && *s == '\r') \
	{ \
		m_lastCh = *s++; \
		n--; \
	} \
	if (n > 0 && *s == '\n') \
	{ \
		m_lastCh = *s++; \
		n--; \
	} \
	if (m_lastCh == 0 || m_lastCh == '\n') \
	{ \
		while (n > 0 && ((m_lastCh = *s) == ' ' || m_lastCh == '\t')) \
		{ \
			s++; \
			n--; \
		} \
	} \
}

int rfc2047::status_next(const char* s, int n)
{
	if (m_stripSp)
		SKIP(s, n);
	if (n <= 0)
		return n;

	rfc2047_entry* entry = NEW rfc2047_entry;
	m_pCurrentEntry = entry;

	entry->pData = NEW acl::string(128);
	entry->pCharset = NEW acl::string(32);
	entry->coding = 0;

	m_List.push_back(entry);
	if (*s == '=')
	{
		m_status = rfc2047_status_equal_question;
		return n - 1;
	}
	m_status = rfc2047_status_data;
	*m_pCurrentEntry->pData << *s;
	return n - 1;
}

int rfc2047::status_data(const char* s, int n)
{
	if (m_stripSp)
		SKIP(s, n);
	if (n <= 0)
		return n;
	while (n > 0)
	{
		if (m_stripSp)
			SKIP(s, n);
		if (n <= 0)
			break;

		if (m_pCurrentEntry->coding > 0)
		{
			if (*s == '?')
			{
				m_status = rfc2047_status_question_equal;
				n--;
				break;
			}
			*m_pCurrentEntry->pData << *s;
			s++;
			n--;
			continue;
		}

		if (*s == '=')
		{
			m_status = rfc2047_status_next;
			break;
		}
		else
		{
			*m_pCurrentEntry->pData << *s;
			s++;
			n--;
		}
	}
	return n;
}

int rfc2047::status_equal_question(const char* s, int n)
{
	if (m_stripSp)
		SKIP(s, n);
	if (n <= 0)
		return n;
	if (*s == '?')
	{
		m_status = rfc2047_status_charset;
		return n - 1;
	}
	m_status = rfc2047_status_data;
	*m_pCurrentEntry->pData << '=';
	*m_pCurrentEntry->pData << *s;
	return n - 1;
}

int rfc2047::status_charset(const char* s, int n)
{
	if (m_stripSp)
		SKIP(s, n);
	if (n <= 0)
		return n;
	if (*s == '?')
	{
		if (m_pCurrentEntry->pCharset->length() == 0)
		{
			*m_pCurrentEntry->pData = "=??";
			m_status = rfc2047_status_data;
			return n - 1;
		}
		m_status = rfc2047_status_question_first;
		return n - 1;
	}
	*m_pCurrentEntry->pCharset << *s;
	return n - 1;
}

int rfc2047::status_question_first(const char* s, int n)
{
	if (m_stripSp)
		SKIP(s, n);
	if (n <= 0)
		return n;
	m_status = rfc2047_status_coding;
	return n;
}

int rfc2047::status_coding(const char* s, int n)
{
	if (m_stripSp)
		SKIP(s, n);
	if (n <= 0)
		return n;
	if (*s == 'B' || *s == 'b')
	{
		m_pCurrentEntry->coding = 'B';
		m_status = rfc2047_status_question_second;
		return n - 1;
	}
	else if (*s == 'Q' || *s == 'q')
	{
		m_pCurrentEntry->coding = 'Q';
		m_status = rfc2047_status_question_second;
		return n - 1;
	}
	
	*m_pCurrentEntry->pData << "=?"
		<< m_pCurrentEntry->pCharset->c_str()
		<< "?";
	m_pCurrentEntry->pCharset->clear();
	m_pCurrentEntry->coding = 0;
	return n - 1;
}

int rfc2047::status_question_second(const char* s, int n)
{
	if (m_stripSp)
		SKIP(s, n);
	if (n <= 0)
		return n;
	if (*s == '?')
	{
		m_status = rfc2047_status_data;
		return n - 1;
	}

	*m_pCurrentEntry->pData << "=?"
		<< m_pCurrentEntry->pCharset->c_str();
	m_pCurrentEntry->pCharset->clear();
	*m_pCurrentEntry->pData << *s;
	m_pCurrentEntry->coding = 0;
	m_status = rfc2047_status_data;
	return n - 1;
}

int rfc2047::status_question_equal(const char* s, int n)
{
	if (m_stripSp)
		SKIP(s, n);
	if (n <= 0)
		return n;
	if (*s == '=')
	{
		m_status = rfc2047_status_next;
		m_pCurrentEntry = NULL;
		return n - 1;
	}

	size_t size = m_pCurrentEntry->pCharset->length()
		+ m_pCurrentEntry->pData->length()
		+ strlen("=???") + 1;
	acl::string* pBuf = NEW acl::string(size);

	*pBuf << "=?" << m_pCurrentEntry->pCharset->c_str()
		<< "?" << m_pCurrentEntry->coding << "?"
		<< m_pCurrentEntry->pData->c_str() << "?"
		<< *s;
	delete m_pCurrentEntry->pData;
	m_pCurrentEntry->pData = pBuf;
	m_status = rfc2047_status_data;
	return n - 1;
}

struct rfc2047_status_matchine 
{
	int   status;
	int   (rfc2047::*func)(const char*, int);
};

static rfc2047_status_matchine statusTab[] =
{
	{ rfc2047_status_next, &rfc2047::status_next },
	{ rfc2047_status_data, &rfc2047::status_data },
	{ rfc2047_status_equal_question, &rfc2047::status_equal_question },
	{ rfc2047_status_charset, &rfc2047::status_charset },
	{ rfc2047_status_question_first, &rfc2047::status_question_first },
	{ rfc2047_status_coding, &rfc2047::status_coding },
	{ rfc2047_status_question_second, &rfc2047::status_question_second },
	{ rfc2047_status_question_equal, &rfc2047::status_question_equal }
};

void rfc2047::decode_update(const char* in, int n)
{
	while (n > 0)
	{
		int ret = (this->*(statusTab[m_status].func))(in, n);
		in += n - ret;
		n = ret;
	}
}

#define EQ(x, y) (((x) == NULL && (y) == NULL)  \
	  || ((x) != NULL && (y) != NULL && !strcasecmp((x), (y))))

static bool decoder_update(rfc2047_entry* entry,
	const char* fromCharset, const char* toCharset,
	acl::mime_code* pDecoder, acl::charset_conv* pConv,
	acl::string* out, acl::string* buf1, acl::string* buf2)
{
	buf1->clear();
	pDecoder->decode_update(entry->pData->c_str(),
			(int) entry->pData->length(), buf1);
	if (buf1->empty())
		return true;

	// 如果源字符集与目标字符集相同则不进行字符集转码

	if (EQ(fromCharset, toCharset) || pConv == NULL)
	{
		out->append(buf1->c_str(), buf1->length());
		return true;
	}

	// 进行字符集转码

	buf2->clear();

	if (!pConv->update_begin(fromCharset, toCharset))
		out->append(buf1->c_str(), buf1->length());
	else if (!pConv->update(buf1->c_str(), buf1->length(), buf2))
		out->append(buf1->c_str(), buf1->length());
	else if (buf2->length() > 0)
		out->append(buf2->c_str(), buf2->length());

	return true;
}

static bool decoder_finish(acl::mime_code* pDecoder, acl::charset_conv* pConv,
	acl::string* out, acl::string* buf1, acl::string* buf2)
{
	buf1->clear();
	pDecoder->decode_finish(buf1);
	if (buf1->empty())
	{
		if (pConv)
		{
			buf2->clear();
			pConv->update_finish(buf2);
			if (buf2->length() > 0)
				out->append(buf2->c_str(), buf2->length());
		}
		return true;
	}

	if (pConv == NULL)
	{
		out->append(buf1->c_str(), buf1->length());
		return true;
	}

	buf2->clear();

	if (!pConv->update(buf1->c_str(), buf1->length(), buf2))
		out->append(buf1->c_str(), buf1->length());
	else
	{
		pConv->update_finish(buf2);
		if (buf2->length() > 0)
			out->append(buf2->c_str(), buf2->length());
	}

	return true;
}

bool rfc2047::decode_finish(const char* toCharset,
	string* out, bool addInvalid /* = true */)
{
	std::list<rfc2047_entry*>::const_iterator cit = m_List.begin();
	string buf1;
	string buf2;
	mime_base64 base64;
	mime_quoted_printable qp;

	// 选择一个默认的解码器，然后根据需要变化
	mime_code* pDecoder = &base64;
	const char *fromCharset = NULL;
	charset_conv conv;
	conv.set_add_invalid(addInvalid);

	for (; cit != m_List.end(); ++cit)
	{
		if ((*cit)->coding == 'Q')
		{
			if (pDecoder != &qp
				|| !EQ((*cit)->pCharset->c_str(), fromCharset))
			{
				if (fromCharset == NULL)
					fromCharset = (*cit)->pCharset->c_str();
				if (*fromCharset == 0)
					fromCharset = NULL;
				if (fromCharset == NULL || toCharset == NULL)
					decoder_finish(pDecoder, NULL, out,
						&buf1, &buf2);
				else
				{
					conv.update_begin(fromCharset, toCharset);
					decoder_finish(pDecoder, &conv, out,
						&buf1, &buf2);
				}
				pDecoder->reset();
			}
			pDecoder = &qp;  // qp 解码
			fromCharset = (*cit)->pCharset->c_str();
			if (*fromCharset == 0)
				fromCharset = NULL;
			decoder_update(*cit, fromCharset, toCharset,
				pDecoder, &conv, out, &buf1, &buf2);
		}
		else if ((*cit)->coding == 'B')
		{
			if (pDecoder != &base64
				|| !EQ((*cit)->pCharset->c_str(), fromCharset))
			{
				if (fromCharset == NULL)
					fromCharset = (*cit)->pCharset->c_str();
				if (*fromCharset == 0)
					fromCharset = NULL;
				if (fromCharset == NULL && toCharset == NULL)
					decoder_finish(pDecoder, NULL, out,
						&buf1, &buf2);
				else
				{
					conv.update_begin(fromCharset, toCharset);
					decoder_finish(pDecoder, &conv, out,
						&buf1, &buf2);
				}
				pDecoder->reset();
			}
			pDecoder = &base64;  // base64 解码
			fromCharset = (*cit)->pCharset->c_str();
			if (*fromCharset == 0)
				fromCharset = NULL;
			decoder_update(*cit, fromCharset, toCharset,
				pDecoder, &conv, out, &buf1, &buf2);
		}
		else
		{
			if (fromCharset == NULL || toCharset == NULL)
				decoder_finish(pDecoder, NULL, out,
					&buf1, &buf2);
			else
			{
				conv.update_begin(fromCharset, toCharset);
				decoder_finish(pDecoder, &conv, out,
					&buf1, &buf2);
			}
			pDecoder->reset();
			out->append((*cit)->pData->c_str(),
				(*cit)->pData->length());
		}
	}

	if (fromCharset != NULL && toCharset != NULL)
	{
		conv.update_begin(fromCharset, toCharset);
		return decoder_finish(pDecoder, &conv, out, &buf1, &buf2);
	}
	else
		return decoder_finish(pDecoder, NULL, out, &buf1, &buf2);
}

bool rfc2047::encode_update(const char* in, int n, acl::string* out,
	const char* charset /* = "gb2312" */, char coding /* = 'B' */)
{
	if (charset == NULL || *charset == 0)
		return false;
	char ch = toupper(coding);
	if (ch != 'B' && ch != 'Q')
		return false;

	acl_assert(in);
	acl_assert(n > 0);
	acl_assert(out);

	if (m_pCurrentEntry == NULL
		|| !EQ(m_pCurrentEntry->pCharset->c_str(), charset)
		|| m_pCurrentEntry->coding != ch)
	{
		if (m_coder)
		{
			acl_assert(m_pCurrentEntry);
			m_coder->encode_finish(out);
			*out << "?=";
			delete m_coder;
			m_coder = NULL;
		}

		m_pCurrentEntry = NEW rfc2047_entry;
		m_pCurrentEntry->pData = NEW acl::string(n * 4/3);
		m_pCurrentEntry->pCharset = NEW acl::string(charset);
		m_pCurrentEntry->pCharset->upper();
		m_pCurrentEntry->coding = ch;

		m_List.push_back(m_pCurrentEntry);

		if (ch == 'B')
		{
			m_coder = NEW mime_base64(m_addCrlf, false);
			*out << "=?" << m_pCurrentEntry->pCharset->c_str()
				<< "?B?";
		}
		else if (ch == 'Q')
		{
			m_coder = NEW mime_quoted_printable(m_addCrlf, false);
			*out << "=?" << m_pCurrentEntry->pCharset->c_str()
				<< "?Q?";
		}
	}

	acl_assert(m_pCurrentEntry);
	acl_assert(m_coder);

	m_coder->encode_update(in, n, out);
	return true;
}

bool rfc2047::encode_finish(string* out)
{
	acl_assert(m_pCurrentEntry);
	acl_assert(m_coder);
	acl_assert(out);

	m_coder->encode_finish(out);
	*out << "?=";
	return true;
}

bool rfc2047::encode(const char* in, int n, acl::string* out,
	const char* charset /* = "gb2312" */, char coding /* = 'B' */,
	bool addCrlf /* = true */)
{
	rfc2047 rfc(false, addCrlf);
	if (rfc.encode_update(in, n, out, charset, coding) == false)
		return false;
	return rfc.encode_finish(out);
}

bool rfc2047::decode(const char* in, int n, acl::string* out,
	const char* to_charset /* = "gb2312" */,
	bool strip_sp /* = false */, bool addInvalid /* = true */)
{
	rfc2047 rfc(strip_sp, false);
	rfc.decode_update(in, n);
	return rfc.decode_finish(to_charset, out, addInvalid);
}

} // namespace acl
