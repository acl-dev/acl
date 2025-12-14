#pragma once
#include "../acl_cpp_define.hpp"
#include "pipe_stream.hpp"

typedef struct z_stream_s z_stream;

namespace acl {

/**
 * Compression level type definition. This enumeration defines a trade-off
 * between compression speed and compression ratio.
 * Higher compression values result in better compression ratio, but slower
 * compression speed.
 */
typedef enum {
	zlib_default = -1,      // Default compression level
	zlib_level0 = 0,        // Lowest compression ratio, actually no compression
	zlib_best_speed = 1,    // Fastest compression speed, lowest compression ratio
	zlib_level1 = zlib_best_speed,
	zlib_level2 = 2,
	zlib_level3 = 3,
	zlib_level4 = 4,
	zlib_level5 = 5,
	zlib_level6 = 6,
	zlib_level7 = 7,
	zlib_level8 = 8,
	zlib_best_compress = 9, // Highest compression ratio, slowest compression speed
	zlib_level9 = zlib_best_compress
} zlib_level_t;

/**
 * Compression window size in compression library. Higher values result in
 * better compression efficiency but use more memory.
 * For HTTP compression transfer, you need to use negative values of these
 * values, e.g., -zlib_wbits_t
 */
enum {
	zlib_wbits_8  = 8,
	zlib_wbits_9  = 9,
	zlib_wbits_10 = 10,
	zlib_wbits_11 = 11,
	zlib_wbits_12 = 12,
	zlib_wbits_13 = 13,
	zlib_wbits_14 = 14,
	zlib_wbits_15 = 15,
};

/**
 * Memory level property in compression library. Higher values use more memory.
 */
typedef enum {
	zlib_mlevel_1 = 1,
	zlib_mlevel_2 = 2,
	zlib_mlevel_3 = 3,
	zlib_mlevel_4 = 4,
	zlib_mlevel_5 = 5,
	zlib_mlevel_6 = 6,
	zlib_mlevel_7 = 7,
	zlib_mlevel_8 = 8,
	zlib_mlevel_9 = 9,
} zlib_mlevel_t;

/**
 * Flush mode in compression/decompression library. Controls whether
 * compression/decompression library flushes buffer.
 * Generally, to get higher compression ratio, you should choose zlib_flush_off
 * mode.
 */
typedef enum {
	zlib_flush_off = 0,     // Do not flush buffer to user buffer
	zlib_flush_partial = 1, // Flush partial buffer to user buffer
	zlib_flush_sync = 2,    // Synchronous flush
	zlib_flush_full = 3,    // Full flush
	zlib_flush_finish = 4   // Full flush and stop compression/decompression
} zlib_flush_t;

enum {
	zlib_flags_zip_begin   = 1,
	zlib_flags_zip_end     = 1 << 1,
	zlib_flags_unzip_begin = 1 << 2,
	zlib_flags_unzip_end   = 1 << 3,
};

class string;

class ACL_CPP_API zlib_stream : public pipe_stream {
public:
	zlib_stream();
	~zlib_stream();

	/**
	 * One-time compression.
	 * @param in {const char*} Source data.
	 * @param len {int} Source data length.
	 * @param out {string*} User buffer to store compressed data.
	 * @param level {zlib_level_t} Compression level. Higher level means higher
	 * compression ratio,
	 *  but slower compression speed.
	 * @return {bool} Whether compression was successful.
	 */
	bool zlib_compress(const char* in, int len, string* out,
		zlib_level_t level = zlib_default);

	/**
	 * One-time decompression.
	 * @param in {const char*} Source compressed data.
	 * @param len {int} Source data length.
	 * @param out {string*} User buffer to store decompressed data.
	 * @param have_zlib_header {bool} Whether there is zlib_header header.
	 *  For HTTP transfer protocol, this value should be set to false.
	 * @param wsize {int} Window size in decompression library.
	 * @return {bool} Whether decompression was successful.
	 */
	bool zlib_uncompress(const char* in, int len, string* out,
		bool have_zlib_header = true, int wsize = 15);

