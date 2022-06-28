#pragma once

class util
{
public:
	util() {}
	~util() {}

	static double stamp_sub(const struct timeval *from, const struct timeval *sub_by);
};
