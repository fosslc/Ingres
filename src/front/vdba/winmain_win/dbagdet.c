/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**   Project : CA/OpenIngres Visual DBA
**
**   Source : dbagdet.c
**   GetDetailInfo function
**
**   Author : Lionel Storck
**
**  History:
**   16-Nov-2000 (schph01)
**    SIR 102821 add function to retrieve comment on tables, views or
**               columns
**   21-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
**   18-Mar-2003 (schph01)
**     sir 107523 management of sequences
**   15-May-2003 (schph01)
**    bug 110247 get the DBMSINFO(unicode_level) information before to
**    retrieve the database informations
**   10-Jun-2004 (schph01)
**    bug 112447 Call the GetCatalogPageSize() function before to
**    retrieve the database informations
**   06-Sep-2005) (drivi01)
**    Bug #115173 Updated createdb dialogs and alterdb dialogs to contain
**    two available unicode normalization options, added group box with
**    two checkboxes corresponding to each normalization, including all
**    expected functionlity.
**    Added aditional unicode_normalization checks.
********************************************************************/

#include "dba.h" 
#include "dbaparse.h"
#include "dbaset.h"
#include "esltools.h"
#include "dbaginfo.h"
#include "dbaset.h"
#include "error.h"
#include "cddsload.h"
#include "dbadlg1.h"
#include "extccall.h"
#include "tchar.h"
extern LPTSTR INGRESII_llDBMSInfo(LPTSTR lpszConstName, LPTSTR lpVer);

int GetDetailInfo(LPUCHAR lpVirtNode,int iobjecttype, void *lpparameters,
                  BOOL bRetainSessForLock, int *ilocsession)
{
	return Task_GetDetailInfoInterruptible (lpVirtNode, iobjecttype, lpparameters,bRetainSessForLock, ilocsession);

	return GetDetailInfoLL(lpVirtNode,iobjecttype,lpparameters,
							bRetainSessForLock, ilocsession);

}

