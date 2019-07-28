#ifndef	__passwd_crypt_include_h__
#define	__passwd_crypt_include_h__

#ifdef	__cplusplus
extern "C" {
#endif

char* passwd_crypt(const char* in);
char* passwd_decrypt(const char* in);

#ifdef	__cplusplus
}
#endif

#endif
