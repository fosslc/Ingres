/*****************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : Visual DBA
**
**    Source : dbasqlpr.c
**    sql build functions and some internal statics one
**
**    Author : Lionel Storck
**
**    History after 04-May-1999:
**
**    23-June-1999 (schph01)
**      bug 97688 :
**      Modify the structure for display "byte varying" type whith the length.
**    20-Jan-2000 (uk$so01)
**      (Bug 100063) Eliminate the undesired compiler's warning
**    26-Mar-2001 (noifr01)
**      (sir 104270) removal of code for managing Ingres/Desktop
**    21-Jun-2001 (schph01)
**      (SIR 103694) STEP 2 support of UNICODE datatypes.
**    08-Jul-2002 (schph01)
**      BUG #108199 In insert statements, ensure there is a space after the
**      comma sign that separates the contains of two columns.
**    27-Mar-2003 (schph01 and noifr01)
**      (sir 107523) management of sequences
**    02-Jun-2003 (schph01)
**      BUG #110316 Add function GenSortedObjList() to generate the column
**      key list with previous order.
**    12-May-2004 (schph01)
**      SIR #111507 Add new column type bigint
**    01-Jul-2004 (schph01)
**      SIR #112590 generate the concurrent_updates option for modify command.
**    07-Apr-2005 (srisu02)
**     Add functionality for 'Delete password' checkbox in 'Alter User' dialog box
**    30-May-2005 (lakvi01)
**		BUG 114596 make sure that the user name is quoted (if required) for
**		the grant statement of SQL.
**    28-Feb-2006 (lunbr01)
**      BUG 115804 Make sure that the user DBMS and role passwords are quoted 
**      for create and alter user and role statements of SQL in case the
**      password contains uppercase or special chars.
**    12-Oct-2006 (wridu01)
**      Bug 116835 Add support for Ansi Date/Time data types
**    06-Nov-2007 (drivi01)
**	Update query for deleting cdds from dd_cdds table to produce a
**	better query plan.
**    02-apr-2009 (drivi01)
**      In efforts to port to Visual Studio 2008, clean up warnings.
**    12-May-2010 (drivi01)
**      Upddated BuildSQLCreIdx function with VECTORWISE_IX 
**      index storage structure.
**    04-Jun-2010 (horda03) b123227
**      Syntax for drop grant on sequence incorrect. "next" is missing as is the
**      CASCADE/RESTRICT text at the end.
******************************************************************************/
#include "dba.h"
#include "dbaset.h"
#include "dbaparse.h"
#include "error.h"
#include "dbadlg1.h"
#include "dbaginfo.h"
#include "rmcmd.h"
#include "domdata.h"
#include "msghandl.h"
#include "dll.h"
#include "resource.h"

static char STR_ON[]=" on ";
static char STR_COMMA[]=", ";
static char STR_ENDSTM[]=" ) ";
static char STR_RPAREN[]=")";
static char STR_LOCATION[]=" , location=(";
static char STR_DEFAULTPRIV[]=" default_privileges = (";
static char STR_NODEFAULTPRIV[]=" nodefault_privileges ";
static char STR_DEFAULTALLPRIV[]=" default_privileges = all ";
static char STR_NOEXPIREDATE[]=" noexpire_date";
static char STR_EXPIREDATE[]=" expire_date=";
static char STR_NOSECLABEL[]=" nolimiting_security_label ";
static char STR_SECLABEL[]=" limiting_security_label=";
static char STR_PROFILE[]=" profile=";
static char STR_NOPROFILE[]=" noprofile";
static char STR_PASSWORD[]=" password=";
static char STR_NOPASSWORD[]=" nopassword";
static char STR_EXTRNLPASSWORD[]=" external_password";
static char STR_SECAUDIT[]=" , security_audit = (";
static char STR_QUOTE[]="'";
static char STR_WITHGRP[]=" with group = ";
static char STR_NOGRP[]=" with nogroup ";
static char STR_PRIV[]=" privileges = (";
static char STR_NOPRIV[]=" noprivileges ";
static char STR_MODIFY[]="modify ";
static char STR_TO[]=" to ";
static char STR_UNIQUE[]=" unique ";
static char STR_WITH[]=" with ";
static char STR_USER[]=" user ";
static char STR_GROUP[]=" group ";
static char STR_ROLE[]=" ROLE ";
static char szBuffer[MAXOBJECTNAME];


static BOOL DBACreateReplicRules(LPUCHAR lpNodeName, LPUCHAR lpDBName,LPUCHAR lpTblName, int callsession)
{
   char buf[200];
   char buf2[200];
   char bufUser[100];
   if (!DBAGetUserName(lpNodeName,bufUser)) {
      ActivateSession(callsession);
      return FALSE;
   }

   wsprintf(buf, "ddgenrul %s %s -u%s",lpDBName,lpTblName, bufUser);
   //"Generating Rules and Procedures for table %s"
   wsprintf(buf2,ResourceString(IDS_TITLE_GENERATE_RULES_PROCEDURES),lpTblName);

   execrmcmd1(lpNodeName, buf, buf2, FALSE);

   ActivateSession(callsession);
   return TRUE;
}
static BOOL DBACreateReplicRulesV11(LPUCHAR lpNodeName, LPUCHAR lpDBName,
                                    LPUCHAR lpTblName , int TblNumber   ,
                                    int callsession)
{
   char buf[200];
   char buf2[200];
   char bufUser[100];
	char szgateway[200];
	BOOL bHasGWSuffix = GetGWClassNameFromString(lpNodeName, szgateway);

   if (!DBAGetUserName(lpNodeName,bufUser)) {
      ActivateSession(callsession);
      return FALSE;
   }
   if (bHasGWSuffix) 
		wsprintf(buf, "ddgenrul %s/%s %d -u%s",lpDBName,szgateway,TblNumber, bufUser);
   else
		wsprintf(buf, "ddgenrul %s %d -u%s",lpDBName,TblNumber, bufUser);
   //"Activating change recording for table %s"
   wsprintf(buf2,ResourceString(IDS_TITLE_ACTIVATING_CH_REC),lpTblName);

   execrmcmd1(lpNodeName, buf, buf2, FALSE);

   ActivateSession(callsession);
   return TRUE;
}

static BOOL DBACreateReplicSup(LPUCHAR lpNodeName, LPUCHAR lpDBName,LPUCHAR lpTblName, int callsession)
{
   char buf[200];
   char buf2[200];
   char bufUser[100];
   if (!DBAGetUserName(lpNodeName,bufUser)) {
      ActivateSession(callsession);
      return FALSE;
   }

   wsprintf(buf, "ddgensup %s %s -u%s",lpDBName,lpTblName, bufUser);
   //"Generating Support Tables for table %s"
   wsprintf(buf2,ResourceString(IDS_TITLE_GENERATING_SUPPORT_TABLE),lpTblName);

   execrmcmd1(lpNodeName, buf, buf2, FALSE);

   ActivateSession(callsession);
   return TRUE;
}
static BOOL DBACreateReplicSupV11(LPUCHAR lpNodeName, LPUCHAR lpDBName,
                                  LPUCHAR lpTblName , int TblNumber   ,
                                  int callsession  )
{
   char buf[200];
   char buf2[200];
   char bufUser[100];
	char szgateway[200];
	BOOL bHasGWSuffix = GetGWClassNameFromString(lpNodeName, szgateway);

   if (!DBAGetUserName(lpNodeName,bufUser)) {
      ActivateSession(callsession);
      return FALSE;
   }
   if (bHasGWSuffix) 
		wsprintf(buf, "ddgensup %s/%s %d -u%s",lpDBName, szgateway, TblNumber, bufUser);
   else
		wsprintf(buf, "ddgensup %s %d -u%s",lpDBName,TblNumber, bufUser);
   //"Generating Support Tables for table %s"
   wsprintf(buf2,ResourceString(IDS_TITLE_GENERATING_SUPPORT_TABLE),lpTblName);

   execrmcmd1(lpNodeName, buf, buf2, FALSE);

   ActivateSession(callsession);
   return TRUE;
}

static BOOL DBADropReplicRules(LPUCHAR lpTableName)

{
   char bufrequest[100];
   int iret,i;
   BOOL bResult = TRUE;
   char * strings[]={
                       "drop procedure %s__i2",
                       "drop procedure %s__u2",
                       "drop procedure %s__d2",
                       "drop procedure %s__i" ,
                       "drop rule  %s__1"     ,
                       "drop procedure %s__u" ,
                       "drop procedure %s__d" ,
                       "drop rule %s__2"      ,
                       "drop rule %s__3" 
                     };
   for (i=0;i<9;i++) {
      wsprintf(bufrequest,strings[i],lpTableName);
      iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
      if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
         bResult=FALSE;
   }
   return bResult;
}
static BOOL DBADropReplicRulesV11(LPUCHAR lpTableName)

{
   char bufrequest[100];
   int iret,i;
   BOOL bResult = TRUE;
   char * strings[]={
                       "drop rule %sdel", 
                       "drop rule %sins", 
                       "drop rule %supd" 
                     };
   for (i=0;i<3;i++) {
      wsprintf(bufrequest,strings[i],lpTableName);
      iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
      if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
         bResult=FALSE;
   }
   return bResult;
}


static DBADropReplicSup(LPUCHAR lpTableName)
{
   char bufrequest[100];
   int iret,i;
   char * strings[]={"drop table %s_s"  ,
                      "drop table %s_a"
                     } ;
   for (i=0;i<2;i++) {
      wsprintf(bufrequest,strings[i],lpTableName);
      iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
      if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
         return FALSE;
   }
   return TRUE;
}

static DBADropReplicSupV11(LPUCHAR lpTableName)
{
   char bufrequest[100];
   int iret,i;

   char * strings[]={"drop table %ssha",
                     "drop table %sarc",
                     "drop procedure %srmi",
                     "drop procedure %srmu",
                     "drop procedure %srmd",
                     "drop procedure %slou",
                     "drop procedure %slod",
                     "drop procedure %sloi",
                     "delete from dd_support_tables where table_name = '%ssha'",
                     "delete from dd_support_tables where table_name = '%sarc'"
                    } ;
   for (i=0;i<10;i++) {
      wsprintf(bufrequest,strings[i],lpTableName);
      iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
      if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
         return FALSE;
   }
   return TRUE;
}

static DBADropReplicSupV11Oi20(LPUCHAR lpTableName)
{
   char bufrequest[100];
   int iret,i,Maxlen;

   char * strings[]={"drop table %ssha",
                     "drop table %sarc",
                     "drop procedure %srmi",
                     "drop procedure %srmu",
                     "drop procedure %srmd",
                     //"drop procedure %slou",
                     //"drop procedure %slod",
                     //"drop procedure %sloi",
                     "delete from dd_support_tables where table_name = '%ssha'",
                     "delete from dd_support_tables where table_name = '%sarc'"
                    } ;
   Maxlen = sizeof (strings) / sizeof (strings[0]);
   for (i=0;i< Maxlen ;i++) {
      wsprintf(bufrequest,strings[i],lpTableName);
      iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
      if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
         return FALSE;
   }
   return TRUE;
}

UCHAR *ConcatStr(Lp1, Lp2, freebytes)
UCHAR *Lp1, *Lp2;
int *freebytes;
{ 
   UCHAR *pstr=Lp1;
   int bytesneeded;
   if (!Lp1)
      return Lp1;

   if ((*freebytes-=x_strlen(Lp2))<=0) {   // update freebytes. Room to copy?
      bytesneeded = (x_strlen(Lp2)>SEGMENT_TEXT_SIZE) ?
                     x_strlen(Lp2) : SEGMENT_TEXT_SIZE;
      pstr=ESL_ReAllocMem(Lp1, 
         x_strlen(Lp1)+bytesneeded+1, x_strlen(Lp1));
      if (!pstr) {
         ESL_FreeMem((void *) Lp1);
         return NULL;
      }
      if (SEGMENT_TEXT_SIZE < x_strlen(Lp2) )
         *freebytes=0;
      else
         *freebytes=SEGMENT_TEXT_SIZE-x_strlen(Lp2);
   }
   x_strcat(pstr, Lp2);
   return pstr;   
}

LPUCHAR GetColTypeStr(LPCOLUMNPARAMS lpColAttr)
{
   int freebytes=SEGMENT_TEXT_SIZE;
   LPUCHAR lpStr;
   UCHAR ustr[MAXOBJECTNAME];
   struct coltype {
      UINT uDataType;
      UCHAR *cDataType;
      BOOL bLnSignificant;
   } ColTypes[] = {
         {INGTYPE_C           ," c("             ,TRUE},
         {INGTYPE_CHAR        ," char("          ,TRUE},
         {INGTYPE_TEXT        ," text("          ,TRUE},
         {INGTYPE_VARCHAR     ," varchar("       ,TRUE},
         {INGTYPE_LONGVARCHAR ," long varchar "  ,FALSE},
         {INGTYPE_BIGINT      ," bigint "        ,FALSE},
         {INGTYPE_INT8        ," bigint "        ,FALSE},
         {INGTYPE_INT4        ," int "           ,FALSE},
         {INGTYPE_INT2        ," smallint"       ,FALSE},
         {INGTYPE_INT1        ," integer1"       ,FALSE},
         {INGTYPE_DECIMAL     ," decimal("       ,TRUE},
         {INGTYPE_FLOAT8      ," float"          ,FALSE},
         {INGTYPE_FLOAT4      ," real"           ,FALSE},
         {INGTYPE_DATE        ," date"           ,FALSE},
         {INGTYPE_ADTE        ," ansidate"                       ,FALSE},
         {INGTYPE_TMWO        ," time without time zone"         ,FALSE},
         {INGTYPE_TMW         ," time with time zone"            ,FALSE},
         {INGTYPE_TME         ," time with local time zone"      ,FALSE},
         {INGTYPE_TSWO        ," timestamp without time zone"    ,FALSE},
         {INGTYPE_TSW         ," timestamp with time zone"       ,FALSE},
         {INGTYPE_TSTMP       ," timestamp with local time zone" ,FALSE},
         {INGTYPE_INYM        ," interval year to month"         ,FALSE},
         {INGTYPE_INDS        ," interval day to second"         ,FALSE},
         {INGTYPE_IDTE        ," ingresdate"                     ,FALSE},
         {INGTYPE_MONEY       ," money"          ,FALSE},
         {INGTYPE_BYTE        ," byte("          ,TRUE},
         {INGTYPE_BYTEVAR     ," byte varying("  ,TRUE},
         {INGTYPE_LONGBYTE    ," long byte"      ,FALSE},
         {INGTYPE_OBJKEY      ," object_key"     ,FALSE},
         {INGTYPE_TABLEKEY    ," table_key"      ,FALSE},
         {INGTYPE_FLOAT       ," float"          ,FALSE},
         {INGTYPE_UNICODE_NCHR," nchar("         ,TRUE},
         {INGTYPE_UNICODE_NVCHR," nvarchar("     ,TRUE},
         {INGTYPE_UNICODE_LNVCHR," long nvarchar",FALSE},
         {INGTYPE_ERROR       ,""                ,FALSE}};

   struct coltype *ColType;

   lpStr=ESL_AllocMem(SEGMENT_TEXT_SIZE+1);

   if (!lpStr) {
      myerror(ERR_ALLOCMEM);
      return NULL;
   }

   /**** get the column type ****/

   ColType=&ColTypes[0];

   while (ColType->uDataType!=INGTYPE_ERROR) {
      if (ColType->uDataType==lpColAttr->uDataType)
         break;
      ColType++;
   }
   
   if (ColType->uDataType==INGTYPE_ERROR) {
      ESL_FreeMem(lpStr);
      return NULL;
   }

   lpStr=ConcatStr(lpStr, ColType->cDataType, &freebytes); 

   if (!lpStr) {
      myerror(ERR_ALLOCMEM);
      return NULL;
   }

   /**** Get the column length if applicable ****/

   if (ColType->bLnSignificant) {
      if (ColType->uDataType==INGTYPE_DECIMAL) {
         my_itoa(lpColAttr->lenInfo.decInfo.nPrecision, ustr, 10);
         lpStr=ConcatStr(lpStr, ustr, &freebytes);
         lpStr=ConcatStr(lpStr, ",", &freebytes);    // added emb 26/11/96
         my_itoa(lpColAttr->lenInfo.decInfo.nScale, ustr, 10);
         lpStr=ConcatStr(lpStr, ustr, &freebytes);
      }
      else {
         my_itoa((int) lpColAttr->lenInfo.dwDataLen, ustr, 10);
         lpStr=ConcatStr(lpStr, ustr, &freebytes);
      }
      lpStr=ConcatStr(lpStr, STR_RPAREN, &freebytes);  /* , */
      if (!lpStr) {
         myerror(ERR_ALLOCMEM);
         return NULL;
      }
   }
   return lpStr;
}

static UCHAR *GenCObjList(pstm, pchkobj, pfreebytes)
UCHAR *pstm;
LPCHECKEDOBJECTS pchkobj;
int *pfreebytes;
{
   BOOL one_found=FALSE;

   if (!pstm)
      return pstm;
   if (!pchkobj)
      return NULL;

   while(pchkobj) {
      if (pchkobj->bchecked) {
         if (!one_found)                  // first one : no comma.
            one_found=TRUE;
         else 
            pstm=ConcatStr(pstm, STR_COMMA, pfreebytes);
         pstm=ConcatStr(pstm,pchkobj->dbname,pfreebytes);
         if (!pstm)
            return NULL;  
      }
      pchkobj = pchkobj->pnext;
   }
   return pstm;
}

static UCHAR *GenSortedObjList(pstm, pidxcols, pfreebytes, ALLColumns, bHeapsort)
UCHAR *pstm;
LPOBJECTLIST pidxcols;
int *pfreebytes;
BOOL ALLColumns;
BOOL bHeapsort;
{
   int iNbKey,i;
   BOOL one_found=FALSE;
   BOOL bIndexFound=FALSE;

   LPOBJECTLIST pStoreidxcols = pidxcols;

   if (!pstm)
      return pstm;
   if (!pidxcols)
      return NULL;

   iNbKey = 0;
   while(pidxcols) { // determine the number of key column in the column list.
      if (((LPIDXCOLS)(pidxcols->lpObject))->attr.bKey)
         iNbKey++;
      pidxcols = pidxcols->lpNext;
   }

   for (i = 1 ; i <= iNbKey; i++, bIndexFound=FALSE)
   {
      pidxcols = pStoreidxcols;
      while(pidxcols) {
         if (ALLColumns || ((LPIDXCOLS)(pidxcols->lpObject))->attr.bKey &&
             ((LPIDXCOLS)(pidxcols->lpObject))->attr.nPrimary == i) {
            bIndexFound = TRUE;
            if (!one_found)                  // first one : no comma.
               one_found=TRUE;
            else 
               pstm=ConcatStr(pstm, STR_COMMA, pfreebytes);
            pstm=ConcatStr(pstm,(LPUCHAR)QuoteIfNeeded(((LPIDXCOLS)(pidxcols->lpObject))->szColName),
               pfreebytes);
            if (((LPIDXCOLS)(pidxcols->lpObject))->attr.bDescending && bHeapsort)
               pstm=ConcatStr(pstm, " desc ", pfreebytes);
            if (!pstm)
               return NULL;
            break;
         }
         pidxcols = pidxcols->lpNext;
      }
      if (!pidxcols && !bIndexFound)
         return NULL;
   }

   if (!one_found) 
      return NULL;
   return pstm;
}
static UCHAR *GenObjList(pstm, pidxcols, pfreebytes, ALLColumns, bHeapsort)
UCHAR *pstm;
LPOBJECTLIST pidxcols;
int *pfreebytes;
BOOL ALLColumns;
BOOL bHeapsort;
{
   BOOL one_found=FALSE;

   if (!pstm)
      return pstm;
   if (!pidxcols)
      return NULL;

   while(pidxcols) {
      if (ALLColumns||((LPIDXCOLS)(pidxcols->lpObject))->attr.bKey) {
         if (!one_found)                  // first one : no comma.
            one_found=TRUE;
         else 
            pstm=ConcatStr(pstm, STR_COMMA, pfreebytes);
         pstm=ConcatStr(pstm,(LPUCHAR)QuoteIfNeeded(((LPIDXCOLS)(pidxcols->lpObject))->szColName),
            pfreebytes);
         if (((LPIDXCOLS)(pidxcols->lpObject))->attr.bDescending && bHeapsort)
            pstm=ConcatStr(pstm, " desc ", pfreebytes);
         if (!pstm)
            return NULL;  
      }
      pidxcols = pidxcols->lpNext;
   }
   if (!one_found) 
      return NULL;
   return pstm;
}
static UCHAR *GenGrantObjList(pstm, pobjlist, pfreebytes)
UCHAR *pstm;
LPOBJECTLIST pobjlist;
int *pfreebytes;
{
   BOOL one_found=FALSE;

   if (!pstm)
      return pstm;
   if (!pobjlist)
      return NULL;

   while(pobjlist) {
      if (!one_found)                  // first one : no comma.
         one_found=TRUE;
      else 
         pstm=ConcatStr(pstm, STR_COMMA, pfreebytes);

      pstm=ConcatStr(pstm,pobjlist->lpObject,pfreebytes);
      if (!pstm)
         return NULL;
      pobjlist = pobjlist->lpNext;
   }
   return pstm;
}



int BuildSQLCreRole(UCHAR **PPstm ,LPROLEPARAMS roleparm)
{
   UCHAR *pstm;
   BOOL bAllDefPriv, one_priv, bNoDefPriv;
   static char STR_CREROLE[]="create role ";
   static struct _SecAud {
       UCHAR *Str;
   } SecAud[USERSECAUDITS];

   static struct _Prv {
       UCHAR *Str;
   } Priv[USERPRIVILEGES];
   unsigned int i, freebytes=SEGMENT_TEXT_SIZE;

   SecAud[USERALLEVENT      ].Str=" all_events";
   SecAud[USERDEFAULTEVENT  ].Str=" default_events";
   SecAud[USERQUERYTEXTEVENT].Str=" query_text";

   Priv[USERCREATEDBPRIV].Str=" createdb";
   Priv[USERTRACEPRIV   ].Str=" trace";
   Priv[USERSECURITYPRIV].Str=" security";
   Priv[USEROPERATORPRIV].Str=" operator";
   Priv[USERMAINTLOCPRIV].Str=" maintain_locations";
   Priv[USERAUDITALLPRIV].Str=" auditor";
   Priv[USERMAINTAUDITPRIV].Str=" maintain_audit";
   Priv[USERMAINTUSERPRIV ].Str=" maintain_users";

   Priv[USERWRITEDOWN ].Str= " write_down";
   Priv[USERWRITEFIXED].Str= " write_fixed";
   Priv[USERWRITEUP].Str= " write_up";
   Priv[USERINSERTDOWN].Str= " insert_down";
   Priv[USERINSERTUP].Str= " insert_up";
   Priv[USERSESSIONSECURITYLABEL].Str= " session_security_label";


   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      *PPstm=NULL;
      return RES_ERR;
   }

