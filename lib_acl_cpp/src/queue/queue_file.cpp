#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include <assert.h>
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/thread.hpp"
#include "acl_cpp/stream/fstream.hpp"
#include "acl_cpp/queue/queue_manager.hpp"
#include "acl_cpp/queue/queue_file.hpp"
#endif

#ifdef ACL_WINDOWS
#include <process.h>
#define getpid _getpid
#endif

#ifdef ACL_WINDOWS
#define PATH_SEP	'\\'
#else
#define PATH_SEP	'/'
#endif

//static __thread unsigned int __counter = 0;
static unsigned int __counter = 0;

namespace acl
{

queue_file::queue_file()
: m_fp(NULL)
, m_locker(true)
, m_bLocked(false)
, m_bLockerOpened(false)
, nwriten_(0)
{

}

queue_file::~queue_file()
{
	this->close();
}

bool queue_file::create(const char* home, const char* queueName,
	const char* extName, unsigned width)
{
	acl_assert(width > 0);

	struct timeval tv;
	acl::string buf;
	acl::fstream* fp = NULL;
	int   i = 0;
	bool  dir_exist;

	ACL_SAFE_STRNCPY(m_home, home, sizeof(m_home));
	ACL_SAFE_STRNCPY(m_queueName, queueName, sizeof(m_queueName));
	ACL_SAFE_STRNCPY(m_extName, extName, sizeof(m_extName));

	unsigned int n;

	while (1)
	{
		// 产生部分文件名
		memset(&tv, 0, sizeof(tv));
		gettimeofday(&tv, NULL);
		safe_snprintf(m_partName, sizeof(m_partName),
			"%u_%lu_%08x_%08x_%u",
			(unsigned int) getpid(),
			(unsigned long) acl::thread::thread_self(),
			(unsigned int) tv.tv_sec,
			(unsigned int) tv.tv_usec,
			(unsigned int) __counter);
		if (__counter++ >= 1024000)
			__counter = 0;

		// 计算队列子目录
		n = queue_manager::hash_queueSub(m_partName, width);
		safe_snprintf(m_queueSub, sizeof(m_queueSub), "%u", n);

		buf.clear();
		buf << m_home << PATH_SEP << m_queueName << PATH_SEP << m_queueSub
			<< PATH_SEP << m_partName << "." << extName;

		fp = NEW fstream;

		dir_exist = false;

		while (true)
		{
			// 排它性创建唯一文件
			if (fp->open(buf.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600) == true)
				goto END;

			logger_warn("open file %s error(%s)", buf.c_str(), acl_last_serror());

			if (acl_last_error() != ENOENT || dir_exist)
				break;

			// 尝试性创建目录
			buf.clear();
			buf << m_home << PATH_SEP << m_queueName << PATH_SEP << m_queueSub;
			if (acl_make_dirs(buf.c_str(), 0700) == -1)
			{
				logger_error("mkdir: %s error(%s)",
					buf.c_str(), acl_last_serror());
				delete fp;
				return false;
			}
			else
				logger("create path: %s ok", buf.c_str());

			dir_exist = true;
			buf.clear();
			buf << m_home << PATH_SEP << m_queueName << PATH_SEP << m_queueSub
				<< PATH_SEP << m_partName << "." << extName;

		}
		delete fp;
		sleep(1);
		if (i++ >= 10) {
			logger_error("can't create file, loop 10 times for (%s)",
				buf.c_str());
			return false;
		}
	}

END:
	m_fp = fp;
	m_filePath = buf.c_str();

	// 打开队列文件对象锁
	if (m_locker.open(m_fp->file_handle()) == false)
	{
		logger_error("open lock for %s error(%s)",
			m_filePath.c_str(), acl_last_serror());
		m_bLockerOpened = false;
	}
	else
		m_bLockerOpened = true;

	return true;
}

bool queue_file::open(const char* filePath)
{
	string home, queueName, queueSub, partName, extName;
	if (queue_manager::parse_filePath(filePath, &home, &queueName, &queueSub,
		&partName, &extName) == false)
	{
		logger_error("filePath(%s) invalid", filePath);
		return false;
	}

	return open(home.c_str(), queueName.c_str(), queueSub.c_str(),
		partName.c_str(), extName.c_str());
}

bool queue_file::open(const char* home, const char* queueName,
	const char* queueSub, const char* partName, const char* extName)
{
	if (m_fp)
		logger_fatal("old file(%s) exist", m_filePath.c_str());

	m_filePath.clear();
	m_filePath << home << PATH_SEP << queueName << PATH_SEP << queueSub
		<< PATH_SEP << partName << "." << extName;

	m_fp = NEW fstream;

	if (m_fp->open(m_filePath.c_str(), O_RDWR, 0600) == false)
	{
		logger_error("open %s error(%s)", m_filePath.c_str(),
			acl_last_serror());
		delete m_fp;
		m_fp = NULL;
		return false;
	}

	if (m_home != home)
		ACL_SAFE_STRNCPY(m_home, home, sizeof(m_home));
	if (m_queueName != queueName)
		ACL_SAFE_STRNCPY(m_queueName, queueName, sizeof(m_queueName));
	if (m_queueSub != queueSub)
		ACL_SAFE_STRNCPY(m_queueSub, queueSub, sizeof(m_queueSub));
	if (m_partName != partName)
		ACL_SAFE_STRNCPY(m_partName, partName, sizeof(m_partName));
	if (m_extName != extName)
		ACL_SAFE_STRNCPY(m_extName, extName, sizeof(m_extName));

	// 打开队列文件对象锁
	if (m_locker.open(m_fp->file_handle()) == false)
	{
		logger_error("open lock for %s error(%s)",
			m_filePath.c_str(), acl_last_serror());
		m_bLockerOpened = false;
	}
	else
	{
		m_bLockerOpened = true;
	}

	// XXX, 需要分析该全路径, 以提取需要的字段
	return true;
}

void queue_file::close()
{
	if (m_fp)
	{
		delete m_fp;
		m_fp = NULL;
		nwriten_ = 0;
	}
}

acl::fstream* queue_file::get_fstream() const
{
	return m_fp;
}

time_t queue_file::get_ctime() const
{
	if (m_fp == NULL)
	{
		logger_error("m_fp null");
		return (time_t) -1;
	}
	else if (m_filePath.empty())
	{
		logger_error("m_filePath empty");
		return (time_t) -1;
	}

	struct acl_stat buf;
	if (acl_stat(m_filePath.c_str(), &buf) == -1)
	{
		logger_error("stat file(%s) error(%s)",
			m_filePath.c_str(), acl_last_serror());
		return (time_t) -1;
	}
	return buf.st_ctime;
}

bool queue_file::write(const void* data, size_t len)
{
	if (data == NULL || len == 0 || len >= (unsigned int) -1)
	{
		logger_error("input invalid");
		return false;
	}
	if (m_fp == NULL)
	{
		logger_error("m_fp null");
		return false;
	}
	if (m_fp->write(data, len) != (int) len)
	{
		logger_error("write error");
		return false;
	}

	nwriten_ += len;
	return true;
}

int queue_file::format(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int ret = vformat(fmt, ap);
	va_end(ap);
	return ret;
}

int queue_file::vformat(const char* fmt, va_list ap)
{
	int ret = m_fp->vformat(fmt, ap);
	if (ret == -1)
	{
		logger_error("write to file error(%s)", last_serror());
		return -1;
	}

	nwriten_ += ret;
	return ret;
}
int queue_file::read(void* buf, size_t len)
{
	if (buf == NULL || len == 0 || len >= (unsigned int) -1)
	{
		logger_error("input invalid");
		return -1;
	}
	if (m_fp == NULL)
	{
		logger_error("m_fp null");
		return -1;
	}
	int   ret;
	if ((ret = m_fp->read(buf, len, false)) < 0)
		return -1;
	return ret;
}

bool queue_file::remove()
{
	this->close();
#ifdef ACL_WINDOWS
	if (_unlink(m_filePath.c_str()) != 0)
#else
	if (unlink(m_filePath.c_str()) != 0)
#endif
	{
		logger_error("unlink %s error(%s)",
			m_filePath.c_str(), acl_last_serror());
		return false;
	}
	return true;
}

bool queue_file::move_file(const char* queueName, const char* extName)
{
	acl::string buf(256);
	bool once_again = false;

	while (true)
	{
		buf.clear();
		buf << m_home << PATH_SEP << queueName << PATH_SEP << m_queueSub
			<< PATH_SEP << m_partName << "." << extName;

#ifdef ACL_WINDOWS
		// 在win32下必须先关闭文件句柄
		this->close();
#endif

		if (rename(m_filePath.c_str(), buf.c_str()) == 0)
			break;

		// 如果返回错误原因是目标路径不存在，则尝试创建目录结构

		if (once_again || acl_last_error() != ENOENT)
		{
			logger_error("move from %s to %s error(%s), errno: %d, %d",
				m_filePath.c_str(), buf.c_str(), acl_last_serror(),
				acl_last_error(), ENOENT);
			return false;
		}

		// 设置重试标志位
		once_again = true;

		buf.clear();
		buf << m_home << PATH_SEP << queueName
			<< PATH_SEP << m_queueSub;

		// 创建队列目录
		if (acl_make_dirs(buf.c_str(), 0700) == -1)
		{
			logger_error("mkdir: %s error(%s)",
				buf.c_str(), acl_last_serror());
			return false;
		}
	}

#ifdef ACL_WINDOWS
	// win32 下需要重新再打开
	return open(m_home, queueName, m_queueSub, m_partName, extName);
#else
	if (m_queueName != queueName)
		ACL_SAFE_STRNCPY(m_queueName, queueName, sizeof(m_queueName));
	if (m_extName != extName)
		ACL_SAFE_STRNCPY(m_extName, extName, sizeof(m_extName));
	m_filePath.clear();
	m_filePath << m_home << PATH_SEP << m_queueName << PATH_SEP
		<< m_queueSub << PATH_SEP << m_partName << "." << m_extName;
#endif
	return true;
}

void queue_file::set_queueName(const char* queueName)
{
	ACL_SAFE_STRNCPY(m_queueName, queueName, sizeof(m_queueName));
	m_filePath.clear();
	m_filePath << m_home << PATH_SEP << m_queueName << PATH_SEP
		<< m_queueSub << PATH_SEP << m_partName << "." << m_extName;
}

void queue_file::set_extName(const char* extName)
{
	ACL_SAFE_STRNCPY(m_extName, extName, sizeof(m_extName));
	m_filePath.clear();
	m_filePath << m_home << PATH_SEP << m_queueName << PATH_SEP
		<< m_queueSub << PATH_SEP << m_partName << "." << m_extName;
}

bool queue_file::lock()
{
	if (m_bLockerOpened == false)
		return false;
	if (m_locker.try_lock() == false)
		return false;
	return true;
}

bool queue_file::unlock()
{
	if (m_bLockerOpened == false)
		return false;
	if (m_locker.unlock() == false)
		return false;
	return true;
}

} // namespace acl
