#include "acl_stdafx.hpp"
#include "internal/mime_state.hpp"
#include "internal/header_opts.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/xml.hpp"
#include "acl_cpp/stream/ifstream.hpp"
#include "acl_cpp/stream/ofstream.hpp"
#include "acl_cpp/stdlib/pipe_stream.hpp"
#include "acl_cpp/stdlib/charset_conv.hpp"
#include "acl_cpp/mime/mime_attach.hpp"
#include "acl_cpp/mime/mime_image.hpp"
#include "acl_cpp/mime/mime_body.hpp"
#include "acl_cpp/mime/mime_base64.hpp"
#include "acl_cpp/mime/mime_uucode.hpp"
#include "acl_cpp/mime/mime_xxcode.hpp"
#include "acl_cpp/mime/mime_quoted_printable.hpp"
#include "acl_cpp/mime/rfc2047.hpp"
#include "acl_cpp/mime/mime.hpp"
#endif

namespace acl {

mime::mime()
{
	m_pMimeState = mime_state_alloc();
	m_bPrimaryHeadFinish = false;
	m_pFilePath = NULL;
	m_pBody = NULL;
	m_pNodes = NULL;
	m_pAttaches = NULL;
	m_pImages = NULL;
}

mime::~mime()
{
	reset();
	mime_state_free(m_pMimeState);
	delete m_pNodes;
	delete m_pAttaches;
	delete m_pImages;
}

mime& mime::reset(void)
{
	m_primaryHeader.reset();
	mime_state_reset(m_pMimeState);
	m_bPrimaryHeadFinish = false;

	if (m_pFilePath)
	{
		acl_myfree(m_pFilePath);
		m_pFilePath = NULL;
	}

	delete m_pBody;
	m_pBody = NULL;

	if (m_pNodes)
	{
		std::list<mime_node*>::iterator it = m_pNodes->begin();
		for (; it != m_pNodes->end(); ++it)
			delete (*it);
		m_pNodes->clear();
	}

	if (m_pAttaches)
	{
		std::list<mime_attach*>::iterator it = m_pAttaches->begin();
		for (; it != m_pAttaches->end(); ++it)
			delete (*it);
		m_pAttaches->clear();
	}

	if (m_pImages)
	{
		std::list<mime_image*>::iterator it = m_pImages->begin();
		for (; it != m_pImages->end(); ++it)
			delete (*it);
		m_pImages->clear();
	}

	return (*this);
}

bool mime::primary_head_ok() const
{
	if (mime_state_head_finish(m_pMimeState))
		return (true);
	else
		return (false);
}

void mime::update_begin(const char* path)
{
	reset();
	if (m_pFilePath)
		acl_myfree(m_pFilePath);
	if (path && *path)
		m_pFilePath = acl_mystrdup(path);
}

bool mime::update(const char* data, size_t len)
{
	return (mime_state_update(m_pMimeState, data, (int) len) == 1
			? true : false);
}

void mime::update_end()
{
	primary_head_finish();
}

void mime::primary_head_finish()
{
	if (m_bPrimaryHeadFinish == true)
		return;

	MIME_NODE* node = m_pMimeState->root;

	m_primaryHeader.set_type(node->ctype_s, node->stype_s);

	// 针对邮件主头部
	ACL_ITER iter;

	if (node->header_to_list) {
		acl_foreach(iter, node->header_to_list) {
			MAIL_ADDR* mail_addr = (MAIL_ADDR*) iter.data;
			add_to(mail_addr->addr);
		}
	}
	if (node->header_cc_list) {
		acl_foreach(iter, node->header_cc_list) {
			MAIL_ADDR* mail_addr = (MAIL_ADDR*) iter.data;
			add_cc(mail_addr->addr);
		}
	}
	if (node->header_bcc_list) {
		acl_foreach(iter, node->header_bcc_list) {
			MAIL_ADDR* mail_addr = (MAIL_ADDR*) iter.data;
			add_bcc(mail_addr->addr);
		}
	}
	if (node->header_list) {
		acl_foreach(iter, node->header_list) {
			HEADER_NV* header = (HEADER_NV*) iter.data;
			add_header(header->name, header->value);
		}
	}

	if (node->header_sender)
		set_sender(node->header_sender);
	if (node->header_from)
		set_from(node->header_from);
	if (node->header_replyto)
		set_replyto(node->header_replyto);
	if (node->header_returnpath)
		set_returnpath(node->header_returnpath);
	if (node->header_subject)
		set_subject(node->header_subject);

	m_bPrimaryHeadFinish = true;
}

bool mime::parse(const char* path)
{
	acl::ifstream fp;

	if (fp.open_read(path) == false) {
		logger_error("open %s error %s", path, acl_last_serror());
		return (false);
	}

	acl::string buf(1024);
	const char* ptr;
	int    ret;

	while (1) {
#if 0
		if (fp.gets(buf, false) == false)
			break;
		ptr = buf.c_str();
		ret = (int) buf.length();
#elif 1
		ret = fp.read(buf.c_str(), buf.capacity() - 1, false);
		if (ret <= 0)
			break;
		ptr = buf.c_str();
#else
		char ch;
		if (fp.read(ch) == false)
			break;
		buf = (char) ch;
		ptr = buf.c_str();
		ret = (int) buf.length();
#endif

		if (mime_state_update(m_pMimeState, ptr, ret) == 1)
			break;
		buf.clear();
	}

	fp.close();
	primary_head_finish();
	if (m_pFilePath)
		acl_myfree(m_pFilePath);
	m_pFilePath = acl_mystrdup(path);
	return (true);
}

static bool save_as(ifstream& in, fstream& out, off_t pos, ssize_t len)
{
	if (pos < 0 || len <= 0)
		return (true);

	char  buf[8192];

	if (in.fseek(pos, SEEK_SET) < 0)
	{
		logger_error("fseek error(%s)", acl_last_serror());
		return (false);
	}

	size_t  size;
	int   ret;

	while (len > 0)
	{
		size = sizeof(buf) > (size_t) len ? (size_t) len : sizeof(buf);
		ret = in.read(buf, size);
		if (ret < 0)
		{
			logger_error("read error(%s)", acl_last_serror());
			return (false);
		}
		if (out.write(buf, ret) == false)
		{
			logger_error("write error(%s), n: %d",
				acl_last_serror(), ret);
			return (false);
		}
		len -= ret;
	}

	return (true);
}

static bool save_as(ifstream& in, fstream& out, MIME_NODE* node)
{
	if (node->header_begin < 0 || node->header_end <= node->header_begin)
	{
		logger_warn("node invalid, header_begin(%ld), "
			"header_end(%ld), body_begin(%ld), body_end(%ld)",
			(long int) node->header_begin,
			(long int) node->header_end,
			(long int) node->body_begin,
			(long int) node->body_end);
		return (true);
	}

	ssize_t len = node->header_end - node->header_begin;
	if (len <= 0)
	{
		logger_warn("header_begin(%ld) >= header_end(%ld)",
			(long int) node->header_begin,
			(long int) node->header_end);
		return (true);
	}
	if (save_as(in, out, node->header_begin, len) == false)
		return (false);

	// 对于 multipart 邮件其 bound_end 应大于 body_begin,
	// 而对于非 multipart 邮件其 bound_end 应为 0，此时应
	// 采用 body_end
	if (node->bound_end > node->body_begin)
		len = node->bound_end - node->body_begin;
	else
		len = node->body_end - node->body_begin;
	if (len <= 0)
		return (true);
	return (save_as(in, out, node->body_begin, len));
}

bool mime::save_as(fstream& out)
{
	if (m_pFilePath == NULL)
	{
		logger_error("no input filePath");
		return (false);
	}

	ACL_ITER iter;
	ifstream in;

	if (in.open_read(m_pFilePath) == false)
	{
		logger_error("open input file %s error(%s)",
			m_pFilePath, acl_last_serror());
		return (false);
	}

	//if (acl_ring_size(&m_pMimeState->root->children) == 0)
	//{
	//	// 如果没有 MIME 结点则说明不是 multipart 格式
	//	return (copy_file(in, out));
	//}

	// 说明是 multipart 格式

	// 输出邮件头
	if (acl::save_as(in, out, m_pMimeState->root) == false)
		return (false);

	// 输出邮件体的各个一级 MIME 结点及所属内容
	acl_foreach(iter, m_pMimeState->root)
	{
		MIME_NODE* node = (MIME_NODE*) iter.data;
		if (acl::save_as(in, out, node) == false)
			return (false);
	}

	// 输出最后一个空行
	if (m_pMimeState->use_crlf)
		out << "\r\n";
	else
		out << "\n";

	return (true);
}

bool mime::save_as(const char* file_path)
{
	if (m_pFilePath == NULL)
	{
		logger_error("no input filePath");
		return (false);
	}

	acl::fstream out;
	if (out.open_trunc(file_path) == false)
	{
		logger_error("open file %s error(%s)",
			file_path, acl_last_serror());
		return (false);
	}

	bool ret = save_as(out);
	if (ret == false)
	{
#ifdef ACL_WINDOWS
		_unlink(file_path);
#else
		unlink(file_path);
#endif
		return (false);
	}
	return (true);
}

bool mime::save_mail(const char* path, const char* filename,
	bool enableDecode /* = true */, const char* toCharset /* = "gb2312" */,
	off_t off /* = 0 */)
{
	mime_body* pBody = get_body_node(true, true, NULL);
	if (pBody == NULL)
		return (false);

	string filepath;
	acl_make_dirs(path, 0700);

	if (!pBody->html_stype()
		&& pBody->parent_ctype() != MIME_CTYPE_MULTIPART)
		//&& pBody->parent_stype() != MIME_STYPE_RELATED)
	{
		filepath << path << "/" << filename;
		return (pBody->save_body(filepath.c_str()));
	}

	const std::list<mime_image*>& images =
		get_images(enableDecode, toCharset, off);
	if (images.empty())
	{
		filepath.clear();
		filepath << path << "/" << filename;
		return (pBody->save_body(filepath.c_str()));
	}

	string buf;
	if (pBody->save_body(buf) == false)
		return (false);

	string subPath(path);
	subPath << "/img";
	acl_make_dirs(subPath.c_str(), 0700);

	const char* name;
	std::list<mime_image*>::const_iterator cit = images.begin();
	for (; cit != images.end(); ++cit)
	{
		name = (*cit)->get_name();
		if (name == NULL)
			continue;
		filepath.clear();
		filepath << subPath.c_str() << "/" << name;
		(*cit)->save(filepath.c_str());
	}

	filepath.clear();
	filepath << path << "/" << filename;
	ofstream fpOut;
	if (fpOut.open_write(filepath.c_str()) == false)
	{
		logger_error("create %s error(%s)",
			filepath.c_str(), acl_last_serror());
		return (false);
	}

#define	SKIP_SP(p) { while (*(p) == ' ' || *(p) == '\t') (p)++; }

	char* s = buf.c_str(), *ptr;
	const mime_image* pImg;
	string cid(64), cidkey(64);
	char last_ch = 0;

	while (true)
	{
		ptr = acl_strcasestr(s, "cid:");
		if (ptr == NULL)
			break;
		fpOut.write(s, ptr - s);
		ptr += 4;
		SKIP_SP(ptr);
		if (*ptr == 0)
		{
			s = NULL;
			break;
		}
		s = ptr;
		cidkey.clear();

		while (*ptr)
		{
			if (*ptr == ' ' || *ptr == '\t'
				|| *ptr == '"' || *ptr == '\'' || *ptr == '>')
			{
				cid.copy(s, ptr - s);
				cidkey = '<';
				cidkey += cid.c_str();
				cidkey += '>';

				last_ch = *ptr;
				ptr++;
				break;
			}
			ptr++;
		}

		if (*ptr == 0 || cidkey.empty())
		{
			fpOut << cid.c_str();
			s = NULL;
			break;
		}

		pImg = get_image(cidkey.c_str(), enableDecode, toCharset, off);
		if (pImg == NULL || (name = pImg->get_name()) == NULL)
		{
			fpOut << cid.c_str();
			if (last_ch)
			{
				fpOut << last_ch;
				last_ch = 0;
			}
			s = ptr;
			continue;
		}

		filepath.clear();
		filepath << "img/" << name;
		fpOut << filepath.c_str();
		if (last_ch)
		{
			fpOut << last_ch;
			last_ch = 0;
		}

		s = ptr;
	}

	if (s)
		fpOut << s;

	return (true);
}

static MIME_NODE *get_alternative(MIME_STATE *pMime)
{
	// 从根结点开始遍历整个 multipart 邮件中所有的子结点，
	// 直至找到第一个不包含子结点的结点，因为遍历过程本身
	// 保证了遍历是自上而下遍历的，即先遍历根结点的子结点，
	// 然后层层遍历子结点的子结点直至满足条件为止

	MIME_NODE* pAlterNative = NULL;
	ACL_ITER iter;

	acl_foreach(iter, pMime)
	{
		MIME_NODE* pNode = (MIME_NODE*) iter.data;
		if (pNode->ctype != MIME_CTYPE_MULTIPART
			|| pNode->stype != MIME_STYPE_ALTERNATIVE)
		{
			continue;
		}
		// 如果该结点的子结点数小于 2 则说明邮件格式有误
		if (acl_ring_size(&pNode->children) < 2)
			continue;
		pAlterNative = pNode;
	}

	return pAlterNative;
}

static MIME_NODE *get_text_html(MIME_NODE *pAlterNative, bool *is_html = NULL)
{
	ACL_ITER iter;
	MIME_NODE* pHtml = NULL, *pText = NULL;

	acl_foreach(iter, pAlterNative)
	{
		MIME_NODE* pNode = (MIME_NODE*) iter.data;
		if (pNode->ctype != MIME_CTYPE_TEXT)
			continue;

		if (pNode->stype == MIME_STYPE_HTML)
		{
			pHtml = pNode;
			break;
		}
		else if (pNode->stype == MIME_STYPE_PLAIN)
		{
			// 仅保留第一个纯文本的结点
			if (pText == NULL)
				pText = pNode;
		}
	}

	if (is_html)
	{
		if (pHtml != NULL)
			*is_html = true;
		else
			*is_html = false;
	}

	return pHtml != NULL ? pHtml : pText;
}

static MIME_NODE *get_text_plain(MIME_NODE *pAlterNative, bool *is_html = NULL)
{
	ACL_ITER iter;
	MIME_NODE* pHtml = NULL, *pText = NULL;

	acl_foreach(iter, pAlterNative)
	{
		MIME_NODE* pNode = (MIME_NODE*) iter.data;
		if (pNode->ctype != MIME_CTYPE_TEXT)
			continue;

		if (pNode->stype == MIME_STYPE_PLAIN)
		{
			pText = pNode;
			break;
		}
		else if (pNode->stype == MIME_STYPE_HTML)
		{
			// 仅保留第一个HTML结点
			if (pHtml == NULL)
				pHtml = pNode;
		}
	}

	if (is_html)
	{
		if (pText == NULL && pHtml != NULL)
			*is_html = true;
		else
			*is_html = false;
	}

	return pText != NULL ? pText : pHtml;
}

// 找到邮件正文结点
static MIME_NODE* body_node(MIME_STATE* pMime, bool htmlFirst,
	bool *is_html = NULL)
{
	if (pMime->root->ctype == MIME_CTYPE_TEXT)
	{
		if (is_html)
			*is_html = pMime->root->stype == MIME_STYPE_HTML;
		return pMime->root;
	}

	if (pMime->root->ctype != MIME_CTYPE_MULTIPART)
		return NULL;
	else if (pMime->root->boundary == NULL)
	{
		logger_warn("no boundary for multipart");
		return NULL;
	}

	MIME_NODE *pAlterNative;

	if (pMime->root->stype == MIME_STYPE_ALTERNATIVE)
		pAlterNative = pMime->root;
	else
		pAlterNative = get_alternative(pMime);

	if (pAlterNative != NULL)
	{
		if (htmlFirst)
			return get_text_html(pAlterNative, is_html);
		else
			return get_text_plain(pAlterNative, is_html);
	}

	if (htmlFirst)
		return get_text_html(pMime->root, is_html);
	else
		return get_text_plain(pMime->root, is_html);
}

#if 0
#define EQ2(x, y) (((x) == NULL && (y) == NULL)  \
	|| ((x) != NULL && (y) != NULL && !strcasecmp((x), (y))))
