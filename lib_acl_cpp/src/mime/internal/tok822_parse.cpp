/*++
 * NAME
 *	tok822_parse 3
 * SUMMARY
 *	RFC 822 address parser
 * SYNOPSIS
 *	#include <tok822.h>
 *
 *	TOK822 *tok822_scan_limit(str, tailp, limit)
 *	const char *str;
 *	TOK822	**tailp;
 *	int	limit;
 *
 *	TOK822 *tok822_scan(str, tailp)
 *	const char *str;
 *	TOK822	**tailp;
 *
 *	TOK822	*tok822_parse_limit(str, limit)
 *	const char *str;
 *	int	limit;
 *
 *	TOK822	*tok822_parse(str)
 *	const char *str;
 *
 *	TOK822	*tok822_scan_addr(str)
 *	const char *str;
 *
 *	VSTRING	*tok822_externalize(buffer, tree, flags)
 *	VSTRING	*buffer;
 *	TOK822	*tree;
 *	int	flags;
 *
 *	VSTRING	*tok822_internalize(buffer, tree, flags)
 *	VSTRING	*buffer;
 *	TOK822	*tree;
 *	int	flags;
 * DESCRIPTION
 *	This module converts address lists between string form and parse
 *	tree formats. The string form can appear in two different ways:
 *	external (or quoted) form, as used in message headers, and internal
 *	(unquoted) form, as used internally by the mail software.
 *	Although RFC 822 expects 7-bit data, these routines pay no
 *	special attention to 8-bit characters.
 *
 *	tok822_scan() converts the external-form string in \fIstr\fR
 *	to a linear token list. The \fItailp\fR argument is a null pointer
 *	or receives the pointer value of the last result list element.
 *
 *	tok822_scan_limit() implements tok822_scan(), which is a macro.
 *	The \fIlimit\fR argument is either zero or an upper bound on the
 *	number of tokens produced.
 *
 *	tok822_parse() converts the external-form address list in
 *	\fIstr\fR to the corresponding token tree. The parser is permissive
 *	and will not throw away information that it does not understand.
 *	The parser adds missing commas between addresses.
 *
 *	tok822_parse_limit() implements tok822_parse(), which is a macro.
 *	The \fIlimit\fR argument is either zero or an upper bound on the
 *	number of tokens produced.
 *
 *	tok822_scan_addr() converts the external-form string in
 *	\fIstr\fR to an address token tree. This is just string to
 *	token list conversion; no parsing is done. This routine is
 *	suitable for data that should contain just one address and no
 *	other information.
 *
 *	tok822_externalize() converts a token list to external form.
 *	Where appropriate, characters and strings are quoted and white
 *	space is inserted. The \fIflags\fR argument is the binary OR of
 *	zero or more of the following:
* .IP TOK822_STR_WIPE
*	Initially, truncate the result to zero length.
* .IP TOK822_STR_TERM
*	Append a null terminator to the result when done.
* .IP TOK822_STR_LINE
*	Append a line break after each comma token, instead of appending
*	whitespace.  It is up to the caller to concatenate short lines to
*	produce longer ones.
* .IP TOK822_STR_TRNC
*	Truncate non-address information to 250 characters per address, to
*	protect Sendmail systems that are vulnerable to the problem in CERT
*	advisory CA-2003-07.
*	This flag has effect with tok822_externalize() only.
* .PP
*	The macro TOK_822_NONE expresses that none of the above features
*	should be activated.
*
*	The macro TOK822_STR_DEFL combines the TOK822_STR_WIPE and
*	TOK822_STR_TERM flags. This is useful for most token to string
*	conversions.
*
*	The macro TOK822_STR_HEAD combines the TOK822_STR_TERM,
	*	TOK822_STR_LINE and TOK822_STR_TRNC flags. This is useful for
	*	the special case of token to mail header conversion.
	*
	*	tok822_internalize() converts a token list to string form,
	*	without quoting. White space is inserted where appropriate.
	*	The \fIflags\fR argument is as with tok822_externalize().
	* STANDARDS
	* .ad
	* .fi
	*	RFC 822 (ARPA Internet Text Messages). In addition to this standard
	*	this module implements additional operators such as % and !. These
	*	are needed because the real world is not all RFC 822. Also, the ':'
	*	operator is allowed to appear inside addresses, to accommodate DECnet.
	*	In addition, 8-bit data is not given special treatment.
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

#include <ctype.h>
#include <string.h>

	/* Utility library. */

