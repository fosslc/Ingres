/*
//  IDMSOINF.H
//
//  Header file for ODBC driver information resources
//
//  (C) Copyright 1994,2000 Ingres Corporation
//
//
//  Change History
//  --------------
//
//  date        programmer          description
//
//  03/30/1994  Dave Ross          Initial coding
//  05/01/1997  Jean Teng          Added OpenSql limits
//  05/21/1997  Jean Teng          Added OpenSql fn infomation
//  06/30/1997  Dave Thole         Added RT_CAT_SPECIAL_COLUMNS_GATEWAY
//  02/22/1999  Dave Thole         Add support for SQLPrimaryKeys.
//  02/17/1999  Dave Thole         Ported to UNIX
//  03/13/2000  Dave Thole         Add support for Ingres Read Only.
//  09/20/2005  Fei  Wei           Delete MAX_STMT_LEN of 30K.
*/

#ifndef _INC_IDMSOINF
#define _INC_IDMSOINF

#define MAX_INFO_TYPE           65535           /* Maximum information type */
#define MAX_COLUMNS_IDMS        1024            /* Maximum columns for IDMS */
#define MAX_COLUMNS_CADB        255             /* Maximum columns for CADB et al */
#define MAX_COLUMNS_INGRES		300				/* Maximum columns fo Ingres */

#define OPENSQL_DDL_INDEX             0L
#define OPENSQL_INDEX_KEYWORDS        0L
#define OPENSQL_MAX_CHAR_LITERAL_LEN  2000
#define OPENSQL_MAX_COLUMNS_IN_INDEX  16
#define OPENSQL_MAX_COLUMNS_IN_TABLE  127
#define OPENSQL_MAX_IDENTIFIER_LEN    18
#define OPENSQL_MAX_ROW_SIZE          2000
#define OPENSQL_MAX_TABLES_IN_SQL     15
#define OPENSQL_OJ_CAPABILITIES       0L
#define OPENSQL_NUMERIC_FUNCTIONS     0L
#define OPENSQL_SQL92_GRANT           0L
#define OPENSQL_SQL92_RELATIONAL_JOIN_OPERATORS 0L
#define OPENSQL_SQL92_REVOKE          0L
#define OPENSQL_STRING_FUNCTIONS      0L
#define OPENSQL_SYSTEM_FUNCTIONS      SQL_FN_SYS_DBNAME + SQL_FN_SYS_USERNAME
#define OPENSQL_TIMEDATE_FUNCTIONS    SQL_FN_TD_CURDATE + SQL_FN_TD_NOW

/*
**  STRINGID
**
**  Describes user defined resource containing an array of string ID's
**  for SQL syntax fragments and other string contants unique to each DBMS:
*/
typedef struct tSTRINGID
{
    char *  fOwnerName;         /* Owner name column (for most tables) */
    char *  fTableName;         /* TABLE table name column */
    char *  fColumnOwner;       /* COLUMN table owner name column */
    char *  fColumnTable;       /* COLUMN table table name column */
    char *  fColumnName;        /* COLUMN table column name column */
    char *  fIndexTable;        /* INDEX table table name column */
    char *  fNullable;          /* SQLSpecialColumns nullable predicate */
    char *  fUpStats;           /* SQLStatistics update statistics command */
    char *  fIndexTableOwner;   /* Joined index table owner column name */
    char *  fIndexTableName;    /* Joined index table table column name */
    char *  fUnique;            /* SQLStatistics unique predicate */
    char *  fProcOwner;         /* Procedure owner name */
    char *  fProcName;          /* Procedure name */
    char *  fProcColName;       /* Procedure column name */
}
STRINGID, FAR * LPSTRINGID;

/*
**  User defined resource types:
*/
#define RT_DRIVERS              256     /* Supported driver table. */
#define RT_SQLDA                257     /* SQLDA */

#define RT_INFOBIT              270     /* SQLGetInfo bitmask value table */
#define RT_INFOINT              271     /* SQLGetInfo integer value table */
#define RT_INFOSTR              272     /* SQLGetInfo string value table */
#define RT_INFOTYPE             273     /* SQLGetTypeInfo data buffer */
#define RT_INFOTYPE_OPENSQL     274     /* SQLGetTypeInfo data subset for OpenSQL */