// --- Begin of fixed part generation : The return of ConcatStr is checked   
// --- only once since ConcatStr does nothing if it first param is NULL

   pstm=ConcatStr(pstm, STR_CREROLE, &freebytes);
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(roleparm->ObjectName), &freebytes);
   pstm=ConcatStr(pstm, STR_WITH, &freebytes);

   if (roleparm->bExtrnlPassword)
      pstm=ConcatStr(pstm, STR_EXTRNLPASSWORD, &freebytes);
   else {
      if (*roleparm->szPassword) {
         pstm=ConcatStr(pstm, STR_PASSWORD, &freebytes);
         pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
         pstm=ConcatStr(pstm, roleparm->szPassword, &freebytes);
         pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
      }
      else
         pstm=ConcatStr(pstm, STR_NOPASSWORD, &freebytes);
   }
   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

   // now concatenates the privileges if any

   one_priv=FALSE;

   for (i=0; i<USERPRIVILEGES; i++) {
      if (roleparm->Privilege[i]) {      // one privilege checked.
         if (one_priv)                  // not first one.
            pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
         else { 
            one_priv=TRUE;
            pstm=ConcatStr(pstm, STR_PRIV, &freebytes);   //with privilege=   
         }
         pstm=ConcatStr(pstm,Priv[i].Str, &freebytes); // copy privilege 
         if (!pstm) {
            *PPstm=NULL;
            return RES_ERR;
         }
      }
   }

   if (one_priv)        // at least on priv were found
      pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);
   else
      pstm=ConcatStr(pstm,STR_NOPRIV, &freebytes);
   
   if (!pstm) {
      *PPstm=NULL;
      return RES_ERR;
   }

   // now generate the security audit if appropriate

   one_priv=FALSE;

   for (i=0; i<USERSECAUDITS; i++) {
      if (roleparm->bSecAudit[i]) {
         if (one_priv)            
            pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
         else { 
            one_priv=TRUE;
            pstm=ConcatStr(pstm, STR_SECAUDIT, &freebytes);
         }
         pstm=ConcatStr(pstm,SecAud[i].Str, &freebytes); 
         if (!pstm) {
            myerror(ERR_ALLOCMEM);
            return RES_ERR;
         }
      }
   }

   if (one_priv)        // at least on priv were found
      pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);
   
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

   // Limiting security label

   if (roleparm->lpLimitSecLabels) {
      if (*(roleparm->lpLimitSecLabels)) {
         pstm=ConcatStr(pstm, STR_SECLABEL, &freebytes);
         pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
         pstm=ConcatStr(pstm, roleparm->lpLimitSecLabels, &freebytes);
         pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
         pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
       }
   }
   // else {  // Requires B1 secure
   //    pstm=ConcatStr(pstm, STR_NOSECLABEL, &freebytes);
   //    pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
   // }


   // Generates the default privileges clause

   bAllDefPriv=TRUE;
   bNoDefPriv=TRUE;

   for (i=0; i<USERPRIVILEGES; i++) { // determines if ALL, NO or some
      if (roleparm->bDefaultPrivilege[i]) 
         bNoDefPriv=FALSE;
      else
         bAllDefPriv=FALSE;
   }

   one_priv=FALSE;
   if (bNoDefPriv)
      pstm=ConcatStr(pstm,STR_NODEFAULTPRIV, &freebytes);
   else if (bAllDefPriv)
      pstm=ConcatStr(pstm,STR_DEFAULTALLPRIV, &freebytes);
   else {
      for (i=0; i<USERPRIVILEGES; i++) {
         if (roleparm->bDefaultPrivilege[i]) {     // one privilege checked.
            if (one_priv)                         // not first one.
               pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
            else { 
               one_priv=TRUE;
               pstm=ConcatStr(pstm, STR_DEFAULTPRIV, &freebytes); 
            }
            pstm=ConcatStr(pstm,Priv[i].Str, &freebytes); // copy privilege 
            if (!pstm) {
               myerror(ERR_ALLOCMEM);
               return RES_ERR;
            }
         }
      }
   }

   if (one_priv)        // at least on priv were found
      pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);
   

   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   *PPstm=pstm;         // return the pointer to the generated SQL stmt.
   return RES_SUCCESS;

}

int BuildSQLCreUsr(UCHAR ** PPstm, LPUSERPARAMS usrparm)
{
   UCHAR *pstm; 
   BOOL one_priv=FALSE, bAllDefPriv, bNoDefPriv;
   static char STR_CREUSR[]="create user ";

   static struct _SecAud {
       UCHAR *Str;
   } SecAud[USERSECAUDITS];

   static struct _Prv {
       UCHAR *Str;
   } Priv[USERPRIVILEGES];
   unsigned int i, freebytes=SEGMENT_TEXT_SIZE;

   SecAud[USERALLEVENT      ].Str=" all_events";
   SecAud[USERDEFAULTEVENT  ].Str=" default_events";
   SecAud[USERQUERYTEXTEVENT].Str=" query_text";

   Priv[USERCREATEDBPRIV].Str=" createdb";
   Priv[USERTRACEPRIV   ].Str=" trace";
   Priv[USERSECURITYPRIV].Str=" security";
   Priv[USEROPERATORPRIV].Str=" operator";
   Priv[USERMAINTLOCPRIV].Str=" maintain_locations";
   Priv[USERAUDITALLPRIV].Str=" auditor";
   Priv[USERMAINTAUDITPRIV].Str=" maintain_audit";
   Priv[USERMAINTUSERPRIV ].Str=" maintain_users";

   Priv[USERWRITEDOWN ].Str= " write_down";
   Priv[USERWRITEFIXED].Str= " write_fixed";
   Priv[USERWRITEUP].Str= " write_up";
   Priv[USERINSERTDOWN].Str= " insert_down";
   Priv[USERINSERTUP].Str= " insert_up";
   Priv[USERSESSIONSECURITYLABEL].Str= " session_security_label";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      *PPstm=NULL;
      return RES_ERR;
   }

// --- Begin of fixed part generation : The return of ConcatStr is checked   
// --- only once since ConcatStr does nothing if it first param is NULL

   pstm=ConcatStr(pstm, STR_CREUSR, &freebytes);
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(usrparm->ObjectName), &freebytes);

   if (*(usrparm->DefaultGroup)) {
      pstm=ConcatStr(pstm, STR_WITHGRP, &freebytes);
      pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(usrparm->DefaultGroup), &freebytes); // group_name
      pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
   }
   else  {
     if (usrparm->szProfileName[0] == '\0') {               // if no profile
       pstm=ConcatStr(pstm, STR_NOGRP, &freebytes);         // nogroup
       pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
       }
     else  {
       pstm=ConcatStr(pstm, STR_WITH, &freebytes);          // only with 
       }
     }


   // expire date

   if (usrparm->ExpireDate[0]) {
      pstm=ConcatStr(pstm, STR_EXPIREDATE, &freebytes);
      pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
      pstm=ConcatStr(pstm, usrparm->ExpireDate, &freebytes);
      pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
   }
   else
      pstm=ConcatStr(pstm, STR_NOEXPIREDATE, &freebytes);

   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

   // Profile Name.

   if (usrparm->szProfileName[0]) {
      pstm=ConcatStr(pstm, STR_PROFILE, &freebytes);
      pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(usrparm->szProfileName), &freebytes);
   }
   else
      pstm=ConcatStr(pstm, STR_NOPROFILE, &freebytes);

   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

   // Password

   if (usrparm->szPassword[0]) {
      pstm=ConcatStr(pstm, STR_PASSWORD, &freebytes);
      pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
      pstm=ConcatStr(pstm, usrparm->szPassword, &freebytes);
      pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
   }
   else {
      if (usrparm->bExtrnlPassword) { // External Password ?
         pstm=ConcatStr(pstm, STR_EXTRNLPASSWORD, &freebytes);
         //pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
      }
      else
         pstm=ConcatStr(pstm, STR_NOPASSWORD, &freebytes);
   }

   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

   // Limiting security label

   if (usrparm->lpLimitSecLabels) {
      if (*(usrparm->lpLimitSecLabels)) {
         pstm=ConcatStr(pstm, STR_SECLABEL, &freebytes);
         pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
         pstm=ConcatStr(pstm, usrparm->lpLimitSecLabels, &freebytes);
         pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
         pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
       }
   }
   // else {  // Requires B1 secure
   //    pstm=ConcatStr(pstm, STR_NOSECLABEL, &freebytes);
   //    pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
   // }

   if (!pstm) {
      *PPstm=NULL;
      return RES_ERR;
   }
   

// --- fixed part is done . Start with the varying part.

   // now concatenates the privileges if any

   one_priv=FALSE;

   for (i=0; i<USERPRIVILEGES; i++) {
      if (usrparm->Privilege[i]) {      // one privilege checked.
         if (one_priv)                  // not first one.
            pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
         else { 
            one_priv=TRUE;
            pstm=ConcatStr(pstm, STR_PRIV, &freebytes);   //with privilege=   
         }
         pstm=ConcatStr(pstm,Priv[i].Str, &freebytes); // copy privilege 
         if (!pstm) {
            *PPstm=NULL;
            return RES_ERR;
         }
      }
   }

   if (one_priv)        // at least on priv were found
      pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);
   else
      pstm=ConcatStr(pstm,STR_NOPRIV, &freebytes);
   
   if (!pstm) {
      *PPstm=NULL;
      return RES_ERR;
   }

   // Genrerates the default privileges clause

   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

   bAllDefPriv=TRUE;
   bNoDefPriv=TRUE;

   for (i=0; i<USERPRIVILEGES; i++) { // determines if ALL, NO or some
      if (usrparm->bDefaultPrivilege[i]) 
         bNoDefPriv=FALSE;
      else
         bAllDefPriv=FALSE;
   }

   one_priv=FALSE;
   if (bNoDefPriv)
      pstm=ConcatStr(pstm,STR_NODEFAULTPRIV, &freebytes);
   else if (bAllDefPriv)
      pstm=ConcatStr(pstm,STR_DEFAULTALLPRIV, &freebytes);
   else {
      for (i=0; i<USERPRIVILEGES; i++) {
         if (usrparm->bDefaultPrivilege[i]) {     // one privilege checked.
            if (one_priv)                         // not first one.
               pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
            else { 
               one_priv=TRUE;
               pstm=ConcatStr(pstm, STR_DEFAULTPRIV, &freebytes); 
            }
            pstm=ConcatStr(pstm,Priv[i].Str, &freebytes); // copy privilege 
            if (!pstm) {
               myerror(ERR_ALLOCMEM);
               return RES_ERR;
            }
         }
      }
   }

   if (one_priv)        // at least on priv were found
      pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);
   
   // now generate the security audit if appropriate

   one_priv=FALSE;

   for (i=0; i<USERSECAUDITS; i++) {
      if (usrparm->bSecAudit[i]) {
         if (one_priv)            
            pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
         else { 
            one_priv=TRUE;
            pstm=ConcatStr(pstm, STR_SECAUDIT, &freebytes);
         }
         pstm=ConcatStr(pstm,SecAud[i].Str, &freebytes); 
         if (!pstm) {
            myerror(ERR_ALLOCMEM);
            return RES_ERR;
         }
      }
   }

   if (one_priv)        // at least on priv were found
      pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);
   
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   *PPstm=pstm;         // return the pointer to the generated SQL stmt.
   return RES_SUCCESS;

}


extern int BuildSQLCreProf(UCHAR ** PPstm, LPPROFILEPARAMS profparm)
{
   UCHAR *pstm; 
   BOOL one_priv=FALSE, bAllDefPriv, bNoDefPriv;
   static char STR_CREPROF[]="create profile ";

   static struct _SecAud {
       UCHAR *Str;
   } SecAud[USERSECAUDITS];

   static struct _Prv {
       UCHAR *Str;
   } Priv[USERPRIVILEGES];
   unsigned int i, freebytes=SEGMENT_TEXT_SIZE;

   SecAud[USERALLEVENT      ].Str=" all_events";
   SecAud[USERDEFAULTEVENT  ].Str=" default_events";
   SecAud[USERQUERYTEXTEVENT].Str=" query_text";

   Priv[USERCREATEDBPRIV].Str=" createdb";
   Priv[USERTRACEPRIV   ].Str=" trace";
   Priv[USERSECURITYPRIV].Str=" security";
   Priv[USEROPERATORPRIV].Str=" operator";
   Priv[USERMAINTLOCPRIV].Str=" maintain_locations";
   Priv[USERAUDITALLPRIV].Str=" auditor";
   Priv[USERMAINTAUDITPRIV].Str=" maintain_audit";
   Priv[USERMAINTUSERPRIV ].Str=" maintain_users";

   Priv[USERWRITEDOWN ].Str= " write_down";
   Priv[USERWRITEFIXED].Str= " write_fixed";
   Priv[USERWRITEUP].Str= " write_up";
   Priv[USERINSERTDOWN].Str= " insert_down";
   Priv[USERINSERTUP].Str= " insert_up";
   Priv[USERSESSIONSECURITYLABEL].Str= " session_security_label";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      *PPstm=NULL;
      return RES_ERR;
   }

// --- Begin of fixed part generation : The return of ConcatStr is checked   
// --- only once since ConcatStr does nothing if its first param is NULL

   pstm=ConcatStr(pstm, STR_CREPROF, &freebytes);
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(profparm->ObjectName), &freebytes);

   if (*(profparm->DefaultGroup)) {
      pstm=ConcatStr(pstm, STR_WITHGRP, &freebytes);
      pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(profparm->DefaultGroup), &freebytes); // group_name
   }
   else 
      pstm=ConcatStr(pstm, STR_NOGRP, &freebytes);         // nogroup

   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

   // expire date

   if (profparm->ExpireDate[0]) {
      pstm=ConcatStr(pstm, STR_EXPIREDATE, &freebytes);
      pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
      pstm=ConcatStr(pstm, profparm->ExpireDate, &freebytes);
      pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
   }
   else
      pstm=ConcatStr(pstm, STR_NOEXPIREDATE, &freebytes);

   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

   // Limiting security label

   if (profparm->lpLimitSecLabels) {
      if (*(profparm->lpLimitSecLabels)) {
         pstm=ConcatStr(pstm, STR_SECLABEL, &freebytes);
         pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
         pstm=ConcatStr(pstm, profparm->lpLimitSecLabels, &freebytes);
         pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
         pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
       }
   }
   // else {  // Requires B1 secure
   //    pstm=ConcatStr(pstm, STR_NOSECLABEL, &freebytes);
   //    pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
   // }

   if (!pstm) {
      *PPstm=NULL;
      return RES_ERR;
   }
   

// --- fixed part is done . Start with the variing part.

   // now concatenates the privileges if any

   one_priv=FALSE;

   for (i=0; i<USERPRIVILEGES; i++) {
      if (profparm->Privilege[i]) {      // one privilege checked.
         if (one_priv)                   // not first one.
            pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
         else { 
            one_priv=TRUE;
            pstm=ConcatStr(pstm, STR_PRIV, &freebytes);   //with privilege=   
         }
         pstm=ConcatStr(pstm,Priv[i].Str, &freebytes); // copy privilege 
         if (!pstm) {
            *PPstm=NULL;
            return RES_ERR;
         }
      }
   }

   if (one_priv)        // at least on priv were found
      pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);
   else
      pstm=ConcatStr(pstm,STR_NOPRIV, &freebytes);
   
   if (!pstm) {
      *PPstm=NULL;
      return RES_ERR;
   }

   // Genrerates the default privileges clause

   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

   bAllDefPriv=TRUE;
   bNoDefPriv=TRUE;

   for (i=0; i<USERPRIVILEGES; i++) { // determines if ALL, NO or some
      if (profparm->bDefaultPrivilege[i]) 
         bNoDefPriv=FALSE;
      else
         bAllDefPriv=FALSE;
   }

   one_priv=FALSE;
   if (bNoDefPriv)
      pstm=ConcatStr(pstm,STR_NODEFAULTPRIV, &freebytes);
   else if (bAllDefPriv)
      pstm=ConcatStr(pstm,STR_DEFAULTALLPRIV, &freebytes);
   else {
      for (i=0; i<USERPRIVILEGES; i++) {
         if (profparm->bDefaultPrivilege[i]) {     // one privilege checked.
            if (one_priv)                         // not first one.
               pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
            else { 
               one_priv=TRUE;
               pstm=ConcatStr(pstm, STR_DEFAULTPRIV, &freebytes); 
            }
            pstm=ConcatStr(pstm,Priv[i].Str, &freebytes); // copy privilege 
            if (!pstm) {
               myerror(ERR_ALLOCMEM);
               return RES_ERR;
            }
         }
      }
   }

   if (one_priv)        // at least on priv were found
      pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);
   
   // now generate the security audit if appropriate

   one_priv=FALSE;

   for (i=0; i<USERSECAUDITS; i++) {
      if (profparm->bSecAudit[i]) {
         if (one_priv)            
            pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
         else { 
            one_priv=TRUE;
            pstm=ConcatStr(pstm, STR_SECAUDIT, &freebytes);
         }
         pstm=ConcatStr(pstm,SecAud[i].Str, &freebytes); 
         if (!pstm) {
            myerror(ERR_ALLOCMEM);
            return RES_ERR;
         }
      }
   }


   if (one_priv)        // at least on priv were found
      pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);
   
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   *PPstm=pstm;         // return the pointer to the generated SQL stmt.
   return RES_SUCCESS;

}

extern int BuildSQLCreGrp(UCHAR **PPstm, LPGROUPPARAMS grpparm)
{
   UCHAR *pstm, *Newpstm; 
   unsigned freebytes=SEGMENT_TEXT_SIZE;
   static char STR_CREGRP[]="create group ";
   static char STR_WITHUSER[]=" with users = (";
   BOOL one_grp=FALSE;

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      *PPstm=pstm;
      return RES_ERR;
   }

   // generates the statement begining (fixed part) 

   fstrncpy(pstm, STR_CREGRP, x_strlen(STR_CREGRP)+1);  // create group
   freebytes-=x_strlen(STR_CREGRP);

   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(grpparm->ObjectName), &freebytes); // group_name
   if (!pstm) {
      *PPstm=pstm;
      return RES_ERR;
   }

   if (!grpparm->lpfirstuser) {  // no users for the group, we are done
     *PPstm=pstm;         // return the pointer to the generated SQL stmt.
     return RES_SUCCESS;
   }      

   // generates the statement end (variable part : n users) 

   pstm=ConcatStr(pstm, STR_WITHUSER, &freebytes);    // with users=(

   Newpstm=GenCObjList(pstm, grpparm->lpfirstuser, &freebytes);
   if (!Newpstm) {
      ESL_FreeMem((void *) pstm);
      *PPstm=NULL;
      return RES_ERR;
   }
   pstm=Newpstm;

   pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);
   if (!pstm) {
      *PPstm=NULL;
      return RES_ERR;
   }
   *PPstm=pstm;         // return the pointer to the generated SQL stmt.
   return RES_SUCCESS;
}

int BuildSQLSecAlrm(UCHAR ** PPstm, LPSECURITYALARMPARAMS secparm)
{
   static char STR_CREBY[]=" by ";
   static char STR_CREIF[]=" if ";
   static char STR_CREWHEN[]=" when ";
   static char STR_TABLE[]=" table ";
   static char STR_DATABASE[]=" database ";
   static char STR_CURINST[]=" current installation ";
   static char STR_CRESEC[]="create security_alarm ";
   static char STR_RAISE[]=" raise dbevent ";
   UCHAR *SuccFail[SECALARMSSUCFAIL], *AccessType[SECALARMSTYPES];
   UCHAR *pstm, *Newpstm; 
   int freebytes=SEGMENT_TEXT_SIZE, i, iMAX;
   BOOL one_found;

   SuccFail[SECALARMSUCCESS]="success";
   SuccFail[SECALARMFAILURE]="failure";
   AccessType[SECALARMSEL]="select";
   AccessType[SECALARMDEL]="delete";
   AccessType[SECALARMINS]="insert";
   AccessType[SECALARMUPD]="update";
   AccessType[SECALARMCONNECT]="connect";
   AccessType[SECALARMDISCONN]="disconnect";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      *PPstm=pstm;
      return RES_ERR;
   }

   fstrncpy(pstm, STR_CRESEC, x_strlen(STR_CRESEC)+1);     // create security
   freebytes -= x_strlen(STR_CRESEC);

   if (*secparm->ObjectName)
      pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(secparm->ObjectName), &freebytes);

   pstm=ConcatStr(pstm, STR_ON, &freebytes);

   // Generate the object type for which the security event are to be logged

   switch (secparm->iObjectType) {
      case OT_TABLE :
         pstm=ConcatStr(pstm, STR_TABLE, &freebytes);
         break;
      case OT_DATABASE:
         pstm=ConcatStr(pstm, STR_DATABASE, &freebytes);
         break;
      case OT_VIRTNODE:
         pstm=ConcatStr(pstm, STR_CURINST, &freebytes);
         break;
   }

   // generates the variable part (n tables or bases) 

   if (secparm->iObjectType!=OT_VIRTNODE) {
      Newpstm=GenCObjList(pstm, secparm->lpfirstTable, &freebytes);
      if (!Newpstm) {
         ESL_FreeMem((void *) pstm);
         *PPstm=NULL;
         return RES_ERR;
      }   
      pstm=Newpstm;
   }

   // generates the success/failure variable part

   iMAX=sizeof(SuccFail)/sizeof(SuccFail[0]); 
   one_found=FALSE;
   for (i=0; i<iMAX; i++) {
      if (secparm->bsuccfail[i]) {      // one privilege checked.
         if (one_found)                  // not first one.
            pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
         else { 
            one_found=TRUE;
            pstm=ConcatStr(pstm, STR_CREIF, &freebytes);   // add "if"   
         }
         pstm=ConcatStr(pstm,SuccFail[i], &freebytes); // copy privilege 
         if (!pstm) {
            *PPstm=NULL;
            return RES_ERR;
         }
      }
   }

   iMAX=sizeof(AccessType)/sizeof(AccessType[0]); 
   one_found=FALSE;
   for (i=0; i<iMAX; i++) {
      if (secparm->baccesstype[i]) {      // one privilege checked.
         if (one_found)                  // not first one.
            pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
         else { 
            one_found=TRUE;
            pstm=ConcatStr(pstm, STR_CREWHEN, &freebytes); // add "when"   
         }
         pstm=ConcatStr(pstm,AccessType[i], &freebytes);  // access types  
         if (!pstm) {
            *PPstm=NULL;
            return RES_ERR;
         }
      }
   }

   // Generates the authid whom logging is performed

   pstm=ConcatStr(pstm,STR_CREBY, &freebytes);  // access types
   switch (secparm->iAuthIdType) {
      case OT_USER:
         pstm=ConcatStr(pstm, STR_USER, &freebytes);
         break;
      case OT_GROUP:
         pstm=ConcatStr(pstm, STR_GROUP, &freebytes);
         break;
      case OT_ROLE:
         pstm=ConcatStr(pstm, STR_ROLE, &freebytes);
         break;
      case OT_PUBLIC:
         break;
   }
   Newpstm=GenCObjList(pstm, secparm->lpfirstUser, &freebytes);
   if (!pstm) {
      *PPstm=NULL;
      return RES_ERR;
   }

   if (secparm->bDBEvent) {
      if (*secparm->DBEvent) {
         pstm=ConcatStr(pstm, STR_RAISE, &freebytes);
         pstm=ConcatStr(pstm, secparm->DBEvent, &freebytes);
         if (secparm->lpDBEventText) {
           pstm=ConcatStr(pstm, " ", &freebytes);
           pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
           pstm=ConcatStr(pstm, secparm->lpDBEventText, &freebytes);
           pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
         }
      }
   }

   if (!pstm) {
      *PPstm=NULL;
      return RES_ERR;
   }

   *PPstm=pstm;         // return the pointer to the generated SQL stmt.
   return RES_SUCCESS;
}

