#ifndef	__ACL_METER_TIME_INCLUDE_H__
#define	__ACL_METER_TIME_INCLUDE_H_

#ifdef	__cplusplus
extern "C" {
#endif

ACL_API double acl_meter_time(const char *filename, int line, const char *info);

#define	ACL_METER_TIME(info)	acl_meter_time(__FILE__, __LINE__, info)

#ifdef	__cplusplus
}
#endif

#endif
