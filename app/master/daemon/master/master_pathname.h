#ifndef	__MASTER_PATHNAME_INCLUDE_H__
#define	__MASTER_PATHNAME_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

extern char *acl_master_pathname(const char *queue_path,
	const char *service_class, const char *service_name);

#ifdef	__cplusplus
}
#endif

#endif

