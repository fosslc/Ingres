/*
//  IDMSOIDS.H
//
//  Header file for IDMS ODBC driver string resource ids
//
//  Separate from idmsodbc.h so resource compiler doesn't
//  run out of heap space.
//
//  (C) Copyright 1994,2002 Ingres Corporation
//
//
//  Change History
//  --------------
//
//  date        programmer          description
//
//  12/02/1992  Dave Ross          Initial coding
//  03/28/1997  Dave Thole         Additions for Ingres
//  06/19/1997  Jean Teng          added IDS_ERR_DS_NOT_SUPPORT
//  09/15/1997  Jean Teng          added IDS_ERR_DT_INTERVAL
//  12/12/1997  Jean Teng          remove msg defines moved to msg file
//  02/17/1999  Dave Thole         Ported to UNIX
//  01/18/2002  Dave Thole         added IDS_DLG_SERVERTYPE
//  12/01/2004  Ralph Loen         added IDS_DLG_DBMS_PWD
//
*/

/*
//  String resource ID's:
*/
#define IDS_MSG                 0
#define IDS_SET_TRANSACTION_TR  IDS_MSG + 1
#define IDS_SET_SESSION_TR      IDS_MSG + 2

/*
//#define IDS_MSG_COMMITTED       IDS_MSG + 3
//#define IDS_MSG_MAXLENGTH       IDS_MSG + 4
//#define IDS_MSG_TRACE           IDS_MSG + 5
//#define IDS_DRIVERCONNECT       IDS_MSG + 6

//#define IDS_ERR_STORAGE         16
//#define IDS_BIND_ARRAY          IDS_ERR_STORAGE + 1
//#define IDS_CACHE_BUFFER        IDS_ERR_STORAGE + 2
//#define IDS_CACHE_SQLDA         IDS_ERR_STORAGE + 3
//#define IDS_CONNECTION          IDS_ERR_STORAGE + 4
//#define IDS_DSNLIST             IDS_ERR_STORAGE + 5
//#define IDS_FETCH_BUFFER        IDS_ERR_STORAGE + 6
//#define IDS_FETCH_SQLDA         IDS_ERR_STORAGE + 7
//#define IDS_PARM_BUFFER         IDS_ERR_STORAGE + 8
//#define IDS_PARM_SQLDA          IDS_ERR_STORAGE + 9
//#define IDS_PARSE_BUFFER        IDS_ERR_STORAGE + 10
//#define IDS_RESULTS_ARRAY       IDS_ERR_STORAGE + 11
//#define IDS_SQL_QUERY           IDS_ERR_STORAGE + 12
//#define IDS_SQL_SAVE            IDS_ERR_STORAGE + 13
//#define IDS_STATEMENT           IDS_ERR_STORAGE + 14
//#define IDS_WORKAREA            IDS_ERR_STORAGE + 15
*/

#define IDS_DLG                   16
#define IDS_DLG_DRIVERCONNECT     IDS_DLG
#define IDS_DLG_DATASOURCE        IDS_DLG + 1
#define IDS_DLG_USERID            IDS_DLG + 2
#define IDS_DLG_PASSWORD          IDS_DLG + 3
#define IDS_DLG_ROLENAME          IDS_DLG + 4
#define IDS_DLG_ROLEPWD           IDS_DLG + 5
#define IDS_DLG_OK                IDOK
#define IDS_DLG_CANCEL            IDCANCEL 
#define IDS_DLG_HELP              IDC_HELP 
#define IDS_DLG_DATABASE          IDS_DLG + 6
#define IDS_DLG_SERVERNAME        IDS_DLG + 7
#define IDS_DLG_SERVERTYPE        IDS_DLG + 8
#define IDS_DLG_NODENAME          IDS_DLG + 9
#define IDS_DLG_TIMEOUT           IDS_DLG + 10
#define IDS_DLG_TASKCODE          IDS_DLG + 11
#define IDS_DLG_DBMS_PWD          IDS_DLG + 12

