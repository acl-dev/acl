/*************************************************************************************************
 * The abstract database API of Tokyo Cabinet
 *                                                      Copyright (C) 2006-2009 Mikio Hirabayashi
 * This file is part of Tokyo Cabinet.
 * Tokyo Cabinet is free software; you can redistribute it and/or modify it under the terms of
 * the GNU Lesser General Public License as published by the Free Software Foundation; either
 * version 2.1 of the License or any later version.  Tokyo Cabinet is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * You should have received a copy of the GNU Lesser General Public License along with Tokyo
 * Cabinet; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA.
 *************************************************************************************************/


#ifndef _TCADB_H                         /* duplication check */
#define _TCADB_H

#if defined(__cplusplus)
#define __TCADB_CLINKAGEBEGIN extern "C" {
#define __TCADB_CLINKAGEEND }
#else
#define __TCADB_CLINKAGEBEGIN
#define __TCADB_CLINKAGEEND
#endif
__TCADB_CLINKAGEBEGIN


#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <tcutil.h>
#include <tchdb.h>
#include <tcbdb.h>
#include <tcfdb.h>
#include <tctdb.h>



/*************************************************************************************************
 * API
 *************************************************************************************************/


typedef struct {                         /* type of structure for an abstract database */
  int omode;                             /* open mode */
  TCMDB *mdb;                            /* on-memory hash database object */
  TCNDB *ndb;                            /* on-memory tree database object */
  TCHDB *hdb;                            /* hash database object */
  TCBDB *bdb;                            /* B+ tree database object */
  TCFDB *fdb;                            /* fixed-length databae object */
  TCTDB *tdb;                            /* table database object */
  int64_t capnum;                        /* capacity number of records */
  int64_t capsiz;                        /* capacity size of using memory */
  uint32_t capcnt;                       /* count for capacity check */
  BDBCUR *cur;                           /* cursor of B+ tree */
  void *skel;                            /* skeleton database */
} TCADB;

enum {                                   /* enumeration for open modes */
  ADBOVOID,                              /* not opened */
  ADBOMDB,                               /* on-memory hash database */
  ADBONDB,                               /* on-memory tree database */
  ADBOHDB,                               /* hash database */
  ADBOBDB,                               /* B+ tree database */
  ADBOFDB,                               /* fixed-length database */
  ADBOTDB,                               /* table database */
  ADBOSKEL                               /* skeleton database */
};


/* Create an abstract database object.
   The return value is the new abstract database object. */
TCADB *tcadbnew(void);


/* Delete an abstract database object.
   `adb' specifies the abstract database object. */
void tcadbdel(TCADB *adb);


/* Open an abstract database.
   `adb' specifies the abstract database object.
   `name' specifies the name of the database.  If it is "*", the database will be an on-memory
   hash database.  If it is "+", the database will be an on-memory tree database.  If its suffix
   is ".tch", the database will be a hash database.  If its suffix is ".tcb", the database will
   be a B+ tree database.  If its suffix is ".tcf", the database will be a fixed-length database.
   If its suffix is ".tct", the database will be a table database.  Otherwise, this function
   fails.  Tuning parameters can trail the name, separated by "#".  Each parameter is composed of
   the name and the value, separated by "=".  On-memory hash database supports "bnum", "capnum",
   and "capsiz".  On-memory tree database supports "capnum" and "capsiz".  Hash database supports
   "mode", "bnum", "apow", "fpow", "opts", "rcnum", "xmsiz", and "dfunit".  B+ tree database
   supports "mode", "lmemb", "nmemb", "bnum", "apow", "fpow", "opts", "lcnum", "ncnum", "xmsiz",
   and "dfunit".  Fixed-length database supports "mode", "width", and "limsiz".  Table database
   supports "mode", "bnum", "apow", "fpow", "opts", "rcnum", "lcnum", "ncnum", "xmsiz", "dfunit",
   and "idx".
   If successful, the return value is true, else, it is false.
   The tuning parameter "capnum" specifies the capacity number of records.  "capsiz" specifies
   the capacity size of using memory.  Records spilled the capacity are removed by the storing
   order.  "mode" can contain "w" of writer, "r" of reader, "c" of creating, "t" of truncating,
   "e" of no locking, and "f" of non-blocking lock.  The default mode is relevant to "wc".
   "opts" can contains "l" of large option, "d" of Deflate option, "b" of BZIP2 option, and "t"
   of TCBS option.  "idx" specifies the column name of an index and its type separated by ":".
   For example, "casket.tch#bnum=1000000#opts=ld" means that the name of the database file is
   "casket.tch", and the bucket number is 1000000, and the options are large and Deflate. */