int GetDetailInfoLL(LPUCHAR lpVirtNode,int iobjecttype, void *lpparameters,
                  BOOL bRetainSessForLock, int *ilocsession)
{
   int iret, SessType;
   char connectname[MAXOBJECTNAME];


   if (!lpparameters) {
      myerror(ERR_INVPARAMS);
      return RES_ERR;
   }

   if (bRetainSessForLock)
      SessType=SESSION_TYPE_CACHEREADLOCK;
   else
      SessType=SESSION_TYPE_CACHENOREADLOCK;

   switch (iobjecttype) {
      case OT_SEQUENCE:
       {
       LPSEQUENCEPARAMS pseqprm=(LPSEQUENCEPARAMS) lpparameters;

       if (!*(pseqprm->objectname)) {
          myerror(ERR_INVPARAMS);
          return RES_ERR;
       }

       if (!*(pseqprm->DBName)) {
          myerror(ERR_INVPARAMS);
          return RES_ERR;
       }

       wsprintf(connectname,"%s::%s",lpVirtNode, pseqprm->DBName);

       iret = Getsession(connectname, SessType, ilocsession);
       if (iret !=RES_SUCCESS)
          return RES_ERR;

       iret=GetInfSequence(pseqprm);

       }
         break;

      case OT_REPLIC_CONNECTION_V11:
       {
       LPREPLCONNECTPARAMS   lpaddrepl= (LPREPLCONNECTPARAMS)lpparameters; ;
       wsprintf(connectname,"%s::%s",lpVirtNode,lpaddrepl->DBName);
       iret = Getsession(connectname, SessType, ilocsession);

       if (iret !=RES_SUCCESS)
         return RES_ERR;
       iret = ReplicConnectV11(lpaddrepl);
       
       }
       break;
      case OT_REPLIC_CONNECTION:
       {
       LPREPLCONNECTPARAMS   lpaddrepl= (LPREPLCONNECTPARAMS)lpparameters; ;
       wsprintf(connectname,"%s::%s",lpVirtNode,lpaddrepl->DBName);
       iret = Getsession(connectname, SessType, ilocsession);

       if (iret !=RES_SUCCESS)
         return RES_ERR;
       iret = ReplicConnect(lpaddrepl);
       }
       break;
      case OT_DBEVENT:
         iret =RES_SUCCESS; // no displayable property
         // TO BE FINISHED session not opened.

         break;
      case OT_ROLE:
         {
            LPROLEPARAMS proleprm=(LPROLEPARAMS) lpparameters;

            if (!*(proleprm->ObjectName)) {
               myerror(ERR_INVPARAMS);
               return RES_ERR;
            }

            FreeAttachedPointers(lpparameters, iobjecttype);

            wsprintf(connectname,"%s::iidbdb",lpVirtNode);

            iret = Getsession(connectname, SessType, ilocsession);
            if (iret !=RES_SUCCESS)
               return RES_ERR;

            iret=GetInfRole(proleprm);

         }
         break;

//JFS Begin
      case OT_REPLIC_CDDS :
      case OT_REPLIC_CDDS_V11 :
         {
            LPREPCDDS lpcdds = (LPREPCDDS)lpparameters;
            wsprintf(connectname,"%s::%s",lpVirtNode,lpcdds->DBName);

            iret = Getsession(connectname, SESSION_TYPE_INDEPENDENT, ilocsession);
            if (iret !=RES_SUCCESS)
               return RES_ERR;

            iret = CDDSLoadAll(lpcdds,lpVirtNode);
         }
         break;
//JFS End

      case OT_GROUP :
         {
            LPGROUPPARAMS pgrpprm=(LPGROUPPARAMS) lpparameters;

            if (!*(pgrpprm->ObjectName)) {
               myerror(ERR_INVPARAMS);
               return RES_ERR;
            }

            FreeAttachedPointers(lpparameters, iobjecttype);

            wsprintf(connectname,"%s::iidbdb",lpVirtNode);

            iret = Getsession(connectname, SessType, ilocsession);
            if (iret !=RES_SUCCESS)
               return RES_ERR;

            iret=GetInfGrp(pgrpprm);

         }
         break;

      case OT_INTEGRITY:
         {
            LPINTEGRITYPARAMS pintegrityprm=(LPINTEGRITYPARAMS) lpparameters;


            if (!*(pintegrityprm->DBName)) {
               myerror(ERR_INVPARAMS);
               return RES_ERR;
            }

            wsprintf(connectname,"%s::%s",lpVirtNode, pintegrityprm->DBName);

            iret = Getsession(connectname, SessType, ilocsession);
            if (iret !=RES_SUCCESS)
               return RES_ERR;

            iret=GetInfIntegrity(pintegrityprm);

         }
         break;

      case OT_LOCATION:
         {
            LPLOCATIONPARAMS plocprm=(LPLOCATIONPARAMS) lpparameters;

            if (!*(plocprm->objectname)) {
               myerror(ERR_INVPARAMS);
               return RES_ERR;
            }

            wsprintf(connectname,"%s::iidbdb",lpVirtNode);

            iret = Getsession(connectname, SessType, ilocsession);
            if (iret !=RES_SUCCESS)
               return RES_ERR;

            iret=GetInfLoc(plocprm);

         }
         break;
         
      case OT_PROCEDURE:
         {
            LPPROCEDUREPARAMS pprocprm=(LPPROCEDUREPARAMS) lpparameters;

            if (!*(pprocprm->objectname)) {
               myerror(ERR_INVPARAMS);
               return RES_ERR;
            }

            if (!*(pprocprm->DBName)) {
               myerror(ERR_INVPARAMS);
               return RES_ERR;
            }

            wsprintf(connectname,"%s::%s",lpVirtNode, pprocprm->DBName);

            iret = Getsession(connectname, SessType, ilocsession);
            if (iret !=RES_SUCCESS)
               return RES_ERR;

            iret=GetInfProcedure(pprocprm);

         }
         break;

      case OT_RULE:
         {
            LPRULEPARAMS pruleprm=(LPRULEPARAMS) lpparameters;

            if (!*(pruleprm->RuleName)) {
               myerror(ERR_INVPARAMS);
               return RES_ERR;
            }

            if (!*(pruleprm->DBName)) {
               myerror(ERR_INVPARAMS);
               return RES_ERR;
            }

            wsprintf(connectname,"%s::%s",lpVirtNode, pruleprm->DBName);

            iret = Getsession(connectname, SessType, ilocsession);
            if (iret !=RES_SUCCESS)
               return RES_ERR;

            iret=GetInfRule(pruleprm);

         }
         break;

      case OT_USER :
         {
            LPUSERPARAMS pusrprm=(LPUSERPARAMS) lpparameters;

            if (!*(pusrprm->ObjectName)) {
               myerror(ERR_INVPARAMS);
               return RES_ERR;
            }

            FreeAttachedPointers(lpparameters, iobjecttype);

            wsprintf(connectname,"%s::iidbdb",lpVirtNode);

            iret = Getsession(connectname, SessType, ilocsession);
            if (iret !=RES_SUCCESS)
               return RES_ERR;

            iret=GetInfUsr(pusrprm);

         }
         break;
         
      case OT_PROFILE :
         {
            LPPROFILEPARAMS pprofprm=(LPPROFILEPARAMS) lpparameters;

            if (!*(pprofprm->ObjectName)) {
               myerror(ERR_INVPARAMS);
               return RES_ERR;
            }

            FreeAttachedPointers(lpparameters, iobjecttype);

            wsprintf(connectname,"%s::iidbdb",lpVirtNode);

            iret = Getsession(connectname, SessType, ilocsession);
            if (iret !=RES_SUCCESS)
               return RES_ERR;

            iret=GetInfProf(pprofprm);

         }
         break;
         
      case OT_VIEW:
         {
            LPVIEWPARAMS pviewprm=(LPVIEWPARAMS) lpparameters;

            if (!*(pviewprm->objectname)) {
               myerror(ERR_INVPARAMS);
               return RES_ERR;
            }

            if (!*(pviewprm->DBName)) {
               myerror(ERR_INVPARAMS);
               return RES_ERR;
            }

            wsprintf(connectname,"%s::%s",lpVirtNode, pviewprm->DBName);

            iret = Getsession(connectname, SessType, ilocsession);
            if (iret !=RES_SUCCESS)
               return RES_ERR;

            iret=GetInfView(pviewprm);

         }
         break;

      case OT_INDEX:
         {
            LPINDEXPARAMS pindexprm=(LPINDEXPARAMS) lpparameters;

            if (!*(pindexprm->objectname)) {
               myerror(ERR_INVPARAMS);
               return RES_ERR;
            }

            if (!*(pindexprm->DBName)) {
               myerror(ERR_INVPARAMS);
               return RES_ERR;
            }

            FreeAttachedPointers(lpparameters, iobjecttype);

            wsprintf(connectname,"%s::%s",lpVirtNode, pindexprm->DBName);

            iret = Getsession(connectname, SessType, ilocsession);
            if (iret !=RES_SUCCESS)
               return RES_ERR;

            x_strcpy(pindexprm->szNodeName, lpVirtNode);
            iret=GetInfIndex(pindexprm);
         }
         break;

      case OT_TABLE:
         {
            LPTABLEPARAMS ptableprm=(LPTABLEPARAMS) lpparameters;

            if (!*(ptableprm->objectname)) {
               myerror(ERR_INVPARAMS);
               return RES_ERR;
            }

            if (!*(ptableprm->DBName)) {
               myerror(ERR_INVPARAMS);
               return RES_ERR;
            }

            FreeAttachedPointers(lpparameters, iobjecttype);

            wsprintf(connectname,"%s::%s",lpVirtNode, ptableprm->DBName);

            iret = Getsession(connectname, SessType, ilocsession);
            if (iret !=RES_SUCCESS)
               return RES_ERR;

            x_strcpy(ptableprm->szNodeName, lpVirtNode);
            iret=GetInfTable(ptableprm);

         }
         break;

      case OT_DATABASE:
         {
            LPDATABASEPARAMS pbaseprm=(LPDATABASEPARAMS) lpparameters;

            wsprintf(connectname,"%s::%s",lpVirtNode,pbaseprm->objectname);
            iret = Getsession(connectname, SessType, ilocsession);
            if (iret !=RES_SUCCESS)
               return RES_ERR;
            iret=GetInfBaseExtend(pbaseprm);
            // retrieve UNICODE information
            pbaseprm->bUnicodeDBNFD = 0;
            pbaseprm->bUnicodeDBNFC = 0;
            if (iret==RES_SUCCESS && GetOIVers() >= OIVERS_26)
            {
                TCHAR tcVersion[40];
	TCHAR tcNormalization[40];
                INGRESII_llDBMSInfo(_T("unicode_level"),tcVersion);
	INGRESII_llDBMSInfo(_T("unicode_normalization"), tcNormalization);
                if (tcVersion[0] != '0' && (x_strcmp(tcNormalization, "NFD")==0)) 
                    pbaseprm->bUnicodeDBNFD = 1;
	if (tcVersion[0]!='0' && (x_strcmp(tcNormalization, "NFC")==0))
	    pbaseprm->bUnicodeDBNFC = 1;
            }
            // Retrieve the current Catalog Page Size
            if (iret==RES_SUCCESS && GetOIVers() >= OIVERS_30)
               iret = GetCatalogPageSize(pbaseprm);

            if (iret==RES_SUCCESS)
               iret=ReleaseSession(*ilocsession, RELEASE_COMMIT);
            else
               ReleaseSession(*ilocsession, RELEASE_ROLLBACK);

            wsprintf(connectname,"%s::iidbdb",lpVirtNode);

            iret = Getsession(connectname, SessType, ilocsession);
            if (iret !=RES_SUCCESS)
               return RES_ERR;

            iret=GetInfBase(pbaseprm);
         }
         break;

      default :
         iret=RES_ERR;
   }

   if (bRetainSessForLock)  // The session was asked to be retained for...
      return iret;          // ... locking purposes, return without releasing.

   if (iret==RES_SUCCESS)
     iret=ReleaseSession(*ilocsession, RELEASE_COMMIT);
   else 
     ReleaseSession(*ilocsession, RELEASE_ROLLBACK);
   return iret;
}


