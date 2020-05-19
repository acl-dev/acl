#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/bitmap.hpp"
#endif
#include "acl_cpp/stdlib/bitmap.hpp"

namespace acl {

bitmap::bitmap(const void* buf, size_t len)
: size_(len)
, count_(0)
{
	assert(len > 0);

	bmp_ = new unsigned char[(len + 7) / 8];
	memcpy(bmp_, buf, (len + 7) / 8);
	recount();
}

bitmap::bitmap(size_t len)
: size_(len)
, count_(0)
{
	assert(len > 0);

	bmp_ = new unsigned char[(len + 7) / 8];
	memset(bmp_, 0, (len + 7) / 8);
}

bitmap::~bitmap(void)
{
	delete[] bmp_;
}

bool bitmap::bit_isset(size_t n) const
{
	if (n < size_) {
		return (bmp_[n / 8] >> n % 8) & 1;
	} else {
		return false;
	}
}

bool bitmap::bit_set(size_t n)
{
	if (n < size_ && !bit_isset(n)) {
		unsigned char t = 1 << n % 8;
		bmp_[n / 8] |= t;
		count_++;
		return true;
	} else {
		return false;
	}
}

bool bitmap::bit_unset(size_t n)
{
	if (bit_isset(n)) {
		unsigned char t = 1 << n % 8;
		bmp_[n / 8] &= ~t;
		count_--;
		return true;
	} else {
		return false;
	}
}

size_t bitmap::tobuf(void* buf, size_t len) const
{
	if (len >= (size_ + 7) / 8) {
		memcpy(buf, bmp_, (size_ + 7) / 8);
		return size_;
	}
	return size_;
}

bool bitmap::frombuf(const void* buf, size_t len)
{
	if (len >= (size_ + 7) / 8) {
		memcpy(bmp_, buf, (size_ + 7) / 8);
		recount();
		return true;
	}
	return false;
}

void bitmap::reset(void)
{
	memset(bmp_, 0, (size_ + 7) / 8);
	count_ = 0;
}

size_t bitmap::size(void) const
{
	return size_;
}

size_t bitmap::space(void) const
{                                                                                   
	return (size_ + 7) / 8;                                                    
}

size_t bitmap::count(void) const
{
	return count_;
}

bool bitmap::full(void) const
{
	return size_ == count_;
}

void bitmap::recount(void)
{
	count_ = 0;
	for (size_t i = 0; i < size_; ++i) {
		if ((bmp_[i / 8] >> i % 8) & 1) {
			count_++;
		}
	}
}

} // namespace acl
