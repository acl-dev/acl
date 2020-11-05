#pragma once

#if defined(_WIN32) || defined (_WIN64)
/* typedef intptr_t ssize_t; */
# ifndef	HAS_SSIZE_T
#  define	HAS_SSIZE_T
/* typedef intptr_t ssize_t; */
#  if defined(_WIN64)
typedef __int64 ssize_t;
#  elif defined(_WIN32)
typedef int ssize_t;
#  else
typedef long ssize_t;
#  endif
# endif
#else
#include <sys/types.h>
#include <sys/socket.h>
#endif

#ifdef FIBER_CPP_LIB
# ifndef FIBER_CPP_API
#  define FIBER_CPP_API
# endif
#elif defined(FIBER_DLL) // || defined(_WINDLL)
# if defined(FIBER_CPP_EXPORTS) || defined(fibercpp_EXPORTS)
#  ifndef FIBER_CPP_API
#   define FIBER_CPP_API __declspec(dllexport)
#  endif
# elif !defined(FIBER_CPP_API)
#  define FIBER_CPP_API __declspec(dllimport)
# endif
#elif !defined(FIBER_CPP_API)
# define FIBER_CPP_API
#endif
