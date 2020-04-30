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

int acl_valid_hostname(const char *name, int gripe)
{
	char   *myname = "acl_valid_hostname";
	const char *cp;
	int     label_length = 0;
	int     label_count = 0;
	int     non_numeric = 0;
	int     ch;

	/*
	 * Trivial cases first.
	 */
	if (*name == 0) {
		if (gripe)
			acl_msg_warn("%s: empty hostname", myname);
		return 0;
	}

	/*
	 * Find bad characters or label lengths. Find adjacent delimiters.
	 */
	for (cp = name; (ch = *(const unsigned char *) cp) != 0; cp++) {
		if (ACL_ISALNUM(ch) || ch == '_') {		/* grr.. */
			if (label_length == 0)
				label_count++;
			label_length++;
			if (label_length > ACL_VALID_LABEL_LEN) {
				if (gripe)
					acl_msg_warn("%s: hostname label too "
						"long: %.100s", myname, name);
				return 0;
			}
			if (!ACL_ISDIGIT(ch))
				non_numeric = 1;
		} else if (ch == '.') {
			if (label_length == 0 || cp[1] == 0) {
				if (gripe)
					acl_msg_warn("%s: misplaced delimiter:"
						" %.100s", myname, name);
				return 0;
			}
			label_length = 0;
		} else if (ch == '-') {
			label_length++;
			if (label_length == 1 || cp[1] == 0 || cp[1] == '.') {
				if (gripe)
					acl_msg_warn("%s: misplaced hyphen:"
						" %.100s", myname, name);
				return 0;
			}
		}
#ifdef SLOPPY_VALID_HOSTNAME
		else if (ch == ':' && acl_valid_ipv6_hostaddr(name, ACL_DONT_GRIPE)) {
			non_numeric = 0;
			break;
		}
#endif
		else {
			if (gripe)
				acl_msg_warn("%s: invalid character %d"
					"(decimal): %.100s", myname, ch, name);
			return 0;
		}
	}

	if (non_numeric == 0) {
		if (gripe)
			acl_msg_warn("%s: numeric hostname: %.100s", myname, name);
#ifndef SLOPPY_VALID_HOSTNAME
		return 0;
#endif
	}
	if (cp - name > ACL_VALID_HOSTNAME_LEN) {
		if (gripe)
			acl_msg_warn("%s: bad length %d for %.100s...",
				myname, (int) (cp - name), name);
		return 0;
	}
	return 1;
}

