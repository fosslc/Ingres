/*****************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : CA/OpenIngres Visual DBA
**
**    Source : dbatls.c
**    Tools functions for dbaparse, dbasqlpr and dbaupd
**
**    Author : Lionel Storck
**
**  History after 04-May-1999:
**
**   18-Jan-2000 (schph01)
**     bug #100034 Add the braces in switch case when the object type is
**     OT_DBEVENT else the bad structure was freed.
**
**   01-Mar-2000 (schph01)
**     Transfert Get_IIDATEFORMAT() function of sqltls.sc into this source
**     because no SQL statement are necessary for known the II_DATE_FORMAT.
**     Bug #99648 :
**        Add Date data type :
**           "MULTINATIONAL4" and mapped to MULTINATIONAL
**           "FINLAND" and mapped to SWEDEN
**        because the input format date are the same.
**     Bug #100666 now when the date data type is not found the function
**     Get_IIDATEFORMAT() return IIDATEFORMAT_UNKNOWN.
**     Before started the populate functionnality, if the date data type
**     is unknown an message is display and populate is not launched.
**     (see modification in "domsplit.c")
**   27-Apr-2000 (schph01)
**     bug 101247 When II_DATE_FORMAT is not defined, the default format was
**     not copied into the right buffer.
**   23-May-2000 (noifr01)
**     (bug 99242)  cleanup for DBCS compliance
**   26-Mar-2001 (noifr01)
**     (sir 104270) removal of code for managing Ingres/Desktop
**   09-May-2001 (schph01)
**       SIR 102509  in CompareLocStruct() function add comparison with new
**       member variable iRawBlocks.
**   06-Jun-2001(schph01)
**      (additional change for SIR 102509) update of previous change
**      because of backend changes
**   28-Mar-2002 (noifr01)
**    (sir 99596) removed additional unused resources/sources
**   26-Mar-2003 (noifr01)
**    (sir 107523) management of sequences
**   06-Jun-2002 (hweho01)
**      Moved the brace "{" to the following line, circumvent the compiler
**      error "unexpected end of file" in AIX.
**      Compiler version: C for AIX 5.023.
*****************************************************************************/

#include <ctype.h>
#include <io.h>
#include "dba.h"
#include "dbaset.h"
#include "dbaparse.h"
#include "dbaginfo.h"
#include "error.h"
#include "dbadlg1.h"
#include "msghandl.h"
#include "dll.h"
#include  <tchar.h>
#include "resource.h"
#include "compat.h"
#include "nm.h"

#ifdef DOUBLEBYTE
#include <windows.h>
#endif /* double byte */

#include "extccall.h"

