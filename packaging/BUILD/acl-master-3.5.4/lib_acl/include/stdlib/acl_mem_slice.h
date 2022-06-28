#ifndef	ACL_MEM_SLICE_INCLUDE_H
#define	ACL_MEM_SLICE_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct ACL_MEM_SLICE ACL_MEM_SLICE;

ACL_API ACL_MEM_SLICE *acl_mem_slice_init(int base, int nslice,
	int nalloc_gc, unsigned int slice_flag);
ACL_API void acl_mem_slice_delay_destroy(void);
ACL_API void acl_mem_slice_destroy(void);
ACL_API int acl_mem_slice_gc(void);
ACL_API void acl_mem_slice_set(ACL_MEM_SLICE *mem_slice);

#ifdef	__cplusplus
}
#endif

#endif