int acl_valid_unix(const char *addr)
{
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

static int valid_ipv4_field(const char *ptr)
{
	int n;

	if (*ptr == '*' && *(ptr + 1) == 0)
		return 1;
	if (!acl_alldig(ptr))
		return 0;
	n = atoi(ptr);
	if (n < 0 || n > 255)
		return 0;

	return 1;
}

static int valid_port(const char *ptr)
{
	int n;

	if (!acl_alldig(ptr))
		return 0;

	n = atoi(ptr);
	return n >= 0 && n <= 65535;
}

static int valid_ipv4_wildcard(const char *addr)
{
	ACL_ARGV *tokens = acl_argv_split(addr, ".");
	char     *ptr, *s4;
	int       i;

	if (tokens->argc != 4) {
		acl_argv_free(tokens);
		return 0;
	}

	for (i = 0; i < 3; i++) {
		ptr = tokens->argv[i];
		if (!valid_ipv4_field(ptr)) {
			acl_argv_free(tokens);
			return 0;
		}
	}

	s4 = tokens->argv[3];
	ptr = strchr(s4, ACL_ADDR_SEP);
	if (ptr == NULL)
		ptr = strchr(s4, ':');
	if (ptr)
		*ptr++ = 0;

	if (!valid_ipv4_field(s4)) {
		acl_argv_free(tokens);
		return 0;
	}

	if (ptr)
		i = valid_port(ptr);
	else
		i = 1;
	acl_argv_free(tokens);
	return i;
}

int acl_valid_hostaddr(const char *addr, int gripe)
{
	const char *myname = "acl_valid_hostaddr";

	/* Trivial cases first. */
	if (*addr == 0) {
		if (gripe)
			acl_msg_warn("%s: empty address", myname);
		return 0;
	}

#define VALID_PORT(x) (acl_alldig((x)) && atoi((x)) > 0)
#define VALID_SEP(x)  ((x) == ACL_ADDR_SEP || (x) == ':')

	/* port */
	if (VALID_PORT(addr))
		return 1;
	/* |port, :port */
	if (VALID_SEP(*addr) && VALID_PORT(addr + 1))
		return 1;
	/* *|port, *:port */
	if (*addr == '*' && VALID_SEP(*(addr + 1)) && VALID_PORT(addr + 2))
		return 1;
	if (valid_ipv4_wildcard(addr))
		return 1;

	/* Protocol-dependent processing next. */
	return acl_valid_ipv4_hostaddr(addr, gripe)
		|| acl_valid_ipv6_hostaddr(addr, gripe);
}

/* acl_valid_ipv4_hostaddr - test dotted quad string for correctness */

int acl_valid_ipv4_hostaddr(const char *addr_in, int gripe)
{
	char *myname = "acl_valid_ipv4_hostaddr";
	int   in_byte = 0, byte_count = 0, byte_val = 0, ch;
	char  addr[128], *ptr;
	const char *cp;

#define BYTES_NEEDED	4

	ACL_SAFE_STRNCPY(addr, addr_in, sizeof(addr));
	if ((ptr = strrchr(addr, ACL_ADDR_SEP)) != NULL) {
		*ptr = 0;
	} else if ((ptr = strrchr(addr, ':')) != NULL) {
		*ptr = 0;
	}

	/*
	 * Scary code to avoid sscanf() overflow nasties.
	 * 
	 * This routine is called by valid_ipv6_hostaddr(). It must not call
	 * that routine, to avoid deadly recursion.
	 */
	for (cp = addr; (ch = *(unsigned const char *) cp) != 0; cp++) {
		if (ACL_ISDIGIT(ch)) {
			if (in_byte == 0) {
				in_byte = 1;
				byte_val = 0;
				byte_count++;
			}
			byte_val *= 10;
			byte_val += ch - '0';
			if (byte_val > 255) {
				if (gripe)
					acl_msg_warn("%s: invalid octet value:"
						" %.100s", myname, addr);
				return 0;
			}
		} else if (ch == '.') {
			if (in_byte == 0 || cp[1] == 0) {
				if (gripe)
					acl_msg_warn("%s: misplaced dot:"
						" %.100s", myname, addr);
				return 0;
			}
			/* XXX Allow 0.0.0.0 but not 0.1.2.3 */
			if (byte_count == 1 && byte_val == 0
			    && addr[strspn(addr, "0.")]) {
				if (gripe)
					acl_msg_warn("%s: bad initial octet"
						" value: %.100s", myname, addr);
				return 0;
			}
			in_byte = 0;
		} else {
			if (gripe)
				acl_msg_warn("%s: invalid character %d"
					"(decimal): %.100s", myname, ch, addr);
			return 0;
		}
	}

	if (byte_count != BYTES_NEEDED && strcmp(addr, "0") != 0) {
		if (gripe)
			acl_msg_warn("%s: invalid octet count: %.100s",
				myname, addr);
		return 0;
	}

	return 1;
}

/* acl_valid_ipv6_hostaddr - validate IPv6 address syntax */

int acl_valid_ipv6_hostaddr(const char *addr_in, int gripe)
{
	const char *myname = "acl_valid_ipv6_hostaddr";
	int  null_field = 0, field = 0, len = 0;
	char addr[128], *ptr;
	const unsigned char *cp;

	ACL_SAFE_STRNCPY(addr, addr_in, sizeof(addr));
	if ((ptr = strrchr(addr, ACL_ADDR_SEP)) != NULL) {
		*ptr = 0;
	}

	if ((ptr = strrchr(addr, '%')) != NULL) {
		*ptr = 0;
	}

	cp = (const unsigned char *) addr;

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
	for (;;) {
		switch (*cp) {
		case 0:
			/* Terminate the loop. */
			if (field < 2) {
				if (gripe)
					acl_msg_warn("%s: too few `:' in IPv6 "
						"address: %.100s", myname, addr);
				return 0;
			} else if (len == 0 && null_field != field - 1) {
				if (gripe)
					acl_msg_warn("%s: bad null last field "
						"in IPv6 address: %.100s",
						myname, addr);
				return 0;
			} else
				return 1;
		case '.':
			/* Terminate the loop. */
			if (field < 2 || field > 6) {
				if (gripe)
					acl_msg_warn("%s: malformed IPv4-in-IPv6"
						" address: %.100s", myname, addr);
				return 0;
			} 
			/* NOT: acl_valid_hostaddr(). Avoid recursion. */
			return acl_valid_ipv4_hostaddr((const char *) cp - len, gripe);
		case ':':
		/* advance by exactly 1 character position or terminate. */
			if (field == 0 && len == 0 && ACL_ISALNUM(cp[1])) {
				if (gripe)
					acl_msg_warn("%s: bad null first field"
						" in IPv6 address: %.100s",
						myname, addr);
				return 0;
			}
			field++;
			if (field > 7) {
				if (gripe)
					acl_msg_warn("%s: too many `:' in"
						" IPv6 address: %.100s",
						myname, addr);
				return 0;
			}
			cp++;
			len = 0;
			if (*cp == ':') {
				if (null_field > 0) {
					if (gripe)
						acl_msg_warn("%s: too many `::'"
							" in IPv6 address: %.100s",
							myname, addr);
					return 0;
				}
				null_field = field;
			} 
			break;
		default:
			/* Advance by at least 1 character position or terminate. */
			len = (int) strspn((const char *) cp, "0123456789abcdefABCDEF");
			if (len /* - strspn((char *) cp, "0") */ > 4) {
				if (gripe)
					acl_msg_warn("%s: malformed IPv6 address: %.100s",
						myname, addr);
				return 0;
			}
			if (len <= 0) {
				if (gripe)
					acl_msg_warn("%s: invalid character"
						" %d(decimal) in IPv6 address: %.100s",
						myname, *cp, addr);
				return 0;
			}
			cp += len;
			break;
		}
	}
}
