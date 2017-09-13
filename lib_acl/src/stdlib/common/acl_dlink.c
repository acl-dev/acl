#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef ACL_UNIX
#include <ctype.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_array.h"
#include "stdlib/acl_dlink.h"

#endif

static void dlink_free_callback(void *arg)
{
	ACL_DITEM *pitem;

	if(arg == NULL)
		return;
	pitem = (ACL_DITEM *) arg;
	pitem->pnode = NULL;	/* sanity set to be null */
	
	acl_myfree(pitem);
}

/*
 * 功能:	找出某个长整数在数组中的下标范围位置
 * 参数:
 * a:		结构数组指针
 * n:		长整数
 * 返回值:	下标索引, 该索引满足如下条件:
 *		(idx >= 0 && idx < a->count - 1
 *		 && n >= a->items[idx]->begin
 *		 && n <  a->item[a->count - 1]->begin )
 *		or (idx == 0 && n < a->item[idx]->begin)
 *		or (idx == a->count - 1 && n >= a->item[idx]->begin)
 * 失败:	-1
 * 成功:	>= 0
 * 说明:	if idx == 0 ----> 说明在数组的开始位置添加或在开始位置前插入
 * 		if idx > 0 && idx <= a->count - 1 -----> 说明在数组的中间的某一位置
 *		if idx > a->count - 1 -----> 说明在数组的最后一位置添加
 */
static int scope_pos(const ACL_ARRAY *a, acl_int64 n)
{
	ACL_DITEM *pitem_left, *pitem_right;
	int lidx, hidx, midx, ridx, idx;

	lidx  = 0;
	hidx = acl_array_size(a) - 1;
	ridx = 0;
	idx  = 0;
	while(lidx <= hidx) {
		/*
		 * find the postion where n >= item_low->begin
		 * and n < item_high->begin
		 * NOTICE:
		 * item_low->begin <= item_low->end < item_high->begin !!!!
		 * because the dlink is sorted correctly, or some error must happen
		 * if(item_low->begin == item_high->begin
		 *   || item_low->end == item_high->begin)
		 * then we should merge them first
		 */

		midx = (lidx + hidx)/2;
		ridx = midx + 1;
		if(ridx > hidx)	{
			/*
			 * here hidx == 0 or hidx == a->count - 1, we've been
			 * out of the search scope now, so break out of the loop
			 */

			if(hidx == 0)
				idx = 0;
			else if (hidx == acl_array_size(a) - 1)
				idx = acl_array_size(a) - 1;
			else	/* an error happens */
				idx = -1;
			break;
		}

		pitem_left = (ACL_DITEM *) acl_array_index(a, midx);
		pitem_right = (ACL_DITEM *) acl_array_index(a, ridx);

		if(n >= pitem_left->begin && n < pitem_right->begin) {
			idx = midx;	/* find it :) */
			break;
		}
		/* not find, continue...... */
		if(n >= pitem_right->begin)
			lidx = ridx;
		else if(n < pitem_left->begin)
			hidx = midx;
		else {	/* why does the array not to be sorted ? */
			idx = -1;
			break;
		}
	}

	return idx;
}

static int begin_pos(const ACL_ARRAY *a, acl_int64 n)
{
	return scope_pos(a, n);
}

static int end_pos(const ACL_ARRAY *a, acl_int64 n)
{
	return scope_pos(a, n);
}

#ifdef	_USE_PRED_INSERT_
static ACL_DITEM *dlink_pred_insert(ACL_ARRAY *a, int idx_position,
	acl_int64 begin, acl_int64 end)
{
	ACL_DITEM *pitem;
	int ret;

	pitem = acl_mymalloc(sizeof(ACL_DITEM));
	if(pitem == NULL)
		return NULL;
	pitem->begin = begin;
	pitem->end   = end;
	ret = acl_array_pred_insert(a, idx_position, pitem);
	if(ret < 0) {
		acl_myfree(pitem);
		return NULL;
	}
	pitem->pnode = NULL;

	return pitem;
}
#endif

