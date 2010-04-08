/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
**
**  Project : libmon library for the Ingres Monitor ActiveX control
**
**  Source : sqltls.sc
**
**  derived from the winmain\sqltls.sc file 
**  sql tools
**
**  History
**
**	20-Nov-2001 (noifr01)
**      extracted and adapted from the winmain\sqltls.sc file
*/


#define STD_SQL_CALL

#include "libmon.h"
#include "error.h"

exec sql include sqlca;
exec sql include sqlda;

#define LONGVARCHARDISPCHAR 40
#define LVCNULLSTRING "<NULL>"
#define MAXLVC 2000
int * poslvc;
int * ilvc;
int nblvc = 0;

#include "tchar.h"
#include "gl.h"
#include "iicommon.h"

typedef IISQLDA FAR *LPSQLDA;

static int SQLCodeToDom(long sqlcode); // to be removed when SQLDynn OK

static BOOL   bSqlErrorManagement;
static BOOL   bSqlErrorSaveText;

static char  tcTmpFileName[_MAX_PATH]= ("");
VOID LIBMON_SetTemporaryFileName(LPTSTR pFileName)
{
  x_strcpy(tcTmpFileName,pFileName);
  if (!tcTmpFileName[0])
    LIBMON_DisableSqlErrorManagmt();
  else
    LIBMON_EnableSqlErrorManagmt();
}

LPTSTR LIBMON_GetTemporaryFileName()
{
   return tcTmpFileName;
}

LPUCHAR SQLGetErrorText(long *lplret)
{  
   exec sql begin declare section;
   unsigned char *SQLErrorTxt;
   long lret=0;
   exec sql end declare section;

   SQLErrorTxt=ESL_AllocMem(SQLERRORTXTLEN);   // enough for now
   if (SQLErrorTxt) 
      exec sql inquire_sql (:SQLErrorTxt=errortext, :lret=errorno); 
   *lplret=lret;
   return SQLErrorTxt;
}

static BOOL bLastErr50 = FALSE;

BOOL isLastErr50()
{
   return bLastErr50;
}

// lpsqlda defined as void far * in the include
int ExecSQLImm1(LPUCHAR lprequest,BOOL bcommit, void far *lpsqldav,
      BOOL (*lpproc)(void *), void *usrparm, long *presultcount)
/******************************************************************
* Function : dynamicaly executes an sql statement                 *
* Parameters :                                                    *
*     lprequest : SQL statement to be executed.                   *
*     bcommit   : a boolean telling whenever or not the proc has  *
*                 commit after the statement has been executed.   *                             
*                 bcommit=TRUE and lpproc not NULL is not allowed *
*     lpsqldatav: SQLDA describing the host variables to be       *
*                 returned. The SQLDA can be initialized by using *
*                 SQLDAInit. A null pointer tells no sqlda.       * 
*     proc      : address of the function to be given control for *
*                 each row returned from the database. (select    *                             
*                 loop).                                          *
*                 The proc called processes each row by filling   * 
*                 up a specific structure. The address of this    *
*                 structure is given by the caller (void *argv).  * 
*                 A null pointer means no proc to execute.        *
*     usrparm   : address of the above proc structure, or NULL.   *
*     NOTE :      if lpsqlda is null, proc and usrpar are ignored.*
* Returns : an integer (see dba.h)                                *
*                 RES_SUCCESS if succesful,                       *
*                 RES_sqlerr if a 'handled' error occured         *     
*                 RES_ERR if the error is not handled             *
******************************************************************/
{
   long lret;
   long lreturned;

   LPSQLDA lpsqlda = (LPSQLDA)lpsqldav;   // see comment

   BOOL not_fnd=FALSE;
#ifdef STD_SQL_CALL
   exec sql begin declare section;
   unsigned char *lpimmreq=lprequest;  
   exec sql end declare section;

   bLastErr50 = FALSE;
   if (lpproc && bcommit)  /* cannot do both updates and select loop */ 
      return RES_ERR;

   EnableSqlSaveText();

   if (lpsqlda)
      if (lpproc) {
         exec sql execute immediate :lpimmreq using descriptor lpsqlda;
         exec sql begin;     
               (*lpproc)(usrparm); 
         exec sql end;       
      }
      else
         exec sql execute immediate :lpimmreq using descriptor lpsqlda;
   else {
         exec sql execute immediate :lpimmreq;
   }
#else  /* DOES NOT support the "USING SQLDA" form and the BEGIN-END */
   EnableSqlSaveText();
   IIsqInit(&sqlca);
   IIwritio((long)0,(short *)0,(long)1,(long)32,(long)0,lprequest);
   IIsyncup((char *)0,(long)0);
#endif
   lret=sqlca.sqlcode;

   lreturned = (long) sqlca.sqlerrd[2];
   ManageSqlError(&sqlca); // Keep track of the SQL error if any
   switch (lret) {
     case 4702L:
       DisableSqlSaveText();
       return RES_TIMEOUT; 
       break;
     case -39100L:
       DisableSqlSaveText();
       return RES_SQLNOEXEC;
       break;
     case -30100L:
       DisableSqlSaveText();
       return RES_30100_TABLENOTFOUND;
       break;
     case -30200L:
       DisableSqlSaveText();
       return RES_ALREADYEXIST;
       break;
     case -50L:  // success with warning
       bLastErr50 = TRUE;
       break;
     case -34000L:
       DisableSqlSaveText();
       return RES_NOGRANT;
       break;
     case 100L:  // success with warning
       not_fnd=TRUE;
       break;
     case 0L:  // success 
       break;
     case 700L:   // success with warning (populate)
	 case 710L:
       break;
/*     case 710L:
       exec sql commit;
       ManageSqlError(&sqlca); // Keep track of the SQL error if any
       return RES_MOREDBEVENT;  
       break;     */
     default:
       DisableSqlSaveText();
       return RES_ERR;
   }
   *presultcount = lreturned;
   if (bcommit) {
      exec sql commit;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      if (sqlca.sqlcode!=0 && sqlca.sqlcode!=710L) {
        exec sql rollback;
        DisableSqlSaveText();
        return RES_ERR;
      }
   }

   DisableSqlSaveText();
   if (not_fnd)
     return RES_ENDOFDATA;
   else
     return RES_SUCCESS;
}