#include "stdlib/acl_vstring.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_stringops.h"

	/* Global library. */

#include "lex_822.hpp"
#include "quote_822_local.hpp"
#include "tok822.hpp"

	/*
	 * I suppose this is my favorite macro. Used heavily for tokenizing.
	 */
#define COLLECT(t,s,c,cond) { \
	while ((c = *(unsigned char *) s) != 0) { \
		if (c == '\\') { \
			if ((c = *(unsigned char *)++s) == 0) \
			break; \
		} else if (!(cond)) { \
			break; \
		} \
		ACL_VSTRING_ADDCH(t->vstr, IS_SPACE_TAB_CR_LF(c) ? ' ' : c); \
		s++; \
	} \
	ACL_VSTRING_TERMINATE(t->vstr); \
}

#define COLLECT_SKIP_LAST(t,s,c,cond) { COLLECT(t,s,c,cond); if (*s) s++; }

	/*
	 * Not quite as complex. The parser depends heavily on it.
	 */
#define SKIP(tp, cond) { \
	while (tp->type && (cond)) \
	tp = tp->prev; \
}

#define MOVE_COMMENT_AND_CONTINUE(tp, right) { \
	TOK822 *prev = tok822_unlink(tp); \
	right = tok822_prepend(right, tp); \
	tp = prev; \
	continue; \
}

#define SKIP_MOVE_COMMENT(tp, cond, right) { \
	while (tp->type && (cond)) { \
		if (tp->type == TOK822_COMMENT) \
		MOVE_COMMENT_AND_CONTINUE(tp, right); \
		tp = tp->prev; \
	} \
}

/*
 * Single-character operators. We include the % and ! operators because not
 * all the world is RFC822. XXX Make this operator list configurable when we
 * have a real rewriting language. Include | for aliases file parsing.
 */
static char tok822_opchar[] = "|%!" LEX_822_SPECIALS;
static void tok822_quote_atom(TOK822 *);
static const char *tok822_comment(TOK822 *, const char *);
static TOK822 *tok822_group(int, TOK822 *, TOK822 *, int);
static void tok822_copy_quoted(ACL_VSTRING *, char *, const char *);
static int tok822_append_space(TOK822 *);

#define DO_WORD		(1<<0)		/* finding a word is ok here */
#define DO_GROUP	(1<<1)		/* doing an address group */

#define ADD_COMMA	','		/* resynchronize */
#define NO_MISSING_COMMA 0

/* tok822_internalize - token tree to string, internal form */

ACL_VSTRING *tok822_internalize(ACL_VSTRING *vp, TOK822 *tree, int flags)
{
	TOK822 *tp;

	if (flags & TOK822_STR_WIPE)
		ACL_VSTRING_RESET(vp);

	for (tp = tree; tp; tp = tp->next) {
		switch (tp->type) {
		case ',':
			ACL_VSTRING_ADDCH(vp, tp->type);
			if (flags & TOK822_STR_LINE) {
				ACL_VSTRING_ADDCH(vp, '\n');
				continue;
			}
			break;
		case TOK822_ADDR:
			tok822_internalize(vp, tp->head, TOK822_STR_NONE);
			break;
		case TOK822_COMMENT:
		case TOK822_ATOM:
		case TOK822_QSTRING:
			acl_vstring_strcat(vp, acl_vstring_str(tp->vstr));
			break;
		case TOK822_DOMLIT:
			ACL_VSTRING_ADDCH(vp, '[');
			acl_vstring_strcat(vp, acl_vstring_str(tp->vstr));
			ACL_VSTRING_ADDCH(vp, ']');
			break;
		case TOK822_STARTGRP:
			ACL_VSTRING_ADDCH(vp, ':');
			break;
		default:
			if (tp->type >= TOK822_MINTOK)
				acl_msg_panic("tok822_internalize: unknown operator %d", tp->type);
			ACL_VSTRING_ADDCH(vp, tp->type);
		}
		if (tok822_append_space(tp))
			ACL_VSTRING_ADDCH(vp, ' ');
	}
	if (flags & TOK822_STR_TERM)
		ACL_VSTRING_TERMINATE(vp);
	return (vp);
}

