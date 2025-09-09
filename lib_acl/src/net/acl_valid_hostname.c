/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <string.h>
#include <ctype.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_argv.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_stringops.h"
#include "net/acl_valid_hostname.h"

#endif

/* acl_valid_hostname - screen out bad hostnames */

static const char *check_hostname(const char *name) {
	const char *cp;
	int     label_length = 0;
	int     non_numeric = 0;
	int     ch;

	/*
	 * Trivial cases first.
	 */
	if (*name == 0) {
		return "empty hostname";
	}

	/*
	 * Find bad characters or label lengths. Find adjacent delimiters.
	 */
	for (cp = name; (ch = *(const unsigned char *) cp) != 0; cp++) {
		if (ACL_ISALNUM(ch) || ch == '_') {		/* grr.. */
			label_length++;
			if (label_length > ACL_VALID_LABEL_LEN) {
				return "hostname label too long";
			}
			if (!ACL_ISDIGIT(ch)) {
				non_numeric = 1;
			}
		} else if (ch == '.') {
			if (label_length == 0 || cp[1] == 0) {
				return "misplaced delimiter";
			}
			label_length = 0;
		} else if (ch == '-') {
			label_length++;
			if (label_length == 1 || cp[1] == 0 || cp[1] == '.') {
				return "misplaced hyphen";
			}
		}
#ifdef SLOPPY_VALID_HOSTNAME
		else if (ch == ':' && acl_valid_ipv6_hostaddr(name, ACL_DONT_GRIPE)) {
			non_numeric = 0;
			break;
		}
#endif
		else {
			return "invalid character";
		}
	}

	if (non_numeric == 0) {
#ifndef SLOPPY_VALID_HOSTNAME
		return "numeric hostname";
#endif
	}
	if (cp - name > ACL_VALID_HOSTNAME_LEN) {
		return "bad length for";
	}
	return NULL;
}

int acl_valid_hostname(const char *name, int gripe) {
	const char *err = check_hostname(name);
	if (err == NULL) {
		return 1;
	}
	if (gripe) {
		acl_msg_warn("%s: %s, name=%s", __FUNCTION__, err,
			name ? name : "(null)");
	}
	return 0;
}

int acl_valid_unix(const char *addr) {
	size_t nslash = 0, nchars = 0;
	char lastch = 0;

	if (addr == NULL || strlen(addr) < 2) {
		return 0;
	}

#ifdef ACL_LINUX
	/* We use '@' as the first char in path for abstract unix for Linux */
	if (*addr == '@') {
		return 1;
	}
#endif
	while (*addr != 0) {
		if (*addr == '/') {
			nslash++;
		} else {
			nchars++;
		}
		lastch = *addr++;
	}

	if (nslash == 0 || nchars == 0 || lastch == 0 || lastch == '/') {
		return 0;
	}
	return 1;
}

/* acl_valid_hostaddr - verify numerical address syntax */

static int valid_ipv4_field(const char *ptr) {
	int n;

	if (*ptr == '*' && *(ptr + 1) == 0) {
		return 1;
	}
	n = acl_safe_atoi(ptr, -1);
	return n >= 0 && n <= 255 ? 1 : 0;
}

static int valid_port(const char *ptr) {
	int n;

	if (!acl_alldig(ptr)) {
		return 0;
	}

	n = acl_safe_atoi(ptr, -1);
	return n >= 0 && n <= 65535;
}

static int valid_ipv4_wildcard(const char *addr) {
	ACL_ARGV_VIEW *tokens = acl_argv_view_split(addr, ".");
	char     *ptr, *s4;
	int       i;

	if (tokens->argv.argc != 4) {
		acl_argv_view_free(tokens);
		return 0;
	}

	for (i = 0; i < 3; i++) {
		ptr = tokens->argv.argv[i];
		if (!valid_ipv4_field(ptr)) {
			acl_argv_view_free(tokens);
			return 0;
		}
	}

	s4 = tokens->argv.argv[3];
	ptr = strchr(s4, ACL_ADDR_SEP);
	if (ptr == NULL) {
		ptr = strchr(s4, ':');
	}
	if (ptr) {
		*ptr++ = 0;
	}

	if (!valid_ipv4_field(s4)) {
		acl_argv_view_free(tokens);
		return 0;
	}

	if (ptr) {
		i = valid_port(ptr);
	} else {
		i = 1;
	}
	acl_argv_view_free(tokens);
	return i;
}