int BuildSQLDBGrant (PPstm, objtype, objname, pchkobj)
UCHAR **PPstm; 
int objtype;
UCHAR *objname;
LPCHECKEDOBJECTS pchkobj;
{
   static char STR_GRANT[]="grant access on database ";
   UCHAR *pstm, *Newpstm; 
   int freebytes=SEGMENT_TEXT_SIZE;

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      *PPstm=pstm;
      return RES_ERR;
   }
   pstm=ConcatStr(pstm,STR_GRANT, &freebytes);  // grant...on database   
   Newpstm=GenCObjList(pstm,pchkobj,&freebytes); // database list
   if (!Newpstm) {
      ESL_FreeMem((void *) pstm);
      *PPstm=NULL;
      return RES_ERR;
   }   
   pstm=ConcatStr(pstm,STR_TO, &freebytes);     // "to"  
   switch (objtype) {
      case OT_USER :
         pstm=ConcatStr(pstm,STR_USER, &freebytes);   // "USER"  
         break;
      case OT_GROUP :
         pstm=ConcatStr(pstm,STR_GROUP, &freebytes);  // "GROUP"  
         break;
      case OT_ROLE :
         pstm=ConcatStr(pstm,STR_ROLE, &freebytes);  // "ROLE"  
         break;
      case OT_PUBLIC :
         break;
      default : 
         return RES_ERR;   
   } 
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(objname),&freebytes);  // user/group name  
   if (!pstm) {
      *PPstm=NULL;
      return RES_ERR;
   }
   *PPstm=pstm;         // return the pointer to the generated SQL stmt.
   return RES_SUCCESS;
}   


int BuildSQLCreIdx(PPstm, idxparm)
UCHAR **PPstm; 
LPINDEXPARAMS idxparm;
{
   UCHAR *pstm, *Newpstm, *pCreIdx, *pKeyList, *NewpKeyList, Uwork[15]; 
   unsigned freebytes=SEGMENT_TEXT_SIZE;
   BOOL AllColumns=TRUE, CheckedColumns=!AllColumns;
   static char STR_CREIDX[]="create index ";
   static char STR_CREUIDX[]="create unique index ";
   static char STR_WITHUSER[]=" with users = (";
   static struct _IndexStorage {
      int nStructure;
      char *cStructure;
   } IndexStorage [] = {
      IDX_BTREE,"btree",
      IDX_ISAM ,"isam",
      IDX_HASH ,"hash",
      IDX_RTREE,"rtree",
      IDX_VWIX, "vectorwise_ix",
      0 ,NULL};
   struct _IndexStorage *pIndexStorage = &IndexStorage[0];


/* ---- allocate the buffer to be returned to the caller ---- */

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      *PPstm=pstm;
      return RES_ERR;
   }

/* ---- generates the statement begining (fixed part) ---- */

   if (idxparm->nUnique==IDX_UNIQUE_NO) 
         pCreIdx=STR_CREIDX;              /* create index */
   else
         pCreIdx=STR_CREUIDX;             /* create unique index */

   fstrncpy(pstm, pCreIdx, x_strlen(pCreIdx)+1);  /* copy create index */
   freebytes-=x_strlen(pCreIdx);

   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(idxparm->objectname), &freebytes); /* index_name */
   pstm=ConcatStr(pstm, STR_ON, &freebytes);              /* on         */
   
   pstm=ConcatStr(pstm,(LPUCHAR)QuoteIfNeeded(idxparm->TableOwner),&freebytes);/* table_name, including owner */
   pstm=ConcatStr(pstm,".",&freebytes);
   pstm=ConcatStr(pstm,(LPUCHAR)QuoteIfNeeded(idxparm->TableName),&freebytes);

   if (!pstm) {         /* test only once for the above...    */
      *PPstm=pstm;      /* stmt since ConcatStr does ...     */
      return RES_ERR;   /* nothing if a NULL ptr is passed on.*/
   }

/* ---- generate the column list for the index ---- */ 

   pstm=ConcatStr(pstm, "(", &freebytes);       /* (                 */
   Newpstm=GenObjList(pstm,idxparm->lpidxCols,  /*   col1, col2...   */
       &freebytes, AllColumns, FALSE);          /*                   */
   if (!Newpstm) {   
      ESL_FreeMem((void *) pstm);
      *PPstm=NULL;   
      return RES_ERR;
   }
   pstm=Newpstm;

	/* 22-Dec-98: don't generate the with clause for gateways.                  */
	/* (exit the function before appending the with clause syntax).             */
	/* The dialog  has been changed (in dbadlg1!creatidx.c), and doesn't        */
	/* propose any more the corresponding parameters, for Gateways              */
	/* (previously, the syntax was generated but probably ignored by at least   */
	/* some gateways                                                            */
	if (GetOIVers() == OIVERS_NOTOI) {
		pstm=ConcatStr(pstm, ")", &freebytes); 
		*PPstm=pstm;
		if (!pstm) 
			return RES_ERR;
		return RES_SUCCESS;
   }

   pstm=ConcatStr(pstm, ") with ", &freebytes); /* ) with             */
   if (!pstm) {   
      *PPstm=pstm;   
      return RES_ERR;
   }

/* ---- generate the column list on which the index is keyed ---- */ 

   pKeyList=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pKeyList) {
      *PPstm=NULL;
      ESL_FreeMem((void *) pstm);
      return RES_ERR;
   }
   NewpKeyList=GenObjList(pKeyList,            /* get the col list  */
      idxparm->lpidxCols,                      /*   col1, col2...   */
      &freebytes, CheckedColumns, FALSE);      /* or NULL           */
   if (NewpKeyList) {                              /* if any generate   */
      pKeyList=NewpKeyList;
      pstm=ConcatStr(pstm, "key=(", &freebytes);   /* key=(             */
      pstm=ConcatStr(pstm, pKeyList, &freebytes);  /*   col1, col2...   */
      pstm=ConcatStr(pstm, "), ", &freebytes);     /* ),                */
   }
   ESL_FreeMem((void *) pKeyList);              /* no longer used */
   if (!pstm) {      
      *PPstm=pstm;   
      return RES_ERR;
   }

/* ---- generate the structure parameter ---- */ 

   pstm=ConcatStr(pstm, " structure=",   /*  with structure= */
      &freebytes);
   if (!pstm) {      
      *PPstm=pstm;   
      return RES_ERR;
   }

/* ----  Look for the storage structure, return RES_ERR if unknown ---- */ 
  
   while (pIndexStorage->cStructure) {
      if (pIndexStorage->nStructure==idxparm->nStructure) 
         break;
      pIndexStorage++;
   }
   if (!pIndexStorage->cStructure) {  /* the storage structure is unkown */  
      *PPstm=pstm;
      return RES_ERR;
   }
   pstm=ConcatStr(pstm,pIndexStorage->cStructure, &freebytes);
   if (!pstm) {         /* test only once for the above...     */
      *PPstm=pstm;      /* stmt since ConcatStr does ...       */
      return RES_ERR;   /* nothing if a NULL ptr is passed on. */
   }

/* ---- Generate the fill factor parameter ---- */

   pstm=ConcatStr(pstm, ", fillfactor=", &freebytes);
   wsprintf(Uwork,"%d", idxparm->nFillFactor);
   if (!pstm) {         /* test only once for the above...     */
      *PPstm=pstm;      /*  stmt since ConcatStr does ...      */
      return RES_ERR;   /* nothing if a NULL ptr is passed on. */
   }
   pstm=ConcatStr(pstm, Uwork, &freebytes);

/* ---- Generate the minpages and maxpages parameters for hash only ---- */

   if (idxparm->nStructure==IDX_HASH) {
       pstm=ConcatStr(pstm, ", minpages=", &freebytes);
       wsprintf(Uwork,"%ld", idxparm->nMinPages);
       pstm=ConcatStr(pstm, Uwork, &freebytes);
       pstm=ConcatStr(pstm, ", maxpages=", &freebytes);
       wsprintf(Uwork,"%ld", idxparm->nMaxPages);
       pstm=ConcatStr(pstm, Uwork, &freebytes);
       if (!pstm) {         /* test only once for the above...     */
          *PPstm=pstm;      /*  stmt since ConcatStr does ...      */
          return RES_ERR;   /* nothing if a NULL ptr is passed on. */
          }
   }

/* --- Generate the leaffill and nonleaffill parameters for btree only --- */

   if (idxparm->nStructure==IDX_BTREE) {
       pstm=ConcatStr(pstm, ", leaffill=", &freebytes);
       wsprintf(Uwork,"%d", idxparm->nLeafFill);
       pstm=ConcatStr(pstm, Uwork, &freebytes);
       pstm=ConcatStr(pstm, ", nonleaffill=", &freebytes);
       wsprintf(Uwork,"%d", idxparm->nNonLeafFill);
       pstm=ConcatStr(pstm, Uwork, &freebytes);
       if (!pstm) {         /* test only once for the above...     */
          *PPstm=pstm;      /*  stmt since ConcatStr does ...      */
          return RES_ERR;   /* nothing if a NULL ptr is passed on. */
          }
   }

/* ---- Generate the location list if any ---- */

   if (idxparm->lpLocations) {       
      pstm=ConcatStr(pstm, STR_LOCATION, &freebytes);
      Newpstm=GenObjList(pstm,idxparm->lpLocations, /*   loc1, loc2... */
          &freebytes, TRUE, FALSE);
      pstm=ConcatStr(pstm, ")", &freebytes);
      if (!pstm) {         /* test only once for the above...     */
         *PPstm=pstm;      /*  stmt since ConcatStr does ...      */
         return RES_ERR;   /* nothing if a NULL ptr is passed on. */
      }
   }
      
/* ---- Generate the allocation  parameter ---- */

   pstm=ConcatStr(pstm, ", allocation=", &freebytes);
   wsprintf(Uwork,"%ld", idxparm->lAllocation);
   pstm=ConcatStr(pstm, Uwork, &freebytes);
   if (!pstm) {         /* test only once for the above...      */
      *PPstm=pstm;      /*  stmt since ConcatStr does ...       */
      return RES_ERR;   /* nothing if a NULL ptr is passed on.  */
   }

/* ---- Generate the extend  parameter ---- */

   pstm=ConcatStr(pstm, ", extend=", &freebytes);
   wsprintf(Uwork,"%ld", idxparm->lExtend);
   pstm=ConcatStr(pstm, Uwork, &freebytes);
   if (!pstm) {         /* test only once for the above...      */
      *PPstm=pstm;      /*  stmt since ConcatStr does ...       */
      return RES_ERR;   /* nothing if a NULL ptr is passed on.  */
   }

/* ---- Generate the compression parameter ---- */
    //
    // Spatial Index RTEE: DBMS does not accept the 'nocompression' key word.
    if (idxparm->nStructure != IDX_RTREE)
    {
        if (idxparm->bKeyCompression || idxparm->bDataCompression) {
           pstm=ConcatStr(pstm, ", compression=(", &freebytes);
           if (idxparm->bKeyCompression)
              pstm=ConcatStr(pstm, "key, ", &freebytes);
           else
              pstm=ConcatStr(pstm, "nokey, ", &freebytes);
           if (idxparm->bDataCompression)
              pstm=ConcatStr(pstm, "data) ", &freebytes);
           else
              pstm=ConcatStr(pstm, "nodata) ", &freebytes);
        }             
        else
           pstm=ConcatStr(pstm, ", nocompression", &freebytes);
        if (!pstm) {         /* test only once for the above...      */
           *PPstm=pstm;      /*  stmt since ConcatStr does ...       */
           return RES_ERR;   /* nothing if a NULL ptr is passed on.  */
        }
    }
      
/* ---- Generate the persistence parameter ---- */

   if (idxparm->bPersistence)
      pstm=ConcatStr(pstm, ", persistence", &freebytes);
   else
      pstm=ConcatStr(pstm, ", nopersistence", &freebytes);
   if (!pstm) {         /* test only once for the above...      */
      *PPstm=pstm;      /*  stmt since ConcatStr does ...       */
      return RES_ERR;   /* nothing if a NULL ptr is passed on.  */
   }

/* ---- Generate the unique scope if applicable ---- */

   switch (idxparm->nUnique) {
      case IDX_UNIQUE_ROW  :    
         pstm=ConcatStr(pstm, ", unique_scope=row", &freebytes);
         break;
      case IDX_UNIQUE_STATEMENT :
         pstm=ConcatStr(pstm, ", unique_scope=statement", &freebytes);
         break;
   }
    //
    // Open Ingres Version 2.0 Begin
    if (idxparm->nStructure == IDX_RTREE)
    {
        char range [160];
        sprintf (range, ", range=((%s, %s), (%s, %s))", idxparm->minX, idxparm->minY, idxparm->maxX, idxparm->maxY);
        pstm = ConcatStr(pstm, range, &freebytes);
    }
    // Generate the page_size.
    if (GetOIVers () != OIVERS_12)
    {
        char pageSize [24];
        wsprintf (pageSize, ", page_size = %u ", idxparm->uPage_size);
        if (idxparm->uPage_size > 2048)
            pstm = ConcatStr(pstm, pageSize, &freebytes);
    }
    //
    // Open Ingres Version 2.0 End


   if (!pstm) {         /* test only once for the above...      */
      *PPstm=pstm;      /*  stmt since ConcatStr does ...       */
      return RES_ERR;   /* nothing if a NULL ptr is passed on.  */
   }

   *PPstm=pstm;         /* return the pointer to the generated SQL stmt.*/
   return RES_SUCCESS;
}                          
int BuildSQLCreTbl(UCHAR **PPstm,LPTABLEPARAMS tblparm)
{
   BOOL bOne_Obj;
   int freebytes=SEGMENT_TEXT_SIZE, i;
   UCHAR *pstm, ustr[MAXOBJECTNAME];
   LPCOLUMNPARAMS lpPrimaryKeyColumns[MAXIMUM_COLUMNS];
   static char STR_PRIMKEY[]=", primary key (";
   static char STR_CRETBL[]="create table ";
   static char STR_FORGNKEY[]=" foreign key (";
   static char STR_REFERENCES[]=") references ";
   static char STR_WITHDEFAULT[]=" with default ";
   static char STR_NOTDEFAULT[]=" not default ";
   static char STR_WITHNULL[]=" with null ";
   static char STR_NOTNULL[]=" not null ";
   static char STR_CONSTRAINT[]=",constraint ";
   static char STR_CHECK[]=" check ";
   static char STR_LPAREN[]="(";
   static char STR_JRNL[]=" journaling "; 
   static char STR_NOJRNL[]=" nojournaling ";
   static char STR_DUP[]=",duplicates "; 
   static char STR_NODUP[]=",noduplicates ";
   static char STR_AUDITKEY[]=",security_audit_key = (";

   LPOBJECTLIST lpRefList;
   LPOBJECTLIST lpColList;
   LPREFERENCEPARAMS lpRefParm;
   LPREFERENCECOLS lpColParm;
   LPCOLUMNPARAMS lpColAttr;

   struct granultype {
      int nGranType;
      UCHAR *cGranType;
   } GranulTypes[] = {
         {USE_DEFAULT,              ""},      
         {LABEL_GRAN_TABLE,         ",label_granularity=table "},      
         {LABEL_GRAN_ROW,           ",label_granularity=row "},
         {LABEL_GRAN_SYS_DEFAULTS,  ",label_granularity=system_default"},
         {LABEL_GRAN_UNKOWN,        ""}};

   struct granultype *GranulType;

   struct secaudittype {
      int uSecAuditType;
      UCHAR *cSecAuditType;
   } SecAuditTypes[] = {
         {USE_DEFAULT,                 ""},      
         {SECURITY_AUDIT_TABLEROW,     " ,security_audit =(table, row) "},
         {SECURITY_AUDIT_TABLENOROW,   " ,security_audit =(table, norow) "},
         {SECURITY_AUDIT_ROW,          " ,security_audit =(row) "},
         {SECURITY_AUDIT_NOROW,        " ,security_audit =(norow) "},
         {SECURITY_AUDIT_TABLE,        " ,security_audit =(table) "},
         {SECURITY_AUDIT_UNKOWN,       ""}};

   struct secaudittype *SecAuditType;

   struct coltype {
      UINT uDataType;
      UCHAR *cDataType;
      BOOL bLnSignificant;
   } ColTypes[] = {
         {INGTYPE_C           ," c("             ,TRUE},
         {INGTYPE_CHAR        ," char("          ,TRUE},
         {INGTYPE_TEXT        ," text("          ,TRUE},
         {INGTYPE_VARCHAR     ," varchar("       ,TRUE},
         {INGTYPE_LONGVARCHAR ," long varchar "  ,FALSE},
         {INGTYPE_INT8        ,"bigint"          ,FALSE},
         {INGTYPE_INT4        ," int "           ,FALSE},
         {INGTYPE_INT2        ," smallint"       ,FALSE},
         {INGTYPE_INT1        ," integer1"       ,FALSE},
         {INGTYPE_DECIMAL     ," decimal("       ,TRUE},
         {INGTYPE_FLOAT8      ," float"          ,FALSE},
         {INGTYPE_FLOAT4      ," real"           ,FALSE},
         {INGTYPE_DATE        ," date"           ,FALSE},
         {INGTYPE_ADTE        ," ansidate"                       ,FALSE},
         {INGTYPE_TMWO        ," time without time zone"         ,FALSE},
         {INGTYPE_TMW         ," time with time zone"            ,FALSE},
         {INGTYPE_TME         ," time with local time zone"      ,FALSE},
         {INGTYPE_TSWO        ," timestamp without time zone"    ,FALSE},
         {INGTYPE_TSW         ," timestamp with time zone"       ,FALSE},
         {INGTYPE_TSTMP       ," timestamp with local time zone" ,FALSE},
         {INGTYPE_INYM        ," interval year to month"         ,FALSE},
         {INGTYPE_INDS        ," interval day to second"         ,FALSE},
         {INGTYPE_IDTE        ," ingresdate"                     ,FALSE},
         {INGTYPE_MONEY       ," money"          ,FALSE},
         {INGTYPE_BYTE        ," byte("          ,TRUE},
         {INGTYPE_BYTEVAR     ," byte varying"   ,FALSE},
         {INGTYPE_LONGBYTE    ," long byte"      ,FALSE},
         {INGTYPE_OBJKEY      ," object_key"     ,FALSE},
         {INGTYPE_TABLEKEY    ," table_key"      ,FALSE},
         {INGTYPE_FLOAT       ," float"          ,FALSE},
         {INGTYPE_ERROR       ,""                ,FALSE}};

   struct coltype *ColType;

   for  (i=0; i<MAXIMUM_COLUMNS; i++)
      lpPrimaryKeyColumns[i]=NULL;      

/**** Verify input parameters ****/

   if (!*(tblparm->objectname)) {
      myerror(ERR_INVPARAMS);
      return RES_ERR;
   }

   if (!tblparm->lpColumns && !tblparm->bCreateAsSelect) {
      myerror(ERR_INVPARAMS);
      return RES_ERR;
   }

/****  allocate the buffer to be returned to the caller ****/

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      *PPstm=pstm;
      return RES_ERR;
   }