PRIVILEGE_IDxSTRING GrantPrivilegesString  [GRANT_MAX_PRIVILEGES];
IDandString GrantPrivileges                [GRANT_MAX_PRIVILEGES] =
{
   { GRANT_ACCESS,               IDS_I_G_ACCESS               ,  "access",               FALSE},
   { GRANT_CREATE_PROCEDURE,     IDS_I_G_CREATE_PROCEDURE     ,  "create_procedure",     FALSE},
   { GRANT_CREATE_TABLE,         IDS_I_G_CREATE_TABLE         ,  "create_table",         FALSE},
   { GRANT_CONNECT_TIME_LIMIT,   IDS_I_G_CONNECT_TIME_LIMIT   ,  "connect_time_limit ",  TRUE},    
   { GRANT_DATABASE_ADMIN,       IDS_I_G_DATABASE_ADMIN       ,  "db_admin",             FALSE},
   { GRANT_LOCKMOD,              IDS_I_G_LOCKMOD              ,  "Lockmode",             FALSE},
   { GRANT_QUERY_IO_LIMIT,       IDS_I_G_QUERY_IO_LIMIT       ,  "query_io_limit ",      TRUE},
   { GRANT_QUERY_ROW_LIMIT,      IDS_I_G_QUERY_ROW_LIMIT      ,  "query_row_limit ",     TRUE},
   { GRANT_SELECT_SYSCAT,        IDS_I_G_SELECT_SYSCAT        ,  "select_syscat",        FALSE},
   { GRANT_SESSION_PRIORITY,     IDS_I_G_SESSION_PRIORITY     ,  "session_priority ",    TRUE},
   { GRANT_UPDATE_SYSCAT,        IDS_I_G_UPDATE_SYSCAT        ,  "update_syscat",        FALSE},
   { GRANT_TABLE_STATISTICS,     IDS_I_G_TABLE_STATISTICS     ,  "table_statistics",     FALSE},
   { GRANT_IDLE_TIME_LIMIT,      IDS_I_G_IDLE_TIME_LIMIT      ,  "idle_time_limit ",     TRUE},
   { GRANT_CREATE_SEQUENCE,      IDS_I_G_CREATE_SEQUENCE      ,  "create_sequence",      FALSE},

   { GRANT_NO_ACCESS,            IDS_I_G_NO_ACCESS            ,  "noaccess",             FALSE},
   { GRANT_NO_CREATE_PROCEDURE,  IDS_I_G_NO_CREATE_PROCEDURE  ,  "nocreate_procedure",   FALSE},
   { GRANT_NO_CREATE_TABLE,      IDS_I_G_NO_CREATE_TABLE      ,  "nocreate_table",       FALSE},
   { GRANT_NO_CONNECT_TIME_LIMIT,IDS_I_G_NO_CONNECT_TIME_LIMIT,  "noconnect_time_limit", FALSE}, 
   { GRANT_NO_DATABASE_ADMIN,    IDS_I_G_NO_DATABASE_ADMIN    ,  "nodb_admin",           FALSE},
   { GRANT_NO_LOCKMOD,           IDS_I_G_NO_LOCKMOD           ,  "nolockmode",           FALSE},
   { GRANT_NO_QUERY_IO_LIMIT,    IDS_I_G_NO_QUERY_IO_LIMIT    ,  "noquery_io_limit",     FALSE},
   { GRANT_NO_QUERY_ROW_LIMIT,   IDS_I_G_NO_QUERY_ROW_LIMIT   ,  "noquery_row_limit",    FALSE},
   { GRANT_NO_SELECT_SYSCAT,     IDS_I_G_NO_SELECT_SYSCAT     ,  "noselect_syscat",      FALSE},
   { GRANT_NO_SESSION_PRIORITY,  IDS_I_G_NO_SESSION_PRIORITY  ,  "nosession_priority",   FALSE},
   { GRANT_NO_UPDATE_SYSCAT,     IDS_I_G_NO_UPDATE_SYSCAT     ,  "noupdate_syscat",      FALSE},
   { GRANT_NO_TABLE_STATISTICS,  IDS_I_G_NO_TABLE_STATISTICS  ,  "notable_statistics",   FALSE},
   { GRANT_NO_IDLE_TIME_LIMIT,   IDS_I_G_NO_IDLE_TIME_LIMIT   ,  "noidle_time_limit",    FALSE},
   { GRANT_NO_CREATE_SEQUENCE,   IDS_I_G_NO_CREATE_SEQUENCE   ,  "nocreate_sequence",    FALSE},

   { GRANT_SELECT,               IDS_I_G_SELECT               ,  "select",               FALSE},
   { GRANT_INSERT,               IDS_I_G_INSERT               ,  "insert",               FALSE},
   { GRANT_UPDATE,               IDS_I_G_UPDATE               ,  "update",               FALSE},
   { GRANT_DELETE,               IDS_I_G_DELETE               ,  "delete",               FALSE},
   { GRANT_COPY_INTO,            IDS_I_G_COPY_INTO            ,  "copy_into",            FALSE},
   { GRANT_COPY_FROM,            IDS_I_G_COPY_FROM            ,  "copy_from",            FALSE},
   { GRANT_REFERENCE,            IDS_I_G_REFERENCE            ,  "references",           FALSE},
   { GRANT_RAISE,                IDS_I_G_RAISE                ,  "raise",                FALSE},
   { GRANT_REGISTER,             IDS_I_G_REGISTER             ,  "register",             FALSE},
   { GRANT_EXECUTE,              IDS_I_G_EXECUTE              ,  "execute",              FALSE},
   { GRANT_QUERY_CPU_LIMIT,      IDS_I_G_QUERY_CPU_LIMIT      ,  "query_cpu_limit ",     TRUE},
   { GRANT_QUERY_PAGE_LIMIT,     IDS_I_G_QUERY_PAGE_LIMIT     ,  "query_page_limit ",    TRUE},
   { GRANT_QUERY_COST_LIMIT,     IDS_I_G_QUERY_COST_LIMIT     ,  "query_cost_limit ",    TRUE},
   { GRANT_NO_QUERY_CPU_LIMIT,   IDS_I_G_NO_QUERY_CPU_LIMIT   ,  "noquery_cpu_limit",    FALSE},
   { GRANT_NO_QUERY_PAGE_LIMIT,  IDS_I_G_NO_QUERY_PAGE_LIMIT  ,  "noquery_page_limit",   FALSE},
   { GRANT_NO_QUERY_COST_LIMIT,  IDS_I_G_NO_QUERY_COST_LIMIT  ,  "noquery_cost_limit",   FALSE}

};
/*
**
**  Generate the columns list with translate data type for a Gateway.
**  LPTSTR GenerateColList( LPTABLEPARAMS lpSrcTbl )
**  input :
**      LPTABLEPARAMS lpSrcTbl
**
**  Translate  money    -> decimal(??,19,2)
**              C       -> char (??,lenInfo.dwDataLen)
**              text    -> char(??,lenInfo.dwDataLen)
**              int1    -> int2(??)
**
**  return :
**      NULL    ConcateStrings() failed.
**      LPSTR   columns list for statement.
*/
LPTSTR GenerateColList( LPTABLEPARAMS lpSrcTbl )
{
    int m;
    TCHAR tbuf[200];
    LPTSTR lpString = NULL;
    LPOBJECTLIST ls;
    LPCOLUMNPARAMS lpCol;
    
    ls = lpSrcTbl->lpColumns;
    while (ls)
    {
        lpCol = (LPCOLUMNPARAMS)ls->lpObject;
        ls = ls->lpNext;
        memset(tbuf,'\0', sizeof(tbuf));
        switch (lpCol->uDataType) {
            case INGTYPE_MONEY:
                wsprintf(tbuf,_T(" float8(%s)"),lpCol->szColumn);
                break;
            case INGTYPE_C:
                wsprintf(tbuf,_T(" char (%s,%ld)"),lpCol->szColumn,lpCol->lenInfo.dwDataLen);
                break;
            case INGTYPE_TEXT:
                wsprintf(tbuf,_T(" char (%s)"),lpCol->szColumn);
                //wsprintf(tbuf,_T(" varchar (%s,%ld)"),lpCol->szColumn,lpCol->lenInfo.dwDataLen);
                break;
            case INGTYPE_INT1:
                wsprintf(tbuf,_T(" int2(%s)"),lpCol->szColumn);
                break;
            default:
                wsprintf(tbuf,_T(" %s"),lpCol->szColumn);
                break;
        }
        if (*tbuf) {
            m = ConcateStrings (&lpString, tbuf, (LPSTR)0);
            if (!m) {
                ESL_FreeMem((void *)lpString);
                return NULL;
            }
        if (ls)
            m = ConcateStrings (&lpString, _T(","), (LPSTR)0);
            if (!m) {
                ESL_FreeMem((void *)lpString);
                return NULL;
            }
        }
        else    {
            ESL_FreeMem((void *)lpString);
            return NULL;
        }
    } // While End

    return lpString;
}


void StrToLower(pu)
LPUCHAR pu;
{
	_tcslwr(pu);
}
   

LPCHECKEDOBJECTS GetNextCheckedObject(pcheckedobjects)
LPCHECKEDOBJECTS pcheckedobjects;
{
   if (!pcheckedobjects) {
      myerror(ERR_LIST);
      return pcheckedobjects;
   }
   if (! pcheckedobjects->pnext) {
      pcheckedobjects->pnext=ESL_AllocMem(sizeof(CHECKEDOBJECTS));
      if (!pcheckedobjects->pnext) 
         myerror(ERR_ALLOCMEM);
   }
   return pcheckedobjects->pnext;     
}

LPCHECKEDOBJECTS AddCheckedObject (LPCHECKEDOBJECTS lc, LPCHECKEDOBJECTS obj)
{
   if (obj)
      {
         obj->pnext = lc;
         lc = obj;
      }
      return (lc);
}