/* strip_address - strip non-address text from address expression */

static void strip_address(ACL_VSTRING *vp, ssize_t start, TOK822 *addr)
{
	ACL_VSTRING *tmp;

	/*
	 * Emit plain <address>. Discard any comments or phrases.
	 */
	ACL_VSTRING_TERMINATE(vp);
	acl_msg_warn("stripping too many comments from address: %.100s...",
			acl_vstring_str(vp) + start);
	//acl_printable(vstring_str(vp) + start, '?')); //zsx
	acl_vstring_truncate(vp, start);
	ACL_VSTRING_ADDCH(vp, '<');
	if (addr) {
		tmp = acl_vstring_alloc(100);
		tok822_internalize(tmp, addr, TOK822_STR_TERM);
		quote_822_local_flags(vp, acl_vstring_str(tmp),
			QUOTE_FLAG_8BITCLEAN | QUOTE_FLAG_APPEND);
		acl_vstring_free(tmp);
	}
	ACL_VSTRING_ADDCH(vp, '>');
}

/* tok822_externalize - token tree to string, external form */

ACL_VSTRING *tok822_externalize(ACL_VSTRING *vp, TOK822 *tree, int flags)
{
	ACL_VSTRING *tmp;
	TOK822 *tp;
	ssize_t start = 0;
	TOK822 *addr = 0;
	ssize_t addr_len = 0;

	/*
	 * Guard against a Sendmail buffer overflow (CERT advisory CA-2003-07).
	 * The problem was that Sendmail could store too much non-address text
	 * (comments, phrases, etc.) into a static 256-byte buffer.
	 * 
	 * When the buffer fills up, fixed Sendmail versions remove comments etc.
	 * and reduce the information to just <$g>, which expands to <address>.
	 * No change is made when an address expression (text separated by
	 * commas) contains no address. This fix reportedly also protects
	 * Sendmail systems that are still vulnerable to this problem.
	 * 
	 * Postfix takes the same approach, grudgingly. To avoid unnecessary damage,
	 * Postfix removes comments etc. only when the amount of non-address text
	 * in an address expression (text separated by commas) exceeds 250 bytes.
	 * 
	 * With Sendmail, the address part of an address expression is the
	 * right-most <> instance in that expression. If an address expression
	 * contains no <>, then Postfix guarantees that it contains at most one
	 * non-comment string; that string is the address part of the address
	 * expression, so there is no ambiguity.
	 * 
	 * Finally, we note that stress testing shows that other code in Sendmail
	 * 8.12.8 bluntly truncates ``text <address>'' to 256 bytes even when
	 * this means chopping the <address> somewhere in the middle. This is a
	 * loss of control that we're not entirely comfortable with. However,
	 * unbalanced quotes and dangling backslash do not seem to influence the
	 * way that Sendmail parses headers, so this is not an urgent problem.
	 */
#define MAX_NONADDR_LENGTH 250

#define RESET_NONADDR_LENGTH { \
	start = (ssize_t) ACL_VSTRING_LEN(vp); \
	addr = 0; \
	addr_len = 0; \
}