static ACL_DITEM *dlink_succ_insert(ACL_ARRAY *a, int idx_position,
	acl_int64 begin, acl_int64 end)
{
	ACL_DITEM *pitem;
	int ret;

	pitem = (ACL_DITEM *) acl_mymalloc(sizeof(ACL_DITEM));
	if(pitem == NULL)
		return NULL;
	pitem->begin = begin;
	pitem->end = end;
	ret = acl_array_succ_insert(a, idx_position, pitem);
	if(ret < 0) {
		acl_myfree(pitem);
		return NULL;
	}
	pitem->pnode = NULL;

	return pitem;
}

static ACL_DITEM *dlink_append(ACL_ARRAY *a, acl_int64 begin, acl_int64 end)
{
	ACL_DITEM *pitem;
	int ret;

	pitem = (ACL_DITEM *) acl_mymalloc(sizeof(ACL_DITEM));
	if(pitem == NULL)
		return NULL;
	pitem->begin = begin;
	pitem->end   = end;
	ret = acl_array_append(a, pitem);
	if(ret < 0) {
		acl_myfree(pitem);
		return NULL;
	}
	pitem->pnode = NULL;

	return pitem;
}

static ACL_DITEM *dlink_prepend(ACL_ARRAY *a, acl_int64 begin, acl_int64 end)
{
	ACL_DITEM *pitem;
	int ret;

	pitem = (ACL_DITEM *) acl_mymalloc(sizeof(ACL_DITEM));
	if(pitem == NULL)
		return NULL;
	pitem->begin = begin;
	pitem->end   = end;
	ret = acl_array_prepend(a, pitem);
	if(ret < 0) {
		acl_myfree(pitem);
		return NULL;
	}
	pitem->pnode = NULL;

	return pitem;
}

static int dlink_node_merge(ACL_ARRAY *a, int idx_obj_begin, int idx_src_begin)
{
	int ret;

	if(idx_obj_begin >= idx_src_begin)
		return 0;

	ret = acl_array_mv_idx(a, idx_obj_begin, idx_src_begin, dlink_free_callback);
	if(ret < 0)
		return -1;

	return 0;
}