LPCHECKEDOBJECTS FreeCheckedObjects(pcheckedobjects)
LPCHECKEDOBJECTS pcheckedobjects;
{
   LPCHECKEDOBJECTS ptemp;

   while(pcheckedobjects) {
      ptemp=pcheckedobjects->pnext;
      ESL_FreeMem(pcheckedobjects);  
      pcheckedobjects = ptemp;
   }
   return pcheckedobjects;
}



int GetIdentifier (const char* str)
{
   int i;
   
   for (i = 0; i <GRANT_MAX_PRIVILEGES; i++)
   {
       if (x_stricmp (GrantPrivilegesString [i].str, str) == 0)
           return GrantPrivilegesString [i].id;
   }
   return -1;
}


BOOL  HasPrivilegeRelatedValue (int ConstDefine)
{
   int i;
   
   for (i = 0; i <GRANT_MAX_PRIVILEGES; i++)
   {
       if (GrantPrivileges [i].id == ConstDefine)
           return GrantPrivileges [i].bRelatedValue;
   }
   return FALSE;
}

char*  GetPrivilegeString (int ConstDefine)
{
   int i;
   
   for (i = 0; i <GRANT_MAX_PRIVILEGES; i++)
   {
       if (GrantPrivileges [i].id == ConstDefine)
           return GrantPrivileges [i].sql;
   }
   return NULL;
}

char*  GetPrivilegeString2 (int ConstDefine)
{
   int i;
   
   for (i = 0; i <GRANT_MAX_PRIVILEGES; i++)
   {
       if (GrantPrivilegesString [i].id == ConstDefine)
           return GrantPrivilegesString [i].str;
   }
   return NULL;
}

void   PrivilegeString_INIT ()
{
   int   i;
   char* aString;
   
   for (i = 0; i <GRANT_MAX_PRIVILEGES; i++)
   {
       aString = ResourceString ((UINT)GrantPrivileges[i].ids);
       if (aString)
       {
           GrantPrivilegesString [i].str = ESL_AllocMem (x_strlen (aString) +1);
           if (!GrantPrivilegesString [i].str)
           {
               // Masqued Emb 17/10/95 for msg globalization
               //ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
               break;
           }
           else
           {
               GrantPrivilegesString [i].id = GrantPrivileges[i].id;
               x_strcpy (GrantPrivilegesString [i].str, aString);
           }
       }
   }
}

void   PrivilegeString_DONE ()
{
   int   i;
   
   for (i = 0; i <GRANT_MAX_PRIVILEGES; i++)
   {
       if (GrantPrivilegesString [i].str)
       {
           ESL_FreeMem (GrantPrivilegesString [i].str);
           GrantPrivilegesString [i].str = NULL;
       }
   }
}

