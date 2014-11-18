#ifndef	__PRIVATE_FIFO_INCLUDE_H__
#define	__PRIVATE_FIFO_INCLUDE_H__

#include "stdlib/acl_fifo.h"
#include "stdlib/acl_malloc.h"

void private_fifo_init(ACL_FIFO *fifo);
ACL_FIFO *private_fifo_new(void);
void private_fifo_free(ACL_FIFO *fifo, void (*free_fn)(void *));
ACL_FIFO_INFO *private_fifo_push(ACL_FIFO *fifo, void *data);
void *private_fifo_pop(ACL_FIFO *fifo);
void private_delete_info(ACL_FIFO *fifo, ACL_FIFO_INFO *info);
void *private_fifo_head(ACL_FIFO *fifo);
void *private_fifo_tail(ACL_FIFO *fifo);
int private_fifo_size(ACL_FIFO *fifo);

#define	PRIVATE_FIFO_PUSH(_fifo, _data) do  \
{  \
	ACL_FIFO_INFO *_info;  \
	_info = (ACL_FIFO_INFO *) acl_default_malloc(__FILE__, __LINE__, sizeof(*_info));  \
	_info->data = (_data);  \
	if ((_fifo)->tail == NULL) {  \
		_info->prev = _info->next = NULL;  \
		(_fifo)->head = (_fifo)->tail = _info;  \
	} else {  \
		(_fifo)->tail->next = _info;  \
		_info->prev = (_fifo)->tail;  \
		_info->next = NULL;  \
		(_fifo)->tail = _info;  \
	}  \
	(_fifo)->cnt++;  \
} while (0)

#define	PRIVATE_FIFO_POP(_fifo, _data_ptr) do  \
{  \
	ACL_FIFO_INFO *_info;  \
	if ((_fifo)->head != NULL) {  \
		_info = (_fifo)->head;  \
		if ((_fifo)->head->next) {  \
			(_fifo)->head->next->prev = NULL;  \
			(_fifo)->head = (_fifo)->head->next;  \
		} else {  \
			(_fifo)->head = (_fifo)->tail = NULL;  \
		}  \
		(_data_ptr) = _info->data;  \
		acl_default_free(__FILE__, __LINE__, _info);  \
		(_fifo)->cnt--;  \
	} else {  \
		(_data_ptr) = NULL;  \
	}  \
} while (0)

#endif
