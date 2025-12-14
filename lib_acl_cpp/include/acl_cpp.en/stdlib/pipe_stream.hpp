#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include "noncopyable.hpp"
#include "string.hpp"

namespace acl {

/**
 * Bidirectional input/output buffered pipe stream. This stream can not only receive input data, but also output
 * input data. Pure virtual base class, subclasses need to implement three interface functions
 */
class ACL_CPP_API pipe_stream : public noncopyable {
public:
	pipe_stream() {}
	virtual ~pipe_stream() {}

	/**
	 * Data input/output interface
	 * @param in {const char*} Address of input data
	 * @param len {size_t} Input data length
	 * @param out {string*} Buffer for storing output result, cannot be empty
	 * @param max {size_t} Length limit for desired output result. If 0, it
	 *  means there is no limit, and all output results are stored in out buffer
	 * @return {int} Length of output data. If < 0, it indicates error
	 */
	virtual int push_pop(const char* in, size_t len, string* out, size_t max) = 0;

	/**
	 * Final processed output data interface
	 * @param out {string*} Buffer for storing output result, cannot be empty
	 * @param max {size_t} Length limit for desired output result. If 0, it
	 *  means there is no limit, and all output results are stored in out buffer
	 * @return {int} Length of output data. If < 0, it indicates error
	 */
	virtual int pop_end(string* out, size_t max) = 0;

	/**
	 * Clear internal buffer
	 */
	virtual void clear() {}
};

/**
 * String processing bidirectional management stream
 */
class ACL_CPP_API pipe_string : public pipe_stream {
public:
	pipe_string();
	pipe_string(string& s);
	virtual ~pipe_string();

	// Four virtual functions in pipe_stream base class
	virtual int push_pop(const char* in, size_t len, string* out, size_t max);
	virtual int pop_end(string* out, size_t max);
	virtual void clear() {
		m_pBuf->clear();
		m_pos = 0;
	}

	string& get_buf() const {
		return (*m_pBuf);
	}

	char* c_str() const {
		return (m_pBuf->c_str());
	}

	size_t length() const {
		return (m_pBuf->length());
	}

	bool empty() const {
		return (m_pBuf->empty());
	}

private:
	string* m_pBuf;
	string* m_pSavedBufPtr;
	size_t  m_pos;
};

/**
 * Pipe stream manager. This class manages all pipe streams, passes data sequentially to all pipe streams'
 * input interfaces, and gets data from all pipe streams' output interfaces, then passes data to
 * next pipe stream's input interface, and so on, until the last pipe stream
 */
class ACL_CPP_API pipe_manager : public noncopyable {
public:
	pipe_manager();
	~pipe_manager();

	/**
	 * Register new pipe stream processor in append mode
	 * @param stream {pipe_stream*} Pipe stream processor object
	 * @return {bool} Returns false if this pipe stream processor object already exists
	 */
	bool push_back(pipe_stream* stream);

	/**
	 * Register new pipe stream processor in prepend mode
	 * @param stream {pipe_stream*} Pipe stream processor object
	 * @return {bool} Returns false if this pipe stream processor object already exists
	 */
	bool push_front(pipe_stream* stream);

	/**
	 * Application adds new data to pipe stream manager. This manager passes data sequentially to all registered pipe stream
	 * processors, and receives processing results from registered pipe stream processors, passing them sequentially to the next
	 * @param src {const char*} Address of data to be processed
	 * @param len {size_t} src data length
	 * @param out {pipe_stream*} If not empty, this pipe processor will be the last pipe processor that only receives
	 *  input without output
	 * @return {bool} Whether an error occurred
	 */
	bool update(const char* src, size_t len, pipe_stream* out = NULL);

	/**
	 * Must call this function once at the end, so that data in some pipes' buffers can be flushed
	 * to the final pipe at once
	 * @param out {pipe_stream*} If not empty, this pipe processor will be the last pipe processor that only receives
	 *  input without output
	 * @return {bool} Whether an error occurred
	 */
	bool update_end(pipe_stream* out = NULL);

	pipe_manager& operator<<(const string&);
	pipe_manager& operator<<(const string*);
	pipe_manager& operator<<(const char*);
#if defined(_WIN32) || defined(_WIN64)
	pipe_manager& operator<<(__int64);
	pipe_manager& operator<<(unsigned __int64);
#else
	pipe_manager& operator<<(long long int);
	pipe_manager& operator<<(unsigned long long int);
#endif
	pipe_manager& operator<<(long);
	pipe_manager& operator<<(unsigned long);
	pipe_manager& operator<<(int);
	pipe_manager& operator<<(unsigned int);
	pipe_manager& operator<<(short);
	pipe_manager& operator<<(unsigned short);
	pipe_manager& operator<<(char);
	pipe_manager& operator<<(unsigned char);

	char* c_str() const;
	size_t length() const;
	void clear();

private:
	std::list<pipe_stream*> m_streams;
	string* m_pBuf1, *m_pBuf2;
	pipe_string* m_pPipeStream;
};

} // namespace acl

