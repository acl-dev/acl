#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_stdlib.h"
#include "aio/acl_aio.h"

#endif

#include "aio.h"

int aio_timeout_callback(ACL_ASTREAM *astream)
{
	int   ret = 0;

	/* 需要将缓冲区清空，以防被重复利用 */
	ACL_VSTRING_RESET(&astream->strbuf);

	/* 流引用计数加1，以防止流被异常关闭 */
	astream->nrefer++;

	if (astream->timeo_handles) {
		ACL_ITER iter;
		ACL_FIFO timeo_handles;

		/* XXX: 必须将各个回调句柄从回调队列中一一提出置入一个单独队列中,
		 * 因为 ACL_AIO 在回调过程中有可能发生嵌套，防止被重复调用
		 */

		acl_fifo_init(&timeo_handles);
		acl_foreach_reverse(iter, astream->timeo_handles) {
			AIO_TIMEO_HOOK *handle = (AIO_TIMEO_HOOK*) iter.data;
			if (handle->disable)
				continue;
			acl_fifo_push(&timeo_handles, handle);
		}

		while (1) {
			AIO_TIMEO_HOOK *handle = acl_fifo_pop(&timeo_handles);
			if (handle == NULL)
				break;
			ret = handle->callback(astream, handle->ctx);
			if (ret < 0) {
				astream->nrefer--;
				return (ret);
			}
		}
	}

	astream->nrefer--;
	return (ret);
}

void aio_close_callback(ACL_ASTREAM *astream)
{
	/* 需要将缓冲区清空，以防被重复利用 */
	ACL_VSTRING_RESET(&astream->strbuf);

	/* 流引用计数加1，以防止流被异常关闭 */
	astream->nrefer++;

	if (astream->close_handles) {
		ACL_ITER iter;
		ACL_FIFO close_handles;

		/* XXX: 必须将各个回调句柄从回调队列中一一提出置入一个单独队列中,
		 * 因为 ACL_AIO 在回调过程中有可能发生嵌套，防止被重复调用
		 */

		acl_fifo_init(&close_handles);
		acl_foreach_reverse(iter, astream->close_handles) {
			AIO_CLOSE_HOOK *handle = (AIO_CLOSE_HOOK*) iter.data;
			if (handle->disable)
				continue;
			acl_fifo_push(&close_handles, handle);
		}

		while (1) {
			AIO_CLOSE_HOOK *handle = acl_fifo_pop(&close_handles);
			void *ctx;

			if (handle == NULL)
				break;
			/* xxx: 关闭回调仅能被调用一次 */
			ctx = handle->ctx;
			handle->disable = 1;
			handle->ctx = NULL;
			if (handle->callback(astream, ctx) < 0) {
				astream->nrefer--;
				return;
			}
		}
	}

	astream->nrefer--;
}