#endif

mime_body* mime::get_body_node(bool htmlFirst,
	bool enableDecode /* = true */,
	const char* toCharset /* = "gb2312" */,
	off_t off /* = 0 */)
{
	if (m_pBody != NULL)
	{
		//const char* ptr = m_pBody->get_toCharset();
		//if (EQ2(toCharset, ptr))
		//	return m_pBody;
		delete m_pBody;
	}

	MIME_NODE* node = body_node(m_pMimeState, htmlFirst);
	if (node == NULL)
		return NULL;

	m_pBody = NEW mime_body(m_pFilePath, node, htmlFirst,
			enableDecode, toCharset, off);
	return m_pBody;
}

mime_body* mime::get_html_body(bool enableDecode /* = true */,
	const char* toCharset /* = "gb2312" */, off_t off /* = 0 */)
{
	if (m_pBody != NULL)
		delete m_pBody;

	bool is_html = false;
	MIME_NODE* node = body_node(m_pMimeState, true, &is_html);
	if (node == NULL || !is_html)
		return NULL;

	m_pBody = NEW mime_body(m_pFilePath, node, true,
		enableDecode, toCharset, off);
	return m_pBody;
}

mime_body* mime::get_plain_body(bool enableDecode /* = true */,
	const char* toCharset /* = "gb2312" */, off_t off /* = 0 */)
{
	if (m_pBody != NULL)
		delete m_pBody;

	bool is_html = false;
	MIME_NODE* node = body_node(m_pMimeState, false, &is_html);
	if (node == NULL || is_html)
		return NULL;

	m_pBody = NEW mime_body(m_pFilePath, node, false,
		enableDecode, toCharset, off);
	return m_pBody;
}

