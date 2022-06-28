#ifndef	__HTTP_REDIRECT_INCLUDE_H__
#define	__HTTP_REDIRECT_INCLUDE_H__

typedef struct {
	char *domain_from;
	char *domain_to;
	int   size_from;
} HTTP_DOMAIN_MAP;

void http_redirect_init(void);
void http_redirect_end(void);
HTTP_DOMAIN_MAP *http_redirect_lookup(const char *domain);

#endif