#define ENFORCE_NONADDR_LENGTH do { \
	if (addr && (ssize_t) ACL_VSTRING_LEN(vp) - addr_len > start + MAX_NONADDR_LENGTH) \
		strip_address(vp, start, addr->head); \
} while(0)

	if (flags & TOK822_STR_WIPE)
		ACL_VSTRING_RESET(vp);

	if (flags & TOK822_STR_TRNC)
		RESET_NONADDR_LENGTH;

	for (tp = tree; tp; tp = tp->next) {
		switch (tp->type) {
		case ',':
			if (flags & TOK822_STR_TRNC)
				ENFORCE_NONADDR_LENGTH;
			ACL_VSTRING_ADDCH(vp, tp->type);
			ACL_VSTRING_ADDCH(vp, (flags & TOK822_STR_LINE) ? '\n' : ' ');
			if (flags & TOK822_STR_TRNC)
				RESET_NONADDR_LENGTH;
			continue;

			/*
			 * XXX In order to correctly externalize an address, it is not
			 * sufficient to quote individual atoms. There are higher-level
			 * rules that say when an address localpart needs to be quoted.
			 * We wing it with the quote_822_local() routine, which ignores
			 * the issue of atoms in the domain part that would need quoting.
			 */
		case TOK822_ADDR:
			addr = tp;
			tmp = acl_vstring_alloc(100);
			tok822_internalize(tmp, tp->head, TOK822_STR_TERM);
			addr_len = (ssize_t) ACL_VSTRING_LEN(vp);
			quote_822_local_flags(vp, acl_vstring_str(tmp),
				QUOTE_FLAG_8BITCLEAN | QUOTE_FLAG_APPEND);
			addr_len = (ssize_t) ACL_VSTRING_LEN(vp) - addr_len;
			acl_vstring_free(tmp);
			break;
		case TOK822_ATOM:
		case TOK822_COMMENT:
			acl_vstring_strcat(vp, acl_vstring_str(tp->vstr));
			break;
		case TOK822_QSTRING:
			ACL_VSTRING_ADDCH(vp, '"');
			tok822_copy_quoted(vp, acl_vstring_str(tp->vstr), "\"\\\r\n");
			ACL_VSTRING_ADDCH(vp, '"');
			break;
		case TOK822_DOMLIT:
			ACL_VSTRING_ADDCH(vp, '[');
			tok822_copy_quoted(vp, acl_vstring_str(tp->vstr), "\\\r\n");
			ACL_VSTRING_ADDCH(vp, ']');
			break;
		case TOK822_STARTGRP:
			ACL_VSTRING_ADDCH(vp, ':');
			break;
		case '<':
			if (tp->next && tp->next->type == '>') {
				addr = tp;
				addr_len = 0;
			}
			ACL_VSTRING_ADDCH(vp, '<');
			break;
		default:
			if (tp->type >= TOK822_MINTOK)
				acl_msg_panic("tok822_externalize: unknown operator %d", tp->type);
			ACL_VSTRING_ADDCH(vp, tp->type);
		}
		if (tok822_append_space(tp))
			ACL_VSTRING_ADDCH(vp, ' ');
	}
	if (flags & TOK822_STR_TRNC)
		ENFORCE_NONADDR_LENGTH;

	if (flags & TOK822_STR_TERM)
		ACL_VSTRING_TERMINATE(vp);
	return (vp);
}

/* tok822_copy_quoted - copy a string while quoting */

static void tok822_copy_quoted(ACL_VSTRING *vp, char *str, const char *quote_set)
{
	int     ch;

	while ((ch = *(unsigned char *) str++) != 0) {
		if (strchr(quote_set, ch))
			ACL_VSTRING_ADDCH(vp, '\\');
		ACL_VSTRING_ADDCH(vp, ch);
	}
}

/* tok822_append_space - see if space is needed after this token */