const std::list<mime_node*>& mime::get_mime_nodes(bool enableDecode /* = true */,
	const char* toCharset /* = "gb2312" */, off_t off /* = 0 */)
{
	if (m_pNodes == NULL)
		m_pNodes = NEW std::list<mime_node*>;
	else if (!m_pNodes->empty())
		return (*m_pNodes);

	if (m_pMimeState == NULL)
		return (*m_pNodes);

	ACL_ITER iter;
	MIME_NODE* node;
	acl_foreach(iter, m_pMimeState)
	{
		node = (MIME_NODE*) iter.data;
		m_pNodes->push_back(NEW mime_node(m_pFilePath, node,
				enableDecode, toCharset, off));
	}
	return (*m_pNodes);
}

static bool has_content_id(MIME_NODE* node)
{
	return mime_head_value(node, "Content-ID") == NULL ? false : true;
}

const std::list<mime_attach*>& mime::get_attachments(bool enableDecode /* = true */,
	const char* toCharset /* = "gb2312" */, off_t off /* = 0 */,
	bool all /* = true */)
{
	if (m_pAttaches == NULL)
		m_pAttaches = NEW std::list<mime_attach*>;
	else if (!m_pAttaches->empty())
		return (*m_pAttaches);

	if (m_pMimeState == NULL)
		return (*m_pAttaches);

#define EQ	!strcasecmp
#define CHECK(t) (EQ((t), "message") || EQ((t), "image") || EQ((t), "application"))

	ACL_ITER iter;
	mime_attach* attach;
	MIME_NODE* node;
	acl_foreach(iter, m_pMimeState)
	{
		node = (MIME_NODE*) iter.data;
		if (node->header_filename != NULL || has_content_id(node))
		{
			attach = NEW mime_attach(m_pFilePath, node,
					enableDecode, toCharset, off);
			m_pAttaches->push_back(attach);
		}
		else if (all && node->ctype_s && CHECK(node->ctype_s))
		{
			attach = NEW mime_attach(m_pFilePath, node,
				enableDecode, toCharset, off);
			m_pAttaches->push_back(attach);
		}
	}

	return (*m_pAttaches);
}

