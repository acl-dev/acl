#include "acl_stdafx.hpp"
#include "internal/tok822.hpp"
#ifndef ACL_PREPARE_COMPILE
#include <string.h>
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/mime/rfc2047.hpp"
#include "acl_cpp/mime/rfc822.hpp"
#endif

#if !defined(ACL_MIME_DISABLE)

namespace acl {

#define	STR	acl_vstring_str
#define	LEN	ACL_VSTRING_LEN

static const char * const months[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
	NULL
};

static const char * const wdays[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static unsigned parsedig(const char **p)
{
	unsigned i = 0;

	while (isdigit((int) (unsigned char) **p)) {
		i = i * 10 + **p - '0';
		++*p;
	}
	return i;
}

#define	leap(y)	(((y) % 400) == 0 || (((y) % 4) == 0 && (y) % 100) )

static const unsigned mlength[] =
{
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

#define	mdays(m,y)	( (m) != 2 ? mlength[(m) - 1] : leap(y) ? 29 : 28)

static const char *const zonenames[] =
{
	"UT", "GMT",
	"EST", "EDT",
	"CST", "CDT",
	"MST", "MDT",
	"PST", "PDT",
	"Z",
	"A", "B", "C", "D", "E", "F", "G", "H", "I", "K", "L", "M", 
	"N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y",
	NULL
};

#define	ZH(n)	( (n) * 60 * 60 )

static const int zoneoffset[] =
{
	0, 0,
	ZH(-5), ZH(-4),
	ZH(+8), ZH(-5),
	ZH(-7), ZH(-6),
	ZH(-8), ZH(-7),
	0,

	ZH(-1), ZH(-2), ZH(-3), ZH(-4), ZH(-5), ZH(-6),
	ZH(-7), ZH(-8), ZH(-9), ZH(-10), ZH(-11), ZH(-12),
	ZH(1), ZH(2), ZH(3), ZH(4), ZH(5), ZH(6), ZH(7),
	ZH(8), ZH(9), ZH(10), ZH(11), ZH(12)
};

static unsigned parsekey(const char **mon, const char * const *ary)
{
	unsigned m = 0, j = 0;

	for (m = 0; ary[m]; m++) {
		for (j = 0; ary[m][j]; j++) {
			if (tolower(ary[m][j]) != tolower((*mon)[j])) {
				break;
			}
		}
		if (!ary[m][j]) {
			*mon += j;
			return m + 1;
		}
	}

	return 0;
}

static int parsetime(const char **t)
{
	unsigned h = 0, m = 0, s = 0;

	if (!isdigit((int)(unsigned char) **t)) {
		return -1;
	}
	h = parsedig(t);
	if (h > 23) {
		return -1;
	}
	if (**t != ':') {
		return -1;
	}
	++*t;
	if (!isdigit((int) (unsigned char) **t)) {
		return -1;
	}
	m = parsedig(t);
	if (**t == ':') {
		++*t;
		if (!isdigit((int) (unsigned char) **t)) {
			return -1;
		}
		s = parsedig(t);
	}
	if (m > 59 || s > 59) {
		return -1;
	}
	return h * 60 * 60 + m * 60 + s;
}

rfc822::rfc822(void)
{
}

rfc822::~rfc822(void)
{
	reset();
}

time_t rfc822::parse_date(const char *rfcdt)
{
	unsigned day = 0, mon = 0, year = 0;
	int secs   = 0;
	int offset = 0;
	time_t t   = 0;
	unsigned y = 0;
	int sign   = 1;
	unsigned n = 0;

	/* Ignore day of the week.  Tolerate "Tue, 25 Feb 1997 ... "
	 ** without the comma.  Tolerate "Feb 25 1997 ...".
	 */

	while (!day || !mon) {
		if (!*rfcdt) {
			return 0;
		}
		if (isalpha((int)(unsigned char)*rfcdt)) {
			if (mon) {
				return 0;
			}
			mon = parsekey(&rfcdt, months);
			if (mon) {
				continue;
			}

			while (*rfcdt && isalpha((int)(unsigned char)*rfcdt)) {
				++rfcdt;
			}
			continue;
		}

		if (isdigit((int)(unsigned char)*rfcdt)) {
			if (day) {
				return 0;
			}
			day = parsedig(&rfcdt);
			if (!day) {
				return 0;
			}
			continue;
		}
		++rfcdt;
	}

	while (*rfcdt && isspace((int)(unsigned char)*rfcdt)) {
		++rfcdt;
	}
	if (!isdigit((int)(unsigned char)*rfcdt)) {
		return 0;
	}
	year = parsedig(&rfcdt);
	if (year < 70) {
		year += 2000;
	}
	if (year < 100) {
		year += 1900;
	}

	while (*rfcdt && isspace((int)(unsigned char)*rfcdt)) {
		++rfcdt;
	}

	if (day == 0 || mon == 0 || mon > 12 || day > mdays(mon, year)) {
		return 0;
	}

	secs = parsetime(&rfcdt);
	if (secs < 0) {
		return 0;
	}

	offset = 0;

	/* RFC822 sez no parenthesis, but I've seen (EST) */

	while (*rfcdt) {
		if (isalnum((int)(unsigned char)*rfcdt) || *rfcdt == '+' ||
			*rfcdt == '-') {

			break;
		}
		++rfcdt;
	}

	if (isalpha((int)(unsigned char)*rfcdt)) {
		n = parsekey(&rfcdt, zonenames);
		if (n > 0) {
			offset = zoneoffset[n - 1];
		}
	} else {
		switch (*rfcdt) {
		case '-':
			sign = -1;
			++rfcdt;
			break;
		case '+':
		default:
			++rfcdt;
			break;
		}

		if (isdigit((int)(unsigned char)*rfcdt)) {
			n = parsedig(&rfcdt);
			if (n > 2359 || (n % 100) > 59) {
				n = 0;
			}
			offset = sign * ( (n % 100) * 60 + n / 100 * 60 * 60);
		}
	}

	if (year < 1970) {
		printf("%s(%d)\n", __FUNCTION__, __LINE__);
		return 0;
	}

	t = 0;
	for (y = 1970; y < year; y++) {
		if (leap(y)) {
			if (year - y >= 4) {
				y += 3;
				t += ( 365 * 3 + 366 ) * 24 * 60 * 60;
				continue;
			}
			t += 24 * 60 * 60;
		}
		t += 365 * 24 * 60 * 60;
	}

	for (y = 1; y < mon; y++) {
		t += mdays(y, year) * 24 * 60 * 60;
	}

	return t + (day - 1) * 24 * 60 * 60 + secs - offset;
}

void rfc822::mkdate(time_t t, char* buf, size_t size, tzone_t zone)
{
	if (zone == tzone_cst) {
		mkdate_cst(t, buf, size);
	} else {
		mkdate_gmt(t, buf, size);
	}
}

void rfc822::mkdate_gmt(time_t t, char *buf, size_t size)
{
	struct tm *p;

#ifdef	ACL_WINDOWS
# if _MSC_VER >= 1500
	struct tm gmt_buf;
	p = &gmt_buf;
	if (gmtime_s(p, &t) != 0) {
		p = NULL;
	}
# else
	p = gmtime(&t);
# endif
#else
	struct tm tm_buf;
	p = gmtime_r(&t, &tm_buf);
#endif

	safe_snprintf(buf, size, "%s, %02d %s %04d %02d:%02d:%02d (GMT)",
		wdays[p->tm_wday],
		p->tm_mday,
		months[p->tm_mon],
		p->tm_year + 1900,
		p->tm_hour,
		p->tm_min,
		p->tm_sec);
}

#define USE_TIME_DAYLIGHT	1

void rfc822::mkdate_cst(time_t t, char *buf, size_t size)
{
	struct tm *p;
	long offset = 0;

#ifdef	ACL_WINDOWS
# if _MSC_VER >= 1500
	struct tm tm_buf;
	long s;
	p = &tm_buf;
	if (localtime_s(p, &t) != 0) {
		p = NULL;
	}
# else
	p = localtime(&t);
# endif
#else
	struct tm tm_buf;
	p = localtime_r(&t, &tm_buf);
#endif

	buf[0] = 0;
	if (p == NULL) {
		return;
	}

#if	USE_TIME_ALTZONE

	offset = -_timezone;

	if (p->tm_isdst > 0) {
		offset = -altzone;
	}

	if (offset % 60) {
		offset = 0;
#ifdef	ACL_WINDOWS
# if _MSC_VER >= 1500
		p = &tm_buf;
		if (gmtime_s(p, &t) != 0) {
			p = NULL;
		}
# else
		p = gmtime(&t);
# endif
#else
		p = gmtime_r(&t, &tm_buf);
#endif
	}
	offset /= 60;
#else
#if	USE_TIME_DAYLIGHT

#ifdef ACL_WINDOWS
# if _MSC_VER >= 1500
	if ( _get_timezone(&s) != 0) {
		s = 0;
	}
	offset =- s;
# else
	offset = - _timezone;
# endif
#elif !defined(ACL_FREEBSD) && !defined(MINGW)  // XXX -zsx
	offset = - timezone;
#endif

	if (p == NULL) {
		return;
	}

	if (p->tm_isdst > 0) {
		offset += 60 * 60;
	}
	if (offset % 60) {
		offset = 0;
#ifdef	ACL_WINDOWS
# if _MSC_VER >= 1500
		p = &tm_buf;
		if (gmtime_s(p, &t) != 0) {
			p = NULL;
		}
# else
		p = gmtime(&t);
# endif
#else
		p = gmtime_r(&t, &tm_buf);
#endif
	}
	offset /= 60;
#else
#if	USE_TIME_GMTOFF
	offset = p->tm_gmtoff;

	if (offset % 60) {
		offset = 0;
#ifdef	ACL_WINDOWS
# if _MSC_VER >= 1500
		p = &tm_buf;
		if (gmtime_s(p, &t) != 0) {
			p = NULL;
		}
# else
		p = gmtime(&t);
# endif
#else
		p = gmtime_r(&t, &tm_buf);
#endif
	}
	offset /= 60;
#else
#ifdef	ACL_WINDOWS
# if _MSC_VER >= 1500
	p = &tm_buf;
	if (gmtime_s(p, &t) != 0) {
		p = NULL;
	}
# else
	p = gmtime(&t);
# endif
#else
	p = gmtime_r(&t, &tm_buf);
#endif
	offset = 0;
#endif
#endif
#endif

	offset = (offset % 60) + offset / 60 * 100;

#if defined(ACL_WINDOWS) && _MSC_VER >= 1500
	_snprintf_s(buf, size, size, "%s, %02d %s %04d %02d:%02d:%02d %+05d (CST)",
		wdays[p->tm_wday],
		p->tm_mday,
		months[p->tm_mon],
		p->tm_year + 1900,
		p->tm_hour,
		p->tm_min,
		p->tm_sec,
		offset);
#else
	safe_snprintf(buf, size, "%s, %02d %s %04d %02d:%02d:%02d %+05ld (CST)",
		wdays[p->tm_wday],
		p->tm_mday,
		months[p->tm_mon],
		p->tm_year + 1900,
		p->tm_hour,
		p->tm_min,
		p->tm_sec,
		offset);
#endif
}

const std::list<rfc822_addr*>& rfc822::parse_addrs(const char* in,
	const char* to_charset /* = "utf-8" */)
{
	reset();

	if (to_charset == NULL) {
		to_charset = "gb18030";
	}

	if (in == NULL || *in == 0) {
		logger_error("input invalid");
		return addrs_;
	}
	TOK822 *tree = tok822_parse(in);
	if (tree == NULL) {
		logger_error("tok822_parse(%s) error", in);
		return addrs_;
	}

	const ACL_VSTRING* comment_prev = NULL;
	string buf;

	for (TOK822 *tp = tree; tp; tp = tp->next) {
		if (tp->type == TOK822_ATOM
			|| tp->type == TOK822_COMMENT
			|| tp->type == TOK822_QSTRING
			|| tp->vstr != NULL) {

			comment_prev = tp->vstr;
		}

		if (tp->type != TOK822_ADDR || tp->head == NULL) {
			continue;
		}

		ACL_VSTRING* addrp = acl_vstring_alloc(32);
		(void) tok822_internalize(addrp, tp->head, TOK822_STR_DEFL);
		rfc822_addr* addr = (rfc822_addr*)
			acl_mymalloc(sizeof(rfc822_addr));
		addr->addr = acl_vstring_export(addrp);
		if (comment_prev) {
			buf.clear();
			rfc2047::decode(STR(comment_prev),
				(int) LEN(comment_prev), &buf, to_charset);
			addr->comment = acl_mystrdup(buf.c_str());
			comment_prev = NULL;
		} else {
			addr->comment = NULL;
		}
		addrs_.push_back(addr);
	}

	tok822_free_tree(tree);
	return addrs_;
}

const rfc822_addr* rfc822::parse_addr(const char* in,
	const char* to_charset /* = "utf-8" */)
{
	const std::list<rfc822_addr*> addr_list = parse_addrs(in, to_charset);
	if (addr_list.empty()) {
		return NULL;
	}
	std::list<rfc822_addr*>::const_iterator cit = addr_list.begin();
	acl_assert(cit != addr_list.end());
	return *cit;
}

bool rfc822::check_addr(const char* in)
{
#define	VALID1(x) (((x) >= '0' && (x) <= '9')  \
	|| ((x) >= 'a' && (x) <= 'z')  \
	|| ((x) >= 'A' && (x) <= 'Z'))

#define	VALID2(x) (((x) >= '0' && (x) <= '9')  \
	|| ((x) >= 'a' && (x) <= 'z')  \
	|| ((x) >= 'A' && (x) <= 'Z')  \
	|| (x) == '-'  \
	|| (x) == '_'  \
	|| (x) == '.')

	while (*in == ' ' || *in == '\t') {
		in++;
	}
	if (*in == ';' || *in == ',') {
		return false;
	}

	const rfc822_addr* addr = parse_addr(in);
	if (addr == NULL || addr->addr == NULL) {
		return false;
	}
	const char* at = addr->addr;
	//printf(">>%s, %s\r\n", addr->comment ? addr->comment : "null", addr->addr);
	if (!VALID1(*at)) {
		return false;
	}
	at++;

	while (*at) {
		if (*at == '@') {
			// 必须保证 @ 前一个字符的有效性遵守 VALID1
			if (!VALID1(*(at - 1))) {
				return false;
			}
			break;
		}
		if (!VALID2(*at)) {
			return false;
		}
		at++;
	}
	if (*at != '@') {
		return false;
	}
	at++;

	int  dot = 0;
	bool first = true;
	while (*at) {
		if (first) {
			// at: [a-z]|[A-Z]|[0-9]
			if (!VALID1(*at)) {
				return false;
			}
			first = false;
		} else if (*at == '.') {
			dot++;
			first = true;
		} else if (!VALID2(*at)) {
			return false;
		}
		at++;
	}

	if (!VALID1(*(at - 1)) || dot == 0) {
		return false;
	}
	return true;
}

void rfc822::reset(void)
{
	std::list<rfc822_addr*>::iterator it = addrs_.begin();
	for (; it != addrs_.end(); ++it) {
		acl_myfree((*it)->addr);
		if ((*it)->comment) {
			acl_myfree((*it)->comment);
		}
		acl_myfree(*it);
	}

	addrs_.clear();
}

}  // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