static BOOL islvc(int pos)
{
  int i;
  for (i=0;i<nblvc;i++) {
    if (ilvc[i]==pos)
      return TRUE;
  }
  return FALSE;
}
                    
int ExecSQLImm(LPUCHAR lprequest,BOOL bcommit, void far *lpsqldav,
      BOOL (*lpproc)(void *), void *usrparm)
{
	long lresultcount;
	return ExecSQLImm1(lprequest,bcommit, lpsqldav, lpproc, usrparm, &lresultcount);
}


static int SQLCodeToDom(long sqlcode) // to be removed when SQLDynn OK
{

   switch (sqlcode) {
     case 700L:   // success with warning (populate)
     case 0L :
        return RES_SUCCESS;
        break;
     case 100L :
        return RES_ENDOFDATA;
        break;
     case 4702L:
       return RES_TIMEOUT; 
       break;
     case -30100L:
       return RES_30100_TABLENOTFOUND;
       break;
     case -30200L:
       return RES_ALREADYEXIST;
       break;
     case -50L:  // success with warning
       return RES_SUCCESS; // should be SUCCESSWANING   
       break;
     case -34000L:
       return RES_NOGRANT;
       break;
     default:
       return RES_ERR;
   }
}


// SQL error Management 

void DisableSqlSaveText()
{
   exec sql begin declare section;
   int iSaveQuery;
   exec sql end declare section;

   exec sql inquire_sql(:iSaveQuery=savequery);

   if (iSaveQuery) {
      bSqlErrorSaveText=FALSE;
      exec sql set_sql(savequery=0);
   }
}

void EnableSqlSaveText()
{
   exec sql begin declare section;
   int iSaveQuery;
   exec sql end declare section;

   exec sql inquire_sql(:iSaveQuery=savequery);

   if (!iSaveQuery) {
      bSqlErrorSaveText=TRUE;
      exec sql set_sql(savequery=1);
   }
}

void LIBMON_EnableSqlErrorManagmt()
{
   bSqlErrorManagement=TRUE;
}
void LIBMON_DisableSqlErrorManagmt()
{
   bSqlErrorManagement=FALSE;
}

static void LIBMON_WriteInFile(LPCURRENTSQLERRINFO lpFreeEntry, char * FileName)
{
  char      buf[256];
  char      buf2[256];
  HFILE     hFile;
  OFSTRUCT  openBuff;
  char      *p, *p2;
  if (FileName[0])
  {
    // open for write with share denied,
    // preceeded by create if does not exist yet
    hFile = OpenFile(FileName, &openBuff, OF_EXIST);
    if (hFile != HFILE_ERROR)
      hFile = OpenFile(FileName, &openBuff, OF_SHARE_DENY_NONE | OF_WRITE);
    else {
      hFile = OpenFile(FileName, &openBuff, OF_CREATE);
      if (hFile != HFILE_ERROR) {
        _lclose(hFile);
        hFile = OpenFile(FileName, &openBuff, OF_SHARE_DENY_NONE | OF_WRITE);
      }
    }

    if (hFile != HFILE_ERROR) {
      if (_llseek(hFile, 0, 2) != HFILE_ERROR) {
        // statement that produced the error
        x_strcpy(buf, "Erroneous statement:");
        _lwrite(hFile, buf, x_strlen(buf));
        _lwrite(hFile, "\r\n", 2);
        _lwrite(hFile, lpFreeEntry->tcSqlStm,
                       x_strlen(lpFreeEntry->tcSqlStm));
        _lwrite(hFile, "\r\n", 2);

        // Sql error code
        x_strcpy(buf, "Sql error code: %ld");
        wsprintf(buf2, buf, lpFreeEntry->sqlcode);
        _lwrite(hFile, buf2, x_strlen(buf2));
        _lwrite(hFile, "\r\n", 2);

        // Sql error text line after line and time after time
        x_strcpy(buf, "Sql error text:");
        _lwrite(hFile, buf, x_strlen(buf));
        _lwrite(hFile, "\r\n", 2);
        p = lpFreeEntry->tcSqlErrTxt;
        while (p) {
          p2 = x_strchr(p, '\n');
          if (p2) {
            _lwrite(hFile, p, p2-p);
            _lwrite(hFile, "\r\n", 2);
            p = ++p2;
          }
          else {
            _lwrite(hFile, p, x_strlen(p));
            _lwrite(hFile, "\r\n", 2);
            break;
          }
        }

        // separator for next statement
        memset(buf, '-', 70);
        buf[70] = '\0';
        _lwrite(hFile, buf, x_strlen(buf));
        _lwrite(hFile, "\r\n", 2);

      }
      _lclose(hFile);
    }
  }
}