const std::list<mime_image*>& mime::get_images(bool enableDecode /* = true */,
	const char* toCharset /* = "gb2312" */, off_t off /* = 0 */)
{
	if (m_pImages == NULL)
		m_pImages = NEW std::list<mime_image*>;
	else if (!m_pImages->empty())
		return (*m_pImages);

	if (m_pMimeState == NULL)
		return (*m_pImages);

	ACL_ITER iter;
	MIME_NODE* node;
	acl_foreach(iter, m_pMimeState)
	{
		node = (MIME_NODE*) iter.data;
		if (node->ctype != MIME_CTYPE_IMAGE)
			continue;
		m_pImages->push_back(NEW mime_image(m_pFilePath,
				node, enableDecode, toCharset, off));
	}
	return (*m_pImages);
}

mime_image* mime::get_image(const char* cid, bool enableDecode /* = true */,
	const char* toCharset /* = "gb2312" */, off_t off /* = 0 */)
{
	const char* ptr;
	const std::list<mime_image*>& images =
		get_images(enableDecode, toCharset, off);
	std::list<mime_image*>::const_iterator cit = images.begin();
	for (; cit != images.end(); ++cit)
	{
		ptr = (*cit)->header_value("Content-ID");
		if (ptr != NULL && strcmp(ptr, cid) == 0)
			return (*cit);
	}

	return (NULL);
}

