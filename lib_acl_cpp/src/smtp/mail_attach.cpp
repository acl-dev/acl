#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/mime/mime_code.hpp"
#include "acl_cpp/mime//rfc2047.hpp"
#include "acl_cpp/stream/ostream.hpp"
#include "acl_cpp/stream/ifstream.hpp"
#include "acl_cpp/smtp/mail_attach.hpp"

namespace acl {

mail_attach::mail_attach(const char* filepath, const char* content_type,
	const char* charset)
: filepath_(filepath)
, ctype_(content_type)
, cid_(64)
, charset_(charset)
{
	string filename;
	filename.basename(filepath);

	// 文件名需要采用 rfc2047 编码
	if (!rfc2047::encode(filename.c_str(), (int) filename.size(),
		&filename_, charset, 'B', false))
	{
		filename_ = filename;
	}
}

mail_attach::~mail_attach()
{
}

mail_attach& mail_attach::set_content_id(const char* id)
{
	if (id && *id)
		cid_.format("%s", id);
	return *this;
}

bool mail_attach::save_to(mime_code* coder, string& out)
{
	if (coder)
		build_header(coder->get_encoding_type(), out);
	else
		build_header(NULL, out);

	string buf;
	if (ifstream::load(filepath_.c_str(), &buf) == false)
	{
		logger_error("load %s error %s",
			filepath_.c_str(), last_serror());
		return false;
	}

	if (coder)
	{
		coder->reset();
		coder->encode_update(buf.c_str(), (int) buf.size(), &out);
		coder->encode_finish(&out);
	}
	else
		out.append(buf);

	return true;
}

bool mail_attach::save_to(mime_code* coder, ostream& out)
{
	string header;

	if (coder)
		build_header(coder->get_encoding_type(), header);
	else
		build_header(NULL, header);

	if (out.write(header) == -1)
	{
		logger_error("write to stream error %s", last_serror());
		return false;
	}

	ifstream in;
	if (in.open_read(filepath_.c_str()) == false)
	{
		logger_error("open %s error %s",
			filepath_.c_str(), last_serror());
		return false;
	}

	char  buf[8192];
	int   ret;
	string body(8192);

	while (!in.eof())
	{
		ret = in.read(buf, sizeof(buf), false);
		if (ret == -1)
			break;
		if (coder == NULL)
		{
			if (out.write(buf, ret) == -1)
			{
				logger_error("write body error %s",
					last_serror());
				return false;
			}
		}

		coder->encode_update(buf, ret, &body);
		if (body.empty())
			continue;

		if (out.write(body) == -1)
		{
			logger_error("write body error %s", last_serror());
			return false;
		}
		body.clear();
	}

	if (coder)
	{
		coder->encode_finish(&body);
		body.append("\r\n");
	}
	else
		body.append("\r\n");

	if (out.write(body) == -1)
	{
		logger_error("write body error %s", last_serror());
		return false;
	}

	return true;
}

void mail_attach::build_header(const char* transfer_encoding, string& out)
{
	out.format_append("Content-Type: %s;\r\n", ctype_.c_str());
	out.format_append("\tname=\"%s\"\r\n", filename_.c_str());

	if (transfer_encoding && *transfer_encoding)
		out.format_append("Content-Transfer-Encoding: %s\r\n",
			transfer_encoding);

	if (cid_.empty())
		out.format_append("Content-Disposition: attachment;\r\n"
			"\tfilename=\"%s\"\r\n", filename_.c_str());
	else
		out.format_append("Content-ID: <%s>\r\n", cid_.c_str());

	out.append("\r\n");
}

} // namespace acl