//
// UK Sotheavut. December-1996
// This Function is added for OpenIngres DBMS V2.0

BOOL VDBA20xGetDetailPKeyInfo (LPTABLEPARAMS lpTS, LPTSTR lpVnode)
{
    int ires, hdl;
    TCHAR tchszConnection [100];
    wsprintf (tchszConnection,"%s::%s", lpVnode, lpTS->DBName);
    ires = Getsession (tchszConnection, SESSION_TYPE_CACHENOREADLOCK, &hdl);
    if (ires != RES_SUCCESS)
    {
        ires = ReleaseSession(hdl, RELEASE_COMMIT);
        return FALSE;
    }
    lpTS->lpPrimaryColumns = StringList_Done (lpTS->lpPrimaryColumns);
    ires = VDBA2xGetTablePrimaryKeyInfo (lpTS);
    if (ires != RES_SUCCESS)
    {
        ires = ReleaseSession(hdl, RELEASE_COMMIT);
        return FALSE;
    }
    ires = ReleaseSession(hdl, RELEASE_COMMIT);
    return TRUE;
}

//
// UK Sotheavut. December-1996
// This Function is added for OpenIngres DBMS V2.0

BOOL VDBA20xGetDetailUKeyInfo (LPTABLEPARAMS lpTS, LPTSTR lpVnode)
{
    int ires, hdl;
    TCHAR tchszConnection [100];
    wsprintf (tchszConnection,"%s::%s", lpVnode, lpTS->DBName);
    ires = Getsession (tchszConnection, SESSION_TYPE_CACHENOREADLOCK, &hdl);
    if (ires != RES_SUCCESS)
    {
        ires = ReleaseSession(hdl, RELEASE_COMMIT);
        return FALSE;
    }
    lpTS->lpUnique = VDBA20xTableUniqueKey_Done (lpTS->lpUnique);
    ires = VDBA2xGetTableUniqueKeyInfo (lpTS);
    if (ires != RES_SUCCESS)
    {
        ires = ReleaseSession(hdl, RELEASE_COMMIT);
        return FALSE;
    }
    ires = ReleaseSession(hdl, RELEASE_COMMIT);
    return TRUE;
}

int VDBAGetCommentInfo ( LPTSTR lpNodeName,LPTSTR lpDBName, LPTSTR lpObjName,
                         LPTSTR lpObjOwner,LPTSTR *lpObjComment, LPOBJECTLIST lpObjColumn)
{
    int ires, hdl;
    TCHAR tchszConnection [MAXOBJECTNAME*2];
    wsprintf (tchszConnection,"%s::%s", lpNodeName, lpDBName);
    ires = Getsession (tchszConnection, SESSION_TYPE_CACHENOREADLOCK, &hdl);
    if (ires != RES_SUCCESS)
        return RES_ERR;

    ires = SQLGetComment( lpNodeName, lpDBName, lpObjName, lpObjOwner,
                          lpObjComment, lpObjColumn);

   if (ires==RES_SUCCESS)
     ires=ReleaseSession(hdl, RELEASE_COMMIT);
   else 
     ReleaseSession(hdl, RELEASE_ROLLBACK);

    return ires;
}