static ACL_DITEM *dlink_add(ACL_ARRAY *a, acl_int64 begin, acl_int64 end)
{
	ACL_DITEM *pitem_right, *pitem_left, *pitem;
	int idx_begin, idx_end;
	int	ret;

	/* sanity check, maybe useless */
	/* because it's used internal */
	if(begin > end)
		return NULL;

	idx_begin = begin_pos(a, begin);
	if(idx_begin < 0 || idx_begin >= acl_array_size(a)) /* an error happened */
		return NULL;

	idx_end   = end_pos(a, end);
	if(idx_end < 0 || idx_end >= acl_array_size(a))	/* an error happened */
		return NULL;

	if(idx_begin > idx_end)	/* an error happened */
		return NULL;

	if(acl_array_size(a) == 0) {	/* the d-link is empty so just add one :) */
		pitem = dlink_append(a, begin, end);
		return pitem;
	}

	pitem_left  = (ACL_DITEM *) acl_array_index(a, idx_begin);
	pitem_right = (ACL_DITEM *) acl_array_index(a, idx_end);

	/* if idx_end == 0 then idx_begin must be equal to 0, I'm sure it :) */
	if (idx_begin == idx_end) {
		/*
		 * pitem_left == pitem_right
		 * here idx_begin maybe one of: 0, a->count - 1,
		 * or the one between 0 and a->count
		 * this is to say the begin and end is on the same d-link
		 */

		if (end + 1 < pitem_left->begin) {
			/*
			 * here idx_begin == idx_end must be equal to 0
			 * the begin and the end must be less
			 * than the next node's begin
			 * add one new node before the one
			 */
			pitem = dlink_prepend(a, begin, end);
			return pitem;
		}

		if (begin > pitem_left->end + 1) {
			/*
			 * this is to say begin and end
			 * between the current node's end
			 * the next node's begin, and we just
			 * insert one new node between the
			 * current node and the next node, when
			 * the next node is NULL(which say that
			 * idx_begin == idx_end == a->count - 1),
			 * just insert one new node after
			 * the the last node
			 */
			pitem = dlink_succ_insert(a, idx_begin, begin, end);
			return pitem;
		}

		/*
		 * here idx_begin == idx_end must in the middle
		 * here begin <= pitem_left->end,
		 * and end < the next node's begin,
		 * and end >= pitem_left->begin
		 */

		if (begin < pitem_left->begin)
			/*
			 * just merge, which happens when
			 * idx_begin == idx_end == 0 && begin < pitem_left->begin
			 * && end >= pitem_left->begin
			 */
			pitem_left->begin = begin;

		if (end > pitem_left->end)
			/*
			 * just merge, pitem_left->begin <= pitem_left->end
			 * pitem_left->begin <= end < pitem_right->end
			 * or pitem_right == NULL
			 */
			pitem_left->end = end;

		return pitem_left;
	}

	/*
	 * idx_end > idx_begin, idx_begin >= 0, idx_end <= a->count - 1
	 * ===> 0 <= idx_begin < idx_end <= a->count - 1;
	 *
	 * idx_end > idx_begin, so the begin and end
	 * are separately in defferent node,
	 * because the d-link is sored correctly:).
	 * ===> I just only merge them--the reason is shown below:
	 * idx_end > idx_begin, d-link is sorted correctly
	 * ===>	pitem_left->begin < pitem_right->begin <= end,
	 *	begin < pitem_right->begin;
	 */

	if (begin < pitem_left->begin) {
		/*
		 * in the first position of the array
		 * idx_begin == 0 and idx_end >= 1
		 * just merge :)
		 */

		pitem_left->begin = begin;
	} 

	/*
	 * ===>  pitem_left->begin
	 * ===>  <= begin
	 * ===>  ......
	 * ===>  <  pitem_right->begin
	 * ===>  <= end
	 */

	if (begin <= pitem_left->end + 1) {
		/*
		 * ===>  pitem_left->begin
		 * ===>  <= begin
		 * +++>  <= pitem_left->end
		 * ===>  <  pitem_right->begin
		 * ===>  <= end
		 * ===>  so, just merge the nodes between the
		 * ===>  pitem_left node and the pitem_right node,
		 * ===>  and include both of them
		 */

		/*
		 * merge the pitem_left node, begin ---> end into pitem_right
		 * node, and merge all nodes into one node which are between 
		 * the pitem_left node and the pitem_right node,
		 * include both of pitem_left node and pitem_right node
		 */

		if (end > pitem_right->end) {
			/*
			 * ===>  pitem_left->begin
			 * ===>  <= begin
			 * ===>  <= pitem_left->end
			 * ===>  <  pitem_right->begin
			 * +++>  <= pitem_right->end
			 * ===>  <  end
			 */

			pitem_right->end = end;
		}
		/*
		 * else
		 * ===>  pitem_left->begin
		 * ===>  <= begin
		 * ===>  <= pitem_left->end
		 * ===>  <  pitem_right->begin
		 * ===>  <= end
		 * +++>  <= pitem_right->end
		 */

		pitem_right->begin = pitem_left->begin;

		ret = dlink_node_merge(a, idx_begin, idx_end);
		if(ret < 0)
			return NULL;

		return pitem_right;
	}

	/*
	 * ===>  pitem_left->begin
	 * ===>  <= pitem_left->end
	 * ===>  <  begin
	 * ===>  <  pitem_right->begin
	 * ===>  <= end
	 *
	 * merge the begin--->end into pitem_right node
	 * and at the same time, merge all the nodes into idx_begin + 1
	 * which between the idx_begin + 1 node and the pitem_right node,
	 * include idx_begin + 1 node and pitem_righ node
	 */

	if (end > pitem_right->end) {
		/*
		 * ===>  pitem_left->begin
		 * ===>  <= pitem_left->end
		 * ===>  <  begin
		 * ===>  <  pitem_right->begin
		 * +++>  <= pitem_right->end
		 * ===>  <  end
		 */

		pitem_right->end = end;
	}
	/*
	 * else
	 * ===>  pitem_left->begin
	 * ===>  <= pitem_left->end
	 * ===>  <  begin
	 * ===>  <  pitem_right->begin
	 * ===>  <  end
	 * +++>  <= pitem_right->end
	 */

	pitem_right->begin = begin;

	ret = dlink_node_merge(a, idx_begin + 1, idx_end);
	if(ret < 0)
		return NULL;

	return pitem_right;
}