static int tok822_append_space(TOK822 *tp)
{
	TOK822 *next;

	if (tp == 0 || (next = tp->next) == 0 || tp->owner != 0)
		return (0);
	if (tp->type == ',' || tp->type == TOK822_STARTGRP || next->type == '<')
		return (1);

#define NON_OPERATOR(x) \
	(x->type == TOK822_ATOM || x->type == TOK822_QSTRING \
	 || x->type == TOK822_COMMENT || x->type == TOK822_DOMLIT \
	 || x->type == TOK822_ADDR)

	return (NON_OPERATOR(tp) && NON_OPERATOR(next));
}

/* tok822_scan_limit - tokenize string */

TOK822 *tok822_scan_limit(const char *str, TOK822 **tailp, int tok_count_limit)
{
	TOK822 *head = 0;
	TOK822 *tail = 0;
	TOK822 *tp;
	int     ch;
	int     tok_count = 0;

	/*
	 * XXX 2822 new feature: Section 4.1 allows "." to appear in a phrase (to
	 * allow for forms such as: Johnny B. Goode <johhny@domain.org>. I cannot
	 * handle that at the tokenizer level - it is not context sensitive. And
	 * to fix this at the parser level requires radical changes to preserve
	 * white space as part of the token stream. Thanks a lot, people.
	 */
	while ((ch = *(const unsigned char *) str++) != 0) {
		if (IS_SPACE_TAB_CR_LF(ch))
			continue;
		if (ch == '(') {
			tp = tok822_alloc(TOK822_COMMENT, (char *) 0);
			str = tok822_comment(tp, str);
		} else if (ch == '[') {
			tp = tok822_alloc(TOK822_DOMLIT, (char *) 0);
			COLLECT_SKIP_LAST(tp, str, ch, ch != ']');
		} else if (ch == '"') {
			tp = tok822_alloc(TOK822_QSTRING, (char *) 0);
			COLLECT_SKIP_LAST(tp, str, ch, ch != '"');
		} else if (ch != '\\' && strchr(tok822_opchar, ch)) {
			tp = tok822_alloc(ch, (char *) 0);
		} else {
			tp = tok822_alloc(TOK822_ATOM, (char *) 0);
			str -= 1;				/* \ may be first */
			COLLECT(tp, str, ch, !IS_SPACE_TAB_CR_LF(ch) && !strchr(tok822_opchar, ch));
			tok822_quote_atom(tp);
		}
		if (head == 0) {
			head = tail = tp;
			while (tail->next)
				tail = tail->next;
		} else {
			tail = tok822_append(tail, tp);
		}
		if (tok_count_limit > 0 && ++tok_count >= tok_count_limit)
			break;
	}
	if (tailp)
		*tailp = tail;
	return (head);
}

/* tok822_parse_limit - translate external string to token tree */

