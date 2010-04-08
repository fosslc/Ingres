/*
** Copyright (c) 2009 Ingres Corporation
*/

/*
** Name: idmsxsql.h
**
** Description:
**	Header file for IDMS/DB SQL interface.
**
**	#define XSQ_NOBLK  to exclude SQL control blocks.
**
** History:
**	18-dec-1991 (rosda01)
**	    Initial coding.
**	01-sep-1992 (rosda01)
**	    Added Commit Continue.
**	25-nov-1992 (rosda01)
**	    Unbundled from SQI.
**	14-apr-1994 (dilju01)
**	    Added QSRV trace flag for CADB.
**	09-jan-1997 (tenje01)
**	    Added connH/tranH/pStmt to SQB.
**	17-jan-1997 (tenje01)
**	    Added SqlCancel.
**	07-feb-1997 (thoda04)
**	    Moved connH/tranH to SESS.
**	03-mar-1997 (tenje01)
**	    Added pDbc to SQB.
**	17-feb-1999 (thoda04)
**	    Conversion to UNIX.
**	12-jul-2001 (somsa01)
**	    Cleaned up compiler warnings.
**      05-nov-2002 (loera01)
**          Removed dependency on API datatypes.
**      17-oct-2003 (loera01)
**          Added SqlExecDirect, SqlExecProc, and SqlPutParms.  Removed
**          SqlExecuteBulk.
**      13-nov-2003 (loera01)
**          Removed obsolete arguments from SqlToIngresAPI() and
**          added SqlPutSegment().
**     20-nov-2003 (loera01)
**          More cleanup on arguments sent to SqlToIngresAPI().
**          Remove obsolete SqlToIngresAPI() macro definitions.
**     04-dec-2003 (loera01) SIR 111409
**          Added SqlFetchSegment().
**     18-jun-2004 (Ralph Loen) SIR 116260
**          Added SqlDescribeInput().
**   30-Jan-2008 (Ralph Loen) SIR 119723
**          Added SqlSetPos().
**     05-jan-2009 (Ralph Loen) Bug 121358
**          Added SqlFetchUnbound(). 
*/

#ifndef _INC_IDMSXSQL
#define _INC_IDMSXSQL

/*#include "sqdsiinc.h" */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NT_GENERIC
typedef char * LPSTR;
typedef void * LPVOID;
#endif

#ifdef  NT_GENERIC            /* passed ini tag and dbname */
#define DBNLEN 16
#else
#define DBNLEN 8
#endif

/*
*   IDMS SQL control block anchor:
*
*   This block serves as an anchor to the IDMS SQL protocol
*   blocks that are passed on every call.  The blocks must
*   be allocated and the SQB must be set up before the first
*   SQL call.
*/
typedef struct tSQB {
   struct tSESS FAR *pSession;     /* -->SQL session anchor       */
   SQLCA_TYPE  FAR *pSqlca;        /* -->SQL communication area   */
   unsigned short  Trace;          /* trace option flags          */
   unsigned short  Options;        /* piggyback and other options */
   void FAR *pStmt;                /* -> current stmt control blk */
   void FAR *pDbc;                 /* -> current dbc              */
} SQB,
  FAR * LPSQB;

#define     OPT_IDMSCALL   0x0001  /* Trace IDMS calls            */
#define     OPT_IDMSTIME   0x0002  /* Time  IDMS calls            */
#define     OPT_IDMSSID    0x0004  /* Snap  IDMS SQLSID           */
#define     OPT_IDMSDSI    0x0008  /* Snap  IDMS DSICB            */
#define     OPT_IDMSCA     0x0010  /* Snap  IDMS SQLCA            */
#define     OPT_IDMSCIB    0x0020  /* Snap  IDMS SQLCIB           */
#define     OPT_IDMSPIB    0x0040  /* Snap  IDMS SQLPIB           */
#define     OPT_IDMSPBF    0x0080  /* Snap  IDMS parm buffer      */
#define     OPT_IDMSTBF    0x0100  /* Snap  IDMS tuple buffer     */
#define     OPT_IDMSDAI    0x0200  /* Snap  IDMS input SQLDA      */
#define     OPT_IDMSDAO    0x0400  /* Snap  IDMS output SQLDA     */
#define     OPT_IDMSEXS    0x0800  /* Snap  IDMS syntax string    */
#define     OPT_QSRVTRACE  0x8000  /* Trace calls to QSRV         */
#define     OPT_QSRVDEBUG  0x4000  /* Snap key QSRV control blocks*/

#define     SQB_OPEN       0x0001  /* Piggyback open              */
#define     SQB_CLOSE      0x0002  /* Piggyback close             */
#define     SQB_COMMIT     0x0004  /* Piggyback commit            */
#define     SQB_COMMITC    0x0008  /* Piggyback commit continue   */
#define     SQB_SUSPEND    0x0010  /* Piggyback suspend           */

/*
*   SQL interface function prototypes:
*/
RETCODE SqlToIngresAPI (LPSQB, i2, LPSTR);

/*
*   IDMS SQL call macros:
*
*   Parms: sqb   -->SQL Protocol Block anchor
*          com   -->SQL command code
*   The next three macro definitions point at the same argument.
*          dbn   -->DBName
*          syn   -->syntax
*          tbuf  -->tuple (fetch) buffer
*
*/
#define SqlX(sqb,com)            SqlToIngresAPI(sqb,com,(LPSTR)0)
#define SqlClose(sqb)            SqlX(sqb,SQLCLOSE)
#define SqlCommit(sqb)           SqlX(sqb,SQLCOMMIT)
#define SqlConnect(sqb,dbn)      SqlToIngresAPI(sqb,SQLCONNECT,dbn)
#define SqlDescribeInput(sqb) \
                                 SqlX(sqb,SQLDESCRIBE_INPUT)
#define SqlExecute(sqb) \
                                 SqlX(sqb,SQLEXECUTE)
#define SqlExecProc(sqb) \
                                 SqlX(sqb,SQLEXECPROC)
#define SqlPutParms(sqb) \
                                 SqlX(sqb,SQLPUTPARMS)
#define SqlPutSegment(sqb) \
                                 SqlX(sqb,SQLPUTSEGMENT)
#define SqlExecImm(sqb,syn)  SqlToIngresAPI(sqb,SQLEXECUTI,syn)
#define SqlFetch(sqb,tbuf) \
                                 SqlToIngresAPI(sqb,SQLFETCH,tbuf)
#define SqlFetchSegment(sqb,tbuf) \
                                 SqlToIngresAPI(sqb,SQLFETCHSEG,tbuf)
#define SqlFetchUnbound(sqb,tbuf) \
                                 SqlToIngresAPI(sqb,SQLFETCHUNBOUND,tbuf)
#define SqlOpen(sqb) \
                                 SqlX(sqb,SQLOPEN)
#define SqlPrepare(sqb,syn) \
                SqlToIngresAPI(sqb,SQLPREPARE,syn)
#define SqlExecDirect(sqb,syn) \
                SqlToIngresAPI(sqb,SQLEXEC_DIRECT,syn)
#define SqlRelease(sqb)          SqlX(sqb,SQLRELEASE)
#define SqlRollback(sqb)         SqlX(sqb,SQLROLLBCK)
#define SqlSuspend(sqb)          SqlX(sqb,SQLSUSPEND)
#define SqlCancel(sqb)           SqlX(sqb,SQLCANCEL)
#define SqlSetPos(sqb)           SqlX(sqb,SQLSETPOS)

#ifdef __cplusplus
}
#endif

#endif  /* _INC_IDMSXSQL */
