#ifndef	_YPIPI_CINLUDE_H
#define	_YPIPI_CINLUDE_H

typedef struct YPIPE YPIPE;

YPIPE *ypipe_new(void);
int    ypipe_check_read(YPIPE *self);
void  *ypipe_read(YPIPE *self);
void   ypipe_write(YPIPE *self, void *data);
int    ypipe_flush(YPIPE *self);
void   ypipe_free(YPIPE *self, void(*free_fun)(void*));

#ifdef __cplusplus
}
#endif
#endif