static void *dlink_iter_head(ACL_ITER *iter, struct ACL_DLINK *dlink)
{
	return dlink->parray->iter_head(iter, dlink->parray);
}

static void *dlink_iter_next(ACL_ITER *iter, struct ACL_DLINK *dlink)
{
	return dlink->parray->iter_next(iter, dlink->parray);
}

static void *dlink_iter_tail(ACL_ITER *iter, struct ACL_DLINK *dlink)
{
	return dlink->parray->iter_tail(iter, dlink->parray);
}

static void *dlink_iter_prev(ACL_ITER *iter, struct ACL_DLINK *dlink)
{
	return dlink->parray->iter_prev(iter, dlink->parray);
}

ACL_DLINK *acl_dlink_create(int nsize)
{
	ACL_DLINK *plink;

	plink = (ACL_DLINK *) acl_mymalloc(sizeof(ACL_DLINK));
	plink->parray = NULL;
	plink->call_back_data = NULL;
	nsize = nsize > 0 ? nsize : 1;
	plink->parray = acl_array_create(nsize);
	if(plink->parray == NULL) {
		acl_myfree(plink);
		return NULL;
	}

	plink->iter_head = dlink_iter_head;
	plink->iter_next = dlink_iter_next;
	plink->iter_tail = dlink_iter_tail;
	plink->iter_prev = dlink_iter_prev;

	return plink;
}

void acl_dlink_free(ACL_DLINK *plink)
{
	if(plink == NULL)
		return;
	if(plink->parray)
		acl_array_destroy(plink->parray, dlink_free_callback);
	acl_myfree(plink);
}

ACL_DITEM *acl_dlink_lookup_by_item(const ACL_DLINK *plink, ACL_DITEM *pitem)
{
	return acl_dlink_lookup2_by_item(plink, pitem, NULL);
}

ACL_DITEM *acl_dlink_lookup2_by_item(const ACL_DLINK *plink, ACL_DITEM *pitem, int *pidx)
{
	int i;
	ACL_ARRAY *parray = plink->parray;

	for(i = 0; i < acl_array_size(parray) - 1; i++) {
		if((ACL_DITEM *) acl_array_index(parray, i) == pitem) {
			if (pidx)
				*pidx = i;
			return pitem;
		}
	}
	return NULL;
}

ACL_DITEM *acl_dlink_lookup(const ACL_DLINK *plink, acl_int64 n)
{
	return acl_dlink_lookup2(plink, n, NULL);
}

ACL_DITEM *acl_dlink_lookup2(const ACL_DLINK *plink, acl_int64 n, int *pidx)
{
	int lidx, midx, hidx;

	lidx = 0;
	hidx = acl_array_size(plink->parray) - 1;
	while(lidx <= hidx) {
		ACL_DITEM* pitem;

		midx  = (lidx + hidx) / 2;
		pitem = (ACL_DITEM *) acl_array_index(plink->parray, midx);
		if(n >= pitem->begin && n <= pitem->end) {
			if (pidx)
				*pidx = midx;
			return pitem;
		}
		if(n < pitem->begin)
			hidx = midx - 1;
		else if(n > pitem->end)
			lidx = midx + 1;
		else			/* why does this array not to be sorted ? */
			break;
	}

	return NULL;	/*not in the d_link scope */
}

