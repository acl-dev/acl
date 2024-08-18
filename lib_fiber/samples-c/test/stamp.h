#ifndef __STAMP_INCLUDE_H__
#define __STAMP_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

static double stamp_sub(const struct timeval *from, const struct timeval *sub)
{
	struct timeval res;

	memcpy(&res, from, sizeof(struct timeval));

	res.tv_usec -= sub->tv_usec;
	if (res.tv_usec < 0) {
		--res.tv_sec;
		res.tv_usec += 1000000;
	}
	res.tv_sec -= sub->tv_sec;

	return (res.tv_sec * 1000.0 + res.tv_usec/1000.0);
}

#ifdef __cplusplus
}
#endif

#endif
