#pragma once

#ifdef MINGW
#include <sys/time.h>
#endif

class util
{
public:
	util() {}
	~util() {}

	static double stamp_sub(const struct timeval *from, const struct timeval *sub_by);
};