// Emb 24/10/95 : log file management
static void LIBMON_ManageLogFile(LPCURRENTSQLERRINFO lpFreeEntry)
{
  char      bufenv[_MAX_PATH];
  char      *penv = getenv("II_SYSTEM");

  if (!penv) {
    MessageBox(GetFocus(),
                  ("II_SYSTEM system variable not found"),
                  NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
    return ;
  }
  fstrncpy(bufenv,penv,_MAX_PATH);


  if (bufenv[0]) {
    // build full file name with path
    x_strcat(bufenv, "\\ingres\\files\\errvdba.log");
    LIBMON_WriteInFile (lpFreeEntry, bufenv);
  }
}

static void LIBMON_ManageTemporaryLogFile(LPCURRENTSQLERRINFO lpFreeEntry)
{
  LIBMON_WriteInFile (lpFreeEntry, LIBMON_GetTemporaryFileName());
}

static BOOL LIBMON_ManageSqlError(void  *Sqlca)
{
   IISQLCA *psqlca=(IISQLCA *) Sqlca;
   CURRENTSQLERRINFO TempSqlError;
   LPUCHAR lpPartToCopy;
   LPUCHAR *lpKeyWord, Keyword[]={  /* NULL, */  /* trick not to remove text */
                              "describe ",
                              "execute immediate ",
                              "prepare dbadynstm from ",
                              NULL};
   exec sql begin declare section;
   unsigned char SQLErrorTxt[SQLERRORTXTLEN];
   unsigned char SQLQueryTxt[SQLERRORTXTLEN];
   exec sql end declare section;


   if (!bSqlErrorManagement)
      return TRUE;

   // Check if we will fill an entry, otherwise quit

   if (psqlca->sqlcode == 0)
      return TRUE;

   if (psqlca->sqlcode == 100L || psqlca->sqlcode == 700L || psqlca->sqlcode == 710L)
      return TRUE;    // populate problem - don't report even as a warning

   memset (SQLQueryTxt, 0, sizeof(SQLQueryTxt));

   if (bSqlErrorSaveText) {
      exec sql inquire_sql (:SQLQueryTxt=querytext); 
      suppspace(SQLQueryTxt);
   }
   else
      x_strcpy(SQLQueryTxt, "Unknown statement - statistic data only");

   // do not use the error text in psqlca, it is only 70 chars. Instead
   // use the inquire_sql statement to retrieve the full text

   memset (SQLErrorTxt, 0, sizeof(SQLErrorTxt));

   exec sql inquire_sql (:SQLErrorTxt=errortext);
   suppspace(SQLErrorTxt);

   TempSqlError.sqlcode=psqlca->sqlcode;

   x_strcpy(TempSqlError.tcSqlErrTxt,SQLErrorTxt);

   lpPartToCopy = NULL;     // Emb 24/10/95 otherwise crash if no keyword found
   lpKeyWord=&Keyword[0];
   while (*lpKeyWord) {
      if (lpPartToCopy=x_strstr(SQLQueryTxt, *lpKeyWord)) {
         lpPartToCopy=SQLQueryTxt+x_strlen(*lpKeyWord);
         break;
      }
      lpKeyWord++;
   }

   if (!lpPartToCopy)
      lpPartToCopy=SQLQueryTxt;

   x_strcpy(TempSqlError.tcSqlStm,lpPartToCopy);

   // Emb 24/10/95 : log file management
   LIBMON_ManageLogFile(&TempSqlError);

   LIBMON_ManageTemporaryLogFile(&TempSqlError);

   return TRUE;

}

BOOL ManageSqlError(void  *Sqlca)
{
   IISQLCA *psqlca = (IISQLCA *) Sqlca;
   long lerrcode  = psqlca->sqlcode;
   BOOL bRes      = LIBMON_ManageSqlError(Sqlca);
   psqlca->sqlcode = lerrcode;
   return bRes;
}


BOOL AddSqlError(char * statement, char * retmessage, int icode)
{
   CURRENTSQLERRINFO  SqlErrInfTemp;

   if (!bSqlErrorManagement)
      return TRUE;

   fstrncpy(SqlErrInfTemp.tcSqlStm, statement,  sizeof(SqlErrInfTemp.tcSqlStm)-1);
   fstrncpy(SqlErrInfTemp.tcSqlErrTxt, retmessage, sizeof(SqlErrInfTemp.tcSqlErrTxt)-1);

   SqlErrInfTemp.sqlcode=icode;

   // Emb 24/10/95 : log file management
   LIBMON_ManageLogFile(&SqlErrInfTemp);

   LIBMON_ManageTemporaryLogFile(&SqlErrInfTemp);

   return TRUE;
}


int MonitorGetServerInfo(LPSERVERDATAMAX pServerDataMax)
{
  exec sql begin declare section;
    long max_conn;
    char bufwhere[MAXOBJECTNAME];
  exec sql end declare section; 

  wsprintf(bufwhere,"%%%s%%",pServerDataMax->serverdatamin.listen_address);

  exec sql repeated select max_connections
           into   :max_conn
           from   ima_dbms_server_parameters
           where  server_name like :bufwhere;
  exec sql begin;
    ManageSqlError(&sqlca);
    pServerDataMax->active_max_sessions=max_conn;
  exec sql end;
  ManageSqlError(&sqlca);

  return SQLCodeToDom(sqlca.sqlcode);
}

int MonitorGetAllServerInfo(LPSERVERDATAMAX pServerDataMax)
{
  exec sql begin declare section;
    long num_sess,max_sess,num_active,max_active;
    char bufwhere[MAXOBJECTNAME];
  exec sql end declare section; 

  wsprintf(bufwhere,"%%%s%%",pServerDataMax->serverdatamin.listen_address);

  exec sql execute procedure ima_set_server_domain;
  ManageSqlError(&sqlca);
      
  exec sql commit;
  ManageSqlError(&sqlca); 
      
  exec sql execute procedure ima_set_vnode_domain;
  ManageSqlError(&sqlca); 
      
  exec sql commit;
  ManageSqlError(&sqlca);

  exec sql repeated select blk_num_sessions, blk_max_sessions, blk_num_active, blk_max_active
           into   :num_sess       , :max_sess       , :num_active   , :max_active
           from   ima_block_info
           where  server like :bufwhere;
  exec sql begin;
    ManageSqlError(&sqlca);
    pServerDataMax->current_connections     = num_sess;
    pServerDataMax->current_max_connections = max_sess;
    pServerDataMax->active_current_sessions = num_active;
    pServerDataMax->active_max_sessions     = max_active;
  exec sql end;
  ManageSqlError(&sqlca);
  if ( sqlca.sqlcode != 0 ) { 
    pServerDataMax->current_connections     = -1;
    pServerDataMax->current_max_connections = -1;
    pServerDataMax->active_current_sessions = -1;
    pServerDataMax->active_max_sessions     = -1;
  }
return SQLCodeToDom(sqlca.sqlcode);
}

int OldMonitorGetDetailSessionInfo(LPSESSIONDATAMAX psessdta)
{
  exec sql begin declare section;
    char bufloc[1001];
    char bufnew1[MAXOBJECTNAME];
    char bufnew2[MAXOBJECTNAME];
    char bufnew3[MAXOBJECTNAME];
    char bufnew4[MAXOBJECTNAME];
    char bufnew5[MAXOBJECTNAME];
    char bufnew6[MAXOBJECTNAME];
    char bufnew7[MAXOBJECTNAME];
    char bufnew8[MAXOBJECTNAME];
    char bufnew9[MAXOBJECTNAME];
    char bufnew10[MAXOBJECTNAME];
    char bufnew11[MAXOBJECTNAME];
    char bufev1[MAXOBJECTNAME];
	int iserverpid = (int) psessdta->sessiondatamin.server_pid;
  exec sql end declare section; 
  x_strcpy(bufev1,psessdta->sessiondatamin.session_id);
  if (bufev1[0] == '\0')
    return RES_ERR;
  exec sql repeated select b.session_mask  ,a.db_owner         ,a.session_group,
                  a.session_role  ,a.session_activity ,a.activity_detail,
                  a.session_query ,a.client_host      ,a.client_pid,
                  a.client_terminal,a.client_user     ,a.client_connect_string
             into :bufnew1        ,:bufnew2           ,:bufnew3 ,
                  :bufnew4        ,:bufnew5           ,:bufnew6 ,
                  :bufloc         ,:bufnew7           ,:bufnew8 ,
                  :bufnew9        ,:bufnew10          ,:bufnew11
             from ima_server_sessions a,
                  ima_server_sessions_extra b
            where a.session_id = :bufev1 and
                  a.session_id = b.session_id and
				  a.server_pid = :iserverpid and 
				  trim(a.server) = trim(b.Server)

				  ;
  exec sql begin;
    ManageSqlError(&sqlca);

    fstrncpy(psessdta->client_host, bufnew7,
                                      sizeof(psessdta->client_host));
    fstrncpy(psessdta->client_pid , bufnew8,
                                      sizeof(psessdta->client_pid));
    fstrncpy(psessdta->client_terminal, bufnew9,
                                      sizeof(psessdta->client_terminal));
    fstrncpy(psessdta->client_user, bufnew10,
                                      sizeof(psessdta->client_user));
    fstrncpy(psessdta->client_connect_string, bufnew11,
                                      sizeof(psessdta->client_connect_string));

    fstrncpy(psessdta->session_mask, bufnew1,
                                      sizeof(psessdta->session_mask));
    suppspace(bufnew2);
    if (bufnew2[0] == '\0')
      x_strcpy(psessdta->db_owner,LIBMON_getNoneString());
    else
      fstrncpy(psessdta->db_owner, bufnew2,
                                      sizeof(psessdta->db_owner));
    suppspace(bufnew3);
    if (bufnew3[0] == '\0')
      x_strcpy(psessdta->session_group,LIBMON_getNoneString());
    else
      fstrncpy(psessdta->session_group, bufnew3,
                                      sizeof(psessdta->session_group));
    suppspace(bufnew4);
    if (bufnew4[0] == '\0')
      x_strcpy(psessdta->session_role,LIBMON_getNoneString());
    else
      fstrncpy(psessdta->session_role, bufnew4,
                                      sizeof(psessdta->session_role));
    suppspace(bufnew5);
    if (bufnew4[0] == '\0')
      x_strcpy(psessdta->session_activity,LIBMON_getNoneString());
    else
      fstrncpy(psessdta->session_activity, bufnew5,
                                      sizeof(psessdta->session_activity));
    fstrncpy(psessdta->activity_detail, bufnew6,
                                      sizeof(psessdta->activity_detail));
    fstrncpy(psessdta->session_full_query, bufloc,
                                      sizeof(psessdta->session_full_query));
  exec sql end;
    ManageSqlError(&sqlca);
  return SQLCodeToDom(sqlca.sqlcode);
}

int MonitorGetDetailSessionInfo(LPSESSIONDATAMAX psessdta)
{
  exec sql begin declare section;
    char bufloc[1001];
    char bufnew1[MAXOBJECTNAME];
    char bufnew2[MAXOBJECTNAME];
    char bufnew3[MAXOBJECTNAME];
    char bufnew4[MAXOBJECTNAME];
    char bufnew5[MAXOBJECTNAME];
    char bufnew6[MAXOBJECTNAME];
    char bufnew7[MAXOBJECTNAME];
    char bufnew8[MAXOBJECTNAME];
    char bufnew9[MAXOBJECTNAME];
    char bufnew10[MAXOBJECTNAME];
    char bufnew11[MAXOBJECTNAME];
    char bufev1[MAXOBJECTNAME];
	char bufdesc[256+1];
	short null1;
	int iserverpid = (int) psessdta->sessiondatamin.server_pid;
  exec sql end declare section; 
  x_strcpy(bufev1,psessdta->sessiondatamin.session_id);
  if (bufev1[0] == '\0')
    return RES_ERR;

  if ( GetOIVers() == OIVERS_NOTOI ||
       GetOIVers() == OIVERS_12 ||
       GetOIVers() == OIVERS_20 ) {
        x_strcpy(psessdta->session_description,"<N/A>");
        return OldMonitorGetDetailSessionInfo(psessdta);
  }

  exec sql repeated select b.session_mask  ,a.db_owner         ,a.session_group,
                  a.session_role  ,a.session_activity ,a.activity_detail,
                  a.session_query ,a.client_host      ,a.client_pid,
                  a.client_terminal,a.client_user     ,a.client_connect_string,
				  c.session_desc
             into :bufnew1        ,:bufnew2           ,:bufnew3 ,
                  :bufnew4        ,:bufnew5           ,:bufnew6 ,
                  :bufloc         ,:bufnew7           ,:bufnew8 ,
                  :bufnew9        ,:bufnew10          ,:bufnew11,
				  :bufdesc:null1
             from ima_server_sessions a,
                  ima_server_sessions_extra b,
				  ima_server_sessions_desc c
            where a.session_id = :bufev1 and
                  a.session_id = b.session_id and
				  a.server_pid = :iserverpid and
				  a.session_id = c.session_id and 
				  trim(a.server) = trim(b.server) and
				  trim(a.server) = trim(c.server);
  exec sql begin;
    ManageSqlError(&sqlca);

    fstrncpy(psessdta->client_host, bufnew7,
                                      sizeof(psessdta->client_host));
    fstrncpy(psessdta->client_pid , bufnew8,
                                      sizeof(psessdta->client_pid));
    fstrncpy(psessdta->client_terminal, bufnew9,
                                      sizeof(psessdta->client_terminal));
    fstrncpy(psessdta->client_user, bufnew10,
                                      sizeof(psessdta->client_user));
    fstrncpy(psessdta->client_connect_string, bufnew11,
                                      sizeof(psessdta->client_connect_string));

    fstrncpy(psessdta->session_mask, bufnew1,
                                      sizeof(psessdta->session_mask));
    suppspace(bufnew2);
    if (bufnew2[0] == '\0')
      x_strcpy(psessdta->db_owner,LIBMON_getNoneString());
    else
      fstrncpy(psessdta->db_owner, bufnew2,
                                      sizeof(psessdta->db_owner));
    suppspace(bufnew3);
    if (bufnew3[0] == '\0')
      x_strcpy(psessdta->session_group,LIBMON_getNoneString());
    else
      fstrncpy(psessdta->session_group, bufnew3,
                                      sizeof(psessdta->session_group));
    suppspace(bufnew4);
    if (bufnew4[0] == '\0')
      x_strcpy(psessdta->session_role,LIBMON_getNoneString());
    else
      fstrncpy(psessdta->session_role, bufnew4,
                                      sizeof(psessdta->session_role));
    suppspace(bufnew5);
    if (bufnew4[0] == '\0')
      x_strcpy(psessdta->session_activity,LIBMON_getNoneString());
    else
      fstrncpy(psessdta->session_activity, bufnew5,
                                      sizeof(psessdta->session_activity));
    fstrncpy(psessdta->activity_detail, bufnew6,
                                      sizeof(psessdta->activity_detail));
    fstrncpy(psessdta->session_full_query, bufloc,
                                      sizeof(psessdta->session_full_query));
    if (null1==-1)
       x_strcpy(psessdta->session_description,"<N/A>"); 
	   else
    fstrncpy(psessdta->session_description, bufdesc,
                                      sizeof(psessdta->session_description));

 
  exec sql end;
    ManageSqlError(&sqlca);
  return SQLCodeToDom(sqlca.sqlcode);
}

int MonitorGetDetailLockListInfo( LPLOCKLISTDATAMAX plocklistdta )
{
  exec sql begin declare section;
    long  locklst_related_llb_id_id;
    long  locklst_related_count;
    long  locklst_id;
  exec sql end declare section;
    locklst_id = plocklistdta->locklistdatamin.locklist_id;
  exec sql repeated select locklist_related_llb_id_id,locklist_related_count
             into :locklst_related_llb_id_id,:locklst_related_count
             from ima_locklists
            where locklist_id = :locklst_id;
  exec sql begin;
    ManageSqlError(&sqlca);
    plocklistdta->locklist_related_llb_id_id = locklst_related_llb_id_id;
    plocklistdta->locklist_related_count     = locklst_related_count;
  exec sql end;
    ManageSqlError(&sqlca);
  return SQLCodeToDom(sqlca.sqlcode);
}

int MonitorGetDetailLockSummary( LPLOCKSUMMARYDATAMIN pLckSummary )
{
  exec sql begin declare section;
  long sumlck1,sumlck2,sumlck3,sumlck4,sumlck5,sumlck6,sumlck7,sumlck8,
       sumlck9,sumlck10,sumlck11,sumlck12,sumlck13,sumlck14,sumlck15,
       sumlck16,sumlck17,sumlck18,sumlck19,sumlck20,sumlck21,sumlck22,
       sumlck23;
  exec sql end declare section;
  exec sql repeated select lkd_stat_create_list  ,lkd_stat_release_all ,lkd_llb_inuse        ,
                  lock_lists_remaining  ,lock_lists_available ,lkd_rsh_size         ,
                  lkd_lkh_size          ,lkd_stat_request_new ,lkd_stat_request_cvt ,
                  lkd_stat_convert      ,lkd_stat_release     ,lkd_stat_cancel      ,
                  lkd_stat_rlse_partial ,lkd_lkb_inuse        ,locks_remaining      ,
                  locks_available       ,lkd_stat_dlock_srch  ,lkd_stat_cvt_deadlock,
                  lkd_stat_deadlock     ,lkd_stat_convert_wait,lkd_stat_wait        ,
                  lkd_max_lkb           ,lkd_rsb_inuse
             into :sumlck1              ,:sumlck2             ,:sumlck3 ,
                  :sumlck4              ,:sumlck5             ,:sumlck6 ,
                  :sumlck7              ,:sumlck8             ,:sumlck9 ,
                  :sumlck10             ,:sumlck11            ,:sumlck12,
                  :sumlck13             ,:sumlck14            ,:sumlck15,
                  :sumlck16             ,:sumlck17            ,:sumlck18,
                  :sumlck19             ,:sumlck20            ,:sumlck21,
                  :sumlck22             ,:sumlck23
             from ima_lk_summary_view;
  exec sql begin;
    ManageSqlError(&sqlca);
    pLckSummary->sumlck_lklist_created_list        = sumlck1;
    pLckSummary->sumlck_lklist_released_all        = sumlck2 +1L;
    pLckSummary->sumlck_lklist_inuse               = sumlck3 -1L;
    pLckSummary->sumlck_lklist_remaining           = sumlck4 +1L;
    pLckSummary->sumlck_total_lock_lists_available = sumlck5;
    pLckSummary->sumlck_rsh_size                   = sumlck6;
    pLckSummary->sumlck_lkh_size                   = sumlck7;
    pLckSummary->sumlck_lkd_request_new            = sumlck8;
    pLckSummary->sumlck_lkd_request_cvt            = sumlck9;
    pLckSummary->sumlck_lkd_convert                = sumlck10;
    pLckSummary->sumlck_lkd_release                = sumlck11+2L;
    pLckSummary->sumlck_lkd_cancel                 = sumlck12;
    pLckSummary->sumlck_lkd_rlse_partial           = sumlck13;
    pLckSummary->sumlck_lkd_inuse                  = sumlck14-1L; /* current one normally released once request done */
    pLckSummary->sumlck_locks_remaining            = sumlck15+2L; /*    "    */
    pLckSummary->sumlck_locks_available            = sumlck16;
    pLckSummary->sumlck_lkd_dlock_srch             = sumlck17;
    pLckSummary->sumlck_lkd_cvt_deadlock           = sumlck18;
    pLckSummary->sumlck_lkd_deadlock               = sumlck19;
    pLckSummary->sumlck_lkd_convert_wait           = sumlck20;
    pLckSummary->sumlck_lkd_wait                   = sumlck21;
    pLckSummary->sumlck_lkd_max_lkb                = sumlck22;
    pLckSummary->sumlck_lkd_rsb_inuse              = sumlck23-2L; /* current one normally released once request done */
  exec sql end;
    ManageSqlError(&sqlca);
  return SQLCodeToDom(sqlca.sqlcode);
}
int MonitorGetDetailLogSummary( LPLOGSUMMARYDATAMIN pLogSummary )
{
  exec sql begin declare section;
  long db_adds, db_removes, tx_begins, tx_ends, log_writes,
       log_write_ios, log_read_ios, log_waits, log_split_waits,
       log_free_waits, log_stall_waits, log_forces, log_group_commit,
       log_group_count, check_commit_timer, timer_write, kbytes_written,
       inconsistent_db;
  exec sql end declare section;
  exec sql  repeated select lgd_stat_add           ,lgd_stat_remove      ,lgd_stat_begin,
                  lgd_stat_end           ,lgd_stat_write       ,lgd_stat_writeio,
                  lgd_stat_readio        ,lgd_stat_wait        ,lgd_stat_split,
                  lgd_stat_free_wait     ,lgd_stat_stall_wait  ,lgd_stat_force,
                  lgd_stat_group_force   ,lgd_stat_group_count ,lgd_stat_pgyback_check,
                  lgd_stat_pgyback_write ,lgd_stat_kbytes      ,lgd_stat_inconsist_db
             into
                  :db_adds               ,:db_removes          ,:tx_begins,
                  :tx_ends               ,:log_writes          ,:log_write_ios,
                  :log_read_ios          ,:log_waits           ,:log_split_waits,
                  :log_free_waits        ,:log_stall_waits     ,:log_forces,
                  :log_group_commit      ,:log_group_count     ,:check_commit_timer,
                  :timer_write           ,:kbytes_written      ,:inconsistent_db
             from 
                  ima_lgmo_lgd
             where
                  vnode = dbmsinfo('ima_vnode');
  exec sql begin;

    pLogSummary->lgd_add           = db_adds;
    pLogSummary->lgd_remove        = db_removes;
    pLogSummary->lgd_begin         = tx_begins;
    pLogSummary->lgd_end           = tx_ends+1; /* current tx will be ended at display time*/
    pLogSummary->lgd_write         = log_writes;
    pLogSummary->lgd_writeio       = log_write_ios;
    pLogSummary->lgd_readio        = log_read_ios;
    pLogSummary->lgd_wait          = log_waits;
    pLogSummary->lgd_split         = log_split_waits;
    pLogSummary->lgd_free_wait     = log_free_waits;
    pLogSummary->lgd_stall_wait    = log_stall_waits;
    pLogSummary->lgd_force         = log_forces;
    pLogSummary->lgd_group_force   = log_group_commit;
    pLogSummary->lgd_group_count   = log_group_count;
    pLogSummary->lgd_pgyback_check = check_commit_timer;
    pLogSummary->lgd_pgyback_write = timer_write;
    pLogSummary->lgd_kbytes        = kbytes_written /2;/*value is number of */
                                                       /*blocks of 512 bytes*/
    pLogSummary->lgd_inconsist_db  = inconsistent_db;

  exec sql end;
    ManageSqlError(&sqlca);
  return SQLCodeToDom(sqlca.sqlcode);
}

int deriveBlockNo(char *str)
{
  long  discard;
  int  block;
  if ((sscanf(str,"<%ld,%d,%d>",&discard,&block,&discard)) != 3) {
    return(-1);
  } 
  return(block);
}


int MonitorGetDetailLogHeader( LPLOGHEADERDATAMIN pLogHeader )
{
  exec sql begin declare section;
    long block_count, block_size, buf_count, logfull_interval,
         abort_interval, cp_interval, reserved_blocks;
    char begin_addr[31];
    char end_addr[31];
    char cp_addr[31];
    char status[101]; 
  exec sql end declare section;
  exec sql repeated select lgd_status      ,lfb_hdr_lgh_count     ,lfb_hdr_lgh_size,
                  lfb_buf_cnt     ,lfb_hdr_lgh_l_logfull ,lfb_hdr_lgh_l_abort,
                  lfb_hdr_lgh_l_cp,lfb_hdr_lgh_begin     ,lfb_hdr_lgh_end,
                  lfb_hdr_lgh_cp  ,lfb_reserved_space
             into :status         ,:block_count          ,:block_size,
                  :buf_count      ,:logfull_interval     ,:abort_interval,
                  :cp_interval    ,:begin_addr           ,:end_addr,
                  :cp_addr        ,:reserved_blocks
             from ima_lgmo_lgd lgd,ima_lgmo_lfb lfb
            where lgd.vnode = lfb.vnode
                  and lfb.vnode = dbmsinfo('ima_vnode');
  exec sql begin;
	ManageSqlError(&sqlca);
    x_strcpy( pLogHeader->status     , status);
    x_strcpy( pLogHeader->begin_addr , begin_addr);
    x_strcpy( pLogHeader->end_addr   , end_addr);
    x_strcpy( pLogHeader->cp_addr    , cp_addr);
    pLogHeader->block_count       = block_count;
    pLogHeader->block_size        = block_size;
    pLogHeader->buf_count         = buf_count;
    pLogHeader->logfull_interval  = logfull_interval;
    pLogHeader->abort_interval    = abort_interval;
    pLogHeader->cp_interval       = cp_interval;
    pLogHeader->reserved_blocks   = reserved_blocks / block_size;

    /*
    ** derive the rest of the required information
    */
    pLogHeader->begin_block = deriveBlockNo(pLogHeader->begin_addr);
    pLogHeader->end_block   = deriveBlockNo(pLogHeader->end_addr);
    pLogHeader->cp_block    = deriveBlockNo(pLogHeader->cp_addr);

    pLogHeader->inuse_blocks = (pLogHeader->end_block > pLogHeader->begin_block) ?
                                pLogHeader->end_block - pLogHeader->begin_block :
                                (pLogHeader->block_count - pLogHeader->begin_block) +
                                pLogHeader->end_block;
    pLogHeader->avail_blocks = pLogHeader->block_count - (pLogHeader->reserved_blocks +
                                                          pLogHeader->inuse_blocks);
    if ((pLogHeader->cp_block +
         pLogHeader->cp_interval) > pLogHeader->block_count) {
      pLogHeader->next_cp_block = (pLogHeader->cp_block +
                                   pLogHeader->cp_interval) -
                                   pLogHeader->block_count;
    } else {
      pLogHeader->next_cp_block = (pLogHeader->cp_block +
                                   pLogHeader->cp_interval);
    }
  exec sql end;
  ManageSqlError(&sqlca);
  return SQLCodeToDom(sqlca.sqlcode);
}

int DBA_ShutdownServer ( LPSERVERDATAMIN pServer , int ShutdownType)
{
  exec sql begin declare section;
    char servername[MAXOBJECTNAME];
    char vnode[MAXOBJECTNAME];
  exec sql end declare section;

  if ((x_strcmp(pServer->server_class,"IINMSVR") == 0) ||
      (x_strcmp(pServer->server_class,"IUSVR"  ) == 0) ||
      (x_strcmp(pServer->server_class,"COMSVR" ) == 0) )  {
      return RES_SVRTYPE_NOSHUTDOWN;
  }
  
  exec sql repeated select dbmsinfo('ima_vnode') into :vnode;
  ManageSqlError(&sqlca); 
  
  exec sql commit;
  ManageSqlError(&sqlca); 

  wsprintf(servername,"%s::/@%s",vnode,pServer->listen_address);

  switch (ShutdownType) {
    case SHUTDOWN_HARD:
    {
      char szSqlStm[MAXOBJECTNAME*3];
      int ires;

      //wsprintf(szSqlStm,"set lockmode session where readlock=nolock, timeout=1");
      //ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);
      

      wsprintf(szSqlStm,"execute procedure ima_set_server_domain");
      ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);
      
      wsprintf(szSqlStm,"execute procedure ima_set_vnode_domain");
      ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);

      wsprintf(szSqlStm,"commit");
      ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);

      wsprintf(szSqlStm,"set autocommit off");
      ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);

      wsprintf(szSqlStm,"execute procedure ima_stop_server (server_id='%s')",servername);
      ires=ExecSQLImm(szSqlStm,FALSE, NULL, NULL, NULL);

      return RES_SUCCESS;
      break;
    }
    case SHUTDOWN_SOFT:
        exec sql execute procedure ima_shut_server (server_id=:servername);
      break;
    default:
      return RES_ERR;
      break;
  }
  if (sqlca.sqlcode == 700L)  { // indicates that a message in database procedure has executed.
    ManageSqlError(&sqlca);
    return RES_ERR;
  }
  ManageSqlError(&sqlca);
  return SQLCodeToDom(sqlca.sqlcode);
}