/*
//#define IDS_DRV                 48
//#define IDS_DRV_DATABASE        IDS_DRV + 1
//#define IDS_DRV_DATASOURCE      IDS_DRV + 2
//#define IDS_DRV_DICTIONARY      IDS_DRV + 3
//#define IDS_DRV_NODE            IDS_DRV + 4
//#define IDS_DRV_TASK            IDS_DRV + 5
//#define IDS_DRV_TIMEOUT         IDS_DRV + 6
//#define IDS_DRV_USERID          IDS_DRV + 7

//#define IDS_ERR                 64
//#define IDS_ERR_ARGUMENT        IDS_ERR + 1
//#define IDS_ERR_ASSIGNMENT      IDS_ERR + 2
//#define IDS_ERR_CANCELED        IDS_ERR + 3
//#define IDS_ERR_CANCEL_FREE     IDS_ERR + 4
//#define IDS_ERR_COLUMN          IDS_ERR + 5
//#define IDS_ERR_CONSTRAINT      IDS_ERR + 6
//#define IDS_ERR_CURSOR_DUP      IDS_ERR + 7
//#define IDS_ERR_CURSOR_INVALID  IDS_ERR + 8
//#define IDS_ERR_CURSOR_NONE     IDS_ERR + 9
//#define IDS_ERR_CURSOR_STATE    IDS_ERR + 10
//#define IDS_ERR_DATE_OVERFLOW   IDS_ERR + 11
//#define IDS_ERR_DESCRIBE        IDS_ERR + 12
//#define IDS_ERR_DESCRIPTOR      IDS_ERR + 13
//#define IDS_ERR_DRIVER          IDS_ERR + 14
//#define IDS_ERR_ESCAPE          IDS_ERR + 15
//#define IDS_ERR_EXEC_DATA       IDS_ERR + 16
//#define IDS_ERR_EXTENSION       IDS_ERR + 17
//#define IDS_ERR_FETCH           IDS_ERR + 18
//#define IDS_ERR_INFO_TYPE       IDS_ERR + 19
//#define IDS_ERR_INVALID         IDS_ERR + 20
//#define IDS_ERR_INVALID_NOW     IDS_ERR + 21
//#define IDS_ERR_LENGTH          IDS_ERR + 22
//#define IDS_ERR_MISMATCH        IDS_ERR + 23
//#define IDS_ERR_NOINDICATOR     IDS_ERR + 24
//#define IDS_ERR_NOMATCH         IDS_ERR + 25
//#define IDS_ERR_NOMESSAGE       IDS_ERR + 26
//#define IDS_ERR_NOPROMPT        IDS_ERR + 27
//#define IDS_ERR_NOT_CAPABLE     IDS_ERR + 28
//#define IDS_ERR_NUMERIC         IDS_ERR + 29
//#define IDS_ERR_OPTION          IDS_ERR + 30
//#define IDS_ERR_OPTION_CHANGED  IDS_ERR + 31
//#define IDS_ERR_PARMS           IDS_ERR + 32
//#define IDS_ERR_PARM_LIST       IDS_ERR + 33
//#define IDS_ERR_PARM_TYPE       IDS_ERR + 34
//#define IDS_ERR_PRECISION       IDS_ERR + 35
//#define IDS_ERR_RANGE           IDS_ERR + 36
//#define IDS_ERR_RESOURCE        IDS_ERR + 37
//#define IDS_ERR_SCALE           IDS_ERR + 38
//#define IDS_ERR_SECTION         IDS_ERR + 39
//#define IDS_ERR_SEQUENCE        IDS_ERR + 40
//#define IDS_ERR_STATE           IDS_ERR + 41
//#define IDS_ERR_SYNTAX          IDS_ERR + 42
//#define IDS_ERR_TRACE           IDS_ERR + 43
//#define IDS_ERR_TRANSDLL        IDS_ERR + 44
//#define IDS_ERR_TRUNC_STRING    IDS_ERR + 45
//#define IDS_ERR_TRUNCATION      IDS_ERR + 46
//#define IDS_ERR_TYPE_ATTRIBUTE  IDS_ERR + 47
//#define IDS_ERR_TYPE_INVALID    IDS_ERR + 48
//#define IDS_ERR_TYPE_NONE       IDS_ERR + 49
//#define IDS_ERR_UNCOMMITTED     IDS_ERR + 50
//#define IDS_ERR_NOT_SUPPORTED   IDS_ERR + 51
//#define IDS_ERR_DS_NOT_SUPPORT  IDS_ERR + 52
//#define IDS_ERR_DT_INTERVALS    IDS_ERR + 53
*/

