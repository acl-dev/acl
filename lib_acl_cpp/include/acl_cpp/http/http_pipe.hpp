#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/pipe_stream.hpp"

namespace acl {

class charset_conv;
class pipe_stream;

class ACL_CPP_API http_pipe
{
public:
	http_pipe(void);
	virtual ~http_pipe(void);
	void set_charset(charset_conv* conv);
	bool set_charset(const char* from, const char* to);
	void append(pipe_stream* ps);
	void reset();
	bool update(const char* in, size_t len);
	bool update_end(void);

	pipe_manager& get_manager(void);
protected:
private:
	pipe_manager manager_;
	charset_conv* conv_;
};

} // namespace acl