int acl_valid_hostaddr(const char *addr, int gripe) {
	return acl_check_hostaddr(addr, gripe) == ACL_HOSTADDR_TYPE_NONE ? 0 : 1;
}

int acl_check_hostaddr(const char *addr, int gripe) {
	int n;

	/* Trivial cases first. */
	if (addr == NULL || *addr == 0) {
		if (gripe) {
			acl_msg_warn("%s: address null", __FUNCTION__);
		}
		return ACL_HOSTADDR_TYPE_NONE;
	}

#define VALID_PORT(x) ((n = acl_safe_atoi((x), -1) >= 0) && n <= 65535)
#define VALID_SEP(x)  ((x) == ACL_ADDR_SEP || (x) == ':')

	/* port */
	if (VALID_PORT(addr)) {
		return ACL_HOSTADDR_TYPE_WILDCARD;
	}

	/* '|port', ':port' */
	if (VALID_SEP(*addr) && VALID_PORT(addr + 1)) {
		return ACL_HOSTADDR_TYPE_WILDCARD;
	}

	/* '*|port', '*:port' */
	if (*addr == '*' && VALID_SEP(*(addr + 1)) && VALID_PORT(addr + 2)) {
		return ACL_HOSTADDR_TYPE_WILDCARD;
	}

	/* '*.*.*.1|port' */
	if (valid_ipv4_wildcard(addr)) {
		return ACL_HOSTADDR_TYPE_WILDCARD;
	}

	if (*addr == '[') {
		if (acl_valid_ipv6_hostaddr(addr, gripe)) {
			return ACL_HOSTADDR_TYPE_IPV6;
		}
		return ACL_HOSTADDR_TYPE_NONE;
	}

	/* Protocol-dependent processing next. */
	if (acl_valid_ipv4_hostaddr(addr, gripe)) {
		return ACL_HOSTADDR_TYPE_IPV4;
	}

	if (acl_valid_ipv6_hostaddr(addr, gripe)) {
		return ACL_HOSTADDR_TYPE_IPV6;
	}
	return ACL_HOSTADDR_TYPE_NONE;
}

/* acl_valid_ipv4_hostaddr - test dotted quad string for correctness */

static const char *check_ipv4(const char *addr, const char *end) {
	int   in_byte = 0, byte_count = 0, byte_val = 0;
	const char *cp;

#define BYTES_NEEDED	4

	/*
	 * Scary code to avoid sscanf() overflow nasties.
	 * 
	 * This routine is called by valid_ipv6_hostaddr(). It must not call
	 * that routine, to avoid deadly recursion.
	 */
	for (cp = addr; cp < end; cp++) {
		const int ch = *(unsigned const char *) cp;
		if (ACL_ISDIGIT(ch)) {
			if (in_byte == 0) {
				in_byte = 1;
				byte_val = 0;
				byte_count++;
			}
			byte_val *= 10;
			byte_val += ch - '0';
			if (byte_val > 255) {
				return "invalid octet value";
			}
		} else if (ch == '.') {
			if (in_byte == 0 || cp[1] == 0) {
				return "misplaced dot";
			}
			/* XXX Allow 0.0.0.0 but not 0.1.2.3 */
			if (byte_count == 1 && byte_val == 0
				&& addr[strspn(addr, "0.")])
			{
				return "bad initial octet value";
			}
			in_byte = 0;
		} else {
			return "invalid character";
		}
	}

	if (byte_count != BYTES_NEEDED && strcmp(addr, "0") != 0) {
		return "invalid octet count";
	}

	return NULL;
}

