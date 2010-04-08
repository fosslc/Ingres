/*--------------------------------------------------------------------*
*    Copyright (c) 2009, Ingres Corporation                           *
*                                                                     *
*    Module Name:      SQLCMD.H                                       *
*    Description:      Command code definitions                       *
*                                                                     *
*    Mods                                                             *
*                                                                     *
*    Initials     Date            Description                         *
*    --------     --------        ------------------------------------*
*      BYB        01/20/90        registration                        *
*    Jean Teng    01/17/1997	  added sqlcancel                     * 
*    Ralph Loen   03/14/2003      Removed double question marks from  *
*                                 some comments to prevent trigraph   *
*                                 compiler warnings.                  *
*    Ralph Loen   10/17/2003      Added SQLEXEC_DIRECT, SQLEXEC_PROC, *
*                                 and SQL_PUTPARMS.                   *
*    Ralph Loen   11/13/2003      Added SQL_PUTSEGMENT.               *
*    Ralph Loen   11/20/2003      Removed obsolete command            *
*    Ralph Loen   12/04/2003      Added SQLFETCHSEG.                  *
*                                 definitions.                        *
*    Ralph Loen   06/18/2006      Added SQLDESCRIBE_INPUT. SIR 116260 *
**   30-Jan-2008 (Ralph Loen) SIR 119723
**          Added SQLSETPOS.
*    Ralph Loen   01/05/2009      Added SQLFETCHUNBOUND. Bug 121358   *
*                                                                     * 
*--------------------------------------------------------------------*/


#define      SQLNOP     0     /* no-op commands      */
#define      SQLCANCEL  1     /* cancels sql processing */  
#define      SQLCLOSE   2
#define      SQLCOMMIT  3
#define      SQLCONNECT 4
#define      SQLDECLARE 5
#define      SQLDESCRIB 6
#define      SQLDELETE   7
#define      SQLEXEC_DIRECT 8 /* Execute direct */
#define      SQLEXECPROC    9 /* Execute procedure */
#define      SQLEXECUTE 10
#define      SQLEXECUTI 11
#define      SQLFETCH   12
#define      SQLFETCHSEG  13
#define      SQLINSERT  14
#define      SQLOPEN    15
#define      SQLPREPARE 16
#define      SQLPUTPARMS    17 /* Put parameters */
#define      SQLPUTSEGMENT  18 /* Put segments */
#define      SQLRELEASE 19
#define      SQLROLLBCK 20
#define      SQLSUSPEND 21
#define      SQLSELECT  22
#define      SQLDESCRIBE_INPUT  23
#define      SQLSETPOS          24
#define      SQLFETCHUNBOUND 25
