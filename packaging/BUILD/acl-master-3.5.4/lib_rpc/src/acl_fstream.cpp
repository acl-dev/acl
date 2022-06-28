#include "stdafx.h"
#include "google/protobuf/io/acl_fstream.h"

namespace google {
namespace protobuf {
namespace io {

acl_ifstream::acl_ifstream(acl::istream* input, int block_size)
: copying_input_(input)
, impl_(&copying_input_, block_size)
{
}

acl_ifstream::~acl_ifstream()
{
}

bool acl_ifstream::Next(const void** data, int* size)
{
	return impl_.Next(data, size);
}

void acl_ifstream::BackUp(int count)
{
	impl_.BackUp(count);
}

bool acl_ifstream::Skip(int count)
{
	return impl_.Skip(count);
}

int64 acl_ifstream::ByteCount() const
{
	return impl_.ByteCount();
}

acl_ifstream::CopyingAclInputStream::CopyingAclInputStream(acl::istream* input)
	: input_(input)
{
}

acl_ifstream::CopyingAclInputStream::~CopyingAclInputStream()
{
}

int acl_ifstream::CopyingAclInputStream::Read(void* buffer, int size)
{
	int ret = input_->read(buffer, (size_t) size, false);
	return ret;
}

//////////////////////////////////////////////////////////////////////////

acl_ofstream::acl_ofstream(acl::ostream* output,
	int block_size /* = -1 */)
: copying_output_(output)
, impl_(&copying_output_, block_size)
{

}

acl_ofstream::~acl_ofstream()
{
	impl_.Flush();
}

bool acl_ofstream::Flush()
{
	return impl_.Flush();
}

bool acl_ofstream::Next(void** data, int* size)
{
	return impl_.Next(data, size);
}

void acl_ofstream::BackUp(int count)
{
	impl_.BackUp(count);
}

int64 acl_ofstream::ByteCount() const
{
	return impl_.ByteCount();
}

acl_ofstream::CopyingAclOutputStream::CopyingAclOutputStream(
	acl::ostream* output)
	: output_(output)
{
}

acl_ofstream::CopyingAclOutputStream::~CopyingAclOutputStream()
{
}

bool acl_ofstream::CopyingAclOutputStream::Write(
	const void* buffer, int size)
{
	return output_->write(buffer, (size_t) size) == size ? true : false;
}

}  // namespace io
}  // namespace protobuf
}  // namespace google