int acl_valid_ipv4_hostaddr(const char *addr, int gripe) {
	const char *end, *err;

	if (addr == NULL || *addr == 0) {
		if (gripe) {
			acl_msg_warn("%s: address null", __FUNCTION__);
		}
		return 0;
	}

	if ((end = strrchr(addr, ACL_ADDR_SEP)) == NULL
		&& (end = strrchr(addr, ':')) == NULL)
	{
		end = addr + strlen(addr);
	}

	err = check_ipv4(addr, end);
	if (err == NULL) {
		return 1;
	}
	if (gripe) {
		acl_msg_warn("%s: %s address: %.100s", __FUNCTION__, err, addr);
	}
	return 0;
}

/* acl_valid_ipv6_hostaddr - validate IPv6 address syntax */

static const char *check_ipv6(const char *cp, const char *end) {
	int  null_field = 0, field = 0, len = 0;

	/*
	 * FIX 200501 The IPv6 patch validated syntax with getaddrinfo(), but I
	 * am not confident that everyone's system library routines are robust
	 * enough, like buffer overflow free. Remember, the valid_hostmumble()
	 * routines are meant to protect Postfix against malformed information
	 * in data received from the network.
	 * 
	 * We require eight-field hex addresses of the form 0:1:2:3:4:5:6:7,
	 * 0:1:2:3:4:5:6a.6b.7c.7d, or some :: compressed version of the same.
	 * 
	 * Note: the character position is advanced inside the loop. I have
	 * added comments to show why we can't get stuck.
	 */
	while (cp < end) {
		switch (*cp) {
		case '.':
			/* Terminate the loop. */
			if (field < 2 || field > 6) {
				return "malformed format";
			}
			/* NOT: acl_valid_hostaddr(). Avoid recursion. */
			return check_ipv4(cp - len, end);
		case ':':
		/* advance by exactly 1 character position or terminate. */
			if (field == 0 && len == 0 && ACL_ISALNUM(cp[1])) {
				return "bad null first field";
			}
			field++;
			if (field > 7) {
				return "too many `:'";
			}
			cp++;
			len = 0;
			if (*cp == ':') {
				if (null_field > 0) {
					return "too many `::'";
				}
				null_field = field;
			} 
			break;
		default:
			/* Advance by at least 1 character position or terminate. */
			len = (int) strspn(cp, "0123456789abcdefABCDEF");
			if (len /* - strspn((char *) cp, "0") */ > 4) {
				return "malformed format";
			}
			if (len <= 0) {
				return "invalid character";
			}
			cp += len;
			break;
		}
	}

	/* Terminate the loop. */
	if (field < 2) {
		return "too few `:'";
	}
	if (len == 0 && null_field != field - 1) {
		return "bad null last field";
	}
	return NULL;
}

int acl_valid_ipv6_hostaddr(const char *addr, int gripe) {
	const char *myname = "acl_valid_ipv6_hostaddr";
	const char *begin = addr, *end, *err;
	const char *ptr;

	if (addr == NULL || *addr == 0) {
		if (gripe) {
			acl_msg_warn("%s: address null", myname);
		}
		return 0;
	}

	/* For the standard format: [2001:db8::1]:443 */
	if (*addr == '[') {
		ptr = strchr(addr, ']');
		if (ptr == NULL) {
			if (!gripe) {
				return 0;
			}
			acl_msg_warn("%s: no `]', addr=%s", myname, addr);
			return 0;
		}

		begin++;
		if (ptr == begin) {
			if (!gripe) {
				return 0;
			}
			acl_msg_warn("%s: bad [], addr=%s", myname, addr);
			return 0;
		}
		end = ptr;
	} else if ((ptr = strrchr(addr, ACL_ADDR_SEP)) != NULL) {
		end = ptr;
	} else {
		end = addr + strlen(addr);
	}

	if ((ptr = strrchr(begin, '%')) != NULL && ptr < end) {
		end = ptr;
	}

	err = check_ipv6(begin, end);
	if (err == NULL) {
		return 1;
	}

	if (gripe) {
		acl_msg_warn("%s: %s in IPv6 address: %.100s",
			myname, err, addr);
	}
	return 0;
}

