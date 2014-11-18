#ifndef STR_H
#define STR_H

extern unsigned int str_copy(char *,char *);
extern int str_diff(char *,char *);
extern int str_diffn(char *,char *,unsigned int);
extern unsigned int str_len(char *);
extern unsigned int str_chr(char *,int);
extern unsigned int str_rchr(char *,int);
extern int str_start(char *,char *);

#define str_equal(s,t) (!str_diff((s),(t)))

#endif