ACL_DITEM *acl_dlink_lookup_range(const ACL_DLINK *plink, acl_int64 begin,
	acl_int64 end, int *pidx)
{
	ACL_DITEM *ditem;

	if (end < begin)
		return NULL;
	ditem = acl_dlink_lookup2(plink, begin, pidx);
	if (ditem == NULL)
		return NULL;
	if (ditem->end >= end)
		return ditem;
	return NULL;
}

ACL_DITEM *acl_dlink_lookup_larger(const ACL_DLINK *plink,
	acl_int64 off, int *pidx)
{
	int   i, size;

	size = acl_array_size(plink->parray);

	for (i = 0; i < size; i++) {
		ACL_DITEM* pitem = (ACL_DITEM*)
			acl_array_index(plink->parray, i);
		if (pitem->end >= off) {
			if (pidx)
				*pidx = i;
			return pitem;
		}
	}

	return NULL;	/*not in the d_link scope */
}

ACL_DITEM *acl_dlink_lookup_lower(const ACL_DLINK *plink,
	acl_int64 off, int *pidx)
{
	int   i, size;

	size = acl_array_size(plink->parray);

	for (i = size - 1; i >= 0; i--) {
		ACL_DITEM* pitem = (ACL_DITEM*)
			acl_array_index(plink->parray, i);
		if (pitem->begin <= off) {
			if (pidx)
				*pidx = i;
			return pitem;
		}
	}

	return NULL;	/*not in the d_link scope */
}

ACL_DITEM *acl_dlink_insert(ACL_DLINK *plink, acl_int64 begin, acl_int64 end)
{
	if (begin > end) {
		acl_int64 tmp;
		/* swap the begin and end if end < begin */
		tmp   = begin;
		begin = end;
		end   = tmp;
	}

	if(acl_array_size(plink->parray) == 0) {
		/* this is the first item of the array */
		return dlink_append(plink->parray, begin, end);
	}

	/* 此函数内有可能进行了结点项的合并 */
	return dlink_add(plink->parray, begin, end);
}

int acl_dlink_delete(ACL_DLINK *plink, acl_int64 n)
{
	const ACL_DITEM *ditem;
	int  idx;

	ditem = acl_dlink_lookup2(plink, n, &idx);
	if (ditem == NULL)
		return -1;
	acl_array_delete_idx(plink->parray, idx, dlink_free_callback);
	return 0;
}

int acl_dlink_delete_by_item(ACL_DLINK *plink, ACL_DITEM *pitem)
{
	int ret;

	ret = acl_array_delete_obj(plink->parray, pitem, dlink_free_callback);
	if (ret < 0)	/* this is impossile, but a sanity check */
		return -1;
	return 0;
}