void FreeAttachedPointers(void * pvoid, int iobjecttype)
{
   switch (iobjecttype)
   {
       case OT_DATABASE:
       {
           LPDATABASEPARAMS lpDBParam = (LPDATABASEPARAMS) pvoid;
           FreeObjectList(lpDBParam->lpDBExtensions); // free locations extension
           lpDBParam->lpDBExtensions= NULL;
           break;
       }

       case OT_USER:
       {
           LPUSERPARAMS lpuser = (LPUSERPARAMS) pvoid;

           lpuser->lpfirstdb = FreeCheckedObjects (lpuser->lpfirstdb);
           if (lpuser->lpLimitSecLabels) {
              ESL_FreeMem(lpuser->lpLimitSecLabels);
              lpuser->lpLimitSecLabels=NULL; 
           }
           lpuser->lpfirstdb = NULL;

           break;
       }

       case OT_PROFILE:
       {
           LPPROFILEPARAMS lpprof = (LPPROFILEPARAMS) pvoid;

           if (lpprof->lpLimitSecLabels) {
              ESL_FreeMem(lpprof->lpLimitSecLabels);
              lpprof->lpLimitSecLabels=NULL;
           }
           lpprof->lpLimitSecLabels = NULL;

           break;
       }

       case OT_GROUP:
       {
           LPGROUPPARAMS lpgroup= (LPGROUPPARAMS) pvoid;

           lpgroup->lpfirstuser = FreeCheckedObjects (lpgroup->lpfirstuser);
           lpgroup->lpfirstdb   = FreeCheckedObjects (lpgroup->lpfirstdb  );
           lpgroup->lpfirstdb   = NULL;
           lpgroup->lpfirstuser = NULL;

           break;
       }

       case OT_GROUPUSER: 
       {

           break;
       }
//JFS Begin
       case OT_REPLIC_CDDS:
       {
           LPREPCDDS lpcdds     = (LPREPCDDS) pvoid;
           LPOBJECTLIST lpo;

           for (lpo=lpcdds->registered_tables; lpo ; lpo=lpo->lpNext)
               {
               FreeObjectList(((LPDD_REGISTERED_TABLES)lpo->lpObject)->lpCols);
               ((LPDD_REGISTERED_TABLES)lpo->lpObject)->lpCols=NULL; 
               }

           FreeObjectList(lpcdds->distribution);
           FreeObjectList(lpcdds->connections);
           FreeObjectList(lpcdds->registered_tables);

           lpcdds->distribution=NULL;
           lpcdds->connections=NULL;
           lpcdds->registered_tables=NULL;
           break;
       }
       
       
//JFS End

       case OT_ROLE        : 
       {
           LPROLEPARAMS lprole  = (LPROLEPARAMS) pvoid;

           lprole->lpfirstdb    = FreeCheckedObjects (lprole->lpfirstdb);
           lprole->lpfirstdb    = NULL;
           if (lprole->lpLimitSecLabels)
               ESL_FreeMem(lprole->lpLimitSecLabels);
           lprole->lpLimitSecLabels=NULL;
           break;
       }

       case OT_LOCATION    : 
       {
           // No linked list
           break;
       }

       case OT_TABLE       :
         {
           LPTABLEPARAMS lptbl = (LPTABLEPARAMS) pvoid;
           LPOBJECTLIST lpscanref;
           if (lptbl->lpColumnHeader)
           {
                ESL_FreeMem (lptbl->lpColumnHeader);
                lptbl->lpColumnHeader = NULL;
           }
           if (lptbl->lpSelectStatement)
           {
                ESL_FreeMem (lptbl->lpSelectStatement);
                lptbl->lpSelectStatement = NULL;
           }

           if (lptbl->StorageParams.lpidxCols)
           {
               FreeObjectList (lptbl->StorageParams.lpidxCols);
               lptbl->StorageParams.lpidxCols = NULL;
           }

           if (lptbl->StorageParams.lpMaintainCols)
           {
               FreeObjectList (lptbl->StorageParams.lpMaintainCols);
               lptbl->StorageParams.lpMaintainCols = NULL;
           }
           lptbl->lpColumns    = VDBA20xColumnList_Done (lptbl->lpColumns);
           lptbl->lpReferences = VDBA20xTableReferences_Done (lptbl->lpReferences);
           lptbl->lpUnique     = VDBA20xTableUniqueKey_Done  (lptbl->lpUnique);
           lptbl->lpCheck      = VDBA20xTableCheck_Done      (lptbl->lpCheck);
           lptbl->lpDropObjectList = StringList_Done (lptbl->lpDropObjectList);

           lptbl->primaryKey.constraintWithClause.pListLocation = StringList_Done(lptbl->primaryKey.constraintWithClause.pListLocation);
           lptbl->primaryKey.pListTableColumn  = StringList_Done(lptbl->primaryKey.pListTableColumn);
           lptbl->primaryKey.pListKey          = StringList_Done(lptbl->primaryKey.pListKey);

           lpscanref=lptbl->lpConstraint;
           while (lpscanref) {          
              if (((LPCONSTRAINTPARAMS)lpscanref->lpObject)->lpText)
                 ESL_FreeMem(
                    ((LPCONSTRAINTPARAMS)lpscanref->lpObject)->lpText);
              lpscanref=lpscanref->lpNext;
           }
           FreeObjectList(lptbl->lpConstraint);    // free columns list.
           lptbl->lpConstraint= NULL;

           FreeObjectList(lptbl->lpLocations);  // free locations list.
           lptbl->StorageParams.lpLocations= NULL;
           // TO BE FINISHED : lpLocation pointer are duplicate for MODIFY 
           lptbl->lpLocations= NULL;

           FreeObjectList(lptbl->StorageParams.lpNewLocations);  // free locations list.
           lptbl->StorageParams.lpNewLocations= NULL;
           lptbl->lpszTableCheck=NULL;
         }

         break;

       case OT_TABLELOCATION:
       {

           break;
       }

       case OT_VIEW: 
       {
           LPVIEWPARAMS lpview = (LPVIEWPARAMS) pvoid;

           if (lpview->lpSelectStatement)
           {
               ESL_FreeMem (lpview->lpSelectStatement);
               lpview->lpSelectStatement = NULL;
           }

           if (lpview->lpViewText)
           {
               ESL_FreeMem (lpview->lpViewText);
               lpview->lpViewText = NULL;
           }
           break;
       }

       case OT_VIEWTABLE: 
       {

           break;
       }

       case OT_INDEX:
         {
           LPINDEXPARAMS lpidx = (LPINDEXPARAMS) pvoid;

           if (lpidx->lpLocations)
           {
               FreeObjectList (lpidx->lpLocations);
               lpidx->lpLocations = NULL;
           }
           if (lpidx->lpidxCols)
           {
               FreeObjectList (lpidx->lpidxCols);
               lpidx->lpidxCols = NULL;
           }
           if (lpidx->lpMaintainCols)
           {
               FreeObjectList (lpidx->lpMaintainCols);
               lpidx->lpMaintainCols = NULL;
           }
         }
         break;

       case OT_INTEGRITY: 
       {
           LPINTEGRITYPARAMS lpintegrity = (LPINTEGRITYPARAMS) pvoid;

           if (lpintegrity->lpSearchCondition)
           {
               ESL_FreeMem (lpintegrity->lpSearchCondition);
               lpintegrity->lpSearchCondition = NULL;
           }
           if (lpintegrity->lpIntegrityText)
           {
               ESL_FreeMem (lpintegrity->lpIntegrityText);
               lpintegrity->lpIntegrityText = NULL;
           }
       }

       case OT_PROCEDURE: 
         {
           LPPROCEDUREPARAMS lpproc = (LPPROCEDUREPARAMS) pvoid;
      
           if (lpproc->lpProcedureParams)
           {
               ESL_FreeMem (lpproc->lpProcedureParams);
               lpproc->lpProcedureParams = NULL;
           }
           if (lpproc->lpProcedureDeclare)
           {
               ESL_FreeMem (lpproc->lpProcedureDeclare);
               lpproc->lpProcedureDeclare = NULL;
           }
           if (lpproc->lpProcedureStatement)
           {
               ESL_FreeMem (lpproc->lpProcedureStatement);
               lpproc->lpProcedureStatement = NULL;
           }
           if (lpproc->lpProcedureText)
           {
               ESL_FreeMem (lpproc->lpProcedureText);
               lpproc->lpProcedureText = NULL;
           }
         }
         break;

       case OT_RULE: 
       {
         {
           LPRULEPARAMS lprule = (LPRULEPARAMS) pvoid;
           if (lprule->lpTableCondition)
           {
               ESL_FreeMem (lprule->lpTableCondition);
               lprule->lpTableCondition = NULL;
           }
           if (lprule->lpProcedure)
           {
               ESL_FreeMem (lprule->lpProcedure);
               lprule->lpProcedure = NULL;
           }
           if (lprule->lpRuleText)
           {
               ESL_FreeMem (lprule->lpRuleText);
               lprule->lpRuleText = NULL;
           }
         }
         break;
       }

       case OT_RULEWITHPROC: 
       {

           break;
       }

       case OT_RULEPROC    : 
       {

           break;
       }

       case OT_SCHEMAUSER  : 
       {

           break;
       }

       case OT_SYNONYM     : 
       {
           // No linked list
           break;
       }

       case OT_SYNONYMOBJECT:
       {
           // No linked list
           break;
       }

       case OT_DBEVENT     :
       {
           break;
       }
       case OTLL_SECURITYALARM:
       {
           LPSECURITYALARMPARAMS lpsecurity = (LPSECURITYALARMPARAMS) pvoid;

           lpsecurity->lpfirstTable = FreeCheckedObjects (lpsecurity->lpfirstTable);
           lpsecurity->lpfirstUser  = FreeCheckedObjects (lpsecurity->lpfirstUser);
           if (lpsecurity->lpDBEventText) {
              ESL_FreeMem(lpsecurity->lpDBEventText);
              lpsecurity->lpDBEventText=NULL;
           }
           lpsecurity->lpfirstTable = NULL;
           lpsecurity->lpfirstUser  = NULL;

           break;
       }

       case OTLL_GRANT:
       {
           LPGRANTPARAMS lpgrant = (LPGRANTPARAMS) pvoid;

           if (lpgrant->lpgrantee)
           {
               FreeObjectList (lpgrant->lpgrantee);
               lpgrant->lpgrantee = NULL;
           }
           if (lpgrant->lpobject)
           {
               FreeObjectList (lpgrant->lpobject);
               lpgrant->lpobject = NULL;
           }
           if (lpgrant->lpcolumn)
           {
               FreeObjectList (lpgrant->lpcolumn);
               lpgrant->lpcolumn = NULL;
           }

           break;
       }

// revoke
       case OTLL_REVOKE:
       {
           LPREVOKEPARAMS lprevoke = (LPREVOKEPARAMS) pvoid;

           if (lprevoke->lpgrantee)
           {
               FreeObjectList (lprevoke->lpgrantee);
               lprevoke->lpgrantee = NULL;
           }
           if (lprevoke->lpobject)
           {
               FreeObjectList (lprevoke->lpobject);
               lprevoke->lpobject = NULL;
           }
           if (lprevoke->lpcolumn)
           {
               FreeObjectList (lprevoke->lpcolumn);
               lprevoke->lpcolumn = NULL;
           }

           break;
       }

       case OT_REPLIC_CONNECTION:
       case OT_REPLIC_CONNECTION_V11:
         break;

       case OT_SEQUENCE :
           break;

       default:
           myerror (ERR_OBJECTNOEXISTS);
           break;
   }
}
static BOOL CompareLocStruct(LPLOCATIONPARAMS Struct1,
                             LPLOCATIONPARAMS Struct2)
{
   int i;
   if (x_strcmp(Struct1->objectname,Struct2->objectname))
      return FALSE;
   if (x_strcmp(Struct1->AreaName,Struct2->AreaName))
      return FALSE;
   for (i=0; i<LOCATIONUSAGES; i++) {
      if (Struct1->bLocationUsage[i]!=Struct2->bLocationUsage[i])
         return FALSE;
   }
   if (Struct1->iRawPct != Struct2->iRawPct)
      return FALSE;

   return TRUE;

}
static BOOL CompareGrpStruct(LPGROUPPARAMS Struct1,LPGROUPPARAMS Struct2)
{
   LPCHECKEDOBJECTS lpInner, lpOuter;
   
   /* Check object name first */ 

   if (x_strcmp(Struct1->ObjectName,Struct2->ObjectName))
      return FALSE;

   /* Now check user list : CAn be obtimized by keeping a pointer to the
      last entry in ther inner loop when all the prev entries has been
      checked                                                           */

   lpOuter=Struct1->lpfirstuser;
   
   while (lpOuter) {                   /* Outer loop : all struct1 users */
      lpInner=Struct1->lpfirstuser;

      while (lpInner) {                /* Inner loop : all struct2 users */
         if (!x_strcmp(lpOuter->dbname, lpInner->dbname))  // same user
            if (lpOuter->bchecked==lpInner->bchecked)    // same check status
               break;                  /* Ok leave ptr not null */
         lpInner=lpInner->pnext;      /* next Inner user */
      }

      if (!lpInner)
         return FALSE;                 // should have leaved with a non NULL
      lpOuter=lpOuter->pnext;         /* Next outer user */
   }
   return TRUE;

}
static BOOL CompareUsrStruct(LPUSERPARAMS Struct1,LPUSERPARAMS Struct2)
{
   int i;

   /* Check object name first */ 

   if (x_strcmp(Struct1->ObjectName,Struct2->ObjectName))
      return FALSE;

   /* check the default group */

   if (x_strcmp(Struct1->DefaultGroup,Struct2->DefaultGroup))
      return FALSE;

   /* check for the privileges */

   for (i=0; i<USERPRIVILEGES; i++)
      if (Struct1->Privilege[i]!=Struct2->Privilege[i])
         return FALSE;

   /* check for the default privileges */

   for (i=0; i<USERPRIVILEGES; i++)
      if (Struct1->bDefaultPrivilege[i]!=Struct2->bDefaultPrivilege[i])
         return FALSE;

   /* check for the securty audit privileges */

   for (i=0; i<USERSECAUDITS; i++)
      if (Struct1->bSecAudit[i]!=Struct2->bSecAudit[i])
         return FALSE;

   /* check the expire date */

   if (x_strcmp(Struct1->ExpireDate,Struct2->ExpireDate))
      return FALSE;

   /* Check for the limiting security label */

   if (Struct1->lpLimitSecLabels)
      if (Struct2->lpLimitSecLabels) {
         if (x_strcmp(Struct1->lpLimitSecLabels,Struct2->lpLimitSecLabels))
            return FALSE;
      }
      else
         return FALSE;
   else
      if (Struct2->lpLimitSecLabels)
         return FALSE;

   /* check the profile name group */

   if (x_strcmp(Struct1->szProfileName,Struct2->szProfileName))
      return FALSE;

   /* check the external password option */

   if (Struct1->bExtrnlPassword!=Struct2->bExtrnlPassword)
      return FALSE;

   return TRUE;             

}
static BOOL CompareProfStruct(LPPROFILEPARAMS Struct1,LPPROFILEPARAMS Struct2)
{
   int i;

   /* Check object name first */ 

   if (x_strcmp(Struct1->ObjectName,Struct2->ObjectName))
      return FALSE;

   /* check the default group */

   if (x_strcmp(Struct1->DefaultGroup,Struct2->DefaultGroup))
      return FALSE;

   /* check for the privileges */

   for (i=0; i<USERPRIVILEGES; i++)
      if (Struct1->Privilege[i]!=Struct2->Privilege[i])
         return FALSE;

   /* check for the default privileges */

   for (i=0; i<USERPRIVILEGES; i++)
      if (Struct1->bDefaultPrivilege[i]!=Struct2->bDefaultPrivilege[i])
         return FALSE;

   /* check for the securty audit privileges */

   for (i=0; i<USERSECAUDITS; i++)
      if (Struct1->bSecAudit[i]!=Struct2->bSecAudit[i])
         return FALSE;

   /* check the expire date */

   if (x_strcmp(Struct1->ExpireDate,Struct2->ExpireDate))
      return FALSE;

   /* Check for the limiting security label */

   if (Struct1->lpLimitSecLabels)
      if (Struct2->lpLimitSecLabels) {
         if (x_strcmp(Struct1->lpLimitSecLabels,Struct2->lpLimitSecLabels))
            return FALSE;
      }
      else
         return FALSE;
   else
      if (Struct2->lpLimitSecLabels)
         return FALSE;

   return TRUE;             

}

