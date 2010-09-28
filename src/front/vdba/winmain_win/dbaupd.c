/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project : CA/OpenIngres Visual DBA
**
**  Source : dbaupd.c
**
**  Add/alter/drop object, populate table (sql side)
**
**  Author : Lionel Storck
**           Changed Emb Sep. 5, 95 to manage long values in populate
**
**  History:
**  25-jan-2000 (schph01)
**      bug #100034
**      Additional change, Generate the sql syntax with quote when you
**      drag and drop a "user" into the "group name".
**  14-Feb-2000 (somsa01)
**      Corrected include of types.h and stat.h .
**  29-Feb-2000 (schph01)
**      Sir #100640 :
**      - checked the column data type before formating the value.
**      - add the sign + and - to see this value like a numeric value.
**      - release the memory when an error it produced 
**  01-Mar-2000 (schph01)
**      Bug #99080
**      Remove the modification for the sign (+ and -) do in change
**      #445107(Sir #100640) and checked if the sign if only at the begining of
**      the string.
**  02-Mar-2000 (schph01)
**      Bug #97680
**      Additional change for Bug #97680, Generate the sql syntax with quote
**      when you drop a procedure.
**  03-May-2000 (schph01)
**      Bug #101396 Changed the generated syntax when a Read Only database
**      is created.
**      Removed the blank between the -r option and the location name. Took
**      this opportunity to reverse the order between the DbName and the
**      location name, consistently with the syntax described in the Command
**      Reference Guide.
**  20-May-2000 (noifr01)
**     (bug 99242) clean-up for DBCS compliance: removed unused CSV_Record()
**     function for improving readability when looking for DBCS potential
**     issues
**  16-Nov-2000(schph01)
**     SIR 102821 execute the sql command for generated a comment for a
**                table,view or columns
**  26-Mar-2001 (noifr01)
**     (sir 104270) removal of code for managing Ingres/Desktop
**  09-May-2001(schph01)
**     (SIR 102509) Add new option(rawpct) in generated syntax for
**     "Create Location" command.
** 30-May-2001 (uk$so01)
**    SIR #104736, Integrate IIA and Cleanup old code of populate.
** 20-Jun-2001 (schph01)
**    SIR 103694 
**       - add -n option in syntax of Createdb
**       - in function GetValidDataType() add the new type (NCHAR,NVARCHAR,
**         LONG_NVARCHAR).
** 02-Jul-2001 (hanje04)
**	Changed _itot to _itoa because there is no equivalent function for 
**	_itot on Unix
** 13-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
** 28-Mar-2002 (noifr01)
**   (sir 99596) removed additional unused resources/sources
** 08-Jul-2002 (schph01)
**    BUG #108199 In insert statements, ensure there is a space after the
**    comma sign that separates the contains of two columns.
** 18-Mar-2003 (schph01)
**    sir 107523 management of sequences
** 31-Oct-2003 (noifr01)
**    restored trailing } (and added CR) that seemed to have been lost upon
**    massive ingres30->main codeline propagation
** 12-May-2004 (schph01)
**    SIR #111507 Add management for new column type bigint
** 26-May-2004 (schph01)
**    SIR #112460 Add parameter -page_size for createdb command.
** 17-May-2004 (schph01)
**    (SIR 112514) Add management for Alter column syntax.
** 06-Sep-2005) (drivi01)
**    Bug #115173 Updated createdb dialogs and alterdb dialogs to contain
**    two available unicode normalization options, added group box with
**    two checkboxes corresponding to each normalization, including all
**    expected functionlity.
**    Added command line flag for NFC normalization.
**  25-May-2010 (drivi01) Bug 123817
**    Expand bufrequest buffer to MAXOBJECTNAME * 3 + 400.
**    For the types of strings this buffer is used it needs to be at least
**    three times MAXOBJECTNAME to account for long ids and also
**    big enough to contain some SQL query language.
**  24-Jun-2010 (drivi01)
**    Expand the size of bufrequest and remove hardcoded legnth.
**    The buffer should be big enough to fit at least 2 MAXOBJECTNAME.
******************************************************************************/

#include <fcntl.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <share.h>
#include <stdarg.h>
#include <time.h>
#include <io.h>
#include <stdio.h>
#include <ctype.h>

#ifdef WIN32
#ifndef MAINWIN
 #include <tchar.h>
#endif
#endif

#include "port.h"
#include "dba.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "error.h"
#include "dbaset.h"
#include "dbaparse.h"
#include "msghandl.h"
#include "dbadlg1.h"    // ps for ZEROINIT 07/04/96
#include "dll.h" 
#include "winutils.h"
#include "resource.h"

#include "extccall.h"

#ifdef DOUBLEBYTE
#include <windows.h>
#endif /* doublebyte */
 
extern LPTSTR VDBA20xCreateTableSyntax (LPTABLEPARAMS lpTS);


#define CONVERT_DATATYPE_INCOMPATIBLE   -1
#define CONVERT_DATATYPE_UNKNOWN         0
#define CONVERT_DATATYPE_COMPATIBLE      1
#define CONVERT_DATATYPE_TRANSLATED		 2

static int GetValidDataType( int ColType , LPGATEWAYCAPABILITIESINFO pGWInfo)
{
    switch (ColType) {
        case INGTYPE_LONGVARCHAR:
        case INGTYPE_BYTE:
        case INGTYPE_BYTEVAR:
        case INGTYPE_LONGBYTE:
        case INGTYPE_OBJKEY:
        case INGTYPE_TABLEKEY:
        case INGTYPE_SECURITYLBL:
        case INGTYPE_SHORTSECLBL:
        case INGTYPE_UNICODE_NCHR:
        case INGTYPE_UNICODE_NVCHR:
        case INGTYPE_UNICODE_LNVCHR:
        case INGTYPE_INT8:
        case INGTYPE_BIGINT:
            return CONVERT_DATATYPE_INCOMPATIBLE;
        case INGTYPE_CHAR:
        case INGTYPE_VARCHAR:
        case INGTYPE_INT4:
        case INGTYPE_INT2:
        case INGTYPE_DECIMAL:
        case INGTYPE_FLOAT8:
        case INGTYPE_FLOAT4:
        case INGTYPE_FLOAT:
        case INGTYPE_INTEGER:// equivalent to INGTYPE_INT4
            return CONVERT_DATATYPE_COMPATIBLE;
        case INGTYPE_DATE:
            if (pGWInfo->bDateTypeAvailable == TRUE)
                return CONVERT_DATATYPE_COMPATIBLE;
            else
                return CONVERT_DATATYPE_INCOMPATIBLE;
        case INGTYPE_MONEY:
        case INGTYPE_C:
        case INGTYPE_TEXT:
        case INGTYPE_INT1:
                return CONVERT_DATATYPE_TRANSLATED;
        default :
            return CONVERT_DATATYPE_UNKNOWN;
    }
}

/*
**
**  ConvertColType2GWType (LPTABLEPARAMS pDestTblInfo , LPTABLEPARAMS pSrcTblInfo ,                        
                           LPGATEWAYCAPABILITIESINFO pGWInfo)
**
**  - translate OpenIngres column type in Gateway column type
**      OpenIngres type | Gateway type 
**      ------------------------------
**      C               | char
**      text            | varchar
**      date            | date      if in pGWInfo->bDateTypeAvailable is TRUE
**      date            | No conversion - abort if in pGWInfo->bDateTypeAvailable is FALSE
**      money           | float ????????????????????????????
**      ----------------------------------------
**      char            | not changed
**      varchar         | not changed
**      int             | not changed
**      smallint        | not changed
**      decimal         | not changed
**      float           | not changed
**      real            | not changed
**      byte            | No conversion - abort
**      long varchar    | No conversion - abort
**      byte varying    | No conversion - abort
**      long byte       | No conversion - abort
**      object_key      | No conversion - abort
**      table_key       | No conversion - abort
**      nchar           | No conversion - abort
**      nvarchar        | No conversion - abort
**      long nvarchar   | No conversion - abort
**
**  - return
**      RES_ERR  - if GetValidDataType() return CONVERT_DATATYPE_INCOMPATIBLE
**               - if column data type is a date and pGWInfo->bDateTypeAvailable == FALSE
**
**      RES_SUCCESS - the data type column is valid or it is translated.
*/
static int ConvertColType2GWType (LPTABLEPARAMS pDestTblInfo ,
								  LPGATEWAYCAPABILITIESINFO pGWInfo,
								  LPTABLEPARAMS pSrcTblInfo)
{
    char ErrorMsg[1024];
    LPUCHAR lpType;
    LPUCHAR lpNewType;
    BOOL bAskQuestion = TRUE;
    LPCOLUMNPARAMS lpcol;
    LPOBJECTLIST lpList;
    int iTypeCol;

    lpList = pDestTblInfo->lpColumns;
    while(lpList)
    {
        ZEROINIT(ErrorMsg);
        lpcol = (LPCOLUMNPARAMS)lpList->lpObject;
        iTypeCol = GetValidDataType(lpcol->uDataType,pGWInfo);

        switch (iTypeCol){
            case CONVERT_DATATYPE_INCOMPATIBLE:
                lpType = GetColTypeStr(lpcol);
                //"Data Type %s, of Column %s, cannot be converted for \n"
                //"this Gateway. Drag and Drop operation is canceled."
                wsprintf(ErrorMsg,ResourceString(IDS_F_DRAG_DROP_COLUMN) ,
                               lpType,lpcol->szColumn);
                ESL_FreeMem(lpType);
                TS_MessageBox( TS_GetFocus(),ErrorMsg,NULL,MB_ICONSTOP | MB_OK | MB_TASKMODAL );
                return RES_ERR;
            case CONVERT_DATATYPE_TRANSLATED:
                {
                COLUMNPARAMS ColParDest;
                memset(&ColParDest,'\0',sizeof(COLUMNPARAMS));
                memcpy(&ColParDest,lpcol,sizeof(COLUMNPARAMS));
                pSrcTblInfo->bGWConvertDataTypes = TRUE;
                // translate ingres columns Data type into Gateway columns data type.
                switch (lpcol->uDataType) {
                    case INGTYPE_MONEY:
                        ColParDest.uDataType = INGTYPE_FLOAT8;
                        /*ColParDest.uDataType = INGTYPE_DECIMAL;
                        ColParDest.lenInfo.dwDataLen = 19;
                        ColParDest.lenInfo.decInfo.nPrecision = 19;
                        ColParDest.lenInfo.decInfo.nScale = 2;*/
                        break;
                    case INGTYPE_C:
                        ColParDest.uDataType = INGTYPE_CHAR;
                        break;
                    case INGTYPE_TEXT:
                        ColParDest.uDataType = INGTYPE_CHAR;
                        break;
                    case INGTYPE_INT1:
                        ColParDest.uDataType = INGTYPE_INT2;
                        break;
                }
                if (bAskQuestion) {
                    int iret;
                    lpType = GetColTypeStr(lpcol);
                    lpNewType = GetColTypeStr(&ColParDest);
                    //"Column %s will be converted from Type : %s \n"
                    //"to Type %s . Continue with this message and confirmation ?\n"
                    //" YES\t: Convert Column type and continue asking the question for other columns.\n"
                    //" NO\t: Convert Column type and don't ask the question for other columns.\n"
                    //" CANCEL\t: Abort the drag drop operation."

                    wsprintf(ErrorMsg,ResourceString(IDS_F_CONFIRM_CONVERT_COL),
                                      lpcol->szColumn,lpType,lpNewType );
                    iret = TS_MessageBox( TS_GetFocus(),ErrorMsg, "Warning",MB_ICONEXCLAMATION | MB_YESNOCANCEL | MB_TASKMODAL ); 
                    ESL_FreeMem(lpType);
                    ESL_FreeMem(lpNewType);
                    if ( iret == IDNO )
                        bAskQuestion = FALSE;
                    if ( iret == IDYES )
                        bAskQuestion = TRUE;
                    if ( iret == IDCANCEL )
                        return RES_ERR;
                }
                memmove(lpcol,&ColParDest,sizeof(COLUMNPARAMS));
                break;
                }
            case CONVERT_DATATYPE_UNKNOWN:
                //"Unknown Data Type for column name : %s\n"
                //"Drag and Drop operation is canceled."
                wsprintf(ErrorMsg,ResourceString(IDS_F_UNKNOWN_DATATYPE) ,lpcol->szColumn);
                TS_MessageBox( TS_GetFocus(),ErrorMsg,NULL,MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL );
                return RES_ERR;
        }
        lpList = lpList->lpNext;
    }
    return RES_SUCCESS;
}