static void mime_node_dump(const char* from_path, const char* dump_path,
	MIME_NODE *node, bool decode)
{
	ifstream in;
	fstream out;
	string path;
	char *pbuf;
	static int  i = 0;

	if (dump_path == NULL)
		return;

	if (in.open_read(from_path) == false)
		return;

	path.format("%s/node_%d.txt", dump_path, i++);
	if (out.open_trunc(path.c_str()) == false) {
		logger_error("open %s error(%s)", path.c_str(),
			acl_last_serror());
		return;
	}

	ssize_t dlen = node->header_end - node->header_begin;

	out.puts(">---------header begin--------<");
	off_t pos = (off_t) in.fseek(node->header_begin, SEEK_SET);
	pbuf = (char*) acl_mymalloc(dlen);
	printf(">>>%s: header begin: %ld, end: %ld, len: %ld\n",
		__FUNCTION__, (long int) node->header_begin,
		(long int) node->header_end, (long int) dlen);

	int   ret;
	if ((ret = in.read(pbuf, dlen, true)) < 0) {
		acl_myfree(pbuf);
		return;
	}
	printf(">>>%s: ret: %d\n", __FUNCTION__, ret);

	out.write(pbuf, ret);

	out.puts(">---------header end----------<");
	acl_myfree(pbuf);

	dlen = node->body_end - node->body_begin;
	printf(">>>%s: body begin: %ld, end: %ld, len: %ld\r\n",
		__FUNCTION__, (long int) node->body_begin,
		(long int) node->body_end, (long int) dlen);

	out.format(">---------body begin(length: %d)----------<\r\n", (int) dlen);

	if (dlen <= 0) {
		printf(">>>%s: body_begin(%ld) >= body_end(%ld), len: %d\r\n",
			__FUNCTION__, (long int) node->body_begin,
			(long int) node->body_end, (int) dlen);
		out.close();
		in.close();
		return;
	}
	pos = (off_t) in.fseek(node->body_begin, SEEK_SET);
	if (pos == -1)
	{
		printf(">>>%s: fseek error(%s)\r\n", __FUNCTION__,
			acl_last_serror());
		return;
	}

	pbuf = (char*) acl_mymalloc(dlen);
	if ((ret = in.read(pbuf, dlen, true)) == -1) {
		acl_myfree(pbuf);
		return;
	}

	mime_code* decoder;
	if (decode)
		decoder = mime_code::create(node->encoding);
	else
		decoder = NULL;

	if (decoder == NULL)
		out.write(pbuf, dlen);
	else
	{
		acl::string result(8192);
		const char* ptr = pbuf;
		ssize_t n;

		while (dlen > 0)
		{
			n = dlen > 8192 ? 8192 : dlen;
			//n = dlen;
			decoder->decode_update(ptr, n, &result);
			if (result.length() > 0)
			{
				out.write(result.c_str(), result.length());
				result.clear();
			}
			dlen -= n;
			ptr += n;
		}

		decoder->decode_finish(&result);
		if (result.length() > 0)
			out.write(result.c_str(), result.length());
		delete decoder;
	}

	acl_myfree(pbuf);
	out.puts(">---------body end------------<");
	out.close();
	in.close();
}