	///////////////////////////////////////////////////////////////
	//
	//           The following are streaming compression and streaming decompression methods.
	//
	///////////////////////////////////////////////////////////////

	/**
	 * Initialize compression process. For streaming compression mode, the calling
	 * sequence is:
	 * zip_begin->zip_update->zip_finish. If any step in the middle
	 * fails, you should call zip_reset.
	 * @param level {zlib_level_t} Compression level. Higher level means higher
	 * compression
	 *  ratio, but slower compression speed.
	 * @param wbits {zlib_wbits_t} Window size in compression library. Higher
	 * values result in
	 * better compression efficiency but use more memory. For HTTP transfer
	 * compression, you should use
	 *  negative values of these values, e.g., -zlib_wbits_15
	 * @param mlevel {zlib_mlevel_t} Memory level property in compression library.
	 * Higher values
	 *  result in better compression efficiency but use more memory.
	 * @return {bool} Whether compression initialization was successful. Failure
	 * reasons are generally
	 *  related to whether parameters are correct.
	 */
	bool zip_begin(zlib_level_t level = zlib_default,
		int wbits = zlib_wbits_15,
		zlib_mlevel_t mlevel = zlib_mlevel_9);

	/**
	 * Call this function in a loop to compress source data.
	 * @param in {const char*} Source data.
	 * @param len {int} Source data length.
	 * @param out {string*} User buffer. This function appends to user
	 *  provided buffer with compressed results. Users should check the buffer
	 * capacity before calling this function to ensure the data length appended by
	 * this function. Additionally,
	 * depending on the zlib_flush_t option chosen, user buffer data may not have
	 * read all
	 *  results.
	 * @param flag {zlib_flush_t} Buffer flush mode in compression library.
	 *  zlib_flush_off: Data may not be flushed to user buffer immediately.
	 * zlib library itself flushes in its own way, which may get higher compression
	 * ratio.
	 *  zlib_flush_partial: Data may be partially flushed to user buffer.
	 *  zlib_flush_sync: Synchronously flush data to user buffer.
	 *  zlib_flush_full: Flush all buffered data in zlib library to user buffer.
	 * zlib_flush_finish: Calling this parameter indicates compression process
	 * ends, and will
	 * flush all results to user buffer. This parameter generally does not need to
	 * be used, because
	 * after calling zip_finish, it will automatically flush all buffers to user
	 * buffer.
	 * @return {bool} Whether compression process failed.
	 */
	bool zip_update(const char* in, int len, string* out,
		zlib_flush_t flag = zlib_flush_off);

	/**
	 * Call this parameter to indicate compression process ends.
	 * @param out {string} User buffer. This function will flush zlib library
	 * buffer
	 *  data to user buffer in appending mode.
	 * @return {bool} Whether successful.
	 */
	bool zip_finish(string* out);

	/**
	 * Determine whether compression process has finished.
	 * @return {bool}
	 */
	bool zip_finished() const {
		return (zlib_flags_ & zlib_flags_zip_end) != 0;
	}

	/**
	 * Reset compression state. Generally only called when compression process
	 * fails.
	 * @return {bool} Whether successful.
	 */
	bool zip_reset();

	/**
	 * In compression process, you can use this function to calculate crc32
	 * checksum value of data.
	 * @param n {unsigned} Last calculated checksum value. Write 0 for the first
	 * time.
	 * @param buf {const void*} Address of data to be checksummed. Write NULL for
	 * the first use.
	 * @param dlen {size_t} Length of buf data. Write 0 for the first use.
	 * @return {unsinged} Current calculated checksum value.
	 */
	unsigned crc32_update(unsigned n, const void* buf, size_t dlen);

	/**
	 * Initialize decompression process. For streaming decompression mode, the
	 * calling sequence is:
	 * unzip_begin->unzip_update->unzip_finish. If any step in the middle
	 * fails, you should call unzip_reset.
	 * @param have_zlib_header {bool} Whether there is zlib_header header.
	 *  For HTTP transfer protocol, this value should be set to false.
	 * @param wsize {int} Window size in decompression library.
	 * @return {bool} Whether decompression initialization was successful. Failure
	 * reasons are generally
	 *  related to whether parameters are correct.
	 */
	bool unzip_begin(bool have_zlib_header = true, int wsize = 15);

