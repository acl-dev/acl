#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_allocator.h"

#endif

#include "allocator.h"

void mem_pool_create(ACL_ALLOCATOR *allocator)
{
	acl_allocator_pool_add(allocator, "8 Buffer",  8,
		ACL_MEM_TYPE_8_BUF, NULL, NULL, NULL);
	acl_allocator_pool_add(allocator, "16 Buffer",  16,
		ACL_MEM_TYPE_16_BUF, NULL, NULL, NULL);
	acl_allocator_pool_add(allocator, "32 Buffer",  32,
		ACL_MEM_TYPE_32_BUF, NULL, NULL, NULL);
	acl_allocator_pool_add(allocator, "64 Buffer",  64,
		ACL_MEM_TYPE_64_BUF, NULL, NULL, NULL);
	acl_allocator_pool_add(allocator, "128 Buffer",  128,
		ACL_MEM_TYPE_128_BUF, NULL, NULL, NULL);
	acl_allocator_pool_add(allocator, "256 Buffer",  256,
		ACL_MEM_TYPE_256_BUF, NULL, NULL, NULL);
	acl_allocator_pool_add(allocator, "512 Buffer",  512,
		ACL_MEM_TYPE_512_BUF, NULL, NULL, NULL);
	acl_allocator_pool_add(allocator, "1K Buffer",  1024,
		ACL_MEM_TYPE_1K_BUF, NULL, NULL, NULL);
	acl_allocator_pool_add(allocator, "2K Buffer",  2048,
		ACL_MEM_TYPE_2K_BUF, NULL, NULL, NULL);
	acl_allocator_pool_add(allocator, "4K Buffer",  4096,
		ACL_MEM_TYPE_4K_BUF, NULL, NULL, NULL);
	acl_allocator_pool_add(allocator, "8K Buffer",  8192,
		ACL_MEM_TYPE_8K_BUF, NULL, NULL, NULL);
	acl_allocator_pool_add(allocator, "16K Buffer",  16384,
		ACL_MEM_TYPE_16K_BUF, NULL, NULL, NULL);
	acl_allocator_pool_add(allocator, "32K Buffer",  32768,
		ACL_MEM_TYPE_32K_BUF, NULL, NULL, NULL);
	acl_allocator_pool_add(allocator, "64K Buffer",  65536,
		ACL_MEM_TYPE_64K_BUF, NULL, NULL, NULL);

	acl_allocator_pool_add(allocator, "128K Buffer",  131072,
		ACL_MEM_TYPE_128K_BUF, NULL, NULL, NULL);
	acl_allocator_pool_add(allocator, "256K Buffer",  262144,
		ACL_MEM_TYPE_256K_BUF, NULL, NULL, NULL);
	acl_allocator_pool_add(allocator, "512K Buffer",  524288,
		ACL_MEM_TYPE_512K_BUF, NULL, NULL, NULL);
	acl_allocator_pool_add(allocator, "1M Buffer",  1048576,
		ACL_MEM_TYPE_1M_BUF, NULL, NULL, NULL);
}

void mem_pool_status(ACL_ALLOCATOR *allocator)
{
	int type = (int) ACL_MEM_TYPE_NONE;
	ACL_MEM_POOL *pool;

	acl_msg_info("-----------------------------------------------------------------------------------------------\r\n");
	acl_msg_info("%-20s%-26s%-25s%-15s%-10s", "Buffer type", "Allocated",
		"Freed", "Diff", "Buffered");
	acl_msg_info("-----------------------------------------------------------------------------------------------\r\n");

	for (; type < (int) ACL_MEM_TYPE_MAX; type++) {
		pool = allocator->MemPools[type];
		if (pool) {
			acl_msg_info("%-20salloc=%-20ldfree=%-20lddiff=%-10ldbuff=%-10d",
				pool->label, (long) pool->nalloc, (long) pool->nfree,
				(long) (pool->nalloc - pool->nfree),
				(int) acl_stack_size(pool->pstack));
		}
	}

	acl_msg_info("-----------------------------------------------------------------------------------------------\r\n");
}
