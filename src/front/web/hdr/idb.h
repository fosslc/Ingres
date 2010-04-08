/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: idb.h
**
** Description:
**      Public header for the ICE database interface. The IDB is a
**      general purpose wrapper for the Ingres API.
**
** History:
**      20-nov-1996 (wilan06)
**          Created
**      20-oct-1996 (harpa06)
**          Added GetProcedureResult();
**      15-jan-1998 (harpa06)
**          Added support for obtaining DBMS error messages. s88270
**      16-Apr-98 (fanra01)
**          Add a connect timeout parameter.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
# ifndef ICEDBM_INCLUDED
# define ICEDBM_INCLUDED

# include <iiapi.h>

/* ICE result type */
typedef int     ICERES;

/* execute procedure parameter block. */
typedef struct tagIDBPROCPARMBLOCK
{
    II_UINT2            cParm;
    IIAPI_DESCRIPTOR*   aDesc;
    IIAPI_DATAVALUE*    aParmData;
} IDBPROCPARMBLOCK, * PIDBPROCPARMBLOCK; /* ippb */

typedef struct tagIDBFETCHROWBLOCK
{
    int                 cColumns;       /* total no of columns to be fetched */
    int                 cColumnGroups;  /* number of structs in agcp */
    int                 cBlobs;         /* number of blob columns */
    bool                fDataEnd;       /* no more data to fetch if TRUE */
    char*               pRowBuffer;     /* ptr to start of row data */
    void**              aUserData;      /* ptr to array of cBlobs elements */
    IIAPI_GETCOLPARM*   agcp;           /* structs passed to getColumns() */
} IDBFETCHROWBLOCK, *PIDBFETCHROWBLOCK;

typedef II_PTR          HIDBCONN;
typedef II_PTR          HIDBSTMT;
typedef II_PTR          HIDBTRAN;

typedef ICERES (FNBLOBHANDLER)(IIAPI_DATAVALUE*, IIAPI_DESCRIPTOR*, II_BOOL,
                               void*);

typedef FNBLOBHANDLER *PFNBLOBHANDLER;

/* Externally visible function declarations */
ICERES IDBOpenConnection (char* vnode, char* userID, char* password,
                          II_PTR* hconn, char** ppszMessage, i4  nTimeout);
ICERES IDBCloseConnection (II_PTR hconn, char** ppszMessage);

ICERES IDBExecuteQuery (II_PTR hconn, char* query, II_PTR* phstmt,
                        II_PTR* phtran, IIAPI_GETDESCRPARM* pgdp,
                        PIDBFETCHROWBLOCK* ppidfr, char** ppszMessage);
ICERES IDBFetchResultRow (II_PTR hstmt, IIAPI_GETDESCRPARM* pgdp,
                          PIDBFETCHROWBLOCK pidfr, PFNBLOBHANDLER pfnBlob);
ICERES IDBCloseStatement (II_PTR hstmt, PIDBFETCHROWBLOCK pidfr);
ICERES IDBCancelStatement (II_PTR hstmt, PIDBFETCHROWBLOCK pidfr);
ICERES IDBCommit (II_PTR htran);
ICERES IDBRollback (II_PTR htran);
ICERES IDBGetRowCount (II_PTR hstmt, II_LONG* prowCount);
ICERES IDBIsBlobColumn (IIAPI_DESCRIPTOR* desc);

ICERES IDBConvertData (IIAPI_DESCRIPTOR* pds, IIAPI_DATAVALUE* dv, char* result,
                       int cbResult);
int IDBGetFieldWidth (IIAPI_DESCRIPTOR* pds, IIAPI_DATAVALUE* pdv);
ICERES IDBExecuteProcedure (II_PTR hconn, char* procName, II_PTR* phstmt,
                            II_PTR* phtran, PIDBPROCPARMBLOCK pippb,
                            char** ppszMessage);
ICERES IDBCreateProcParmBlock (int cParm, PIDBPROCPARMBLOCK* ppippb);
ICERES IDBFreeProcParmBlock (PIDBPROCPARMBLOCK pippb);
ICERES IDBSetProcParm (PIDBPROCPARMBLOCK pippb, II_ULONG ix, char* name,
                       IIAPI_DT_ID type, II_BOOL isNull, II_UINT2 length,
                       void* value);

ICERES IDBInit (void);
ICERES IDBTerm (void);


/* Internal utility functions */
ICERES GetStmtDesc (II_PTR hstmt, IIAPI_GETDESCRPARM* pgdp,char** ppszMessage);
ICERES SetStmtDesc (II_PTR hstmt, II_INT2 cParm, IIAPI_DESCRIPTOR* pdesc,
                    char** ppszMessage);
ICERES PutProcedureParms (II_PTR hstmt, IDBPROCPARMBLOCK* pippb, 
                          char** ppszMessage);
ICERES GetProcedureResult (II_PTR hstmt, char** ppszMessage);
ICERES GetApiResult (IIAPI_GENPARM* genParm, char **ppszMessage);
ICERES DoExecuteQuery (II_PTR hconn, char* query, II_LONG qt, II_PTR* phstmt,
                       II_PTR* phtran, char **ppszMessage);
II_LONG GetQueryType (char *query);

void nothing ();

# ifdef DEBUG
#  define DBPRINT printf
# else
#  define DBPRINT nothing
# endif

# define NULL_STR_REP    "(NULL)"

/* lengths of various numbers as strings, including - sign */
# define MAX_FLOAT_STR_LEN       16
# define MAX_DBL_STR_LEN         24
# define MAX_INT1_STR_LEN        4
# define MAX_INT2_STR_LEN        6
# define MAX_INT4_STR_LEN        11
# define MAX_DATETIME_STR_LEN    26

/* various SQL keywords */
# define SELECT_KEYWORD         "SELECT "
# define EXECPROC_KEYWORD       "EXECUTE PROCEDURE "
# define EXECIMMED_KEYWORD      "EXECUTE IMMEDIATE "
# define EXECUTE_KEYWORD        "EXECUTE "


# endif