	/**
	 * Call this function in a loop to decompress source data.
	 * @param in {const char*} Compressed source data.
	 * @param len {int} Source data length.
	 * @param out {string*} User buffer. This function appends to user
	 *  provided buffer with decompressed results. Users should check the buffer
	 * capacity before calling this function to ensure the data length appended by
	 * this function. Additionally,
	 * depending on the zlib_flush_t option chosen, user buffer data may not have
	 * read all
	 *  results.
	 * @param flag {zlib_flush_t} Buffer flush mode in decompression library.
	 *  zlib_flush_off: Data may not be flushed to user buffer immediately.
	 *    zlib library itself flushes in its own way.
	 *  zlib_flush_partial: Data may be partially flushed to user buffer.
	 *  zlib_flush_sync: Synchronously flush data to user buffer.
	 *  zlib_flush_full: Flush all buffered data in zlib library to user buffer.
	 * zlib_flush_finish: Calling this parameter indicates decompression process
	 * ends, and will
	 * flush all results to user buffer. This parameter generally does not need to
	 * be used, because
	 * after calling zip_finish, it will automatically flush all buffers to user
	 * buffer.
	 * @return {bool} Whether decompression process failed.
	 */
	bool unzip_update(const char* in, int len, string* out,
		zlib_flush_t flag = zlib_flush_off);

	/**
	 * Call this parameter to indicate decompression process ends.
	 * @param out {string} User buffer. This function will flush zlib library
	 * buffer
	 *  data to user buffer in appending mode.
	 * @return {bool} Whether successful.
	 */
	bool unzip_finish(string* out);

	/**
	 * Determine whether decompression process has finished.
	 * @return {bool}
	 */
	bool unzip_finished() const {
		return (zlib_flags_ & zlib_flags_unzip_end) != 0;
	}

	/**
	 * Reset decompression state. Generally only called when decompression process
	 * fails.
	 * @return {bool} Whether successful.
	 */
	bool unzip_reset();

	/**
	 * Get current zstream handle.
	 * @return {z_stream*}
	 */
	z_stream* get_zstream() const {
		return zstream_;
	}

	/**
	 * When using dynamic library loading method, you can use this function to set
	 * the dynamic library's full path.
	 */
	static void set_loadpath(const char* path);

	/**
	 * When you need to get the dynamic library's full path, you can get the
	 * dynamic library's full path through this function.
	 * @return {const char*} Returns NULL if not set.
	 */
	static const char* get_loadpath();

	/**
	 * Manually load dynamic library zlib method. This is for static linking.
	 * Generally, you do not need to call this function.
	 * Additionally, when zlib dynamic library is not in the program's directory,
	 * you need to first call
	 * set_loadpath() to set zlib dynamic library's full path.
	 * This function internally uses pthread_once() to ensure it is only called
	 * once, so even if multiple
	 * threads call it simultaneously, it is safe.
	 * @return {bool} Whether loading was successful.
	 */
	static bool zlib_load_once();

	///////////////////////////////////////////////////////////////

	bool pipe_zip_begin(zlib_level_t level = zlib_default,
		zlib_flush_t flag = zlib_flush_off);
	bool pipe_unzip_begin(zlib_flush_t flag = zlib_flush_off);

	// pipe_stream virtual function implementation

	virtual int push_pop(const char* in, size_t len,
		string* out, size_t max = 0);
	virtual int pop_end(string* out, size_t max = 0);
	virtual void clear();

private:
	z_stream* zstream_;
	bool finished_;
	bool is_compress_;
	unsigned zlib_flags_;
	zlib_flush_t flush_;

	bool update(int (*func)(z_stream*, int), zlib_flush_t flag,
		const char* in, int len, string* out);
	bool flush_out(int (*func)(z_stream*, int),
		zlib_flush_t flag, string* out);
};

} // namespace acl

