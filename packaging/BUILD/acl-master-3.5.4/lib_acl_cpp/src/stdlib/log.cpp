#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#endif

namespace acl {

static bool m_bOpened = false;

void log::open(const char* recipients, const char* procname /* = unknown */,
	const char* cfg /* = NULL */)
{
	if (m_bOpened) {
		return;
	}

	acl_assert(recipients);
	acl_assert(procname);

	const char* ptr = strrchr(procname, '/');
	if (ptr) {
		procname = ptr + 1;
	}
#ifdef	ACL_WINDOWS
	if (ptr == NULL) {
		ptr = strrchr(procname, '\\');
		if (ptr) {
			procname = ptr + 1;
		}
	}
#endif

	acl_msg_open(recipients, procname);
	m_bOpened = true;
	if (cfg) {
		acl_debug_init(cfg);
	}
}

void log::close(void)
{
	if (m_bOpened) {
		acl_msg_close();
		acl_debug_end();
		m_bOpened = false;
	}
}

void log::debug_init(const char* cfg)
{
	if (cfg && *cfg) {
		acl_debug_init(cfg);
	}
}

void log::stdout_open(bool onoff)
{
	acl_msg_stdout_enable(onoff ? 1 : 0);
}

void log::msg1(const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vmsg2(fmt, ap);
	va_end(ap);
}

void log::vmsg2(const char* fmt, va_list ap)
{
	acl_msg_info2(fmt, ap);
}

void log::msg4(const char* fname, int line, const char* func,
	const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vmsg5(fname, line, func, fmt, ap);
	va_end(ap);
}

void log::vmsg5(const char* fname, int line, const char* func,
	const char* fmt, va_list ap)
{
	const char* ptr = strrchr(fname, '/');
	if (ptr) {
		fname = ptr + 1;
	}
#ifdef	ACL_WINDOWS
	if (ptr == NULL) {
		ptr = strrchr(fname, '\\');
		if (ptr) {
			fname = ptr + 1;
		}
	}
#endif
	acl::string s;
	s.format("%s(%d), %s: ", fname, line, func);
	s.vformat_append(fmt, ap);
	acl_msg_info("%s", s.c_str());
}

void log::msg3(size_t section, size_t level, const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vmsg4(section, level, fmt, ap);
	va_end(ap);
}

void log::vmsg4(size_t section, size_t level, const char* fmt, va_list ap)
{
	if (acl_do_debug((int) section, (int) level)) {
		vmsg2(fmt, ap);
	}
}

void log::msg6(size_t section, size_t level, const char* fname,
	int line, const char* func, const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vmsg7(section, level, fname, line, func, fmt, ap);
	va_end(ap);
}

void log::vmsg7(size_t section, size_t level, const char* fname,
	int line, const char* func, const char* fmt, va_list ap)
{
	if (!acl_do_debug((int) section, (int) level)) {
		return;
	}

	const char* ptr = strrchr(fname, '/');
	if (ptr) {
		fname = ptr + 1;
	}
#ifdef	ACL_WINDOWS
	if (ptr == NULL) {
		ptr = strrchr(fname, '\\');
		if (ptr) {
			fname = ptr + 1;
		}
	}
#endif

	acl::string s;
	s.format("%s(%d), %s: ", fname, line, func);
	s.vformat_append(fmt, ap);
	acl_msg_info("%s", s.c_str());
}

void log::warn1(const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vwarn2(fmt, ap);
	va_end(ap);
}

void log::vwarn2(const char* fmt, va_list ap)
{
	acl_msg_warn2(fmt, ap);
}

void log::warn4(const char* fname, int line, const char* func,
	const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vwarn5(fname, line, func, fmt, ap);
	va_end(ap);
}

void log::vwarn5(const char* fname, int line, const char* func,
	const char* fmt, va_list ap)
{
	const char* ptr = strrchr(fname, '/');
	if (ptr) {
		fname = ptr + 1;
	}
#ifdef	ACL_WINDOWS
	if (ptr == NULL) {
		ptr = strrchr(fname, '\\');
		if (ptr) {
			fname = ptr + 1;
		}
	}
#endif

	acl::string s;
	s.format("%s(%d), %s: ", fname, line, func);
	s.vformat_append(fmt, ap);
	acl_msg_warn("%s", s.c_str());
}

void log::error1(const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	verror2(fmt, ap);
	va_end(ap);
}

void log::verror2(const char* fmt, va_list ap)
{
	acl_msg_error2(fmt, ap);
}

void log::error4(const char* fname, int line, const char* func,
	const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	verror5(fname, line, func, fmt, ap);
	va_end(ap);
}

void log::verror5(const char* fname, int line, const char* func,
	const char* fmt, va_list ap)
{
	const char* ptr = strrchr(fname, '/');
	if (ptr) {
		fname = ptr + 1;
	}
#ifdef	ACL_WINDOWS
	if (ptr == NULL) {
		ptr = strrchr(fname, '\\');
		if (ptr) {
			fname = ptr + 1;
		}
	}
#endif

	acl::string s;
	s.format("%s(%d), %s: ", fname, line, func);
	s.vformat_append(fmt, ap);
	acl_msg_error("%s", s.c_str());
}

void log::fatal1(const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfatal2(fmt, ap);
	va_end(ap);
}

void log::vfatal2(const char* fmt, va_list ap)
{
	acl_msg_fatal2(fmt, ap);
}

void log::fatal4(const char* fname, int line, const char* func,
	const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfatal5(fname, line, func, fmt, ap);
	va_end(ap);
}

void log::vfatal5(const char* fname, int line, const char* func,
	const char* fmt, va_list ap)
{
	const char* ptr = strrchr(fname, '/');
	if (ptr) {
		fname = ptr + 1;
	}
#ifdef	ACL_WINDOWS
	if (ptr == NULL) {
		ptr = strrchr(fname, '\\');
		if (ptr) {
			fname = ptr + 1;
		}
	}
#endif

	acl::string s;
	s.format("%s(%d), %s: ", fname, line, func);
	s.vformat_append(fmt, ap);
	acl_msg_fatal("%s", s.c_str());
}

} // namespace acl
