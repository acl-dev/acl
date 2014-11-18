/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_env_ext_h_
#define	_env_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

void __db_shalloc_init __P((REGINFO *, size_t));
size_t __db_shalloc_size __P((size_t, size_t));
int __db_shalloc __P((REGINFO *, size_t, size_t, void *));
void __db_shalloc_free __P((REGINFO *, void *));
size_t __db_shalloc_sizeof __P((void *));
u_int32_t __db_tablesize __P((u_int32_t));
void __db_hashinit __P((void *, u_int32_t));
int __db_fileinit __P((DB_ENV *, DB_FH *, size_t, int));
int __db_overwrite __P((DB_ENV *, const char *));
int  __dbenv_set_alloc __P((DB_ENV *, void *(*)(size_t), void *(*)(void *, size_t), void (*)(void *)));
int __dbenv_get_encrypt_flags __P((DB_ENV *, u_int32_t *));
int __dbenv_set_encrypt __P((DB_ENV *, const char *, u_int32_t));
int  __dbenv_set_flags __P((DB_ENV *, u_int32_t, int));
int  __dbenv_set_data_dir __P((DB_ENV *, const char *));
int  __dbenv_set_intermediate_dir __P((DB_ENV *, int, u_int32_t));
void __dbenv_set_errcall __P((DB_ENV *, void (*)(const DB_ENV *, const char *, const char *)));
void __dbenv_get_errfile __P((DB_ENV *, FILE **));
void __dbenv_set_errfile __P((DB_ENV *, FILE *));
void __dbenv_get_errpfx __P((DB_ENV *, const char **));
void __dbenv_set_errpfx __P((DB_ENV *, const char *));
void __dbenv_set_msgcall __P((DB_ENV *, void (*)(const DB_ENV *, const char *)));
void __dbenv_get_msgfile __P((DB_ENV *, FILE **));
void __dbenv_set_msgfile __P((DB_ENV *, FILE *));
int  __dbenv_set_paniccall __P((DB_ENV *, void (*)(DB_ENV *, int)));
int  __dbenv_set_shm_key __P((DB_ENV *, long));
int  __dbenv_set_tas_spins __P((DB_ENV *, u_int32_t));
int  __dbenv_set_tmp_dir __P((DB_ENV *, const char *));
int  __dbenv_set_verbose __P((DB_ENV *, u_int32_t, int));
int __db_mi_env __P((DB_ENV *, const char *));
int __db_mi_open __P((DB_ENV *, const char *, int));
int __db_env_config __P((DB_ENV *, char *, u_int32_t));
int __dbenv_open __P((DB_ENV *, const char *, u_int32_t, int));
int __dbenv_remove __P((DB_ENV *, const char *, u_int32_t));
int __dbenv_close_pp __P((DB_ENV *, u_int32_t));
int __dbenv_close __P((DB_ENV *, int));
int __dbenv_get_open_flags __P((DB_ENV *, u_int32_t *));
int __db_appname __P((DB_ENV *, APPNAME, const char *, u_int32_t, DB_FH **, char **));
int __db_home __P((DB_ENV *, const char *, u_int32_t));
int __db_apprec __P((DB_ENV *, DB_LSN *, DB_LSN *, int, u_int32_t));
int    __log_backup __P((DB_ENV *, DB_LOGC *, DB_LSN *, DB_LSN *, u_int32_t));
int __env_openfiles __P((DB_ENV *, DB_LOGC *, void *, DBT *, DB_LSN *, DB_LSN *, double, int));
int __db_e_attach __P((DB_ENV *, u_int32_t *));
int __db_e_detach __P((DB_ENV *, int));
int __db_e_remove __P((DB_ENV *, u_int32_t));
int __db_r_attach __P((DB_ENV *, REGINFO *, size_t));
int __db_r_detach __P((DB_ENV *, REGINFO *, int));
int __dbenv_stat_print_pp __P((DB_ENV *, u_int32_t));
void __db_print_fh __P((DB_ENV *, DB_FH *, u_int32_t));
void __db_print_fileid __P((DB_ENV *, u_int8_t *, const char *));
void __db_print_mutex __P((DB_ENV *, DB_MSGBUF *, DB_MUTEX *, const char *, u_int32_t));
void __db_dl __P((DB_ENV *, const char *, u_long));
void __db_dl_pct __P((DB_ENV *, const char *, u_long, int, const char *));
void __db_dlbytes __P((DB_ENV *, const char *, u_long, u_long, u_long));
void __db_print_reginfo __P((DB_ENV *, REGINFO *, const char *));
int __db_stat_not_built __P((DB_ENV *));

#if defined(__cplusplus)
}
#endif
#endif /* !_env_ext_h_ */
