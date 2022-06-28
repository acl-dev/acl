/*++
 * NAME
 *	header_token 3
 * SUMMARY
 *	mail header parser
 * SYNOPSIS
 *	#include <header_token.h>
 *
 *	typedef struct {
 * .in +4
 *	    int     type;
 *	    const char *u.value;
 *	    ...
 * .in
 *	} HEADER_TOKEN;
 *
 *	ssize_t	header_token(token, token_len, token_buffer, ptr,
 *				specials, terminator)
 *	HEADER_TOKEN *token;
 *	ssize_t	token_len;
 *	VSTRING *token_buffer;
 *	const char **ptr;
 *	const char *specials;
 *	int	terminator;
 * DESCRIPTION
 *	This module parses a mail header value (text after field-name:)
 *	into tokens. The parser understands RFC 822 linear white space,
 *	quoted-string, comment, control characters, and a set of
 *	user-specified special characters.
 *
 *	A result token type is one of the following:
 * .IP HEADER_TOK_QSTRING
 *	Quoted string as per RFC 822.
 * .IP HEADER_TOK_TOKEN
 *	Token as per RFC 822, and the special characters supplied by the
 *	caller.
 * .IP other
 *	The value of a control character or special character.
 * .PP
 *	header_token() tokenizes the input and stops after a user-specified
 *	terminator (ignoring all tokens that exceed the capacity of
 *	the result storage), or when it runs out of space for the result.
 *	The terminator is not stored. The result value is the number of
 *	tokens stored, or -1 when the input was exhausted before any tokens
 *	were found.
 *
 *	Arguments:
 * .IP token
 *	Result array of HEADER_TOKEN structures. Token string values
 *	are pointers to null-terminated substrings in the token_buffer.
 * .IP token_len
 *	Length of the array of HEADER_TOKEN structures.
 * .IP token_buffer
 *	Storage for result token string values.
 * .IP ptr
 *	Input/output read position. The input is a null-terminated string.
 * .IP specials
 *	Special characters according to the relevant RFC, or a
 *	null pointer (default to the RFC 822 special characters).
 *	This must include the optional terminator if one is specified.
 * .IP terminator
 *	The special character to stop after, or zero.
 * BUGS
 *	Eight-bit characters are not given special treatment.
 * SEE ALSO
 *	RFC 822 (ARPA Internet Text Messages)
 * DIAGNOSTICS
 *	Fatal errors: memory allocation problem.
 * LICENSE
 * .ad
 * .fi
 *	The Secure Mailer license must be distributed with this software.
 * AUTHOR(S)
 *	Wietse Venema
 *	IBM T.J. Watson Research
 *	P.O. Box 704
 *	Yorktown Heights, NY 10598, USA
 *--*/

/* System library. */

#include "acl_stdafx.hpp"

#if !defined(ACL_MIME_DISABLE)

#include <string.h>
#include <ctype.h>

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_vstring.h"

/* Global library. */

#include "lex_822.hpp"
#include "header_token.hpp"

/* Application-specific. */

 /*
  * Silly little macros.
  */
#define STR(x)	acl_vstring_str(x)
#define LEN(x)	ACL_VSTRING_LEN(x)
#define CU_CHAR_PTR(x)	((const unsigned char *) (x))

/* header_token - parse out the next item in a message header */