/*
** DuplicateTableParamsStruct
**
**  Input : LPTABLEPARAMS desttbl : Destination Structure
**          LPTABLEPARAMS lpTS    : Source Structure
**  Output :
**
**  Return :
**      RES_ERR :   - duplicate list object failed. 
**                  - Memory allocation failed.
**      RES_SUCCESS
*/
static int DuplicateTableParamsStruct( LPTABLEPARAMS desttbl, LPTABLEPARAMS lpTS )
{
    LPOBJECTLIST lpSrc;
    lstrcpy ( desttbl->DBName,     lpTS->DBName);      // Database name
    lstrcpy ( desttbl->szNodeName, lpTS->szNodeName);  // Virtual node for drag&drop
    lstrcpy ( desttbl->objectname, lpTS->objectname);  // Table name
    lstrcpy ( desttbl->szSchema,   lpTS->szSchema);    // Schema

    // List of COLUMNPARAMS
    lpSrc = lpTS->lpColumns;
    while (lpSrc)
    {
        LPOBJECTLIST lpNew=AddListObjectTail(&desttbl->lpColumns,sizeof(COLUMNPARAMS));
        if (!lpNew)
            return RES_ERR;
        memcpy((char*) lpNew->lpObject,(char*)lpSrc->lpObject,sizeof(COLUMNPARAMS));
        // Not Available for gateway
        ((LPCOLUMNPARAMS)lpNew->lpObject)->nPrimary     = 0;
        ((LPCOLUMNPARAMS)lpNew->lpObject)->bUnique      = FALSE;
        ((LPCOLUMNPARAMS)lpNew->lpObject)->bDefault     = FALSE;
        ((LPCOLUMNPARAMS)lpNew->lpObject)->lpszDefSpec  = NULL;
        ((LPCOLUMNPARAMS)lpNew->lpObject)->lpszCheck    = NULL;
        ((LPCOLUMNPARAMS)lpNew->lpObject)->nSecondary   = 0;
        ((LPCOLUMNPARAMS)lpNew->lpObject)->bHasStatistics = FALSE;
        lpSrc=lpSrc->lpNext;
    }
    //used only in FreeAttachPointer
    desttbl->lpszTableCheck = NULL;
    desttbl->bJournaling = lpTS->bJournaling;  // TRUE if use journaling
    desttbl->bDuplicates = lpTS->bDuplicates;  // Allow duplicate rows
    desttbl->nSecAudit   = SECURITY_AUDIT_UNKOWN; // security audit type
    desttbl->lpOldReferences = NULL;
    desttbl->lpOldUnique = NULL;
    desttbl->bCreateAsSelect = lpTS->bCreateAsSelect;//TRUE: If Create table as selecte is to be generated
    if (lpTS->lpColumnHeader)   {
        int iSqlLen = x_strlen(lpTS->lpColumnHeader)+1;
        desttbl->lpColumnHeader = (LPSTR)ESL_AllocMem ( iSqlLen );
        if (!desttbl->lpColumnHeader)
            return RES_ERR;
        fstrncpy(desttbl->lpColumnHeader,lpTS->lpColumnHeader,iSqlLen);
    }
    // Create table as selecte ... only
    if (lpTS->lpSelectStatement)    {
        int iSqlLen = x_strlen(lpTS->lpSelectStatement)+1;
        desttbl->lpSelectStatement = (LPSTR)ESL_AllocMem ( iSqlLen );
        if (!desttbl->lpSelectStatement)
            return RES_ERR;
        fstrncpy(desttbl->lpSelectStatement,lpTS->lpSelectStatement,iSqlLen);
    }
    desttbl->bCreate  = FALSE; // TRUE if creating a table.
    desttbl->bPrimKey = FALSE; // TRUE if one col has a rank in prim.key >0
    return RES_SUCCESS;
}


int DBAAddObject(LPUCHAR lpVirtNode,int iobjecttype, void *lpparameters, ...)
{
	// never invoked with a session hdl (4th parameter), since in 
	// this case DBAAddObjectLL is supposed to be called directly
	return Task_DbaAddObjectInteruptible   ((LPCTSTR) lpVirtNode,  iobjecttype, lpparameters, -1);
	//return  DBAAddObject(LPUCHAR lpVirtNode,int iobjecttype, void *lpparameters);

}

int DBAAddObjectLL(LPUCHAR lpVirtNode,int iobjecttype, void *lpparameters, ...)

/******************************************************************
* Function : Add a new object according to the input parameters   *
* Parameters :                                                    *
*        lpVirtNode : Virtual node to connect to or NULL if a     *
*                    Session Handle is passed(va arg).            *
*        iobjecttype : type of the object to be added.            *
*        lpparameters : Properties and parent of the object to be *
*                    added.                                       *
*        ... va_arg : Handle of the session to re-activate, this  *
*                    parameter is ignored if lpVirtonode is not   *
*                    NULL.                                        *
* returns :                                                       *
*     Either RES_SUCCESS, add was OK or RES_ERR if problems       *
******************************************************************/

