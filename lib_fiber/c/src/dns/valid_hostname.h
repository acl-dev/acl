#ifndef __VALID_HOSTNAME_INCLUDE_H__
#define __VALID_HOSTNAME_INCLUDE_H__

#ifdef  __cplusplus
extern "C" {
#endif

 /* External interface */

#define VALID_HOSTNAME_LEN	255	/* RFC 1035 */
#define VALID_LABEL_LEN		63	/* RFC 1035 */

#define DONT_GRIPE		0
#define DO_GRIPE		1

int valid_hostname(const char *, int);
int valid_hostaddr(const char *, int);
int valid_ipv4_hostaddr(const char *, int);
int valid_ipv6_hostaddr(const char *, int);

#ifdef  __cplusplus
}
#endif

#endif