#define IDS_CAT                 176
#define IDS_CAT_NAME            IDS_CAT  + 1
#define IDS_CAT_ORDER_COLUMNS   IDS_CAT  + 2
#define IDS_CAT_ORDER_STATS     IDS_CAT  + 3
#define IDS_CAT_ORDER_TABLES    IDS_CAT  + 4
#define IDS_CAT_SCHEMA          IDS_CAT  + 5
#define IDS_CAT_TABLE           IDS_CAT  + 6

#define IDS_IDMS                208
#define IDS_IDMS_INDEXQUERY     IDS_IDMS + 1
#define IDS_IDMS_ISCHEMA        IDS_IDMS + 2
#define IDS_IDMS_ITABLE         IDS_IDMS + 3
#define IDS_IDMS_KNAME          IDS_IDMS + 4
#define IDS_IDMS_NOTNULL        IDS_IDMS + 5
#define IDS_IDMS_ORDER_SETS     IDS_IDMS + 6
#define IDS_IDMS_REQ_NULL       IDS_IDMS + 7
#define IDS_IDMS_REQ_UNIQ       IDS_IDMS + 8
#define IDS_IDMS_RSYN           IDS_IDMS + 9
#define IDS_IDMS_RSYN_MNAM      IDS_IDMS + 10
#define IDS_IDMS_RSYN_ONAM      IDS_IDMS + 11
#define IDS_IDMS_STATISTICS     IDS_IDMS + 12
#define IDS_IDMS_TABLE_OWNER    IDS_IDMS + 13
#define IDS_IDMS_TABLE_NAME     IDS_IDMS + 14
#define IDS_IDMS_UNIQUE         IDS_IDMS + 15

#define IDS_CADB                240
#define IDS_CADB_COLNAME        IDS_CADB + 1
#define IDS_CADB_INDEXQUERY     IDS_CADB + 2
#define IDS_CADB_INDEXNAME      IDS_CADB + 3
#define IDS_CADB_IOWNER         IDS_CADB + 4
#define IDS_CADB_ITABLENAME     IDS_CADB + 5
#define IDS_CADB_KINDEXNAME     IDS_CADB + 6
#define IDS_CADB_NOTNULL        IDS_CADB + 7
#define IDS_CADB_OWNER          IDS_CADB + 8
#define IDS_CADB_TABLENAME      IDS_CADB + 9
#define IDS_CADB_STATISTICS     IDS_CADB + 10
#define IDS_CADB_UNIQUE         IDS_CADB + 11

#define IDS_INGRES              256
#define IDS_INGRES_TABLE_OWNER  IDS_INGRES + 1
#define IDS_INGRES_TABLE_NAME   IDS_INGRES + 2
#define IDS_INGRES_COLUMN_OWNER IDS_INGRES + 3
#define IDS_INGRES_COLUMN_TABLE IDS_INGRES + 4
#define IDS_INGRES_COLUMN_NAME  IDS_INGRES + 5
#define IDS_INGRES_ITABLE_OWNER IDS_INGRES + 6
#define IDS_INGRES_ITABLE_NAME  IDS_INGRES + 7
#define IDS_INGRES_PROC_OWNER   IDS_INGRES + 8
#define IDS_INGRES_PROC_NAME    IDS_INGRES + 9
#define IDS_INGRES_PROCCOL_COLNAME  IDS_INGRES + 10