int DBA_RemoveSession ( LPSESSIONDATAMIN pSession , char *ServerName )
{
   exec sql begin declare section;
      char srvname[MAXOBJECTNAME];
      char session[MAXOBJECTNAME];
      char extradata[MAXOBJECTNAME];
   exec sql end declare section;

   if (pSession->session_id[0] == '\0' || ServerName == NULL || ServerName[0] == '\0' )
      return RES_ERR;

   x_strcpy (session,pSession->session_id);
   exec sql repeated select dbmsinfo('ima_vnode') into :extradata;
   ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
   wsprintf(srvname,"%s::/@%s",extradata,ServerName);

   //x_strcpy (srvname,ServerName);
   
   exec sql execute procedure ima_remove_session (session_id = :session ,server_id=:srvname);
   Sleep( 1400); // give time to be done before refresh occurs

   ManageSqlError(&sqlca);

   if (sqlca.sqlcode == 700L)  { // indicates that a message in database procedure has executed.
      ManageSqlError(&sqlca);    // no row found in the database procedure
      return RES_ERR;
   }
   if (sqlca.sqlcode == -37000L) // the current session has removed
      return RES_SUCCESS;

   
   return SQLCodeToDom(sqlca.sqlcode);
}

int MonitorFindCurrentSessionId(char * SessId)
{
   exec sql begin declare section;
      char session[MAXOBJECTNAME];
   exec sql end declare section;

   exec sql repeated select dbmsinfo('session_id') into :session;
   ManageSqlError(&sqlca);
   exec sql commit;
   x_strcpy (SessId,session);
   return SQLCodeToDom(sqlca.sqlcode);
}  