static BOOL CompareRoleStruct(LPROLEPARAMS Struct1,LPROLEPARAMS Struct2)
{
   int i;

   /* Check object name first */ 

   if (x_strcmp(Struct1->ObjectName,Struct2->ObjectName))
      return FALSE;

   /* check for the privileges */

   for (i=0; i<USERPRIVILEGES; i++)
      if (Struct1->Privilege[i]!=Struct2->Privilege[i])
         return FALSE;

   /* check for the default privileges */

   for (i=0; i<USERPRIVILEGES; i++)
      if (Struct1->bDefaultPrivilege[i]!=Struct2->bDefaultPrivilege[i])
         return FALSE;

   /* check for the securty audit privileges */

   for (i=0; i<USERSECAUDITS; i++)
      if (Struct1->bSecAudit[i]!=Struct2->bSecAudit[i])
         return FALSE;

   /* Check for the limiting security label */

   if (Struct1->lpLimitSecLabels)
      if (Struct2->lpLimitSecLabels) {
         if (x_strcmp(Struct1->lpLimitSecLabels,Struct2->lpLimitSecLabels))
            return FALSE;
      }
      else
         return FALSE;
   else
      if (Struct2->lpLimitSecLabels)
         return FALSE;

   return TRUE;             

}

static BOOL CompareRuleStruct(LPRULEPARAMS Struct1,
                              LPRULEPARAMS Struct2)
{

   /* Check object name first */ 

   if (x_strcmp(Struct1->RuleName,Struct2->RuleName))
      return FALSE;

   /* check the rule definition (text) */

   if (x_strcmp(Struct1->lpRuleText,Struct2->lpRuleText))
      return FALSE;


   return TRUE;

}