bool tcadbopen(TCADB *adb, const char *name);


/* Close an abstract database object.
   `adb' specifies the abstract database object.
   If successful, the return value is true, else, it is false.
   Update of a database is assured to be written when the database is closed.  If a writer opens
   a database but does not close it appropriately, the database will be broken. */
bool tcadbclose(TCADB *adb);


/* Store a record into an abstract database object.
   `adb' specifies the abstract database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, it is overwritten. */
bool tcadbput(TCADB *adb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Store a string record into an abstract object.
   `adb' specifies the abstract database object.
   `kstr' specifies the string of the key.
   `vstr' specifies the string of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, it is overwritten. */
bool tcadbput2(TCADB *adb, const char *kstr, const char *vstr);


/* Store a new record into an abstract database object.
   `adb' specifies the abstract database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, this function has no effect. */
bool tcadbputkeep(TCADB *adb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Store a new string record into an abstract database object.
   `adb' specifies the abstract database object.
   `kstr' specifies the string of the key.
   `vstr' specifies the string of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, this function has no effect. */
bool tcadbputkeep2(TCADB *adb, const char *kstr, const char *vstr);


/* Concatenate a value at the end of the existing record in an abstract database object.
   `adb' specifies the abstract database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If there is no corresponding record, a new record is created. */
bool tcadbputcat(TCADB *adb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Concatenate a string value at the end of the existing record in an abstract database object.
   `adb' specifies the abstract database object.
   `kstr' specifies the string of the key.
   `vstr' specifies the string of the value.
   If successful, the return value is true, else, it is false.
   If there is no corresponding record, a new record is created. */
bool tcadbputcat2(TCADB *adb, const char *kstr, const char *vstr);


/* Remove a record of an abstract database object.
   `adb' specifies the abstract database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is true, else, it is false. */
bool tcadbout(TCADB *adb, const void *kbuf, int ksiz);


/* Remove a string record of an abstract database object.
   `adb' specifies the abstract database object.
   `kstr' specifies the string of the key.
   If successful, the return value is true, else, it is false. */
bool tcadbout2(TCADB *adb, const char *kstr);


/* Retrieve a record in an abstract database object.
   `adb' specifies the abstract database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the value of the corresponding
   record.  `NULL' is returned if no record corresponds.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when
   it is no longer in use. */
void *tcadbget(TCADB *adb, const void *kbuf, int ksiz, int *sp);


/* Retrieve a string record in an abstract database object.
   `adb' specifies the abstract database object.
   `kstr' specifies the string of the key.
   If successful, the return value is the string of the value of the corresponding record.
   `NULL' is returned if no record corresponds.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call when it is no longer in use. */
char *tcadbget2(TCADB *adb, const char *kstr);


/* Get the size of the value of a record in an abstract database object.
   `adb' specifies the abstract database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is the size of the value of the corresponding record, else,
   it is -1. */
int tcadbvsiz(TCADB *adb, const void *kbuf, int ksiz);


/* Get the size of the value of a string record in an abstract database object.
   `adb' specifies the abstract database object.
   `kstr' specifies the string of the key.
   If successful, the return value is the size of the value of the corresponding record, else,
   it is -1. */
int tcadbvsiz2(TCADB *adb, const char *kstr);


/* Initialize the iterator of an abstract database object.
   `adb' specifies the abstract database object.
   If successful, the return value is true, else, it is false.
   The iterator is used in order to access the key of every record stored in a database. */
bool tcadbiterinit(TCADB *adb);


/* Get the next key of the iterator of an abstract database object.
   `adb' specifies the abstract database object.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the next key, else, it is
   `NULL'.  `NULL' is returned when no record is to be get out of the iterator.
   Because an additional zero code is appended at the end of the region of the return value, the
   return value can be treated as a character string.  Because the region of the return value is
   allocated with the `malloc' call, it should be released with the `free' call when it is no
   longer in use.  It is possible to access every record by iteration of calling this function.
   It is allowed to update or remove records whose keys are fetched while the iteration.
   However, it is not assured if updating the database is occurred while the iteration.  Besides,
   the order of this traversal access method is arbitrary, so it is not assured that the order of
   storing matches the one of the traversal access. */
void *tcadbiternext(TCADB *adb, int *sp);


/* Get the next key string of the iterator of an abstract database object.
   `adb' specifies the abstract database object.
   If successful, the return value is the string of the next key, else, it is `NULL'.  `NULL' is
   returned when no record is to be get out of the iterator.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call when it is no longer in use.  It is possible to access every
   record by iteration of calling this function.  However, it is not assured if updating the
   database is occurred while the iteration.  Besides, the order of this traversal access method
   is arbitrary, so it is not assured that the order of storing matches the one of the traversal
   access. */
char *tcadbiternext2(TCADB *adb);


/* Get forward matching keys in an abstract database object.
   `adb' specifies the abstract database object.
   `pbuf' specifies the pointer to the region of the prefix.
   `psiz' specifies the size of the region of the prefix.
   `max' specifies the maximum number of keys to be fetched.  If it is negative, no limit is
   specified.
   The return value is a list object of the corresponding keys.  This function does never fail.
   It returns an empty list even if no key corresponds.
   Because the object of the return value is created with the function `tclistnew', it should be
   deleted with the function `tclistdel' when it is no longer in use.  Note that this function
   may be very slow because every key in the database is scanned. */
TCLIST *tcadbfwmkeys(TCADB *adb, const void *pbuf, int psiz, int max);


/* Get forward matching string keys in an abstract database object.
   `adb' specifies the abstract database object.
   `pstr' specifies the string of the prefix.
   `max' specifies the maximum number of keys to be fetched.  If it is negative, no limit is
   specified.
   The return value is a list object of the corresponding keys.  This function does never fail.
   It returns an empty list even if no key corresponds.
   Because the object of the return value is created with the function `tclistnew', it should be
   deleted with the function `tclistdel' when it is no longer in use.  Note that this function
   may be very slow because every key in the database is scanned. */
TCLIST *tcadbfwmkeys2(TCADB *adb, const char *pstr, int max);


/* Add an integer to a record in an abstract database object.
   `adb' specifies the abstract database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `num' specifies the additional value.
   If successful, the return value is the summation value, else, it is `INT_MIN'.
   If the corresponding record exists, the value is treated as an integer and is added to.  If no
   record corresponds, a new record of the additional value is stored. */
int tcadbaddint(TCADB *adb, const void *kbuf, int ksiz, int num);


/* Add a real number to a record in an abstract database object.
   `adb' specifies the abstract database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `num' specifies the additional value.
   If successful, the return value is the summation value, else, it is Not-a-Number.
   If the corresponding record exists, the value is treated as a real number and is added to.  If
   no record corresponds, a new record of the additional value is stored. */
double tcadbadddouble(TCADB *adb, const void *kbuf, int ksiz, double num);


/* Synchronize updated contents of an abstract database object with the file and the device.
   `adb' specifies the abstract database object.
   If successful, the return value is true, else, it is false. */
bool tcadbsync(TCADB *adb);


/* Optimize the storage of an abstract database object.
   `adb' specifies the abstract database object.
   `params' specifies the string of the tuning parameters, which works as with the tuning
   of parameters the function `tcadbopen'.  If it is `NULL', it is not used.
   If successful, the return value is true, else, it is false.
   This function is useful to reduce the size of the database storage with data fragmentation by
   successive updating. */
bool tcadboptimize(TCADB *adb, const char *params);


/* Remove all records of an abstract database object.
   `adb' specifies the abstract database object.
   If successful, the return value is true, else, it is false. */
bool tcadbvanish(TCADB *adb);


/* Copy the database file of an abstract database object.
   `adb' specifies the abstract database object.
   `path' specifies the path of the destination file.  If it begins with `@', the trailing
   substring is executed as a command line.
   If successful, the return value is true, else, it is false.  False is returned if the executed
   command returns non-zero code.
   The database file is assured to be kept synchronized and not modified while the copying or
   executing operation is in progress.  So, this function is useful to create a backup file of
   the database file. */
bool tcadbcopy(TCADB *adb, const char *path);


/* Begin the transaction of an abstract database object.
   `adb' specifies the abstract database object.
   If successful, the return value is true, else, it is false.
   The database is locked by the thread while the transaction so that only one transaction can be
   activated with a database object at the same time.  Thus, the serializable isolation level is
   assumed if every database operation is performed in the transaction.  All updated regions are
   kept track of by write ahead logging while the transaction.  If the database is closed during
   transaction, the transaction is aborted implicitly. */
bool tcadbtranbegin(TCADB *adb);


/* Commit the transaction of an abstract database object.
   `adb' specifies the abstract database object.
   If successful, the return value is true, else, it is false.
   Update in the transaction is fixed when it is committed successfully. */
bool tcadbtrancommit(TCADB *adb);


/* Abort the transaction of an abstract database object.
   `adb' specifies the abstract database object.
   If successful, the return value is true, else, it is false.
   Update in the transaction is discarded when it is aborted.  The state of the database is
   rollbacked to before transaction. */
bool tcadbtranabort(TCADB *adb);


/* Get the file path of an abstract database object.
   `adb' specifies the abstract database object.
   The return value is the path of the database file or `NULL' if the object does not connect to
   any database.  "*" stands for on-memory hash database.  "+" stands for on-memory tree
   database. */
const char *tcadbpath(TCADB *adb);


/* Get the number of records of an abstract database object.
   `adb' specifies the abstract database object.
   The return value is the number of records or 0 if the object does not connect to any database
   instance. */
uint64_t tcadbrnum(TCADB *adb);


/* Get the size of the database of an abstract database object.
   `adb' specifies the abstract database object.
   The return value is the size of the database or 0 if the object does not connect to any
   database instance. */
uint64_t tcadbsize(TCADB *adb);


/* Call a versatile function for miscellaneous operations of an abstract database object.
   `adb' specifies the abstract database object.
   `name' specifies the name of the function.  All databases support "put", "out", "get",
   "putlist", "outlist", and "getlist".  "put" is to store a record.  It receives a key and a
   value, and returns an empty list.  "out" is to remove a record.  It receives a key, and
   returns an empty list.  "get" is to retrieve a record.  It receives a key, and returns a list
   of the values.  "putlist" is to store records.  It receives keys and values one after the
   other, and returns an empty list.  "outlist" is to remove records.  It receives keys, and
   returns an empty list.  "getlist" is to retrieve records.  It receives keys, and returns keys
   and values of corresponding records one after the other.
   `args' specifies a list object containing arguments.
   If successful, the return value is a list object of the result.  `NULL' is returned on failure.
   Because the object of the return value is created with the function `tclistnew', it
   should be deleted with the function `tclistdel' when it is no longer in use. */
TCLIST *tcadbmisc(TCADB *adb, const char *name, const TCLIST *args);



/*************************************************************************************************
 * features for experts
 *************************************************************************************************/


typedef struct {                         /* type of structure for a extra database skeleton */
  void *opq;                             /* opaque pointer */
  void (*del)(void *);                   /* destructor */
  bool (*open)(void *, const char *);
  bool (*close)(void *);
  bool (*put)(void *, const void *, int, const void *, int);
  bool (*putkeep)(void *, const void *, int, const void *, int);
  bool (*putcat)(void *, const void *, int, const void *, int);
  bool (*out)(void *, const void *, int);
  void *(*get)(void *, const void *, int, int *);
  int (*vsiz)(void *, const void *, int);
  bool (*iterinit)(void *);
  void *(*iternext)(void *, int *);
  TCLIST *(*fwmkeys)(void *, const void *, int, int);
  int (*addint)(void *, const void *, int, int);
  double (*adddouble)(void *, const void *, int, double);
  bool (*sync)(void *);
  bool (*optimize)(void *, const char *);
  bool (*vanish)(void *);
  bool (*copy)(void *, const char *);
  bool (*tranbegin)(void *);
  bool (*trancommit)(void *);
  bool (*tranabort)(void *);
  const char *(*path)(void *);
  uint64_t (*rnum)(void *);
  uint64_t (*size)(void *);
  TCLIST *(*misc)(void *, const char *, const TCLIST *);
  bool (*putproc)(void *, const void *, int, const void *, int, TCPDPROC, void *);
  bool (*foreach)(void *, TCITER, void *);
} ADBSKEL;

/* type of the pointer to a mapping function.
   `map' specifies the pointer to the destination manager.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   `op' specifies the pointer to the optional opaque object.
   The return value is true to continue iteration or false to stop iteration. */
typedef bool (*ADBMAPPROC)(void *map, const char *kbuf, int ksiz, const char *vbuf, int vsiz,
                           void *op);


/* Set an extra database sleleton to an abstract database object.
   `adb' specifies the abstract database object.
   `skel' specifies the extra database skeleton.
   If successful, the return value is true, else, it is false. */
bool tcadbsetskel(TCADB *adb, ADBSKEL *skel);


/* Get the open mode of an abstract database object.
   `adb' specifies the abstract database object.
   The return value is `ADBOVOID' for not opened database, `ADBOMDB' for on-memory hash database,
  `ADBONDB' for on-memory tree database, `ADBOHDB' for hash database, `ADBOBDB' for B+ tree
  database, `ADBOFDB' for fixed-length database, `ADBOTDB' for table database. */
int tcadbomode(TCADB *adb);


/* Get the concrete database object of an abstract database object.
   `adb' specifies the abstract database object.
   The return value is the concrete database object depend on the open mode or 0 if the object
   does not connect to any database instance. */
void *tcadbreveal(TCADB *adb);


/* Store a record into an abstract database object with a duplication handler.
   `adb' specifies the abstract database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   `proc' specifies the pointer to the callback function to process duplication.
   `op' specifies an arbitrary pointer to be given as a parameter of the callback function.  If
   it is not needed, `NULL' can be specified.
   If successful, the return value is true, else, it is false.
   This function does not work for the table database. */
bool tcadbputproc(TCADB *adb, const void *kbuf, int ksiz, const void *vbuf, int vsiz,
                  TCPDPROC proc, void *op);


/* Process each record atomically of an abstract database object.
   `adb' specifies the abstract database object.
   `iter' specifies the pointer to the iterator function called for each record.
   `op' specifies an arbitrary pointer to be given as a parameter of the iterator function.  If
   it is not needed, `NULL' can be specified.
   If successful, the return value is true, else, it is false. */
bool tcadbforeach(TCADB *adb, TCITER iter, void *op);


/* Map records of an abstract database object into another B+ tree database.
   `adb' specifies the abstract database object.
   `keys' specifies a list object of the keys of the target records.  If it is `NULL', every
   record is processed.
   `bdb' specifies the B+ tree database object into which records emitted by the mapping function
   are stored.
   `proc' specifies the pointer to the mapping function called for each record.
   `op' specifies specifies the pointer to the optional opaque object for the mapping function.
   `csiz' specifies the size of the cache to sort emitted records.  If it is negative, the
   default size is specified.  The default size is 268435456.
   If successful, the return value is true, else, it is false. */
bool tcadbmapbdb(TCADB *adb, TCLIST *keys, TCBDB *bdb, ADBMAPPROC proc, void *op, int64_t csiz);


/* Emit records generated by the mapping function into the result map.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false. */
bool tcadbmapbdbemit(void *map, const char *kbuf, int ksiz, const char *vbuf, int vsiz);



__TCADB_CLINKAGEEND
#endif                                   /* duplication check */


/* END OF FILE */