/**** Generates the fixed part : create table table_name ( ****/

   pstm=ConcatStr(pstm, STR_CRETBL, &freebytes);  /* copy create table */
   if (*tblparm->szSchema) {
      pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(tblparm->szSchema), &freebytes);
      pstm=ConcatStr(pstm, ".", &freebytes);
   }
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(tblparm->objectname), &freebytes); /* table name */

   if (tblparm->bCreateAsSelect)
   {    // Create Table as Select ....
        if (tblparm->lpColumnHeader)
        {
            pstm = ConcatStr (pstm, "(", &freebytes);
            pstm = ConcatStr (pstm, tblparm->lpColumnHeader, &freebytes);
            pstm = ConcatStr (pstm, ")", &freebytes);
        }
        pstm = ConcatStr (pstm, " as ", &freebytes);
        pstm = ConcatStr (pstm, tblparm->lpSelectStatement, &freebytes);
        if (!pstm) 
        {
            myerror(ERR_ALLOCMEM);
            return RES_ERR;
        }
   }
   else
   {
        pstm=ConcatStr(pstm, STR_LPAREN, &freebytes);          /* (          */
        if (!pstm) {
            myerror(ERR_ALLOCMEM);
            return RES_ERR;
        }

        if (!tblparm->lpColumns) {
            myerror(ERR_INVPARAMS);
            return RES_ERR;
        }

/***** Generate the column list (required) ****/

   
   lpColList=(LPOBJECTLIST) tblparm->lpColumns;

   bOne_Obj=FALSE;

   while (lpColList) {      // list of columns : mandatory

      lpColAttr=(LPCOLUMNPARAMS) lpColList->lpObject;

      if (!lpColAttr) {
         ESL_FreeMem(pstm);
         myerror(ERR_INVPARAMS);
         return RES_ERR;
      }

      if (lpColAttr->nPrimary>0)
         lpPrimaryKeyColumns[lpColAttr->nPrimary-1]=lpColAttr;

      if (bOne_Obj) 
         pstm=ConcatStr(pstm, STR_COMMA, &freebytes);     /* , */
      else
         bOne_Obj=TRUE;
      
      suppspace(lpColAttr->szColumn);
      pstm=ConcatStr(pstm, lpColAttr->szColumn, &freebytes); // col name

      if (!pstm) {
         myerror(ERR_ALLOCMEM);
         return RES_ERR;
      }

      /**** get the column type ****/

      ColType=&ColTypes[0];

      while (ColType->uDataType!=INGTYPE_ERROR) {
         if (ColType->uDataType==lpColAttr->uDataType)
            break;
         ColType++;
      }
      
      if (ColType->uDataType==INGTYPE_ERROR) {
         ESL_FreeMem(pstm);
         myerror(ERR_INVPARAMS);
         return RES_ERR;
      }

      pstm=ConcatStr(pstm, ColType->cDataType, &freebytes); 

      if (!pstm) {
         myerror(ERR_ALLOCMEM);
         return RES_ERR;
      }

      /**** Get the column length if applicable ****/

      if (ColType->bLnSignificant) {
         if (ColType->uDataType==INGTYPE_DECIMAL) {
            my_itoa(lpColAttr->lenInfo.decInfo.nPrecision, ustr, 10);
            pstm=ConcatStr(pstm, ustr, &freebytes);
            pstm=ConcatStr(pstm, ",", &freebytes);    // added emb 26/11/96
            my_itoa(lpColAttr->lenInfo.decInfo.nScale, ustr, 10);
            pstm=ConcatStr(pstm, ustr, &freebytes);
         }
         else {
            /* Old
            my_itoa((int) lpColAttr->lenInfo.dwDataLen, ustr, 10);
            pstm=ConcatStr(pstm, ustr, &freebytes);
            */
            //
            // Vut changes 09-August-1995
            //
            switch (ColType->uDataType)
            {
               case INGTYPE_C:
               case INGTYPE_TEXT:
                   if (lpColAttr->lenInfo.dwDataLen > 0)
                       my_itoa((int) lpColAttr->lenInfo.dwDataLen, ustr, 10);
                   else
                       my_itoa((int) 1, ustr, 10);
                   break;
               default:
                   my_itoa((int) lpColAttr->lenInfo.dwDataLen, ustr, 10);
                   break;
            }
            pstm=ConcatStr(pstm, ustr, &freebytes);
         }
         pstm=ConcatStr(pstm, STR_RPAREN, &freebytes);  /* , */
         if (!pstm) {
            myerror(ERR_ALLOCMEM);
            return RES_ERR;
         }
      }



      /**** generate the COLUMN constraints if any ****/

//      if (lpColAttr->bUnique && tblparm->nbKey != 1)
      if (lpColAttr->bUnique)
         pstm=ConcatStr(pstm, STR_UNIQUE, &freebytes); 
      if (lpColAttr->bNullable)
         pstm=ConcatStr(pstm, STR_WITHNULL, &freebytes); 
      else
         pstm=ConcatStr(pstm, STR_NOTNULL, &freebytes); 
      if (lpColAttr->bDefault) {
         pstm=ConcatStr(pstm, STR_WITHDEFAULT, &freebytes);
         if (lpColAttr->lpszDefSpec)
            if (*(lpColAttr->lpszDefSpec))
              pstm=ConcatStr(pstm, lpColAttr->lpszDefSpec, &freebytes);
      }
      else
         pstm=ConcatStr(pstm, STR_NOTDEFAULT, &freebytes); 

      if (lpColAttr->lpszCheck) 
         if (*(lpColAttr->lpszCheck)) {
            pstm=ConcatStr(pstm, STR_CHECK, &freebytes);
            pstm=ConcatStr(pstm, STR_LPAREN, &freebytes);
            pstm=ConcatStr(pstm, lpColAttr->lpszCheck, &freebytes);
            pstm=ConcatStr(pstm, STR_RPAREN, &freebytes);
         }

      if (!pstm) {
         myerror(ERR_ALLOCMEM);
         return RES_ERR;
      }

      lpColList=lpColList->lpNext;
   }

   if (!bOne_Obj) {              // no columns : error
      myerror(ERR_INVPARAMS);
      return RES_ERR;
   }


/**** Generate the TABLE foreign key contraints if any ****/

   lpRefList=tblparm->lpReferences;

   while (lpRefList) {

      lpRefParm=(LPREFERENCEPARAMS) lpRefList->lpObject;

      if (!lpRefParm) {
         ESL_FreeMem(pstm);
         myerror(ERR_INVPARAMS);
         return RES_ERR;
      }


      if (*(lpRefParm->szConstraintName)) {   // copy constraint name if any
         pstm=ConcatStr(pstm, STR_CONSTRAINT, &freebytes); 
         pstm=ConcatStr(pstm, lpRefParm->szConstraintName, &freebytes);
      }
      else 
         pstm=ConcatStr(pstm, STR_COMMA, &freebytes);     /* , */

      pstm=ConcatStr(pstm, STR_FORGNKEY, &freebytes);  /* foreign key( */

      lpColList=(LPOBJECTLIST) lpRefParm->lpColumns;

      bOne_Obj=FALSE;

      while (lpColList) {      // list of referencing columns : mandatory

         lpColParm=(LPREFERENCECOLS) lpColList->lpObject;
         if (!lpColParm) {
            ESL_FreeMem(pstm);
            myerror(ERR_INVPARAMS);
            return RES_ERR;
         }

         if (bOne_Obj) 
            pstm=ConcatStr(pstm, STR_COMMA, &freebytes);     /* , */
         else
            bOne_Obj=TRUE;   

         pstm=ConcatStr(pstm, lpColParm->szRefingColumn, &freebytes);
         if (!pstm) {
            myerror(ERR_ALLOCMEM);
            return RES_ERR;
         }

         lpColList=lpColList->lpNext;
      }

      if (!bOne_Obj) {              // no columns : error
         myerror(ERR_INVPARAMS);
         return RES_ERR;
      }

      pstm=ConcatStr(pstm, STR_REFERENCES, &freebytes);     /* ) */
      if (*lpRefParm->szTable)
         pstm=ConcatStr(pstm, lpRefParm->szTable, &freebytes); /* refd table */
      else
         pstm=ConcatStr(pstm, tblparm->objectname, &freebytes); /* same one */


      if (!pstm) {
         myerror(ERR_INVPARAMS);
         return RES_ERR;
      }

      lpColList=(LPOBJECTLIST) lpRefParm->lpColumns;

      bOne_Obj=FALSE;

      while (lpColList) {      // list of referenced columns : optional

         lpColParm=(LPREFERENCECOLS) lpColList->lpObject;
         if (!lpColParm) {
            ESL_FreeMem(pstm);
            myerror(ERR_INVPARAMS);
            return RES_ERR;
         }

         if (bOne_Obj) 
            pstm=ConcatStr(pstm, STR_COMMA, &freebytes);     /* , */
         else {
            pstm=ConcatStr(pstm, STR_LPAREN, &freebytes);    /* ( */
            bOne_Obj=TRUE;
         }

         pstm=ConcatStr(pstm, lpColParm->szRefedColumn, &freebytes);
         if (!pstm) {
            myerror(ERR_ALLOCMEM);
            return RES_ERR;
         }

         lpColList=lpColList->lpNext;
      }


      if (bOne_Obj)                                      // columns list...
         pstm=ConcatStr(pstm, STR_ENDSTM, &freebytes);   // generated : )

      lpRefList=lpRefList->lpNext;
   }

/**** Generate the TABLE contraints if any (Drag and drop only) ****/

   lpRefList=tblparm->lpConstraint;

   while (lpRefList) {
      pstm=ConcatStr(pstm, STR_COMMA, &freebytes);     /* , */
      if ((LPCONSTRAINTPARAMS)lpRefList->lpObject)
         if (((LPCONSTRAINTPARAMS)lpRefList->lpObject)->lpText)
           pstm=ConcatStr(pstm,
               ((LPCONSTRAINTPARAMS)lpRefList->lpObject)->lpText, &freebytes);
         else
            return RES_ERR;
      else
         return RES_ERR;
      if (!pstm) {
         myerror(ERR_ALLOCMEM);
         return RES_ERR;
      }

      lpRefList=lpRefList->lpNext;
   }

/**** Generates the check constraint if any. ****/

      // to be FINISHED when the pointer to the OBJECTLIST will be implemented

/**** Generates primary key if any. ****/

   if (lpPrimaryKeyColumns[0])   // At least one column specified as primary
      pstm=ConcatStr(pstm, STR_PRIMKEY, &freebytes);

   for  (i=0; i<MAXIMUM_COLUMNS; i++) {
      if (lpPrimaryKeyColumns[i]==NULL)
         break;
      if (i>0)
         pstm=ConcatStr(pstm, STR_COMMA, &freebytes);     /* , */
      pstm=ConcatStr(pstm, lpPrimaryKeyColumns[i]->szColumn, &freebytes);
   }

   if (lpPrimaryKeyColumns[0])   // At least one column specified as primary
      pstm=ConcatStr(pstm, STR_RPAREN, &freebytes);       /* )          */



   pstm=ConcatStr(pstm, STR_RPAREN, &freebytes);       /* )          */

/**** Generates the With clause ****/ 
   } // Else of (if (tblparm->bCreateAsSelect)

   pstm=ConcatStr(pstm, STR_WITH, &freebytes);            /* WITH       */

   if (tblparm->bJournaling)
      pstm=ConcatStr(pstm, STR_JRNL, &freebytes);     /* journaling */
   else
      pstm=ConcatStr(pstm, STR_NOJRNL, &freebytes);   /* journaling */

   if (tblparm->bDuplicates)
      pstm=ConcatStr(pstm, STR_DUP, &freebytes);      /* duplicates */
   else
      pstm=ConcatStr(pstm, STR_NODUP, &freebytes);    /* noduplicates */

   //
   // OpenIngres Version only -> page size
   //
   if (GetOIVers() != OIVERS_12)
   {
        char pageSize [24];
        if (tblparm->uPage_size > 2048)
        {
            wsprintf (pageSize, ", page_size=%u ", tblparm->uPage_size);
            pstm = ConcatStr (pstm, pageSize, &freebytes);   
        }
   }
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

/**** Generate the location list if any ****/

   if (tblparm->lpLocations) {  
      pstm=ConcatStr(pstm, STR_LOCATION, &freebytes);
      pstm=GenObjList(pstm,tblparm->lpLocations, /*   loc1, loc2... */
          &freebytes, TRUE, FALSE);
      pstm=ConcatStr(pstm, STR_RPAREN, &freebytes);
      if (!pstm) {         /* test only once for the above...     */
         *PPstm=pstm;      /*  stmt since ConcatStr does ...      */
         return RES_ERR;   /* nothing if a NULL ptr is passed on. */
      }
   }

/**** Label granularity ****/

      GranulType=&GranulTypes[0];

      while (GranulType->nGranType!=LABEL_GRAN_UNKOWN) {
         if (GranulType->nGranType==tblparm->nLabelGran)
            break;
         GranulType++;
      }
      
      if (GranulType->nGranType==LABEL_GRAN_UNKOWN) {
         ESL_FreeMem(pstm);
         myerror(ERR_INVPARAMS);
         return RES_ERR;
      }
                                       
      pstm=ConcatStr(pstm, GranulType->cGranType, &freebytes); 

      if (!pstm) {
         myerror(ERR_ALLOCMEM);
         return RES_ERR;
      }

/**** Security audit option ****/


      SecAuditType=&SecAuditTypes[0];

      while (SecAuditType->uSecAuditType!=SECURITY_AUDIT_UNKOWN) {
         if (SecAuditType->uSecAuditType==tblparm->nSecAudit) {
            pstm=ConcatStr(pstm, SecAuditType->cSecAuditType, &freebytes); 
            break;
            if (!pstm) {
               myerror(ERR_ALLOCMEM);
               return RES_ERR;
            }
         }
         SecAuditType++;
      }
      
      if (SecAuditType->uSecAuditType==SECURITY_AUDIT_UNKOWN) {
         ESL_FreeMem(pstm);
         myerror(ERR_INVPARAMS);
         return RES_ERR;
      }


      if (!pstm) {
         myerror(ERR_ALLOCMEM);
         return RES_ERR;
      }


/**** Security audit key, if any ****/

      if (*(tblparm->szSecAuditKey)) {
         pstm=ConcatStr(pstm, STR_AUDITKEY, &freebytes); 
         pstm=ConcatStr(pstm, tblparm->szSecAuditKey, &freebytes); 
         pstm=ConcatStr(pstm, STR_RPAREN, &freebytes); 
         if (!pstm) {
            myerror(ERR_ALLOCMEM);
            return RES_ERR;
         }
      }

   *PPstm=pstm;         /* return the pointer to the generated SQL stmt.*/
   return RES_SUCCESS;
      
}
int BuildSQLCreProc(PPstm, prcparm)
UCHAR **PPstm; 
LPPROCEDUREPARAMS prcparm;
{
   UCHAR *pstm; 
   unsigned freebytes=SEGMENT_TEXT_SIZE;
   static char STR_CREPRC[]="create procedure ";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      *PPstm=pstm;
      return RES_ERR;
   }



   /* because of problems with some C compiler the following tests are
      nexted instead of using the ANSI left to right evaluation */

   if (prcparm->bOnlyText)
      if (prcparm->lpProcedureText) 
         if (*(prcparm->lpProcedureText)) {
            pstm=ConcatStr(pstm, prcparm->lpProcedureText, &freebytes);
            if (!pstm) {
               *PPstm=pstm;
               return RES_ERR;
            }
            else {
               *PPstm=pstm;       /* return ptr to generated SQL stmt.*/
               return RES_SUCCESS;
            }
         }


   // Create is the caller so we do not have the full text */
   // generates the statement begining (fixed part) 

   fstrncpy(pstm, STR_CREPRC, x_strlen(STR_CREPRC)+1);  /* create procedure */
   freebytes-=x_strlen(STR_CREPRC);

   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(prcparm->objectname), &freebytes); /* proc_name */
   if (!pstm) {
      *PPstm=pstm;
      return RES_ERR;
   }

   /* because of problems with som C compiler the following tests are
      nexted instead of using the ANSI left to right evaluation */

   if (prcparm->lpProcedureParams) {
      if (*(prcparm->lpProcedureParams)) {
         pstm=ConcatStr(pstm, " (", &freebytes);
         pstm=ConcatStr(pstm, prcparm->lpProcedureParams, &freebytes);
         pstm=ConcatStr(pstm, ") ", &freebytes);
      }
   }

   pstm=ConcatStr(pstm, " as ", &freebytes);

   if (prcparm->lpProcedureDeclare) {
      if (*(prcparm->lpProcedureDeclare)) {
         pstm=ConcatStr(pstm, "declare ", &freebytes);
         pstm=ConcatStr(pstm, prcparm->lpProcedureDeclare, &freebytes);
      }
   }
   if (!prcparm->lpProcedureStatement) {
      if (!*(prcparm->lpProcedureStatement)) {
         if (!pstm) {
            *PPstm=pstm;
            return RES_ERR;
         }
      }
   }
   pstm=ConcatStr(pstm, " begin ", &freebytes);
   pstm=ConcatStr(pstm, prcparm->lpProcedureStatement, &freebytes);
   pstm=ConcatStr(pstm, " end ", &freebytes);
   if (!pstm) {
      *PPstm=pstm;
      return RES_ERR;
   }
   *PPstm=pstm;         /* return the pointer to the generated SQL stmt.*/
   return RES_SUCCESS;
}

int BuildSQLCreSeq(PPstm, seqparm)
UCHAR **PPstm; 
LPSEQUENCEPARAMS seqparm;
{
   UCHAR *pstm; 
   unsigned freebytes=SEGMENT_TEXT_SIZE;
   static char STR_CRESEQ[]      = "create sequence ";
   static char STR_AS[]          = " as ";
   static char STR_STARTWITH[]   = " start with ";
   static char STR_INCREMENTBY[] = " increment by ";
   static char STR_MINVALUE[]    = " minvalue ";
   static char STR_MAXVALUE[]    = " maxvalue ";
   static char STR_CACHE[]       = " cache ";
   static char STR_NOCACHE[]     = " nocache ";
   static char STR_CYCLE[]       = " cycle ";
   static char STR_NOCYCLE[]     = " nocycle ";
   static char STR_DATA_INTEGER[]= " integer";
   static char STR_DATA_DECIMAL[]= " decimal";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      *PPstm=pstm;
      return RES_ERR;
   }

   fstrncpy(pstm, STR_CRESEQ, x_strlen(STR_CRESEQ)+1);  /* create sequence */
   freebytes-=x_strlen(STR_CRESEQ);

   if (*(seqparm->objectowner)) {
      pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(seqparm->objectowner), &freebytes); /* seq_owner */
      pstm=ConcatStr(pstm, ".", &freebytes);
   }
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(seqparm->objectname), &freebytes); /* seq_name */

   pstm=ConcatStr(pstm, STR_AS, &freebytes);

   if ( seqparm->bDecimalType )
   {
      pstm=ConcatStr(pstm, "decimal ", &freebytes);
      if (*((seqparm->szDecimalPrecision)))
      {
         pstm=ConcatStr(pstm, "(", &freebytes);
         pstm=ConcatStr(pstm, seqparm->szDecimalPrecision, &freebytes);
         pstm=ConcatStr(pstm, ") ", &freebytes);
      }
   }
   else
      pstm=ConcatStr(pstm, "integer ", &freebytes);

   if (*(seqparm->szStartWith))
   {
      pstm=ConcatStr(pstm, STR_STARTWITH, &freebytes);
      pstm=ConcatStr(pstm, seqparm->szStartWith, &freebytes);
   }

   if (*(seqparm->szIncrementBy))
   {
      pstm=ConcatStr(pstm, STR_INCREMENTBY, &freebytes);
      pstm=ConcatStr(pstm, seqparm->szIncrementBy, &freebytes);
   }

   if (*(seqparm->szMinValue))
   {
      pstm=ConcatStr(pstm, STR_MINVALUE, &freebytes);
      pstm=ConcatStr(pstm, seqparm->szMinValue, &freebytes);
   }

   if (*(seqparm->szMaxValue))
   {
      pstm=ConcatStr(pstm, STR_MAXVALUE, &freebytes);
      pstm=ConcatStr(pstm, seqparm->szMaxValue, &freebytes);
   }

   if (*(seqparm->szCacheSize))
   {
      pstm=ConcatStr(pstm, STR_CACHE, &freebytes);
      pstm=ConcatStr(pstm, seqparm->szCacheSize, &freebytes);
   }
   else
      pstm=ConcatStr(pstm, STR_NOCACHE, &freebytes);

   if (seqparm->bCycle)
      pstm=ConcatStr(pstm, STR_CYCLE, &freebytes);
   else
      pstm=ConcatStr(pstm, STR_NOCYCLE, &freebytes);

   if (!pstm) {
      *PPstm=pstm;
      return RES_ERR;
   }

   *PPstm=pstm;         /* return the pointer to the generated SQL stmt.*/
   return RES_SUCCESS;
}

