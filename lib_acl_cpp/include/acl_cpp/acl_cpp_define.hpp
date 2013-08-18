#pragma once

#ifndef ACL_CPP_API
# ifdef ACL_CPP_DLL
#  ifdef ACL_CPP_EXPORTS
#   define ACL_CPP_API __declspec(dllexport)
#  else
#   define ACL_CPP_API __declspec(dllimport)
#  endif
# else
#  define ACL_CPP_API
# endif
#endif
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef	WIN32
# pragma warning(disable:4251)
//# if !defined(VC2003) && !defined(VC6)
//extern "C" { FILE _iob[3] = {__iob_func()[0], __iob_func()[1], __iob_func()[2]}; }
//extern "C" { FILE _iob[3]; }
//# endif
# ifndef ssize_t
#  define ssize_t long
# endif
#else
# ifdef HAVE_MEMCACHED
#  undef	HAVE_MEMCACHED
# endif
#endif