int MonitorGetDetailTransactionInfo   ( LPLOGTRANSACTDATAMAX lpTranDataMax)
{
   exec sql begin declare section;
      char sessid[33];
      char cp_addr[33],first_addr[33],last_addr[33];
      long state_force,state_wait,internal_pid,external_pid;
   exec sql end declare section;

   x_strcpy (sessid,lpTranDataMax->logtxdatamin.tx_transaction_id);
   exec sql repeated select tx_first_log_address, tx_last_log_address ,
                   tx_cp_log_address   , tx_state_force      ,
                   tx_state_wait       , tx_pr_id_id         ,
                   tx_server_pid
            into   :first_addr         , :last_addr          ,
                   :cp_addr            , :state_force        ,
                   :state_wait         , :internal_pid       ,
                   :external_pid
	         from
		             ima_log_transactions
            where tx_transaction_id = :sessid;
  exec sql begin;
    ManageSqlError(&sqlca);
    x_strcpy (lpTranDataMax->tx_first_log_address ,  first_addr);
    x_strcpy (lpTranDataMax->tx_last_log_address  ,  last_addr);
    x_strcpy (lpTranDataMax->tx_cp_log_address    ,  cp_addr);
    lpTranDataMax->tx_state_force       =  state_force;
    lpTranDataMax->tx_state_wait        =  state_wait;
    lpTranDataMax->tx_internal_pid      =  65536L+internal_pid;
    lpTranDataMax->tx_external_pid      =  external_pid;
  exec sql end;
  ManageSqlError(&sqlca);
  return SQLCodeToDom(sqlca.sqlcode);
}