int BuildSQLCreView(PPstm, viewparm)
UCHAR **PPstm; 
LPVIEWPARAMS viewparm;
{
   UCHAR *pstm; 
   unsigned freebytes=SEGMENT_TEXT_SIZE;
   static char STR_CREVIEW[]="create view ";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      *PPstm=pstm;
      return RES_ERR;
   }

   // generates the statement begining (fixed part) 

   // Emb 26/06/96 : workaround for differences
   // in getdetailinfo (column text_segment of iiviews)
   if (viewparm->bOnlyText) {
        pstm=ConcatStr(pstm, viewparm->lpViewText, &freebytes); /* view text */
        *PPstm=pstm;
        if (!pstm) 
          return RES_ERR;
        return RES_SUCCESS;
   }

   fstrncpy(pstm, STR_CREVIEW, x_strlen(STR_CREVIEW)+1);  /* create view */
   freebytes-=x_strlen(STR_CREVIEW);

   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(viewparm->objectname), &freebytes); /* view_name */
   if (!pstm) {
      *PPstm=pstm;
      return RES_ERR;
   }
   /*  generate the 'select' statement */
   pstm=ConcatStr(pstm, " ", &freebytes); /* view_name */
   pstm=ConcatStr(pstm, viewparm->lpSelectStatement, &freebytes);
   if (viewparm->WithCheckOption)
      pstm=ConcatStr(pstm, " with check option", &freebytes);
   if (!pstm) {         /* test only once for the above...      */
      *PPstm=pstm;      /*  stmt since ConcatStr does ...       */
      return RES_ERR;   /* nothing if a NULL ptr is passed on.  */
   }

   *PPstm=pstm;         /* return the pointer to the generated SQL stmt.*/
   return RES_SUCCESS;
}
int BuildSQLGrant(PPstm, grantparm)
UCHAR **PPstm; 
LPGRANTPARAMS grantparm;
{
   UCHAR *pstm, *pstring;
   int i;
   unsigned freebytes=SEGMENT_TEXT_SIZE;
   static char STR_CREGRANT[]="grant  ";
   BOOL One_Found=FALSE;
   typedef struct GrantObjectTypes {
         int int_value; 
         UCHAR *uchar_value;
   } GRANTOBJECTTYPE, FAR *LPGRANTOBJECTTYPE;
   static GRANTOBJECTTYPE grantobjectype[]={
         {OT_USER,     "user "}, 
         {OT_GROUP,    "group "}, 
         {OT_PUBLIC,   " "},           /* no type in statement */ 
         {OT_TABLE,    "table "}, 
         {OT_PROCEDURE,"procedure "}, 
         {OT_DATABASE, "database "}, 
         {OT_DBEVENT,  "dbevent "}, 
         {OT_VIEW,     " "},           /* Vut: Ingres say tha syntax incorrect if I use "view ", why ? */
         {OT_ROLE,     "role "},
         {OT_SEQUENCE, "next on sequence "},
         {-1,          NULL}};
   LPGRANTOBJECTTYPE lpgrantobjectype=grantobjectype;

   if (!grantparm->lpobject) {       /* no object to grant = error */
//      *PPstm=pstm;   
      return RES_ERR;
   }
   if (!grantparm->lpgrantee) {      /* no grantees = error */       
//      *PPstm=pstm;   
      return RES_ERR;
   }

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      *PPstm=pstm;
      return RES_ERR;
   }

   /* generates the statement begining (fixed part) */

   fstrncpy(pstm, STR_CREGRANT, x_strlen(STR_CREGRANT)+1);  /* grant */
   freebytes-=x_strlen(STR_CREGRANT);

   /* generates the privilege list  */

   if (grantparm->ObjectType!=OT_ROLE && grantparm->ObjectType!=OT_SEQUENCE ) { 
      for (i=0; i<GRANT_MAX_PRIVILEGES; i++) {
         if (grantparm->Privileges[i]) {
            if (!One_Found)
               One_Found=TRUE;
            else
               pstm=ConcatStr(pstm, ", ", &freebytes);
            pstring=GetPrivilegeString(i);
            if (!pstring) {
               *PPstm=NULL;   
               ESL_FreeMem((void *) pstm);
               return RES_ERR;
            }
            pstm=ConcatStr(pstm, pstring, &freebytes);
            if (!pstm) {      
               *PPstm=pstm;   
               return RES_ERR;
            }
            if (HasPrivilegeRelatedValue(i)) {
               UCHAR ustr[10];
               my_itoa(grantparm->PrivValues[i], ustr, 10);
               pstm=ConcatStr(pstm, ustr, &freebytes);
               if (!pstm) {      
                  *PPstm=pstm;   
                  return RES_ERR;
               }
            }
            /* generates the excluding columns if any */
            if (grantparm->lpcolumn) {       
               pstm=ConcatStr(pstm, " excluding (", &freebytes);
               pstm=GenGrantObjList(pstm, grantparm->lpcolumn, &freebytes);
               pstm=ConcatStr(pstm, " )", &freebytes);
            }
         }
      }
      if (!One_Found) {
         *PPstm=NULL;   
         ESL_FreeMem((void *) pstm);
         return RES_ERR;
      }
   }

   if (!pstm) {      
      *PPstm=pstm;   
      return RES_ERR;
   }

   /* generates the object list */

   if (grantparm->ObjectType!=OT_ROLE && grantparm->ObjectType!=OT_SEQUENCE)
      pstm=ConcatStr(pstm, STR_ON, &freebytes);
   while (lpgrantobjectype->uchar_value) {   /* look for object type */
      if (lpgrantobjectype->int_value == grantparm->ObjectType)
         break;
      ++lpgrantobjectype;
   }
   if (lpgrantobjectype->int_value ==-1) {   /* object type unknown */
      ESL_FreeMem((void *) pstm);
      *PPstm=NULL;   
      return RES_ERR;
   }
   if (grantparm->ObjectType==OT_DATABASE && grantparm->bInstallLevel) {
      pstm=ConcatStr(pstm, "current installation", &freebytes);
   }
   else {
      if (grantparm->ObjectType!=OT_ROLE)
         pstm=ConcatStr(pstm, lpgrantobjectype->uchar_value, &freebytes);
      pstm=GenGrantObjList(pstm, grantparm->lpobject, &freebytes);
   }
   if (!pstm) {
      *PPstm=pstm;
      return RES_ERR;
   }

   /* generates the auth id list */

   pstm=ConcatStr(pstm, " to  ", &freebytes);
   lpgrantobjectype=grantobjectype;
   while (lpgrantobjectype->uchar_value) {   /* look for grantee type */
      if (lpgrantobjectype->int_value == grantparm->GranteeType)
         break;
      ++lpgrantobjectype;
   }
   if (lpgrantobjectype->int_value ==-1) {   /* grantee type unknown */
      ESL_FreeMem((void *) pstm);
      *PPstm=NULL;   
      return RES_ERR;
   }

   pstm=ConcatStr(pstm, lpgrantobjectype->uchar_value, &freebytes);

   pstm=GenGrantObjList(pstm, grantparm->lpgrantee, &freebytes);
   if (!pstm) {                               
      *PPstm=pstm;   
      return RES_ERR;
   }

   /* generates the grant option if appropriate */

   if (grantparm->grant_option) {       
      pstm=ConcatStr(pstm, " with grant option", &freebytes);
      if (!pstm) {                               
         *PPstm=pstm;   
         return RES_ERR;
      }
   }
   *PPstm=pstm;         /* return the pointer to the generated SQL stmt.*/
   return RES_SUCCESS;
}
int BuildSQLRevoke(PPstm, revokeparm)
UCHAR **PPstm; 
LPREVOKEPARAMS revokeparm;
{
   int i;
   UCHAR *pstm, *pstring;
   unsigned freebytes=SEGMENT_TEXT_SIZE;
   static char STR_CREREVOKE[]="revoke ";
   static char STR_REVOKEGRANT[]="grant option for ";
   BOOL One_Found=FALSE;
   typedef struct RevokeObjectTypes {
         int int_value; 
         UCHAR *uchar_value;
   } REVOKEOBJECTTYPE, FAR *LPREVOKEOBJECTTYPE;
   static REVOKEOBJECTTYPE revokeobjectype[]={
         {OT_USER,     "user "     }, 
         {OT_PUBLIC,   " "         },           /* no type in statement */ 
         {OT_GROUP,    "group "    }, 
         {OT_ROLE,     "role "     }, 
         {OT_TABLE,    "table "    }, 
         {OT_PROCEDURE,"procedure "}, 
         {OT_DATABASE, "database " }, 
         {OT_DBEVENT,  "dbevent "  }, 
         {OT_VIEW,     " "         },           /* Vut changes: Use the default specification   */
         {OT_SEQUENCE, "sequence " },
         {-1, NULL                 }
   };

   LPREVOKEOBJECTTYPE lprevokeobjectype=revokeobjectype;

   if (!revokeparm->lpobject) {       /* no object to revoke = error */
//      *PPstm=pstm;   
      return RES_ERR;
   }
   if (!revokeparm->lpgrantee) {      /* no grantees = error */       
//      *PPstm=pstm;   
      return RES_ERR;
   }

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      *PPstm=pstm;
      return RES_ERR;
   }

   /* generates the statement begining (fixed part) */

   fstrncpy(pstm, STR_CREREVOKE, x_strlen(STR_CREREVOKE)+1);  /* revoke */
   freebytes-=x_strlen(STR_CREREVOKE);

   /* generates the revoke of the grant option if appropriate */

   if (revokeparm->grant_option) {
      pstm=ConcatStr(pstm, STR_REVOKEGRANT, &freebytes);
      if (!pstm) {      
         *PPstm=pstm;   
         return RES_ERR;
      }
   }

   if (revokeparm->ObjectType!=OT_ROLE) {
      if(revokeparm->ObjectType==OT_SEQUENCE) {
         pstm=ConcatStr(pstm, " next ", &freebytes);
      }
      else {
          /* generates the privileges to be dropped  */
          One_Found=FALSE;
          for (i=0; i<GRANT_MAX_PRIVILEGES; i++) {
             if (revokeparm->Privileges[i]) {
                if (!One_Found)
                   One_Found=TRUE;
                else 
                   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

                pstring=GetPrivilegeString(i);
                if (!pstring) {
                   *PPstm=NULL;   
                   ESL_FreeMem((void *) pstm);
                   return RES_ERR;
                }
                pstm=ConcatStr(pstm, pstring, &freebytes);
                if (!pstm) {      
                   *PPstm=pstm;   
                   return RES_ERR;
                }
             }
          }
      }

      /* generates the object list */

      pstm=ConcatStr(pstm, STR_ON, &freebytes);
      while (lprevokeobjectype->uchar_value) {   /* look for object type */
         if (lprevokeobjectype->int_value == revokeparm->ObjectType)
            break;
         ++lprevokeobjectype;
      }
      if (lprevokeobjectype->int_value ==-1) {   /* object type unknown */
         ESL_FreeMem((void *) pstm);
         *PPstm=NULL;   
         return RES_ERR;
      }
      if (revokeparm->ObjectType == OT_DATABASE && revokeparm->bInstallLevel)
         pstm=ConcatStr(pstm, "current installation", &freebytes);
      else
         pstm=ConcatStr(pstm, lprevokeobjectype->uchar_value, &freebytes);
   }

  if (!(revokeparm->ObjectType == OT_DATABASE && revokeparm->bInstallLevel))
     pstm=GenGrantObjList(pstm, revokeparm->lpobject, &freebytes);
   if (!pstm) {      
      *PPstm=pstm;   
      return RES_ERR;
   }

   /* generates the auth id list */

   pstm=ConcatStr(pstm, " from ", &freebytes);
   lprevokeobjectype=revokeobjectype;
   while (lprevokeobjectype->uchar_value) {   /* look for grantee type */
      if (lprevokeobjectype->int_value == revokeparm->GranteeType)
         break;
      ++lprevokeobjectype;
   }
   if (lprevokeobjectype->int_value ==-1) {   /* grantee type unknown */
      ESL_FreeMem((void *) pstm);
      *PPstm=NULL;   
      return RES_ERR;
   }
   pstm=ConcatStr(pstm, lprevokeobjectype->uchar_value, &freebytes);
   pstm=GenGrantObjList(pstm, revokeparm->lpgrantee, &freebytes);
   if (!pstm) {                               
      *PPstm=pstm;   
      return RES_ERR;
   }

   /* generates the cascade / restrict parameter */
   if (revokeparm->ObjectType!=OT_DATABASE &&
       revokeparm->ObjectType!=OT_ROLE) {
      if (revokeparm->cascade) 
         pstm=ConcatStr(pstm, " cascade", &freebytes);
      else
         pstm=ConcatStr(pstm, " restrict", &freebytes);
   }
   if (!pstm) {                               
      *PPstm=pstm;   
      return RES_ERR;
   }
   *PPstm=pstm;         /* return the pointer to the generated SQL stmt.*/
   return RES_SUCCESS;
}
UCHAR *BuildSQLSelect(char **ColList, char *TableName)
/******************************************************************
* Function :                                                      * 
*           Create a select SQL statement from a table name and   *
*           a comlumn list                                        *
* Parameters :                                                    *
*           ColList :                                             * 
*              Pointer to an array of column list.                *
*           TableName :                                           *
*              Table name the select refers to.                   *
*  Returns :                                                      *
*           A pointer to the built SQL statement or NULL.         *
******************************************************************/
{
   char *pstm=NULL;
   BOOL One_Found=FALSE;
   int freebytes=SEGMENT_TEXT_SIZE;

   /* alloc memory for the statement to build */

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm)
      return NULL;

   /* generate the column list */

   while (ColList) {
      if (!One_Found) {
         pstm=ConcatStr(pstm, "select ", &freebytes);
         One_Found=TRUE;
      }
      else
         pstm=ConcatStr(pstm, ", ", &freebytes);
      pstm=ConcatStr(pstm, *ColList, &freebytes);
      if (!pstm)
         return pstm;
      ColList++;
   }
   if (One_Found) {     /* no column generated : error */ 
      ESL_FreeMem(pstm);
      return NULL;
   }
   pstm=ConcatStr(pstm, " from ", &freebytes);
   pstm=ConcatStr(pstm, TableName, &freebytes);
   return pstm; /* returns the pointer or null if ConacStr failed */
}
int GenAlterLocation(LPLOCATIONPARAMS pLocationParams)
{
   UCHAR bufrequest[400], bufcomma[2];
   struct int2string *usagetype;
   struct int2string usagetypes[]= {
       { LOCATIONDATABASE  ,"database"   },
       { LOCATIONWORK      ,"work"       },
       { LOCATIONJOURNAL   ,"journal"    },
       { LOCATIONCHECKPOINT,"checkpoint" },
       { LOCATIONDUMP      ,"dump"       },
       { LOCATIONALL       ,"(all)"      },
       { 0                 ,(char *)0    }
   };
   if (!*(pLocationParams->objectname)) 
      return RES_ERR;

   wsprintf(bufrequest,"alter location %s ", (LPUCHAR)QuoteIfNeeded(pLocationParams->objectname));


   pLocationParams->bLocationUsage[LOCATIONNOUSAGE]=TRUE;
   for (usagetype=usagetypes;usagetype->pchar;usagetype++) {
     if (pLocationParams->bLocationUsage[usagetype->item]) 
       pLocationParams->bLocationUsage[LOCATIONNOUSAGE]=FALSE;
   }

   if (pLocationParams->bLocationUsage[LOCATIONNOUSAGE])
     x_strcat(bufrequest," with nousage");
   else {
     x_strcat(bufrequest," with usage = (");
     bufcomma[1] = bufcomma[0] = '\0';
     for (usagetype=usagetypes;usagetype->pchar;usagetype++) {
       if (pLocationParams->bLocationUsage[usagetype->item]) {
         x_strcat(bufrequest,bufcomma);
         x_strcat(bufrequest,usagetype->pchar);
         bufcomma[0]=',';
       }
     }
     x_strcat(bufrequest,")");
   }

   return ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
}

int GenAlterUser(LPUSERPARAMS usrparm)
{
   int iret;
   UCHAR *pstm; 
   BOOL one_priv=FALSE, bAllDefPriv, bNoDefPriv;

   static char STR_ALTERUSR[]="alter user ";
   static struct _SecAud {
       UCHAR *Str;
   } SecAud[USERSECAUDITS];

   static struct _Prv {
       UCHAR *Str;
   } Priv[USERPRIVILEGES];
   unsigned int i, freebytes=SEGMENT_TEXT_SIZE;

   SecAud[USERALLEVENT      ].Str=" all_events";
   SecAud[USERDEFAULTEVENT  ].Str=" default_events";
   SecAud[USERQUERYTEXTEVENT].Str=" query_text";

   Priv[USERCREATEDBPRIV].Str=" createdb";
   Priv[USERTRACEPRIV   ].Str=" trace";
   Priv[USERSECURITYPRIV].Str=" security";
   Priv[USEROPERATORPRIV].Str=" operator";
   Priv[USERMAINTLOCPRIV].Str=" maintain_locations";
   Priv[USERAUDITALLPRIV].Str=" auditor";
   Priv[USERMAINTAUDITPRIV].Str=" maintain_audit";
   Priv[USERMAINTUSERPRIV ].Str=" maintain_users";

   Priv[USERWRITEDOWN ].Str= " write_down";
   Priv[USERWRITEFIXED].Str= " write_fixed";
   Priv[USERWRITEUP].Str= " write_up";
   Priv[USERINSERTDOWN].Str= " insert_down";
   Priv[USERINSERTUP].Str= " insert_up";
   Priv[USERSESSIONSECURITYLABEL].Str= " session_security_label";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

// --- Begin of fixed part generation : The return of ConcatStr is checked   
// --- only once since ConcatStr does nothing if it first param is NULL

   pstm=ConcatStr(pstm, STR_ALTERUSR, &freebytes);
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(usrparm->ObjectName), &freebytes);

   if (*(usrparm->DefaultGroup)) {
      pstm=ConcatStr(pstm, STR_WITHGRP, &freebytes);
      pstm=ConcatStr(pstm, usrparm->DefaultGroup, &freebytes); // group_name
   }
   else 
      pstm=ConcatStr(pstm, STR_NOGRP, &freebytes);         // nogroup

   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

   // expire date

   if (usrparm->ExpireDate[0]) {
      pstm=ConcatStr(pstm, STR_EXPIREDATE, &freebytes);
      pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
      pstm=ConcatStr(pstm, usrparm->ExpireDate, &freebytes);
      pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
   }
   else
      pstm=ConcatStr(pstm, STR_NOEXPIREDATE, &freebytes);

   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

   //   Profile Name

   if (usrparm->szProfileName[0]) {
      pstm=ConcatStr(pstm, STR_PROFILE, &freebytes);
      pstm=ConcatStr(pstm, usrparm->szProfileName, &freebytes);
   }
   else
      pstm=ConcatStr(pstm, STR_NOPROFILE, &freebytes);

   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

   // Password

   if (usrparm->szPassword[0]) {
      pstm=ConcatStr(pstm, STR_PASSWORD, &freebytes);
      pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
      pstm=ConcatStr(pstm, usrparm->szPassword, &freebytes);
      pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
	  pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
   }
   else if (usrparm->bExtrnlPassword) { // External Password ?
         pstm=ConcatStr(pstm, STR_EXTRNLPASSWORD, &freebytes);
		 pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
   }
   else if (usrparm->bRemovePwd) {
         pstm=ConcatStr(pstm, STR_NOPASSWORD, &freebytes);
	     pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
   }

   
   // Limiting security label

   if (usrparm->lpLimitSecLabels) {
      if (*(usrparm->lpLimitSecLabels)) {
         pstm=ConcatStr(pstm, STR_SECLABEL, &freebytes);
         pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
         pstm=ConcatStr(pstm, usrparm->lpLimitSecLabels, &freebytes);
         pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
         pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
       }
   }
   // else {  // Requires B1 secure
   //    pstm=ConcatStr(pstm, STR_NOSECLABEL, &freebytes);
   //    pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
   // }

   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

// --- fixed part is done . Start with the variing part.

   // now concatenates the privileges if any

   one_priv=FALSE;

   for (i=0; i<USERPRIVILEGES; i++) {
      if (usrparm->Privilege[i]) {      // one privilege checked.
         if (one_priv)                  // not first one.
            pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
         else { 
            one_priv=TRUE;
            pstm=ConcatStr(pstm, STR_PRIV, &freebytes);   //with privilege=   
         }
         pstm=ConcatStr(pstm,Priv[i].Str, &freebytes); // copy privilege 
         if (!pstm) {
            myerror(ERR_ALLOCMEM);
            return RES_ERR;
         }
      }
   }

   if (one_priv)        // at least on priv were found
      pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);
   else
      pstm=ConcatStr(pstm,STR_NOPRIV, &freebytes);
   
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   // Genrerates the default privileges clause

   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

   bAllDefPriv=TRUE;
   bNoDefPriv=TRUE;

   for (i=0; i<USERPRIVILEGES; i++) { // determines if ALL, NO or some
      if (usrparm->bDefaultPrivilege[i]) 
         bNoDefPriv=FALSE;
      else
         bAllDefPriv=FALSE;
   }

   one_priv=FALSE;
   if (bNoDefPriv)
      pstm=ConcatStr(pstm,STR_NODEFAULTPRIV, &freebytes);
   else if (bAllDefPriv)
      pstm=ConcatStr(pstm,STR_DEFAULTALLPRIV, &freebytes);
   else {
      for (i=0; i<USERPRIVILEGES; i++) {
         if (usrparm->bDefaultPrivilege[i]) {     // one privilege checked.
            if (one_priv)                         // not first one.
               pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
            else { 
               one_priv=TRUE;
               pstm=ConcatStr(pstm, STR_DEFAULTPRIV, &freebytes); 
            }
            pstm=ConcatStr(pstm,Priv[i].Str, &freebytes); // copy privilege 
            if (!pstm) {
               myerror(ERR_ALLOCMEM);
               return RES_ERR;
            }
         }
      }
   }

   if (one_priv)        // at least on priv were found
      pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);

   // now generate the security audit if appropriate

   one_priv=FALSE;

   for (i=0; i<USERSECAUDITS; i++) {
      if (usrparm->bSecAudit[i]) {
         if (one_priv)            
            pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
         else { 
            one_priv=TRUE;
            pstm=ConcatStr(pstm, STR_SECAUDIT, &freebytes);
         }
         pstm=ConcatStr(pstm,SecAud[i].Str, &freebytes); 
         if (!pstm) {
            myerror(ERR_ALLOCMEM);
            return RES_ERR;
         }
      }
   }

   if (one_priv)        // at least on priv were found
      pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);
   
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }


   iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL);
   ESL_FreeMem(pstm);
   return iret;

}

int GenAlterProfile(LPPROFILEPARAMS profparm)
{
   int iret;
   UCHAR *pstm; 
   BOOL one_priv=FALSE, bAllDefPriv, bNoDefPriv;

   static char STR_ALTERUSR[]="alter profile ";
   static struct _SecAud {
       UCHAR *Str;
   } SecAud[USERSECAUDITS];

   static struct _Prv {
       UCHAR *Str;
   } Priv[USERPRIVILEGES];
   unsigned int i, freebytes=SEGMENT_TEXT_SIZE;

   SecAud[USERALLEVENT      ].Str=" all_events";
   SecAud[USERDEFAULTEVENT  ].Str=" default_events";
   SecAud[USERQUERYTEXTEVENT].Str=" query_text";

   Priv[USERCREATEDBPRIV].Str=" createdb";
   Priv[USERTRACEPRIV   ].Str=" trace";
   Priv[USERSECURITYPRIV].Str=" security";
   Priv[USEROPERATORPRIV].Str=" operator";
   Priv[USERMAINTLOCPRIV].Str=" maintain_locations";
   Priv[USERAUDITALLPRIV].Str=" auditor";
   Priv[USERMAINTAUDITPRIV].Str=" maintain_audit";
   Priv[USERMAINTUSERPRIV ].Str=" maintain_users";

   Priv[USERWRITEDOWN ].Str= " write_down";
   Priv[USERWRITEFIXED].Str= " write_fixed";
   Priv[USERWRITEUP].Str= " write_up";
   Priv[USERINSERTDOWN].Str= " insert_down";
   Priv[USERINSERTUP].Str= " insert_up";
   Priv[USERSESSIONSECURITYLABEL].Str= " session_security_label";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

// --- Begin of fixed part generation : The return of ConcatStr is checked   
// --- only once since ConcatStr does nothing if it first param is NULL
   if (profparm->bDefProfile) {
      pstm=ConcatStr(pstm, "alter default profile " , &freebytes);
   }
   else {
      pstm=ConcatStr(pstm, STR_ALTERUSR, &freebytes);
      pstm=ConcatStr(pstm, (LPTSTR)QuoteIfNeeded(profparm->ObjectName), &freebytes);
   }
   if (*(profparm->DefaultGroup)) {
      pstm=ConcatStr(pstm, STR_WITHGRP, &freebytes);
      pstm=ConcatStr(pstm, (LPTSTR)QuoteIfNeeded(profparm->DefaultGroup), &freebytes); // group_name
   }
   else 
      pstm=ConcatStr(pstm, STR_NOGRP, &freebytes);         // nogroup

   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

   // expire date

   if (profparm->ExpireDate[0]) {
      pstm=ConcatStr(pstm, STR_EXPIREDATE, &freebytes);
      pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
      pstm=ConcatStr(pstm, profparm->ExpireDate, &freebytes);
      pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
   }
   else
      pstm=ConcatStr(pstm, STR_NOEXPIREDATE, &freebytes);

   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

   // Limiting security label

   if (profparm->lpLimitSecLabels) {
      if (*(profparm->lpLimitSecLabels)) {
         pstm=ConcatStr(pstm, STR_SECLABEL, &freebytes);
         pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
         pstm=ConcatStr(pstm, profparm->lpLimitSecLabels, &freebytes);
         pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
         pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
       }
   }
   // else {  // Requires B1 secure
   //    pstm=ConcatStr(pstm, STR_NOSECLABEL, &freebytes);
   //    pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
   // }

   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

// --- fixed part is done . Start with the variing part.

   // now concatenates the privileges if any

   one_priv=FALSE;

   for (i=0; i<USERPRIVILEGES; i++) {
      if (profparm->Privilege[i]) {      // one privilege checked.
         if (one_priv)                  // not first one.
            pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
         else { 
            one_priv=TRUE;
            pstm=ConcatStr(pstm, STR_PRIV, &freebytes);   //with privilege=   
         }
         pstm=ConcatStr(pstm,Priv[i].Str, &freebytes); // copy privilege 
         if (!pstm) {
            myerror(ERR_ALLOCMEM);
            return RES_ERR;
         }
      }
   }

   if (one_priv)        // at least on priv were found
      pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);
   else
      pstm=ConcatStr(pstm,STR_NOPRIV, &freebytes);
   
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   // Genrerates the default privileges clause

   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

   bAllDefPriv=TRUE;
   bNoDefPriv=TRUE;

   for (i=0; i<USERPRIVILEGES; i++) { // determines if ALL, NO or some
      if (profparm->bDefaultPrivilege[i]) 
         bNoDefPriv=FALSE;
      else
         bAllDefPriv=FALSE;
   }

   one_priv=FALSE;
   if (bNoDefPriv)
      pstm=ConcatStr(pstm,STR_NODEFAULTPRIV, &freebytes);
   else if (bAllDefPriv)
      pstm=ConcatStr(pstm,STR_DEFAULTALLPRIV, &freebytes);
   else {
      for (i=0; i<USERPRIVILEGES; i++) {
         if (profparm->bDefaultPrivilege[i]) {     // one privilege checked.
            if (one_priv)                         // not first one.
               pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
            else { 
               one_priv=TRUE;
               pstm=ConcatStr(pstm, STR_DEFAULTPRIV, &freebytes); 
            }
            pstm=ConcatStr(pstm,Priv[i].Str, &freebytes); // copy privilege 
            if (!pstm) {
               myerror(ERR_ALLOCMEM);
               return RES_ERR;
            }
         }
      }
   }

   if (one_priv)        // at least on priv were found
      pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);

   // now generate the security audit if appropriate

   one_priv=FALSE;

   for (i=0; i<USERSECAUDITS; i++) {
      if (profparm->bSecAudit[i]) {
         if (one_priv)            
            pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
         else { 
            one_priv=TRUE;
            pstm=ConcatStr(pstm, STR_SECAUDIT, &freebytes);
         }
         pstm=ConcatStr(pstm,SecAud[i].Str, &freebytes); 
         if (!pstm) {
            myerror(ERR_ALLOCMEM);
            return RES_ERR;
         }
      }
   }

   if (one_priv)        // at least on priv were found
      pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);
   
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }


   iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL);
   ESL_FreeMem(pstm);
   return iret;

}

