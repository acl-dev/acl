/*++
 * NAME
 *	tok822_tree 3
 * SUMMARY
 *	assorted token tree operators
 * SYNOPSIS
 *	#include <tok822.h>
 *
 *	TOK822	*tok822_append(t1, t2)
 *	TOK822	*t1;
 *	TOK822	*t2;
 *
 *	TOK822	*tok822_prepend(t1, t2)
 *	TOK822	*t1;
 *	TOK822	*t2;
 *
 *	TOK822	*tok822_cut_before(tp)
 *	TOK822	*tp;
 *
 *	TOK822	*tok822_cut_after(tp)
 *	TOK822	*tp;
 *
 *	TOK822	*tok822_unlink(tp)
 *	TOK822	*tp;
 *
 *	TOK822	*tok822_sub_append(t1, t2)
 *	TOK822	*t1;
 *
 *	TOK822	*tok822_sub_prepend(t1, t2)
 *	TOK822	*t1;
 *	TOK822	*t2;
 *
 *	TOK822	*tok822_sub_keep_before(t1, t2)
 *	TOK822	*tp;
 *
 *	TOK822	*tok822_sub_keep_after(t1, t2)
 *	TOK822	*tp;
 *
 *	int	tok822_apply(list, type, action)
 *	TOK822	*list;
 *	int	type;
 *	int	(*action)(TOK822 *token);
 *
 *	int	tok822_grep(list, type)
 *	TOK822	*list;
 *	int	type;
 *
 *	TOK822	*tok822_free_tree(tp)
 *	TOK822	*tp;
 * DESCRIPTION
 *	This module manipulates trees of token structures. Trees grow
 *	to the right or downwards. Operators are provided to cut and
 *	combine trees in various manners.
 *
 *	tok822_append() appends the token list \fIt2\fR to the right
 *	of token list \fIt1\fR. The result is the last token in \fIt2\fR.
 *	The appended list inherits the \fIowner\fR attribute from \fIt1\fR.
 *	The parent node, if any, is not updated.
 *
 *	tok822_prepend() inserts the token list \fIt2\fR to the left
 *	of token \fIt1\fR. The result is the last token in \fIt2\fR.
 *	The appended list inherits the \fIowner\fR attribute from \fIt1\fR.
 *	The parent node, if any, is not updated.
 *
 *	tok822_cut_before() breaks a token list on the left side of \fItp\fR
 *	and returns the left neighbor of \tItp\fR.
 *
 *	tok822_cut_after() breaks a token list on the right side of \fItp\fR
 *	and returns the right neighbor of \tItp\fR.
 *
 *	tok822_unlink() disconnects a token from its left and right neighbors
 *	and returns the left neighbor of \tItp\fR.
 *
 *	tok822_sub_append() appends the token list \fIt2\fR to the right
 *	of the token list below \fIt1\fR. The result is the last token
 *	in \fIt2\fR.
 *
 *	tok822_sub_prepend() prepends the token list \fIt2\fR to the left
 *	of the token list below \fIt1\fR. The result is the last token
 *	in \fIt2\fR.
 *
 *	tok822_sub_keep_before() keeps the token list below \fIt1\fR on the
 *	left side of \fIt2\fR and returns the tail of the disconnected list.
 *
 *	tok822_sub_keep_after() keeps the token list below \fIt1\fR on the
 *	right side of \fIt2\fR and returns the head of the disconnected list.
 *
 *	tok822_apply() applies the specified action routine to all tokens
 *	matching the given type (to all tokens when a null type is given).
 *	Processing terminates when the action routine returns a non-zero
 *	value. The result is the last result returned by the action routine.
 *	tok822_apply() does not traverse vertical links.
 *
 *	tok822_grep() returns a null-terminated array of pointers to tokens
 *	matching the specified type (all tokens when a null type is given).
 *	tok822_grep() does not traverse vertical links. The result must be
 *	given to myfree().
 *
 *	tok822_free_tree() destroys a tree of token structures and
 *	conveniently returns a null pointer.
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

/* Utility library. */
#include "acl_stdafx.hpp"

#if !defined(ACL_MIME_DISABLE)

#include "stdlib/acl_mymalloc.h"

/* Global library. */

#include "tok822.hpp"

/* tok822_append - insert token list, return end of inserted list */

