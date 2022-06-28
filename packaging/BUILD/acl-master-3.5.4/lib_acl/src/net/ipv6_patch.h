#ifndef ACL_IPV6_PATCH_INCLUDE_H
#define ACL_IPV6_PATCH_INCLUDE_H

#if 1
#if defined(AF_INET6)
# define ACL_IPV6
#endif
#endif

#if defined(ACL_IPV6)
# define ACL_AF_INET		AF_INET6
# define acl_sockaddr_in	sockaddr_in6
# define acl_sin_family		sin6_family
# define acl_sin_addr		sin6_addr
# define acl_sin_port		sin6_port
#else
# define ACL_AF_INET		AF_INET
# define acl_sockaddr_in	sockaddr_in
# define acl_sin_family		sin_family
# define acl_sin_addr		sin_addr
# define acl_sin_port		sin_port
#endif

#endif