int GenAlterGroup(LPGROUPPARAMS oldgrpparm,LPGROUPPARAMS grpparm)
   
{
   UCHAR *pstm; 
   unsigned freebytes=SEGMENT_TEXT_SIZE;
   static char STR_ALTERGRP[]="alter group ";
   static char STR_ADDUSER[]=" add users (";
   static char STR_DROPUSR[]=" drop users (";
   BOOL one_usr=FALSE;
   int iret;
   LPCHECKEDOBJECTS oldpchkobj;
   LPCHECKEDOBJECTS newpchkobj;

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   iret = RES_SUCCESS;

   // First remove all the users from the group, this is done by 
   // extracting the users from oldparm not in grpparm

   fstrncpy(pstm, STR_ALTERGRP, x_strlen(STR_ALTERGRP)+1);  // alter group
   freebytes-=x_strlen(STR_ALTERGRP);

   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(grpparm->ObjectName), &freebytes); // group_name


   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   
   pstm=ConcatStr(pstm, STR_DROPUSR, &freebytes); // start usr list
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   
   // generate the list of users from oldparm now longer in the
   // latest version

   one_usr=FALSE;
   oldpchkobj=oldgrpparm->lpfirstuser;

   while (oldpchkobj) {

      newpchkobj=grpparm->lpfirstuser;

      while (newpchkobj) {
         if (!x_strcmp(oldpchkobj->dbname,newpchkobj->dbname))
            break; // user still in group
         newpchkobj=newpchkobj->pnext;
      }

      if (!newpchkobj) { // user has been removed from the group
         if (!one_usr) 
            one_usr=TRUE;
         else
            pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
         pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(oldpchkobj->dbname), &freebytes); 
         if (!pstm) {
            myerror(ERR_ALLOCMEM);
            return RES_ERR;
         }
      }
      oldpchkobj=oldpchkobj->pnext;
   }

   if (one_usr) {
      pstm=ConcatStr(pstm, STR_ENDSTM, &freebytes); 
      if (!pstm) {
         myerror(ERR_ALLOCMEM);
         return RES_ERR;
      }

      iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL); // drop users fro grp
   
      if (iret!=RES_SUCCESS) {
         ESL_FreeMem(pstm);
         return iret;
      }
   }

   freebytes=SEGMENT_TEXT_SIZE;  // reuse the same pstm buffer

   // Now add all the new users to the group, this is done by 
   // extracting the users from grpparm not in oldgrpparm

   fstrncpy(pstm, STR_ALTERGRP, x_strlen(STR_ALTERGRP)+1);  // alter group
   freebytes-=x_strlen(STR_ALTERGRP);

   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(grpparm->ObjectName), &freebytes); // group_name


   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   
   pstm=ConcatStr(pstm, STR_ADDUSER, &freebytes); // start usr list
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   
   // generate the list of users from newparm not in the
   // older version

   one_usr=FALSE;
   newpchkobj=grpparm->lpfirstuser;

   while (newpchkobj) {
      oldpchkobj=oldgrpparm->lpfirstuser;

      while (oldpchkobj) {
         if (!x_strcmp(newpchkobj->dbname,oldpchkobj->dbname))
            break; // user still in group
         oldpchkobj=oldpchkobj->pnext;
      }

      if (!oldpchkobj) { // user has been removed from the group
         if (!one_usr) 
            one_usr=TRUE;
         else
            pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
         pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(newpchkobj->dbname), &freebytes); 
         if (!pstm) {
            myerror(ERR_ALLOCMEM);
            return RES_ERR;
         }
      }
      newpchkobj=newpchkobj->pnext;
   }

   if (one_usr) {
      pstm=ConcatStr(pstm, STR_ENDSTM, &freebytes); 
      if (!pstm) {
         myerror(ERR_ALLOCMEM);
         return RES_ERR;
      }
      iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL); // drop users fro grp
   
      if (iret!=RES_SUCCESS) {
         ESL_FreeMem(pstm);
         pstm = NULL;      // Vut Adds 13-Sep-1995
         return iret;
      }
   }
   ESL_FreeMem(pstm);      // Vut Adds 13-Sep-1995
   return iret;

}
int GenAlterRole(LPROLEPARAMS roleparm)
{
   int iret;
   UCHAR *pstm; 
   BOOL one_priv=FALSE, bAllDefPriv, bNoDefPriv;

   static char STR_ALTERROLE[]="alter role ";
   static struct _SecAud {
       UCHAR *Str;
   } SecAud[USERSECAUDITS];

   static struct _Prv {
       UCHAR *Str;
   } Priv[USERPRIVILEGES];
   unsigned int i, freebytes=SEGMENT_TEXT_SIZE;

   SecAud[USERALLEVENT      ].Str=" all_events";
   SecAud[USERDEFAULTEVENT  ].Str=" default_events";
   SecAud[USERQUERYTEXTEVENT].Str=" query_text";

   Priv[USERCREATEDBPRIV].Str=" createdb";
   Priv[USERTRACEPRIV   ].Str=" trace";
   Priv[USERSECURITYPRIV].Str=" security";
   Priv[USEROPERATORPRIV].Str=" operator";
   Priv[USERMAINTLOCPRIV].Str=" maintain_locations";
   Priv[USERAUDITALLPRIV].Str=" auditor";
   Priv[USERMAINTAUDITPRIV].Str=" maintain_audit";
   Priv[USERMAINTUSERPRIV ].Str=" maintain_users";

   Priv[USERWRITEDOWN ].Str= " write_down";
   Priv[USERWRITEFIXED].Str= " write_fixed";
   Priv[USERWRITEUP].Str= " write_up";
   Priv[USERINSERTDOWN].Str= " insert_down";
   Priv[USERINSERTUP].Str= " insert_up";
   Priv[USERSESSIONSECURITYLABEL].Str= " session_security_label";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

// --- Begin of fixed part generation : The return of ConcatStr is checked   
// --- only once since ConcatStr does nothing if it first param is NULL

   pstm=ConcatStr(pstm, STR_ALTERROLE, &freebytes);
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(roleparm->ObjectName), &freebytes);
   pstm=ConcatStr(pstm, STR_WITH, &freebytes);

   // Password

   if (roleparm->szPassword[0]) {
      pstm=ConcatStr(pstm, STR_PASSWORD, &freebytes);
      pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
      pstm=ConcatStr(pstm, roleparm->szPassword, &freebytes);
      pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
   }
   else {
      if (roleparm->bExtrnlPassword) { // External Password ?
         pstm=ConcatStr(pstm, STR_EXTRNLPASSWORD, &freebytes);
        // pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
      }
      else
         pstm=ConcatStr(pstm, STR_NOPASSWORD, &freebytes);
   }

   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

   // Limiting security label

   if (roleparm->lpLimitSecLabels) {
      if (*(roleparm->lpLimitSecLabels)) {
         pstm=ConcatStr(pstm, STR_SECLABEL, &freebytes);
         pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
         pstm=ConcatStr(pstm, roleparm->lpLimitSecLabels, &freebytes);
         pstm=ConcatStr(pstm, STR_QUOTE, &freebytes);
         pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
       }
   }
   // else {  // Requires B1 secure
   //    pstm=ConcatStr(pstm, STR_NOSECLABEL, &freebytes);
   //    pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
   // }

   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

// --- fixed part is done . Start with the variing part.

   // now concatenates the privileges if any

   one_priv=FALSE;

   for (i=0; i<USERPRIVILEGES; i++) {
      if (roleparm->Privilege[i]) {      // one privilege checked.
         if (one_priv)                  // not first one.
            pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
         else { 
            one_priv=TRUE;
            pstm=ConcatStr(pstm, STR_PRIV, &freebytes);   //with privilege=   
         }
         pstm=ConcatStr(pstm,Priv[i].Str, &freebytes); // copy privilege 
         if (!pstm) {
            myerror(ERR_ALLOCMEM);
            return RES_ERR;
         }
      }
   }

   if (one_priv)        // at least on priv were found
      pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);
   else
      pstm=ConcatStr(pstm,STR_NOPRIV, &freebytes);
   
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   // Genrerates the default privileges clause

   pstm=ConcatStr(pstm, STR_COMMA, &freebytes);

   bAllDefPriv=TRUE;
   bNoDefPriv=TRUE;

   for (i=0; i<USERPRIVILEGES; i++) { // determines if ALL, NO or some
      if (roleparm->bDefaultPrivilege[i]) 
         bNoDefPriv=FALSE;
      else
         bAllDefPriv=FALSE;
   }

   one_priv=FALSE;
   if (bNoDefPriv)
      pstm=ConcatStr(pstm,STR_NODEFAULTPRIV, &freebytes);
   else if (bAllDefPriv)
      pstm=ConcatStr(pstm,STR_DEFAULTALLPRIV, &freebytes);
   else {
      for (i=0; i<USERPRIVILEGES; i++) {
         if (roleparm->bDefaultPrivilege[i]) {     // one privilege checked.
            if (one_priv)                         // not first one.
               pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
            else { 
               one_priv=TRUE;
               pstm=ConcatStr(pstm, STR_DEFAULTPRIV, &freebytes); 
            }
            pstm=ConcatStr(pstm,Priv[i].Str, &freebytes); // copy privilege 
            if (!pstm) {
               myerror(ERR_ALLOCMEM);
               return RES_ERR;
            }
         }
      }
   }

   if (one_priv)        // at least on priv were found
      pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);

   // now generate the security audit if appropriate

   one_priv=FALSE;

   for (i=0; i<USERSECAUDITS; i++) {
      if (roleparm->bSecAudit[i]) {
         if (one_priv)            
            pstm=ConcatStr(pstm, STR_COMMA, &freebytes);
         else { 
            one_priv=TRUE;
            pstm=ConcatStr(pstm, STR_SECAUDIT, &freebytes);
         }
         pstm=ConcatStr(pstm,SecAud[i].Str, &freebytes); 
         if (!pstm) {
            myerror(ERR_ALLOCMEM);
            return RES_ERR;
         }
      }
   }

   if (one_priv)        // at least on priv were found
      pstm=ConcatStr(pstm,STR_ENDSTM, &freebytes);
   
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }


   iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL);
   ESL_FreeMem(pstm);
   return iret;

}

int GenAlterSequence(LPSEQUENCEPARAMS sequenceparm)
{
   UCHAR *pstm; 
   int iret;
   unsigned freebytes=SEGMENT_TEXT_SIZE;
   static char STR_ALTERSEQ[]    = " alter sequence ";
   static char STR_STARTWITH[]   = " restart with ";
   static char STR_INCREMENTBY[] = " increment by ";
   static char STR_MINVALUE[]    = " minvalue ";
   static char STR_MAXVALUE[]    = " maxvalue ";
   static char STR_CACHE[]       = " cache ";
   static char STR_NOCACHE[]     = " nocache ";
   static char STR_CYCLE[]       = " cycle ";
   static char STR_NOCYCLE[]       = " nocycle ";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm)
      return RES_ERR;

   fstrncpy(pstm, STR_ALTERSEQ, x_strlen(STR_ALTERSEQ)+1);  /* alter sequence */
   freebytes-=x_strlen(STR_ALTERSEQ);

   if (*(sequenceparm->objectowner)) {
      pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(sequenceparm->objectowner), &freebytes); /* seq_owner */
      pstm=ConcatStr(pstm, ".", &freebytes);
   }
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(sequenceparm->objectname), &freebytes); /* seq_name */

   if (*(sequenceparm->szStartWith))
   {
      pstm=ConcatStr(pstm, STR_STARTWITH, &freebytes);
      pstm=ConcatStr(pstm, sequenceparm->szStartWith, &freebytes);
   }

   if (*(sequenceparm->szIncrementBy))
   {
      pstm=ConcatStr(pstm, STR_INCREMENTBY, &freebytes);
      pstm=ConcatStr(pstm, sequenceparm->szIncrementBy, &freebytes);
   }

   if (*(sequenceparm->szMinValue))
   {
      pstm=ConcatStr(pstm, STR_MINVALUE, &freebytes);
      pstm=ConcatStr(pstm, sequenceparm->szMinValue, &freebytes);
   }

   if (*(sequenceparm->szMaxValue))
   {
      pstm=ConcatStr(pstm, STR_MAXVALUE, &freebytes);
      pstm=ConcatStr(pstm, sequenceparm->szMaxValue, &freebytes);
   }

   if (*(sequenceparm->szCacheSize))
   {
      pstm=ConcatStr(pstm, STR_CACHE, &freebytes);
      pstm=ConcatStr(pstm, sequenceparm->szCacheSize, &freebytes);
   }
   else
      pstm=ConcatStr(pstm, STR_NOCACHE, &freebytes);


   if (sequenceparm->bCycle)
      pstm=ConcatStr(pstm, STR_CYCLE, &freebytes);
   else
      pstm=ConcatStr(pstm, STR_NOCYCLE, &freebytes);

   if (!pstm)
      return RES_ERR;

   iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL);
   ESL_FreeMem(pstm);
   return iret;
}



int ModifyReloc(LPSTORAGEPARAMS lpStorageParams)
{
   UCHAR *pstm;
   UCHAR *Newpstm;
   int freebytes=SEGMENT_TEXT_SIZE, iret;
   static char STR_TORELOC[]=" to relocate with oldlocation = (";
   static char STR_NEWLOC[]=", newlocation = (";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   pstm=ConcatStr(pstm, STR_MODIFY, &freebytes);
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(lpStorageParams->objectname), &freebytes);
   pstm=ConcatStr(pstm, STR_TORELOC, &freebytes);

   /* old location list */

   if (lpStorageParams->lpLocations) {       
      Newpstm=GenObjList(pstm,lpStorageParams->lpLocations, /* locations */
          &freebytes, TRUE, FALSE);
      if (!pstm) {   
         myerror(ERR_ALLOCMEM);
         return RES_ERR;
      }
      pstm=Newpstm;
   }
   else {
      ESL_FreeMem(pstm);
      return RES_ERR;
   }

   pstm=ConcatStr(pstm, STR_ENDSTM, &freebytes);

   /* New location list */

   if (lpStorageParams->lpNewLocations) {       
      pstm=ConcatStr(pstm, STR_NEWLOC, &freebytes);
      Newpstm=GenObjList(pstm,lpStorageParams->lpNewLocations, /* locations */
          &freebytes, TRUE, FALSE);
      if (!Newpstm) {   
         myerror(ERR_ALLOCMEM);
         return RES_ERR;
      }
      pstm=Newpstm;
   }
   else {
      ESL_FreeMem(pstm);
      return RES_ERR;
   }
   pstm=ConcatStr(pstm, STR_ENDSTM, &freebytes);
   if (!pstm) {   
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL);
   ESL_FreeMem(pstm);
   return iret;
}
int ModifyReorg(LPSTORAGEPARAMS lpStorageParams)
{
   UCHAR *pstm;
   UCHAR *Newpstm;
   int freebytes=SEGMENT_TEXT_SIZE, iret;
   static char STR_TOREORG[]=" to reorganize with location = (";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   pstm=ConcatStr(pstm, STR_MODIFY, &freebytes);
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(lpStorageParams->objectname), &freebytes);
   pstm=ConcatStr(pstm, STR_TOREORG, &freebytes);

   /* location list */

   if (lpStorageParams->lpNewLocations) {       
      Newpstm=GenObjList(pstm,lpStorageParams->lpNewLocations, /* locations */
          &freebytes, TRUE, FALSE);
      if (!pstm) {   
         myerror(ERR_ALLOCMEM);
         return RES_ERR;
      }
   }
   else {
      ESL_FreeMem(pstm);
      return RES_ERR;
   }

   pstm=ConcatStr(pstm, STR_ENDSTM, &freebytes);
   if (!pstm) {   
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL);
   ESL_FreeMem(pstm);
   return iret;
}

int ModifyTrunc(LPSTORAGEPARAMS lpStorageParams)
{
   UCHAR *pstm;
   int freebytes=SEGMENT_TEXT_SIZE, iret;
   static char STR_TOTRUNC[]=" to truncated";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   pstm=ConcatStr(pstm, STR_MODIFY, &freebytes);
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(lpStorageParams->objectname), &freebytes);
   pstm=ConcatStr(pstm, STR_TOTRUNC, &freebytes);

   if (!pstm) {   
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL);
   ESL_FreeMem(pstm);
   return iret;
}

int ModifyReadOnly(LPSTORAGEPARAMS lpStorageParams)
{
   UCHAR *pstm;
   int freebytes=SEGMENT_TEXT_SIZE, iret;
   static char STR_TOREADONLY[]=" to readonly";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   pstm=ConcatStr(pstm, STR_MODIFY, &freebytes);
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(lpStorageParams->objectname), &freebytes);
   pstm=ConcatStr(pstm, STR_TOREADONLY, &freebytes);

   if (!pstm) {   
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL);
   ESL_FreeMem(pstm);
   return iret;
}

int ModifyNoReadOnly(LPSTORAGEPARAMS lpStorageParams)
{
   UCHAR *pstm;
   int freebytes=SEGMENT_TEXT_SIZE, iret;
   static char STR_TONOREADONLY[]=" to noreadonly";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   pstm=ConcatStr(pstm, STR_MODIFY, &freebytes);
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(lpStorageParams->objectname), &freebytes);
   pstm=ConcatStr(pstm, STR_TONOREADONLY, &freebytes);

   if (!pstm) {   
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL);
   ESL_FreeMem(pstm);
   return iret;
}

int ModifyRecoveryAllowed(LPSTORAGEPARAMS lpStorageParams)
{
   UCHAR *pstm;
   int freebytes=SEGMENT_TEXT_SIZE, iret;
   static char STR_TORECOVERYALLOWED[]=" to table_recovery_allowed";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   pstm=ConcatStr(pstm, STR_MODIFY, &freebytes);
   pstm=ConcatStr(pstm, lpStorageParams->objectname, &freebytes);
   pstm=ConcatStr(pstm, STR_TORECOVERYALLOWED, &freebytes);

   if (!pstm) {   
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL);
   ESL_FreeMem(pstm);
   return iret;
}

int ModifyRecoveryDisallowed(LPSTORAGEPARAMS lpStorageParams)
{
   UCHAR *pstm;
   int freebytes=SEGMENT_TEXT_SIZE, iret;
   static char STR_TORECOVERYDISALLOWED[]=" to table_recovery_disallowed";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   pstm=ConcatStr(pstm, STR_MODIFY, &freebytes);
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(lpStorageParams->objectname), &freebytes);
   pstm=ConcatStr(pstm, STR_TORECOVERYDISALLOWED, &freebytes);

   if (!pstm) {   
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL);
   ESL_FreeMem(pstm);
   return iret;
}
int ModifyLogConsistent(LPSTORAGEPARAMS lpStorageParams)
{
   UCHAR *pstm;
   int freebytes=SEGMENT_TEXT_SIZE, iret;
   static char STR_TOLOGCONSISTENT[]=" to log_consistent";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   pstm=ConcatStr(pstm, STR_MODIFY, &freebytes);
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(lpStorageParams->objectname), &freebytes);
   pstm=ConcatStr(pstm, STR_TOLOGCONSISTENT, &freebytes);

   if (!pstm) {   
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL);
   ESL_FreeMem(pstm);
   return iret;
}