int DBA_OpenCloseServer (LPSERVERDATAMIN pServer ,BOOL bOpen)
{
  exec sql begin declare section;
    char servername[MAXOBJECTNAME];
    char vnode[MAXOBJECTNAME];
  exec sql end declare section;

  char szSqlStm[MAXOBJECTNAME*3];
  int ires;

  wsprintf(szSqlStm,"set autocommit off");
  ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);
  if (ires != RES_SUCCESS)
    ManageSqlError(&sqlca); 

  exec sql repeated select dbmsinfo('ima_vnode') into :vnode;
  if (sqlca.sqlcode != 0)
    ManageSqlError(&sqlca);


  wsprintf(servername,"%s::/@%s",vnode,pServer->listen_address);

  wsprintf(szSqlStm,"execute procedure ima_set_server_domain");
  ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);
  if (ires != RES_SUCCESS)
    ManageSqlError(&sqlca); 

  wsprintf(szSqlStm,"execute procedure ima_set_vnode_domain");
  ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);
  if (ires != RES_SUCCESS)
    ManageSqlError(&sqlca); 

  wsprintf(szSqlStm,"commit");
  ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);
  if (ires != RES_SUCCESS)
    ManageSqlError(&sqlca); 


  if (bOpen)
  {
    wsprintf(szSqlStm,"execute procedure ima_open_server (server_id='%s')",servername);
    ires=ExecSQLImm(szSqlStm,FALSE, NULL, NULL, NULL);
  }
  else
  {
    wsprintf(szSqlStm,"execute procedure ima_close_server (server_id='%s')",servername);
    ires=ExecSQLImm(szSqlStm,FALSE, NULL, NULL, NULL);
  }
  if (ires != RES_SUCCESS)
    ManageSqlError(&sqlca);

  wsprintf(szSqlStm,"commit");
  ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);
  if (ires != RES_SUCCESS)
    ManageSqlError(&sqlca);

  if (sqlca.sqlcode == 700L)  { // indicates that a message in database procedure has executed.
    ManageSqlError(&sqlca);
    return RES_ERR;
  }
  return SQLCodeToDom(sqlca.sqlcode);
}