TOK822 *tok822_parse_limit(const char *str, int tok_count_limit)
{
	TOK822 *head;
	TOK822 *tail;
	TOK822 *right;
	TOK822 *first_token;
	TOK822 *last_token;
	TOK822 *tp;
	int     state;

	/*
	 * First, tokenize the string, from left to right. We are not allowed to
	 * throw away any information that we do not understand. With a flat
	 * token list that contains all tokens, we can always convert back to
	 * string form.
	 */
	if ((first_token = tok822_scan_limit(str, &last_token, tok_count_limit)) == 0)
		return (0);

	/*
	 * For convenience, sandwich the token list between two sentinel tokens.
	 */
#define GLUE(left,rite) { left->next = rite; rite->prev = left; }

	head = tok822_alloc(0, (char *) 0);
	GLUE(head, first_token);
	tail = tok822_alloc(0, (char *) 0);
	GLUE(last_token, tail);

	/*
	 * Next step is to transform the token list into a parse tree. This is
	 * done most conveniently from right to left. If there is something that
	 * we do not understand, just leave it alone, don't throw it away. The
	 * address information that we're looking for sits in-between the current
	 * node (tp) and the one called right. Add missing commas on the fly.
	 */
	state = DO_WORD;
	right = tail;
	tp = tail->prev;
	while (tp->type) {
		if (tp->type == TOK822_COMMENT) {	/* move comment to the side */
			MOVE_COMMENT_AND_CONTINUE(tp, right);
		} else if (tp->type == ';') {		/* rh side of named group */
			right = tok822_group(TOK822_ADDR, tp, right, ADD_COMMA);
			state = DO_GROUP | DO_WORD;
		} else if (tp->type == ':' && (state & DO_GROUP) != 0) {
			tp->type = TOK822_STARTGRP;
			(void) tok822_group(TOK822_ADDR, tp, right, NO_MISSING_COMMA);
			SKIP(tp, tp->type != ',');
			right = tp;
			continue;
		} else if (tp->type == '>') {		/* rh side of <route> */
			right = tok822_group(TOK822_ADDR, tp, right, ADD_COMMA);
			SKIP_MOVE_COMMENT(tp, tp->type != '<', right);
			(void) tok822_group(TOK822_ADDR, tp, right, NO_MISSING_COMMA);
			SKIP(tp, tp->type > 0xff || strchr(">;,:", tp->type) == 0);
			right = tp;
			state |= DO_WORD;
			continue;
		} else if (tp->type == TOK822_ATOM || tp->type == TOK822_QSTRING
				|| tp->type == TOK822_DOMLIT) {
			if ((state & DO_WORD) == 0)
				right = tok822_group(TOK822_ADDR, tp, right, ADD_COMMA)->next;
			state &= ~DO_WORD;
		} else if (tp->type == ',') {
			right = tok822_group(TOK822_ADDR, tp, right, NO_MISSING_COMMA);
			state |= DO_WORD;
		} else {
			state |= DO_WORD;
		}
		tp = tp->prev;
	}
	(void) tok822_group(TOK822_ADDR, tp, right, NO_MISSING_COMMA);

	/*
	 * Discard the sentinel tokens on the left and right extremes. Properly
	 * terminate the resulting list.
	 */
	tp = (head->next != tail ? head->next : 0);
	tok822_cut_before(head->next);
	tok822_free(head);
	tok822_cut_before(tail);
	tok822_free(tail);
	return (tp);
}

/* tok822_quote_atom - see if an atom needs quoting when externalized */

static void tok822_quote_atom(TOK822 *tp)
{
	char   *cp;
	int     ch;

	/*
	 * RFC 822 expects 7-bit data. Rather than quoting every 8-bit character
	 * (and still passing it on as 8-bit data) we leave 8-bit data alone.
	 */
	for (cp = acl_vstring_str(tp->vstr); (ch = *(unsigned char *) cp) != 0; cp++) {
		if ( /* !ISASCII(ch) || */ ch == ' '
			|| ACL_ISCNTRL(ch) || strchr(tok822_opchar, ch))
		{
			tp->type = TOK822_QSTRING;
			break;
		}
	}
}

/* tok822_comment - tokenize comment */

static const char *tok822_comment(TOK822 *tp, const char *str)
{
	int     level = 1;
	int     ch;

	/*
	 * XXX We cheat by storing comments in their external form. Otherwise it
	 * would be a royal pain to preserve \ before (. That would require a
	 * recursive parser; the easy to implement stack-based recursion would be
	 * too expensive.
	 */
	ACL_VSTRING_ADDCH(tp->vstr, '(');

	while ((ch = *(const unsigned char *) str) != 0) {
		ACL_VSTRING_ADDCH(tp->vstr, ch);
		str++;
		if (ch == '(') {			/* comments can nest! */
			level++;
		} else if (ch == ')') {
			if (--level == 0)
				break;
		} else if (ch == '\\') {
			if ((ch = *(unsigned char *) str) == 0)
				break;
			ACL_VSTRING_ADDCH(tp->vstr, ch);
			str++;
		}
	}
	ACL_VSTRING_TERMINATE(tp->vstr);
	return (str);
}

/* tok822_group - cluster a group of tokens */