int ModifyLogInconsistent(LPSTORAGEPARAMS lpStorageParams)
{
   UCHAR *pstm;
   int freebytes=SEGMENT_TEXT_SIZE, iret;
   static char STR_TOLOGINCONSISTENT[]=" to log_inconsistent";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   pstm=ConcatStr(pstm, STR_MODIFY, &freebytes);
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(lpStorageParams->objectname), &freebytes);
   pstm=ConcatStr(pstm, STR_TOLOGINCONSISTENT, &freebytes);

   if (!pstm) {   
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL);
   ESL_FreeMem(pstm);
   return iret;
}
int ModifyPhysInconsistent(LPSTORAGEPARAMS lpStorageParams)
{
   UCHAR *pstm;
   int freebytes=SEGMENT_TEXT_SIZE, iret;
   static char STR_TOPHYSINCONSISTENT[]=" to phys_inconsistent";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   pstm=ConcatStr(pstm, STR_MODIFY, &freebytes);
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(lpStorageParams->objectname), &freebytes);
   pstm=ConcatStr(pstm, STR_TOPHYSINCONSISTENT, &freebytes);

   if (!pstm) {   
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL);
   ESL_FreeMem(pstm);
   return iret;
}

int ModifyPhysconsistent(LPSTORAGEPARAMS lpStorageParams)
{
   UCHAR *pstm;
   int freebytes=SEGMENT_TEXT_SIZE, iret;
   static char STR_TOPHYSCONSISTENT[]=" to phys_consistent";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   pstm=ConcatStr(pstm, STR_MODIFY, &freebytes);
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(lpStorageParams->objectname), &freebytes);
   pstm=ConcatStr(pstm, STR_TOPHYSCONSISTENT, &freebytes);

   if (!pstm) {   
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL);
   ESL_FreeMem(pstm);
   return iret;
}

int ModifyMerge(LPSTORAGEPARAMS lpStorageParams)
{
   UCHAR *pstm;
   int freebytes=SEGMENT_TEXT_SIZE, iret;
   static char STR_TOMERGE[]=" to merge";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   pstm=ConcatStr(pstm, STR_MODIFY, &freebytes);
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(lpStorageParams->objectname), &freebytes);
   pstm=ConcatStr(pstm, STR_TOMERGE, &freebytes);

   if (!pstm) {   
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL);
   ESL_FreeMem(pstm);
   return iret;
}

int ModifyAddPages(LPSTORAGEPARAMS lpStorageParams)
{
   UCHAR *pstm, ustr[MAXOBJECTNAME];
   int freebytes=SEGMENT_TEXT_SIZE, iret;
   static char STR_TOADDEXTND[]=" to add_extend ";
   static char STR_WITHEXTEND[]=" with extend =";

   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   pstm=ConcatStr(pstm, STR_MODIFY, &freebytes);
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(lpStorageParams->objectname), &freebytes);
   pstm=ConcatStr(pstm, STR_TOADDEXTND, &freebytes);

   if (lpStorageParams->lExtend>0) {
      pstm=ConcatStr(pstm, STR_WITHEXTEND, &freebytes);
      my_itoa((int) lpStorageParams->lExtend, ustr, 10);
      pstm=ConcatStr(pstm, ustr, &freebytes);
   }

   if (!pstm) {   
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }

   iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL);
   ESL_FreeMem(pstm);
   return iret;
}
int ModifyChgStorage(LPSTORAGEPARAMS lpStorageParams)
{
   UCHAR *pstm, *Newpstm, *lpLstCol, Uwork[15];
   int freebytes=SEGMENT_TEXT_SIZE, freebytes2=SEGMENT_TEXT_SIZE, iret;
   static char STR_TOADDEXTND[]=" to add_extend ";
   static char STR_WITHEXTEND[]=" with extend =";
   static struct _ObjectStorage {
      int nStructure;
      char *cStructure;
   } ObjectStorage [] = {
      IDX_BTREE,"btree",
      IDX_ISAM ,"isam",
      IDX_HASH ,"hash",
      IDX_HEAP ,"heap",
      IDX_HEAPSORT ,"heapsort",
      0 ,NULL};
   struct _ObjectStorage *lpObjectStorage = &ObjectStorage[0]; 


   pstm=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!pstm) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   pstm=ConcatStr(pstm, STR_MODIFY, &freebytes);

   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(lpStorageParams->objectowner), &freebytes);
   pstm=ConcatStr(pstm, ".", &freebytes);
   pstm=ConcatStr(pstm, (LPUCHAR)QuoteIfNeeded(lpStorageParams->objectname), &freebytes);

   pstm=ConcatStr(pstm, STR_TO, &freebytes);

/* ----  Look for the storage structure, return RES_ERR if unknown ---- */ 
  
   while (lpObjectStorage->cStructure) {
      if (lpObjectStorage->nStructure==lpStorageParams->nStructure) 
         break;
      lpObjectStorage++;
   }
   if (!lpObjectStorage->cStructure) {  /* the storage structure is unkown */
      ESL_FreeMem(pstm);
      return RES_ERR;
   }
   pstm=ConcatStr(pstm,lpObjectStorage->cStructure, &freebytes);
   if (!pstm) {         /* test only once for the above...     */
      ESL_FreeMem(pstm);/* stmt since ConcatStr does ...       */
      return RES_ERR;   /* nothing if a NULL ptr is passed on. */
   }

   /* ---  Unique : Only if isam, btree or hash --- */

   switch (lpStorageParams->nStructure) {
      case IDX_BTREE:
      case IDX_ISAM:
      case IDX_HASH:
      if (lpStorageParams->nUnique != IDX_UNIQUE_NO)
         pstm=ConcatStr(pstm,STR_UNIQUE, &freebytes);
      break;
   }

   if (!pstm) {   
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }


   /* ---- generate the column list  ---- */ 

   lpLstCol=ESL_AllocMem(SEGMENT_TEXT_SIZE);
   if (!lpLstCol) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
   }
   if (lpStorageParams->lpidxCols) {
      lpLstCol=ConcatStr(lpLstCol, STR_ON, &freebytes2);     
      if (!lpLstCol) {
         myerror(ERR_ALLOCMEM);
         return RES_ERR;
      }
      Newpstm=GenSortedObjList(lpLstCol,lpStorageParams->lpidxCols,
                               &freebytes2, FALSE,
                               (lpStorageParams->nStructure == IDX_HEAPSORT)?TRUE:FALSE);
      if (!Newpstm) {
         ESL_FreeMem(pstm);
         ESL_FreeMem(lpLstCol);
         myerror(ERR_ALLOCMEM);
         return RES_ERR;
      }
      lpLstCol=Newpstm;
      pstm=ConcatStr(pstm, lpLstCol, &freebytes);
      if (!pstm) {   
         myerror(ERR_ALLOCMEM);
         return RES_ERR;
      }
   }
   ESL_FreeMem(lpLstCol);

   pstm=ConcatStr(pstm, STR_WITH, &freebytes);

   /* ---- Generate the compression parameter ---- */

   if (lpStorageParams->bKeyCompression || lpStorageParams->bDataCompression || lpStorageParams->bHiDataCompression) {
      pstm=ConcatStr(pstm, " compression=(", &freebytes);
      if (lpStorageParams->bKeyCompression)
         pstm=ConcatStr(pstm, "key, ", &freebytes);
      else
         pstm=ConcatStr(pstm, "nokey, ", &freebytes);
      if (lpStorageParams->bDataCompression)
         pstm=ConcatStr(pstm, "data) ", &freebytes);
      else
      if (lpStorageParams->bHiDataCompression)
         pstm=ConcatStr(pstm, "hidata) ", &freebytes);
      else
         pstm=ConcatStr(pstm, "nodata) ", &freebytes);
   }             
   else
      pstm=ConcatStr(pstm, " nocompression", &freebytes);
      
   if (!pstm) {         /* test only once for the above...      */
      myerror(ERR_ALLOCMEM);
      return RES_ERR;   /* nothing if a NULL ptr is passed on.  */
   }
   /* ---- Generate the persistence parameter ---- */

   if (lpStorageParams->nObjectType==OT_INDEX)
      if (lpStorageParams->bPersistence)
         pstm=ConcatStr(pstm, ", persistence", &freebytes);
      else
         pstm=ConcatStr(pstm, ", nopersistence", &freebytes);
      if (!pstm) {         /* test only once for the above...      */
         myerror(ERR_ALLOCMEM);
         return RES_ERR;   /* nothing if a NULL ptr is passed on.  */
      }

   /* ---- Generate the unique scope if applicable ---- */

   switch (lpStorageParams->nUnique) {
      case IDX_UNIQUE_ROW  :    
         pstm=ConcatStr(pstm, ", unique_scope=row", &freebytes);
         break;
      case IDX_UNIQUE_STATEMENT :
         pstm=ConcatStr(pstm, ", unique_scope=statement", &freebytes);
         break;
   }
    //
    // Open Ingres Version 2.0
    // Generate the page_size. 
    if (GetOIVers () != OIVERS_12)
    {
        char pageSize [24];
        wsprintf (pageSize, ", page_size = %u ", lpStorageParams->uPage_size);
        if (lpStorageParams->uPage_size > 2048)
            pstm=ConcatStr(pstm, pageSize, &freebytes);
    }

   /* ---- Generate the fill factor parameter ---- */

   if (lpStorageParams->nStructure==IDX_ISAM ||
       lpStorageParams->nStructure==IDX_HASH ||
       lpStorageParams->nStructure==IDX_BTREE) {
      pstm=ConcatStr(pstm, ", fillfactor=", &freebytes);
      wsprintf(Uwork,"%d", lpStorageParams->nFillFactor);
      if (!pstm) {         /* test only once for the above...     */
         myerror(ERR_ALLOCMEM);
         return RES_ERR;   /* nothing if a NULL ptr is passed on. */
      }
      pstm=ConcatStr(pstm, Uwork, &freebytes);
   }

   /* ---- Generate the minpages and maxpages parameters for hash only ---- */

   if (lpStorageParams->nStructure==IDX_HASH) {
       pstm=ConcatStr(pstm, ", minpages=", &freebytes);
       wsprintf(Uwork,"%ld", lpStorageParams->nMinPages);
       pstm=ConcatStr(pstm, Uwork, &freebytes);
       pstm=ConcatStr(pstm, ", maxpages=", &freebytes);
       wsprintf(Uwork,"%ld", lpStorageParams->nMaxPages);
       pstm=ConcatStr(pstm, Uwork, &freebytes);
       if (!pstm) {         /* test only once for the above...     */
          myerror(ERR_ALLOCMEM);
          return RES_ERR;   /* nothing if a NULL ptr is passed on. */
          }
   }

   /* --- Generate the leaffill and nonleaffill parameters for btree only --- */

   if (lpStorageParams->nStructure==IDX_BTREE) {
       pstm=ConcatStr(pstm, ", leaffill=", &freebytes);
       wsprintf(Uwork,"%d", lpStorageParams->nLeafFill);
       pstm=ConcatStr(pstm, Uwork, &freebytes);
       pstm=ConcatStr(pstm, ", nonleaffill=", &freebytes);
       wsprintf(Uwork,"%d", lpStorageParams->nNonLeafFill);
       pstm=ConcatStr(pstm, Uwork, &freebytes);
       if (!pstm) {         /* test only once for the above...     */
          myerror(ERR_ALLOCMEM);
          return RES_ERR;   /* nothing if a NULL ptr is passed on. */
          }
   }

   /* ---- Generate the allocation  parameter ---- */

   pstm=ConcatStr(pstm, ", allocation=", &freebytes);
   wsprintf(Uwork,"%ld", lpStorageParams->lAllocation);
   pstm=ConcatStr(pstm, Uwork, &freebytes);
   if (!pstm) {         /* test only once for the above...      */
      myerror(ERR_ALLOCMEM);
      return RES_ERR;   /* nothing if a NULL ptr is passed on.  */
   }

   /* ---- Generate the extend  parameter ---- */

   pstm=ConcatStr(pstm, ", extend=", &freebytes);
   wsprintf(Uwork,"%ld", lpStorageParams->lExtend);
   pstm=ConcatStr(pstm, Uwork, &freebytes);
   if (!pstm) {         /* test only once for the above...      */
      myerror(ERR_ALLOCMEM);
      return RES_ERR;   /* nothing if a NULL ptr is passed on.  */
   }

   /* ---- Generate the concurrent_updates  option ---- */

   if (lpStorageParams->bConcurrentUpdates)
   {
      pstm=ConcatStr(pstm, ", concurrent_updates", &freebytes);
      if (!pstm) {
         myerror(ERR_ALLOCMEM);
         return RES_ERR;
      }
   }

   iret=ExecSQLImm(pstm,FALSE, NULL, NULL, NULL);
   ESL_FreeMem(pstm);
   return iret;
}

// JFS Begin
// added for CDDS

int BuildSQLCDDS    (UCHAR **PPstm, LPREPCDDS lpCdds, LPUCHAR lpNodeName, LPUCHAR lpDBName, int isession)
{
  char         achBufReq[500];
  LPOBJECTLIST lpO;
  int          iret;

  //if (lpCdds->bAlter)
  // {
  // update dd_distribution 
  wsprintf(achBufReq,
           "delete from dd_distribution where dd_routing = %d ",
           lpCdds->cdds);

  iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

  if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
    goto endfunc;
   //}

  for (lpO=lpCdds->distribution;lpO;lpO=lpO->lpNext)
     {
     wsprintf(achBufReq,
           "insert into dd_distribution (localdb,source,target,dd_routing) \
           values ( %d , %d , %d , %d )",
           ((LPDD_DISTRIBUTION)(lpO->lpObject))->localdb,
           ((LPDD_DISTRIBUTION)(lpO->lpObject))->source,
           ((LPDD_DISTRIBUTION)(lpO->lpObject))->target,
           lpCdds->cdds);
     iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

     if (iret!=RES_SUCCESS)
         goto endfunc;
     }

  // update dd_registered_tables

  //if (lpCdds->bAlter)
  // {
  wsprintf(achBufReq,
           "delete from dd_registered_tables where dd_routing = %d",
           lpCdds->cdds);
  iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

  if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
     goto endfunc;
   //}
  for (lpO=lpCdds->registered_tables;lpO;lpO=lpO->lpNext)
     {
     LPOBJECTLIST lpO2;

     if (!((LPDD_REGISTERED_TABLES)(lpO->lpObject))->bStillInCDDS)
       continue;

     // if one table has been copied from another CDDS
     // we have to delete it

//  if (lpCdds->bAlter)
//   {
     wsprintf(achBufReq,
           "delete from dd_registered_tables where tablename = '%s'",
           ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tablename);
     iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

     if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
        goto endfunc;
     //  }
     // Now add the tables



     wsprintf(achBufReq,
"insert into dd_registered_tables \
(tablename,table_no,tables_created,columns_registered,\
rules_created,replication_type,replica_set,\
dd_routing,rule_lookup_table, \
priority_lookup_table \
) values ('%s', 0,'%s','%s','%s', 1, 0, %d,'%s','%s')",

           ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tablename,
//           ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->table_created,
           ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->table_created_ini,
           ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->colums_registered,
//         ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->rules_created,
         ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->rules_created_ini,
           lpCdds->cdds,
           ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->rule_lookup_table,
           ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->priority_lookup_table);

     iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

     if (iret!=RES_SUCCESS)
         goto endfunc;

//  if (lpCdds->bAlter)
//   {
     // delete columns associated with this table
     wsprintf(achBufReq,
           "delete from dd_registered_columns where tablename = '%s'",
           ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tablename);

     iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

     if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
         goto endfunc;
//       }
     // Add columns

     for (lpO2=((LPDD_REGISTERED_TABLES)(lpO->lpObject))->lpCols;lpO2;lpO2=lpO2->lpNext)
       {
        // delete columns associated with this column name
//  if (lpCdds->bAlter)
//   {
        wsprintf(achBufReq,
           "delete from dd_registered_columns where column_name = '%s' and tablename ='%s'",
           ((LPDD_REGISTERED_COLUMNS)(lpO2->lpObject))->columnname,
           ((LPDD_REGISTERED_TABLES)  (lpO->lpObject))->tablename);

        iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

        if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
           goto endfunc;
//       }
        // add them
        wsprintf(achBufReq,
"insert into dd_registered_columns (tablename,column_name,key_column,replicated_column) \
values ('%s','%s','%s','%s')",
           ((LPDD_REGISTERED_TABLES)  (lpO->lpObject))->tablename,
           ((LPDD_REGISTERED_COLUMNS)(lpO2->lpObject))->columnname,
           ((LPDD_REGISTERED_COLUMNS)(lpO2->lpObject))->key_colums,
           ((LPDD_REGISTERED_COLUMNS)(lpO2->lpObject))->rep_colums);

         iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

         if (iret!=RES_SUCCESS)
            goto endfunc;
       }
     }
  if (iret==RES_ENDOFDATA)
   iret=RES_SUCCESS;
  if ( lpCdds->iReplicType == REPLIC_V105) {
      wsprintf (achBufReq,
               "update dd_connections set config_changed = date('now')");
      iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);
      if (iret!=RES_SUCCESS)
         return iret;
  }

  if (iret==RES_SUCCESS) {
     iret= CommitSession(isession); // because of remote commands
     if (iret!=RES_SUCCESS)
       goto endfunc;
     // Create/Drop rules/support tables if needed
     for (lpO=lpCdds->registered_tables;lpO;lpO=lpO->lpNext) {

       if (
           (((LPDD_REGISTERED_TABLES)(lpO->lpObject))->table_created[0]    =='T')&&
           (((LPDD_REGISTERED_TABLES)(lpO->lpObject))->table_created_ini[0]!='T')
          ) {
           DBACreateReplicSup( lpNodeName,
                               lpDBName,
                        ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tablename,
                                  isession);
       }
       if (
           (((LPDD_REGISTERED_TABLES)(lpO->lpObject))->table_created[0]    !='T')&&
           (((LPDD_REGISTERED_TABLES)(lpO->lpObject))->table_created_ini[0]=='T')
          ) {
           UCHAR buf[120];
           BOOL bResult;
           int iret;
           bResult=DBADropReplicSup(
                        ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tablename);
           if (bResult) {
             wsprintf(buf,
                    "update dd_registered_tables set tables_created = ' ' where tablename = '%s'",
                    ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tablename
                     );
             iret=ExecSQLImm(buf,FALSE, NULL, NULL, NULL);
             if (iret!=RES_SUCCESS) {
                iret=ExecSQLImm("rollback",FALSE, NULL, NULL, NULL);
             }
           }
           CommitSession(isession); // because of remote commands
       }
       if (
           (((LPDD_REGISTERED_TABLES)(lpO->lpObject))->rules_created[0]    =='R')&&
           (((LPDD_REGISTERED_TABLES)(lpO->lpObject))->rules_created_ini[0]!='R')
          ) {
           DBACreateReplicRules( lpNodeName,
                                 lpDBName,
                        ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tablename, isession);
       }
       if (
           (((LPDD_REGISTERED_TABLES)(lpO->lpObject))->rules_created[0]    !='R')&&
           (((LPDD_REGISTERED_TABLES)(lpO->lpObject))->rules_created_ini[0]=='R')
          ) {
           UCHAR buf[120];
           BOOL bResult;
           int iret;
           bResult=DBADropReplicRules(
                        ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tablename);
           if (bResult) {
             wsprintf(buf,
                    "update dd_registered_tables set rules_created = ' ' where tablename = '%s'",
                    ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tablename
                     );
             iret=ExecSQLImm(buf,FALSE, NULL, NULL, NULL);
             if (iret!=RES_SUCCESS) {
                iret=ExecSQLImm("rollback",FALSE, NULL, NULL, NULL);
             }
           }
           CommitSession(isession); // because of remote commands
       }
     }
  }

