/*
* SQLCA                                   05/18/89 15:11:35   01/18/95
*CONTAINS PTF# PORT SERVER 3.0 FROM PC.                DILJU01 01/18/95
*CONTAINS PTF# REMOVED REFERENCE TO SQLSID TYPE        DILJU01 06/15/94
*
* 07/02/1997 Jean Teng added pointer to handle multiple messages
* 02/03/1999 Dave Thole      Converted to use iiapidep.h
* 06/24/1999 Dave Thole   Added SQLCODE, SQLERC to SQLCA_MSG_TYPE
* 05/08/2000 Dave Thole   Added pdbc
* 11/05/2002 Ralph Loen   Removed dependency on API data types.
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**      17-nov-2003 (loera01)
**          Cast SQLCODE to RETCODE.
*/

 /*******************************************************************
 *                                                                  *
 *   Header file for the description of SQLCA                       *
 *                                                                  *
 ********************************************************************/

#ifndef _INC_SQLCA
#define _INC_SQLCA

#ifndef _WIN32   /* for non-Windows platforms */
#define FAR
#define NEAR
#define BOOL bool
#define DWORD UDWORD
#define WORD  UWORD
#define register
#endif /* _WIN32 */

typedef struct tsqlca_msg * LPSQLCAMSG; 

typedef struct tsqlca_msg
{
    char           SQLERM[256];    /* Text of error message         */
    LPSQLCAMSG     SQLPTR;         /* ptr to next sqlca_msg_type    */
    RETCODE        SQLCODE;        /* SQL error code                */
    long           SQLERC;         /* Extended info error code      */
    unsigned char  SQLSTATE[5];    /* ANSI SQLSTATE value           */
    BOOL           isaDriverMsg;   /* TRUE=Driver msg; FALSE=DBMS msg */
    DWORD          irowCurrent;    /* Current row of rowset in error*/
    DWORD          icolCurrent;    /* Current col of row in error   */
}  SQLCA_MSG_TYPE;

 typedef struct tsqlca  {

    char           SQLCAID[8];     /* SQLCA* eyecatcher             */
    RETCODE        SQLCODE;        /* SQL error code                */
    long           SQLSS1[2];      /* Reserved                      */
    long           SQLERC;         /* Extended info error code      */
    i4             SQLSREL;        /* Server release number         */
    i4             SQLNRP;         /* Number of rows processed      */
    i4             SQLRS2;         /* reserved                      */
    i4             SQLSER;         /* Error offset                  */
    i4             SQLRS3;         /* reserved                      */
    i4             SQLLNO;         /* Source file line number       */
    i4             SQLMCT;         /* error message count           */
    i4             SQLARC;         /* archive info                  */
    i4             SQLFJB;         /* #free jnl buffers             */
    i4             SQLRS4[2];      /* reserved                      */
    i4             SQLERL;         /* Length of error message       */
#define SQLCA_BASE_LEN   72
    char           SQLSTATE[5];    /* ANSI SQLSTATE value           */
    char           SQLRS5[11];     /* reserved                      */
    char           SQLERRP[8];     /* DB2T                          */
    char           SQLWARN[8];     /* DB2T                          */
    LPSQLCAMSG     SQLPTR;         /* -> next sqlca msg if exists   */
    void *         pdbc;           /* -> parent connection          */
    DWORD          irowCurrent;    /* Current row of rowset in error*/
    DWORD          icolCurrent;    /* Current col of row in error   */
  }  SQLCA_TYPE;
 typedef SQLCA_TYPE    (* SQLCA_P);

#endif