static BOOL CompareProcStruct(LPPROCEDUREPARAMS Struct1,
                              LPPROCEDUREPARAMS Struct2)
{

   /* Check object name first */ 

   if (x_strcmp(Struct1->objectname,Struct2->objectname))
      return FALSE;

   /* check the rule definition (text) */

   if (x_strcmp(Struct1->lpProcedureText,Struct2->lpProcedureText))
      return FALSE;


   return TRUE;

}

// JFS Begin
static BOOL CmpCol(LPOBJECTLIST lpOL1,LPOBJECTLIST lpOL2)
{
   while (lpOL1)
       {
       LPDD_REGISTERED_COLUMNS lpC1=(LPDD_REGISTERED_COLUMNS)lpOL1->lpObject;
       LPOBJECTLIST lpOL=lpOL2;
       BOOL bFind = FALSE;
       while (lpOL && !bFind)
           {
           LPDD_REGISTERED_COLUMNS lpC2=(LPDD_REGISTERED_COLUMNS)lpOL->lpObject;
           if (

              lstrcmp (lpC1->columnname,
                       lpC2->columnname) == 0  &&
              lstrcmp (lpC1->key_colums,
                       lpC2->key_colums) == 0  &&
              lstrcmp (lpC1->rep_colums,
                       lpC2->rep_colums) == 0
                      )
           bFind=TRUE;
           lpOL=lpOL->lpNext;
           }

       if (!bFind)
           return FALSE;
       lpOL1=lpOL1->lpNext;
       }
   return TRUE;
}

static BOOL CmpTbls(LPOBJECTLIST lpOL1,LPOBJECTLIST lpOL2)
{
   while (lpOL1)
       {
       LPDD_REGISTERED_TABLES lpT1 =(LPDD_REGISTERED_TABLES)lpOL1->lpObject;
       LPOBJECTLIST lpOL=lpOL2;
       BOOL bFind = FALSE;
       while (lpOL && !bFind)
           {
           LPDD_REGISTERED_TABLES lpT  =(LPDD_REGISTERED_TABLES)lpOL->lpObject;
           if (
              lpT1->cdds == lpT->cdds &&
              lstrcmp ( lpT->tablename,lpT1->tablename)==0 &&
              lstrcmp ( lpT->tableowner,lpT1->tableowner)==0 &&
              lstrcmp ( lpT->table_created,lpT1->table_created)==0 &&
              lstrcmp ( lpT->colums_registered,lpT1->colums_registered)==0 &&
              lstrcmp ( lpT->rules_created,lpT1->rules_created)==0 &&
              lstrcmp ( lpT->rule_lookup_table,lpT1->rule_lookup_table)==0 &&
              lstrcmp ( lpT->priority_lookup_table,lpT1->priority_lookup_table)==0
              )
              {
              bFind=TRUE;
              if (!CmpCol(lpT1->lpCols,lpT->lpCols) ||
                  !CmpCol(lpT->lpCols, lpT1->lpCols)
                 )
                return FALSE;
              
              }
           lpOL=lpOL->lpNext;
           }

       if (!bFind)
           return FALSE;
       lpOL1=lpOL1->lpNext;
       }
   return TRUE;
}