static TOK822 *tok822_group(int group_type, TOK822 *left, TOK822 *right, int sync_type)
{
	TOK822 *group;
	TOK822 *sync;
	TOK822 *first;

	/*
	 * Cluster the tokens between left and right under their own parse tree
	 * node. Optionally insert a resync token.
	 */
	if (left != right && (first = left->next) != right) {
		tok822_cut_before(right);
		tok822_cut_before(first);
		group = tok822_alloc(group_type, (char *) 0);
		tok822_sub_append(group, first);
		tok822_append(left, group);
		tok822_append(group, right);
		if (sync_type) {
			sync = tok822_alloc(sync_type, (char *) 0);
			tok822_append(left, sync);
		}
	}
	return (left);
}

/* tok822_scan_addr - convert external address string to address token */

TOK822 *tok822_scan_addr(const char *addr)
{
	TOK822 *tree = tok822_alloc(TOK822_ADDR, (char *) 0);

	tree->head = tok822_scan(addr, &tree->tail);
	return (tree);
}

#ifdef TEST

#include <unistd.h>
#include <vstream.h>
#include <readlline.h>

/* tok822_print - display token */

static void tok822_print(TOK822 *list, int indent)
{
	TOK822 *tp;

	for (tp = list; tp; tp = tp->next) {
		if (tp->type < TOK822_MINTOK) {
			vstream_printf("%*s %s \"%c\"\n", indent, "", "OP", tp->type);
		} else if (tp->type == TOK822_ADDR) {
			vstream_printf("%*s %s\n", indent, "", "address");
			tok822_print(tp->head, indent + 2);
		} else if (tp->type == TOK822_STARTGRP) {
			vstream_printf("%*s %s\n", indent, "", "group \":\"");
		} else {
			vstream_printf("%*s %s \"%s\"\n", indent, "",
					tp->type == TOK822_COMMENT ? "comment" :
					tp->type == TOK822_ATOM ? "atom" :
					tp->type == TOK822_QSTRING ? "quoted string" :
					tp->type == TOK822_DOMLIT ? "domain literal" :
					tp->type == TOK822_ADDR ? "address" :
					"unknown\n", vstring_str(tp->vstr));
		}
	}
}

int     main(int unused_argc, char **unused_argv)
{
	VSTRING *vp = vstring_alloc(100);
	TOK822 *list;
	VSTRING *buf = vstring_alloc(100);

#define TEST_TOKEN_LIMIT 20

	while (readlline(buf, VSTREAM_IN, (int *) 0)) {
		while (VSTRING_LEN(buf) > 0 && vstring_end(buf)[-1] == '\n') {
			vstring_end(buf)[-1] = 0;
			vstring_truncate(buf, VSTRING_LEN(buf) - 1);
		}
		if (!isatty(vstream_fileno(VSTREAM_IN)))
			vstream_printf(">>>%s<<<\n\n", vstring_str(buf));
		list = tok822_parse_limit(vstring_str(buf), TEST_TOKEN_LIMIT);
		vstream_printf("Parse tree:\n");
		tok822_print(list, 0);
		vstream_printf("\n");

		vstream_printf("Internalized:\n%s\n\n",
				vstring_str(tok822_internalize(vp, list, TOK822_STR_DEFL)));
		vstream_fflush(VSTREAM_OUT);
		vstream_printf("Externalized, no newlines inserted:\n%s\n\n",
				vstring_str(tok822_externalize(vp, list,
						TOK822_STR_DEFL | TOK822_STR_TRNC)));
		vstream_fflush(VSTREAM_OUT);
		vstream_printf("Externalized, newlines inserted:\n%s\n\n",
				vstring_str(tok822_externalize(vp, list,
						TOK822_STR_DEFL | TOK822_STR_LINE | TOK822_STR_TRNC)));
		vstream_fflush(VSTREAM_OUT);
		tok822_free_tree(list);
	}
	vstring_free(vp);
	vstring_free(buf);
	return (0);
}

#endif
#endif // !defined(ACL_MIME_DISABLE)
