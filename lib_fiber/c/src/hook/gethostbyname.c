#include "stdafx.h"
#include "common.h"
#include "fiber.h"
#include "hook.h"

#ifdef SYS_UNIX

typedef struct hostent *(*gethostbyname_fn)(const char *);
static gethostbyname_fn __sys_gethostbyname = NULL;

#ifndef __APPLE__
typedef int (*gethostbyname_r_fn)(const char *, struct hostent *, char *,
	size_t, struct hostent **, int *);

static gethostbyname_r_fn __sys_gethostbyname_r = NULL;
#endif


static void hook_api(void)
{
	__sys_gethostbyname = (gethostbyname_fn) dlsym(RTLD_NEXT,
			"gethostbyname");
	assert(__sys_gethostbyname);

#ifndef __APPLE__
	__sys_gethostbyname_r = (gethostbyname_r_fn) dlsym(RTLD_NEXT,
			"gethostbyname_r");
	assert(__sys_gethostbyname_r);
#endif
}

static pthread_once_t __once_control = PTHREAD_ONCE_INIT;

static void hook_init(void)
{
	if (pthread_once(&__once_control, hook_api) != 0) {
		abort();
	}
}

/****************************************************************************/

static void free_fn(void *ctx)
{
	mem_free(ctx);
}

struct hostent *acl_fiber_gethostbyname(const char *name)
{
	struct hostent *result;
	static __thread struct hostent res;
#define BUF_LEN	4096
	static __thread char buf[BUF_LEN];

	char  *fiber_buf;
	static struct hostent *fiber_res;
	static __thread int  __fiber_buf_key;
	static __thread int  __fiber_res_key;

	if (!var_hook_sys_api) {
		return acl_fiber_gethostbyname_r(name, &res, buf, BUF_LEN,
				&result, &h_errno) == 0 ? result : NULL;
	}

	fiber_buf = (char *) acl_fiber_get_specific(__fiber_buf_key);
	if (fiber_buf == NULL) {
		fiber_buf = (char *) mem_malloc(BUF_LEN);
		acl_fiber_set_specific(&__fiber_buf_key, fiber_buf, free_fn);
	}
	assert(fiber_buf);

	fiber_res = (struct hostent *) acl_fiber_get_specific(__fiber_res_key);
	if (fiber_res == NULL) {
		fiber_res = (struct hostent *) mem_malloc(sizeof(struct hostent));
		acl_fiber_set_specific(&__fiber_res_key, fiber_res, free_fn);
	}
	assert(fiber_res);

	return acl_fiber_gethostbyname_r(name, fiber_res, fiber_buf, BUF_LEN,
			&result, &h_errno) == 0 ? result : NULL;
}

static struct addrinfo *get_addrinfo(const char *name)
{
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if (getaddrinfo(name, NULL, &hints, &res) != 0) {
		msg_error("%s(%d): getaddrinfo error", __FUNCTION__, __LINE__);
		return NULL;
	}

	return res;
}

#define MAX_COUNT	64

static int save_result(const char *name, struct hostent *ent,
	const struct addrinfo *res, char *buf, size_t buflen, size_t ncopied)
{
	const struct addrinfo *ai;
	size_t len, i;

	if (res->ai_canonname && *res->ai_canonname) {
		name = res->ai_canonname;
	}

	len = strlen(name) + 1;
	if (len >= buflen) {
		return 0;
	}

	SAFE_STRNCPY(buf, name, len);
	ent->h_name = buf;
	buf += len;

	for (ai = res, i = 0; ai != NULL; ai = ai->ai_next) {
		SOCK_ADDR *sa = (SOCK_ADDR *) ai->ai_addr;

		len = sizeof(struct in6_addr) > sizeof(struct in_addr) ?
			sizeof(struct in6_addr) : sizeof(struct in_addr);
		//len = sizeof(struct in_addr);
		ncopied += len;
		if (ncopied > buflen) {
			break;
		}

		if (ai->ai_family == AF_INET) {
			len = sizeof(struct in_addr);
			memcpy((void *) buf, &sa->in.sin_addr, len);
			ent->h_length = 4;
#ifdef	AF_INET6
		} else if (ai->ai_family == AF_INET6) {
			len = sizeof(struct in6_addr);
			memcpy((void *) buf, &sa->in6.sin6_addr, len);
			ent->h_length = 16;
#endif
		} else {
			continue;
		}

		if (i >= MAX_COUNT) {
			break;
		}

		ent->h_addrtype = ai->ai_family;
		ent->h_addr_list[i] = buf;
		buf                += len;
		i++;
	}

	return (int) i;
}

int acl_fiber_gethostbyname_r(const char *name, struct hostent *ent,
	char *buf, size_t buflen, struct hostent **result, int *h_errnop)
{
	size_t ncopied = 0, len, n;
	struct addrinfo *res;

#ifdef __APPLE__
	if (__sys_gethostbyname == NULL) {
#else
	if (__sys_gethostbyname_r == NULL) {
#endif
		hook_init();
	}

	if (!var_hook_sys_api) {
#ifdef __APPLE__
		*result = __sys_gethostbyname(name);
		if (result == NULL) {
			if (h_errnop) {
				*h_errnop = h_errno;
			}
			return -1;
		}
		return 0;
#else
		return __sys_gethostbyname_r ? __sys_gethostbyname_r
			(name, ent, buf, buflen, result, h_errnop) : -1;
#endif
	}

	memset(ent, 0, sizeof(struct hostent));
	memset(buf, 0, buflen);

	/********************************************************************/

	len = strlen(name);
	ncopied += len;
	if (ncopied + 1 >= buflen) {
		msg_error("%s(%d): n(%d) > buflen(%d)",
			__FUNCTION__, __LINE__, (int) ncopied, (int) buflen);

		if (h_errnop) {
			*h_errnop = ERANGE;
		}
		return -1;
	}

	/********************************************************************/

	if ((res = get_addrinfo(name)) == NULL) {
		if (h_errnop) {
			*h_errnop = NO_DATA;
		}
		return -1;
	}

	/********************************************************************/

	len = 8 * MAX_COUNT;
	ncopied += len;
	if (ncopied >= buflen) {
		msg_error("%s(%d): n(%d) > buflen(%d)",
			__FUNCTION__, __LINE__, (int) ncopied, (int) buflen);
		if (h_errnop) {
			*h_errnop = ERANGE;
		}
		return -1;
	}

	ent->h_addr_list = (char**) buf;
	buf += len;

	n = save_result(name, ent, res, buf, buflen, ncopied);

	freeaddrinfo(res);

	if (n > 0) {
		*result = ent;
		return 0;
	}

	msg_error("%s(%d), %s: i == 0, ncopied: %d, buflen: %d",
		__FILE__, __LINE__, __FUNCTION__, (int) ncopied, (int) buflen);

	if (h_errnop) {
		*h_errnop = ERANGE;
	}

	return -1;
}


struct hostent *gethostbyname(const char *name)
{
	return acl_fiber_gethostbyname(name);
}

#ifdef __APPLE__
extern int gethostbyname_r(const char *name, struct hostent *ent,
	char *buf, size_t buflen, struct hostent **result, int *h_errnop);
#endif

int gethostbyname_r(const char *name, struct hostent *ent,
	char *buf, size_t buflen, struct hostent **result, int *h_errnop)
{
	return acl_fiber_gethostbyname_r(name, ent, buf, buflen,
			result, h_errnop);
}

#endif /* SYS_UNIX */
