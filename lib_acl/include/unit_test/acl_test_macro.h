#ifndef	ACL_TEST_MACRO_INCLUDE_H
#define	ACL_TEST_MACRO_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#define AUT_SET_STR(__test_line__, __name__, __value__) do {  \
	__value__ = aut_line_getvalue(__test_line__, __name__);  \
	if (__value__ == NULL) {  \
		printf("%s(%d): getvalue error for %s, line=%d\n",  \
			__FILE__, __LINE__,  \
			__name__, aut_line_number(__test_line__));  \
		return (-1);  \
	}  \
} while (0)

#define AUT_SET_INT(__test_line__, __name__, __value__) do {  \
	const char *__ptr__;  \
	__ptr__ = aut_line_getvalue(__test_line__, __name__);  \
	if (__ptr__ == NULL) {  \
		printf("%s(%d): getvalue error for %s, line=%d\n",  \
			__FILE__, __LINE__,  \
			__name__, aut_line_number(__test_line__));  \
		return (-1);  \
	}  \
	__value__ = atoi(__ptr__);  \
} while (0)

#define AUT_RETURN_ERROR(__test_line__) do {  \
	printf("%s(%d): %s error, line=%d\n",  \
		__FILE__, __LINE__,  \
		aut_line_cmdname(__test_line__),  \
		aut_line_number(__test_line__));  \
	return (-1);  \
} while (0)

#ifdef	__cplusplus
}
#endif

#endif

