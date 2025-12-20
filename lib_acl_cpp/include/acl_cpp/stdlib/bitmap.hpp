#pragma once
#include "../acl_cpp_define.hpp"
#include "noncopyable.hpp"

namespace acl {

class ACL_CPP_API bitmap : public noncopyable {
public:
	/**
	 * Constructor
	 * @param buf {const void*} Source data memory bit mapping area
	 * @param len {size_} Number of bits set in buf bit mapping memory area
	 */
	bitmap(const void* buf, size_t len);

	/**
	 * Constructor
	 * @param len {size_t} Maximum number of bit mappings that can be accommodated
	 */
	bitmap(size_t len);

	~bitmap();

	/**
	 * Map the given value in the bit set
	 * @param n {size_t}
	 * @return {bool} Returns true indicates add was successful, otherwise
	 * indicates value is out of bounds or already exists
	 */
	bool bit_set(size_t n);

	/**
	 * Determine whether the given data has been set in the bit mapping
	 * @param n {size_t}
	 * @return {bool} Determine whether the specified value exists in the bit
	 * mapping set
	 */
	bool bit_isset(size_t n) const;

	/**
	 * Remove the specified value from the bit set
	 * @param n {size_t}
	 * @return {bool} Returns false indicates value is out of bounds or does not
	 * exist in the bit set
	 */
	bool bit_unset(size_t n);

	/**
	 * Copy bitmap information to buf
	 * @param buf {void*} Store copy result
	 * @param len {size_t} Maximum length of buf
	 * @return {size_t} Returns successfully copied memory length. Returns 0
	 * indicates buf is too small
	 */
	size_t tobuf(void* buf, size_t len) const;

	/**
	 * Set current bitmap information from buf
	 * @param buf {const void*} Bitmap information to set
	 * @param len {size_t} Length of buf
	 * @return true on success, false on failure
	 */
	bool frombuf(const void* buf, size_t len);

	/**
	 * Reset current bitmap to 0
	 */
	void reset(void);

	/**
	 * Get the number of bits that can be stored in the current bit mapping storage
	 * space
	 * @return {size_t}
	 */
	size_t size() const;

	/**
	 * Get internal storage space size (bytes)
	 */
	size_t space() const;

	/**
	 * Get the number currently set
	 * @return {size_t}
	 */
	size_t count() const;

	/**
	 * Whether current bitmap is full
	 * @return {bool}
	 */
	bool full() const;

public:
	const unsigned char* get_bmp() const {
		return bmp_;
	}

	unsigned char* get_bmp() {
		return bmp_;
	}

private:
	unsigned char *bmp_;
	size_t size_;
	size_t count_;

	// Recalculate count number
	void recount();
};

} // namespace acl

