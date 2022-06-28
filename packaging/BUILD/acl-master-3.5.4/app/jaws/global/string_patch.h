#ifndef	__STRING_PATCH_INCLUDE_H__
#define	__STRING_PATCH_INCLUDE_H__

#ifdef	__cplusplus
extern "C"
#endif

#ifdef	HAVE_NO_STRCASESTR
char *strcasestr(char *haystack, char *needle);
#endif

#ifdef	__cplusplus
}
#endif

#endif