void mime::mime_debug(const char* save_path, bool decode /* = true */)
{
	MIME_STATE* state = m_pMimeState;
	// 如果 multipart 格式, 则分析各个部分数据
	MIME_STATE state_dummy;
	ACL_ITER iter;

	if (m_pFilePath == NULL)
	{
		logger_error("m_pFilePath null");
		return;
	}

	if (save_path == NULL)
		return;

	printf("primary node ctype: %s, stype: %s\r\n",
		get_ctype(), get_stype());

	state_dummy.root = state->root;
	mime_state_foreach_init(&state_dummy);
	string header_name, header_filename;

	acl_foreach(iter, &state_dummy)
	{
		MIME_NODE *node = (MIME_NODE*) iter.data;
		printf("child node->ctype: %s, stype: %s\r\n",
			node->ctype_s ? node->ctype_s : "null",
			node->stype_s ? node->stype_s : "null");

		if (node->boundary)
			printf(">>boundary: %s\r\n",
				acl_vstring_str(node->boundary));

		if (node->header_filename)
		{
			header_filename.clear();
			if (rfc2047::decode(node->header_filename,
				(int) strlen(node->header_filename),
				&header_filename,
				"gbk", true, false) == false)
			{
				printf(">>filename: %s\r\n",
					node->header_filename);
			}
			else
				printf(">>filename: %s\r\n",
					header_filename.c_str());
		}
		if (node->charset)
			printf(">>charset: %s\r\n", node->charset);
		if (node->header_name)
		{
			header_name.clear();
			if (rfc2047::decode(node->header_name,
				(int) strlen(node->header_name),
				&header_name, "gbk", true, false) == false)
			{
				printf(">>name: %s\r\n", node->header_name);
			}
			else
				printf(">>name: %s\r\n", header_name.c_str());
		}

		mime_node_dump(m_pFilePath, save_path, node, decode);
	}
}

} // namespace acl
