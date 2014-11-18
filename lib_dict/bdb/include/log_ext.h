/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_log_ext_h_
#define	_log_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __log_open __P((DB_ENV *));
int __log_find __P((DB_LOG *, int, u_int32_t *, logfile_validity *));
int __log_valid __P((DB_LOG *, u_int32_t, int, DB_FH **, u_int32_t, logfile_validity *));
int __log_dbenv_refresh __P((DB_ENV *));
void __log_get_cached_ckp_lsn __P((DB_ENV *, DB_LSN *));
void __log_region_destroy __P((DB_ENV *, REGINFO *));
int __log_vtruncate __P((DB_ENV *, DB_LSN *, DB_LSN *, DB_LSN *));
int __log_is_outdated __P((DB_ENV *, u_int32_t, int *));
int __log_inmem_lsnoff __P((DB_LOG *, DB_LSN *, size_t *));
int __log_inmem_newfile __P((DB_LOG *, u_int32_t));
int __log_inmem_chkspace __P((DB_LOG *, size_t));
void __log_inmem_copyout __P((DB_LOG *, size_t, void *, size_t));
void __log_inmem_copyin __P((DB_LOG *, size_t, void *, size_t));
int __log_archive_pp __P((DB_ENV *, char **[], u_int32_t));
void __log_autoremove __P((DB_ENV *));
int __log_cursor_pp __P((DB_ENV *, DB_LOGC **, u_int32_t));
int __log_cursor __P((DB_ENV *, DB_LOGC **));
int __log_c_close __P((DB_LOGC *));
int __log_c_get __P((DB_LOGC *, DB_LSN *, DBT *, u_int32_t));
void __log_dbenv_create __P((DB_ENV *));
int __log_set_lg_bsize __P((DB_ENV *, u_int32_t));
int __log_set_lg_max __P((DB_ENV *, u_int32_t));
int __log_set_lg_regionmax __P((DB_ENV *, u_int32_t));
int __log_set_lg_dir __P((DB_ENV *, const char *));
void __log_get_flags __P((DB_ENV *, u_int32_t *));
void __log_set_flags __P((DB_ENV *, u_int32_t, int));
int __log_check_sizes __P((DB_ENV *, u_int32_t, u_int32_t));
int __log_put_pp __P((DB_ENV *, DB_LSN *, const DBT *, u_int32_t));
int __log_put __P((DB_ENV *, DB_LSN *, const DBT *, u_int32_t));
void __log_txn_lsn __P((DB_ENV *, DB_LSN *, u_int32_t *, u_int32_t *));
int __log_newfile __P((DB_LOG *, DB_LSN *, u_int32_t));
int __log_flush_pp __P((DB_ENV *, const DB_LSN *));
int __log_flush __P((DB_ENV *, const DB_LSN *));
int __log_flush_int __P((DB_LOG *, const DB_LSN *, int));
int __log_file_pp __P((DB_ENV *, const DB_LSN *, char *, size_t));
int __log_name __P((DB_LOG *, u_int32_t, char **, DB_FH **, u_int32_t));
int __log_rep_put __P((DB_ENV *, DB_LSN *, const DBT *));
int __log_stat_pp __P((DB_ENV *, DB_LOG_STAT **, u_int32_t));
int __log_stat_print_pp __P((DB_ENV *, u_int32_t));
int __log_stat_print __P((DB_ENV *, u_int32_t));

#if defined(__cplusplus)
}
#endif
#endif /* !_log_ext_h_ */
