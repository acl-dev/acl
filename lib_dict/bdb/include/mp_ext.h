/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_mp_ext_h_
#define	_mp_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __memp_alloc __P((DB_MPOOL *, REGINFO *, MPOOLFILE *, size_t, roff_t *, void *));
#ifdef DIAGNOSTIC
void __memp_check_order __P((DB_MPOOL_HASH *));
#endif
int __memp_bhwrite __P((DB_MPOOL *, DB_MPOOL_HASH *, MPOOLFILE *, BH *, int));
int __memp_pgread __P((DB_MPOOLFILE *, DB_MUTEX *, BH *, int));
int __memp_pg __P((DB_MPOOLFILE *, BH *, int));
void __memp_bhfree __P((DB_MPOOL *, DB_MPOOL_HASH *, BH *, u_int32_t));
int __memp_fget_pp __P((DB_MPOOLFILE *, db_pgno_t *, u_int32_t, void *));
int __memp_fget __P((DB_MPOOLFILE *, db_pgno_t *, u_int32_t, void *));
int __memp_fcreate_pp __P((DB_ENV *, DB_MPOOLFILE **, u_int32_t));
int __memp_fcreate __P((DB_ENV *, DB_MPOOLFILE **));
int __memp_set_clear_len __P((DB_MPOOLFILE *, u_int32_t));
int __memp_get_fileid __P((DB_MPOOLFILE *, u_int8_t *));
int __memp_set_fileid __P((DB_MPOOLFILE *, u_int8_t *));
int __memp_get_flags __P((DB_MPOOLFILE *, u_int32_t *));
int __memp_set_flags __P((DB_MPOOLFILE *, u_int32_t, int));
int __memp_get_ftype __P((DB_MPOOLFILE *, int *));
int __memp_set_ftype __P((DB_MPOOLFILE *, int));
int __memp_set_lsn_offset __P((DB_MPOOLFILE *, int32_t));
int __memp_set_pgcookie __P((DB_MPOOLFILE *, DBT *));
void __memp_last_pgno __P((DB_MPOOLFILE *, db_pgno_t *));
char * __memp_fn __P((DB_MPOOLFILE *));
char * __memp_fns __P((DB_MPOOL *, MPOOLFILE *));
int __memp_fopen_pp __P((DB_MPOOLFILE *, const char *, u_int32_t, int, size_t));
int __memp_fopen __P((DB_MPOOLFILE *, MPOOLFILE *, const char *, u_int32_t, int, size_t));
int __memp_fclose_pp __P((DB_MPOOLFILE *, u_int32_t));
int __memp_fclose __P((DB_MPOOLFILE *, u_int32_t));
int __memp_mf_discard __P((DB_MPOOL *, MPOOLFILE *));
int __memp_fput_pp __P((DB_MPOOLFILE *, void *, u_int32_t));
int __memp_fput __P((DB_MPOOLFILE *, void *, u_int32_t));
int __memp_fset_pp __P((DB_MPOOLFILE *, void *, u_int32_t));
int __memp_fset __P((DB_MPOOLFILE *, void *, u_int32_t));
void __memp_dbenv_create __P((DB_ENV *));
int __memp_get_cachesize __P((DB_ENV *, u_int32_t *, u_int32_t *, int *));
int __memp_set_cachesize __P((DB_ENV *, u_int32_t, u_int32_t, int));
int __memp_set_mp_max_openfd __P((DB_ENV *, int));
int __memp_set_mp_max_write __P((DB_ENV *, int, int));
int __memp_set_mp_mmapsize __P((DB_ENV *, size_t));
int __memp_nameop __P((DB_ENV *, u_int8_t *, const char *, const char *, const char *));
int __memp_get_refcnt __P((DB_ENV *, u_int8_t *, u_int32_t *));
int __memp_ftruncate __P((DB_MPOOLFILE *, db_pgno_t, u_int32_t));
int __memp_open __P((DB_ENV *));
int __memp_dbenv_refresh __P((DB_ENV *));
void __memp_region_destroy __P((DB_ENV *, REGINFO *));
int __memp_register_pp __P((DB_ENV *, int, int (*)(DB_ENV *, db_pgno_t, void *, DBT *), int (*)(DB_ENV *, db_pgno_t, void *, DBT *)));
int __memp_register __P((DB_ENV *, int, int (*)(DB_ENV *, db_pgno_t, void *, DBT *), int (*)(DB_ENV *, db_pgno_t, void *, DBT *)));
int __memp_stat_pp __P((DB_ENV *, DB_MPOOL_STAT **, DB_MPOOL_FSTAT ***, u_int32_t));
int __memp_stat_print_pp __P((DB_ENV *, u_int32_t));
int  __memp_stat_print __P((DB_ENV *, u_int32_t));
void __memp_stat_hash __P((REGINFO *, MPOOL *, u_int32_t *));
int __memp_sync_pp __P((DB_ENV *, DB_LSN *));
int __memp_sync __P((DB_ENV *, DB_LSN *));
int __memp_fsync_pp __P((DB_MPOOLFILE *));
int __memp_fsync __P((DB_MPOOLFILE *));
int __mp_xxx_fh __P((DB_MPOOLFILE *, DB_FH **));
int __memp_sync_int __P((DB_ENV *, DB_MPOOLFILE *, u_int32_t, db_sync_op, u_int32_t *));
int __memp_mf_sync __P((DB_MPOOL *, MPOOLFILE *));
int __memp_trickle_pp __P((DB_ENV *, int, int *));

#if defined(__cplusplus)
}
#endif
#endif /* !_mp_ext_h_ */