static BOOL CompareCDDSStruct(LPREPCDDS Struct1,
                              LPREPCDDS Struct2)
{

   LPOBJECTLIST lpOL1;
   LPOBJECTLIST lpOL2;

   if ( Struct1->cdds != Struct2->cdds )
       return FALSE;

   for (lpOL1 = Struct1->distribution ; lpOL1 ; lpOL1=lpOL1->lpNext)
           {
           LPDD_DISTRIBUTION lpD1=lpOL1->lpObject;

           BOOL bfind=FALSE;
           for (lpOL2 = Struct2->distribution ; lpOL2 && !bfind ; lpOL2=lpOL2->lpNext)
               {
               LPDD_DISTRIBUTION lpD2=lpOL2->lpObject;
               if (
               lpD1->localdb ==
               lpD2->localdb &&
               lpD1->source  ==
               lpD2->source  &&
               lpD1->target  ==
               lpD2->target
               )
               bfind=TRUE;
               }
           if (!bfind)
               return FALSE;
           }


   for (lpOL1 = Struct2->distribution ; lpOL1 ; lpOL1=lpOL1->lpNext)
           {
           LPDD_DISTRIBUTION lpD1=lpOL1->lpObject;
           BOOL bfind=FALSE;
           for (lpOL2 = Struct1->distribution ; lpOL2 && !bfind ; lpOL2=lpOL2->lpNext)
               {
               LPDD_DISTRIBUTION lpD2=lpOL2->lpObject;

               if (
               lpD1->localdb ==
               lpD2->localdb &&
               lpD1->source  ==
               lpD2->source  &&
               lpD1->target  ==
               lpD2->target
               )
               bfind=TRUE;
               }
           if (!bfind)
               return FALSE;
           }
   
   if (!CmpTbls(Struct1->registered_tables,Struct2->registered_tables))
       return FALSE;
   if (!CmpTbls(Struct2->registered_tables,Struct1->registered_tables))
       return FALSE;

   return TRUE;
}
// JFS End
static BOOL CompareConnectiontruct(LPREPLCONNECTPARAMS Struct1,
                                   LPREPLCONNECTPARAMS Struct2)
{
   if (x_strcmp(Struct1->szDBName,Struct2->szDBName)          ||
       x_strcmp(Struct1->szVNode,Struct2->szVNode)            ||
       x_strcmp(Struct1->szDescription,Struct2->szDescription)||
       Struct1->nNumber != Struct2->nNumber                 ||
       Struct1->nServer != Struct2->nServer                 ||
       Struct1->nType   != Struct2->nType                   )
      return FALSE;

   return TRUE;
}
static BOOL CompareConnectionStructV11(LPREPLCONNECTPARAMS Struct1,
                                       LPREPLCONNECTPARAMS Struct2)
{
   if (x_strcmp(Struct1->szDBName,Struct2->szDBName)          ||
       x_strcmp(Struct1->szVNode,Struct2->szVNode)            ||
       x_strcmp(Struct1->szDescription,Struct2->szDescription)||
       x_strcmp(Struct1->szDBMStype,Struct2->szDBMStype)      ||
       Struct1->nNumber != Struct2->nNumber )
      return FALSE;

   return TRUE;
}

static BOOL CompareSequenceStruct(LPSEQUENCEPARAMS Struct1,
                                  LPSEQUENCEPARAMS Struct2)
{
   if ( x_strcmp(Struct1->DBName,Struct2->DBName)           != 0  ||
        x_strcmp(Struct1->objectname,Struct2->objectname)   != 0  ||
        x_strcmp(Struct1->objectowner,Struct2->objectowner) != 0  ||
        Struct1->bDecimalType   != Struct2->bDecimalType          ||
        Struct1->iseq_length    != Struct2->iseq_length           ||
        x_strcmp(Struct1->szDecimalPrecision, Struct2->szDecimalPrecision) != 0 ||
        x_strcmp(Struct1->szStartWith,Struct2->szStartWith ) != 0 ||
        x_strcmp(Struct1->szIncrementBy,Struct2->szIncrementBy) != 0 ||
        x_strcmp(Struct1->szNextValue,Struct2->szNextValue ) != 0 ||
        x_strcmp(Struct1->szMinValue,Struct2->szMinValue   ) != 0 ||
        x_strcmp(Struct1->szMaxValue,Struct2->szMaxValue   ) != 0 ||
        x_strcmp(Struct1->szCacheSize, Struct2->szCacheSize) != 0 ||
        Struct1->bCycle         != Struct2->bCycle                ||
        Struct1->bOrder         != Struct2->bOrder )
      return FALSE;

   return TRUE;

}

BOOL AreObjectsIdentical(void *Struct1, void *Struct2, int iobjecttype)
{
   switch (iobjecttype)
   {
      case OT_LOCATION:
            return CompareLocStruct((LPLOCATIONPARAMS) Struct1,
                                    (LPLOCATIONPARAMS) Struct2);
      case OT_GROUP:
            return CompareGrpStruct((LPGROUPPARAMS) Struct1,
                                    (LPGROUPPARAMS) Struct2);
      case OT_USER:
            return CompareUsrStruct((LPUSERPARAMS) Struct1,
                                    (LPUSERPARAMS) Struct2);
      case OT_PROFILE:
            return CompareProfStruct((LPPROFILEPARAMS) Struct1,
                                    (LPPROFILEPARAMS) Struct2);
      case OT_ROLE:
            return CompareRoleStruct((LPROLEPARAMS) Struct1,
                                     (LPROLEPARAMS) Struct2);
      case OT_RULE:
            return CompareRuleStruct((LPRULEPARAMS) Struct1,
                                     (LPRULEPARAMS) Struct2);
      case OT_PROCEDURE:
            return CompareProcStruct((LPPROCEDUREPARAMS) Struct1,
                                     (LPPROCEDUREPARAMS) Struct2);
      // JFS Begin
      case OT_REPLIC_CDDS:
           return CompareCDDSStruct((LPREPCDDS) Struct1,
                                    (LPREPCDDS) Struct2);
      // JFS End
      case OT_REPLIC_CONNECTION:
            return CompareConnectiontruct((LPREPLCONNECTPARAMS) Struct1,
                                         (LPREPLCONNECTPARAMS) Struct2);
      case OT_REPLIC_CONNECTION_V11:
            return CompareConnectionStructV11((LPREPLCONNECTPARAMS) Struct1,
                                             (LPREPLCONNECTPARAMS) Struct2);
      // desktop / normal
      case OT_DATABASE:
            myerror(ERR_INVPARAMS);
            return FALSE;
      case OT_SEQUENCE:
            return CompareSequenceStruct((LPSEQUENCEPARAMS) Struct1,
                                         (LPSEQUENCEPARAMS) Struct2);
 
      default :
         myerror(ERR_INVPARAMS);
         return FALSE;
   }
}

// Modified Emb 04/03/96 : must read 2 bytes, but storage must
// be sizeof(int) and preset to 0 (for 32bit model)
int ReadInt(int InputFile)
{
  char c1[sizeof(int)] ;

  memset(c1, 0, sizeof(int));
  read(InputFile, c1, 2);
  return(*(int *)c1) ;
}

