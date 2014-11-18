/* Do not edit: automatically built by gen_rec.awk. */

#ifndef	__rep_AUTO_H
#define	__rep_AUTO_H
typedef struct ___rep_update_args {
	DB_LSN	first_lsn;
	int	num_files;
} __rep_update_args;

typedef struct ___rep_fileinfo_args {
	size_t	pgsize;
	db_pgno_t	pgno;
	db_pgno_t	max_pgno;
	int	filenum;
	int32_t	id;
	u_int32_t	type;
	u_int32_t	flags;
	DBT	uid;
	DBT	info;
} __rep_fileinfo_args;

#endif