endfunc:
   return iret;
}
//PS 
int BuildSQLCDDSV11 (UCHAR **PPstm, LPREPCDDS lpCdds, LPUCHAR lpNodeName, LPUCHAR lpDBName, int isession)
{
  char         achBufReq[500];
  LPOBJECTLIST lpO;
  int          iret;


  // update dd_cdds
  if (lpCdds->bAlter) {
      wsprintf(achBufReq,
              "delete from dd_cdds r where cdds_no = %d "
              "and not exists (select 1 from dd_input_queue i "
              "where i.cdds_no = r.cdds_no) "
              "and not exists (select 1 from dd_distrib_queue d "
              "where d.cdds_no = r.cdds_no)"
              , lpCdds->cdds);

      iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);
      if (iret == RES_ENDOFDATA)  {
        char buff[200];
        wsprintf(achBufReq,"commit");
        iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);
        //"cannot alter CDDS %d with "
        //"outstanding input or distribution queue entries"
        wsprintf(buff,ResourceString(IDS_F_ALTER_CDDS) , lpCdds->cdds);
        TS_MessageBox( TS_GetFocus(),buff,NULL,MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL );
        iret = RES_ERR;
        goto endfunc;
      }

      if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
        goto endfunc;
  }
  wsprintf(achBufReq,
        "insert into dd_cdds (cdds_no,cdds_name,collision_mode,error_mode) "
        "values ( %d , '%s' , %d , %d )",
        lpCdds->cdds,lpCdds->szCDDSName,
        lpCdds->collision_mode,lpCdds->error_mode);
  iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

  if (iret!=RES_SUCCESS)
      goto endfunc;

  if (lpCdds->bAlter) {
      // update dd_db_cdds
      wsprintf(achBufReq,
               "delete from dd_db_cdds where cdds_no = %d",lpCdds->cdds);
  
      iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);
  
      if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
        goto endfunc;
  }

  for (lpO=lpCdds->connections;lpO;lpO=lpO->lpNext)  {
      LPDD_CONNECTION lp1=(LPDD_CONNECTION)(lpO->lpObject);
      if ( (UINT)lp1->CDDSNo !=lpCdds->cdds || !lp1->bIsInCDDS)
         continue;
       wsprintf(achBufReq,
           "insert into dd_db_cdds (database_no,cdds_no,target_type,"
           "server_no) "
           "values ( %d , %d , %d , %d )",
           ((LPDD_CONNECTION)(lpO->lpObject))->dbNo,
           lpCdds->cdds,
           ((LPDD_CONNECTION)(lpO->lpObject))->nTypeRepl,
           ((LPDD_CONNECTION)(lpO->lpObject))->nServerNo );

      iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);
      if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
        goto endfunc;
  }


  if (lpCdds->bAlter) {
  // update dd_distribution 
  wsprintf(achBufReq,
           "delete from dd_paths where cdds_no = %d ",
           lpCdds->cdds);

  iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

  if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
    goto endfunc;
  }

  for (lpO=lpCdds->distribution;lpO;lpO=lpO->lpNext)  {
      if ( (UINT)((LPDD_DISTRIBUTION)(lpO->lpObject))->CDDSNo !=lpCdds->cdds)
         continue;
      wsprintf(achBufReq,
          "insert into dd_paths (localdb,sourcedb,targetdb,cdds_no) "
          "values ( %d , %d , %d , %d )",
          ((LPDD_DISTRIBUTION)(lpO->lpObject))->localdb,
          ((LPDD_DISTRIBUTION)(lpO->lpObject))->source,
          ((LPDD_DISTRIBUTION)(lpO->lpObject))->target,
          lpCdds->cdds);
     iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

     if (iret!=RES_SUCCESS)
         goto endfunc;
  }

  // delete reg columns associated with this cdds
  wsprintf(achBufReq,
           "delete from dd_regist_columns "
           "where table_no in "
           "(select table_no from dd_regist_tables where cdds_no = %d)",
           lpCdds->cdds);
  iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

  if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
      goto endfunc;

  if (lpCdds->bAlter) {
    wsprintf(achBufReq,
              "delete from dd_regist_tables where cdds_no = %d",
              lpCdds->cdds);
    iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

    if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
       goto endfunc;
  }

  for (lpO=lpCdds->registered_tables;lpO;lpO=lpO->lpNext) {
     char ColRegist[MAXOBJECTNAME];
     char cddsLookup[MAXOBJECTNAME];
     char curdate[MAXOBJECTNAME];
     LPOBJECTLIST lpO2;
     LPDD_REGISTERED_TABLES lptable=(LPDD_REGISTERED_TABLES)lpO->lpObject;

     if (!((LPDD_REGISTERED_TABLES)(lpO->lpObject))->bStillInCDDS)
       continue;

     ReplicDateNow(curdate);

     if (((LPDD_REGISTERED_TABLES)(lpO->lpObject))->colums_registered[0]=='C')
         wsprintf(ColRegist,"'%s'",curdate);
     else
         wsprintf(ColRegist,"''");
      
     if (x_strlen(((LPDD_REGISTERED_TABLES)(lpO->lpObject))->cdds_lookup_table_v11)!=0)
        wsprintf(cddsLookup,"'%s'",curdate);
     else
         wsprintf(cddsLookup,"''");

     wsprintf(achBufReq,
              "insert into dd_regist_tables "
              "(table_no,table_name,table_owner,columns_registered,"
              "supp_objs_created,rules_created,cdds_no,cdds_lookup_table,"
              "prio_lookup_table,index_used )"
              "values (%d,'%s','%s',%s,'%s','%s', %d,'%s','%s','%s')",
              ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->table_no,
              ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tablename,
              ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tableowner,
              ColRegist,
              ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->szDate_table_created,
              ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->szDate_rules_created,
              lpCdds->cdds,
              ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->cdds_lookup_table_v11,
              ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->priority_lookup_table_v11,
              ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->IndexName);

     iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

     if (iret!=RES_SUCCESS)
         goto endfunc;
     
     wsprintf(achBufReq,
              "update dd_last_number set last_number = %d "
              "where column_name = 'table_no'",lpCdds->LastNumber);
               
//                  ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->table_no);

     iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);
     
     if (iret!=RES_SUCCESS)
         goto endfunc;

     // Add columns

     for (lpO2=((LPDD_REGISTERED_TABLES)(lpO->lpObject))->lpCols;lpO2;lpO2=lpO2->lpNext) {
        if (((LPDD_REGISTERED_COLUMNS)(lpO2->lpObject))->rep_colums[0] =='R'){
			wsprintf(achBufReq,
                    "insert into dd_regist_columns "
                    "(table_no,column_name,column_sequence,key_sequence) "
                    "select %d, '%s', column_sequence, %d "
                    "from iicolumns "
                    "where table_name='%s' and table_owner='%s' and "
                    "column_name='%s' group by column_sequence",
                    ((LPDD_REGISTERED_TABLES)  (lpO->lpObject))->table_no,
                    ((LPDD_REGISTERED_COLUMNS)(lpO2->lpObject))->columnname,
                    ((LPDD_REGISTERED_COLUMNS)(lpO2->lpObject))->key_sequence,
                    ((LPDD_REGISTERED_TABLES)  (lpO->lpObject))->tablename,
                    ((LPDD_REGISTERED_TABLES)  (lpO->lpObject))->tableowner,
                    ((LPDD_REGISTERED_COLUMNS)(lpO2->lpObject))->columnname
                );
          iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

          if (iret!=RES_SUCCESS)
            goto endfunc;
        }
        else {
          // if this column is not replicated column_sequence = 0
          wsprintf(achBufReq,
                    "insert into dd_regist_columns (table_no,column_name,column_sequence,key_sequence) "
                    "values (%d, '%s', 0, %d)",
                    ((LPDD_REGISTERED_TABLES)  (lpO->lpObject))->table_no,
                    ((LPDD_REGISTERED_COLUMNS)(lpO2->lpObject))->columnname,
                    ((LPDD_REGISTERED_COLUMNS)(lpO2->lpObject))->key_sequence
                  );
          iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

          if (iret!=RES_SUCCESS)
            goto endfunc;

        }
     }
  }
  if (iret==RES_ENDOFDATA)
   iret=RES_SUCCESS;
  if (iret==RES_SUCCESS) {
     iret= CommitSession(isession); // because of remote commands
     if (iret!=RES_SUCCESS)
       goto endfunc;
     // Create/Drop rules/support tables if needed
     for (lpO=lpCdds->registered_tables;lpO;lpO=lpO->lpNext) {
       // LPOBJECTLIST lpO2;

       if (
           (((LPDD_REGISTERED_TABLES)(lpO->lpObject))->table_created[0]    =='T')&&
           (((LPDD_REGISTERED_TABLES)(lpO->lpObject))->table_created_ini[0]!='T')
          ) {
           DBACreateReplicSupV11( lpNodeName,
                                  lpDBName,
                                  StringWithOwner(((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tablename,  /* table_name, including owner */
                                                  ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tableowner,
                                                  szBuffer), 
                                  ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->table_no, isession);
       }
       if (
           (((LPDD_REGISTERED_TABLES)(lpO->lpObject))->table_created[0]    !='T')&&
           (((LPDD_REGISTERED_TABLES)(lpO->lpObject))->table_created_ini[0]=='T'))  {
           char Proc[MAXOBJECTNAME];
           BOOL bResult;
           int iret,tablelen;

           tablelen=x_strlen(((LPDD_REGISTERED_TABLES)lpO->lpObject)->tablename);
           if (tablelen>10)  {
              char name[20];
              fstrncpy(name,((LPDD_REGISTERED_TABLES)lpO->lpObject)->tablename,11);
              wsprintf(Proc,"%s%05d",name,
                                        ((LPDD_REGISTERED_TABLES)lpO->lpObject)->table_no);
            }
            else
              wsprintf(Proc,"%s%05d",((LPDD_REGISTERED_TABLES)lpO->lpObject)->tablename,
                                        ((LPDD_REGISTERED_TABLES)lpO->lpObject)->table_no);

           if (GetOIVers () == OIVERS_12) 
              bResult=DBADropReplicSupV11(Proc);
            else
              bResult=DBADropReplicSupV11Oi20(Proc);

           if (bResult == FALSE)
               iret=ExecSQLImm("rollback",FALSE, NULL, NULL, NULL);
           else {
               char buf[MAXOBJECTNAME*3];
               wsprintf(buf,
                        "update dd_regist_tables set supp_objs_created = '' "
                        "where table_name = '%s' and table_owner='%s'",
                        ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tablename,
                        ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tableowner
                         );
               iret=ExecSQLImm(buf,FALSE, NULL, NULL, NULL);
               if (iret!=RES_SUCCESS && iret != RES_ENDOFDATA)
                   iret=ExecSQLImm("rollback",FALSE, NULL, NULL, NULL);
               else
                   CommitSession(isession); // because of remote commands
           }
       }
       if (
           (((LPDD_REGISTERED_TABLES)(lpO->lpObject))->rules_created[0]    =='R')&&
           (((LPDD_REGISTERED_TABLES)(lpO->lpObject))->rules_created_ini[0]!='R')
          ) {
           UCHAR buf[120];
           char Proc[MAXOBJECTNAME];
           int tablelen;
           DBACreateReplicRulesV11( lpNodeName,
                                    lpDBName,
                        ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tablename,
                        ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->table_no, isession);
           if (GetOIVers () == OIVERS_12) { 
               tablelen=x_strlen(((LPDD_REGISTERED_TABLES)lpO->lpObject)->tablename);
               if (tablelen>10)  {
                  char name[20];
                  fstrncpy(name,((LPDD_REGISTERED_TABLES)lpO->lpObject)->tablename,11);
                  wsprintf(Proc,"%s%05dins",name,
                                            ((LPDD_REGISTERED_TABLES)lpO->lpObject)->table_no);
               }
               else
                  wsprintf(Proc,"%s%05dins",((LPDD_REGISTERED_TABLES)lpO->lpObject)->tablename,
                                            ((LPDD_REGISTERED_TABLES)lpO->lpObject)->table_no);
               
               if (AreInstalledRules ( Proc ) == FALSE){
                  wsprintf(buf,
                         "update dd_regist_tables set rules_created = '' "
                         "where table_name = '%s' and table_owner='%s'",
                         ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tablename,
                         ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tableowner
                          );
                  iret=ExecSQLImm(buf,FALSE, NULL, NULL, NULL);
                  if (iret!=RES_SUCCESS && iret != RES_ENDOFDATA)
                     iret=ExecSQLImm("rollback",FALSE, NULL, NULL, NULL);
                  else
                     CommitSession(isession); // because of remote commands
               }
           }
       }
       if (
           (((LPDD_REGISTERED_TABLES)(lpO->lpObject))->rules_created[0]    !='R')&&
           (((LPDD_REGISTERED_TABLES)(lpO->lpObject))->rules_created_ini[0]=='R')
          ) {
           UCHAR buf[120];
           char Proc[MAXOBJECTNAME];
           BOOL bResult;
           int iret,tablelen;
           
           if (GetOIVers () == OIVERS_12) { 
               tablelen=x_strlen(((LPDD_REGISTERED_TABLES)lpO->lpObject)->tablename);
               if (tablelen>10)  {
                  char name[20];
                  fstrncpy(name,((LPDD_REGISTERED_TABLES)lpO->lpObject)->tablename,11);
                  wsprintf(Proc,"%s%05d",name,
                                            ((LPDD_REGISTERED_TABLES)lpO->lpObject)->table_no);
                }
                else
                  wsprintf(Proc,"%s%05d",((LPDD_REGISTERED_TABLES)lpO->lpObject)->tablename,
                                            ((LPDD_REGISTERED_TABLES)lpO->lpObject)->table_no);
               
               
               bResult=DBADropReplicRulesV11(Proc);
           }
           else   {
               int ret;
               ret = ForceFlush(((LPDD_REGISTERED_TABLES)lpO->lpObject)->tablename,
                                ((LPDD_REGISTERED_TABLES)lpO->lpObject)->tableowner );
               if(ret != RES_ERR) 
                  bResult = TRUE;
           }
           if (bResult) {
             wsprintf(buf,
                    "update dd_regist_tables set rules_created = '' "
                    "where table_name = '%s' and table_owner='%s'",
                    ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tablename,
                    ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tableowner
                     );
             iret=ExecSQLImm(buf,FALSE, NULL, NULL, NULL);
             if (iret!=RES_SUCCESS && iret != RES_ENDOFDATA)
                iret=ExecSQLImm("rollback",FALSE, NULL, NULL, NULL);
             else
                CommitSession(isession); // because of remote commands

           }
           else
              iret=ExecSQLImm("rollback",FALSE, NULL, NULL, NULL);
       }
     }
  }

endfunc:
   return iret;
}

int BuildSQLDropCDDS    (UCHAR **PPstm, LPREPCDDS lpCdds)
{
  char         achBufReq[512];
  LPOBJECTLIST lpO;
  int          iret;

  // update dd_distribution 

  wsprintf(achBufReq,
           "delete from dd_distribution where dd_routing = %d",
           lpCdds->cdds);

  iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

  if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
     goto endfunc;

  // delete dd_registered_tables

  wsprintf(achBufReq,
           "delete from dd_registered_tables where dd_routing = %d",
           lpCdds->cdds);

  iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

  if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
     goto endfunc;

  for (lpO=lpCdds->registered_tables;lpO;lpO=lpO->lpNext)
     {
     // delete columns associated with this table
     wsprintf(achBufReq,
           "delete from dd_registered_columns where tablename = '%s'",
           ((LPDD_REGISTERED_TABLES)(lpO->lpObject))->tablename);

     iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);

     if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
         goto endfunc;
     }

  if (iret==RES_ENDOFDATA)
   iret=RES_SUCCESS;

endfunc:

   return iret;
}

int BuildSQLDropCDDSV11  (UCHAR **PPstm, LPREPCDDS lpCdds)
{
  char         achBufReq[512];
  int          iret;

  wsprintf(achBufReq,"delete from dd_cdds where cdds_no = %d",lpCdds->cdds);
  iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);
  if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
     goto endfunc;

  wsprintf(achBufReq,"delete from dd_db_cdds where cdds_no = %d",lpCdds->cdds);
  iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);
  if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
     goto endfunc;

  wsprintf(achBufReq,"delete from dd_paths where cdds_no = %d",lpCdds->cdds);
  iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);
  if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
     goto endfunc;

  wsprintf(achBufReq,
           "delete from dd_regist_columns "
           "where table_no in "
           "(select table_no from dd_regist_tables where cdds_no = %d)",
           lpCdds->cdds);
  iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);
  if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
     goto endfunc;

  wsprintf(achBufReq,"delete from dd_regist_tables where cdds_no = %d",lpCdds->cdds);
  iret = ExecSQLImm(achBufReq,FALSE, NULL, NULL, NULL);
  if (iret!=RES_SUCCESS && iret!=RES_ENDOFDATA)
     goto endfunc;
endfunc:
   if (iret==RES_ENDOFDATA)
      iret=RES_SUCCESS;
   return iret;
}

// JFS End

int GenAlterReplicConnection (LPREPLCONNECTPARAMS pOldReplConnectParams ,LPREPLCONNECTPARAMS pReplConnectParams)
{
   char bufrequest[400];
   int iret;
  if (pOldReplConnectParams->bLocalDb == TRUE)
    {
   wsprintf (bufrequest,
         "update dd_connections set dba_comment = '%s' ,server_role = %d ,"
         "database_no = %d , node_name = '%s' ,target_type = %d "
         "where database_no = %d and "
         "node_name = '%s' and "
         "target_type = %d and "
         "dba_comment = '%s' and "
         "database_name = '%s' and "
         "server_role   = %d",pReplConnectParams->szDescription,
                              pReplConnectParams->nServer,
                              pReplConnectParams->nNumber,
                              pReplConnectParams->szVNode,
                              pReplConnectParams->nType,
                              pOldReplConnectParams->nNumber,
                              pOldReplConnectParams->szVNode,
                              pOldReplConnectParams->nType,
                              pOldReplConnectParams->szDescription,
                              pOldReplConnectParams->szDBName,
                              pOldReplConnectParams->nServer);
    }
  else
   wsprintf (bufrequest,
         "update dd_connections set dba_comment = '%s' ,server_role = %d ,"
         "target_type = %d "
         "where database_no = %d and "
         "node_name = '%s' and "
         "target_type = %d and "
         "dba_comment = '%s' and "
         "database_name = '%s' and "
         "server_role   = %d",pReplConnectParams->szDescription,
                              pReplConnectParams->nServer,
                              pReplConnectParams->nType,
                              pReplConnectParams->nNumber,
                              pReplConnectParams->szVNode,
                              pOldReplConnectParams->nType,
                              pOldReplConnectParams->szDescription,
                              pReplConnectParams->szDBName,
                              pOldReplConnectParams->nServer);

  iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
  
  if (iret!=RES_SUCCESS)
    return iret;

  if ( pReplConnectParams->nReplicVersion == REPLIC_V105) {
      wsprintf (bufrequest,
               "update dd_connections set config_changed = date('now')");
      iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
      if (iret!=RES_SUCCESS)
         return iret;
  }
 
  
  if (pOldReplConnectParams->bLocalDb == TRUE) {
    ZEROINIT(bufrequest);
    wsprintf (bufrequest,
              "update dd_installation set database_no = %d ,node_name = '%s',"
              "target_type = %d ,dba_comment = '%s' "
              "where database_no = %d and "
              "node_name = '%s' and "
              "target_type = %d and "
              "dba_comment = '%s'",pReplConnectParams->nNumber,
                                   pReplConnectParams->szVNode,
                                   pReplConnectParams->nType,
                                   pReplConnectParams->szDescription,
                                   pOldReplConnectParams->nNumber,
                                   pOldReplConnectParams->szVNode,
                                   pOldReplConnectParams->nType,
                                   pOldReplConnectParams->szDescription );
  iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
  }    
  return iret;
}


int GenAlterReplicConnectionV11 (LPREPLCONNECTPARAMS pOldReplConnectParams ,LPREPLCONNECTPARAMS pReplConnectParams)
{
  char bufrequest[400];
  int iret;
  if (pOldReplConnectParams->bLocalDb == TRUE)  {
    wsprintf (bufrequest,
          "update dd_databases set remark = '%s' ,"
          "database_no = %d ,"
          "database_owner = '%s' ,"
          "vnode_name = '%s',"
          "dbms_type = '%s' ,"
          "local_db = 1"
          "where database_no = %d" ,pReplConnectParams->szDescription,
                      pReplConnectParams->nNumber,
                      pReplConnectParams->szDBOwner,
                      pReplConnectParams->szVNode,
                      pReplConnectParams->szDBMStype,
                      pOldReplConnectParams->nNumber );
  }
  else
    wsprintf (bufrequest,
          "update dd_databases set remark = '%s' ,"
          "dbms_type = '%s', local_db = 0, database_name = '%s',"
          "database_owner = '%s',vnode_name = '%s'"
          "where database_no = %d",pReplConnectParams->szDescription,
                      pReplConnectParams->szDBMStype,
                      pReplConnectParams->szDBName,
                      pReplConnectParams->szDBOwner,
                      pReplConnectParams->szVNode,
                      pOldReplConnectParams->nNumber );

  iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
  
  return iret;
}

int UpdCfgChanged(int iReplicType)
{
  char buf[500];
  if ( iReplicType== REPLIC_V105) {
      wsprintf (buf,
               "update dd_connections set config_changed = date('now')");
      return  ExecSQLImm(buf,FALSE, NULL, NULL, NULL);
  }
  if ( iReplicType== REPLIC_V11) {
      wsprintf (buf,
               "UPDATE dd_databases SET config_changed = DATE_GMT('now')");
      return  ExecSQLImm(buf,FALSE, NULL, NULL, NULL);
  }
  return RES_SUCCESS;
}









UINT StringList_Count (LPSTRINGLIST list)
{
    UINT uCount = 0;
    LPSTRINGLIST ls = list;
    while (ls)
    {
        uCount++;
        ls = ls->next;
    }
    return uCount;
}


LPSTRINGLIST StringList_Add  (LPSTRINGLIST oldList, LPCTSTR lpszString)
{
    LPTSTR lpszText;
    LPSTRINGLIST lpStruct, ls;

    if (!lpszString)
        return oldList;
    lpszText = (LPTSTR)ESL_AllocMem (lstrlen (lpszString) +1);
    lpStruct = (LPSTRINGLIST)ESL_AllocMem (sizeof (STRINGLIST));
    if (!(lpszText && lpStruct))
    {
        myerror (ERR_ALLOCMEM);
        return oldList;
    }
    lstrcpy (lpszText, lpszString);
    lpStruct->next       = NULL;
    lpStruct->lpszString = lpszText;
    lpStruct->extraInfo1 = 0;
    if (!oldList)
    {
        return lpStruct;
    }
    ls = oldList;
    while (ls)
    {
        if (!ls->next)
        {
            ls->next = lpStruct; 
            break;
        }
        ls = ls->next;
    }
    return oldList;
}


LPSTRINGLIST StringList_AddObject  (LPSTRINGLIST oldList, LPCTSTR lpszString, LPSTRINGLIST* lpObj)
{
    LPTSTR lpszText;
    LPSTRINGLIST lpStruct, ls;

    if (!lpszString)
        return oldList;
    lpszText = (LPTSTR)ESL_AllocMem (lstrlen (lpszString) +1);
    lpStruct = (LPSTRINGLIST)ESL_AllocMem (sizeof (STRINGLIST));
    if (!(lpszText && lpStruct))
    {
        myerror (ERR_ALLOCMEM);
        return oldList;
    }
    lstrcpy (lpszText, lpszString);
    lpStruct->next       = NULL;
    lpStruct->lpszString = lpszText;
    lpStruct->extraInfo1 = 0;
    if (lpObj)
        *lpObj = lpStruct;
    if (!oldList)
    {
        return lpStruct;
    }
    ls = oldList;
    while (ls)
    {
        if (!ls->next)
        {
            ls->next = lpStruct; 
            break;
        }
        ls = ls->next;
    }
    return oldList;
}


LPSTRINGLIST StringList_Search (LPSTRINGLIST oldList, LPCTSTR lpszString)
{
    LPSTRINGLIST ls;
    ls = oldList;
    while (ls)
    {
        if (x_stricmp (ls->lpszString, lpszString) == 0)
            return ls;
        ls = ls->next;
    }
    return NULL;
}

LPSTRINGLIST StringList_Remove (LPSTRINGLIST oldList, LPCTSTR lpszString)
{
    LPSTRINGLIST ls     = oldList;
    LPSTRINGLIST lpPrec = NULL, tmp;

    while (ls)
    {
        if (lstrcmpi (ls->lpszString, lpszString) == 0)
        {
            tmp = ls;
            if (lpPrec == NULL)             // Remove the head of the list 
                oldList = oldList->next;
            else
                lpPrec->next = ls->next;    // Remove somewhere else
            
            ESL_FreeMem (tmp->lpszString);
            ESL_FreeMem (tmp);
            return oldList;
        }
        lpPrec = ls;
        ls = ls->next;
    }
    return oldList;
}


LPSTRINGLIST StringList_Done (LPSTRINGLIST oldList)
{
    LPSTRINGLIST ls, lpTemp;
    ls = oldList;
    while (ls)
    {
        lpTemp = ls;
        ls = ls->next;
        ESL_FreeMem ((void*)(lpTemp->lpszString));
        ESL_FreeMem ((void*)lpTemp);
    }
    return NULL;
}


LPSTRINGLIST StringList_Copy (LPSTRINGLIST lpList)
{
    LPSTRINGLIST ls = lpList, list = NULL, lpObj;
       
    while (ls)
    {
        list = StringList_AddObject (list, ls->lpszString, &lpObj);
        if (!lpObj)
        {
             list = StringList_Done (list);
             ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
             return NULL;
        }
        lpObj->extraInfo1 = ls->extraInfo1;
        lpObj->extraInfo2 = ls->extraInfo2;
        lpObj->extraInfo3 = ls->extraInfo3;
        ls = ls->next;
    }
    return list;
}