// Modified Emb 04/03/96 : must read 4 bytes, but storage must
// be sizeof(long) and preset to 0 (for 32bit model)
long ReadLong(int InputFile)
{
  char c1[sizeof(long)] ;

  memset(c1, 0, sizeof(long));
  read(InputFile, c1, 4) ;
  return(*(long *)c1) ;
}

static LPUCHAR DoubleTheQuotes(LPUCHAR lpIn, int ilen)
{
   LPUCHAR lpOut, lpOutStart;
   int i;

   lpOut=lpOutStart=ESL_AllocMem(ilen*2+3);  // Max possible length

   *lpOut++='\'';
   for (i=0; i<ilen; i++) {
#ifdef DOUBLEBYTE
		if (IsDBCSLeadByte(*lpIn)) // Cannot be a quote
		{
			*lpOut=*lpIn; 	/* copy the first part of the DBCS character */
							/* the rest is done underneath */
			i++;
		} else
#endif /* double byte */
      if (*lpIn=='\'') 
         *lpOut++='\'';
      *lpOut++=*lpIn++;
   }
   *lpOut++='\'';
   *lpOut='\0';
   return lpOutStart;
}


int CopyDataAcrossTables(LPUCHAR lpFromNode, LPUCHAR lpFromDb,
                         LPUCHAR lpFromOwner, LPUCHAR lpFromTable,
                         LPUCHAR lpToNode, LPUCHAR lpToDb,
                         LPUCHAR lpToOwner, LPUCHAR lpToTable,
                         BOOL bGWConvertDataTypes,LPTSTR lpColLst)
{
   int iret;
   UCHAR szFileId[_MAX_PATH];
   OFSTRUCT ofFileStruct;

#ifdef WIN32
   UCHAR szTempPath[_MAX_PATH];
   if (GetTempPath(sizeof(szTempPath), szTempPath) == 0)
     return RES_ERR;
   if (GetTempFileName(szTempPath, "dba", 0, szFileId) == 0)
     return RES_ERR;
#else
   GetTempFileName(0, "dba", 0, szFileId);
#endif

   iret=ExportTableData(lpFromNode, lpFromDb, lpFromOwner,
                        lpFromTable, szFileId,
                        bGWConvertDataTypes, lpColLst);

   if (HasLoopInterruptBeenRequested()) {
      OpenFile((LPCSTR)szFileId, &ofFileStruct, OF_DELETE);
      return RES_ERR;
   }

   if (iret!=RES_SUCCESS) {
      OpenFile((LPCSTR)szFileId, &ofFileStruct, OF_DELETE);
      return iret;
   }
   iret=ImportTableData(lpToNode, lpToDb, lpToOwner,
                        lpToTable, szFileId);
   OpenFile((LPCSTR)szFileId, &ofFileStruct, OF_DELETE);
   return iret;
}

int ExportTableData(LPUCHAR lpNode, LPUCHAR lpDb, LPUCHAR lpOwner,
                    LPUCHAR lpTable, LPUCHAR lpFileId, BOOL bGWConvertDataTypes,LPTSTR LpLstCol)
{
   int iret, iSession;
   char szSqlStm[100+3*MAXOBJECTNAME];
   char szSqlTempoStm[100+3*MAXOBJECTNAME];
   char szConnectName[2*MAXOBJECTNAME+2+1];

   wsprintf(szSqlStm, "copy table %s.%s() into '%s'",
                      QuoteIfNeeded(lpOwner), QuoteIfNeeded(lpTable), lpFileId);
   if (bGWConvertDataTypes) {
      wsprintf(szSqlTempoStm, "declare global temporary table session.%s "
                         "as select %s from %s.%s "
                         "on commit preserve rows with norecovery",
                         StringWithoutOwner(lpTable), LpLstCol,lpOwner,StringWithoutOwner(lpTable));
      wsprintf(szSqlStm, "copy table session.%s() into '%s'",
                         StringWithoutOwner(lpTable), lpFileId);
   }

   if (!x_strcmp(lpNode,ResourceString((UINT)IDS_I_LOCALNODE)))
      x_strcpy(szConnectName, lpDb);
   else 
      wsprintf(szConnectName,"%s::%s",lpNode, lpDb);

   iret = Getsession(szConnectName,SESSION_TYPE_INDEPENDENT, &iSession);

   if (iret!=RES_SUCCESS)
      return iret;         

   if (bGWConvertDataTypes)
      iret=ExecSQLImm(szSqlTempoStm,FALSE, NULL, NULL, NULL);

   if (iret==RES_SUCCESS)
      iret=ExecSQLImm(szSqlStm,FALSE, NULL, NULL, NULL);

   if (iret==RES_SUCCESS)
      iret=ReleaseSession(iSession, RELEASE_COMMIT);
   else 
      ReleaseSession(iSession, RELEASE_ROLLBACK);

   return iret;
}

int ImportTableData(LPUCHAR lpNode, LPUCHAR lpDb, LPUCHAR lpOwner,
                    LPUCHAR lpTable, LPUCHAR lpFileId)
{
   int iret, iSession;
   char szSqlStm[100+3*MAXOBJECTNAME];
   char szConnectName[2*MAXOBJECTNAME+2+1];

   if(lpOwner[0])
      wsprintf(szSqlStm, "copy table %s.%s() from '%s'",
                    QuoteIfNeeded(lpOwner), QuoteIfNeeded(lpTable), lpFileId);
   else
      wsprintf(szSqlStm, "copy table %s() from '%s'",
                    QuoteIfNeeded(lpTable), lpFileId);

   if (!x_strcmp(lpNode,ResourceString((UINT)IDS_I_LOCALNODE)))
      x_strcpy(szConnectName, lpDb);
   else 
      wsprintf(szConnectName,"%s::%s",lpNode, lpDb);


   iret = Getsession(szConnectName,SESSION_TYPE_INDEPENDENT, &iSession);

   if (iret!=RES_SUCCESS)
      return iret;

   iret=ExecSQLImm(szSqlStm,FALSE, NULL, NULL, NULL);

   if (iret==RES_SUCCESS)
      iret=ReleaseSession(iSession, RELEASE_COMMIT);
   else 
      ReleaseSession(iSession, RELEASE_ROLLBACK);

   return iret;
}