#define RT_CAT_STRINGID         300     /* Catalog String ID table */
#define RT_CAT_ACCESS_OWNER     310     /* SQLTables owner query from view */
#define RT_CAT_ACCESS_RECORD    311     /* SQLTables network record query from view */
#define RT_CAT_ACCESS_SYSTEM    312     /* SQLTables system table query from view */
#define RT_CAT_ACCESS_TABLE     313     /* SQLTables user table list from view */
#define RT_CAT_ACCESS_VIEW      314     /* SQLTables view list from view */
#define RT_CAT_COLUMNS          320     /* SQLColumns query */
#define RT_CAT_COLUMNS_SQLDA    321     /* SQLColumns SQLDA */
#define RT_CAT_SPEC_OINX        330     /* SQLSpecialColumns query using table proc */
#define RT_CAT_SPECIAL_COLUMNS  331     /* SQLSpecialColumns query using catalog */
#define RT_CAT_SPECIAL_COLUMNS_GATEWAY1 332 /* SQLSpecialColumns query using catalog  */
#define RT_CAT_SPECIAL_COLUMNS_GATEWAY2 333 /* SQLSpecialColumns query using catalog  */
#define RT_CAT_SPECIAL_COLUMNS_PRIMARY_KEY  334  /* SQLSpecialColumns query using primary key */
#define RT_CAT_STAT_OINX        340     /* SQLStatistics table query using proc */
#define RT_CAT_STATS_TABLE      341     /* SQLStatistics table query using catalog */
#define RT_CAT_STATS_INDEX_CN   342     /* SQLStatistics non-unique clustered index query */
#define RT_CAT_STATS_INDEX_CU   343     /* SQLStatistics unique clustered index query */
#define RT_CAT_STATS_INDEX_HN   344     /* SQLStatistics non-unique hash index query */
#define RT_CAT_STATS_INDEX_HU   345     /* SQLStatistics unique hash index query */
#define RT_CAT_STATS_INDEX_ON   346     /* SQLStatistics non-unique other index query */
#define RT_CAT_STATS_INDEX_OU   347     /* SQLStatistics unique other index query */
#define RT_CAT_TABLES_OWNER     350     /* SQLTables owner query from SCHEMA */
#define RT_CAT_TABLES_RECORD    351     /* SQLTables network record query from TABLE */
#define RT_CAT_TABLES_SYSTEM    352     /* SQLTables system table query from TABLE */
#define RT_CAT_TABLES_TABLE     353     /* SQLTables user table list from TABLE */
#define RT_CAT_TABLES_TYPES     354     /* SQLTables type list from constant query */
#define RT_CAT_TABLES_VIEW      355     /* SQLTables view list from TABLE */
#define RT_CAT_PROC             360     /* SQLProcedures query */
#define RT_CAT_PROCCOL          361     /* SQLProcedureColumns query */
#define RT_CAT_PROCCOL_GATEWAY  362     /* SQLProcedureColumns query using gateway tables */
#define RT_CAT_PRIMARY_KEYS1    363     /* SQLPrimaryKeys using iialt_columns */
#define RT_CAT_PRIMARY_KEYS2    364     /* SQLPrimaryKeys using iitables and iicolumns */
#define	RT_CAT_USE_SYSTABLES_VIEW 365   /* SQLTables use system table query from TABLE */
#define RT_CAT_TABLES_USESYSTABLE 366

/*
**  User defined resource ids:
*/
#define IDU_IDMS                1       /* Driver for CA-IDMS (MF) */
#define IDU_IDMSUNIX            2       /* Driver for IDMS/UNIX */
#define IDU_CADB                3       /* Driver for CA-DB (VAX) */
#define IDU_CADBUNIX            4       /* Driver for CA-DB/UNIX */
#define IDU_DATACOM             5       /* Driver for Datacom/UNIX */
#define IDU_INGRES              6       /* Driver for Ingres */
#define IDU_DBCS                10      /* Type info CA-IDMS with DBCS */
#define IDU_IDMSOINX            11      /* Stringid for CA-IDMS with OINX */
#define IDU_COLUMNS             20      /* Resource used by SQLColumns */
#define IDU_TYPES               21      /* Resource used by SQLTables, type list */
#define IDU_TYPEINFO            22      /* Resource used by SQLGetTypeInfo */

#endif  /* _INC_IDMSOINF */