TOK822 *tok822_append(TOK822 *t1, TOK822 *t2)
{
    TOK822 *next = t1->next;

    t1->next = t2;
    t2->prev = t1;

    t2->owner = t1->owner;
    while (t2->next)
	(t2 = t2->next)->owner = t1->owner;

    t2->next = next;
    if (next)
	next->prev = t2;
    return (t2);
}

/* tok822_prepend - insert token list, return end of inserted list */

TOK822 *tok822_prepend(TOK822 *t1, TOK822 *t2)
{
	TOK822 *prev = t1->prev;

	if (prev)
	prev->next = t2;
	t2->prev = prev;

	t2->owner = t1->owner;
	while (t2->next)
	(t2 = t2->next)->owner = t1->owner;

	t2->next = t1;
	t1->prev = t2;
	return (t2);
}

/* tok822_cut_before - split list before token, return predecessor token */

TOK822 *tok822_cut_before(TOK822 *tp)
{
    TOK822 *prev = tp->prev;

    if (prev) {
	prev->next = 0;
	tp->prev = 0;
    }
    return (prev);
}

/* tok822_cut_after - split list after token, return successor token */

TOK822 *tok822_cut_after(TOK822 *tp)
{
    TOK822 *next = tp->next;

    if (next) {
	next->prev = 0;
	tp->next = 0;
    }
    return (next);
}

/* tok822_unlink - take token away from list, return predecessor token */

TOK822 *tok822_unlink(TOK822 *tp)
{
    TOK822 *prev = tp->prev;
    TOK822 *next = tp->next;

    if (prev)
	prev->next = next;
    if (next)
	next->prev = prev;
    tp->prev = tp->next = 0;
    return (prev);
}

/* tok822_sub_append - append sublist, return end of appended list */

TOK822 *tok822_sub_append(TOK822 *t1, TOK822 *t2)
{
    if (t1->head) {
	return (t1->tail = tok822_append(t1->tail, t2));
    } else {
	t1->head = t2;
	while (t2->next)
	    (t2 = t2->next)->owner = t1;
	return (t1->tail = t2);
    }
}

/* tok822_sub_prepend - prepend sublist, return end of prepended list */

TOK822 *tok822_sub_prepend(TOK822 *t1, TOK822 *t2)
{
    TOK822 *tp;

    if (t1->head) {
	tp = tok822_prepend(t1->head, t2);
	t1->head = t2;
	return (tp);
    } else {
	t1->head = t2;
	while (t2->next)
	    (t2 = t2->next)->owner = t1;
	return (t1->tail = t2);
    }
}

/* tok822_sub_keep_before - cut sublist, return tail of disconnected list */

TOK822 *tok822_sub_keep_before(TOK822 *t1, TOK822 *t2)
{
    TOK822 *tail = t1->tail;

    if ((t1->tail = tok822_cut_before(t2)) == 0)
	t1->head = 0;
    return (tail);
}

/* tok822_sub_keep_after - cut sublist, return head of disconnected list */

TOK822 *tok822_sub_keep_after(TOK822 *t1, TOK822 *t2)
{
    TOK822 *head = t1->head;

    if ((t1->head = tok822_cut_after(t2)) == 0)
	    t1->tail = 0;
    return (head);
}

/* tok822_free_tree - destroy token tree */

TOK822 *tok822_free_tree(TOK822 *tp)
{
	if (tp) {
		if (tp->next)
			tok822_free_tree(tp->next);
		if (tp->head)
			tok822_free_tree(tp->head);
		tok822_free(tp);
	}
	return (0);
}

/* tok822_apply - apply action to specified tokens */

int     tok822_apply(TOK822 *tree, int type, TOK822_ACTION action)
{
	TOK822 *tp;
	int     result = 0;

	for (tp = tree; tp; tp = tp->next) {
		if (type == 0 || tp->type == type)
			if ((result = action(tp)) != 0)
				break;
	}
	return (result);
}

/* tok822_grep - list matching tokens */

TOK822 **tok822_grep(TOK822 *tree, int type)
{
	TOK822 **list;
	TOK822 *tp;
	int     count;

	for (count = 0, tp = tree; tp; tp = tp->next)
		if (type == 0 || tp->type == type)
			count++;

	list = (TOK822 **) acl_mymalloc(sizeof(*list) * (count + 1));

	for (count = 0, tp = tree; tp; tp = tp->next)
		if (type == 0 || tp->type == type)
			list[count++] = tp;

	list[count] = 0;
	return (list);
}

#endif // !defined(ACL_MIME_DISABLE)
