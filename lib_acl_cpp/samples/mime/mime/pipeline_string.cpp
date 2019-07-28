#include "stdafx.h"
#include "pipeline_string.h"

using namespace acl;

pipeline_string::pipeline_string()
: strip_sp_(false)
{

}

pipeline_string::~pipeline_string()
{

}

void pipeline_string::clear()
{
	strip_sp_ = false;
}

int pipeline_string::push_pop(const char* in, size_t len,
	string* out, size_t /* = 0 */)
{
	int   n = 0;

	// 有些邮件系统在组包时可能会因添加回车而把
	// 字符集弄乱了，所以此处尝试去年回车与行首
	// 空格，此纠错方式仅针对 HTML 格式的正文内容
	while (len > 0)
	{
		if ((*in) == '\r' || (*in) == '\n')
		{
			strip_sp_ = true;
			in++;
			len--;
			while (len > 0 && (*in == '\r' || *in == '\n'))
			{
				in++;
				len--;
			}
		}
		else if (strip_sp_)
		{
			strip_sp_ = false;
			while (len > 0 && (*in == ' ' || *in == '\t'))
			{
				in++;
				len--;
			}
		}
		else
		{
			(*out) << *((const unsigned char*) in);
			in++;
			len--;
			n++;
		}

	}

	return n;
}

int pipeline_string::pop_end(string* out, size_t max /* = 0 */)
{
	(void) out;
	(void) max;
	return 0;
}