{
   va_list ap;          /* argument pointer */ 
   char connectname[2*MAXOBJECTNAME+2+1];
   int iret, Sesshdl;
   char bufrequest[(MAXOBJECTNAME * 3) + 400];
   char bufcomma[2];
   UCHAR *pSQLstm;
   int ilocsession;
   if (!lpparameters)
      return myerror(ERR_ADDOBJECT);

   if (!lpVirtNode)   {             // we are not called with a virtual ... 
      va_start(ap, lpparameters);   // ... node but with a ...
      Sesshdl=va_arg(ap, int);     // ... session handle instead.
      va_end(ap);
   }


   switch (iobjecttype) {
      case OT_DATABASE:
        {
         LPCREATEDB pCreateDb = (LPCREATEDB) lpparameters;
         char buff[350];

         wsprintf(buff,"createdb ");

         if (pCreateDb->bUnicodeDatabaseNFD)
             x_strcat(buff, "-n ");

         if (pCreateDb->bUnicodeDatabaseNFC)
             x_strcat(buff, "-i ");

         if (pCreateDb->bReadOnly)
         {
            if (pCreateDb->LocDatabase[0] != 0)
            {
                x_strcat(buff, pCreateDb->szDBName);
                x_strcat(buff, " -r");
                x_strcat(buff,  pCreateDb->LocDatabase);
            }
            else
            {
                x_strcat(buff, pCreateDb->szDBName);
                x_strcat(buff, " -rii_database");
            }
         }
         else
           x_strcat(buff, pCreateDb->szDBName);

         // Star management
         if (pCreateDb->bDistributed)
         {
           x_strcat(buff, "/star");
           if (pCreateDb->szCoordName[0])
           {
             x_strcat(buff, " ");
             x_strcat(buff, pCreateDb->szCoordName);
           }
         }

         //store the location name for database
         if (pCreateDb->LocDatabase[0] != 0 && !pCreateDb->bReadOnly)
         {
           x_strcat(buff," -d");
           x_strcat(buff,pCreateDb->LocDatabase);
         }

         //store the location name for checkpoint
         if (pCreateDb->LocCheckpoint[0] != 0 && !pCreateDb->bReadOnly)
         {
           x_strcat(buff," -c");
           x_strcat(buff,pCreateDb->LocCheckpoint);
         }

         //store the location name for journal
         if (pCreateDb->LocJournal[0] != 0 && !pCreateDb->bReadOnly)
         {
           x_strcat(buff," -j");
           x_strcat(buff,pCreateDb->LocJournal);
         }

         //store the location name for dump
         if (pCreateDb->LocDump[0] != 0 && !pCreateDb->bReadOnly)
         {
           x_strcat(buff," -b");
           x_strcat(buff,pCreateDb->LocDump);
         }

         //store the location name for work
         if (pCreateDb->LocWork[0] != 0 && !pCreateDb->bReadOnly)
         {
           x_strcat(buff," -w");
           x_strcat(buff,pCreateDb->LocWork);
         }

         // store the selected products
         if (pCreateDb->bGenerateFrontEnd)
         {
           if (!pCreateDb->bAllFrontEnd)
           {
              x_strcat(buff," -f");
              x_strcat(buff,pCreateDb->Products);
           }
           else
             ;    // nothing means all
         }
         else
         {
             if (!pCreateDb->bReadOnly)
                x_strcat(buff," -fnofeclients");
         }

         //store the selected language
         if (pCreateDb->Language[0] != 0)
           {
           x_strcat (buff," -l");
           x_strcat (buff,pCreateDb->Language);
           }

         // checked private or public
           if (pCreateDb->bPrivate)
               x_strcat(buff," -p");

         //store the selected user
         if (pCreateDb->szUserName[0] != 0)
           {
           x_strcat (buff," -u");
           x_strcat (buff,pCreateDb->szUserName);
           }

         //store the catalogs page size
         if (pCreateDb->szCatalogsPageSize[0]!=0)
           {
           x_strcat (buff," -page_size=");
           x_strcat (buff,pCreateDb->szCatalogsPageSize);
           }

//         TS_MessageBox( TS_GetFocus(),buff,NULL,MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL );
         //"Creating Database %s::%s"
         wsprintf(bufrequest,ResourceString(IDS_TITLE_CREATE_DATABASE),lpVirtNode,
                             pCreateDb->szDBName);

         execrmcmd(lpVirtNode,buff,bufrequest);
         return RES_SUCCESS;
         }


         break;

      case OT_LOCATION:
         {
         struct LocationParams * pLocationParams =
                     (struct LocationParams * ) lpparameters;
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
         if (!*(pLocationParams->AreaName)) 
            return RES_ERR;

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::iidbdb",lpVirtNode);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         wsprintf(bufrequest,"create location %s with area='%s',",
                  QuoteIfNeeded(pLocationParams->objectname), pLocationParams->AreaName);


         pLocationParams->bLocationUsage[LOCATIONNOUSAGE]=TRUE;
         for (usagetype=usagetypes;usagetype->pchar;usagetype++) {
            if (pLocationParams->bLocationUsage[usagetype->item]) 
               pLocationParams->bLocationUsage[LOCATIONNOUSAGE]=FALSE;
         }

         if (pLocationParams->bLocationUsage[LOCATIONNOUSAGE])
            x_strcat(bufrequest,"nousage");
         else {
            x_strcat(bufrequest," usage = (");
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
         if (GetOIVers() >= OIVERS_26)
         {
            if (pLocationParams->iRawPct > 0)
            {
               TCHAR tcTmpBuff[30];
               _itoa(pLocationParams->iRawPct,tcTmpBuff,10);
               x_strcat(bufrequest,", rawpct=");
               x_strcat(bufrequest, tcTmpBuff);
            }
         }
         iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;

      case OT_USER:
         {
            struct UserParams * pUserParams =
                        (struct UserParams * ) lpparameters;

            if (!lpVirtNode)
               iret=ActivateSession(Sesshdl);
            else {
               wsprintf(connectname,"%s::iidbdb",lpVirtNode);
               iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
            }
            if (iret !=RES_SUCCESS)
               return iret;

            iret=BuildSQLCreUsr(&pSQLstm,pUserParams);

            if (iret!=RES_SUCCESS)
               break;

            iret=ExecSQLImm(pSQLstm,FALSE, NULL, NULL, NULL );
            ESL_FreeMem((void *) pSQLstm);
            if (iret!=RES_SUCCESS)
               break;        

            if (pUserParams->lpfirstdb) {     // if databases are to be granted
               iret=BuildSQLDBGrant(&pSQLstm,OT_USER,pUserParams->ObjectName,
                  pUserParams->lpfirstdb);
               if (iret!=RES_SUCCESS)
                  break;
               iret=ExecSQLImm(pSQLstm,FALSE, NULL, NULL, NULL);
               ESL_FreeMem((void *) pSQLstm);
            }
            break;
         }
      case OT_PROFILE:
         {
            struct ProfileParams * pProfileParams =
                        (struct ProfileParams * ) lpparameters;

            if (!lpVirtNode)
               iret=ActivateSession(Sesshdl);
            else {
               wsprintf(connectname,"%s::iidbdb",lpVirtNode);
               iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
            }
            if (iret !=RES_SUCCESS)
               return iret;

            iret=BuildSQLCreProf(&pSQLstm,pProfileParams);

            if (iret!=RES_SUCCESS)
               break;

            iret=ExecSQLImm(pSQLstm,FALSE, NULL, NULL, NULL );
            ESL_FreeMem((void *) pSQLstm);

            break;        
         }
      case OT_GROUP:
         {
         struct GroupParams * pGroupParams =
                     (struct GroupParams * ) lpparameters;
         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::iidbdb",lpVirtNode);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         iret=BuildSQLCreGrp(&pSQLstm,pGroupParams);

         if (iret!=RES_SUCCESS)
            break;

         iret=ExecSQLImm(pSQLstm,FALSE, NULL, NULL, NULL);
         ESL_FreeMem((void *) pSQLstm);
         if (iret!=RES_SUCCESS)
            break;        
         if (pGroupParams->lpfirstdb) {     // if databases are to be granted
            iret=BuildSQLDBGrant(&pSQLstm,OT_GROUP,pGroupParams->ObjectName,
               pGroupParams->lpfirstdb);
            if (iret!=RES_SUCCESS)
               break;
            iret=ExecSQLImm(pSQLstm,FALSE, NULL, NULL, NULL);
            ESL_FreeMem((void *) pSQLstm);
         }
         }
         break;

      case OT_GROUPUSER:
         {
         LPGROUPUSERPARAMS pGroupUser = (LPGROUPUSERPARAMS) lpparameters;
         if (!pGroupUser->GroupName[0] ||
             !pGroupUser->ObjectName[0]  ) 
            return RES_ERR;

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::iidbdb",lpVirtNode);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         wsprintf(bufrequest,"alter group %s add users ( %s )",
                  QuoteIfNeeded(pGroupUser->GroupName),QuoteIfNeeded(pGroupUser->ObjectName));

         iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;

      case OT_ROLE:
         {
         LPROLEPARAMS pRoleParams = (LPROLEPARAMS) lpparameters;
         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::iidbdb",lpVirtNode);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;
         iret=BuildSQLCreRole(&pSQLstm,pRoleParams);
         if (iret!=RES_SUCCESS)
            break;

         iret=ExecSQLImm(pSQLstm,FALSE, NULL, NULL, NULL);
         ESL_FreeMem((void *) pSQLstm);

         if (iret!=RES_SUCCESS)
            break;        
         if (pRoleParams->lpfirstdb) {     // if databases are to be granted
            iret=BuildSQLDBGrant(&pSQLstm,OT_ROLE,pRoleParams->ObjectName,
               pRoleParams->lpfirstdb);
            if (iret!=RES_SUCCESS)
               break;
            iret=ExecSQLImm(pSQLstm,FALSE, NULL, NULL, NULL);
            ESL_FreeMem((void *) pSQLstm);
         }
         }       
         break;
      case OT_RULE:
         {
            int freebytes;
            struct RuleParams * pRuleParams =
                     (struct RuleParams * ) lpparameters;
            if (!lpVirtNode)
                iret=ActivateSession(Sesshdl);
            else {
                wsprintf(connectname,"%s::%s",lpVirtNode,pRuleParams->DBName);
                iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
            }
            if (iret !=RES_SUCCESS)
                return iret;
            freebytes=SEGMENT_TEXT_SIZE;
            pSQLstm = ESL_AllocMem(SEGMENT_TEXT_SIZE);
            if (!pRuleParams->bOnlyText) {
               if (pSQLstm)
                 pSQLstm=ConcatStr(pSQLstm, "create rule ", &freebytes);
               if (pSQLstm)
                 pSQLstm=ConcatStr(pSQLstm, (LPUCHAR)QuoteIfNeeded(pRuleParams->RuleName), &freebytes);
               if (pSQLstm)
                 pSQLstm=ConcatStr(pSQLstm, " ", &freebytes);
               if (pSQLstm)
                 pSQLstm=ConcatStr(pSQLstm, pRuleParams->lpTableCondition, &freebytes);
               if (pSQLstm)
                 pSQLstm=ConcatStr(pSQLstm, " execute procedure ", &freebytes);
               if (pSQLstm)
                 pSQLstm=ConcatStr(pSQLstm, pRuleParams->lpProcedure, &freebytes);
            //    wsprintf(bufrequest,"create rule %s %s execute procedure %s ",
            //        pRuleParams->RuleName, 
            //        pRuleParams->lpTableCondition,
            //        pRuleParams->lpProcedure);
            }
            else {
               if (pSQLstm)
                 pSQLstm=ConcatStr(pSQLstm, pRuleParams->lpRuleText, &freebytes);
                //wsprintf(bufrequest,"%s", pRuleParams->lpRuleText);
            }
            if (!pSQLstm) {
               ReleaseSession(ilocsession, RELEASE_ROLLBACK);
               myerror(ERR_ALLOCMEM);
               return RES_ERR;
            }
            iret=ExecSQLImm(pSQLstm,FALSE , NULL, NULL, NULL);
            ESL_FreeMem((void *) pSQLstm);
            pSQLstm = NULL;
         }
         break;

      case OT_TABLE: {
         BOOL bGenericGateway = FALSE;
         if (GetOIVers() == OIVERS_NOTOI)
            bGenericGateway = TRUE;
         if (bGenericGateway) {
            TABLEPARAMS desttbl;
            GATEWAYCAPABILITIESINFO GWInfo;
            LPTSTR lpSQL;
            struct TableParams *pTableParams = (struct TableParams *) lpparameters;
            pTableParams->bGWConvertDataTypes = FALSE;
            if (!lpVirtNode)
               iret=ActivateSession(Sesshdl);
            else {
               wsprintf(connectname,"%s::%s",lpVirtNode,pTableParams->DBName);
               iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
            }
            if (iret !=RES_SUCCESS)
               return iret;
            memset(&desttbl,'\0',sizeof(TABLEPARAMS));
            if (GetCapabilitiesInfo(pTableParams, &GWInfo ) != RES_SUCCESS ) {
                iret=RES_ERR;
                break;
            }
            if ( DuplicateTableParamsStruct(&desttbl , pTableParams ) == RES_ERR ) {
               FreeAttachedPointers (&desttbl, OT_TABLE);
               iret=RES_ERR;
               break;
            }
            if ( ConvertColType2GWType (&desttbl, &GWInfo ,pTableParams) == RES_ERR ) { // verify data type.
               FreeAttachedPointers (&desttbl, OT_TABLE);
               iret=RES_ERR;
               break;
            }
            lpSQL = VDBA20xCreateTableSyntax (&desttbl);
            FreeAttachedPointers (&desttbl, OT_TABLE);
            if (!lpSQL)
               break;
            iret=ExecSQLImm(lpSQL, FALSE, NULL, NULL, NULL);
            if (iret==RES_ENDOFDATA) 
               iret=RES_SUCCESS;
            ESL_FreeMem((void *) lpSQL);
            break;
         }

         {
         LPTSTR lpSQL;
         struct TableParams *pTableParams =
            (struct TableParams *) lpparameters;
         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pTableParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         lpSQL = VDBA20xCreateTableSyntax (pTableParams);
         if (!lpSQL)
             break; 
#if 0
         iret=BuildSQLCreTbl(&pSQLstm,pTableParams);
         if (iret!=RES_SUCCESS)
            break;        
         iret=ExecSQLImm(pSQLstm,FALSE, NULL, NULL, NULL);
         if (iret==RES_ENDOFDATA) 
            iret=RES_SUCCESS;
         ESL_FreeMem((void *) pSQLstm);
#else
         if (iret!=RES_SUCCESS)
            break;        
         iret=ExecSQLImm(lpSQL, FALSE, NULL, NULL, NULL);
         if (iret==RES_ENDOFDATA) 
            iret=RES_SUCCESS;
         ESL_FreeMem((void *) lpSQL);
         //
         // UK-25JUNE1997: Add a piece of code to modify table structure if needed.
         // If the original table has been modified to HEAP with aditional parameters,
         // then the statement below will not be executed.
         if (iret == RES_SUCCESS && 
            pTableParams->StorageParams.nStructure != 0 && 
            pTableParams->StorageParams.nStructure != IDX_HEAP)
         {
             iret = ModifyChgStorage (&(pTableParams->StorageParams));
             if (iret==RES_ENDOFDATA) 
                 iret=RES_SUCCESS;
         }
         if (iret!=RES_SUCCESS)
             break;        
#endif
         }
      }
      break;
      case OT_VIEW:
         {
         struct ViewParams * pViewParams =
                     (struct ViewParams * ) lpparameters;

         if (pViewParams->bOnlyText) {
            if (!pViewParams->lpViewText)
               return RES_ERR;
            if (!*(pViewParams->lpViewText))
               return RES_ERR;
         }
         else {
            if (!pViewParams->lpSelectStatement)
               return RES_ERR;
            if (!*(pViewParams->lpSelectStatement))
               return RES_ERR;
         }

         if (!*(pViewParams->DBName))
            return RES_ERR;
         if (!*(pViewParams->objectname))
            return RES_ERR;
         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pViewParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;
         iret=BuildSQLCreView(&pSQLstm,pViewParams);
         if (iret!=RES_SUCCESS)
            break;
         iret=ExecSQLImm(pSQLstm,FALSE , NULL, NULL, NULL);
            ESL_FreeMem((void *) pSQLstm);
         }       
         break;
      case OT_INTEGRITY:
         {
            struct IntegrityParams * pIntegrityParams =
                        (struct IntegrityParams * ) lpparameters;

            if (!lpVirtNode)
               iret=ActivateSession(Sesshdl);
            else {
               wsprintf(connectname,"%s::%s",lpVirtNode,
                        pIntegrityParams->DBName);
               iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
            }
            if (iret !=RES_SUCCESS)
               return iret;

            if (pIntegrityParams->bOnlyText) {
               if (!pIntegrityParams->lpIntegrityText) 
                  iret=RES_ERR;
               else if (!(*(pIntegrityParams->lpIntegrityText)))
                  iret=RES_ERR;
               else 
                  iret = ExecSQLImm(pIntegrityParams->lpIntegrityText,
                                    FALSE, NULL, NULL, NULL);
            }
            else {
               int freebytes;
               if (!pIntegrityParams->lpSearchCondition)
                  return RES_ERR;

               freebytes=SEGMENT_TEXT_SIZE;
               pSQLstm = ESL_AllocMem(SEGMENT_TEXT_SIZE);
               if (pSQLstm)
                 pSQLstm=ConcatStr(pSQLstm, "create integrity on ", &freebytes);
               if (pSQLstm)
               {
                 pSQLstm=ConcatStr(pSQLstm, (LPUCHAR)QuoteIfNeeded(pIntegrityParams->TableOwner), &freebytes);
                 pSQLstm=ConcatStr(pSQLstm, ".", &freebytes);
                 pSQLstm=ConcatStr(pSQLstm, (LPUCHAR)QuoteIfNeeded(pIntegrityParams->TableName), &freebytes);
               }
               if (pSQLstm)
                 pSQLstm=ConcatStr(pSQLstm, " is ", &freebytes);
               if (pSQLstm)
                 pSQLstm=ConcatStr(pSQLstm, pIntegrityParams->lpSearchCondition, &freebytes);

               if (!pSQLstm) {
                 ReleaseSession(ilocsession, RELEASE_ROLLBACK);
                 myerror(ERR_ALLOCMEM);
                 return RES_ERR;
               }

               iret = ExecSQLImm(pSQLstm,FALSE, NULL, NULL, NULL);
               ESL_FreeMem((void *) pSQLstm);
               pSQLstm = NULL;
            }

         }
         break;
      case OT_DBEVENT:
         {
         struct DBEventParams * pDBEventParams =
                     (struct DBEventParams * ) lpparameters;

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pDBEventParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         sprintf (bufrequest,"create dbevent %s",pDBEventParams->ObjectName);
         iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;

      case OT_REPLIC_REGTABLE:
         {
         struct ReplRegTableParams * pReplRegTableParams =
                     (struct ReplRegTableParams * ) lpparameters;

         myerror(ERR_REPLICCDDS); // this code should no more be used

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pReplRegTableParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         sprintf (bufrequest,
               "insert into dd_registered_tables "
                  "(tablename, tables_created, columns_registered,"
                  " rules_created, replication_type, dd_routing,"
                  " rule_lookup_table, priority_lookup_table) "
               "values ('%s', '%s', '%s', '%s', %d, %d, '%s', '%s')", 
               pReplRegTableParams->TableName,
               pReplRegTableParams->TablesCreated,
               pReplRegTableParams->ColRegistered,
               pReplRegTableParams->RulesCreated,
               pReplRegTableParams->ReplType,
               pReplRegTableParams->ddRouting,
               pReplRegTableParams->RuleLookupTbl,
               pReplRegTableParams->PrioLookupTbl);

         iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;
      case OT_REPLIC_CONNECTION:
         {
         struct ReplConnectParams * pReplConnectParams =
                     (struct ReplConnectParams * ) lpparameters;
         //ZEROINIT(bufrequest);
         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pReplConnectParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         if (pReplConnectParams->bLocalDb == TRUE &&
             pReplConnectParams->bAlter   == FALSE) {
           sprintf (bufrequest,
                    "insert into dd_installation "
                    "(database_id, database_no, node_name, "
                    "target_type, dba_comment) "
                    "values ('', %d, '%s', %d,'%s')", 
                          pReplConnectParams->nNumber,
                          pReplConnectParams->szVNode,
                          pReplConnectParams->nType,
                          pReplConnectParams->szDescription);
         
          iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
          if (iret!=RES_SUCCESS)
            break;
          sprintf (bufrequest,
                  "insert into dd_connections "
                  "(database_no, node_name,  database_name, "
                  "server_role, target_type, dba_comment) "
                  "values (%d, '%s', '%s', %d, %d, '%s')", 
                      pReplConnectParams->nNumber,
                      pReplConnectParams->szVNode,
                      pReplConnectParams->szDBName,
                      pReplConnectParams->nServer,
                      pReplConnectParams->nType,
                      pReplConnectParams->szDescription);
           iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
           if (iret!=RES_SUCCESS)
              break;
           iret=UpdCfgChanged(pReplConnectParams->nReplicVersion);
           break;
         }
         if (pReplConnectParams->bAlter   == FALSE &&
             pReplConnectParams->bLocalDb == FALSE ||
             pReplConnectParams->bAlter   == TRUE &&
             pReplConnectParams->bLocalDb == FALSE ) {
            if (pReplConnectParams->nReplicVersion==REPLIC_V10) {
             sprintf (bufrequest,
                   "insert into dd_connections "
                     "(database_no, node_name,  database_name, "
                     "server_role, target_type, dba_comment) "
                   "values (%d, '%s', '%s', %d, %d, '%s')", 
                   pReplConnectParams->nNumber,
                   pReplConnectParams->szVNode,
                   pReplConnectParams->szDBName,
                   pReplConnectParams->nServer,
                   pReplConnectParams->nType,
                   pReplConnectParams->szDescription);
            }
            else  {
               sprintf (bufrequest,
                   "insert into dd_connections "
                     "(database_no, node_name,  database_name, "
                     "server_role, target_type, dba_comment,database_owner) "
                   "values (%d, '%s', '%s', %d, %d, '%s' ,'%s')", 
                   pReplConnectParams->nNumber,
                   pReplConnectParams->szVNode,
                   pReplConnectParams->szDBName,
                   pReplConnectParams->nServer,
                   pReplConnectParams->nType,
                   pReplConnectParams->szDescription,
                   pReplConnectParams->szDBOwner);


            }

         }
         iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         if (iret!=RES_SUCCESS)
            break;
         iret=UpdCfgChanged(pReplConnectParams->nReplicVersion);
         }
         break;
      case OT_REPLIC_CONNECTION_V11:
         {
         struct ReplConnectParams * pReplConnectParams =
                     (struct ReplConnectParams * ) lpparameters;
        
         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pReplConnectParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         if (pReplConnectParams->bLocalDb == TRUE &&
             pReplConnectParams->bAlter   == FALSE) {
           wsprintf (bufrequest,
                    "insert into dd_databases "
                    "(database_no, vnode_name, "
                    "database_name, dbms_type, "
                    "local_db, remark, "
                    "database_owner) "
                    "values (%d, '%s', '%s', '%s', 1,'%s','%s')", 
                          pReplConnectParams->nNumber,
                          pReplConnectParams->szVNode,
                          pReplConnectParams->szDBName,
                          pReplConnectParams->szDBMStype,
                          pReplConnectParams->szDescription,
                          pReplConnectParams->szDBOwner);
         
           iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
           if (iret!=RES_SUCCESS)
              break;
           iret=UpdCfgChanged(pReplConnectParams->nReplicVersion);
           break;
         }
         if (pReplConnectParams->bAlter   == FALSE &&
             pReplConnectParams->bLocalDb == FALSE ||
             pReplConnectParams->bAlter   == TRUE &&
             pReplConnectParams->bLocalDb == FALSE ) {
             wsprintf (bufrequest,
                   "insert into dd_databases "
                   "(database_no, vnode_name, "
                   "database_name, dbms_type, "
                   "local_db, remark, database_owner, config_changed) "
                   "values (%d, '%s', '%s', '%s', 0, '%s' ,'%s' ,DATE_GMT('now'))", 
                   pReplConnectParams->nNumber,
                   pReplConnectParams->szVNode,
                   pReplConnectParams->szDBName,
                   pReplConnectParams->szDBMStype,
                   pReplConnectParams->szDescription,
                   pReplConnectParams->szDBOwner );
             }
         iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         if (iret!=RES_SUCCESS)
            break;
         iret=UpdCfgChanged(pReplConnectParams->nReplicVersion);
         }
         break;
      case OTLL_REPLIC_CDDS:
         {
         struct ReplDistribParams * pReplDistribParams =
                     (struct ReplDistribParams * ) lpparameters;

         myerror(ERR_REPLICCDDS); // this code should no more be used

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pReplDistribParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         sprintf (bufrequest,
               "insert into dd_distribution "
                  "(localdb, source, target, dd_routing)"
               "values (%d, %d, %d, %d )", 
               pReplDistribParams->Local_DBno,
               pReplDistribParams->Source_DBno,
               pReplDistribParams->Target_DBno,
               pReplDistribParams->ddRouting);

         iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;
      case OT_REPLIC_MAILUSER:
         {
         struct ReplMailParams * pReplMailParams =
                     (struct ReplMailParams * ) lpparameters;

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pReplMailParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         sprintf (bufrequest,
               "insert into dd_mail_alert "
               "values ('%s')", 
               pReplMailParams->szMailText);

         iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;
      // JFS Begin
      case OT_REPLIC_CDDS:
       {
         UCHAR     buf[100];
         LPUCHAR   pc;
         int       sess;
         LPREPCDDS lpcdds= (LPREPCDDS)lpparameters;

         if (!lpVirtNode){
            iret=ActivateSession(Sesshdl);
            sess=Sesshdl;
         }
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,lpcdds->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
            sess=ilocsession;
         }
         if (iret !=RES_SUCCESS)
            return iret;
         fstrncpy(buf,GetSessionName(sess), sizeof(buf));
         pc = x_strstr(buf,"::");
         if (pc)
            *pc='\0';
         iret=BuildSQLCDDS (NULL, lpcdds, buf, lpcdds->DBName, sess);
         if (iret!=RES_SUCCESS)
            break;
         iret=UpdCfgChanged(lpcdds->iReplicType);
      }
      break;
      case OT_REPLIC_CDDS_V11:
      {
         UCHAR     buf[100];
         LPUCHAR   pc;
         int       sess;
         LPREPCDDS lpcdds= (LPREPCDDS)lpparameters;

         if (!lpVirtNode){
            iret=ActivateSession(Sesshdl);
            sess=Sesshdl;
         }
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,lpcdds->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
            sess=ilocsession;
         }
         if (iret !=RES_SUCCESS)
            return iret;
         fstrncpy(buf,GetSessionName(sess), sizeof(buf));
         pc = x_strstr(buf,"::");
         if (pc)
            *pc='\0';
         iret=BuildSQLCDDSV11 (NULL, lpcdds, buf, lpcdds->DBName, sess);
         if (iret!=RES_SUCCESS)
            break;
         iret=UpdCfgChanged(lpcdds->iReplicType);
      }
      break;
      // JFS End
      case OT_PROCEDURE:
         {
         struct ProcedureParams * pProcedureParams =
                     (struct ProcedureParams * ) lpparameters;

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pProcedureParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         iret=BuildSQLCreProc(&pSQLstm,pProcedureParams);

         if (iret!=RES_SUCCESS)
            break;

         iret=ExecSQLImm(pSQLstm,FALSE, NULL, NULL, NULL);
         ESL_FreeMem((void *) pSQLstm);
         }

        break;
      case OT_SEQUENCE:
         {
         struct SequenceParams * pSequenceParams =
                     (struct SequenceParams * ) lpparameters;

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pSequenceParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         iret=BuildSQLCreSeq(&pSQLstm,pSequenceParams);

         if (iret!=RES_SUCCESS)
            break;

         iret=ExecSQLImm(pSQLstm,FALSE, NULL, NULL, NULL);
         ESL_FreeMem((void *) pSQLstm);
         }

        break;


      case OT_INDEX:
         {
			if (GetOIVers() == OIVERS_NOTOI) {
				struct IndexParams *pIndexParams =
					(struct IndexParams *) lpparameters;

				if (!lpVirtNode)
					iret=ActivateSession(Sesshdl);
				else {
					wsprintf(connectname,"%s::%s",lpVirtNode,pIndexParams->DBName);
					iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);

				}
				if (iret !=RES_SUCCESS)
					return iret;

				iret=BuildSQLCreIdx(&pSQLstm,pIndexParams);
				if (iret!=RES_SUCCESS)
					break;        

				iret=ExecSQLImm(pSQLstm,FALSE, NULL, NULL, NULL);
				ESL_FreeMem((void *) pSQLstm);
			}
			else {
				DMLCREATESTRUCT* pCreateStruct = (DMLCREATESTRUCT*)lpparameters;
				if (!lpVirtNode)
					iret = ActivateSession (Sesshdl);
				else 
				{
					wsprintf(connectname,"%s::%s",lpVirtNode, pCreateStruct->tchszDatabase);
					iret = Getsession(connectname, SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
				}
				if (iret != RES_SUCCESS)
					return iret;
				if (pCreateStruct->lpszStatement)
					iret = ExecSQLImm((LPUCHAR)pCreateStruct->lpszStatement, FALSE, NULL, NULL, NULL);
				else
				if (pCreateStruct->lpszStatement2)
				{
					iret = ExecSQLImm((LPUCHAR)pCreateStruct->lpszStatement2, FALSE, NULL, NULL, NULL);
					ESL_FreeMem ((LPVOID)pCreateStruct->lpszStatement2);
				}
				else
				{
					iret = RES_ERR; // OLD CODE: BuildSQLCreIdx failed. ?
					break;
				}
			}
            /* --
            when checking the sql return code on a create index the +100 
            return code (RES_ENDOFDATA) means that the index was created
            on an empty table : this is okay  */ 

            if (iret==RES_ENDOFDATA) 
                iret=RES_SUCCESS;
            //  else                    // TO BE FINISHED: LINES REMOVED BECAUSE
            //      if (iret!=RES_SUCCESS) // RETURN IS NOT ALLOWED (RELEASESESSION
            //         return iret;        // is needed. DOne if "break"                       

         }
         break;
      case OTLL_SECURITYALARM:
         {
         struct SecurityAlarmParams *pSecurityAlarmParams =
                     (struct SecurityAlarmParams *) lpparameters;

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pSecurityAlarmParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;
         iret=BuildSQLSecAlrm(&pSQLstm,
                     (struct SecurityAlarmParams *) lpparameters);
         if (iret!=RES_SUCCESS)
            break;
         iret=ExecSQLImm(pSQLstm,FALSE , NULL, NULL, NULL);
         ESL_FreeMem((void *) pSQLstm);
         }
         break;
      case OTLL_GRANT:
      {
         struct GrantParams *pGrantParams =
                  (struct GrantParams *) lpparameters;

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            if (pGrantParams->ObjectType==OT_DATABASE  ||
                pGrantParams->ObjectType==OT_ROLE) 
               wsprintf(connectname,"%s::iidbdb",lpVirtNode);
            else      
               wsprintf(connectname,"%s::%s",lpVirtNode,pGrantParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;
         iret=BuildSQLGrant(&pSQLstm,
                  (struct GrantParams *) lpparameters);
         if (iret!=RES_SUCCESS)
            break;
         iret=ExecSQLImm(pSQLstm,FALSE , NULL, NULL, NULL);
         ESL_FreeMem((void *) pSQLstm);
      }
      break;

      case OT_SYNONYM:
         {
         struct SynonymParams * pSynonymParams =
                     (struct SynonymParams * ) lpparameters;

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pSynonymParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;
         wsprintf (bufrequest,"create synonym %s for %s.%s",
                       QuoteIfNeeded(pSynonymParams->ObjectName),
                       QuoteIfNeeded(pSynonymParams->RelatedObjectOwner),
                       QuoteIfNeeded(pSynonymParams->RelatedObject));

         iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);

         }
         break;

      default:
         return RES_ENDOFDATA;
         break;
   }
      if (!lpVirtNode)
            return iret;

      if (iret==RES_SUCCESS)
         iret=ReleaseSession(ilocsession, RELEASE_COMMIT);
      else 
         ReleaseSession(ilocsession, RELEASE_ROLLBACK);
      return iret;
}


int DBADropObject(LPUCHAR lpVirtNode,int iobjecttype,void *lpparameters, ...)

/******************************************************************
* Function : Drop an object according to the input parameters     *
* Parameters :                                                    *
*        lpVirtNode : Virtual node to connect to or NULL if a     *
*                    Session Handle is passed(va arg).            *
*        iobjecttype : type of the object to be dropped.          *
*        lpparameters : parents of the object to be droped.       *
*        ... va_arg : Handle of the session to re-activate, this  *
*                    parameter is ignored if lpVirtonode is not   *
*                    NULL.                                        *
* returns :                                                       *
*     Either RES_SUCCESS, drop was OK or RES-ERR if problems      *
******************************************************************/

{
   va_list ap;          /* argument pointer */ 
   char connectname[2*MAXOBJECTNAME+2+1];
   int iret, Sesshdl;
   UCHAR *pSQLstm;
   char bufrequest[2*MAXOBJECTNAME+200];
   int ilocsession;

   if (!lpparameters)
      return myerror(ERR_ADDOBJECT);

   if (!lpVirtNode)   {             // we are no called with a virtual ... 
      va_start(ap, lpparameters);   // ... node but with a ...
      Sesshdl=va_arg(ap, int);     // ... session handle instead.
      va_end(ap);
   }

   switch (iobjecttype) {
      case OT_DATABASE:
         return RES_ERR;
         break;
      case OT_LOCATION:
         {
         struct LocationParams * pLocationParams =
                     (struct LocationParams * ) lpparameters;
         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            sprintf(connectname,"%s::iidbdb",lpVirtNode);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;
         wsprintf(bufrequest,"drop location %s",QuoteIfNeeded(pLocationParams->objectname));
         iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);

         }
         break;
      case OT_RULE:
         {
         //char szObjNameQuoted[MAXOBJECTNAME*2];
         struct RuleParams * pRuleParams =
                     (struct RuleParams * ) lpparameters;
         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pRuleParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;
         wsprintf(bufrequest,"drop rule %s.%s ", QuoteIfNeeded(pRuleParams->RuleOwner),
                                                 QuoteIfNeeded(pRuleParams->RuleName));
         iret=ExecSQLImm(bufrequest,FALSE , NULL, NULL, NULL);
         }
         break;
      case OT_INTEGRITY:
         {
         struct IntegrityParams * pIntegrityParams =
                     (struct IntegrityParams * ) lpparameters;

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pIntegrityParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         sprintf (bufrequest,"drop integrity on %s.%s %d",
                              QuoteIfNeeded(pIntegrityParams->TableOwner),
                              QuoteIfNeeded(pIntegrityParams->TableName),
                              pIntegrityParams->number);

         iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;
      case OT_USER:
         {
         struct UserParams * pUserParams =
                     (struct UserParams * ) lpparameters;
         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::iidbdb",lpVirtNode);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;
         wsprintf(bufrequest,"drop user %s", QuoteIfNeeded(pUserParams->ObjectName));
         iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;
      case OT_PROFILE:
         {
         TCHAR tchszMode[16];
         LPPROFILEPARAMS pProfileParams = (LPPROFILEPARAMS) lpparameters;
         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::iidbdb",lpVirtNode);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;
         lstrcpy (tchszMode, pProfileParams->bRestrict? "restrict": "cascade");
         wsprintf(bufrequest,"drop profile %s %s", QuoteIfNeeded(pProfileParams->ObjectName), tchszMode);
         iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;
      case OT_GROUP:
         {
         struct GroupParams * pGroupParams =
                     (struct GroupParams * ) lpparameters;
         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::iidbdb",lpVirtNode);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;
         /************************************************************/
         /* First we have to drop all the users from the group : the */
         /* dialog box already ask the DBA about this : no need to   */
         /* reject the statement                                     */
         /************************************************************/
         wsprintf(bufrequest,"alter group %s drop all",QuoteIfNeeded(pGroupParams->ObjectName));
         iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         if (iret!=RES_SUCCESS)
             break;
         wsprintf(bufrequest,"drop group %s", QuoteIfNeeded(pGroupParams->ObjectName));
         iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;

      case OT_GROUPUSER:
         {
         LPGROUPUSERPARAMS pGroupUser = (LPGROUPUSERPARAMS) lpparameters;
         if (!pGroupUser->GroupName[0] ||
             !pGroupUser->ObjectName[0]  ) 
            return RES_ERR;

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::iidbdb",lpVirtNode);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         wsprintf(bufrequest,"alter group %s drop users ( %s )",
                  QuoteIfNeeded(pGroupUser->GroupName),QuoteIfNeeded(pGroupUser->ObjectName));

         iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;

      case OT_ROLE:
         {
         struct RoleParams * pRoleParams =
                     (struct RoleParams * ) lpparameters;
         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::iidbdb",lpVirtNode);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;
         wsprintf(bufrequest,"drop role %s", QuoteIfNeeded(pRoleParams->ObjectName));
         iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;
      case OT_DBEVENT:
         {
         struct DBEventParams * pDBEventParams =
                     (struct DBEventParams * ) lpparameters;
         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pDBEventParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;
         wsprintf(bufrequest,"drop dbevent %s",pDBEventParams->ObjectName);
         iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;
      case OT_PROCEDURE:
         {
         struct ProcedureParams * pProcedureParams =
                     (struct ProcedureParams * ) lpparameters;
         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pProcedureParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;
         wsprintf(bufrequest,"drop procedure %s.%s",QuoteIfNeeded(pProcedureParams->objectowner),
                                                    QuoteIfNeeded(pProcedureParams->objectname));
         iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;
      case OT_SEQUENCE:
         {
         struct SequenceParams * pSequenceParams =
                     (struct SequenceParams * ) lpparameters;
         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pSequenceParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;
         wsprintf(bufrequest,"drop sequence %s.%s",QuoteIfNeeded(pSequenceParams->objectowner),
                                                   QuoteIfNeeded(pSequenceParams->objectname));
         iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;
      case OT_TABLE:
         {
         struct TableParams * pTableParams =
                     (struct TableParams * ) lpparameters;
         if (!*(pTableParams->objectname))
            return RES_ERR; 
         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pTableParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;
         wsprintf(bufrequest,"drop table %s.%s",QuoteIfNeeded(pTableParams->szSchema),
                                                QuoteIfNeeded(pTableParams->objectname));
         iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;
      case OT_INDEX:
         {
         struct IndexParams * pIndexParams =
                     (struct IndexParams * ) lpparameters;
         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pIndexParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;
         wsprintf(bufrequest,"drop index %s.%s",QuoteIfNeeded(pIndexParams->objectowner),
                                                QuoteIfNeeded(pIndexParams->objectname));
         iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;     
      case OT_VIEW:
         {
         struct ViewParams * pViewParams =
                     (struct ViewParams * ) lpparameters;
         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pViewParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;
         wsprintf(bufrequest,"drop view %s.%s", QuoteIfNeeded(pViewParams->szSchema),
                                                QuoteIfNeeded(pViewParams->objectname));
         iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;
      case OT_SYNONYM:
         {
         struct SynonymParams * pSynonymParams =
                     (struct SynonymParams * ) lpparameters;

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pSynonymParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         wsprintf (bufrequest,"drop synonym %s ",QuoteIfNeeded(pSynonymParams->ObjectName));

         iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
        
         }
         break;
      case OTLL_SECURITYALARM:
         {
         struct SecurityAlarmParams *pSecurityAlarmParams =
                     (struct SecurityAlarmParams *) lpparameters;

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            if (pSecurityAlarmParams->iObjectType==OT_DATABASE ||
				pSecurityAlarmParams->iObjectType==OT_VIRTNODE)
               wsprintf(connectname,"%s::iidbdb",lpVirtNode);
            else 
               wsprintf(connectname,"%s::%s",lpVirtNode,pSecurityAlarmParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;
        
         if (pSecurityAlarmParams->iObjectType==OT_VIRTNODE) {
            wsprintf (bufrequest,"drop security_alarm on current installation %d",
                  pSecurityAlarmParams->SecAlarmId);
         }
         else if (pSecurityAlarmParams->iObjectType==OT_DATABASE) {
            wsprintf (bufrequest,"drop security_alarm on database %s %d",
                  pSecurityAlarmParams->lpfirstTableIn->dbname,
                  pSecurityAlarmParams->SecAlarmId);
         }
         else {
            wsprintf (bufrequest,"drop security_alarm on %s %d",
                      pSecurityAlarmParams->lpfirstTableIn->dbname,
                      pSecurityAlarmParams->SecAlarmId);
         }
         iret=ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;
      case OTLL_GRANT:
      {
            struct RevokeParams *pRevokeParams =
                     (struct RevokeParams *) lpparameters;
            int granteetypes[]={OT_USER, OT_GROUP, OT_ROLE, 0}, i=0;
            if (!lpVirtNode)
               iret=ActivateSession(Sesshdl);
            else {
               if (pRevokeParams->ObjectType==OT_DATABASE ||
                   pRevokeParams->ObjectType==OT_ROLE) 
                  wsprintf(connectname,"%s::iidbdb",lpVirtNode);
               else      
                  wsprintf(connectname,"%s::%s",lpVirtNode,pRevokeParams->DBName);

               iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
            }
            if (iret !=RES_SUCCESS)
               return iret;
            if (pRevokeParams->GranteeType==OT_UNKNOWN) {
               i=0;
               while (granteetypes[i]) { /* try with each possible types */
                  pRevokeParams->GranteeType=granteetypes[i];
                  iret=BuildSQLRevoke(&pSQLstm,
                           (struct RevokeParams *) lpparameters);
                  if (iret==RES_SUCCESS) {
                     iret=ExecSQLImm(pSQLstm,FALSE , NULL, NULL, NULL);
                     if (iret=RES_SUCCESS) {
                        ESL_FreeMem((void *) pSQLstm);
                        goto enddrop;
                     }
                  }
                  else {
                     iret=RES_ERR;
                     goto enddrop;
                  }
                  ESL_FreeMem((void *) pSQLstm);
                  i++;
               }
               iret=RES_ERR;
               goto enddrop;
            }
            else {
               iret=BuildSQLRevoke(&pSQLstm,
                        (struct RevokeParams *) lpparameters);
               if (iret!=RES_SUCCESS)
                  break;
               iret=ExecSQLImm(pSQLstm,FALSE , NULL, NULL, NULL);
               ESL_FreeMem((void *) pSQLstm);
            }
      }                             
      break;
      case OT_REPLIC_REGTABLE:
         {
         struct ReplRegTableParams * pReplRegTableParams =
                     (struct ReplRegTableParams * ) lpparameters;

         myerror(ERR_REPLICCDDS); // this code should no more be used

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pReplRegTableParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         sprintf (bufrequest,
               "delete from dd_registered_tables where tablename = '%s'",
                  pReplRegTableParams->TableName);

         iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;
      case OT_REPLIC_CONNECTION:
         {
         struct ReplConnectParams * pReplConnectParams =
                     (struct ReplConnectParams * ) lpparameters;

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pReplConnectParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         sprintf (bufrequest,
               "delete from dd_connections where database_no = %d", 
               pReplConnectParams->nNumber);

         iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         if (iret!=RES_SUCCESS)
            break;
         iret=UpdCfgChanged(pReplConnectParams->nReplicVersion);
         }
         break;
      case OT_REPLIC_CONNECTION_V11:
         {
         struct ReplConnectParams * pReplConnectParams =
                     (struct ReplConnectParams * ) lpparameters;

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pReplConnectParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         wsprintf (bufrequest,
               "delete from dd_databases where database_no = %d and local_db != 1", 
                pReplConnectParams->nNumber);
         iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         if (iret !=RES_SUCCESS)
            break;

         wsprintf (bufrequest,
               "delete from dd_db_cdds where database_no = %d", 
                pReplConnectParams->nNumber);
         iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         if (iret==RES_ENDOFDATA)
            iret=RES_SUCCESS;
         if (iret !=RES_SUCCESS)
            break;

         wsprintf (bufrequest,
               "delete from dd_paths where (localdb = %d or sourcedb = %d or targetdb = %d )", 
                pReplConnectParams->nNumber,pReplConnectParams->nNumber,pReplConnectParams->nNumber);
         iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         if (iret==RES_ENDOFDATA)
            iret=RES_SUCCESS;
         if (iret !=RES_SUCCESS)
            break;
         iret=UpdCfgChanged(pReplConnectParams->nReplicVersion);
         }
         break;

       case OT_REPLIC_CDDS:
         {
         LPREPCDDS lpcdds= (LPREPCDDS)lpparameters;


         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,lpcdds->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }

         if (iret !=RES_SUCCESS)
            return iret;

         iret=BuildSQLDropCDDS    (NULL, lpcdds);
         if (iret!=RES_SUCCESS)
            break;
         iret=UpdCfgChanged(lpcdds->iReplicType);

         }
         break;

       case OT_REPLIC_CDDS_V11:
         {
         LPREPCDDS lpcdds= (LPREPCDDS)lpparameters;


         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,lpcdds->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }

         if (iret !=RES_SUCCESS)
            return iret;

         iret=BuildSQLDropCDDSV11(NULL,lpcdds);
         if (iret!=RES_SUCCESS)
            break;
         iret=UpdCfgChanged(lpcdds->iReplicType);

         }
         break;

      case OTLL_REPLIC_CDDS:
         {
         struct ReplDistribParams * pReplDistribParams =
                     (struct ReplDistribParams * ) lpparameters;

         myerror(ERR_REPLICCDDS); // this code should no more be used

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pReplDistribParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         sprintf (bufrequest,
               "delete from dd_distribution "
                  "where localdb = %d"
                  " and "
                  "source = %d "
                  " and "
                  "target= %d "
                  " and "
                  "dd_routing = %d",
               pReplDistribParams->Local_DBno,
               pReplDistribParams->Source_DBno,
               pReplDistribParams->Target_DBno,
               pReplDistribParams->ddRouting);

         iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;
      case OT_REPLIC_MAILUSER:
         {
         struct ReplMailParams * pReplMailParams =
                     (struct ReplMailParams * ) lpparameters;

         if (!lpVirtNode)
            iret=ActivateSession(Sesshdl);
         else {
            wsprintf(connectname,"%s::%s",lpVirtNode,pReplMailParams->DBName);
            iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
         }
         if (iret !=RES_SUCCESS)
            return iret;

         sprintf (bufrequest,
               "delete from dd_mail_alert where mail_username = '%s' ",
               pReplMailParams->szMailText);

         iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
         }
         break;

      default:
         return RES_ENDOFDATA;
         break;
   }
enddrop:

   if (!lpVirtNode)
      return iret;

   if (iret==RES_SUCCESS)
     iret=ReleaseSession(ilocsession, RELEASE_COMMIT);
   else 
     ReleaseSession(ilocsession, RELEASE_ROLLBACK);
   return iret;
}


int DBAAlterObject(void *lpObjParam1, void* lpObjParam2,
               int iobjectype, int Sesshdl)
/***********************************************************************
*
***********************************************************************/
{
   int iret; 
   iret=ActivateSession(Sesshdl);
   if (iret!=RES_SUCCESS)
      return iret;
   switch (iobjectype) {
      case OT_REPLIC_CONNECTION: {
         struct ReplConnectParams * pReplConnectParams =
                     (struct ReplConnectParams * ) lpObjParam1;
         iret=GenAlterReplicConnection ( (LPREPLCONNECTPARAMS) lpObjParam1,
                                         (LPREPLCONNECTPARAMS) lpObjParam2);
         if (iret!=RES_SUCCESS)
            break;
         iret=UpdCfgChanged(pReplConnectParams->nReplicVersion);
      }
      break;
      case OT_REPLIC_CONNECTION_V11: {
         struct ReplConnectParams * pReplConnectParams =
                     (struct ReplConnectParams * ) lpObjParam1;
         iret=GenAlterReplicConnectionV11 ( (LPREPLCONNECTPARAMS) lpObjParam1,
                                          (LPREPLCONNECTPARAMS) lpObjParam2);
         if (iret!=RES_SUCCESS)
            break;
         iret=UpdCfgChanged(pReplConnectParams->nReplicVersion);
      }
      break;
      case OT_LOCATION: {
         iret=GenAlterLocation((LPLOCATIONPARAMS) lpObjParam2);
      }
      break;
      case OT_GROUP: {
         iret=GenAlterGroup((LPGROUPPARAMS) lpObjParam1,
                            (LPGROUPPARAMS) lpObjParam2);
      }
      break;
      case OT_USER: {
         iret=GenAlterUser((LPUSERPARAMS) lpObjParam2);
      }
      break;
      case OT_PROFILE: {
         iret=GenAlterProfile((LPPROFILEPARAMS) lpObjParam2);
      }
      break;
      case OT_ROLE: {
         iret=GenAlterRole((LPROLEPARAMS) lpObjParam2);
      }
      break;
      case OT_PROCEDURE : { 
         ((LPPROCEDUREPARAMS)lpObjParam2)->bOnlyText=TRUE;
         iret=DBADropObject(NULL, iobjectype, lpObjParam2, Sesshdl);
         if (iret!=RES_SUCCESS)   
            break;
         iret=DBAAddObjectLL(NULL, iobjectype, lpObjParam2, Sesshdl);
      }
      break;
      case OT_RULE: {     
         ((LPRULEPARAMS)lpObjParam2)->bOnlyText=TRUE;
         iret=DBADropObject(NULL, iobjectype, lpObjParam2, Sesshdl);
         if (iret!=RES_SUCCESS)   
            break;
         iret=DBAAddObjectLL(NULL, iobjectype, lpObjParam2, Sesshdl);
      }
      break;
      // JFS Begin
      case OT_REPLIC_CDDS:
      case OT_REPLIC_CDDS_V11:
         iret=DBAAddObjectLL(NULL, iobjectype, lpObjParam2, Sesshdl);
         break;
      // JFS End

      case OT_DATABASE:
        break;

      // OpenIngres Desktop
      case OT_TABLE:
        iret=VDBA20xGenAlterTable ((LPTABLEPARAMS)lpObjParam1, (LPTABLEPARAMS)lpObjParam2);
        break;
      case OT_SEQUENCE:
          iret=GenAlterSequence(lpObjParam2);
       break;
      default :
         iret=RES_ERR;
   }
   if (iret==RES_SUCCESS)
     iret=ReleaseSession(Sesshdl, RELEASE_COMMIT);
   else 
     ReleaseSession(Sesshdl, RELEASE_ROLLBACK);
   return iret;
}
int DBAModifyObject(LPSTORAGEPARAMS lpStorageParams, int Sesshdl)
/***********************************************************************
*
***********************************************************************/
{
   int iret; 
   iret=ActivateSession(Sesshdl);
   if (iret!=RES_SUCCESS)
      return iret;
   switch (lpStorageParams->nModifyAction) {
      case MODIFYACTION_RELOC  : {
         iret=ModifyReloc(lpStorageParams);
      }
      break;
      case MODIFYACTION_REORG  : {
         iret=ModifyReorg(lpStorageParams);
      }
      break;
      case MODIFYACTION_TRUNC  : {
         iret=ModifyTrunc(lpStorageParams);
      }
      break;
      case MODIFYACTION_MERGE  : {
         iret=ModifyMerge(lpStorageParams);
      }
      break;
      case MODIFYACTION_ADDPAGE: {
         iret=ModifyAddPages(lpStorageParams);
      }
      break;
      case MODIFYACTION_CHGSTORAGE: 
         iret=ModifyChgStorage(lpStorageParams);
         break;
      case MODIFYACTION_READONLY:
         iret=ModifyReadOnly(lpStorageParams);
         break;
      case MODIFYACTION_NOREADONLY:
         iret=ModifyNoReadOnly(lpStorageParams);
         break;
      case MODIFYACTION_PHYSINCONSIST:
         iret=ModifyPhysInconsistent(lpStorageParams);
         break;
       case MODIFYACTION_PHYSCONSIST:
         iret=ModifyPhysconsistent(lpStorageParams);
         break;
       case MODIFYACTION_LOGINCONSIST:
         iret=ModifyLogInconsistent(lpStorageParams);
         break;
       case MODIFYACTION_LOGCONSIST:
         iret=ModifyLogConsistent(lpStorageParams);
         break;
       case MODIFYACTION_RECOVERYDISALLOWED:
         iret=ModifyRecoveryDisallowed(lpStorageParams);
         break;
       case MODIFYACTION_RECOVERYALLOWED:
         iret=ModifyRecoveryAllowed(lpStorageParams);
         break;
      default :
         iret=RES_ERR;
   }
   if (iret==RES_SUCCESS || iret==RES_ENDOFDATA)
     iret=ReleaseSession(Sesshdl, RELEASE_COMMIT);
   else 
     ReleaseSession(Sesshdl, RELEASE_ROLLBACK);
   return iret;
}


int DBAUpdMobile(LPUCHAR lpVirtNode, LPREPMOBILE lprepmobile)

{
   char connectname[2*MAXOBJECTNAME+2+1];
   int  iret;
   char bufrequest[400];
   int  ilocsession;

   if (!lprepmobile)
      return myerror(ERR_ADDOBJECT);

   wsprintf(connectname,"%s::%s",lpVirtNode,lprepmobile->DBName);
   iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
   
   if (iret !=RES_SUCCESS)
      return iret;

   wsprintf(bufrequest,
           "update dd_mobile_opt "
           "set collision_mode = %d,error_mode=%d",
           lprepmobile->collision_mode,
           lprepmobile->error_mode);

   iret = ExecSQLImm(bufrequest,FALSE, NULL, NULL, NULL);
  
   if (iret==RES_SUCCESS)
      iret=ReleaseSession(ilocsession, RELEASE_COMMIT);
   else 
      ReleaseSession(ilocsession, RELEASE_ROLLBACK);
   return iret;
}



//
// UK.S
// VDBA20x
//
void StringList_CleanUp (LPSTRINGLIST* lpList, ...)
{
    va_list Marker;
    LPSTRINGLIST*  lpstrArg;

    va_start (Marker, lpList);
    while (lpstrArg = va_arg (Marker, LPSTRINGLIST*))
    {
        *lpstrArg = StringList_Done (*lpstrArg);
    }
    *lpList = StringList_Done (*lpList);
}

//
// We should alter the constraints (drop) before altering the columns because
// we use the option RESTRICT in drop column. That is if the column has constarints
// then drop column will failed.
// We must alter the constraints (add) after altering the columns.
int VDBA20xGenAlterTable (LPTABLEPARAMS lpOLDTS, LPTABLEPARAMS lpNEWTS)
{
    int m,  ires1;
    LPTSTR       lpDropCol = NULL, lpAddCol = NULL;
    LPSTRINGLIST 
        list      = NULL, 
        lpDropCols= NULL, 
        lpAddCols = NULL,
        lpAlterPk = NULL, 
        lpAlterFk = NULL, 
        lpAlterUn = NULL, 
        lpAlterCh = NULL,
        lpAlterCols = NULL;
    //
    // NOTE: We use the extraInfo1 field of STRINGLIST as followings:
    //       0: The string is generated for dropping.
    //       1: The string is generated for adding.
    //

    // Alter the Primary Key.
    // Drop old primary key.
    lpAlterPk = VDBA20xAlterPrimaryKeySyntax (lpOLDTS, lpNEWTS, &m);
    if (!m)
    {
        StringList_CleanUp (&lpDropCols, &lpAddCols, &lpAlterPk, &lpAlterFk, &lpAlterUn, &lpAlterCh, NULL);
        return RES_ERR;
    }
    list = lpAlterPk;
    while (list)
    {
        if (!list->extraInfo1)
        {
            ires1 = ExecSQLImm (list->lpszString, FALSE, NULL, NULL, NULL);
            if (ires1 != RES_SUCCESS)
            {
                StringList_CleanUp (&lpDropCols, &lpAddCols, &lpAlterPk, &lpAlterFk, &lpAlterUn, &lpAlterCh, NULL);
                return ires1;
            }
        }
        list = list->next;
    }
    //
    // Alter the Unique Keys.
    // Drop old Unique keys.
    lpAlterUn = VDBA20xAlterUniqueKeySyntax (lpOLDTS, lpNEWTS, &m);
    if (!m)
    {
        StringList_CleanUp (&lpDropCols, &lpAddCols, &lpAlterPk, &lpAlterFk, &lpAlterUn, &lpAlterCh, NULL);
        return RES_ERR;
    }
    list = lpAlterUn;
    while (list)
    {
        if (!list->extraInfo1)
        {
            ires1 = ExecSQLImm (list->lpszString, FALSE, NULL, NULL, NULL);
            if (ires1 != RES_SUCCESS)
            {
                StringList_CleanUp (&lpDropCols, &lpAddCols, &lpAlterPk, &lpAlterFk, &lpAlterUn, &lpAlterCh, NULL);
                return ires1;
            }
        }
        list = list->next;
    }
    //
    // Alter the Check Conditions.
    // Drop old Check Conditions.
    lpAlterCh = VDBA20xAlterCheckSyntax (lpOLDTS, lpNEWTS, &m);
    if (!m)
    {
        StringList_CleanUp (&lpDropCols, &lpAddCols, &lpAlterPk, &lpAlterFk, &lpAlterUn, &lpAlterCh, NULL);
        return RES_ERR;
    }
    list = lpAlterCh;
    while (list)
    {
        if (!list->extraInfo1)
        {
            ires1 = ExecSQLImm (list->lpszString, FALSE, NULL, NULL, NULL);
            if (ires1 != RES_SUCCESS)
            {
                StringList_CleanUp (&lpDropCols, &lpAddCols, &lpAlterPk, &lpAlterFk, &lpAlterUn, &lpAlterCh, NULL);
                return ires1;
            }
        }
        list = list->next;
    }
    //
    // Alter the Foreign Keys.
    // Drop old foreign keys.
    lpAlterFk = VDBA20xAlterForeignKeySyntax (lpOLDTS, lpNEWTS, &m);
    if (!m)
    {
        StringList_CleanUp (&lpDropCols, &lpAddCols, &lpAlterPk, &lpAlterFk, &lpAlterUn, &lpAlterCh, NULL);
        return RES_ERR;
    }
    list = lpAlterFk;
    while (list)
    {
        if (!list->extraInfo1)
        {
            ires1 = ExecSQLImm (list->lpszString, FALSE, NULL, NULL, NULL);
            if (ires1 != RES_SUCCESS)
            {
                StringList_CleanUp (&lpDropCols, &lpAddCols, &lpAlterPk, &lpAlterFk, &lpAlterUn, &lpAlterCh, NULL);
                return ires1;
            }
        }
        list = list->next;
    }
    //
    // Drop the columns:
    // Drop one column at a time. All columns to be dropped, bust be dropped in a 
    // single transaction (a single session).
    //
    lpDropCols = VDBA20xDropTableColumnSyntax (lpOLDTS, lpNEWTS, NULL);
    list = lpDropCols;
    while (list)
    {
        ires1 = ExecSQLImm (list->lpszString, FALSE, NULL, NULL, NULL);
        if (ires1 != RES_SUCCESS)
        {
            StringList_CleanUp (&lpDropCols, &lpAddCols, &lpAlterPk, &lpAlterFk, &lpAlterUn, &lpAlterCh, NULL);
            return ires1;
        }
        list = list->next;
    }
    //
    // Add the columns:
    // Add one column at a time. All columns to be added, bust be added in a 
    // single transaction (a single session).
    //
    lpAddCols = VDBA20xAddTableColumnSyntax (lpOLDTS, lpNEWTS, NULL);
    list = lpAddCols;
    while (list)
    {
        ires1 = ExecSQLImm (list->lpszString, FALSE, NULL, NULL, NULL);
        if (ires1 != RES_SUCCESS)
        {
            StringList_CleanUp (&lpDropCols, &lpAddCols, &lpAlterPk, &lpAlterFk, &lpAlterUn, &lpAlterCh, NULL);
            return ires1;
        }
        list = list->next;
    }
    if (GetOIVers() >= OIVERS_30)
    {
        //
        // Alter the columns:
        // Alter one column at a time. All columns to be Altered, must be added in a 
        // single transaction (a single session).
        //
        lpAlterCols = VDBA20xAlterTableColumnSyntax (lpOLDTS, lpNEWTS, NULL);
        list = lpAlterCols;
        while (list)
        {
            ires1 = ExecSQLImm (list->lpszString, FALSE, NULL, NULL, NULL);
            if (ires1 != RES_SUCCESS)
            {
                StringList_CleanUp (&lpDropCols, &lpAddCols, &lpAlterPk, &lpAlterFk, &lpAlterUn, &lpAlterCh, &lpAlterCols, NULL);
                return ires1;
            }
            list = list->next;
        }
    }
    // Alter the Primary Key.
    // Add new primary key.
    list = lpAlterPk;
    while (list)
    {
        if (list->extraInfo1)
        {
            ires1 = ExecSQLImm (list->lpszString, FALSE, NULL, NULL, NULL);
            if (ires1 != RES_SUCCESS)
            {
                StringList_CleanUp (&lpDropCols, &lpAddCols, &lpAlterPk, &lpAlterFk, &lpAlterUn, &lpAlterCh, &lpAlterCols, NULL);
                return ires1;
            }
        }
        list = list->next;
    }
    //
    // Alter the Unique Keys.
    // Add New Unique keys.
    list = lpAlterUn;
    while (list)
    {
        if (list->extraInfo1)
        {
            ires1 = ExecSQLImm (list->lpszString, FALSE, NULL, NULL, NULL);
            if (ires1 != RES_SUCCESS)
            {
                StringList_CleanUp (&lpDropCols, &lpAddCols, &lpAlterPk, &lpAlterFk, &lpAlterUn, &lpAlterCh, &lpAlterCols, NULL);
                return ires1;
            }
        }
        list = list->next;
    }
    //
    // Alter the Check Conditions.
    // Add New Check Conditions.
    list = lpAlterCh;
    while (list)
    {
        if (list->extraInfo1)
        {
            ires1 = ExecSQLImm (list->lpszString, FALSE, NULL, NULL, NULL);
            if (ires1 != RES_SUCCESS)
            {
                StringList_CleanUp (&lpDropCols, &lpAddCols, &lpAlterPk, &lpAlterFk, &lpAlterUn, &lpAlterCh, &lpAlterCols, NULL);
                return ires1;
            }
        }
        list = list->next;
    }
    //
    // Alter the Foreign Keys.
    // Add new foreign keys.
    list = lpAlterFk;
    while (list)
    {
        if (list->extraInfo1)
        {
            ires1 = ExecSQLImm (list->lpszString, FALSE, NULL, NULL, NULL);
            if (ires1 != RES_SUCCESS)
            {
                StringList_CleanUp (&lpDropCols, &lpAddCols, &lpAlterPk, &lpAlterFk, &lpAlterUn, &lpAlterCh, &lpAlterCols, NULL);
                return ires1;
            }
        }
        list = list->next;
    }
    StringList_CleanUp (&lpDropCols, &lpAddCols, &lpAlterPk, &lpAlterFk, &lpAlterUn, &lpAlterCh, &lpAlterCols, NULL);
    return RES_SUCCESS;
}


#define RETURNBUFSIZE 8000


/*
** int VDBA20xGenCommentObject( LPTSTR lpVirtNode, LPTSTR lpDbName,
**                              LPTSTR lpGeneralComment, LPSTRINGLIST lpColComment)
**
**  Execute the SQL request for "comment on ..."
**
**  Input parameter:
**
**     LPTSTR lpVirtNode         : Node name
**     LPTSTR lpDbName           : Database name
**     LPTSTR lpGeneralComment   : general comment on table or view.
**     LPSTRINGLIST lpColComment : Contains the list of all SQL request
**                                 (one request by column) necessary to add or
**                                 to modify a comment.
**
**  return parameter:
**     RES_ERR or RES_SUCCESS
*/
int VDBA20xGenCommentObject( LPTSTR lpVirtNode,
                             LPTSTR lpDbName,
                             LPTSTR lpGeneralComment, LPSTRINGLIST lpColComment)
{
    TCHAR connectname[MAXOBJECTNAME];
    int   iret, ilocsession;
    LPSTRINGLIST list = NULL;

    wsprintf(connectname,"%s::%s",lpVirtNode,lpDbName);
    iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);

    if (iret !=RES_SUCCESS)
        return iret;

    list = lpColComment;
    while (list)
    {
        iret = ExecSQLImm (list->lpszString, FALSE, NULL, NULL, NULL);
        if (iret != RES_SUCCESS)
            break;
        list = list->next;
    }

    if (iret == RES_SUCCESS && lstrlen(lpGeneralComment)) {
        iret=ExecSQLImm(lpGeneralComment, FALSE, NULL, NULL, NULL);
    }

    if (iret==RES_SUCCESS)
        iret=ReleaseSession (ilocsession, RELEASE_COMMIT);
    else 
        ReleaseSession(ilocsession, RELEASE_ROLLBACK);
    return iret;
}