ssize_t header_token(HEADER_TOKEN *token, ssize_t token_len,
	ACL_VSTRING *token_buffer, const char **ptr,
	const char *user_specials, int user_terminator)
{
	ssize_t comment_level;
	const unsigned char *cp;
	ssize_t len;
	int     ch;
	ssize_t tok_count;
	ssize_t n;

	/*
	 * Initialize.
	 */
	ACL_VSTRING_RESET(token_buffer);
	cp = CU_CHAR_PTR(*ptr);
	tok_count = 0;
	if (user_specials == 0)
		user_specials = LEX_822_SPECIALS;

	/*
	 * Main parsing loop.
	 * 
	 * XXX What was the reason to continue parsing when user_terminator is
	 * specified? Perhaps this was needed at some intermediate stage of
	 * development?
	 */
	while ((ch = *cp) != 0 && (user_terminator != 0 || tok_count < token_len)) {
		cp++;

		/*
		 * Skip RFC 822 linear white space.
		 */
		if (IS_SPACE_TAB_CR_LF(ch))
			continue;

		/*
		 * Terminator.
		 */
		if (ch == user_terminator)
			break;

		/*
		 * Skip RFC 822 comment.
		 */
		if (ch == '(') {
			comment_level = 1;
			while ((ch = *cp) != 0) {
				cp++;
				if (ch == '(') {  /* comments can nest! */
					comment_level++;
				} else if (ch == ')') {
					if (--comment_level == 0)
						break;
				} else if (ch == '\\') {
					if ((ch = *cp) == 0)
						break;
					cp++;
				}
			}
			continue;
		}

		/*
		 * Copy quoted text according to RFC 822.
		 */
		if (ch == '"') {
			if (tok_count < token_len) {
				token[tok_count].u.offset = (ssize_t) LEN(token_buffer);
				token[tok_count].type = HEADER_TOK_QSTRING;
			}
			while ((ch = *cp) != 0) {
				cp++;
				if (ch == '"')
					break;
				if (ch == '\n') {		/* unfold */
					if (tok_count < token_len) {
						len = (ssize_t) LEN(token_buffer);
						while (len > 0 && IS_SPACE_TAB_CR_LF(STR(token_buffer)[len - 1]))
							len--;
						if (len < (ssize_t) LEN(token_buffer))
							acl_vstring_truncate(token_buffer, len);
					}
					continue;
				}
				if (ch == '\\') {
					if (tok_count < token_len)
						ACL_VSTRING_ADDCH(token_buffer, ch);

					if (*cp == 0)
						break;
					ch = *cp;
					cp++;
				}
				if (tok_count < token_len)
					ACL_VSTRING_ADDCH(token_buffer, ch);
			}
			if (tok_count < token_len) {
				ACL_VSTRING_ADDCH(token_buffer, 0);
				tok_count++;
			}
			continue;
		}

		/*
		 * Control, or special.
		 */
		if (strchr(user_specials, ch) || ACL_ISCNTRL(ch)) {
			if (tok_count < token_len) {
				token[tok_count].u.offset = (ssize_t) LEN(token_buffer);
				token[tok_count].type = ch;
				ACL_VSTRING_ADDCH(token_buffer, ch);
				ACL_VSTRING_ADDCH(token_buffer, 0);
				tok_count++;
			}
			continue;
		}

		/*
		 * Token.
		 */
		else {
			if (tok_count < token_len) {
				token[tok_count].u.offset = (ssize_t) LEN(token_buffer);
				token[tok_count].type = HEADER_TOK_TOKEN;
				ACL_VSTRING_ADDCH(token_buffer, ch);
			}
			while ((ch = *cp) != 0 && !IS_SPACE_TAB_CR_LF(ch)
				&& !ACL_ISCNTRL(ch) && !strchr(user_specials, ch)) {
					cp++;
					if (tok_count < token_len)
						ACL_VSTRING_ADDCH(token_buffer, ch);
				}
				if (tok_count < token_len) {
					ACL_VSTRING_ADDCH(token_buffer, 0);
					tok_count++;
				}
				continue;
		}
	}

	/*
	 * Ignore a zero-length item after the last terminator.
	 */
	if (tok_count == 0 && ch == 0)
		return (-1);

	/*
	 * Finalize. Fill in the string pointer array, now that the token buffer
	 * is no longer dynamically reallocated as it grows.
	 */
	*ptr = (const char *) cp;
	for (n = 0; n < tok_count; n++)
		token[n].u.value = STR(token_buffer) + token[n].u.offset;

	//if (acl_msg_verbose)
	//	acl_msg_info("header_token: %s %s %s",
	//		tok_count > 0 ? token[0].u.value : "",
	//		tok_count > 1 ? token[1].u.value : "",
	//		tok_count > 2 ? token[2].u.value : "");

	return (tok_count);
}

#endif // !defined(ACL_MIME_DISABLE)