int acl_dlink_delete_range(ACL_DLINK *plink, acl_int64 begin, acl_int64 end)
{
	ACL_ARRAY *parray = plink->parray;
	ACL_DITEM *pitem, *pitem_low;
	int   i, low, high, size;

	low = 0;
	pitem_low = NULL;
	size = acl_array_size(parray);

	for (i = 0; i < size; i++) {
		pitem = (ACL_DITEM*) acl_array_index(parray, i);
		if (begin > pitem->end)
			continue;

		/* begin <= pitem->end */
		
		if (begin <= pitem->begin) {
			/* begin <= pitem->begin <= pitem->end */
			if (end < pitem->end) {
				/* begin <= pitem->begin <= end < pitem->end */
				pitem->begin = end + 1;
				return 0;
			} else if (end == pitem->end) {
				/* begin <= pitem->begin <= pitem->end == end */
				acl_array_delete_idx(parray, i, dlink_free_callback);
				return 0;
			}
			/* begin <= pitem->begin <= pitem->end < end */
			low = i;
		}

		/* pitem->begin < begin <= pitem->end */

		else if (end == pitem->end) {
			/* pitem->begin < begin <= end == pitem->end */
			pitem->end = begin - 1;
			return 0;
		} else if (end < pitem->end) {
			/* pitem->begin < begin <= end < pitem->end */
			acl_int64 tmp_begin, tmp_end;

			tmp_begin = end + 1;
			tmp_end = pitem->end;
			pitem->end = begin - 1;
			/* add one ditem hole */
			dlink_add(parray, tmp_begin, tmp_end);
			return 0;
		} else {
			/* pitem->begin < begin <= pitem->end < end */
			pitem->end = begin - 1;
			low = i + 1;
			if (low >= size)  /* i is the last item's idx */
				return 0;
			pitem_low = (ACL_DITEM*) acl_array_index(parray, low);
			if (pitem_low->begin > end)
				return 0;
			/* begin < pitem_low->begin <= end */
			if (end < pitem_low->end) {
				/* begin < pitem_low->begin <= end < pitem_low->end */
				pitem_low->begin = end + 1;
				return 0;
			} else if (end == pitem_low->end) {
				/* begin < pitem_low->begin <= pitem_low->end == end */
				acl_array_delete_idx(parray, low, dlink_free_callback);
				return 0;
			}

			/* begin < pitem_low->begin <= pitem_low->end < end */
		}
		break;
	}

	high = size - 1;
	for (i = low + 1; i < size; i++) {
		pitem = (ACL_DITEM*) acl_array_index(parray, i);
		if (end > pitem->end)
			continue;

		/* end <= pitem->end */

		if (end < pitem->begin) {
			/* end < pitem->begin <= pitem->end */
			high = i - 1;
		} else if (end == pitem->begin) {
			/* end == pitem->begin <= pitem->end */
			if (pitem->begin == pitem->end) {
				/* end == pitem->begin == pitem->end */
				high = i;
			} else {
				/* end == pitem->begin < pitem->end */
				pitem->begin = end + 1;
				high = i - 1;
			}
		}

		/* pitem->begin < end <= pitem->end */

		else if (end == pitem->end) {
			/* pitem->begin < end == pitem->end */
			high = i;
		} else {
			/* pitem->begin < end < pitem->end */
			pitem->begin = end + 1;
			high = i - 1;
		}
		break;
	}

	if (low > high)
		return 0;

	return acl_array_delete_range(parray, low, high, dlink_free_callback);
}

ACL_DITEM *acl_dlink_modify(ACL_DLINK *plink, acl_int64 begin, acl_int64 end)
{
	if (begin > end) {
		acl_int64 tmp;
		/* swap the begin andend if end < begin */
		tmp   = begin;
		begin = end;
		end   = tmp;
	}

	return dlink_add(plink->parray, begin, end);
}

ACL_DITEM *acl_dlink_index(const ACL_DLINK *plink, int idx)
{
	return (ACL_DITEM *) acl_array_index(plink->parray, idx);
}

int acl_dlink_size(const ACL_DLINK *plink)
{
	return acl_array_size(plink->parray);
}

/* ++++++++++++++++++++++++++below functions are used only for test ++++++++++++ */

int acl_dlink_list(const ACL_DLINK *plink)
{
	const char *myname = "acl_dlink_list";
	int   i, n;
	ACL_DITEM *item;

	if(plink == NULL || plink->parray == NULL) {
		printf("%s: input error\r\n", myname);
		return-1;
	}

	n = acl_array_size(plink->parray);
	for (i = 0; i < n; i++) {
		item = (ACL_DITEM *) acl_array_index(plink->parray, i);
		if (item == NULL)
			break;
		printf("begin=" ACL_FMT_I64D ", end=" ACL_FMT_I64D "\r\n",
			item->begin, item->end);
	}
	return 0;
}
