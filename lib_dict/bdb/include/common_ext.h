/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_common_ext_h_
#define	_common_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __crypto_region_init __P((DB_ENV *));
int __db_isbigendian __P((void));
int __db_byteorder __P((DB_ENV *, int));
int __db_fchk __P((DB_ENV *, const char *, u_int32_t, u_int32_t));
int __db_fcchk __P((DB_ENV *, const char *, u_int32_t, u_int32_t, u_int32_t));
int __db_ferr __P((const DB_ENV *, const char *, int));
int __db_fnl __P((const DB_ENV *, const char *));
int __db_pgerr __P((DB *, db_pgno_t, int));
int __db_pgfmt __P((DB_ENV *, db_pgno_t));
#ifdef DIAGNOSTIC
void __db_assert __P((const char *, const char *, int));
#endif
int __db_panic_msg __P((DB_ENV *));
int __db_panic __P((DB_ENV *, int));
void __db_err __P((const DB_ENV *, const char *, ...)) __attribute__ ((__format__ (__printf__, 2, 3)));
void __db_errcall __P((const DB_ENV *, int, int, const char *, va_list));
void __db_errfile __P((const DB_ENV *, int, int, const char *, va_list));
void __db_msgadd __P((DB_ENV *, DB_MSGBUF *, const char *, ...)) __attribute__ ((__format__ (__printf__, 3, 4)));
void __db_msg __P((const DB_ENV *, const char *, ...)) __attribute__ ((__format__ (__printf__, 2, 3)));
void __db_logmsg __P((const DB_ENV *, DB_TXN *, const char *, u_int32_t, const char *, ...)) __attribute__ ((__format__ (__printf__, 5, 6)));
int __db_unknown_flag __P((DB_ENV *, char *, u_int32_t));
int __db_unknown_type __P((DB_ENV *, char *, DBTYPE));
int __db_check_txn __P((DB *, DB_TXN *, u_int32_t, int));
int __db_not_txn_env __P((DB_ENV *));
int __db_rec_toobig __P((DB_ENV *, u_int32_t, u_int32_t));
int __db_rec_repl __P((DB_ENV *, u_int32_t, u_int32_t));
int __db_check_lsn __P((DB_ENV *, DB_LSN *, DB_LSN *));
int __db_getlong __P((DB_ENV *, const char *, char *, long, long, long *));
int __db_getulong __P((DB_ENV *, const char *, char *, u_long, u_long, u_long *));
void __db_idspace __P((u_int32_t *, int, u_int32_t *, u_int32_t *));
u_int32_t __db_log2 __P((u_int32_t));
int __db_util_arg __P((char *, char *, int *, char ***));
int __db_util_cache __P((DB *, u_int32_t *, int *));
int __db_util_logset __P((const char *, char *));
void __db_util_siginit __P((void));
int __db_util_interrupted __P((void));
void __db_util_sigresend __P((void));

#if defined(__cplusplus)
}
#endif
#endif /* !_common_ext_h_ */