static int parse_hostaddr(const char *addr, char *ip, size_t size, int *port) {
	char *ptr;

	/* For the standard format: [2001:db8::1]:443 */
	if (*addr == '[') {
		if (*++addr == 0 || *addr == ']') {
			return 0;
		}

		ACL_SAFE_STRNCPY(ip, addr, size);
		ptr = strchr(ip, ']');
		if (ptr == NULL) {
			return 0;
		}

		*ptr++ = 0;
		if (port && (*ptr == ':' || *ptr == '|') && *++ptr != 0) {
			*port = acl_safe_atoi(ptr, -1);
		}
		return 1;
	}

	ACL_SAFE_STRNCPY(ip, addr, size);
	ptr = strchr(ip, ACL_ADDR_SEP);
	if (ptr == NULL) {
		char *ch;
		ptr = strchr(ip, ':');
		if (ptr == NULL) {
			return 1;
		}

		ch = strchr(ptr + 1, ':'); // If more than one ':', maybe IPv6.
		if (ch != NULL) {
			return 0;
		}
	}

	if (ptr == ip) {
		return 0;
	}

	*ptr++ = 0;
	if (port && *ptr != 0) {
		*port = acl_safe_atoi(ptr, -1);
	}
	return 1;
}

int acl_parse_hostaddr(const char *addr, char *ip, size_t size, int *port) {
	char *ptr;

	if (port) {
		*port = -1;
	}

	if (addr == NULL || *addr == 0 || ip == NULL || size == 0) {
		return 0;
	}

	if (parse_hostaddr(addr, ip, size, port) == 0) {
		return 0;
	}

	if ((ptr = strrchr(ip, '%')) != NULL) {
		*ptr = 0;
	}

#if 1
	return acl_valid_hostaddr(addr, 0);
#else
	if (check_ipv6(ip, ip + strlen(ip)) == NULL) {
		return 1;
	}

	// Sanity check if ':' exists in ip address.
	ptr = strrchr(ip, ':');
	if (ptr == NULL) {
		return check_ipv4(ip, ip + strlen(ip)) == NULL;
	}

	if (ptr == ip) {
		return 0;
	}

	*ptr = 0;
	if (port && *(ptr + 1) != 0) {
		*port = acl_safe_atoi(ptr + 1, -1);
	}

	return check_ipv4(ip, ptr) == NULL;
#endif
}

int acl_parse_ipv4_hostaddr(const char *addr, char *ip, size_t size, int *port) {
	char *ptr;

	if (port) {
		*port = -1;
	}

	if (addr == NULL || *addr == 0 || ip == NULL || size == 0) {
		return 0;
	}

	if (parse_hostaddr(addr, ip, size, port) == 0) {
		return 0;
	}

	ptr = strrchr(ip, ':');
	if (ptr == NULL) {
		return check_ipv4(ip, ip + strlen(ip)) == NULL;
	}

	if (ptr == ip) {
		return 0;
	}

	*ptr = 0;
	if (port && *(ptr + 1)!= 0) {
		*port = acl_safe_atoi(ptr + 1, -1);
		if (*port < 0 || *port > 65535) {
			return 0;
		}
	}
	return check_ipv4(ip, ip + strlen(ip)) == NULL;
}

int acl_parse_ipv6_hostaddr(const char *addr, char *ip, size_t size, int *port) {
	char *ptr;

	if (port) {
		*port = -1;
	}

	if (addr == NULL || *addr == 0 || ip == NULL || size == 0) {
		return 0;
	}

	if (parse_hostaddr(addr, ip, size, port) == 0) {
		return 0;
	}

	if ((ptr = strrchr(ip, '%')) != NULL) {
		*ptr = 0;
	}

	ptr = ip + strlen(ip);
	return check_ipv6(ip, ptr) == NULL;
}
