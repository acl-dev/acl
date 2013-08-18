#include "acl_stdafx.hpp"
#include "acl_cpp/stream/fstream.hpp"

namespace acl {

fstream::fstream()
{

}

fstream::~fstream()
{
	close();
}

void fstream::open(ACL_FILE_HANDLE fh, unsigned int oflags)
{
	acl_assert(ACL_VSTREAM_FILE(m_pStream) == ACL_FILE_INVALID);

	m_pStream->fread_fn  = acl_file_read;
	m_pStream->fwrite_fn = acl_file_write;
	m_pStream->fwritev_fn = acl_file_writev;
	m_pStream->fclose_fn = acl_file_close;

	m_pStream->fd.h_file = fh;
	m_pStream->type = ACL_VSTREAM_TYPE_FILE;
	m_pStream->oflags = oflags;
	m_bOpened = true;
	m_bEof = false;
}

bool fstream::open(const char* path, unsigned int oflags, int mode)
{
	if (path == NULL)
		return (false);

	ACL_FILE_HANDLE fh;

	fh = acl_file_open(path, oflags, mode);
	if (fh == ACL_FILE_INVALID)
		return (false);

	m_pStream->fread_fn  = acl_file_read;
	m_pStream->fwrite_fn = acl_file_write;
	m_pStream->fwritev_fn = acl_file_writev;
	m_pStream->fclose_fn = acl_file_close;

	m_pStream->fd.h_file = fh;
	m_pStream->type = ACL_VSTREAM_TYPE_FILE;
	m_pStream->oflags = oflags;
	ACL_SAFE_STRNCPY(m_pStream->remote_addr, path,
		sizeof(m_pStream->remote_addr));
	m_bOpened = true;
	m_bEof = false;
	return (true);
}

const char* fstream::file_path() const
{
	return m_pStream ? m_pStream->path : NULL;
}

bool fstream::open_trunc(const char* path)
{
	return (open(path, O_RDWR | O_CREAT | O_TRUNC, 0600));
}

bool fstream::create(const char* path)
{
	return (open(path, O_RDWR | O_CREAT, 0600));
}

bool fstream::close()
{
	if (m_bOpened == false)
		return (false);

	if (m_pStream == NULL)
		return (true);

	if (m_pStream->type != ACL_VSTREAM_TYPE_FILE)
		return (false);

	ACL_FILE_HANDLE fh = ACL_VSTREAM_FILE(m_pStream);
	if (fh == ACL_FILE_INVALID)
		return (false);
	m_bEof = true;
	m_bOpened = false;
	return (acl_file_close(fh) == 0 ? true : false);
}

acl_off_t fstream::fseek(acl_off_t offset, int whence)
{
	return (acl_vstream_fseek(m_pStream, offset, whence));
}

bool fstream::ftruncate(acl_off_t length)
{
	fseek(0, SEEK_SET); // 需要先将文件指针移到开始位置
	return (acl_file_ftruncate(m_pStream, length) == 0 ? true : false);
}

acl_int64 fstream::fsize() const
{
	return (acl_vstream_fsize(m_pStream));
}

ACL_FILE_HANDLE fstream::file_handle() const
{
	return (ACL_VSTREAM_FILE(m_pStream));
}

} // namespace acl
