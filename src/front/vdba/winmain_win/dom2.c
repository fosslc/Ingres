/***********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**   Project : Ingres Visual DBA
**
**    Source : dom2.c
**    Manage objects add/alter/drop inside a dom type mdi document
**
**    Authors : Emmanuel Blattes
**              Vut Uk
**              Francois Noirot-Nerin
**
**    History after 29-Dec-1999
**
**     05-Jan-2000 (noifr01)
**      bug 99866: (dropping an ICE variable doesn't work). 
**      the section of DomDropObject() for the OT_ICE_SERVER_VARIABLE
**      type was not complete (however, the functionality will be
**      anyhow be disabled (in dbamisc.c) until the ICE API supports it)
**    19-Jan-2000 (noifr01)
**      (bug 100063) (remove build warnings). In this source, 3 assignements
**      were (not) done because of a typo error (such as objType == OT_TABLE instead of
**      objType = OT_TABLE). This did not result in a bug, because the equivalent
**      branch is not provided in DOM windows. The fix is done (rather than removing
**     the code), in the case where the branch would be added in the future
**     (the problem would have been when de-registering a table/view/ procedure 
**     in a STAR database from a "schema" sub-branch. When de-registering from a
**     regular branch, the "type" getting assigned doesn't need to be changed into
**     the same value)
**    08-Feb-2000 (schph01)
**     bug #100388 ( drop view doesn't work ) fill the structure associate
**     with the view object.(field objectname and szSchema).
**    01-Mar-2000 (noifr01)
**     bug 100653: added a messagebox (with access to the "history of errors")
**     when trying to alter an ICE object but there has been an error (usually
**     from the ICE api) while trying to get the properties of the object
**    03-Mar-2000 (schph01)
**      Additional change for Bug #97680, Fill the structure
**      PROCEDUREPARAMS.objectname without the owner name and without the
**      special caractere for display.
**      fill also PROCEDUREPARAMS.objectowner now used in the sql statement.
**    02-Aug-2000 (schph01)
**      bug #102127
**      Initialize the field ICEBUSUNITDOCDATA.suffix with a empty value,
**      because in low level, when this field is empty it is filled with the
**      file extention found in ICEBUSUNITDOCDATA.name.
**    26-Mar-2001 (noifr01)
**      (sir 104270) removal of code for managing Ingres/Desktop
**    09-May-2001 (schph01)
**       SIR 102509  launch the new dialog for created and alter a location.
** 18-Jun-2001 (uk$so01)
**    BUG #104799, Deadlock on modify structure when there exist an opened 
**    cursor on the table.
** 21-Jun-2001 (noifr01/schph01)
**    (bug 105076) fixed a mistake in a test for "modify structure"
** 10-Jul-2001 (hanje04)
**	ansiapi.h does not exist for MAINWIN 4.02. Don't include for MAINWIN.
** 10-Dec-2001 (noifr01)
**  (sir 99596) removal of obsolete code and resources
** 28-Mar-2002 (noifr01)
**  (sir 99596) removed additional unused resources/sources
** 18-Mar-2003 (schph01)
**  sir 107523 Add Branches 'Sequence' and 'Grantees Sequence'
** 15-Apr-2003 (schph01)
**  bug 110073 fill the GRANTPARAMS.privileges[GRANT_EXECUTE] structure 
**  before launch the dialog box,
**  When this boolean array is not initilazed with grant type,the function
**  to make the SQL statement return an empty string.
** 15-May-2003 (schph01)
**  bug 110247 update bUnicodeDatabase with the information retrieve by 
**  GetDetailInfo()
** 01-Oct-2003 (schph01)
**  bug 111019 when added OT_ICE_BUNIT_PAGE_USER or OT_ICE_BUNIT_FACET_USER
**  it is necessary to udpdated OT_ICE_BUNIT_SEC_USER type.
**  Drop OT_ICE_BUNIT_PAGE_USER or OT_ICE_BUNIT_FACET_USER needed refresh 
**  OT_ICE_BUNIT_SEC_USER object type.
**  Drop of OT_ICE_WEBUSER needed refresh of OT_ICE_BUNIT object type
** 03-Nov-2003 (noifr01 and schph01)
**  fixed propagation error upon massive ingres30->main gui tools code
**  propagation
** 07-Jun-2004 (schph01)
**  SIR #112460 update structure member szCatalogsPageSize before launched
**  Alter database dialog
** 05-Aug-2004 (uk$so01)
**    SIR 111014 (build GUI tools in new build environment)
**    Remove the #include <ansiapi.h> and replace A2W by CMmbstowcs
** 06-Sep-2005) (drivi01)
**    Bug #115173 Updated createdb dialogs and alterdb dialogs to contain
**    two available unicode normalization options, added group box with
**    two checkboxes corresponding to each normalization, including all
**    expected functionlity.
** 12-May-2010 (drivi01)
**    Add assignment of stored address location from LPTREERECORD 
**    to tblType in DMLCREATESTRUCT.
**    Add DomCreateIndex function which will bring up "Create Index"
**    dialog for the newly added "Create Index" menu. 
** 04-Jun-2010 (horda03) b123227
**    Allow GRANT on SEQUENCES to be dropped.
************************************************************************/

//
// Includes
//
// esql and so forth management
#include "dba.h"
#include "domdata.h"
#include "domdisp.h"
#include "dbaginfo.h"
#include "error.h"
#include "main.h"
#include "resource.h"
#include "winutils.h"
#include "treelb.e"   // tree list dll
#include "dbadlg1.h"  // dialog boxes part one
#include "getdata.h"
#include "msghandl.h"
#include "dll.h"
#include "dlgres.h"
#include "monitor.h"
#include "starutil.h"
#include "dgerrh.h"   // MessageWithHistoryButton()
#include "extccall.h" // extern "C" interface header functions
#include "ice.h"
#include "tools.h"

extern BOOL MfcDlgCreateDatabase       ( LPCREATEDB          lpcreatedb );
extern BOOL MfcDlgCreateIceRole        ( LPICEROLEDATA       lpcreateIRole,    int nHnode);
extern BOOL MfcDlgCreateIceDbUser      ( LPICEDBUSERDATA     lpCreateDbUser,   int nHnode);
extern BOOL MfcDlgCreateIceDbConnection( LPDBCONNECTIONDATA  lpCreateDbConnect,int nHnode);
extern BOOL MfcDlgCreateIceWebUser     ( LPICEWEBUSERDATA    lpcreateWebUser,  int nHnode);
extern BOOL MfcDlgCreateIceProfile     ( LPICEPROFILEDATA    lpcreateIceProf,  int nHnode);
extern BOOL MfcDlgCreateIceFacetPage   ( LPICEBUSUNITDOCDATA lpcreateIceDoc,   int nHnode);
extern BOOL MfcDlgCreateIceBusinessRole( LPICEBUSUNITROLEDATA lpBuRole,        int nHnode);
extern BOOL MfcDlgCreateIceBusinessUser( LPICEBUSUNITWEBUSERDATA lpBusUnitUser ,int nHnode);
extern BOOL MfcDlgCommonCombobox       ( int nType ,char *szParentName ,int nHnode );
extern BOOL MfcDlgCommonEdit           ( int nType , int nHnode );
extern BOOL MfcDlgIceServerLocation    ( LPICELOCATIONDATA   lpcreateIceLoc ,int nHnode);
extern BOOL MfcDlgIceServerVariable    ( LPICESERVERVARDATA  lpIceVar ,int nHnode);
extern BOOL MfcDlgBuDefRoleUserForFacetPage( int nType ,LPVOID lpStruct,int nHnode);
extern BOOL MfcDlgCreateLocation       ( LPLOCATIONPARAMS lpLocParam);
extern BOOL MfcDlgCreateSequence       ( LPSEQUENCEPARAMS lpsequencedb );

void VDBA_TRACE0 (LPCTSTR lpszText);
static BOOL VDBA_GroupHasUsers()
{
	VDBA_TRACE0 ("VDBA_GroupHasUsers(): Not implemented yet... (for the future only). Do not forget to modify DBADropObject (OT_GROUP)\n");
	return FALSE;
}
//
// STAR specific
//
static int RemoveStarRegistration(HWND hwndDoc, char *DbName, int objType, char *objName)
{
  char buf[3*MAXOBJECTNAME];
  char stmt[3*MAXOBJECTNAME];
  int ilocsession;
  int iret;
  char *NodeName = GetVirtNodeName (GetMDINodeHandle (hwndDoc));


  // buid the statement
  switch (objType) {
    case OT_TABLE:
      wsprintf(stmt, "remove table %s", objName);
      break;
    case OT_VIEW:
      wsprintf(stmt, "remove view %s", objName);
      break;
    case OT_PROCEDURE:
      wsprintf(stmt, "remove procedure %s", objName);
      break;
    default:
      return RES_ERR;
  }


  ShowHourGlass();

  // Get a session on the ddb (note: accepts "(local)" as a node name)
  wsprintf(buf, "%s::%s", NodeName, DbName);
  iret = Getsession(buf, SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
  if (iret !=RES_SUCCESS) {
    RemoveHourGlass ();
    MessageBox(hwndDoc, ResourceString(IDS_ERR_GET_SESSION), NULL, MB_OK|MB_ICONSTOP);
    return RES_ERR;
  }

  // execute the statement
  iret=ExecSQLImm(stmt, FALSE, NULL, NULL, NULL);

  RemoveHourGlass ();

  // release the session
  ReleaseSession(ilocsession, iret==RES_SUCCESS ? RELEASE_COMMIT : RELEASE_ROLLBACK);
  if (iret !=RES_SUCCESS) {
    //_T("Cannot Remove Object From Distributed Database")
    MessageWithHistoryButton(hwndDoc, ResourceString(IDS_ERR_REMOVE_OBJECT));
    return RES_ERR;
  }

  return RES_SUCCESS;
}

static char* GetCDBName(int hnodestruct, char* ddbName, char* bufResult)
{
  LPUCHAR aparentsResult[MAXPLEVEL];
  UCHAR   bufParResult0[MAXOBJECTNAME];
  UCHAR   bufParResult1[MAXOBJECTNAME];
  UCHAR   bufParResult2[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];
  LPUCHAR aparentsTemp[MAXPLEVEL];
  int     iret;

  aparentsResult[0] = bufParResult0;
  aparentsResult[1] = bufParResult1;
  aparentsResult[2] = bufParResult2;

  aparentsTemp[0] = ddbName;
  aparentsTemp[1] = aparentsTemp[2] = NULL;
  iret =  DOMGetFirstRelObject(hnodestruct,
                               OTR_CDB,
                               1,
                               aparentsTemp,
                               FALSE,
                               NULL,
                               NULL,
                               aparentsResult,
                               bufResult,
                               bufOwner,
                               bufComplim);
  if (iret != RES_SUCCESS)
    return "< unknown CDB >";
  return bufResult;
}


//
//  sets a received array of pointers on the parent strings
//  of the current selection in the tree
//
//  returns TRUE if successful
//
BOOL GetCurSelObjParents(LPDOMDATA lpDomData, LPUCHAR parentstrings[MAXPLEVEL])
{
   DWORD         dwCurSel;
   LPTREERECORD  lpRecord;

   dwCurSel=(DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
   if (dwCurSel)
   {
       lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                              LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
       if (lpRecord)
       {
           parentstrings[0] = lpRecord->extra;
           parentstrings[1] = lpRecord->extra2;
           parentstrings[2] = lpRecord->extra3;
           return TRUE;       // Success
       }
   }
   return FALSE;  // Error
}

//
//  returns the complimentary long of the current selection in the tree
//
//  returns -1 in case of error
//
long GetCurSelComplimLong(LPDOMDATA lpDomData)
{
   DWORD        dwCurSel;
   LPTREERECORD lpRecord;

   dwCurSel=(DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
   if (dwCurSel) {
       lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                            LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
       if (lpRecord)
           return lpRecord->complimValue;
   }
   return(-1L);
}

static BOOL NotVisibleDueToFilters(LPDOMDATA lpDomData, int iobjecttype)
{
  BOOL bWithSystem = lpDomData->bwithsystem;
  LPUCHAR lpowner = "";
  UCHAR username[MAXOBJECTNAME];
  BOOL bSuccess;

  // 1) no owner for objecttype ---> visible
  if (iobjecttype!= OT_DATABASE && !NonDBObjectHasOwner(iobjecttype))
    return FALSE; // visible

  // 2) nodeuser $ingres and systemobject not cheched ---> invisible
  bSuccess = DBAGetSessionUser(GetVirtNodeName(lpDomData->psDomNode->nodeHandle), username);
  if (!bSuccess)
    username[0] = '\0';
  if ( !x_stricmp(username, "$ingres") && !bWithSystem)
    return TRUE;  // invisible
  // 3) no filter ---> visible
  if (iobjecttype == OT_DATABASE) {
      lpowner = lpDomData->lpBaseOwner;
  }
  else
    lpowner = lpDomData->lpOtherOwner;
  if (lpowner[0] == '\0')
    return FALSE; // visible

  // 4) filter same as nodeuser ---> visible
  if ( !x_stricmp(lpowner, username))
    return FALSE; // visible

  // 5) invisible
  return TRUE;
}

static int GetCurSelUnsolvedItemObjType(LPDOMDATA lpDomData)
{
  DWORD dwItem = (DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
  if (dwItem) {
    LPTREERECORD  lpRecord;
    lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                              LM_GETITEMDATA, 0, (LPARAM)dwItem);
    if (lpRecord)
      if (lpRecord->unsolvedRecType == -1)
        return lpRecord->recType;
      else
        return lpRecord->unsolvedRecType;
  }
  return -1;  // Error
}

//
// VOID DomAddObject(HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam)
//
// Adds an object according to type of the current selection in the lbtree
//
// At this time, hwndMdi, wParam and lParam not used
//
// TO BE FINISHED

VOID DomAddObject (HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam)
{
   int          objType;          
   int          i,iret;           
   int          level;
   LPUCHAR      parentstrings [MAXPLEVEL];
   UCHAR        Parents[MAXPLEVEL][MAXOBJECTNAME];
   UCHAR        ParentTableOwner[MAXOBJECTNAME];
   DWORD        dwCurSel;
   LPTREERECORD lpRecord;
   UCHAR        AttachedGroup[MAXOBJECTNAME];

   // parameters for multi-call to UpdateDomData/DomUpdateDisplayData
   // (ex:securities)
   int iter;
   int aObjType[10];       // CAREFUL WITH THIS MAX VALUE!
   int alevel  [10];
   int cpt;

   // "Create table at star level" special management: must refresh 2 tables lists
   BOOL   bMustRefreshStarLocalDB = FALSE;
   char   szStarLocalNodeName[MAXOBJECTNAME];
   char   szStarLocalDbName[MAXOBJECTNAME];
   int    starNodeHandle;
   char* tempParentstrings0;

   // dummy instructions to skip compiler warnings at level four
   wParam;
   lParam;

   objType = GetCurSelUnsolvedItemObjType (lpDomData);
   dwCurSel=(DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
   if (!dwCurSel)
       return;             // How did we happen here ?
   lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                               LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

   fstrncpy(ParentTableOwner,lpRecord->tableOwner,MAXOBJECTNAME);
   if (!GetCurSelObjParents (lpDomData, parentstrings))
       return;             // TESTER RETVAL
   for (i=0;i<3;i++) {
       fstrncpy (Parents[i], parentstrings[i], MAXOBJECTNAME);
       parentstrings[i]=Parents[i];
   }

   switch (objType)
   {
       case OT_STATIC_LOCATION:
       case OT_LOCATION:
       {
           LOCATIONPARAMS locparams;
           ZEROINIT (locparams);

           locparams.bCreate = TRUE;
           iret = MfcDlgCreateLocation( &locparams );
           if (!iret)
               return;
       }
       objType = OT_LOCATION;
       level   = 0;
       break;
   // JFS Begin
       case OT_REPLIC_CDDS:
       case OT_STATIC_REPLIC_CDDS:
       case OT_REPLIC_CDDS_V11:
           {
           REPCDDS cdds;
           int SesHndl,nReplicVersion;
           ZEROINIT (cdds);

           x_strcpy (cdds.DBName, parentstrings [0]);     /* Database    */

           {   // Get Database owner: replicable tables should have the samoe
               // owner for this version of replicator
               int restype;
               UCHAR buf[MAXOBJECTNAME];
               if ( DOMGetObjectLimitedInfo(lpDomData->psDomNode->nodeHandle,
                                            parentstrings [0],
                                            "",
                                            OT_DATABASE,
                                            0,
                                            parentstrings,
                                            TRUE,
                                            &restype,
                                            buf,
                                            cdds.DBOwnerName,
                                            buf
                                           )
                                             !=RES_SUCCESS
                  )
                   return;
           }

           nReplicVersion=GetReplicInstallStatus( lpDomData->psDomNode->nodeHandle,
                                                  parentstrings [0],
                                                  cdds.DBOwnerName);
           iret = GetDetailInfo (GetVirtNodeName(lpDomData->psDomNode->nodeHandle),
                                 GetRepObjectType4ll(OT_REPLIC_CDDS,nReplicVersion),
                                 &cdds, FALSE,
                                 &SesHndl);

           if (iret != RES_SUCCESS || nReplicVersion == REPLIC_NOINSTALL)
              return;
           if (nReplicVersion == REPLIC_V10 ||
               nReplicVersion == REPLIC_V105  ) {
              cdds.iReplicType=nReplicVersion;
              iret = DlgCDDS (hwndMdi, &cdds);
           }
           else 
              iret = DlgCDDSv11 (hwndMdi, &cdds);
           
           if (!iret)
              return;

           objType = OT_REPLIC_CDDS;
           level   = 1;
           }
       break;

   // JFS End

       case OT_STATIC_PROFILE:
       case OT_PROFILE:
       {
           PROFILEPARAMS p;
           ZEROINIT (p);

           p.bCreate = TRUE;
           iret = DlgCreateProfile (hwndMdi, &p);

           if (!iret)
               return; 
       }
       objType = OT_PROFILE;
       level   = 0;
       break;

       case OT_STATIC_USER:
       case OT_USER:
       {
           USERPARAMS user;
           ZEROINIT (user);

           user.bCreate = TRUE;
           iret = DlgCreateUser (hwndMdi, &user);
           if (!iret)
               return; 
           x_strcpy(AttachedGroup,user.DefaultGroup);
           parentstrings[1]=AttachedGroup;
       }
       objType = OT_USER;
       level   = 0;
       break;

       //case OT_STATIC_DATABASE:


       case OT_STATIC_GROUP:
       case OT_GROUP:
       {

           GROUPPARAMS groupparams;
           ZEROINIT (groupparams);

           groupparams.bCreate = TRUE;
           iret = DlgCreateGroup (hwndMdi, &groupparams);
           if (!iret)
               return; 
       }
       objType = OT_GROUP;
       level   = 0;
       break;


       case OT_STATIC_GROUPUSER:
       case OT_GROUPUSER:
       {
           GROUPUSERPARAMS gu;
           ZEROINIT (gu);

           x_strcpy (gu.GroupName, parentstrings [0]);
           iret = DlgAddUser2Group (hwndMdi, &gu);
           if (!iret)
               return;
       }

       objType = OT_GROUPUSER;
       level   = 1;
       break;

       case OT_STATIC_RULE:
       case OT_RULE:
       {
              RULEPARAMS rule;
              ZEROINIT  (rule);

              rule.bCreate = TRUE;
              x_strcpy (rule.DBName   , parentstrings [0]);
              x_strcpy (rule.TableName, RemoveDisplayQuotesIfAny(StringWithoutOwner(parentstrings [1])));
              x_strcpy (rule.TableOwner, ParentTableOwner);
       
              iret = DlgCreateRule (hwndMdi, &rule);
              if (!iret)
                  return;
       }
       objType = OT_RULE;
       level   = 2;
       break;



       case OT_STATIC_ROLE:
       case OT_ROLE:
       {
           ROLEPARAMS role;
           ZEROINIT (role);
           role.lpfirstdb = NULL;
           role.bCreate   = TRUE;

           iret = DlgCreateRole (hwndMdi, &role);
           if (iret != IDOK)
               return;
       }
       objType = OT_ROLE;
       level   = 0;
       break;

       case OT_STATIC_TABLE:
       case OT_TABLE:
       {
              TABLEPARAMS tbl;
              ZEROINIT(tbl);

              tbl.bCreate = TRUE;
              x_strcpy (tbl.DBName   , parentstrings [0]);
              // new fields for star
              tbl.hCurNode = lpDomData->psDomNode->nodeHandle;
              tbl.bDDB =  IsStarDatabase(tbl.hCurNode, tbl.DBName);

              //"Warning: The Recommended Method is to Create the Table\n"
              //"On the Source Database, then to Register it As Link.\n"
              //"Do you Still Wish to Create the Table at Star Level?"
              //"Create Table at Star Level"
              if (tbl.bDDB) {
                if (MessageBox (GetFocus(), ResourceString(IDS_ERR_WARNING_STAR),
                    ResourceString(IDS_TITLE_CREATE_TBL_STAR),
                    MB_ICONQUESTION | MB_YESNO) == IDNO)
                  return;
              }

              iret = DlgCreateTable(hwndMdi, &tbl);

              if (iret) {
                if (tbl.bDDB) {
                  bMustRefreshStarLocalDB = TRUE;
                  if (tbl.bCoordinator) {
                    UCHAR bufResult[MAXOBJECTNAME];
                    x_strcpy(szStarLocalNodeName, GetVirtNodeName(tbl.hCurNode));
                    x_strcpy(szStarLocalDbName, GetCDBName(tbl.hCurNode, tbl.DBName, bufResult));
                  }
                  else {
                    if (tbl.bLocalNodeIsLocal)
                      x_strcpy(szStarLocalNodeName, ResourceString((UINT)IDS_I_LOCALNODE));
                    else
                      x_strcpy(szStarLocalNodeName, tbl.szLocalNodeName);
                    x_strcpy(szStarLocalDbName, tbl.szLocalDBName);
                  }
                }
              }

              if (!iret)
                  return;

           objType = OT_TABLE;
           level = 1;
           break;

       }


       case OT_STATIC_VIEW:
       case OT_VIEW:
       {
           VIEWPARAMS v;
           ZEROINIT (v);

           v.bCreate = TRUE;
           x_strcpy (v.DBName, parentstrings [0]);

           // new test for star
           //"Warning: The Recommended Method is to Create the View\n"
           //"On the Source Database, then to Register it As Link.\n"
           //"Do you Still Wish to Create the View at Star Level?"
           //"Create View at Star Level"

           if (IsStarDatabase(lpDomData->psDomNode->nodeHandle, v.DBName)) {
              if (MessageBox (GetFocus(),ResourceString(IDS_ERR_WARNING_VIEW_STAR),
                  ResourceString(IDS_TITLE_CREATE_VIEW_STAR),
                  MB_ICONQUESTION | MB_YESNO) == IDNO)
                return;
            }
           iret = DlgCreateView (hwndMdi, &v);
           if (!iret)
               return;
       }
       level   = 1;
       objType = OT_VIEW;
       break;
      

       case OT_STATIC_PROCEDURE:
       case OT_PROCEDURE:
       {
           PROCEDUREPARAMS proc;
           ZEROINIT (proc);
           
           proc.bCreate = TRUE;
           proc.lpProcedureParams    = NULL;
           proc.lpProcedureDeclare   = NULL;
           proc.lpProcedureStatement = NULL;
           x_strcpy (proc.DBName, parentstrings [0]);

           iret = 0;
           iret = DlgCreateProcedure (hwndMdi, &proc);
           if (!iret)
              return;
       }
       level   = 1;
       objType = OT_PROCEDURE;
       break;

       case OT_STATIC_SEQUENCE:
       case OT_SEQUENCE:
       {
           SEQUENCEPARAMS sequence;
           LPUCHAR   lpVirtNode    = GetVirtNodeName(lpDomData->psDomNode->nodeHandle);
           ZEROINIT (sequence);

           sequence.bCreate = TRUE;
           lstrcpy (sequence.szVNode, lpVirtNode);
           x_strcpy (sequence.DBName, parentstrings [0]);

           iret = 0;
           iret = MfcDlgCreateSequence (&sequence);
           if (!iret)
              return;
       }
       level   = 1;
       objType = OT_SEQUENCE;
       break;


       case OT_STATIC_SECURITY:
       case OT_STATIC_SEC_SEL_SUCC :
       case OT_STATIC_SEC_SEL_FAIL :
       case OT_STATIC_SEC_DEL_SUCC :
       case OT_STATIC_SEC_DEL_FAIL :
       case OT_STATIC_SEC_INS_SUCC :
       case OT_STATIC_SEC_INS_FAIL :
       case OT_STATIC_SEC_UPD_SUCC :
       case OT_STATIC_SEC_UPD_FAIL :
       case OT_S_ALARM_SELSUCCESS_USER :
       case OT_S_ALARM_SELFAILURE_USER :
       case OT_S_ALARM_DELSUCCESS_USER :
       case OT_S_ALARM_DELFAILURE_USER :
       case OT_S_ALARM_INSSUCCESS_USER :
       case OT_S_ALARM_INSFAILURE_USER :
       case OT_S_ALARM_UPDSUCCESS_USER :
       case OT_S_ALARM_UPDFAILURE_USER :
       {
           SECURITYALARMPARAMS security;
           ZEROINIT (security);
           x_strcpy (security.DBName,         parentstrings [0]);        // Database name
           x_strcpy (security.PreselectTable, parentstrings [1]);        // Table name
           x_strcpy (security.PreselectTableOwner, ParentTableOwner);    // Table name
           
           switch (objType)
           {
               case OT_STATIC_SEC_SEL_SUCC :
               case OT_S_ALARM_SELSUCCESS_USER :
                   security.bsuccfail   [SECALARMSUCCESS]  = TRUE;
                   security.baccesstype [SECALARMSEL]      = TRUE;
                   break;
               case OT_STATIC_SEC_SEL_FAIL :
               case OT_S_ALARM_SELFAILURE_USER :
                   security.bsuccfail   [SECALARMFAILURE]  = TRUE;
                   security.baccesstype [SECALARMSEL]      = TRUE;
                   break;
               case OT_STATIC_SEC_DEL_SUCC :
               case OT_S_ALARM_DELSUCCESS_USER :
                   security.bsuccfail   [SECALARMSUCCESS]  = TRUE;
                   security.baccesstype [SECALARMDEL]      = TRUE;
                   break;
               case OT_STATIC_SEC_DEL_FAIL :
               case OT_S_ALARM_DELFAILURE_USER :
                   security.bsuccfail   [SECALARMFAILURE]  = TRUE;
                   security.baccesstype [SECALARMDEL]      = TRUE;
                   break;
               case OT_STATIC_SEC_INS_SUCC :
               case OT_S_ALARM_INSSUCCESS_USER :
                   security.bsuccfail   [SECALARMSUCCESS]  = TRUE;
                   security.baccesstype [SECALARMINS]      = TRUE;
                   break;
               case OT_STATIC_SEC_INS_FAIL :
               case OT_S_ALARM_INSFAILURE_USER :
                   security.bsuccfail   [SECALARMFAILURE]  = TRUE;
                   security.baccesstype [SECALARMINS]      = TRUE;
                   break;
               case OT_STATIC_SEC_UPD_SUCC :
               case OT_S_ALARM_UPDSUCCESS_USER :
                   security.bsuccfail   [SECALARMSUCCESS]  = TRUE;
                   security.baccesstype [SECALARMUPD]      = TRUE;
                   break;
               case OT_STATIC_SEC_UPD_FAIL :
               case OT_S_ALARM_UPDFAILURE_USER :
                   security.bsuccfail   [SECALARMFAILURE]  = TRUE;
                   security.baccesstype [SECALARMUPD]      = TRUE;
                   break;
               default:
                   break;
           }

           iret = DlgCreateSecurityAlarm (hwndMdi, &security);

           if (!iret)
               return;
       }
       objType = OT_STATIC_SECURITY;   // we will loop on 8 values
       level   = 2;
       break;

       case OT_STATIC_DBALARM                 :
       case OT_STATIC_DBALARM_CONN_SUCCESS    :
       case OT_STATIC_DBALARM_CONN_FAILURE    :
       case OT_STATIC_DBALARM_DISCONN_SUCCESS :
       case OT_STATIC_DBALARM_DISCONN_FAILURE :
       case OT_S_ALARM_CO_SUCCESS_USER        :
       case OT_S_ALARM_CO_FAILURE_USER        :
       case OT_S_ALARM_DI_SUCCESS_USER        :
       case OT_S_ALARM_DI_FAILURE_USER        :

       case OT_STATIC_INSTALL_ALARMS:
       {
           SECURITYALARMPARAMS security;
           ZEROINIT (security);
           x_strcpy (security.DBName,         parentstrings [0]);    // Database name
           x_strcpy (security.PreselectTable, parentstrings [0]);    // DBName
		   if (x_strlen(parentstrings [0])==0)
		      security.bInstallLevel = TRUE;
		   else
		      security.bInstallLevel = FALSE;

           switch (objType)
           {
               case OT_STATIC_DBALARM_CONN_SUCCESS :
               case OT_S_ALARM_CO_SUCCESS_USER     :
                   security.bsuccfail   [SECALARMSUCCESS]  = TRUE;
                   security.baccesstype [SECALARMCONNECT]  = TRUE;
                   break;
               case OT_STATIC_DBALARM_CONN_FAILURE :
               case OT_S_ALARM_CO_FAILURE_USER     :
                   security.bsuccfail   [SECALARMFAILURE]  = TRUE;
                   security.baccesstype [SECALARMCONNECT]  = TRUE;
                   break;
               case OT_STATIC_DBALARM_DISCONN_SUCCESS :
               case OT_S_ALARM_DI_SUCCESS_USER        :
                   security.bsuccfail   [SECALARMSUCCESS]  = TRUE;
                   security.baccesstype [SECALARMDISCONN]  = TRUE;
                   break;
               case OT_STATIC_DBALARM_DISCONN_FAILURE :
               case OT_S_ALARM_DI_FAILURE_USER        :
                   security.bsuccfail   [SECALARMFAILURE]  = TRUE;
                   security.baccesstype [SECALARMDISCONN]  = TRUE;
                   break;
           }
           iret = DlgCreateSecurityAlarm2 (hwndMdi, &security);

           if (!iret)
               return;


       }
       objType = OTLL_DBSECALARM;   
       level   = 0;
       break;


       /* _____________________________________________________________________
       //                                                                      &
       // Create security alarm (Reference) Preselect : SUCCESS, FAILURE       &
       //                     (Select, Insert, Update, or Delete)              &
       // _____________________________________________________________________&
       */
  
       case OT_STATIC_R_SEC_SEL_SUCC:
       case OT_STATIC_R_SEC_SEL_FAIL:
       case OT_STATIC_R_SEC_DEL_SUCC:
       case OT_STATIC_R_SEC_DEL_FAIL:
       case OT_STATIC_R_SEC_INS_SUCC:
       case OT_STATIC_R_SEC_INS_FAIL:
       case OT_STATIC_R_SEC_UPD_SUCC:
       case OT_STATIC_R_SEC_UPD_FAIL:
       {
           SECURITYALARMPARAMS security;
           ZEROINIT (security);
           x_strcpy (security.PreselectUser, parentstrings [0]);    // User name

           switch (objType)
           {
               case OT_STATIC_R_SEC_SEL_SUCC:
                   security.bsuccfail   [SECALARMSUCCESS]  = TRUE;
                   security.baccesstype [SECALARMSEL]      = TRUE;
                   break;
               case OT_STATIC_R_SEC_SEL_FAIL:
                   security.bsuccfail   [SECALARMFAILURE]  = TRUE;
                   security.baccesstype [SECALARMSEL]      = TRUE;
                   break;
               case OT_STATIC_R_SEC_DEL_SUCC:
                   security.bsuccfail   [SECALARMSUCCESS]  = TRUE;
                   security.baccesstype [SECALARMDEL]      = TRUE;
                   break;
               case OT_STATIC_R_SEC_DEL_FAIL:
                   security.bsuccfail   [SECALARMFAILURE]  = TRUE;
                   security.baccesstype [SECALARMDEL]      = TRUE;
                   break;
               case OT_STATIC_R_SEC_INS_SUCC:
                   security.bsuccfail   [SECALARMSUCCESS]  = TRUE;
                   security.baccesstype [SECALARMINS]      = TRUE;
                   break;
               case OT_STATIC_R_SEC_INS_FAIL:
                   security.bsuccfail   [SECALARMFAILURE]  = TRUE;
                   security.baccesstype [SECALARMINS]      = TRUE;
                   break;
               case OT_STATIC_R_SEC_UPD_SUCC:
                   security.bsuccfail   [SECALARMSUCCESS]  = TRUE;
                   security.baccesstype [SECALARMUPD]      = TRUE;
                   break;
               case OT_STATIC_R_SEC_UPD_FAIL:
                   security.bsuccfail   [SECALARMFAILURE]  = TRUE;
                   security.baccesstype [SECALARMUPD]      = TRUE;
                   break;
               default:
                   break;
           }

           iret = DlgRefSecurity (hwndMdi, &security);
           fstrncpy(parentstrings[0], security.DBName,MAXOBJECTNAME);
           if (!iret)
               return;
       }
       objType = OT_STATIC_SECURITY;   // we will loop on 8 values
       level   = 2;
       break;

       
       case OT_STATIC_INTEGRITY:
       case OT_INTEGRITY:
       {
           INTEGRITYPARAMS integrityparams;
           ZEROINIT (integrityparams);

           integrityparams.bCreate    = TRUE;
           x_strcpy (integrityparams.DBName   , parentstrings [0]); 
           x_strcpy (integrityparams.TableName, RemoveDisplayQuotesIfAny(StringWithoutOwner(parentstrings [1])));
           x_strcpy (integrityparams.TableOwner, ParentTableOwner); 

           iret = DlgCreateIntegrity (hwndMdi, &integrityparams);
           if (!iret)
               return;
       }
       objType = OT_INTEGRITY;
       level   = 2;
       break;


       case OT_STATIC_DBEVENT:
       case OT_DBEVENT:
       {
           DBEVENTPARAMS dbeventparams;
           ZEROINIT (dbeventparams);

           dbeventparams.bCreate    = TRUE;
           x_strcpy (dbeventparams.DBName   , parentstrings [0]); 

           iret = 0;
           iret = DlgCreateDBevent (hwndMdi, &dbeventparams);
           if (!iret)
               return;
       }
       objType = OT_DBEVENT;
       level   = 1;
       break;


       case OT_STATIC_SYNONYM:
       case OT_SYNONYM:
       {
           SYNONYMPARAMS synonym;
           ZEROINIT (synonym);
           synonym.bCreate = TRUE;
           x_strcpy (synonym.DBName   , parentstrings [0]); 

           iret = DlgCreateSynonym (hwndMdi, &synonym);
           if (!iret)
               return;
       }
       objType = OT_SYNONYM;
       level   = 1;
       break;



       case OT_STATIC_R_INDEXSYNONYM:
       case OTR_INDEXSYNONYM:
       {
           SYNONYMPARAMS synonym;
           DWORD         dwParent;
           LPTREERECORD  lpRecordParent;

           ZEROINIT (synonym);
           x_strcpy (synonym.DBName       ,  parentstrings [0]);
           x_strcpy (synonym.RelatedObject,  RemoveDisplayQuotesIfAny(StringWithoutOwner(parentstrings [2])));

          dwParent = (DWORD)SendMessage(lpDomData->hwndTreeLb,
                                        LM_GETPARENT, 0, dwCurSel);
          if (!dwParent)
              return;
          lpRecordParent = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                      LM_GETITEMDATA, 0, (LPARAM)dwParent);
          fstrncpy (synonym.RelatedObjectOwner,
                    lpRecordParent->ownerName, MAXOBJECTNAME);

           lpHelpStack = StackObject_PUSH (lpHelpStack,
                              StackObject_INIT ((UINT)IDD_CREATESYN_IDX));
           iret = DlgCreateSynonymOnObject (hwndMdi, &synonym);
           lpHelpStack = StackObject_POP (lpHelpStack);
           if (!iret)
               return;
       }
       objType = OT_SYNONYM;
       level   = 1;
       break;


       case OT_STATIC_R_TABLESYNONYM :
       case OTR_TABLESYNONYM:
       {
           SYNONYMPARAMS synonym;
           ZEROINIT (synonym);
           x_strcpy (synonym.DBName,        parentstrings [0]); 
           x_strcpy (synonym.RelatedObject, RemoveDisplayQuotesIfAny(StringWithoutOwner(parentstrings [1]))); 
           x_strcpy (synonym.RelatedObjectOwner, ParentTableOwner); 

           lpHelpStack = StackObject_PUSH (lpHelpStack,
                              StackObject_INIT ((UINT)IDD_CREATESYN_TAB));
           iret = DlgCreateSynonymOnObject (hwndMdi, &synonym);
           lpHelpStack = StackObject_POP (lpHelpStack);
           if (!iret)
               return;
       }
       objType = OT_SYNONYM;
       level   = 1;
       
       break;


       case OT_STATIC_R_VIEWSYNONYM :
       case OTR_VIEWSYNONYM:
       {
           SYNONYMPARAMS synonym;
           ZEROINIT (synonym);
           x_strcpy (synonym.DBName   ,      parentstrings [0]);
           x_strcpy (synonym.RelatedObject,  RemoveDisplayQuotesIfAny(StringWithoutOwner(parentstrings [1])));
           x_strcpy (synonym.RelatedObjectOwner, ParentTableOwner); 

           lpHelpStack = StackObject_PUSH (lpHelpStack,
                                StackObject_INIT ((UINT)IDD_CREATESYN_VIEW));
           iret = DlgCreateSynonymOnObject (hwndMdi, &synonym);
           lpHelpStack = StackObject_POP (lpHelpStack);
           if (!iret)
               return;
       }
       objType = OT_SYNONYM;
       level   = 1;

       break;




       case OT_STATIC_INDEX:
       case OT_INDEX:
       {

                if (GetOIVers() == OIVERS_NOTOI) { // generic gateway: simplified dialog box
                    INDEXPARAMS idx;
                    ZEROINIT(idx);
                    idx.bPersistence = TRUE;
                    idx.bCreate = TRUE;
                    x_strcpy (idx.DBName   , parentstrings [0]); 
                    x_strcpy (idx.TableName, parentstrings [1]);
                    x_strcpy (idx.TableOwner, ParentTableOwner);

                    iret = DlgCreateIndex (hwndMdi, &idx);
                    if (iret != IDOK)
                        return;
                }
                else {
                    DMLCREATESTRUCT cr;
                    memset (&cr, 0, sizeof (cr));
                    lstrcpy (cr.tchszDatabase,    (LPCTSTR)parentstrings [0]);
                    lstrcpy (cr.tchszObject,      (LPCTSTR)RemoveDisplayQuotesIfAny(StringWithoutOwner(parentstrings [1])));
                    lstrcpy (cr.tchszObjectOwner, (LPCTSTR)ParentTableOwner);
		    cr.tblType = getint(lpRecord->szComplim + STEPSMALLOBJ + STEPSMALLOBJ);

                    iret = VDBA_CreateIndex(hwndMdi, &cr);
                    if (iret != IDOK)
                        return;
                }

           objType=OT_INDEX;
           level   = 2;
           break;
       }

       case OT_STATIC_TABLEGRANTEES:
       case OT_STATIC_TABLEGRANT_SEL_USER:
       case OT_STATIC_TABLEGRANT_INS_USER:
       case OT_STATIC_TABLEGRANT_UPD_USER:
       case OT_STATIC_TABLEGRANT_DEL_USER:
       case OT_STATIC_TABLEGRANT_CPI_USER:
       case OT_STATIC_TABLEGRANT_CPF_USER:
       case OT_STATIC_TABLEGRANT_REF_USER:
       case OT_STATIC_TABLEGRANT_ALL_USER:
       case OT_STATIC_TABLEGRANT_INDEX_USER:
       case OT_STATIC_TABLEGRANT_ALTER_USER:
       case OT_TABLEGRANT_SEL_USER:
       case OT_TABLEGRANT_INS_USER:
       case OT_TABLEGRANT_UPD_USER:
       case OT_TABLEGRANT_DEL_USER:
       case OT_TABLEGRANT_CPI_USER:
       case OT_TABLEGRANT_CPF_USER:
       case OT_TABLEGRANT_REF_USER:
       case OT_TABLEGRANT_INDEX_USER:
       case OT_TABLEGRANT_ALTER_USER:
       case OT_TABLEGRANT_ALL_USER:
       case OT_STATIC_VIEWGRANTEES:        
       case OT_STATIC_VIEWGRANT_SEL_USER:  
       case OT_STATIC_VIEWGRANT_INS_USER:  
       case OT_STATIC_VIEWGRANT_UPD_USER:  
       case OT_STATIC_VIEWGRANT_DEL_USER:  
       case OT_VIEWGRANT_SEL_USER:
       case OT_VIEWGRANT_INS_USER:
       case OT_VIEWGRANT_UPD_USER:
       case OT_VIEWGRANT_DEL_USER:
       {
           {
               GRANTPARAMS g;
               ZEROINIT (g);

               switch (objType)
               {
                   case OT_STATIC_VIEWGRANTEES:        
                   case OT_STATIC_VIEWGRANT_SEL_USER:  
                   case OT_STATIC_VIEWGRANT_INS_USER:  
                   case OT_STATIC_VIEWGRANT_UPD_USER:  
                   case OT_STATIC_VIEWGRANT_DEL_USER:
                   case OT_VIEWGRANT_SEL_USER:
                   case OT_VIEWGRANT_INS_USER:
                   case OT_VIEWGRANT_UPD_USER:
                   case OT_VIEWGRANT_DEL_USER:
                       g.ObjectType   = OT_VIEW;
                       break;

                   case OT_STATIC_TABLEGRANTEES:
                   case OT_STATIC_TABLEGRANT_SEL_USER:
                   case OT_STATIC_TABLEGRANT_INS_USER:
                   case OT_STATIC_TABLEGRANT_UPD_USER:
                   case OT_STATIC_TABLEGRANT_DEL_USER:
                   case OT_STATIC_TABLEGRANT_CPI_USER:
                   case OT_STATIC_TABLEGRANT_CPF_USER:
                   case OT_STATIC_TABLEGRANT_REF_USER:
                   case OT_STATIC_TABLEGRANT_ALL_USER:
                   case OT_TABLEGRANT_SEL_USER:
                   case OT_TABLEGRANT_INS_USER:
                   case OT_TABLEGRANT_UPD_USER:
                   case OT_TABLEGRANT_DEL_USER:
                   case OT_TABLEGRANT_CPI_USER:
                   case OT_TABLEGRANT_CPF_USER:
                   case OT_TABLEGRANT_REF_USER:
                   case OT_TABLEGRANT_ALL_USER:
                       g.ObjectType   = OT_TABLE;
                       break;
                   default:
                       break;
               }

               x_strcpy (g.DBName,          parentstrings [0]);      /* Database    */ 
               x_strcpy (g.PreselectObject, parentstrings [1]);      /* Table       */
               x_strcpy (g.PreselectObjectOwner, ParentTableOwner);  /* Table Owner */
           
               switch (objType)
               {
                   case OT_STATIC_TABLEGRANTEES:
                   case OT_STATIC_VIEWGRANTEES:
                       break;
                   case OT_STATIC_TABLEGRANT_SEL_USER:
                   case OT_TABLEGRANT_SEL_USER:
                   case OT_STATIC_VIEWGRANT_SEL_USER:  
                   case OT_VIEWGRANT_SEL_USER:
                       g.PreselectPrivileges [GRANT_SELECT]    = TRUE;
                       break;
                   case OT_STATIC_TABLEGRANT_INS_USER:
                   case OT_TABLEGRANT_INS_USER:
                   case OT_STATIC_VIEWGRANT_INS_USER:  
                   case OT_VIEWGRANT_INS_USER:
                       g.PreselectPrivileges [GRANT_INSERT]    = TRUE;
                       break;
                   case OT_STATIC_TABLEGRANT_UPD_USER:
                   case OT_TABLEGRANT_UPD_USER:
                   case OT_STATIC_VIEWGRANT_UPD_USER:  
                   case OT_VIEWGRANT_UPD_USER:
                       g.PreselectPrivileges [GRANT_UPDATE]    = TRUE;
                       break;
                   case OT_STATIC_TABLEGRANT_DEL_USER:
                   case OT_TABLEGRANT_DEL_USER:
                   case OT_STATIC_VIEWGRANT_DEL_USER:
                   case OT_VIEWGRANT_DEL_USER:
                       g.PreselectPrivileges [GRANT_DELETE]    = TRUE;
                       break;
                   case OT_STATIC_TABLEGRANT_CPI_USER:
                   case OT_TABLEGRANT_CPI_USER:
                       g.PreselectPrivileges [GRANT_COPY_INTO] = TRUE;
                       break;
                   case OT_STATIC_TABLEGRANT_CPF_USER:
                   case OT_TABLEGRANT_CPF_USER:
                       g.PreselectPrivileges [GRANT_COPY_FROM] = TRUE;
                       break;
                   case OT_STATIC_TABLEGRANT_REF_USER:
                   case OT_TABLEGRANT_REF_USER:
                       g.PreselectPrivileges [GRANT_REFERENCE] = TRUE;
                       break;
                   case OT_STATIC_TABLEGRANT_ALL_USER:
                   case OT_TABLEGRANT_ALL_USER:
                       g.PreselectPrivileges [GRANT_SELECT]    = TRUE;
                       g.PreselectPrivileges [GRANT_INSERT]    = TRUE;
                       g.PreselectPrivileges [GRANT_UPDATE]    = TRUE;
                       g.PreselectPrivileges [GRANT_DELETE]    = TRUE;
                       g.PreselectPrivileges [GRANT_COPY_INTO] = TRUE;
                       g.PreselectPrivileges [GRANT_COPY_FROM] = TRUE;
                       g.PreselectPrivileges [GRANT_REFERENCE] = TRUE;
                       break;
                   default:
                       break;
               }

               iret = DlgGrantTable (hwndMdi, &g);
               if (!iret)
                   return;
           }

       }
       objType = OTLL_GRANTEE;
       level   = 1;
       break;

       case OT_STATIC_R_DBGRANT:
       case OT_STATIC_DBGRANTEES:
       case OT_STATIC_DBGRANTEES_ACCESY:
       case OT_STATIC_DBGRANTEES_ACCESN:
       case OT_STATIC_DBGRANTEES_CREPRY:
       case OT_STATIC_DBGRANTEES_CREPRN:
       case OT_STATIC_DBGRANTEES_CRETBY:
       case OT_STATIC_DBGRANTEES_CRETBN:
       case OT_STATIC_DBGRANTEES_DBADMY:
       case OT_STATIC_DBGRANTEES_DBADMN:
       case OT_STATIC_DBGRANTEES_LKMODY:
       case OT_STATIC_DBGRANTEES_LKMODN:
       case OT_STATIC_DBGRANTEES_QRYIOY:
       case OT_STATIC_DBGRANTEES_QRYION:
       case OT_STATIC_DBGRANTEES_QRYRWY:
       case OT_STATIC_DBGRANTEES_QRYRWN:
       case OT_STATIC_DBGRANTEES_UPDSCY:
       case OT_STATIC_DBGRANTEES_UPDSCN:
       case OT_STATIC_DBGRANTEES_SELSCY:
       case OT_STATIC_DBGRANTEES_SELSCN:
       case OT_STATIC_DBGRANTEES_CNCTLY:
       case OT_STATIC_DBGRANTEES_CNCTLN:
       case OT_STATIC_DBGRANTEES_IDLTLY:
       case OT_STATIC_DBGRANTEES_IDLTLN:
       case OT_STATIC_DBGRANTEES_SESPRY:
       case OT_STATIC_DBGRANTEES_SESPRN:
       case OT_STATIC_DBGRANTEES_TBLSTY:
       case OT_STATIC_DBGRANTEES_TBLSTN:
       case OT_STATIC_DBGRANTEES_QRYCPY:
       case OT_STATIC_DBGRANTEES_QRYCPN:
       case OT_STATIC_DBGRANTEES_QRYPGY:
       case OT_STATIC_DBGRANTEES_QRYPGN:
       case OT_STATIC_DBGRANTEES_QRYCOY:
       case OT_STATIC_DBGRANTEES_QRYCON:
       case OT_STATIC_DBGRANTEES_CRSEQY:
       case OT_STATIC_DBGRANTEES_CRSEQN:

       case OT_STATIC_R_DBGRANT_ACCESY:
       case OT_STATIC_R_DBGRANT_ACCESN:
       case OT_STATIC_R_DBGRANT_CREPRY:
       case OT_STATIC_R_DBGRANT_CREPRN:
       case OT_STATIC_R_DBGRANT_CRETBY:
       case OT_STATIC_R_DBGRANT_CRETBN:
       case OT_STATIC_R_DBGRANT_DBADMY:
       case OT_STATIC_R_DBGRANT_DBADMN:
       case OT_STATIC_R_DBGRANT_LKMODY:
       case OT_STATIC_R_DBGRANT_LKMODN:
       case OT_STATIC_R_DBGRANT_QRYIOY:
       case OT_STATIC_R_DBGRANT_QRYION:
       case OT_STATIC_R_DBGRANT_QRYRWY:
       case OT_STATIC_R_DBGRANT_QRYRWN:
       case OT_STATIC_R_DBGRANT_UPDSCY:
       case OT_STATIC_R_DBGRANT_UPDSCN:
       case OT_STATIC_R_DBGRANT_SELSCY:
       case OT_STATIC_R_DBGRANT_SELSCN:
       case OT_STATIC_R_DBGRANT_CNCTLY:
       case OT_STATIC_R_DBGRANT_CNCTLN:
       case OT_STATIC_R_DBGRANT_IDLTLY:
       case OT_STATIC_R_DBGRANT_IDLTLN:
       case OT_STATIC_R_DBGRANT_SESPRY:
       case OT_STATIC_R_DBGRANT_SESPRN:
       case OT_STATIC_R_DBGRANT_TBLSTY:
       case OT_STATIC_R_DBGRANT_TBLSTN:

       case OT_STATIC_R_DBGRANT_QRYCPY:
       case OT_STATIC_R_DBGRANT_QRYCPN:
       case OT_STATIC_R_DBGRANT_QRYPGY:
       case OT_STATIC_R_DBGRANT_QRYPGN:
       case OT_STATIC_R_DBGRANT_QRYCOY:
       case OT_STATIC_R_DBGRANT_QRYCON:
       case OT_STATIC_R_DBGRANT_CRESEQY:
       case OT_STATIC_R_DBGRANT_CRESEQN:


       case OT_DBGRANT_ACCESY_USER: 
       case OT_DBGRANT_ACCESN_USER: 
       case OT_DBGRANT_CREPRY_USER: 
       case OT_DBGRANT_CREPRN_USER: 
       case OT_DBGRANT_CRETBY_USER: 
       case OT_DBGRANT_CRETBN_USER: 
       case OT_DBGRANT_DBADMY_USER: 
       case OT_DBGRANT_DBADMN_USER: 
       case OT_DBGRANT_LKMODY_USER: 
       case OT_DBGRANT_LKMODN_USER: 
       case OT_DBGRANT_QRYIOY_USER: 
       case OT_DBGRANT_QRYION_USER: 
       case OT_DBGRANT_QRYRWY_USER: 
       case OT_DBGRANT_QRYRWN_USER: 
       case OT_DBGRANT_UPDSCY_USER: 
       case OT_DBGRANT_UPDSCN_USER:
       case OT_DBGRANT_SELSCY_USER: 
       case OT_DBGRANT_SELSCN_USER: 
       case OT_DBGRANT_CNCTLY_USER: 
       case OT_DBGRANT_CNCTLN_USER: 
       case OT_DBGRANT_IDLTLY_USER: 
       case OT_DBGRANT_IDLTLN_USER: 
       case OT_DBGRANT_SESPRY_USER: 
       case OT_DBGRANT_SESPRN_USER: 
       case OT_DBGRANT_TBLSTY_USER: 
       case OT_DBGRANT_TBLSTN_USER: 

       case OT_DBGRANT_QRYCPY_USER: 
       case OT_DBGRANT_QRYCPN_USER: 
       case OT_DBGRANT_QRYPGY_USER: 
       case OT_DBGRANT_QRYPGN_USER: 
       case OT_DBGRANT_QRYCOY_USER: 
       case OT_DBGRANT_QRYCON_USER: 
       case OT_DBGRANT_SEQCRN_USER:
       case OT_DBGRANT_SEQCRY_USER:

       case OTR_DBGRANT_ACCESY_DB:
       case OTR_DBGRANT_ACCESN_DB:
       case OTR_DBGRANT_CREPRY_DB:
       case OTR_DBGRANT_CREPRN_DB:
       case OTR_DBGRANT_SEQCRY_DB:
       case OTR_DBGRANT_SEQCRN_DB:
       case OTR_DBGRANT_CRETBY_DB:
       case OTR_DBGRANT_CRETBN_DB:
       case OTR_DBGRANT_DBADMY_DB:
       case OTR_DBGRANT_DBADMN_DB:
       case OTR_DBGRANT_LKMODY_DB:
       case OTR_DBGRANT_LKMODN_DB:
       case OTR_DBGRANT_QRYIOY_DB:
       case OTR_DBGRANT_QRYION_DB:
       case OTR_DBGRANT_QRYRWY_DB:
       case OTR_DBGRANT_QRYRWN_DB:
       case OTR_DBGRANT_UPDSCY_DB:
       case OTR_DBGRANT_UPDSCN_DB:
       case OTR_DBGRANT_SELSCY_DB: 
       case OTR_DBGRANT_SELSCN_DB: 
       case OTR_DBGRANT_CNCTLY_DB: 
       case OTR_DBGRANT_CNCTLN_DB: 
       case OTR_DBGRANT_IDLTLY_DB: 
       case OTR_DBGRANT_IDLTLN_DB: 
       case OTR_DBGRANT_SESPRY_DB: 
       case OTR_DBGRANT_SESPRN_DB: 
       case OTR_DBGRANT_TBLSTY_DB: 
       case OTR_DBGRANT_TBLSTN_DB: 

       case OTR_DBGRANT_QRYCPY_DB: 
       case OTR_DBGRANT_QRYCPN_DB: 
       case OTR_DBGRANT_QRYPGY_DB: 
       case OTR_DBGRANT_QRYPGN_DB: 
       case OTR_DBGRANT_QRYCOY_DB: 
       case OTR_DBGRANT_QRYCON_DB: 


       case OT_STATIC_INSTALL_GRANTEES:
       {
           {
              int gtype;
              GRANTPARAMS g;
              ZEROINIT (g);

              g.ObjectType   = OT_DATABASE;
              x_strcpy (g.DBName,           parentstrings [0]);
              x_strcpy (g.PreselectObject,  parentstrings [0]);
              x_strcpy (g.PreselectGrantee, "");
			  if (x_strlen(parentstrings [0])==0)
				  g.bInstallLevel = TRUE;
			  else
				  g.bInstallLevel = FALSE;
              switch (objType)
              {
                  case OT_STATIC_R_DBGRANT:
                  case OT_STATIC_R_DBGRANT_ACCESY: 
                  case OT_STATIC_R_DBGRANT_ACCESN: 
                  case OT_STATIC_R_DBGRANT_CREPRY: 
                  case OT_STATIC_R_DBGRANT_CREPRN: 
                  case OT_STATIC_R_DBGRANT_CRETBY: 
                  case OT_STATIC_R_DBGRANT_CRETBN: 
                  case OT_STATIC_R_DBGRANT_DBADMY: 
                  case OT_STATIC_R_DBGRANT_DBADMN: 
                  case OT_STATIC_R_DBGRANT_LKMODY: 
                  case OT_STATIC_R_DBGRANT_LKMODN: 
                  case OT_STATIC_R_DBGRANT_QRYIOY: 
                  case OT_STATIC_R_DBGRANT_QRYION: 
                  case OT_STATIC_R_DBGRANT_QRYRWY: 
                  case OT_STATIC_R_DBGRANT_QRYRWN: 
                  case OT_STATIC_R_DBGRANT_UPDSCY: 
                  case OT_STATIC_R_DBGRANT_UPDSCN:
                  case OT_STATIC_R_DBGRANT_SELSCY:
                  case OT_STATIC_R_DBGRANT_SELSCN:
                  case OT_STATIC_R_DBGRANT_CNCTLY:
                  case OT_STATIC_R_DBGRANT_CNCTLN:
                  case OT_STATIC_R_DBGRANT_IDLTLY:
                  case OT_STATIC_R_DBGRANT_IDLTLN:
                  case OT_STATIC_R_DBGRANT_SESPRY:
                  case OT_STATIC_R_DBGRANT_SESPRN:
                  case OT_STATIC_R_DBGRANT_TBLSTY:
                  case OT_STATIC_R_DBGRANT_TBLSTN:

                  case OT_STATIC_R_DBGRANT_QRYCPY:
                  case OT_STATIC_R_DBGRANT_QRYCPN:
                  case OT_STATIC_R_DBGRANT_QRYPGY:
                  case OT_STATIC_R_DBGRANT_QRYPGN:
                  case OT_STATIC_R_DBGRANT_QRYCOY:
                  case OT_STATIC_R_DBGRANT_QRYCON:
                  case OT_STATIC_R_DBGRANT_CRESEQY:
                  case OT_STATIC_R_DBGRANT_CRESEQN:

                  case OTR_DBGRANT_ACCESY_DB:
                  case OTR_DBGRANT_ACCESN_DB:
                  case OTR_DBGRANT_CREPRY_DB:
                  case OTR_DBGRANT_CREPRN_DB:
                  case OTR_DBGRANT_SEQCRY_DB:
                  case OTR_DBGRANT_SEQCRN_DB:
                  case OTR_DBGRANT_CRETBY_DB:
                  case OTR_DBGRANT_CRETBN_DB:
                  case OTR_DBGRANT_DBADMY_DB:
                  case OTR_DBGRANT_DBADMN_DB:
                  case OTR_DBGRANT_LKMODY_DB:
                  case OTR_DBGRANT_LKMODN_DB:
                  case OTR_DBGRANT_QRYIOY_DB:
                  case OTR_DBGRANT_QRYION_DB:
                  case OTR_DBGRANT_QRYRWY_DB:
                  case OTR_DBGRANT_QRYRWN_DB:
                  case OTR_DBGRANT_UPDSCY_DB:
                  case OTR_DBGRANT_UPDSCN_DB:
                  case OTR_DBGRANT_SELSCY_DB: 
                  case OTR_DBGRANT_SELSCN_DB: 
                  case OTR_DBGRANT_CNCTLY_DB: 
                  case OTR_DBGRANT_CNCTLN_DB: 
                  case OTR_DBGRANT_IDLTLY_DB: 
                  case OTR_DBGRANT_IDLTLN_DB: 
                  case OTR_DBGRANT_SESPRY_DB: 
                  case OTR_DBGRANT_SESPRN_DB: 
                  case OTR_DBGRANT_TBLSTY_DB: 
                  case OTR_DBGRANT_TBLSTN_DB: 

                  case OTR_DBGRANT_QRYCPY_DB: 
                  case OTR_DBGRANT_QRYCPN_DB: 
                  case OTR_DBGRANT_QRYPGY_DB: 
                  case OTR_DBGRANT_QRYPGN_DB: 
                  case OTR_DBGRANT_QRYCOY_DB: 
                  case OTR_DBGRANT_QRYCON_DB: 

                      gtype = DBA_GetObjectType  (parentstrings [0], lpDomData);
                      if (gtype != OT_UNKNOWN)
                      {
                          g.GranteeType = gtype;
                          x_strcpy (g.PreselectGrantee, parentstrings [0]);
                      }
                      break;
                  default:
                      break;
              }

              switch (objType)
              {
                  case OT_STATIC_R_DBGRANT_ACCESY:    
                  case OT_STATIC_DBGRANTEES_ACCESY:
                  case OT_DBGRANT_ACCESY_USER: 
                  case OTR_DBGRANT_ACCESY_DB:
                      g.PreselectPrivileges [GRANT_ACCESS]                = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_ACCESN:    
                  case OT_STATIC_DBGRANTEES_ACCESN:
                  case OT_DBGRANT_ACCESN_USER: 
                  case OTR_DBGRANT_ACCESN_DB:
                      g.PreselectPrivileges [GRANT_NO_ACCESS]             = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_CREPRY:    
                  case OT_STATIC_DBGRANTEES_CREPRY:
                  case OT_DBGRANT_CREPRY_USER: 
                  case OTR_DBGRANT_CREPRY_DB:
                      g.PreselectPrivileges [GRANT_CREATE_PROCEDURE]      = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_CREPRN:    
                  case OT_STATIC_DBGRANTEES_CREPRN:
                  case OT_DBGRANT_CREPRN_USER: 
                  case OTR_DBGRANT_CREPRN_DB:
                      g.PreselectPrivileges [GRANT_NO_CREATE_PROCEDURE]   = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_CRETBY:    
                  case OT_STATIC_DBGRANTEES_CRETBY:
                  case OT_DBGRANT_CRETBY_USER: 
                  case OTR_DBGRANT_CRETBY_DB:
                      g.PreselectPrivileges [GRANT_CREATE_TABLE]          = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_CRETBN:    
                  case OT_STATIC_DBGRANTEES_CRETBN:
                  case OT_DBGRANT_CRETBN_USER: 
                  case OTR_DBGRANT_CRETBN_DB:
                      g.PreselectPrivileges [GRANT_NO_CREATE_TABLE]       = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_DBADMY:    
                  case OT_STATIC_DBGRANTEES_DBADMY:
                  case OT_DBGRANT_DBADMY_USER: 
                  case OTR_DBGRANT_DBADMY_DB:
                      g.PreselectPrivileges [GRANT_DATABASE_ADMIN]        = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_DBADMN:    
                  case OT_STATIC_DBGRANTEES_DBADMN:
                  case OT_DBGRANT_DBADMN_USER: 
                  case OTR_DBGRANT_DBADMN_DB:
                      g.PreselectPrivileges [GRANT_NO_DATABASE_ADMIN]     = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_LKMODY:    
                  case OT_STATIC_DBGRANTEES_LKMODY:
                  case OT_DBGRANT_LKMODY_USER: 
                  case OTR_DBGRANT_LKMODY_DB:
                      g.PreselectPrivileges [GRANT_LOCKMOD]               = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_LKMODN:    
                  case OT_STATIC_DBGRANTEES_LKMODN:
                  case OT_DBGRANT_LKMODN_USER: 
                  case OTR_DBGRANT_LKMODN_DB:
                      g.PreselectPrivileges [GRANT_NO_LOCKMOD]            = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_QRYIOY:    
                  case OT_STATIC_DBGRANTEES_QRYIOY:
                  case OT_DBGRANT_QRYIOY_USER: 
                  case OTR_DBGRANT_QRYIOY_DB:
                      g.PreselectPrivileges [GRANT_QUERY_IO_LIMIT]        = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_QRYION:    
                  case OT_STATIC_DBGRANTEES_QRYION:
                  case OT_DBGRANT_QRYION_USER: 
                  case OTR_DBGRANT_QRYION_DB:
                      g.PreselectPrivileges [GRANT_NO_QUERY_IO_LIMIT]     = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_QRYRWY:    
                  case OT_STATIC_DBGRANTEES_QRYRWY:
                  case OT_DBGRANT_QRYRWY_USER: 
                  case OTR_DBGRANT_QRYRWY_DB:
                      g.PreselectPrivileges [GRANT_QUERY_ROW_LIMIT]       = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_QRYRWN:    
                  case OT_STATIC_DBGRANTEES_QRYRWN:
                  case OT_DBGRANT_QRYRWN_USER: 
                  case OTR_DBGRANT_QRYRWN_DB:
                      g.PreselectPrivileges [GRANT_NO_QUERY_ROW_LIMIT]    = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_UPDSCY:    
                  case OT_STATIC_DBGRANTEES_UPDSCY:
                  case OT_DBGRANT_UPDSCY_USER: 
                  case OTR_DBGRANT_UPDSCY_DB:
                      g.PreselectPrivileges [GRANT_UPDATE_SYSCAT]         = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_UPDSCN:
                  case OT_STATIC_DBGRANTEES_UPDSCN:
                  case OT_DBGRANT_UPDSCN_USER:  
                  case OTR_DBGRANT_UPDSCN_DB:
                      g.PreselectPrivileges [GRANT_NO_UPDATE_SYSCAT]      = TRUE;
                      break;

                  case OT_STATIC_R_DBGRANT_SELSCY:
                  case OT_STATIC_DBGRANTEES_SELSCY:
                  case OT_DBGRANT_SELSCY_USER: 
                  case OTR_DBGRANT_SELSCY_DB: 
                      g.PreselectPrivileges [GRANT_SELECT_SYSCAT]         = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_SELSCN:
                  case OT_STATIC_DBGRANTEES_SELSCN:
                  case OT_DBGRANT_SELSCN_USER: 
                  case OTR_DBGRANT_SELSCN_DB: 
                      g.PreselectPrivileges [GRANT_NO_SELECT_SYSCAT]      = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_CNCTLY:
                  case OT_STATIC_DBGRANTEES_CNCTLY:
                  case OT_DBGRANT_CNCTLY_USER: 
                  case OTR_DBGRANT_CNCTLY_DB: 
                      g.PreselectPrivileges [GRANT_CONNECT_TIME_LIMIT]    = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_CNCTLN:
                  case OT_STATIC_DBGRANTEES_CNCTLN:
                  case OT_DBGRANT_CNCTLN_USER: 
                  case OTR_DBGRANT_CNCTLN_DB: 
                      g.PreselectPrivileges [GRANT_NO_CONNECT_TIME_LIMIT] = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_IDLTLY:
                  case OT_STATIC_DBGRANTEES_IDLTLY:
                  case OT_DBGRANT_IDLTLY_USER: 
                  case OTR_DBGRANT_IDLTLY_DB: 
                      g.PreselectPrivileges [GRANT_IDLE_TIME_LIMIT]       = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_IDLTLN:
                  case OT_STATIC_DBGRANTEES_IDLTLN:
                  case OT_DBGRANT_IDLTLN_USER: 
                  case OTR_DBGRANT_IDLTLN_DB: 
                      g.PreselectPrivileges [GRANT_NO_IDLE_TIME_LIMIT]    = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_SESPRY:
                  case OT_STATIC_DBGRANTEES_SESPRY:
                  case OT_DBGRANT_SESPRY_USER: 
                  case OTR_DBGRANT_SESPRY_DB: 
                      g.PreselectPrivileges [GRANT_SESSION_PRIORITY]      = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_SESPRN:
                  case OT_STATIC_DBGRANTEES_SESPRN:
                  case OT_DBGRANT_SESPRN_USER: 
                  case OTR_DBGRANT_SESPRN_DB: 
                      g.PreselectPrivileges [GRANT_NO_SESSION_PRIORITY]   = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_TBLSTY:
                  case OT_STATIC_DBGRANTEES_TBLSTY:
                  case OT_DBGRANT_TBLSTY_USER: 
                  case OTR_DBGRANT_TBLSTY_DB: 
                      g.PreselectPrivileges [GRANT_TABLE_STATISTICS]      = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_TBLSTN:
                  case OT_STATIC_DBGRANTEES_TBLSTN:
                  case OT_DBGRANT_TBLSTN_USER:
                  case OTR_DBGRANT_TBLSTN_DB: 
                      g.PreselectPrivileges [GRANT_NO_TABLE_STATISTICS]   = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_QRYCPN:
                  case OT_STATIC_DBGRANTEES_QRYCPN:
                  case OTR_DBGRANT_QRYCPN_DB :
                  case OT_DBGRANT_QRYCPN_USER:
                      g.PreselectPrivileges [GRANT_NO_QUERY_CPU_LIMIT]   = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_QRYCPY:
                  case OT_STATIC_DBGRANTEES_QRYCPY:
                  case OTR_DBGRANT_QRYCPY_DB:
                  case OT_DBGRANT_QRYCPY_USER:
                      g.PreselectPrivileges [GRANT_QUERY_CPU_LIMIT]   = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_QRYPGN:
                  case OT_STATIC_DBGRANTEES_QRYPGN:
                  case OTR_DBGRANT_QRYPGN_DB:
                  case OT_DBGRANT_QRYPGN_USER:
                      g.PreselectPrivileges [GRANT_NO_QUERY_PAGE_LIMIT]   = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_QRYPGY:
                  case OT_STATIC_DBGRANTEES_QRYPGY:
                  case OTR_DBGRANT_QRYPGY_DB:
                  case OT_DBGRANT_QRYPGY_USER:
                      g.PreselectPrivileges [GRANT_QUERY_PAGE_LIMIT]   = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_QRYCON:
                  case OT_STATIC_DBGRANTEES_QRYCON:
                  case OTR_DBGRANT_QRYCON_DB:
                  case OT_DBGRANT_QRYCON_USER:
                      g.PreselectPrivileges [GRANT_NO_QUERY_COST_LIMIT]   = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_QRYCOY:
                  case OT_STATIC_DBGRANTEES_QRYCOY:
                  case OTR_DBGRANT_QRYCOY_DB:
                  case OT_DBGRANT_QRYCOY_USER:
                      g.PreselectPrivileges [GRANT_QUERY_COST_LIMIT]   = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_CRESEQN:
                  case OT_DBGRANT_SEQCRN_USER:
                  case OTR_DBGRANT_SEQCRN_DB:
                  case OT_STATIC_DBGRANTEES_CRSEQN:
                      g.PreselectPrivileges [GRANT_NO_CREATE_SEQUENCE]   = TRUE;
                      break;
                  case OT_STATIC_R_DBGRANT_CRESEQY:
                  case OT_DBGRANT_SEQCRY_USER:
                  case OTR_DBGRANT_SEQCRY_DB:
                  case OT_STATIC_DBGRANTEES_CRSEQY:
                      g.PreselectPrivileges [GRANT_CREATE_SEQUENCE]   = TRUE;
                      break;
                  default:
                      break;
              }

              iret = DlgGrantDatabase (hwndMdi, &g);
              if (!iret)
                  return;
              objType = OTLL_DBGRANTEE;
              level   = 0;
           }
       }
       break;


       case OT_STATIC_PROCGRANT_EXEC_USER:
       case OT_PROCGRANT_EXEC_USER:
       {
           GRANTPARAMS g;
           ZEROINIT (g);
           
           g.ObjectType   = OT_PROCEDURE;
           g.Privileges [GRANT_EXECUTE] = TRUE;
           x_strcpy (g.DBName,           parentstrings [0]);     /* Database    */
           x_strcpy (g.PreselectObject,  parentstrings [1]);     /* Procedure   */
           x_strcpy (g.PreselectObjectOwner, ParentTableOwner);  /* Proc owner  */
           iret = DlgGrantProcedure (hwndMdi, &g);             
           if (iret != IDOK)
               return;
       }

       objType = OTLL_GRANTEE;
       level   = 1;
       break;


       case OT_STATIC_SEQGRANT_NEXT_USER:
       case OT_SEQUGRANT_NEXT_USER:
       {
           GRANTPARAMS g;
           ZEROINIT (g);

           g.ObjectType   = OT_SEQUENCE;
           g.Privileges [GRANT_NEXT_SEQUENCE] = TRUE;
           x_strcpy (g.DBName,           parentstrings [0]);     /* Database        */
           x_strcpy (g.PreselectObject,  parentstrings [1]);     /* Sequence        */
           x_strcpy (g.PreselectObjectOwner, ParentTableOwner);  /* Sequence owner  */
           iret = DlgGrantProcedure (hwndMdi, &g);
           if (iret != IDOK)
               return;
       }

       objType = OTLL_GRANTEE;
       level   = 1;
       break;

       /* _____________________________________________________________________
       //                                                                      &
       // Grant on Database event. Preselect : RAISE or REGISTER               &
       //                                                                      &
       // _____________________________________________________________________&
       */
       case OT_STATIC_DBEGRANT_RAISE_USER:
       case OT_STATIC_DBEGRANT_REGTR_USER:
       case OT_DBEGRANT_RAISE_USER:
       case OT_DBEGRANT_REGTR_USER:
       {
           GRANTPARAMS g;
           ZEROINIT (g);

           g.ObjectType   = OT_DBEVENT;
           x_strcpy (g.DBName,           parentstrings [0]);     /* Database    */
           x_strcpy (g.PreselectObject,  parentstrings [1]);     /* DBevent name*/
           x_strcpy (g.PreselectObjectOwner, ParentTableOwner);  /* DBEvent Owner*/
           switch (objType)
           {
               case OT_STATIC_DBEGRANT_RAISE_USER:
               case OT_DBEGRANT_RAISE_USER:
                   g.PreselectPrivileges [GRANT_RAISE]     = TRUE;
                   break;
               case OT_STATIC_DBEGRANT_REGTR_USER:
               case OT_DBEGRANT_REGTR_USER:
                   g.PreselectPrivileges [GRANT_REGISTER]  = TRUE;
                   break;
           }
           iret = DlgGrantDBevent (hwndMdi, &g);
           if (!iret)
               return;
       }
       objType = OTLL_GRANTEE;
       level   = 1;
       break;
   
       
       /* _____________________________________________________________________
       //                                                                      &
       // Grant on Table or View (Reference) Preselect : USER; GROUP or ROLE   &
       //                                              : and privileges        &
       // _____________________________________________________________________&
       */
       case OT_STATIC_R_TABLEGRANT    :
       case OT_STATIC_R_TABLEGRANT_SEL:
       case OT_STATIC_R_TABLEGRANT_INS:
       case OT_STATIC_R_TABLEGRANT_UPD:
       case OT_STATIC_R_TABLEGRANT_DEL:
       case OT_STATIC_R_TABLEGRANT_CPI: 
       case OT_STATIC_R_TABLEGRANT_CPF: 
       case OT_STATIC_R_TABLEGRANT_REF:
       case OT_STATIC_R_TABLEGRANT_ALL:
       case OTR_GRANTEE_SEL_TABLE:
       case OTR_GRANTEE_INS_TABLE:
       case OTR_GRANTEE_UPD_TABLE:
       case OTR_GRANTEE_DEL_TABLE:
       case OTR_GRANTEE_CPI_TABLE:
       case OTR_GRANTEE_CPF_TABLE:
       case OTR_GRANTEE_REF_TABLE:
       case OTR_GRANTEE_ALL_TABLE:

       case OT_STATIC_R_VIEWGRANT     :
       case OT_STATIC_R_VIEWGRANT_SEL :
       case OT_STATIC_R_VIEWGRANT_INS :
       case OT_STATIC_R_VIEWGRANT_UPD :
       case OT_STATIC_R_VIEWGRANT_DEL :
       case OTR_GRANTEE_SEL_VIEW:
       case OTR_GRANTEE_INS_VIEW:
       case OTR_GRANTEE_UPD_VIEW:
       case OTR_GRANTEE_DEL_VIEW:
       {
           int gtype;
           GRANTPARAMS g;
           ZEROINIT (g);

           switch (objType)
           {
               case OT_STATIC_R_TABLEGRANT:
                   g.ObjectType = OT_TABLE;
                   break;
               case OT_STATIC_R_TABLEGRANT_SEL:
               case OTR_GRANTEE_SEL_TABLE:
                   g.PreselectPrivileges [GRANT_SELECT]    = TRUE;
                   g.ObjectType = OT_TABLE;
                   break;
               case OT_STATIC_R_TABLEGRANT_INS:
               case OTR_GRANTEE_INS_TABLE:
                   g.PreselectPrivileges [GRANT_INSERT]    = TRUE;
                   g.ObjectType = OT_TABLE;
                   break;
               case OT_STATIC_R_TABLEGRANT_UPD:
               case OTR_GRANTEE_UPD_TABLE:
                   g.PreselectPrivileges [GRANT_UPDATE]    = TRUE;
                   g.ObjectType = OT_TABLE;
                   break;
               case OT_STATIC_R_TABLEGRANT_DEL:
               case OTR_GRANTEE_DEL_TABLE:
                   g.PreselectPrivileges [GRANT_DELETE]    = TRUE;
                   g.ObjectType = OT_TABLE;
                   break;
               case OT_STATIC_R_TABLEGRANT_CPI:
               case OTR_GRANTEE_CPI_TABLE:
                   g.PreselectPrivileges [GRANT_COPY_INTO] = TRUE;
                   g.ObjectType = OT_TABLE;
                   break;
               case OT_STATIC_R_TABLEGRANT_CPF: 
               case OTR_GRANTEE_CPF_TABLE:
                   g.PreselectPrivileges [GRANT_COPY_FROM] = TRUE;
                   g.ObjectType = OT_TABLE;
                   break;
               case OT_STATIC_R_TABLEGRANT_REF:
               case OTR_GRANTEE_REF_TABLE:
                   g.PreselectPrivileges [GRANT_REFERENCE] = TRUE;
                   g.ObjectType = OT_TABLE;
                   break;
               case OT_STATIC_R_TABLEGRANT_ALL:
               case OTR_GRANTEE_ALL_TABLE:
                   g.PreselectPrivileges [GRANT_SELECT]    = TRUE;
                   g.PreselectPrivileges [GRANT_INSERT]    = TRUE;
                   g.PreselectPrivileges [GRANT_UPDATE]    = TRUE;
                   g.PreselectPrivileges [GRANT_DELETE]    = TRUE;
                   g.PreselectPrivileges [GRANT_REFERENCE] = TRUE;
                   g.PreselectPrivileges [GRANT_COPY_INTO] = TRUE;
                   g.PreselectPrivileges [GRANT_COPY_FROM] = TRUE;
                   g.ObjectType = OT_TABLE;
                   break;

               case OT_STATIC_R_VIEWGRANT:
                   g.ObjectType = OT_VIEW;
                   break;
               case OT_STATIC_R_VIEWGRANT_SEL :
               case OTR_GRANTEE_SEL_VIEW:
                   g.PreselectPrivileges [GRANT_SELECT]    = TRUE;
                   g.ObjectType = OT_VIEW;
                   break;
               case OT_STATIC_R_VIEWGRANT_INS :
               case OTR_GRANTEE_INS_VIEW:
                   g.PreselectPrivileges [GRANT_INSERT]    = TRUE;
                   g.ObjectType = OT_VIEW;
                   break;
               case OT_STATIC_R_VIEWGRANT_UPD :
               case OTR_GRANTEE_UPD_VIEW:
                   g.PreselectPrivileges [GRANT_UPDATE]    = TRUE;
                   g.ObjectType = OT_VIEW;
                   break;
               case OT_STATIC_R_VIEWGRANT_DEL :
               case OTR_GRANTEE_DEL_VIEW:
                   g.PreselectPrivileges [GRANT_DELETE]    = TRUE;
                   g.ObjectType = OT_VIEW;
                   break;
               default:
                   break;
           }
           gtype = DBA_GetObjectType (parentstrings [0], lpDomData);
           if (gtype != OT_UNKNOWN)
               g.GranteeType = gtype;
           x_strcpy (g.PreselectGrantee, parentstrings [0]);  /* User, Group, Role */

           iret = DlgGnrefTable (hwndMdi, &g);
           fstrncpy(parentstrings[0],g.DBName, MAXOBJECTNAME);
           if (!iret)
               return;
       }
       objType = OTLL_GRANTEE;
       level   = 1;
       break;

       case OT_STATIC_R_PROCGRANT_EXEC:
       case OT_STATIC_R_PROCGRANT:     
       case OTR_GRANTEE_EXEC_PROC:
       {
           int gtype;
           GRANTPARAMS g;
           ZEROINIT (g);
           
           g.ObjectType = OT_PROCEDURE;
           gtype = DBA_GetObjectType (parentstrings [0], lpDomData);

           if (gtype != OT_UNKNOWN)
               g.GranteeType = gtype;
           x_strcpy (g.PreselectGrantee, parentstrings [0]);  /* User, Group, Role */

           g.Privileges [GRANT_EXECUTE] = TRUE;
           iret = DlgGnrefProcedure (hwndMdi, &g);
           fstrncpy(parentstrings[0],g.DBName, MAXOBJECTNAME);
           if (!iret)
               return;
       }
       objType = OTLL_GRANTEE;
       level   = 1;
       break;

       case OT_STATIC_R_SEQGRANT_NEXT:
       case OT_STATIC_R_SEQGRANT:
       case OTR_GRANTEE_NEXT_SEQU:
       {
           int gtype;
           GRANTPARAMS g;
           ZEROINIT (g);
           
           g.ObjectType = OT_SEQUENCE;
           gtype = DBA_GetObjectType (parentstrings [0], lpDomData);

           if (gtype != OT_UNKNOWN)
               g.GranteeType = gtype;
           x_strcpy (g.PreselectGrantee, parentstrings [0]);  /* User, Group, Role */

           iret = DlgGnrefProcedure (hwndMdi, &g);
           fstrncpy(parentstrings[0],g.DBName, MAXOBJECTNAME);
           if (!iret)
               return;
       }
       objType = OTLL_GRANTEE;
       level   = 1;
       break;
       
       case OT_STATIC_R_DBEGRANT:          
       case OT_STATIC_R_DBEGRANT_RAISE:    
       case OT_STATIC_R_DBEGRANT_REGISTER:
       case OTR_GRANTEE_RAISE_DBEVENT:
       case OTR_GRANTEE_REGTR_DBEVENT:
       {
           int gtype;
           GRANTPARAMS g;
           ZEROINIT (g);

           g.ObjectType    = OT_DBEVENT;
           gtype           = DBA_GetObjectType (parentstrings [0], lpDomData);
           if (gtype != OT_UNKNOWN)
               g.GranteeType = gtype;
           x_strcpy (g.PreselectGrantee, parentstrings [0]);  // User, Group, Role 
           switch (objType)
           {
               case OT_STATIC_R_DBEGRANT_RAISE:
               case OTR_GRANTEE_RAISE_DBEVENT:
                   g.PreselectPrivileges [GRANT_RAISE]     = TRUE;
                   break;
               case OT_STATIC_R_DBEGRANT_REGISTER:
               case OTR_GRANTEE_REGTR_DBEVENT:
                   g.PreselectPrivileges [GRANT_REGISTER]  = TRUE;
                   break;

               default:
                   break;
           }
           iret = DlgGnrefDBevent (hwndMdi, &g);
           fstrncpy(parentstrings[0],g.DBName, MAXOBJECTNAME);
           if (!iret)
               return;
       }
       objType = OTLL_GRANTEE;
       level   = 1;
       break;

/*
//       case OT_STATIC_R_DBGRANT:       
//       {
//           int gtype;
//           GRANTPARAMS g;
//           ZEROINIT (g);
//
//           g.ObjectType    = OT_DATABASE;
//           gtype           = DBA_GetObjectType (parentstrings [0], lpDomData);
//           if (gtype != OT_UNKNOWN)
//               g.GranteeType = gtype;
//           x_strcpy (g.PreselectGrantee, parentstrings [0]);  // User, Group, Role 
//
//           iret = DlgGrantDatabase (hwndMdi, &g);
//           if (!iret)
//               return;
//       }
//       objType = OTLL_GRANTEE;
//       level   = 1;
//       break;
*/       


       case OT_STATIC_DATABASE:
       case OT_DATABASE:
         {
           CREATEDB db;
           ZEROINIT (db);

           //iret = DlgCreateDB (hwndMdi,&db );
           iret = MfcDlgCreateDatabase( &db );
           if (!iret)
               return;
         }
       objType = OT_DATABASE;
       level   = 0;
       break;

       case OT_STATIC_REPLIC_CONNECTION :    
       case OT_REPLIC_CONNECTION:
       {
            int                iret;
           REPLCONNECTPARAMS   addrepl;
//           VNODESEL            xvnode;

           ZEROINIT (addrepl);
//           ZEROINIT (xvnode);

           addrepl.bLocalDb = FALSE;
#ifndef MAINLIB
           vnoderet    = get_vnode_data    (&xvnode);
           DoneWithNodes();
#else
//           vnoderet    = OK;
#endif
           if (TRUE /*vnoderet == OK */)  {
           int restype;
           UCHAR buf[MAXOBJECTNAME];

//           addrepl.vnodelist = xvnode.vnodelist;
           // Get Database owner: replicable tables should have the same
           // owner for this version of replicator
           if ( DOMGetObjectLimitedInfo(lpDomData->psDomNode->nodeHandle,
                                        parentstrings [0],
                                        "",
                                        OT_DATABASE,
                                        0,
                                        parentstrings,
                                        TRUE,
                                        &restype,
                                        buf,
                                        addrepl.szOwner,
                                        buf
                                        )
                                          !=RES_SUCCESS )
                return;

           addrepl.nReplicVersion=GetReplicInstallStatus(
                                            lpDomData->psDomNode->nodeHandle,
                                            parentstrings [0],
                                            addrepl.szOwner);
           x_strcpy (addrepl.DBName, parentstrings [0]);     // Database name
           iret = DlgAddReplConnection	(hwndMdi, &addrepl);
#ifndef MAINLIB
           xvnode.vnodelist = DeleteVnode (xvnode.vnodelist);
#endif
           if (!iret)
              return;
           }
           else
           {
#ifndef MAINLIB
               VnodeError (vnoderet);
#endif
               return;
           }
       }
       objType = OT_REPLIC_CONNECTION;
       level   = 1;
       break;

       


       case OT_STATIC_REPLIC_MAILUSER:
       case OT_REPLIC_MAILUSER:
       {
          REPLMAILPARAMS mail;
          ZEROINIT (mail);

          x_strcpy (mail.DBName, parentstrings [0]);     /* Database    */
          iret = DlgMail (hwndMdi, &mail);

          if (!iret)
              return;
           
       }
       objType = OT_REPLIC_MAILUSER;
       level   = 1;
       break;


       case OT_STATIC_ROLEGRANT_USER  :
       case OT_ROLEGRANT_USER:
       {
           GRANTPARAMS g;
           ZEROINIT   (g);

           x_strcpy (g.PreselectObject, parentstrings [0]);  /* Preselect role */
           iret = DlgGrantRole2User (hwndMdi, &g);
           if (!iret)
               return;
       }
       parentstrings [0] = NULL;  /* more the one role may be granted */
       objType = OT_ROLEGRANT_USER;
       level   = 1;
       break;

       case OTR_GRANTEE_ROLE:
       case OT_STATIC_R_ROLEGRANT:
       {
           GRANTPARAMS g;
           ZEROINIT   (g);

           x_strcpy (g.PreselectGrantee, parentstrings [0]);  /* Preselect user */
           iret = DlgGrantRole2User (hwndMdi, &g);
           if (!iret)
               return;
       }
       parentstrings [0] = NULL;  /* more the one role may be granted */
       objType = OT_ROLEGRANT_USER;
       level   = 1;
       break;
       case OT_ICE_ROLE:
       case OT_STATIC_ICE_ROLE:
       {
           ICEROLEDATA IceRdta;
           ZEROINIT   (IceRdta);
           iret = MfcDlgCreateIceRole( &IceRdta ,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       parentstrings [0] = NULL;
       objType = OT_ICE_ROLE;
       level   = 0;
       break;
       case OT_ICE_DBUSER:
       case OT_STATIC_ICE_DBUSER:
       {
           ICEDBUSERDATA IceDbUserDta;
           ZEROINIT   (IceDbUserDta);
           iret = MfcDlgCreateIceDbUser(&IceDbUserDta ,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       parentstrings [0] = NULL;
       objType = OT_ICE_DBUSER;
       level   = 0;
       break;
       case OT_ICE_DBCONNECTION:
       case OT_STATIC_ICE_DBCONNECTION:
       {
           DBCONNECTIONDATA IceDbConnect;
           ZEROINIT   (IceDbConnect);
           iret = MfcDlgCreateIceDbConnection(&IceDbConnect,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       parentstrings [0] = NULL;
       objType = OT_ICE_DBCONNECTION;
       level   = 0;
       break;
       case OT_ICE_WEBUSER:
       case OT_STATIC_ICE_WEBUSER:
       {
           ICEWEBUSERDATA IceWebUsr;
           ZEROINIT   (IceWebUsr);
           iret = MfcDlgCreateIceWebUser(&IceWebUsr,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       parentstrings [0] = NULL;
       objType = OT_ICE_WEBUSER;
       level   = 0;
       break;
       case OT_ICE_PROFILE:
       case OT_STATIC_ICE_PROFILE:
       {
           ICEPROFILEDATA IceProfile;
           ZEROINIT   (IceProfile);
           iret = MfcDlgCreateIceProfile(&IceProfile,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       parentstrings [0] = NULL;
       objType = OT_ICE_PROFILE;
       level   = 0;
       break;
       case OT_ICE_BUNIT_FACET:
       case OT_STATIC_ICE_BUNIT_FACET:
       {
           ICEBUSUNITDOCDATA Idf;
           ZEROINIT   (Idf);
           Idf.bFacet = TRUE;
           lstrcpy(Idf.icebusunit.Name,parentstrings [0]);
           iret = MfcDlgCreateIceFacetPage(&Idf,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_BUNIT_FACET;
       level   = 1;
       break;
       case OT_ICE_BUNIT_PAGE:
       case OT_STATIC_ICE_BUNIT_PAGE:
       {
           ICEBUSUNITDOCDATA Idp;
           ZEROINIT   (Idp);
           Idp.bFacet = FALSE;
           lstrcpy(Idp.icebusunit.Name,parentstrings [0]);
           iret = MfcDlgCreateIceFacetPage(&Idp,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_BUNIT_PAGE;
       level   = 1;
       break;
       case OT_ICE_BUNIT_SEC_ROLE:
       case OT_STATIC_ICE_BUNIT_SEC_ROLE:
       {
           ICEBUSUNITROLEDATA Ibr;
           ZEROINIT   (Ibr);
           lstrcpy(Ibr.icebusunit.Name,parentstrings [0]);
           iret = MfcDlgCreateIceBusinessRole(&Ibr,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_BUNIT_SEC_ROLE ;
       level   = 1;
       break;
       case OT_ICE_BUNIT_SEC_USER:
       case OT_STATIC_ICE_BUNIT_SEC_USER:
       {
           ICEBUSUNITWEBUSERDATA Ibu;
           ZEROINIT   (Ibu);
           lstrcpy(Ibu.icebusunit.Name,parentstrings [0]);
           iret = MfcDlgCreateIceBusinessUser(&Ibu,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_BUNIT_SEC_USER;
       level   = 1;
       break;
       case OT_ICE_WEBUSER_ROLE:
       case OT_STATIC_ICE_WEBUSER_ROLE:
       {
           iret = MfcDlgCommonCombobox(OT_ICE_WEBUSER_ROLE,
                                       parentstrings [0],
                                       lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_WEBUSER_ROLE;
       level   = 1;
       break;
       case OT_ICE_WEBUSER_CONNECTION:
       case OT_STATIC_ICE_WEBUSER_CONNECTION:
       {
           iret = MfcDlgCommonCombobox(OT_ICE_WEBUSER_CONNECTION,
                                       parentstrings [0],
                                       lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_WEBUSER_CONNECTION;
       level   = 1;
       break;
       case OT_ICE_PROFILE_CONNECTION:
       case OT_STATIC_ICE_PROFILE_CONNECTION:
       {
           iret = MfcDlgCommonCombobox(OT_ICE_PROFILE_CONNECTION,
                                       parentstrings [0],
                                       lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_PROFILE_CONNECTION;
       level   = 1;
       break;
       case OT_ICE_PROFILE_ROLE:
       case OT_STATIC_ICE_PROFILE_ROLE:
       {
           iret = MfcDlgCommonCombobox(OT_ICE_PROFILE_ROLE,
                                       parentstrings [0],
                                       lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_PROFILE_ROLE;
       level   = 1;
       break;
       case OT_ICE_BUNIT_LOCATION:
       case OT_STATIC_ICE_BUNIT_LOCATION:
       {
           iret = MfcDlgCommonCombobox(OT_ICE_BUNIT_LOCATION,
                                       parentstrings [0],
                                       lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_BUNIT_LOCATION;
       level   = 1;
       break;
       case OT_ICE_BUNIT:
       case OT_STATIC_ICE_BUNIT:
       {
           iret = MfcDlgCommonEdit( OT_ICE_BUNIT , lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_BUNIT;
       level   = 1;
       break;
       case OT_ICE_SERVER_APPLICATION:
       case OT_STATIC_ICE_SERVER_APPLICATION:
       {
           iret = MfcDlgCommonEdit( OT_ICE_SERVER_APPLICATION , lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       parentstrings [0] = NULL;
       objType = OT_ICE_SERVER_APPLICATION;
       level   = 0;
       break;
       case OT_ICE_SERVER_LOCATION:
       case OT_STATIC_ICE_SERVER_LOCATION:
       {
           ICELOCATIONDATA IceLoc;
           memset(&IceLoc,0,sizeof(ICELOCATIONDATA));
           iret = MfcDlgIceServerLocation( &IceLoc ,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       parentstrings [0] = NULL;
       objType = OT_ICE_SERVER_LOCATION;
       level   = 0;
       break;
       case OT_ICE_SERVER_VARIABLE:
       case OT_STATIC_ICE_SERVER_VARIABLE:
       {
           ICESERVERVARDATA IceVar;
           memset(&IceVar,0,sizeof(ICESERVERVARDATA));
           iret = MfcDlgIceServerVariable( &IceVar ,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       parentstrings [0] = NULL;
       objType = OT_ICE_SERVER_VARIABLE;
       level   = 0;
       break;
       case OT_ICE_BUNIT_FACET_ROLE:
       case OT_STATIC_ICE_BUNIT_FACET_ROLE:
       case OT_ICE_BUNIT_PAGE_ROLE:
       case OT_STATIC_ICE_BUNIT_PAGE_ROLE:
       {
           ICEBUSUNITDOCROLEDATA IceFrole;

           // objType transformed into non-static type
           if ( objType == OT_STATIC_ICE_BUNIT_FACET_ROLE)
               objType = OT_ICE_BUNIT_FACET_ROLE;
           if ( objType == OT_STATIC_ICE_BUNIT_PAGE_ROLE )
               objType = OT_ICE_BUNIT_PAGE_ROLE;

           memset(&IceFrole,0,sizeof(ICEBUSUNITDOCROLEDATA));
           lstrcpy(IceFrole.icebusunitdoc.icebusunit.Name,parentstrings [0]);
           lstrcpy(IceFrole.icebusunitdoc.name,parentstrings [1]);
           if (objType == OT_ICE_BUNIT_FACET_ROLE)
             IceFrole.icebusunitdoc.bFacet = TRUE;
           else
             IceFrole.icebusunitdoc.bFacet = FALSE;
           iret = MfcDlgBuDefRoleUserForFacetPage( objType,
                                                   &IceFrole,
                                                   lpDomData->psDomNode->nodeHandle );
           if (!iret)
               return;
       }
       level   = 2;
       break;

       case OT_ICE_BUNIT_FACET_USER:
       case OT_STATIC_ICE_BUNIT_FACET_USER:
       case OT_ICE_BUNIT_PAGE_USER:
       case OT_STATIC_ICE_BUNIT_PAGE_USER:
       {
           ICEBUSUNITDOCUSERDATA IceRuser;

           // objType transformed into non-static type
           if ( objType == OT_STATIC_ICE_BUNIT_FACET_USER)
               objType = OT_ICE_BUNIT_FACET_USER;
           if ( objType == OT_STATIC_ICE_BUNIT_PAGE_USER )
               objType = OT_ICE_BUNIT_PAGE_USER;

           memset(&IceRuser,0,sizeof(ICEBUSUNITDOCUSERDATA));
           lstrcpy(IceRuser.icebusunitdoc.icebusunit.Name,parentstrings [0]);
           lstrcpy(IceRuser.icebusunitdoc.name,parentstrings [1]);
           if (objType == OT_ICE_BUNIT_FACET_USER)
             IceRuser.icebusunitdoc.bFacet = TRUE;
           else
             IceRuser.icebusunitdoc.bFacet = FALSE;
           iret = MfcDlgBuDefRoleUserForFacetPage( objType,
                                                   &IceRuser,
                                                   lpDomData->psDomNode->nodeHandle );
           if (!iret)
               return;
       }
       level   = 2;
       break;

       default:
       {
           char message [100];
           wsprintf (
               message,
               ResourceString ((UINT)IDS_I_CREATE_NOTDEFINED), // Create not implemented yet for this type of object: id = %d
               objType);
           MessageBox (GetFocus(), message, NULL,
                       MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
           return;
       }
   }

   //
   // Step 2: calls to UpdateDOMData and DomUpdateDisplayData
   // to refresh internal data and display,
   // since we have added an object
   //

   DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, NULL);

   switch (objType)
   {
       case OT_STATIC_SECURITY     :
           iter = 1;
           aObjType[0] = OT_S_ALARM_SELSUCCESS_USER;
           parentstrings[1]=NULL; /* can be on more than one table */
           objType=OTLL_SECURITYALARM;
           aObjType[0] = OTLL_SECURITYALARM;
           alevel[0]=1;
           break;
       case OT_GROUP:
       case OT_ROLE:
           iter = 2;
           aObjType[0] = objType;
           aObjType[1] = OTLL_DBGRANTEE;
           alevel[0]=0;
           alevel[1]=0;
           break;
       case OT_USER:
           iter = 2;
           aObjType[0] = objType;
           aObjType[1] = OTLL_DBGRANTEE;
           alevel[0]=level;
           alevel[1]=0;
           if (AttachedGroup[0]) {
             iter=3;
             aObjType[2] = OT_GROUPUSER;
             alevel[2]   = 1;
             parentstrings[0]=AttachedGroup;
           }
           break;

       case OT_REPLIC_CDDS:
           // add of a cdds: update registered tables
           iter = 5;
           parentstrings[1]=NULL; // for all CDDS (level=2)
           aObjType[0] = objType;
           aObjType[1] = OT_REPLIC_REGTABLE;
           aObjType[2] = OT_REPLIC_CDDS_DETAIL;
           aObjType[3] = OT_TABLE;
           aObjType[4] = OT_PROCEDURE;
           alevel[0] = 1;
           alevel[1] = 1;
           alevel[2] = 2;
           alevel[3] = 1;
           alevel[4] = 1;
           // parentstrings [0] already set
           break;

       case OT_ICE_BUNIT_PAGE_USER:
       case OT_ICE_BUNIT_FACET_USER:
           iter = 2;
           aObjType[0] = objType;
           aObjType[1] = OT_ICE_BUNIT_SEC_USER;
           alevel[0]=level;
           alevel[1]=1;
           break;

       default:
           iter = 1;
           aObjType[0] = objType;
           alevel[0]=level;
           break;
   }

   // low level refresh
   for (cpt=0; cpt<iter; cpt++)
       UpdateDOMDataAllUsers(
            lpDomData->psDomNode->nodeHandle, // hnodestruct
            aObjType[cpt],                    // iobjecttype
            alevel[cpt],                      // level
            parentstrings,                    // parentstrings
            FALSE,                            // keep system tbls state
            FALSE);                           // bWithChildren

   // display refresh
   for (cpt=0; cpt<iter; cpt++) {
       DOMUpdateDisplayDataAllUsers (
               lpDomData->psDomNode->nodeHandle, // hnodestruct
               aObjType[cpt],                    // iobjecttype
                alevel[cpt],                      // level
                parentstrings,                    // parentstrings
                FALSE,                            // bWithChildren
                ACT_ADD_OBJECT,                   // object added
                0L,                               // no item id
                0);                               // all mdi windows concerned
       if (aObjType[cpt]==OT_DBEVENT)
         UpdateDBEDisplay(lpDomData->psDomNode->nodeHandle, parentstrings[0]);
   }

   DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, NULL);

   // For star : potential refresh of tables list on local db / local node
   if (bMustRefreshStarLocalDB) {
     starNodeHandle = GetVirtNodeHandle(szStarLocalNodeName);
     if (starNodeHandle >= 0) {
       tempParentstrings0 = parentstrings[0];
       parentstrings[0] = szStarLocalDbName;
       UpdateDOMDataDetailAllUsers(starNodeHandle,
                           OT_TABLE,
                           1,
                           parentstrings,
                           FALSE,
                           FALSE,
                           TRUE,      // bOnlyIfExists
                           FALSE,
                           FALSE);
       DOMUpdateDisplayDataAllUsers (starNodeHandle,
                             OT_TABLE,
                             1,
                             parentstrings,
                             FALSE,
                             ACT_ADD_OBJECT,
                             0L,
                             0);
       parentstrings[0] = tempParentstrings0;
     }
   }

   // complimentary management of right pane: refresh it immediately
   UpdateRightPane(hwndMdi, lpDomData, TRUE, 0);

   // Fix Nov 4, 97: must Update number of objects after an add
   // (necessary since we have optimized the function that counts the tree items
   // for performance purposes)
   GetNumberOfObjects(lpDomData, TRUE);
   // display will be updated at next call to the same function

   // warn if created object not visible because of filters
   //_T("Object not displayed due to current filters state")
   //_T("Add Object")
   if (NotVisibleDueToFilters(lpDomData, objType))
     MessageBox (GetFocus(),
                 ResourceString(IDS_ERR_FILTERS_STATE),
                 ResourceString(IDS_TITLE_ADD_OBJECT),
                 MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
}


//
// VOID DomAlterObject(HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam)
//
// Alters an object according to type of the current selection in the lbtree
//
// At this time, hwndMdi, wParam and lParam not used
//
VOID DomAlterObject(HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam)
{
   int       SesHndl;          // current session handle
   int       objType;          // current selection type
   UCHAR     objName[MAXOBJECTNAME]; // current selection object name
   UCHAR     objOwner[MAXOBJECTNAME];
   int       i,iret;             // return value of the dialog box
   int       level;
   LPUCHAR   parentstrings [MAXPLEVEL];
   DWORD     dwCurSel;
   UCHAR     Parents[MAXPLEVEL][MAXOBJECTNAME];
   LPTREERECORD lpRecord;
   int cpt, iter;
   int aObjType[10];   // CAREFUL WITH THIS MAX VALUE!
   int alevel  [10];
   LPUCHAR   lpVirtNode;

   // dummy instructions to skip compiler warnings at level four
   wParam;
   lParam;

   // check we are not on a static position
   if (IsCurSelObjStatic(lpDomData))
   return;

   if (!GetCurSelObjParents (lpDomData, parentstrings))
       return;
   for (i=0;i<3;i++) {
       fstrncpy(Parents[i],parentstrings[i],MAXOBJECTNAME);
       parentstrings[i]=Parents[i];
   }

   objType = GetCurSelUnsolvedItemObjType (lpDomData);
   dwCurSel=(DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
   if (!dwCurSel)
       return;   // How did we happen here ?
   fstrncpy(objName,GetObjName(lpDomData, dwCurSel),MAXOBJECTNAME);

   lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                               LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

   fstrncpy(objOwner,lpRecord->ownerName,MAXOBJECTNAME);
   if (!CanObjectBeAltered(objType,objName,lpRecord->ownerName))
   {
       //
       // Alter is not allowed for this object
       //
       MessageBox (GetFocus(),
                   ResourceString ((UINT)IDS_E_ALTER_OBJECT_DENIED), NULL,
                   MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
       return;
   }

   lpVirtNode    = GetVirtNodeName(lpDomData->psDomNode->nodeHandle);

   switch (objType)
   {

       case OT_DATABASE:
         {
             if (GetOIVers() != OIVERS_NOTOI)  {
               int err;
               CREATEDB db;
               DATABASEPARAMS dtaparam;
               ZEROINIT (db);
               ZEROINIT (dtaparam);

               lstrcpy (dtaparam.objectname, objName);
               dtaparam.DbType = lpRecord->parentDbType;
               err = GetDetailInfo (lpVirtNode, OT_DATABASE, &dtaparam, FALSE,
                                    &SesHndl);
               if (err != RES_SUCCESS)
                   return;
               lstrcpy(db.LocDatabase,dtaparam.szDbDev);
               lstrcpy(db.LocCheckpoint,dtaparam.szChkpDev);
               lstrcpy(db.LocJournal,dtaparam.szJrnlDev);
               lstrcpy(db.LocDump,dtaparam.szDmpDev);
               lstrcpy(db.LocWork,dtaparam.szSortDev);
               db.bAlter = TRUE;
               db.bPrivate = dtaparam.bPrivate;
               lstrcpy (db.szDBName, objName);
               lstrcpy (db.szUserName, dtaparam.szOwner);
               lstrcpy (db.szCatOIToolsAvailables, dtaparam.szCatalogsOIToolsAvailables);
               if (x_strstr(db.szCatOIToolsAvailables,"Y") != NULL)
                   db.bGenerateFrontEnd = TRUE;
               db.lpLocExt = dtaparam.lpDBExtensions;
               db.bReadOnly = dtaparam.bReadOnly;
               db.bUnicodeDatabaseNFD = dtaparam.bUnicodeDBNFD;
               db.bUnicodeDatabaseNFC = dtaparam.bUnicodeDBNFC;
               lstrcpy (db.szCatalogsPageSize, dtaparam.szCatalogsPageSize);
               iret = MfcDlgCreateDatabase( &db );
               FreeAttachedPointers(&dtaparam,OT_DATABASE);
               if (!iret)
                   return;
             }
         }
       objType = OT_DATABASE;
       level   = 0;
       break;

   // PS begin
       case OT_REPLIC_CONNECTION:
       {
         int                iret;
         REPLCONNECTPARAMS  addrepl;
//         VNODESEL           xvnode;

         ZEROINIT (addrepl);
//         ZEROINIT (xvnode);

#ifndef MAINLIB
         vnoderet    = get_vnode_data    (&xvnode);
         DoneWithNodes();
#else
         //vnoderet    = OK;
#endif

         if (TRUE /*vnoderet == OK */)
         {
//             addrepl.vnodelist = xvnode.vnodelist;
           {   // Get Database owner: replicable tables should have the same
               // owner for this version of replicator
               int restype;
               UCHAR buf[MAXOBJECTNAME];
               if ( DOMGetObjectLimitedInfo(lpDomData->psDomNode->nodeHandle,
                                            parentstrings [0],
                                            "",
                                            OT_DATABASE,
                                            0,
                                            parentstrings,
                                            TRUE,
                                            &restype,
                                            buf,
                                            addrepl.szOwner,
                                            buf
                                           )
                                             !=RES_SUCCESS
                  )
                   return;
           }

             x_strcpy (addrepl.DBName  , parentstrings [0]);  // Database name
             addrepl.nNumber = lpRecord->complimValue;      // Database Number

             // Get replicator version
             addrepl.nReplicVersion = GetReplicInstallStatus( lpDomData->psDomNode->nodeHandle,
                                                       parentstrings [0],
                                                       addrepl.szOwner);

             iret = GetDetailInfo (lpVirtNode,
                                   GetRepObjectType4ll(OT_REPLIC_CONNECTION,addrepl.nReplicVersion),
                                   &addrepl, FALSE, &SesHndl);
           
             if (iret != RES_SUCCESS )
                return;

             // fill values for alter connection
             addrepl.bAlter     = TRUE;

             iret = DlgAddReplConnection	(hwndMdi, &addrepl);
#ifndef MAINLIB
             xvnode.vnodelist = DeleteVnode (xvnode.vnodelist);
#endif
             if (!iret)
                 return;
         }
         else
         {
#ifndef MAINLIB
             VnodeError (vnoderet);
#endif
             return;
         }

       }
       objType = OT_REPLIC_CONNECTION;
       level   = 1;
       break;
   // PS end
   // JFS Begin
       case OT_REPLIC_CDDS:
       case OT_REPLIC_CDDS_V11:
       case OT_STATIC_REPLIC_CDDS:
           {
           REPCDDS cdds;
           int SesHndl,nReplicVersion;
           ZEROINIT (cdds);

           x_strcpy (cdds.DBName, parentstrings [0]);     /* Database    */
           cdds.cdds = my_atoi(objName);

           {   // Get Database owner: replicable tables should have the samoe
               // owner for this version of replicator
               int restype;
               UCHAR buf[MAXOBJECTNAME];
               if ( DOMGetObjectLimitedInfo(lpDomData->psDomNode->nodeHandle,
                                            parentstrings [0],
                                            "",
                                            OT_DATABASE,
                                            0,
                                            parentstrings,
                                            TRUE,
                                            &restype,
                                            buf,
                                            cdds.DBOwnerName,
                                            buf
                                           )
                                             !=RES_SUCCESS
                  )
                   return;
           }

           cdds.bAlter = TRUE;
           nReplicVersion=GetReplicInstallStatus( lpDomData->psDomNode->nodeHandle,
                                                  parentstrings [0],
                                                  cdds.DBOwnerName);

           iret = GetDetailInfo (GetVirtNodeName(lpDomData->psDomNode->nodeHandle),
                                 GetRepObjectType4ll(OT_REPLIC_CDDS,nReplicVersion),
                                 &cdds, FALSE, &SesHndl);
           cdds.iReplicType=nReplicVersion;
           if (iret != RES_SUCCESS || nReplicVersion == REPLIC_NOINSTALL)
               return; 

           if (nReplicVersion == REPLIC_V10 ||
               nReplicVersion == REPLIC_V105  )
              iret = DlgCDDS (hwndMdi, &cdds);
           else 
              iret = DlgCDDSv11 (hwndMdi, &cdds);

           if (!iret)
               return;

           objType = OT_REPLIC_CDDS;
           level   = 1;
           }
       break;

   // JFS End

       case OT_LOCATION:
       {
           LOCATIONPARAMS locparams;
           int err;
           ZEROINIT (locparams);

           x_strcpy (locparams.objectname, objName);
           err = GetDetailInfo (lpVirtNode, OT_LOCATION, &locparams, FALSE,
                                &SesHndl);
           if (err != RES_SUCCESS)
               return;
           locparams.bCreate = FALSE;
           iret = MfcDlgCreateLocation( &locparams );
           FreeAttachedPointers (&locparams,  OT_LOCATION);
           if (!iret)
               return;
       }
       objType = OT_LOCATION;
       level   = 0;
       break;

       case OT_PROFILE:
       {
           PROFILEPARAMS profile;
           int err;
           ZEROINIT (profile);

           x_strcpy (profile.ObjectName, objName);
           err = GetDetailInfo (lpVirtNode, OT_PROFILE, &profile, FALSE, &SesHndl);
           if (err != RES_SUCCESS)
               return;
           profile.bCreate = FALSE;
           iret = DlgCreateProfile (hwndMdi, &profile);
           FreeAttachedPointers (&profile,  OT_PROFILE);
           if (!iret)
               return;
       }
       objType = OT_PROFILE;
       level   = 0;
       break;


       case OT_USER:
       {
           USERPARAMS user;
           int err;
           ZEROINIT (user);

           x_strcpy (user.ObjectName, objName);
           err = GetDetailInfo (lpVirtNode, OT_USER, &user, FALSE, &SesHndl);
           if (err != RES_SUCCESS)
               return;
           user.bCreate = FALSE;
           iret = DlgAlterUser (hwndMdi, &user);
           FreeAttachedPointers (&user,  OT_USER);
           if (!iret)
               return;
       }
       objType = OT_USER;
       level   = 0;
       break;

       case OT_GROUP:
       {
           GROUPPARAMS g;
           int err;
           ZEROINIT (g);

           x_strcpy (g.ObjectName, objName);
           err = GetDetailInfo (lpVirtNode, OT_GROUP, &g, FALSE, &SesHndl);
           if (err != RES_SUCCESS)
               return;
           g.bCreate = FALSE;
           iret = DlgAlterGroup (hwndMdi, &g);
           FreeAttachedPointers (&g,  OT_GROUP);
           if (!iret)
               return;
       }
       fstrncpy(Parents[0],objName,MAXOBJECTNAME);
       parentstrings[0]=Parents[0];

       objType = OT_GROUPUSER;
       level   = 1;
       break;

       case OT_ROLE:
       {
           ROLEPARAMS r;
           int err;
           ZEROINIT (r);

           x_strcpy (r.ObjectName, objName);
           err = GetDetailInfo (lpVirtNode, OT_ROLE, &r, FALSE, &SesHndl);
           if (err != RES_SUCCESS)
               return;
           r.bCreate = FALSE;
           iret = DlgAlterRole (hwndMdi, &r);
           FreeAttachedPointers (&r,  OT_ROLE);
           if (!iret)
               return;
       }
       objType = OT_ROLE;
       level   = 0;
       break;

       case OT_TABLE:
       case OT_SCHEMAUSER_TABLE:
           {
                // OPING V20.X
               
                TABLEPARAMS ts;
                int err;
                ZEROINIT (ts);

                lstrcpy (ts.objectname,     RemoveDisplayQuotesIfAny(StringWithoutOwner(objName)));
                lstrcpy (ts.szSchema,       objOwner);
                lstrcpy (ts.DBName,         parentstrings[0]);
                ShowHourGlass ();
                err = GetDetailInfo ((LPSTR)lpVirtNode, OT_TABLE, &ts, FALSE, &SesHndl);
                RemoveHourGlass ();
                if (err != RES_SUCCESS)
                    return;
                iret = VDBA2xAlterTable (hwndMdi, &ts);  
                FreeAttachedPointers (&ts,  OT_TABLE);
                if (!iret)
                    return;
                assert((x_strlen(Quote4DisplayIfNeeded(ts.objectname))+x_strlen(Quote4DisplayIfNeeded(ts.objectname))) < sizeof(objName));

                StringWithOwner((LPUCHAR)Quote4DisplayIfNeeded(ts.objectname), ts.szSchema, objName);
                objType = OT_TABLE;
                level   = 1;          
                break;
           }
       // IngresII Ice
       case OT_ICE_ROLE:
       {
           ICEROLEDATA IceRdta;
           ZEROINIT(IceRdta);
           lstrcpy(IceRdta.RoleName,objName);
           IceRdta.bAlter = TRUE;
           iret = GetICEInfo((LPSTR)lpVirtNode,OT_ICE_ROLE, &IceRdta);
           if(iret!= RES_SUCCESS) {
                MessageWithHistoryButton(GetFocus(), ResourceString(IDS_CANNOT_ALTER_ICE_OBJ_ICE_ERR));
                return;
		   }
           iret = MfcDlgCreateIceRole( &IceRdta, lpDomData->psDomNode->nodeHandle);
           if (!iret)
                return;
       }
       objType = OT_ICE_ROLE;
       level   = 0;
       break;
       case OT_ICE_DBUSER:
       {
           ICEDBUSERDATA Iu;
           ZEROINIT   (Iu);
           lstrcpy(Iu.UserAlias,objName);
           Iu.bAlter =TRUE;
           iret = GetICEInfo((LPSTR)lpVirtNode,OT_ICE_DBUSER, &Iu);
           if(iret!= RES_SUCCESS) {
                MessageWithHistoryButton(GetFocus(), ResourceString(IDS_CANNOT_ALTER_ICE_OBJ_ICE_ERR));
                return;
		   }
           iret = MfcDlgCreateIceDbUser( &Iu ,lpDomData->psDomNode->nodeHandle);
           if (!iret)
                return;
       }
       objType = OT_ICE_DBUSER;
       level   = 0;
       break;
       case OT_ICE_DBCONNECTION:
       {
           DBCONNECTIONDATA Ic;
           ZEROINIT   (Ic);
           lstrcpy(Ic.ConnectionName,objName);
           Ic.bAlter =TRUE;
           iret = GetICEInfo((LPSTR)lpVirtNode,OT_ICE_DBCONNECTION, &Ic);
           if(iret!= RES_SUCCESS) {
                MessageWithHistoryButton(GetFocus(), ResourceString(IDS_CANNOT_ALTER_ICE_OBJ_ICE_ERR));
                return;
		   }
           iret = MfcDlgCreateIceDbConnection(&Ic,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_DBCONNECTION;
       level   = 0;
       break;
       case OT_ICE_WEBUSER:
       {
           ICEWEBUSERDATA Iw;
           ZEROINIT   (Iw);
           lstrcpy(Iw.UserName,objName);
           Iw.bAlter =TRUE;
           iret = GetICEInfo((LPSTR)lpVirtNode,OT_ICE_WEBUSER, &Iw);
           if(iret!= RES_SUCCESS) {
                MessageWithHistoryButton(GetFocus(), ResourceString(IDS_CANNOT_ALTER_ICE_OBJ_ICE_ERR));
                return;
		   }
           iret = MfcDlgCreateIceWebUser(&Iw,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_WEBUSER;
       level   = 0;
       break;
       case OT_ICE_PROFILE:
       {
           ICEPROFILEDATA Ip;
           ZEROINIT   (Ip);
           lstrcpy(Ip.ProfileName,objName);
           Ip.bAlter = TRUE;
           iret = GetICEInfo((LPSTR)lpVirtNode,OT_ICE_PROFILE, &Ip);
           if(iret!= RES_SUCCESS) {
                MessageWithHistoryButton(GetFocus(), ResourceString(IDS_CANNOT_ALTER_ICE_OBJ_ICE_ERR));
                return;
		   }
           iret = MfcDlgCreateIceProfile(&Ip,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_PROFILE;
       level   = 0;
       break;
       case OT_ICE_SERVER_LOCATION:
       {
           ICELOCATIONDATA Il;
           memset(&Il,0,sizeof(ICELOCATIONDATA));
           lstrcpy(Il.LocationName,objName);
           Il.bAlter = TRUE;
           iret = GetICEInfo((LPSTR)lpVirtNode,OT_ICE_SERVER_LOCATION, &Il);
           if(iret!= RES_SUCCESS) {
                MessageWithHistoryButton(GetFocus(), ResourceString(IDS_CANNOT_ALTER_ICE_OBJ_ICE_ERR));
                return;
		   }
           iret = MfcDlgIceServerLocation( &Il ,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_SERVER_LOCATION;
       level   = 0;
       break;
       case OT_ICE_SERVER_VARIABLE:
       {
           ICESERVERVARDATA Iv;
           memset(&Iv,0,sizeof(ICESERVERVARDATA));
           lstrcpy(Iv.VariableName,objName);
           Iv.bAlter = TRUE;
           iret = GetICEInfo((LPSTR)lpVirtNode,OT_ICE_SERVER_VARIABLE, &Iv);
           if(iret!= RES_SUCCESS) {
                MessageWithHistoryButton(GetFocus(), ResourceString(IDS_CANNOT_ALTER_ICE_OBJ_ICE_ERR));
                return;
		   }
           iret = MfcDlgIceServerVariable( &Iv ,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_SERVER_VARIABLE;
       level   = 0;
       break;
       case OT_ICE_BUNIT_SEC_ROLE:
       {
           ICEBUSUNITROLEDATA Ibr;
           ZEROINIT   (Ibr);
           Ibr.bAlter = TRUE;
           lstrcpy(Ibr.icebusunit.Name,parentstrings [0]);
		   lstrcpy(Ibr.icerole.RoleName,objName);
           iret = GetICEInfo((LPSTR)lpVirtNode,OT_ICE_BUNIT_SEC_ROLE, &Ibr);
           if(iret!= RES_SUCCESS) {
                MessageWithHistoryButton(GetFocus(), ResourceString(IDS_CANNOT_ALTER_ICE_OBJ_ICE_ERR));
                return;
		   }
           iret = MfcDlgCreateIceBusinessRole(&Ibr,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_BUNIT_SEC_ROLE ;
       level   = 1;
       break;
       case OT_ICE_BUNIT_SEC_USER:
       {
           ICEBUSUNITWEBUSERDATA Ibu;
           ZEROINIT   (Ibu);
           Ibu.bAlter = TRUE;
           lstrcpy(Ibu.icebusunit.Name,parentstrings [0]);
		   lstrcpy(Ibu.icewevuser.UserName,objName);
           iret = GetICEInfo((LPSTR)lpVirtNode,OT_ICE_BUNIT_SEC_USER, &Ibu);
           if(iret!= RES_SUCCESS) {
                MessageWithHistoryButton(GetFocus(), ResourceString(IDS_CANNOT_ALTER_ICE_OBJ_ICE_ERR));
                return;
		   }
           iret = MfcDlgCreateIceBusinessUser(&Ibu,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_BUNIT_SEC_USER;
       level   = 1;
       break;
       case OT_ICE_BUNIT_FACET:
       {
           ICEBUSUNITDOCDATA Idf;
           ZEROINIT   (Idf);
           Idf.bFacet = TRUE;
           Idf.bAlter = TRUE;
           lstrcpy(Idf.icebusunit.Name,parentstrings[0]);
           OwnerFromString(objName,Idf.name);
           lstrcpy (Idf.suffix , StringWithoutOwner (objName));
           iret = GetICEInfo((LPSTR)lpVirtNode,OT_ICE_BUNIT_FACET, &Idf);
           if(iret!= RES_SUCCESS) {
                MessageWithHistoryButton(GetFocus(), ResourceString(IDS_CANNOT_ALTER_ICE_OBJ_ICE_ERR));
                return;
		   }
           iret = MfcDlgCreateIceFacetPage(&Idf,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_BUNIT_FACET;
       level   = 1;
       break;
       case OT_ICE_BUNIT_PAGE:
       {
           ICEBUSUNITDOCDATA Idp;
           ZEROINIT   (Idp);
           Idp.bFacet = FALSE;
           Idp.bAlter = TRUE;
           lstrcpy(Idp.icebusunit.Name,parentstrings[0]);
           OwnerFromString(objName,Idp.name);
           lstrcpy (Idp.suffix , StringWithoutOwner (objName));
           iret = GetICEInfo((LPSTR)lpVirtNode,OT_ICE_BUNIT_PAGE, &Idp);
           if(iret!= RES_SUCCESS) {
                MessageWithHistoryButton(GetFocus(), ResourceString(IDS_CANNOT_ALTER_ICE_OBJ_ICE_ERR));
                return;
		   }
           iret = MfcDlgCreateIceFacetPage(&Idp,lpDomData->psDomNode->nodeHandle);
           if (!iret)
               return;
       }
       objType = OT_ICE_BUNIT_PAGE;
       level   = 1;
       break;
       case OT_ICE_BUNIT_FACET_ROLE:
       case OT_ICE_BUNIT_PAGE_ROLE:
       {
           ICEBUSUNITDOCROLEDATA IceFrole;
           memset(&IceFrole,0,sizeof(ICEBUSUNITDOCROLEDATA));

           lstrcpy(IceFrole.icebusunitdoc.icebusunit.Name,parentstrings [0]);
           lstrcpy(IceFrole.icebusunitdoc.name,parentstrings [1]);
           lstrcpy(IceFrole.icebusunitdocrole.RoleName, objName);
           if (objType == OT_ICE_BUNIT_FACET_ROLE)
             IceFrole.icebusunitdoc.bFacet = TRUE;
           else
             IceFrole.icebusunitdoc.bFacet = FALSE;
           IceFrole.bAlter = TRUE;
           iret = GetICEInfo((LPSTR)lpVirtNode,objType, &IceFrole);
           if(iret!= RES_SUCCESS) {
                MessageWithHistoryButton(GetFocus(), ResourceString(IDS_CANNOT_ALTER_ICE_OBJ_ICE_ERR));
                return;
		   }
           iret = MfcDlgBuDefRoleUserForFacetPage( objType,
                                                   &IceFrole,
                                                   lpDomData->psDomNode->nodeHandle );
           if (!iret)
               return;
       }
       // Keep objType "as is"
       level   = 2;
       break;

       case OT_ICE_BUNIT_FACET_USER:
       case OT_ICE_BUNIT_PAGE_USER:
       {
           ICEBUSUNITDOCUSERDATA IceRuser;
           memset(&IceRuser,0,sizeof(ICEBUSUNITDOCUSERDATA));
           lstrcpy(IceRuser.icebusunitdoc.icebusunit.Name,parentstrings [0]);
           lstrcpy(IceRuser.icebusunitdoc.name,parentstrings [1]);
           lstrcpy(IceRuser.user.UserName, objName);
           if (objType == OT_ICE_BUNIT_FACET_USER)
             IceRuser.icebusunitdoc.bFacet = TRUE;
           else
             IceRuser.icebusunitdoc.bFacet = FALSE;
           IceRuser.bAlter = TRUE;
           iret = GetICEInfo((LPSTR)lpVirtNode,objType, &IceRuser);
           if(iret!= RES_SUCCESS) {
                MessageWithHistoryButton(GetFocus(), ResourceString(IDS_CANNOT_ALTER_ICE_OBJ_ICE_ERR));
                return;
		   }
           iret = MfcDlgBuDefRoleUserForFacetPage( objType,
                                                   &IceRuser,
                                                   lpDomData->psDomNode->nodeHandle );
           if (!iret)
               return;
       }
       // Keep objType "as is"
       level   = 2;
       break;

       case OT_SEQUENCE:
       {
           SEQUENCEPARAMS seqparams;
           int err;
           ZEROINIT (seqparams);

           lstrcpy (seqparams.objectname,  RemoveDisplayQuotesIfAny(StringWithoutOwner(objName)));
           lstrcpy (seqparams.objectowner, objOwner);
           lstrcpy (seqparams.DBName,      parentstrings[0]);
           lstrcpy (seqparams.szVNode, lpVirtNode);
           err = GetDetailInfo (lpVirtNode, OT_SEQUENCE, &seqparams, FALSE,
                                &SesHndl);
           if (err != RES_SUCCESS)
               return;
           seqparams.bCreate = FALSE;
           iret = MfcDlgCreateSequence( &seqparams );
           FreeAttachedPointers (&seqparams,  OT_SEQUENCE);
           if (!iret)
               return;
       }
       // Keep objType "as is"
       level   = 1;
       break;

       default:
           //
           // Alter not implemented yet for this type of object
           //
           MessageBox(GetFocus(),
                      ResourceString ((UINT)IDS_I_ALTER_NOTDEFINED),
                      NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
           return ;
   } 

   DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, NULL);
       
   switch (objType)
   {
       case OT_STATIC_SECURITY: /* covers all sec.alarm #defines */

           iter = 1; /* all 8 alarms are read at once in the cache,
                        and 8 displays are made in DomUpdateDisplayData */
           parentstrings[1]=NULL; /* Sec.Alarm may apply to multiple tables */
           aObjType[0] = OTLL_SECURITYALARM;
           alevel[0]=1;
           break;

       case OT_REPLIC_CDDS:
           iter = 5;
           parentstrings[1]=NULL; // for all CDDS (level=2)
           aObjType[0] = objType;
           aObjType[1] = OT_REPLIC_REGTABLE;
           aObjType[2] = OT_REPLIC_CDDS_DETAIL;
           aObjType[3] = OT_TABLE;
           aObjType[4] = OT_PROCEDURE;
           alevel[0]=1;
           alevel[1]=1;
           alevel[2] = 2;
           alevel[3] = 1;
           alevel[4] = 1;
           // parentstrings [0] already set
           break;

       case OT_TABLE :
           UpdateDOMDataDetailAllUsers(                  // refreshes columns only
               lpDomData->psDomNode->nodeHandle, // if they are in the cache.
               OT_TABLE,                        // no display (there is no
               1,                                // subbranch for columns)
               parentstrings,                    // columns are used only in
               FALSE,                            // dialogs
               FALSE,TRUE,FALSE,FALSE);    
           parentstrings[1]=objName;             
           UpdateDOMDataDetailAllUsers(                  // refreshes columns only
               lpDomData->psDomNode->nodeHandle, // if they are in the cache.
               OT_COLUMN,                        // no display (there is no
               2,                                // subbranch for columns)
               parentstrings,                    // columns are used only in
               FALSE,                            // dialogs
               FALSE,TRUE,FALSE,FALSE);

           // Alter table can change indexes as of Sep.23
           iter = 2;
           aObjType[0] = OT_TABLE;
           alevel[0]=1;
           aObjType[1] = OT_INDEX;
           alevel[1] = 2;
           break;
       default:
           iter = 1;
           aObjType[0] = objType;
           alevel[0] = level;
           break;
   }
       
   for (cpt=0; cpt<iter; cpt++)
       UpdateDOMDataAllUsers(
           lpDomData->psDomNode->nodeHandle, // hnodestruct
           aObjType[cpt],                    // iobjecttype
           alevel[cpt],                            // level
           parentstrings,                    // parentstrings
           FALSE,                            // keep system tbls state
           FALSE);                           // bWithChildren

   for (cpt=0; cpt<iter; cpt++) {
       DOMUpdateDisplayDataAllUsers (
           lpDomData->psDomNode->nodeHandle, // hnodestruct
           aObjType[cpt],                    // iobjecttype
           alevel[cpt],                      // level
           parentstrings,                    // parentstrings
           FALSE,                            // bWithChildren
           ACT_DROP_OBJECT,                  // object dropped
           0L,                               // no item id
           0);                               // all mdi windows concerned
       if (aObjType[cpt]==OT_DBEVENT)
         UpdateDBEDisplay(lpDomData->psDomNode->nodeHandle, parentstrings[0]);
   }

   // complimentary management of right pane: refresh it immediately
   UpdateRightPane(hwndMdi, lpDomData, TRUE, 0);

   DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, NULL);
}

//
// BOOL DomDropObject(HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam)
//
// Drops an object according to type of the current selection in the lbtree
//
// Modified Emb 19/4/95 : if wParam is not null, don't show the confirm box
//  (usage: drop of the object after having dragged it) 
//
// Modified Emb June 8, 98: lParam was not used, reserved to 0
// Now lParam can be 1L, means "STAR Remove" (opposite of Register as link)
//
// TO BE FINISHED
BOOL DomDropObject(HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam)
{
   int       objType;
   UCHAR     objName[MAXOBJECTNAME];
   UCHAR     objOwner[MAXOBJECTNAME];
   UCHAR     objTypeName[BUFSIZE]; /* used for misc purpose */
   int       i,iret;             
   int       level;
   LPUCHAR   parentstrings [MAXPLEVEL];
   char      rsBuf[BUFSIZE];
   DWORD     dwCurSel;
   UCHAR     Parents[MAXPLEVEL][MAXOBJECTNAME];
   UCHAR     ParentTableOwner[MAXOBJECTNAME];
   UCHAR     buf1[BUFSIZE];
   UCHAR     buf2[BUFSIZE];
   UCHAR     buf3[BUFSIZE];
   LPTREERECORD lpRecord;
   int cpt, iter,inb;
   int aObjType[10];   // CAREFUL WITH THIS MAX VALUE!
   int alevel  [10];
   BOOL aWithChildren [10];
   BOOL bStandardMessage = TRUE;

   // "Drop table created at star level" special management: must refresh 2 tables lists,
   // and also must not refresh OT_SYNONYM on ddb
   BOOL   bMustRefreshStarLocalDB = FALSE;
   char   szStarLocalNodeName[MAXOBJECTNAME];
   char   szStarLocalDbName[MAXOBJECTNAME];
   int    starNodeHandle;
   char* tempParentstrings0;


   // Emb 19/4/95
   if (wParam)
      bStandardMessage = FALSE;   // Don't ask for confirmation

   // Test values in lParam
   if (lParam != 0L && lParam != 1L) {
     assert (FALSE);
     return FALSE;
   }

   if (IsCurSelObjStatic(lpDomData)) 
       return FALSE;

   if (!GetCurSelObjParents (lpDomData, parentstrings))
       return FALSE;
   for (i=0;i<3;i++) {
       fstrncpy(Parents[i],parentstrings[i],MAXOBJECTNAME);
       parentstrings[i]=Parents[i];
   }

   objType = GetCurSelUnsolvedItemObjType (lpDomData);
   dwCurSel=(DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
   if (!dwCurSel)
       return FALSE;   // How did we happen here ?
   fstrncpy(objName,GetObjName(lpDomData, dwCurSel),MAXOBJECTNAME);


   lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                               LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

   fstrncpy(objOwner,lpRecord->ownerName,MAXOBJECTNAME);
   fstrncpy(ParentTableOwner,lpRecord->tableOwner,MAXOBJECTNAME);

   if (!CanObjectBeDeleted(objType,objName,objOwner)) {
       //
       // Drop is not allowed for this object
       //
       ErrorMessage ((UINT)IDS_E_DROP_OBJECT_DENIED, RES_ERR);
       return FALSE;
   }

   for (inb = 0; inb <10 ; inb++ )
      aWithChildren[inb] = FALSE;

   switch (objType) {
      case OT_SYNONYMOBJECT:
         { // objType is changed in the following function call
           int resultlevel;
           fstrncpy(Parents[0],parentstrings[0],MAXOBJECTNAME);
           parentstrings[0]=Parents[0];
           parentstrings[1]=Parents[1];
           parentstrings[2]=Parents[2];
           if (DOMGetObject(GetMDINodeHandle(hwndMdi), objName, objOwner,
                        objType,1, parentstrings, GetMDISystemFlag(hwndMdi),
                        &objType,&resultlevel,parentstrings,buf1, buf2,buf3)!=RES_SUCCESS) {
               //
               // Object not found
               //
               ErrorMessage ((UINT)IDS_E_OBJECT_NOT_FOUND, RES_ERR);
               return FALSE;
           }
           fstrncpy(objOwner,buf2,MAXOBJECTNAME);
         }
         break;
      case OTR_TABLEVIEW:
         objType=OT_VIEW;
         break;
      case OTR_USERGROUP:
         objType=OT_GROUP;
         break;
      case OTR_LOCATIONTABLE:
         objType=OT_TABLE;
         break;

      default:
         break;
      
   }

   switch (objType)
   {
       case OT_ROLEGRANT_USER:
           {
               REVOKEPARAMS r;
               ZEROINIT (r);

               x_strcpy (r.PreselectObject,  parentstrings [0]);
               x_strcpy (r.PreselectGrantee, objName);
               iret = DlgRvkgrol (hwndMdi, &r);
               if (!iret)
                   return FALSE;

               bStandardMessage = FALSE;
           }
           break;

       case OT_VIEWGRANT_SEL_USER  :
       case OT_VIEWGRANT_INS_USER  :
       case OT_VIEWGRANT_UPD_USER  :
       case OT_VIEWGRANT_DEL_USER  :
       case OT_DBEGRANT_RAISE_USER :
       case OT_DBEGRANT_REGTR_USER :
       case OT_PROCGRANT_EXEC_USER :
       case OT_SEQUGRANT_NEXT_USER :
       case OT_TABLEGRANT_SEL_USER :
       case OT_TABLEGRANT_INS_USER :
       case OT_TABLEGRANT_UPD_USER :
       case OT_TABLEGRANT_DEL_USER :
       case OT_TABLEGRANT_CPI_USER:
       case OT_TABLEGRANT_CPF_USER:
       case OT_TABLEGRANT_REF_USER :
       case OT_TABLEGRANT_ALL_USER :
       case OT_TABLEGRANT_INDEX_USER:
       case OT_TABLEGRANT_ALTER_USER:
       {
           int gtype = -1;
           REVOKEPARAMS revoke;
           ZEROINIT    (revoke);

           x_strcpy (revoke.PreselectGrantee, objName);          // User (grantee)
           gtype = DBA_GetObjectType  (objName, lpDomData);
           //if (gtype != OT_UNKNOWN)
           revoke.GranteeType = gtype;

           switch (objType)
           {
               case OT_DBEGRANT_RAISE_USER :
               case OT_DBEGRANT_REGTR_USER :
                   revoke.ObjectType   = OT_DBEVENT;
                   break;
               case OT_PROCGRANT_EXEC_USER :
                   revoke.ObjectType   = OT_PROCEDURE;
                   break;
               case OT_SEQUGRANT_NEXT_USER :
                   revoke.ObjectType   = OT_SEQUENCE;
                   break;
               case OT_VIEWGRANT_SEL_USER  :
               case OT_VIEWGRANT_INS_USER  :
               case OT_VIEWGRANT_UPD_USER  :
               case OT_VIEWGRANT_DEL_USER  :
                   revoke.ObjectType   = OT_VIEW;
                   break;
               default:
                   revoke.ObjectType   = OT_TABLE;
                   break;
           }

           x_strcpy (revoke.DBName,              parentstrings [0]); // Database name
           x_strcpy (revoke.PreselectObject,     RemoveDisplayQuotesIfAny(StringWithoutOwner(parentstrings [1]))); // Table or View or Procedure or DBevent 
           x_strcpy (revoke.PreselectObjectOwner,ParentTableOwner);  // Object's owner
           switch (objType)
           {
               case OT_VIEWGRANT_SEL_USER  :
               case OT_TABLEGRANT_SEL_USER :
                   revoke.PreselectPrivileges [GRANT_SELECT]     = TRUE;
                   break;
               case OT_VIEWGRANT_INS_USER  :
               case OT_TABLEGRANT_INS_USER :
                   revoke.PreselectPrivileges [GRANT_INSERT]     = TRUE;
                   break;
               case OT_VIEWGRANT_UPD_USER  :
               case OT_TABLEGRANT_UPD_USER :
                   revoke.PreselectPrivileges [GRANT_UPDATE]     = TRUE;
                   break;
               case OT_VIEWGRANT_DEL_USER  :
               case OT_TABLEGRANT_DEL_USER :
                   revoke.PreselectPrivileges [GRANT_DELETE]     = TRUE;
                   break;
               case OT_TABLEGRANT_CPI_USER:
                   revoke.PreselectPrivileges [GRANT_COPY_INTO]  = TRUE;
                   break;
               case OT_TABLEGRANT_CPF_USER:
                   revoke.PreselectPrivileges [GRANT_COPY_FROM]  = TRUE;
                   break;
               case OT_TABLEGRANT_REF_USER :
                   revoke.PreselectPrivileges [GRANT_REFERENCE]  = TRUE;
                   break;
               case OT_DBEGRANT_RAISE_USER :
                   revoke.PreselectPrivileges [GRANT_RAISE]      = TRUE;
                   break;
               case OT_DBEGRANT_REGTR_USER :
                   revoke.PreselectPrivileges [GRANT_REGISTER]   = TRUE;
                   break;
               case OT_PROCGRANT_EXEC_USER :
                   revoke.PreselectPrivileges [GRANT_EXECUTE]    = TRUE;
                   break;
               case OT_TABLEGRANT_ALL_USER :
                   revoke.PreselectPrivileges [GRANT_SELECT]    = TRUE;
                   revoke.PreselectPrivileges [GRANT_INSERT]    = TRUE;
                   revoke.PreselectPrivileges [GRANT_DELETE]    = TRUE;
                   revoke.PreselectPrivileges [GRANT_UPDATE]    = TRUE;
                   revoke.PreselectPrivileges [GRANT_REFERENCE] = TRUE;
                   revoke.PreselectPrivileges [GRANT_COPY_INTO] = TRUE;
                   revoke.PreselectPrivileges [GRANT_COPY_FROM] = TRUE;
                   break;
               case OT_SEQUGRANT_NEXT_USER:
                   revoke.PreselectPrivileges [GRANT_NEXT_SEQUENCE]    = TRUE;
                   break;
           }

           iret = DlgRevokeOnTable (hwndMdi, &revoke);
           if (!iret)
               return FALSE;
           bStandardMessage = FALSE;
       }
       break;

       case OT_DBGRANT_ACCESY_USER :
       case OT_DBGRANT_ACCESN_USER :
       case OT_DBGRANT_CREPRY_USER :
       case OT_DBGRANT_CREPRN_USER :
       case OT_DBGRANT_CRETBY_USER :
       case OT_DBGRANT_CRETBN_USER :
       case OT_DBGRANT_DBADMY_USER :
       case OT_DBGRANT_DBADMN_USER :
       case OT_DBGRANT_LKMODY_USER :
       case OT_DBGRANT_LKMODN_USER :
       case OT_DBGRANT_QRYIOY_USER :
       case OT_DBGRANT_QRYION_USER :
       case OT_DBGRANT_QRYRWY_USER :
       case OT_DBGRANT_QRYRWN_USER :
       case OT_DBGRANT_UPDSCY_USER :
       case OT_DBGRANT_UPDSCN_USER :
       case OT_DBGRANT_SELSCY_USER: 
       case OT_DBGRANT_SELSCN_USER: 
       case OT_DBGRANT_CNCTLY_USER: 
       case OT_DBGRANT_CNCTLN_USER: 
       case OT_DBGRANT_IDLTLY_USER: 
       case OT_DBGRANT_IDLTLN_USER: 
       case OT_DBGRANT_SESPRY_USER: 
       case OT_DBGRANT_SESPRN_USER: 
       case OT_DBGRANT_TBLSTY_USER: 
       case OT_DBGRANT_TBLSTN_USER: 

       case OT_DBGRANT_QRYCPY_USER: 
       case OT_DBGRANT_QRYCPN_USER: 
       case OT_DBGRANT_QRYPGY_USER: 
       case OT_DBGRANT_QRYPGN_USER: 
       case OT_DBGRANT_QRYCOY_USER: 
       case OT_DBGRANT_QRYCON_USER: 
       case OT_DBGRANT_SEQCRN_USER:
       case OT_DBGRANT_SEQCRY_USER:

       {
           int gtype;
           REVOKEPARAMS revoke;
           ZEROINIT    (revoke);

           x_strcpy (revoke.DBName,          parentstrings [0]); // Database name
           x_strcpy (revoke.PreselectGrantee, objName);          // User (grantee)
           if (x_strlen(revoke.DBName)==0)
               revoke.bInstallLevel = TRUE;
           else
               revoke.bInstallLevel = FALSE;

           gtype = DBA_GetObjectType  (objName, lpDomData);
           if (gtype != OT_UNKNOWN)
               revoke.GranteeType = gtype;

           revoke.ObjectType   = OT_DATABASE;
           switch (objType)
           {
               case OT_DBGRANT_ACCESY_USER :
                   revoke.PreselectPrivileges [GRANT_ACCESS]               = TRUE;
                   break;                                                  
               case OT_DBGRANT_ACCESN_USER :
                   revoke.PreselectPrivileges [GRANT_NO_ACCESS]            = TRUE;
                   break;
               case OT_DBGRANT_CREPRY_USER :
                   revoke.PreselectPrivileges [GRANT_CREATE_PROCEDURE]     = TRUE;
                   break;
               case OT_DBGRANT_CREPRN_USER :
                   revoke.PreselectPrivileges [GRANT_NO_CREATE_PROCEDURE]  = TRUE;
                   break;
               case OT_DBGRANT_CRETBY_USER :
                   revoke.PreselectPrivileges [GRANT_CREATE_TABLE]         = TRUE;
                   break;
               case OT_DBGRANT_CRETBN_USER :
                   revoke.PreselectPrivileges [GRANT_NO_CREATE_TABLE]      = TRUE;
                   break;
               case OT_DBGRANT_DBADMY_USER :
                   revoke.PreselectPrivileges [GRANT_DATABASE_ADMIN]       = TRUE;
                   break;
               case OT_DBGRANT_DBADMN_USER :
                   revoke.PreselectPrivileges [GRANT_NO_DATABASE_ADMIN]    = TRUE;
                   break;
               case OT_DBGRANT_LKMODY_USER :
                   revoke.PreselectPrivileges [GRANT_LOCKMOD]              = TRUE;
                   break;
               case OT_DBGRANT_LKMODN_USER :
                   revoke.PreselectPrivileges [GRANT_NO_LOCKMOD]           = TRUE;
                   break;
               case OT_DBGRANT_QRYIOY_USER :
                   revoke.PreselectPrivileges [GRANT_QUERY_IO_LIMIT]       = TRUE;
                   break;
               case OT_DBGRANT_QRYION_USER :
                   revoke.PreselectPrivileges [GRANT_NO_QUERY_IO_LIMIT]    = TRUE;
                   break;
               case OT_DBGRANT_QRYRWY_USER :
                   revoke.PreselectPrivileges [GRANT_QUERY_ROW_LIMIT]      = TRUE;
                   break;
               case OT_DBGRANT_QRYRWN_USER :
                   revoke.PreselectPrivileges [GRANT_NO_QUERY_ROW_LIMIT]   = TRUE;
                   break;
               case OT_DBGRANT_UPDSCY_USER :
                   revoke.PreselectPrivileges [GRANT_UPDATE_SYSCAT]        = TRUE;
                   break;
               case OT_DBGRANT_UPDSCN_USER :
                   revoke.PreselectPrivileges [GRANT_NO_UPDATE_SYSCAT]     = TRUE;
                   break;
               case OT_DBGRANT_SELSCY_USER: 
                   revoke.PreselectPrivileges [GRANT_SELECT_SYSCAT]        = TRUE;
                   break;
               case OT_DBGRANT_SELSCN_USER: 
                   revoke.PreselectPrivileges [GRANT_NO_SELECT_SYSCAT]     = TRUE;
                   break;
               case OT_DBGRANT_CNCTLY_USER: 
                   revoke.PreselectPrivileges [GRANT_CONNECT_TIME_LIMIT]   = TRUE;
                   break;
               case OT_DBGRANT_CNCTLN_USER: 
                   revoke.PreselectPrivileges [GRANT_NO_CONNECT_TIME_LIMIT]= TRUE;
                   break;
               case OT_DBGRANT_IDLTLY_USER: 
                   revoke.PreselectPrivileges [GRANT_IDLE_TIME_LIMIT]      = TRUE;
                   break;
               case OT_DBGRANT_IDLTLN_USER: 
                   revoke.PreselectPrivileges [GRANT_NO_IDLE_TIME_LIMIT]= TRUE;
                   break;
               case OT_DBGRANT_SESPRY_USER: 
                   revoke.PreselectPrivileges [GRANT_SESSION_PRIORITY]     = TRUE;
                   break;
               case OT_DBGRANT_SESPRN_USER: 
                   revoke.PreselectPrivileges [GRANT_NO_SESSION_PRIORITY]  = TRUE;
                   break;
               case OT_DBGRANT_TBLSTY_USER: 
                   revoke.PreselectPrivileges [GRANT_TABLE_STATISTICS]     = TRUE;
                   break;
               case OT_DBGRANT_TBLSTN_USER: 
                   revoke.PreselectPrivileges [GRANT_NO_TABLE_STATISTICS]  = TRUE;
                   break;
               case OT_DBGRANT_QRYCPN_USER:
                   revoke.PreselectPrivileges [GRANT_NO_QUERY_CPU_LIMIT]   = TRUE;
                   break;
               case OT_DBGRANT_QRYCPY_USER:
                   revoke.PreselectPrivileges [GRANT_QUERY_CPU_LIMIT]   = TRUE;
                   break;
               case OT_DBGRANT_QRYPGN_USER:
                   revoke.PreselectPrivileges [GRANT_NO_QUERY_PAGE_LIMIT]   = TRUE;
                   break;
               case OT_DBGRANT_QRYPGY_USER:
                   revoke.PreselectPrivileges [GRANT_QUERY_PAGE_LIMIT]   = TRUE;
                   break;
               case OT_DBGRANT_QRYCON_USER:
                   revoke.PreselectPrivileges [GRANT_NO_QUERY_COST_LIMIT]   = TRUE;
                   break;
               case OT_DBGRANT_QRYCOY_USER:
                   revoke.PreselectPrivileges [GRANT_QUERY_COST_LIMIT]   = TRUE;
                   break;
               case OT_DBGRANT_SEQCRN_USER:
                   revoke.PreselectPrivileges [GRANT_NO_CREATE_SEQUENCE]   = TRUE;
                   break;
               case OT_DBGRANT_SEQCRY_USER:
                   revoke.PreselectPrivileges [GRANT_CREATE_SEQUENCE]   = TRUE;
                   break;
               default:
                   break;
           }

           iret = DlgRevokeOnDatabase (hwndMdi, &revoke);
           if (!iret)
               return FALSE;

           bStandardMessage = FALSE;
       }
       break;

       case OT_GROUPUSER:
           bStandardMessage = FALSE;
           wsprintf(
               buf3,
               ResourceString ((UINT)IDS_I_CONFIRM_DROP),          //Confirm Drop on %s
               GetVirtNodeName (GetMDINodeHandle (hwndMdi)));

           wsprintf(
               rsBuf,
               ResourceString ((UINT)IDS_C_DROP_USERFROM_GROUP),   //Drop user %s from group %s
               objName,
               parentstrings[0]);
           iret = MessageBox( GetFocus(), rsBuf, buf3,
                              MB_ICONQUESTION | MB_YESNO | MB_TASKMODAL);
           if (iret != IDYES)
               return FALSE;
           break;

       case OT_LOCATION:
           fstrncpy(objTypeName, ResourceString ((UINT)IDS_I_LOCATION),  MAXOBJECTNAME);
           break;
       case OT_DATABASE:
           fstrncpy(objTypeName, ResourceString ((UINT)IDS_I_DATABASE),  MAXOBJECTNAME);
           break;
       case OT_PROFILE:
           bStandardMessage = FALSE;
           fstrncpy(objTypeName, ResourceString ((UINT)IDS_I_PROFILE),   MAXOBJECTNAME);
           break;
       case OT_USER:
           fstrncpy(objTypeName, ResourceString ((UINT)IDS_I_USER),      MAXOBJECTNAME);
           break;
       case OT_GROUP:
           fstrncpy(objTypeName, ResourceString ((UINT)IDS_I_GROUP),     MAXOBJECTNAME);
           break;
       case OT_ROLE:
           fstrncpy(objTypeName, ResourceString ((UINT)IDS_I_ROLE),      MAXOBJECTNAME);
           break;
       case OT_RULE:
           fstrncpy(objTypeName, ResourceString ((UINT)IDS_I_RULE),      MAXOBJECTNAME);
           break;
       case OT_DBEVENT:
           fstrncpy(objTypeName, ResourceString ((UINT)IDS_I_DBEVENT),   MAXOBJECTNAME);
           break;
       case OT_SYNONYM:
       case OTR_INDEXSYNONYM:
       case OTR_TABLESYNONYM:
       case OTR_VIEWSYNONYM :
           fstrncpy(objTypeName, ResourceString ((UINT)IDS_I_SYNONYM),   MAXOBJECTNAME);
           break;
       case OT_INTEGRITY:
           fstrncpy(objTypeName, ResourceString ((UINT)IDS_I_INTEGRITY), MAXOBJECTNAME);
           break;
       case OT_VIEW:
       case OT_SCHEMAUSER_VIEW:
           fstrncpy(objTypeName, ResourceString ((UINT)IDS_I_VIEW),      MAXOBJECTNAME);
           break;
       case OT_PROCEDURE:
       case OT_SCHEMAUSER_PROCEDURE:
           fstrncpy(objTypeName, ResourceString ((UINT)IDS_I_PROCEDURE), MAXOBJECTNAME);
           break;
       case OT_SEQUENCE:
           fstrncpy(objTypeName, ResourceString ((UINT)IDS_I_SEQUENCE), MAXOBJECTNAME);
           break;
       case OT_INDEX:
           fstrncpy(objTypeName, ResourceString ((UINT)IDS_I_INDEX),     MAXOBJECTNAME);
           break;
       case OT_TABLE:
       case OT_SCHEMAUSER_TABLE:
           fstrncpy(objTypeName, ResourceString ((UINT)IDS_I_TABLE),     MAXOBJECTNAME);
           break;
       case OT_REPLIC_CONNECTION:
           fstrncpy(objTypeName, ResourceString((UINT)IDS_I_REPLIC_CONN),MAXOBJECTNAME);
           break;
       case OT_REPLIC_MAILUSER:
           fstrncpy(objTypeName, ResourceString((UINT)IDS_I_REPLIC_MAILUSR), MAXOBJECTNAME);
           break;

       case OT_REPLIC_CDDS:
           if (!x_strcmp(objName,"0")) {
              //"Cannot Drop CDDS 0"
              MessageBox (GetFocus(), ResourceString(IDS_ERR_DROP_CDDS_ZERO), NULL,
                       MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
              return FALSE;
           }
           fstrncpy(objTypeName, ResourceString((UINT)IDS_I_REPLIC_CDDS), MAXOBJECTNAME);
           break;



       case OT_S_ALARM_SELSUCCESS_USER:
       case OT_S_ALARM_SELFAILURE_USER:
       case OT_S_ALARM_DELSUCCESS_USER:
       case OT_S_ALARM_DELFAILURE_USER:
       case OT_S_ALARM_INSSUCCESS_USER:
       case OT_S_ALARM_INSFAILURE_USER:
       case OT_S_ALARM_UPDSUCCESS_USER:
       case OT_S_ALARM_UPDFAILURE_USER:
       {
           char bufftable [MAXOBJECTNAME];

           OwnerFromString(parentstrings [1],ParentTableOwner);
           GetExactDisplayString (
               (LPTSTR)RemoveDisplayQuotesIfAny(StringWithoutOwner(parentstrings [1])),  /* Table name   */
               ParentTableOwner,   /* Table owner  */
               OT_TABLE,
               parentstrings,
               bufftable);

           wsprintf(
               objTypeName,
               ResourceString ((UINT)IDS_I_DROP_SECURITY_ON),  // Security alarm %ld on %s
               GetCurSelComplimLong (lpDomData),
               bufftable);

           objName[0]='\0';
           break;
       }

       case OT_S_ALARM_CO_SUCCESS_USER :
       case OT_S_ALARM_CO_FAILURE_USER :
       case OT_S_ALARM_DI_SUCCESS_USER :
       case OT_S_ALARM_DI_FAILURE_USER :
       {
           wsprintf(
               objTypeName,
               ResourceString ((UINT)IDS_I_DROP_SECURITY),     // Security alarm %ld,
               GetCurSelComplimLong (lpDomData) );

           objName[0]='\0';
           break;
       }

       case OTR_ALARM_SELSUCCESS_TABLE:
       case OTR_ALARM_SELFAILURE_TABLE: 
       case OTR_ALARM_DELSUCCESS_TABLE: 
       case OTR_ALARM_DELFAILURE_TABLE: 
       case OTR_ALARM_INSSUCCESS_TABLE: 
       case OTR_ALARM_INSFAILURE_TABLE:
       case OTR_ALARM_UPDSUCCESS_TABLE:
       case OTR_ALARM_UPDFAILURE_TABLE:
           wsprintf(
               objTypeName,
               ResourceString ((UINT)IDS_I_DROP_SECURITY_ONDB),// Security alarm %ld on database %s,
               GetCurSelComplimLong (lpDomData),
               parentstrings[0]);
           objName[0]='\0';
           break;

       // OpenIngres ICE
       case OT_ICE_ROLE: 
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_ROLE), MAXOBJECTNAME);//"ICE Role"
           break;
       case OT_ICE_DBUSER:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_DB_USER), MAXOBJECTNAME);//"ICE Database User"
           break;
       case OT_ICE_DBCONNECTION:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_DB_CONNECTION), MAXOBJECTNAME);//"ICE Database Connection"
           break;
       case OT_ICE_WEBUSER:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_WEB_USER), MAXOBJECTNAME);//"ICE WEB User"
           break;
       case OT_ICE_PROFILE:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_PROFILE), MAXOBJECTNAME);//"ICE Profile"
           break;
       case OT_ICE_WEBUSER_ROLE:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_WBR), MAXOBJECTNAME);//"ICE Web User Role Association "
           break;
       case OT_ICE_WEBUSER_CONNECTION:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_WBC), MAXOBJECTNAME);//"ICE Web User Connection Association"
           break;
       case OT_ICE_PROFILE_ROLE:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_PR), MAXOBJECTNAME);//"ICE Profile Role Association"
           break;
       case OT_ICE_PROFILE_CONNECTION:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_PRDBC), MAXOBJECTNAME);//"ICE Profile Database Connection Association"
           break;
       case OT_ICE_BUNIT:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_B_UNIT), MAXOBJECTNAME);//"ICE Business Unit"
           break;
       case OT_ICE_BUNIT_SEC_ROLE:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_BUNIT_ROLE), MAXOBJECTNAME);//"ICE Business Unit Role Association"
           break;
       case OT_ICE_BUNIT_SEC_USER:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_BUNIT_USER), MAXOBJECTNAME);//"ICE Business Unit User Association"
           break;
       case OT_ICE_BUNIT_FACET:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_BUNIT_FACET), MAXOBJECTNAME);//"ICE Business Unit Facet"
           break;
       case OT_ICE_BUNIT_PAGE:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_BUNIT_PAGE), MAXOBJECTNAME);//"ICE Business Unit Page"
           break;
       case OT_ICE_BUNIT_FACET_ROLE:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_BUNIT_FROLE), MAXOBJECTNAME);//"ICE Business Unit Facet Role Association"
           break;
       case OT_ICE_BUNIT_PAGE_ROLE:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_BUNIT_PROLE), MAXOBJECTNAME);//"ICE Business Unit Page Role Association"
           break;
       case OT_ICE_BUNIT_FACET_USER:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_BUNIT_FUSER), MAXOBJECTNAME);//"ICE Business Unit Facet User Association"
           break;
       case OT_ICE_BUNIT_PAGE_USER:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_BUNIT_PUSER), MAXOBJECTNAME);//"ICE Business Unit Page User Association"
           break;
       case OT_ICE_BUNIT_LOCATION:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_BUNIT_LOC), MAXOBJECTNAME);//"ICE Business Unit Location"
           break;
       case OT_ICE_SERVER_APPLICATION:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_SESSION_GRP), MAXOBJECTNAME);//"ICE Session Group"
           break;
       case OT_ICE_SERVER_LOCATION:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_LOCATION), MAXOBJECTNAME);//"ICE Location"
           break;
       case OT_ICE_SERVER_VARIABLE:
           fstrncpy(objTypeName, ResourceString(IDS_I_DROP_ICE_SVR_VAR), MAXOBJECTNAME);//"ICE Server Variable"
           break;

       default:
           ErrorMessage ((UINT)IDS_I_DROP_NOTDEFINED, RES_ERR);// Drop not implemented yet for this type of object,
           return FALSE;
           break;
   }
   
   if (bStandardMessage)
   {
     if (lParam == 0L)
     {
       wsprintf(
           buf3,
           ResourceString ((UINT)IDS_I_CONFIRM_DROP),          // Confirm Drop on %s
           GetVirtNodeName (GetMDINodeHandle (hwndMdi)));
       wsprintf(
           rsBuf,
           ResourceString ((UINT)IDS_C_DROP_OBJECT),           // Drop %s %s,
           objTypeName,
           objName);
     }
     else
     {
       wsprintf(
           buf3,
           ResourceString ((UINT)IDS_I_CONFIRM_REMOVE),          // Confirm Remove on %s
           GetVirtNodeName (GetMDINodeHandle (hwndMdi)));
       wsprintf(
           rsBuf,
           ResourceString ((UINT)IDS_C_REMOVE_OBJECT),           // Remove %s %s,
           objTypeName,
           objName);
     }
     iret = MessageBox (GetFocus(), rsBuf, buf3,
                        MB_ICONQUESTION | MB_YESNO | MB_TASKMODAL);
     if (iret != IDYES)
       return FALSE;
   }

   iret=RES_SUCCESS;

   switch (objType)
   {
       case OT_DATABASE:
           {
           UCHAR username[200];
           wsprintf(
               rsBuf,
               ResourceString ((UINT)IDS_C_DROP_DATABASE),     // LAST CONFIRMATION: Confirm Drop database %s ?
               objName);

           iret = MessageBox (GetFocus(), rsBuf,
                              ResourceString ((UINT)IDS_CONFIRMDROP),
                              MB_ICONQUESTION|MB_YESNO | MB_TASKMODAL);
           if (iret != IDYES)
               return FALSE;
           if(!DBAGetSessionUser(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
                                 username)) {
             ErrorMessage ((UINT)IDS_E_USER_ID_ERROR, RES_ERR);// user id error
             return  FALSE;
           }
             wsprintf(buf1,"destroydb %s -u%s", objName,username);
             wsprintf(
                 buf2,
                 ResourceString ((UINT)IDS_I_DESTROYDB),         // Destroying Database %s::%s
                 GetVirtNodeName (GetMDINodeHandle (hwndMdi)),
                 objName);       
             execrmcmd (GetVirtNodeName (GetMDINodeHandle (hwndMdi)), buf1, buf2);
             level = 0;
             iret=RES_SUCCESS;

           }
       break;

       case OT_LOCATION:
           {
           LOCATIONPARAMS locparams;
           ZEROINIT (locparams);
           level = 0;
           fstrncpy(locparams.objectname,objName,MAXOBJECTNAME);
           iret = DBADropObject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
                               objType, (void *) &locparams);
           }
       break;

       case OT_PROFILE:
           {
               DROPOBJECTSTRUCT drop;
               PROFILEPARAMS p;
               char szTitle [256];
               ZEROINIT (p);
               ZEROINIT (drop);
               level = 0;
               fstrncpy(p.ObjectName,objName,MAXOBJECTNAME);
               //"Drop Profile %s"
               wsprintf (szTitle, ResourceString(IDS_TITLE_PROFILE), p.ObjectName);
               if (VDBA_DropObjectCacadeRestrict(&drop, szTitle))
               {
                   p.bRestrict = drop.bRestrict;
                   iret = DBADropObject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)), objType, (void *) &p);
               }
               else
                   return FALSE;
           }
       break;

       case OT_USER:
           {
           USERPARAMS userparams;
           ZEROINIT (userparams);
           level = 0;
           fstrncpy(userparams.ObjectName,objName,MAXOBJECTNAME);
           iret = DBADropObject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
                               objType, (void *) &userparams);
           }
       break;

       case OT_GROUP:
           {
                GROUPPARAMS groupparams;
                ZEROINIT (groupparams);
                level = 0;
                fstrncpy(groupparams.ObjectName,objName,MAXOBJECTNAME);

                if (VDBA_GroupHasUsers())
                {
                     TCHAR tchszMessage [256];
                     // "The group %s contains user(s).\nDo you wish to continue ?"
                     wsprintf (tchszMessage, ResourceString(IDS_ERR_GROUP_CONTAINS_USER), objName);
                     if (MessageBox (GetFocus(), tchszMessage, ResourceString(IDS_TITLE_CONFIRM), MB_ICONQUESTION|MB_YESNO) == IDYES)
                     {
                         groupparams.bRemoveAllUsers = TRUE;
                         iret = DBADropObject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)), objType, (void *) &groupparams);
                     }
                     else
                         return FALSE;
                }
                else
                      iret = DBADropObject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
                                    objType, (void *) &groupparams);
           }
       break;

       case OT_GROUPUSER:
           {
           GROUPUSERPARAMS groupuserparams;
           ZEROINIT (groupuserparams);
           level = 1;
           fstrncpy(groupuserparams.GroupName, parentstrings[0],MAXOBJECTNAME);
           fstrncpy(groupuserparams.ObjectName,objName,MAXOBJECTNAME);
           iret = DBADropObject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
                               objType, (void *) &groupuserparams);
           }
       break;

       case OT_ROLE:
           {
           ROLEPARAMS roleparams;
           ZEROINIT (roleparams);
           level = 0;
           fstrncpy(roleparams.ObjectName,objName,MAXOBJECTNAME);
           iret = DBADropObject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
                               objType, (void *) &roleparams);
           }
       break;

       case OT_RULE:
           {
           RULEPARAMS ruleparams;
           ZEROINIT (ruleparams);
           level = 2;
           fstrncpy(ruleparams.DBName,parentstrings[0],MAXOBJECTNAME);
           fstrncpy(ruleparams.RuleName,(LPTSTR)RemoveDisplayQuotesIfAny(StringWithoutOwner(objName)),MAXOBJECTNAME);
           fstrncpy(ruleparams.RuleOwner,objOwner,MAXOBJECTNAME);
           iret = DBADropObject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
                               objType, (void *) &ruleparams);
           }
       break;

       case OT_SYNONYM:
       case OTR_INDEXSYNONYM:
       case OTR_TABLESYNONYM:
       case OTR_VIEWSYNONYM :
           {
           SYNONYMPARAMS synonymparams;
           ZEROINIT (synonymparams);
           level = 1;
           objType=OT_SYNONYM;
           fstrncpy(synonymparams.DBName,parentstrings[0],MAXOBJECTNAME);
           fstrncpy(synonymparams.ObjectName,objName,MAXOBJECTNAME);
           iret = DBADropObject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
                               objType, (void *) &synonymparams);
           }
       break;

       case OT_DBEVENT:
           {
           DBEVENTPARAMS dbeventparams;
           ZEROINIT (dbeventparams);
           level = 1;
           fstrncpy(dbeventparams.DBName,parentstrings[0],MAXOBJECTNAME);
           wsprintf(dbeventparams.ObjectName,"%s.%s",QuoteIfNeeded(objOwner),
                                                     QuoteIfNeeded(RemoveDisplayQuotesIfAny(StringWithoutOwner(objName))));
           iret = DBADropObject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
                               objType, (void *) &dbeventparams);
           }
       break;


       case OT_INTEGRITY:
       {
           INTEGRITYPARAMS integrityparams;
           ZEROINIT (integrityparams);
           level = 2;
           integrityparams.number = GetCurSelComplimLong (lpDomData);
           x_strcpy (integrityparams.DBName   , parentstrings [0]); 
           x_strcpy (integrityparams.TableName , RemoveDisplayQuotesIfAny(StringWithoutOwner(parentstrings [1])));
           x_strcpy (integrityparams.TableOwner, ParentTableOwner);

           iret = DBADropObject (GetVirtNodeName (GetMDINodeHandle (hwndMdi)),
                   objType, (void *) &integrityparams);
       }
       break;



       case OTR_ALARM_SELSUCCESS_TABLE:
       case OTR_ALARM_SELFAILURE_TABLE: 
       case OTR_ALARM_DELSUCCESS_TABLE: 
       case OTR_ALARM_DELFAILURE_TABLE: 
       case OTR_ALARM_INSSUCCESS_TABLE: 
       case OTR_ALARM_INSFAILURE_TABLE:
       case OTR_ALARM_UPDSUCCESS_TABLE:
       case OTR_ALARM_UPDFAILURE_TABLE:
           objType=OT_STATIC_SECURITY;
           /* no break after this line */
           /* used to distinguish OTR_ and OT_ objects*/
       case OT_S_ALARM_SELSUCCESS_USER:
       case OT_S_ALARM_SELFAILURE_USER:
       case OT_S_ALARM_DELSUCCESS_USER:
       case OT_S_ALARM_DELFAILURE_USER:
       case OT_S_ALARM_INSSUCCESS_USER:
       case OT_S_ALARM_INSFAILURE_USER:
       case OT_S_ALARM_UPDSUCCESS_USER:
       case OT_S_ALARM_UPDFAILURE_USER:
       {
           LPCHECKEDOBJECTS obj;

           SECURITYALARMPARAMS security;
           ZEROINIT (security);
           level = 2;

           security.lpfirstTableIn = NULL;
           security.iObjectType    = OT_TABLE;
           security.SecAlarmId     = GetCurSelComplimLong (lpDomData);
           x_strcpy (security.DBName,  parentstrings [0]);
               
           obj = ESL_AllocMem (sizeof (CHECKEDOBJECTS));
           if (!obj)
             return FALSE;
           if (objType==OT_STATIC_SECURITY)
             x_strcpy (obj->dbname, GetObjName(lpDomData, dwCurSel));
           else
           {
             // format string for SQL statement 
             OwnerFromString(parentstrings [1],ParentTableOwner);
             wsprintf(obj->dbname,"%s.%s",QuoteIfNeeded(ParentTableOwner),
                QuoteIfNeeded( RemoveDisplayQuotesIfAny(StringWithoutOwner(parentstrings [1]))));
           }
           obj->bchecked   = TRUE;
           obj->pnext      = NULL;
           security.lpfirstTableIn =
                             AddCheckedObject (security.lpfirstTableIn, obj);

           iret = DBADropObject (GetVirtNodeName(GetMDINodeHandle (hwndMdi)),
                   OTLL_SECURITYALARM, (void *) &security);

           security.lpfirstTableIn =
                                FreeCheckedObjects (security.lpfirstTableIn);
           objType=OT_STATIC_SECURITY;
       }
       break;

       case OT_S_ALARM_CO_SUCCESS_USER :
       case OT_S_ALARM_CO_FAILURE_USER :
       case OT_S_ALARM_DI_SUCCESS_USER :
       case OT_S_ALARM_DI_FAILURE_USER :
       {
           LPCHECKEDOBJECTS obj;
           
           SECURITYALARMPARAMS security;
           ZEROINIT (security);
           level = 0;

           security.lpfirstTableIn = NULL;
           security.iObjectType    = OT_DATABASE;
           security.SecAlarmId     = GetCurSelComplimLong (lpDomData);
           x_strcpy (security.DBName,  parentstrings [0]);
		   if (x_strlen(security.DBName)==0)
               security.iObjectType    = OT_VIRTNODE;               
           obj = ESL_AllocMem (sizeof (CHECKEDOBJECTS));
           if (!obj)
             return FALSE;
           if (objType==OT_STATIC_SECURITY)
             x_strcpy (obj->dbname, GetObjName(lpDomData, dwCurSel));
           else 
             x_strcpy (obj->dbname, parentstrings [0]);
           obj->bchecked   = TRUE;
           obj->pnext      = NULL;
           security.lpfirstTableIn =
                             AddCheckedObject (security.lpfirstTableIn, obj);

           iret = DBADropObject (GetVirtNodeName(GetMDINodeHandle (hwndMdi)),
                   OTLL_SECURITYALARM, (void *) &security);

           security.lpfirstTableIn =
                                FreeCheckedObjects (security.lpfirstTableIn);
           objType=OTLL_DBSECALARM;
       }
       break;


       case OT_INDEX:
       {
           INDEXPARAMS idx;
           ZEROINIT (idx);
           level = 2;

           x_strcpy (idx.DBName     ,parentstrings [0]);
           x_strcpy (idx.objectname ,RemoveDisplayQuotesIfAny(StringWithoutOwner(objName)));
           x_strcpy (idx.objectowner,objOwner);

           iret = DBADropObject (GetVirtNodeName (GetMDINodeHandle (hwndMdi)),
                   OT_INDEX, (void *) &idx);

           objType = OT_INDEX;
       }
       break;


       case OT_TABLE:
       case OT_SCHEMAUSER_TABLE:
       {
           TABLEPARAMS table;
           ZEROINIT (table);
           level = 1;

           // STAR management for table registered as link
           if (lpRecord->parentDbType == DBTYPE_DISTRIBUTED) {
             int  objType = getint(lpRecord->szComplim + STEPSMALLOBJ);
             if (objType == OBJTYPE_STARLINK) {
               iret = RemoveStarRegistration(hwndMdi, parentstrings[0], OT_TABLE, objName);
               objType = OT_TABLE;
               break;
             }
           }

           x_strcpy (table.DBName     , parentstrings [0]);
           x_strcpy (table.szSchema   , objOwner);
           x_strcpy (table.objectname , RemoveDisplayQuotesIfAny(StringWithoutOwner(objName)));

           iret = DBADropObject (GetVirtNodeName (GetMDINodeHandle (hwndMdi)),
                   OT_TABLE, (void *) &table);

           objType = OT_TABLE;

           // STAR management for table that was created at star level
           if (iret == RES_SUCCESS) {
             if (lpRecord->parentDbType == DBTYPE_DISTRIBUTED) {
               int  objType = getint(lpRecord->szComplim + STEPSMALLOBJ);
               if (objType == OBJTYPE_STARNATIVE) {
                  /* FINIR STAR : ATTENTE INFOS FNN
                  DEMASQUER : bMustRefreshStarLocalDB = TRUE;
                  - EN ATTENDANT: supposée sur frpagp57::locale1
                  x_strcpy(szStarLocalNodeName, "frpagp57");
                  x_strcpy(szStarLocalDbName, "locale1");
                  */
               }
             }
           }
       }
       break;



       case OT_VIEW:
       case OT_SCHEMAUSER_VIEW:
       {
           VIEWPARAMS view;
           ZEROINIT (view);
           level = 1;

           // STAR management
           if (lpRecord->parentDbType == DBTYPE_DISTRIBUTED) {
             int  objType = getint(lpRecord->szComplim + 4 + STEPSMALLOBJ);
             if (objType == OBJTYPE_STARLINK) {
               iret = RemoveStarRegistration(hwndMdi, parentstrings[0], OT_VIEW, objName);
               objType = OT_VIEW;
               break;
             }
           }

           x_strcpy (view.DBName,     parentstrings [0]);
           x_strcpy (view.objectname , RemoveDisplayQuotesIfAny(StringWithoutOwner(objName)));
           x_strcpy (view.szSchema   , objOwner);

           iret = DBADropObject (GetVirtNodeName (GetMDINodeHandle (hwndMdi)),
                   OT_VIEW, (void *) &view);

           objType = OT_VIEW;
       }
       break;


       case OT_PROCEDURE:
       case OT_SCHEMAUSER_PROCEDURE:
       {
           PROCEDUREPARAMS proc;
           ZEROINIT (proc);
           level = 1;

           // STAR management
           if (lpRecord->parentDbType == DBTYPE_DISTRIBUTED) {
             iret = RemoveStarRegistration(hwndMdi, parentstrings[0], OT_PROCEDURE, objName);
             objType = OT_PROCEDURE;
             break;
           }

           x_strcpy (proc.DBName,      parentstrings [0]);
           x_strcpy (proc.objectname,  RemoveDisplayQuotesIfAny(StringWithoutOwner(objName)));
           x_strcpy (proc.objectowner, objOwner);

           iret = DBADropObject (GetVirtNodeName (GetMDINodeHandle (hwndMdi)),
                   OT_PROCEDURE, (void *) &proc);

           objType = OT_PROCEDURE;
       }
       break;
       case OT_SEQUENCE:
       {
           SEQUENCEPARAMS sequence;
           ZEROINIT (sequence);
           level = 1;

           x_strcpy (sequence.DBName,      parentstrings [0]);
           x_strcpy (sequence.objectname,  RemoveDisplayQuotesIfAny(StringWithoutOwner(objName)));
           x_strcpy (sequence.objectowner, objOwner);

           iret = DBADropObject (GetVirtNodeName (GetMDINodeHandle (hwndMdi)),
                   OT_SEQUENCE, (void *) &sequence);

           objType = OT_SEQUENCE;
       }
       break;

       case OT_REPLIC_CONNECTION:
       case OT_REPLIC_CONNECTION_V11:
       {
           int locobjtype;
           UCHAR buf[MAXOBJECTNAME];
           REPLCONNECTPARAMS   replconnect;
           ZEROINIT (replconnect);
           level = 1;
           
           // Get information for replication connection
           if (DOMGetObjectLimitedInfo(GetMDINodeHandle(hwndMdi), objName, objOwner,
                        objType,1, parentstrings, GetMDISystemFlag(hwndMdi),
                        &locobjtype,Parents[2], Parents[1],Parents[2])!=RES_SUCCESS) {
              MessageBox (GetFocus(),
                          ResourceString ((UINT)IDS_E_OBJECT_NOT_FOUND),
                          NULL,MB_ICONEXCLAMATION|MB_OK | MB_TASKMODAL);
              return FALSE;
           }
           //read owner's database for get the replicator version
           if (DOMGetObjectLimitedInfo(GetMDINodeHandle(hwndMdi),
                                        parentstrings [0],
                                        "",
                                        OT_DATABASE,
                                        0,
                                        parentstrings,
                                        TRUE,
                                        &locobjtype,
                                        buf,
                                        replconnect.szOwner,
                                        buf)!=RES_SUCCESS) {
              MessageBox (GetFocus(),
                          ResourceString ((UINT)IDS_E_OBJECT_NOT_FOUND),
                          NULL,MB_ICONEXCLAMATION|MB_OK | MB_TASKMODAL);
              return FALSE;
           }
           x_strcpy(replconnect.DBName,parentstrings [0]);
           replconnect.nNumber=getint(Parents[1]);
           
           replconnect.nReplicVersion=GetReplicInstallStatus( lpDomData->psDomNode->nodeHandle,
                                                  parentstrings [0],
                                                  replconnect.szOwner);

           iret = DBADropObject (GetVirtNodeName (GetMDINodeHandle (hwndMdi)),
                                 GetRepObjectType4ll(OT_REPLIC_CONNECTION,
                                                     replconnect.nReplicVersion),
                                  (void *) &replconnect);
           // if iret == RES_ENDOFDATA this is the localdb definition 
           // delete not possible
           if (iret == RES_ENDOFDATA) {
              MessageBox (GetFocus(),
                          ResourceString ((UINT)IDS_E_REPL_CANNOT_DELETE_LOCALDB),
                          NULL,MB_ICONEXCLAMATION|MB_OK | MB_TASKMODAL);
              return FALSE;
           }
           objType = OT_REPLIC_CONNECTION;
       }
       break;
       case OT_REPLIC_MAILUSER:
       {
          REPLMAILPARAMS mail;
          ZEROINIT (mail);
          level = 1;

          x_strcpy (mail.DBName, parentstrings [0]);     /* Database    */
          x_strcpy (mail.szMailText, objName);          

          iret = DBADropObject (GetVirtNodeName (GetMDINodeHandle (hwndMdi)),
                   objType, (void *) &mail);

       }    
       break;
       case OT_REPLIC_CDDS:
       {
          int     locobjtype;
          UCHAR   buf[MAXOBJECTNAME];
          REPCDDS cdds;
          LPUCHAR lpVirtNode = GetVirtNodeName (GetMDINodeHandle (hwndMdi));
          int     SesHndl;          // current session handle
          int     replicver;

          ZEROINIT (cdds);
          level = 1;

          //read database ower to get the replicator version
          if (DOMGetObjectLimitedInfo(GetMDINodeHandle(hwndMdi),
                                       parentstrings [0],
                                       "",
                                       OT_DATABASE,
                                       0,
                                       parentstrings,
                                       TRUE,
                                       &locobjtype,
                                       buf,
                                       cdds.DBOwnerName,
                                       buf)!=RES_SUCCESS) {
             MessageBox (GetFocus(),
                         ResourceString ((UINT)IDS_E_OBJECT_NOT_FOUND),
                         NULL,MB_ICONEXCLAMATION|MB_OK | MB_TASKMODAL);
             return FALSE;
          }

          replicver=GetReplicInstallStatus( lpDomData->psDomNode->nodeHandle,
                                                 parentstrings [0],
                                                 cdds.DBOwnerName);

          x_strcpy (cdds.DBName, parentstrings [0]);     /* Database    */
          cdds.cdds= my_atoi(objName);
          cdds.bAlter=TRUE;

          switch (replicver) {
             case REPLIC_V10:
             case REPLIC_V105:
                iret = GetDetailInfo (lpVirtNode, OT_REPLIC_CDDS, &cdds, FALSE, &SesHndl);
                if (iret != RES_SUCCESS)
                   return FALSE;
                cdds.iReplicType=replicver;
                iret = DBADropObject (lpVirtNode,
                                      objType, (void *) &cdds);
                break;
             case REPLIC_V11:
                iret = DBADropObject (lpVirtNode,
                                      OT_REPLIC_CDDS_V11, (void *) &cdds);
                break;
             default:
                myerror(ERR_REPLICTYPE);
                break;
          }
                             
       }    
       break;


       case OT_TABLEGRANT_SEL_USER :
       case OT_TABLEGRANT_INS_USER :
       case OT_TABLEGRANT_UPD_USER :
       case OT_TABLEGRANT_DEL_USER :
       case OT_TABLEGRANT_CPI_USER :
       case OT_TABLEGRANT_CPF_USER :
       case OT_TABLEGRANT_REF_USER :
       case OT_TABLEGRANT_ALL_USER :
       case OT_TABLEGRANT_INDEX_USER:
       case OT_TABLEGRANT_ALTER_USER:
       case OT_DBEGRANT_RAISE_USER :
       case OT_DBEGRANT_REGTR_USER :
       case OT_PROCGRANT_EXEC_USER :
       case OT_VIEWGRANT_SEL_USER  :
       case OT_VIEWGRANT_INS_USER  :
       case OT_VIEWGRANT_UPD_USER  :
       case OT_VIEWGRANT_DEL_USER  :
           objType = OTLL_GRANTEE;
           level   = 1;
           break; // handled in previous switch

       case OT_DBGRANT_ACCESY_USER :
       case OT_DBGRANT_ACCESN_USER :
       case OT_DBGRANT_CREPRY_USER :
       case OT_DBGRANT_CREPRN_USER :
       case OT_DBGRANT_CRETBY_USER :
       case OT_DBGRANT_CRETBN_USER :
       case OT_DBGRANT_DBADMY_USER :
       case OT_DBGRANT_DBADMN_USER :
       case OT_DBGRANT_LKMODY_USER :
       case OT_DBGRANT_LKMODN_USER :
       case OT_DBGRANT_QRYIOY_USER :
       case OT_DBGRANT_QRYION_USER :
       case OT_DBGRANT_QRYRWY_USER :
       case OT_DBGRANT_QRYRWN_USER :
       case OT_DBGRANT_UPDSCY_USER :
       case OT_DBGRANT_UPDSCN_USER :
       case OT_DBGRANT_SELSCY_USER: 
       case OT_DBGRANT_SELSCN_USER: 
       case OT_DBGRANT_CNCTLY_USER: 
       case OT_DBGRANT_CNCTLN_USER: 
       case OT_DBGRANT_IDLTLY_USER: 
       case OT_DBGRANT_IDLTLN_USER: 
       case OT_DBGRANT_SESPRY_USER: 
       case OT_DBGRANT_SESPRN_USER: 
       case OT_DBGRANT_TBLSTY_USER:
       case OT_DBGRANT_TBLSTN_USER:

       case OT_DBGRANT_QRYCPY_USER: 
       case OT_DBGRANT_QRYCPN_USER: 
       case OT_DBGRANT_QRYPGY_USER: 
       case OT_DBGRANT_QRYPGN_USER: 
       case OT_DBGRANT_QRYCOY_USER: 
       case OT_DBGRANT_QRYCON_USER: 
       case OT_DBGRANT_SEQCRN_USER:
       case OT_DBGRANT_SEQCRY_USER:

              objType = OTLL_DBGRANTEE;
              level   = 0;
           break; // handled in previous switch


       case OT_ROLEGRANT_USER:
           objType = OT_ROLEGRANT_USER;
           level   = 1;
           break; // handled in previous switch

       case OT_ICE_ROLE: 
		{
			ICEROLEDATA roledta;
			x_strcpy(roledta.RoleName,objName);
			level = 0;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&roledta);
		}
		break;
       case OT_ICE_DBUSER:
		{
			ICEDBUSERDATA dbuser;
			x_strcpy(dbuser.UserAlias,objName);
			level = 0;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&dbuser);
		}
		break;
       case OT_ICE_DBCONNECTION:
		{
			DBCONNECTIONDATA dbconnection;
			x_strcpy(dbconnection.ConnectionName,objName);
			level = 0;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&dbconnection);
		}
		break;
       case OT_ICE_WEBUSER:
		{
			ICEWEBUSERDATA webuser;
			x_strcpy(webuser.UserName,objName);
			level = 0;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&webuser);
		}
		break;
       case OT_ICE_PROFILE:
		{
			ICEPROFILEDATA profile;
			x_strcpy(profile.ProfileName,objName);
			level = 0;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&profile);
		}
		break;
       case OT_ICE_WEBUSER_ROLE:
		{
			ICEWEBUSERROLEDATA webuserrole;
			x_strcpy(webuserrole.icewebuser.UserName,parentstrings [0]);
			x_strcpy(webuserrole.icerole.RoleName   ,objName);
			level = 1;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&webuserrole);
		}
		break;
       case OT_ICE_WEBUSER_CONNECTION:
		{
			ICEWEBUSERCONNECTIONDATA webuserconnection;
			x_strcpy(webuserconnection.icewebuser.UserName,parentstrings [0]);
			x_strcpy(webuserconnection.icedbconnection.ConnectionName,objName);
			level = 1;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&webuserconnection);
		}
		break;
       case OT_ICE_PROFILE_ROLE:
		{
			ICEPROFILEROLEDATA profilerole;
			x_strcpy(profilerole.iceprofile.ProfileName,parentstrings [0]);
			x_strcpy(profilerole.icerole.RoleName,objName);
			level = 1;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&profilerole);
		}
		break;
       case OT_ICE_PROFILE_CONNECTION:
		{
			ICEPROFILECONNECTIONDATA profileconnect;
			x_strcpy(profileconnect.iceprofile.ProfileName,parentstrings [0]);
			x_strcpy(profileconnect.icedbconnection.ConnectionName,objName);
			level = 1;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&profileconnect);
		}
		break;
       case OT_ICE_BUNIT:
		{
			ICESBUSUNITDATA busunit;
			x_strcpy(busunit.Name,objName);
			level = 0;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&busunit);
		}
		break;
       case OT_ICE_BUNIT_SEC_ROLE:
		{
			ICEBUSUNITROLEDATA busunitrole;
			x_strcpy(busunitrole.icebusunit.Name,parentstrings [0]);
			x_strcpy(busunitrole.icerole.RoleName,objName);
			level = 1;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&busunitrole);
		}
		break;
       case OT_ICE_BUNIT_SEC_USER:
		{
			ICEBUSUNITWEBUSERDATA busunituser;
			x_strcpy(busunituser.icebusunit.Name,parentstrings [0]);
			x_strcpy(busunituser.icewevuser.UserName,objName);
			level = 1;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&busunituser);
		}
		break;
       case OT_ICE_BUNIT_FACET:
       case OT_ICE_BUNIT_PAGE:
		{
			ICEBUSUNITDOCDATA busunitdoc;
			x_strcpy(busunitdoc.icebusunit.Name,parentstrings [0]);
			x_strcpy(busunitdoc.name,objName);
			x_strcpy(busunitdoc.suffix,"");
			level = 1;
			if (objType==OT_ICE_BUNIT_FACET)
				busunitdoc.bFacet=TRUE;
			else
				busunitdoc.bFacet=FALSE;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&busunitdoc);
		}
		break;
       case OT_ICE_BUNIT_FACET_ROLE:
       case OT_ICE_BUNIT_PAGE_ROLE:
		{
			ICEBUSUNITDOCROLEDATA data;
			char bufx[100];
			char * pc;
			x_strcpy(data.icebusunitdoc.icebusunit.Name,parentstrings [0]);
			x_strcpy(bufx,parentstrings[1]);
			pc = x_strchr(bufx,'.');
			if (pc) {
				*pc='\0';
				x_strcpy(data.icebusunitdoc.suffix,pc+1);
			}
			else 
				x_strcpy(data.icebusunitdoc.suffix,"");
			x_strcpy(data.icebusunitdoc.name,bufx); 
			x_strcpy(data.icebusunitdocrole.RoleName,objName);
			level = 2;
			if (objType==OT_ICE_BUNIT_FACET_ROLE)
				data.icebusunitdoc.bFacet=TRUE;
			else
				data.icebusunitdoc.bFacet=FALSE;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&data);
		}
		break;
       case OT_ICE_BUNIT_FACET_USER:
       case OT_ICE_BUNIT_PAGE_USER:
		{
			ICEBUSUNITDOCUSERDATA data;
			char bufx[100];
			char * pc;
			x_strcpy(data.icebusunitdoc.icebusunit.Name,parentstrings [0]);
			x_strcpy(bufx,parentstrings[1]);
			pc = x_strchr(bufx,'.');
			if (pc) {
				*pc='\0';
				x_strcpy(data.icebusunitdoc.suffix,pc+1);
			}
			else 
				x_strcpy(data.icebusunitdoc.suffix,"");
			x_strcpy(data.icebusunitdoc.name,bufx); 
			x_strcpy(data.user.UserName,objName);
			level = 2;
			if (objType==OT_ICE_BUNIT_FACET_USER)
				data.icebusunitdoc.bFacet=TRUE;
			else
				data.icebusunitdoc.bFacet=FALSE;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&data);
		}
		break;
       case OT_ICE_BUNIT_LOCATION:
		{
			ICEBUSUNITLOCATIONDATA busunitlocation;
			x_strcpy(busunitlocation.icebusunit.Name,parentstrings [0]);
			x_strcpy(busunitlocation.icelocation.LocationName,objName);
			level = 1;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&busunitlocation);
		}
		break;
       case OT_ICE_SERVER_APPLICATION:
		{
			ICEAPPLICATIONDATA application;
			x_strcpy(application.Name,objName);
			level = 0;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&application);
		}
		break;
       case OT_ICE_SERVER_LOCATION:
		{
			ICELOCATIONDATA location;
			x_strcpy(location.LocationName,objName);
			level = 0;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&location);
		}
		break;
        case OT_ICE_SERVER_VARIABLE:
		{
			ICESERVERVARDATA serverdta;
			x_strcpy(serverdta.VariableName,objName);
			level = 0;
			iret=ICEDropobject(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
				              objType,(void *)&serverdta);
		}
		break;
 
        case OT_SEQUGRANT_NEXT_USER:
                {
                   level = 2;
                }
                break;

       default:
       MessageBox(GetFocus(),ResourceString ((UINT)IDS_I_DROP_NOTDEFINED),
                  NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
       return FALSE;
   }

   if (iret!=RES_SUCCESS) {
       ErrorMessage ((UINT)IDS_E_DROP_OBJECT_FAILED, iret);    // Drop object Failed
       return FALSE;
   }

   //
   // Step 2: calls to UpdateDOMData and DomUpdateDisplayDataAllUsers
   // to refresh internal data and display,
   // since we have dropped an object
   //

   DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, NULL);
       
   switch (objType)
   {
       case OT_STATIC_SECURITY: /* covers all sec.alarm #defines */

           iter = 1; /* all 8 alarms are read at once in the cache,
                        and 8 displays are made in DomUpdateDisplayDataAllUsers */
           aObjType[0] = OT_S_ALARM_SELSUCCESS_USER;
           parentstrings[1]=NULL; /* Sec.Alarm may apply to multiple tables */
           objType=OTLL_SECURITYALARM;
           aObjType[0] = OTLL_SECURITYALARM;
           alevel[0]=1;
           break;

       case OT_TABLE:
           // Drop of a table should also update the synonyms
           // for the database on which the table is

           // Added Restricted feature if gateway: no synonyms
           // Added star restriction: no synonyms either
           if (bMustRefreshStarLocalDB
               || GetOIVers()==OIVERS_NOTOI
               || (IsStarDatabase(lpDomData->psDomNode->nodeHandle, parentstrings[0]))
               ) {
             // parentstrings [0] is already fine
             iter = 2;
             aObjType[0] = OT_TABLE;
             aObjType[1] = OT_VIEW;
             alevel[0] = 1;
             alevel[1] = 1;
           }
           else {
             iter = 3;
             aObjType[0] = OT_TABLE;
             aObjType[1] = OT_VIEW;
             aObjType[2] = OT_SYNONYM;
             alevel[0] = 1;
             alevel[1] = 1;
             alevel[2] = 1;
             // parentstrings [0] is already fine
           }
           break;

       case OT_VIEW:
           // Drop of a view should also update the synonyms
           // for the database on which the view is

           // Added Restricted feature if gateway: no synonyms
           // Added star restriction: no synonyms either
           if ( GetOIVers()==OIVERS_NOTOI || 
			    (IsStarDatabase(lpDomData->psDomNode->nodeHandle, parentstrings[0]))
              ) {
             iter = 1;
             aObjType[0] = OT_VIEW;
             alevel[0] = 1;
             // parentstrings [0] is already fine
           }
           else {
             iter = 2;
             aObjType[0] = OT_VIEW;
             aObjType[1] = OT_SYNONYM;
             alevel[0] = 1;
             alevel[1] = 1;
             // parentstrings [0] is already fine
           }
           break;

       case OT_INDEX:
           // Drop of a index should also update the synonyms
           // for the database on which the index is
           iter = 2;
           aObjType[0] = OT_INDEX;
           aObjType[1] = OT_SYNONYM;
           alevel[0] = 2;   // database + table
           alevel[1] = 1;   // database only
           // parentstrings [0] and [1] are already fine
           break;

       case OT_USER:
           // Drop of a user: update all groups (they may contain the user)
           // Note : the grants remain, so OTLL_DBGRANTEE not used,
           // as opposed to user creation.
           iter = 3;
           aObjType[0] = objType;
           aObjType[1] = OT_GROUP; // avoids error system 4 for GROUPUSER
           aObjType[2] = OT_GROUPUSER;
           alevel[0] = 0;
           alevel[1] = 0;
           alevel[2] = 1;
           parentstrings[0] = NULL;   // all groups
           break;

       case OT_REPLIC_CDDS:
           // Drop of a cdds: update registered tables
           iter = 3;
           aObjType[0] = objType;
           aObjType[1] = OT_REPLIC_REGTABLE;
           aObjType[2] = OT_TABLE;
           alevel[0] = 1;
           alevel[1] = 1;
           alevel[2] = 1;
           // parentstrings [0] already set
           break;

       case OT_ICE_BUNIT_PAGE_USER:
       case OT_ICE_BUNIT_FACET_USER:
           iter = 2;
           aObjType[0] = objType;
           aObjType[1] = OT_ICE_BUNIT_SEC_USER;
           alevel[0]=level;
           alevel[1]=1;
           break;

       case OT_ICE_WEBUSER:
           iter = 2;
           parentstrings [0] = NULL;

           aObjType[0] = objType;
           aObjType[1] = OT_ICE_BUNIT;
           alevel[0]=level;
           alevel[1]=0;
           aWithChildren[0] = FALSE;
           aWithChildren[1] = TRUE;
           break;

       default:
           iter = 1;
           aObjType[0] = objType;
           alevel[0]=level;
           break;
   }

   for (cpt=0; cpt<iter; cpt++)
       UpdateDOMDataAllUsers(
           lpDomData->psDomNode->nodeHandle, // hnodestruct
           aObjType[cpt],                    // iobjecttype
           alevel[cpt],                      // level
           parentstrings,                    // parentstrings
           FALSE,                            // keep system tbls state
           aWithChildren[cpt]);              // bWithChildren

   if ( objType == OT_ICE_WEBUSER)
   {
       DOMUpdateDisplayAllDataAllUsers(lpDomData->psDomNode->nodeHandle);
   }
   else
   {
       for (cpt=0; cpt<iter; cpt++) {
          DOMUpdateDisplayDataAllUsers (
           lpDomData->psDomNode->nodeHandle, // hnodestruct
           aObjType[cpt],                    // iobjecttype
           alevel[cpt],                      // level
           parentstrings,                    // parentstrings
           FALSE,                            // bWithChildren
           ACT_DROP_OBJECT,                  // object dropped
           0L,                               // no item id
           0);                               // all mdi windows concerned
           if (aObjType[cpt]==OT_DBEVENT)
             UpdateDBEDisplay(lpDomData->psDomNode->nodeHandle, parentstrings[0]);
       }
   }
   DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, NULL);

   // For star : potential refresh of tables list on local db / local node
   // also, refresh views since can have been defined on local table
   if (bMustRefreshStarLocalDB) {
     starNodeHandle = GetVirtNodeHandle(szStarLocalNodeName);
     if (starNodeHandle >= 0) {
       int aTypes[2] = { OT_TABLE, OT_VIEW};
       int  cpt;
       tempParentstrings0 = parentstrings[0];
       parentstrings[0] = szStarLocalDbName;
       for (cpt=0; cpt< sizeof(aTypes)/sizeof(aTypes[0]) ; cpt++) {
         UpdateDOMDataDetailAllUsers(starNodeHandle,
                             aTypes[cpt],
                             1,
                             parentstrings,
                             FALSE,
                             FALSE,
                             TRUE,      // bOnlyIfExists
                             FALSE,
                             FALSE);
         DOMUpdateDisplayDataAllUsers (starNodeHandle,
                               aTypes[cpt],
                               1,
                               parentstrings,
                               FALSE,
                               ACT_DROP_OBJECT,
                               0L,
                               0);
       }
       parentstrings[0] = tempParentstrings0;
     }
   }


   // Fix Nov 4, 97: must Update number of objects after an add
   // (necessary since we have optimized the function that counts the tree items
   // for performance purposes)
   GetNumberOfObjects(lpDomData, TRUE);
   // display will be updated at next call to the same function

   return TRUE;
}



VOID DomGrantObject (HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam)
{
   int       objType;          // current selection type
   UCHAR     objName[MAXOBJECTNAME];
   UCHAR     objOwner[MAXOBJECTNAME];
   DWORD     dwCurSel;
   int       i,iret;             // return value of the dialog box
   int       level;
   LPUCHAR   parentstrings [MAXPLEVEL];
   UCHAR     Parents[MAXPLEVEL][MAXOBJECTNAME];
   LPTREERECORD lpRecord;

   int iter;
   int aObjType[10];   // CAREFUL WITH THIS MAX VALUE!
   int cpt;

   // dummy instructions to skip compiler warnings at level four
   wParam;
   lParam;

   objType = GetCurSelUnsolvedItemObjType (lpDomData);
   dwCurSel=(DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
   if (!dwCurSel)
       return;   // How did we happen here ?
   fstrncpy(objName,GetObjName(lpDomData, dwCurSel),MAXOBJECTNAME);

   lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                               LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
   fstrncpy(objOwner,lpRecord->ownerName,MAXOBJECTNAME);

   if (!GetCurSelObjParents (lpDomData, parentstrings))
       return;       // TESTER RETVAL
   for (i=0;i<3;i++) {
       fstrncpy(Parents[i],parentstrings[i],MAXOBJECTNAME);
       parentstrings[i]=Parents[i];
   }

   switch (objType)
   {
       case OT_TABLE:
       case OT_SCHEMAUSER_TABLE:
       {
           GRANTPARAMS g;
           ZEROINIT (g);

           g.ObjectType   = OT_TABLE;
           x_strcpy (g.DBName,          parentstrings [0]);   /* Database    */ 
           x_strcpy (g.PreselectObject, objName);             /* Table       */
           x_strcpy (g.PreselectObjectOwner,objOwner);        /* Table owner */
                   iret = DlgGrantTable (hwndMdi, &g);
           if (!iret)
               return;
       }
       objType = OTLL_GRANTEE;
       level   = 1;
       break;

       case OT_VIEW:
       case OT_SCHEMAUSER_VIEW:
       {
           GRANTPARAMS g;
           ZEROINIT (g);
           g.ObjectType   = OT_VIEW;
           x_strcpy (g.DBName,          parentstrings [0]);   /* Database    */ 
           x_strcpy (g.PreselectObject, objName);             /* View        */
           x_strcpy (g.PreselectObjectOwner,objOwner);        /* View owner  */
        
           iret = DlgGrantTable (hwndMdi, &g);
           if (!iret)
               return;
       }
       objType = OTLL_GRANTEE;
       level   = 1;
       break;

       case OT_PROCEDURE:
       case OT_SCHEMAUSER_PROCEDURE:
       {
           GRANTPARAMS g;
           ZEROINIT (g);
           
           g.ObjectType   = OT_PROCEDURE;
           g.Privileges [GRANT_EXECUTE] = TRUE;

           x_strcpy (g.DBName,           parentstrings [0]);  /* Database    */
           x_strcpy (g.PreselectObject,  objName);            /* Procedure   */
           x_strcpy (g.PreselectObjectOwner,objOwner);        /* Proc  owner */
           iret = DlgGrantProcedure (hwndMdi, &g);             
           if (iret != IDOK)
               return;
       }
       objType = OTLL_GRANTEE;
       level   = 1;
       break;

       case OT_SEQUENCE:
       {
           GRANTPARAMS g;
           ZEROINIT (g);
           
           g.ObjectType   = OT_SEQUENCE;
           g.Privileges [GRANT_NEXT_SEQUENCE] = TRUE;

           x_strcpy (g.DBName,           parentstrings [0]);  /* Database    */
           x_strcpy (g.PreselectObject,  objName);            /* Procedure   */
           x_strcpy (g.PreselectObjectOwner,objOwner);        /* Proc  owner */
           iret = DlgGrantProcedure (hwndMdi, &g);
           if (iret != IDOK)
               return;
       }
       objType = OTLL_GRANTEE;
       level   = 1;
       break;

       case OT_DBEVENT:
       {
           GRANTPARAMS g;
           ZEROINIT (g);

           g.ObjectType   = OT_DBEVENT;
           x_strcpy (g.DBName,           parentstrings [0]);  /* Database     */
           x_strcpy (g.PreselectObject,  objName);            /* DBevent name */
           x_strcpy (g.PreselectObjectOwner,objOwner);    /* DBEvent owner*/

           iret = DlgGrantDBevent (hwndMdi, &g);
           if (!iret)
               return;
       }
       objType = OTLL_GRANTEE;
       level   = 1;
       break;

       case OT_DATABASE:
       {
           GRANTPARAMS g;
           ZEROINIT (g);

           g.ObjectType   = OT_DATABASE;
           x_strcpy (g.DBName,           objName);
           x_strcpy (g.PreselectObject,  objName);

           iret = DlgGrantDatabase (hwndMdi, &g);
           if (!iret)
               return;
       }
       objType = OTLL_DBGRANTEE;
       level   = 0;
       break;

       case OT_ROLE:
       {
           GRANTPARAMS g;
           ZEROINIT   (g);

           x_strcpy (g.PreselectObject, objName);  /* Preselect role */
           iret = DlgGrantRole2User (hwndMdi, &g);
           if (!iret)
               return;
       }
       parentstrings[0]=objName;
       objType = OT_ROLEGRANT_USER;
       level   = 1;
       break;


       default:
       {
           char message [100];
           //
           // Grant sys err: id = %d
           //
           wsprintf (message, ResourceString ((UINT)IDS_E_GRANT_SYS_ERROR), objType);
           MessageBox (GetFocus(), message, NULL,
                       MB_ICONEXCLAMATION|MB_OK | MB_TASKMODAL);
           return;
       }
   }

   DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, NULL);

   switch (objType)
   {
       default:
           iter = 1;
           aObjType[0] = objType;
           break;
   }

   for (cpt=0; cpt<iter; cpt++)
       UpdateDOMDataAllUsers(
            lpDomData->psDomNode->nodeHandle, // hnodestruct
            aObjType[cpt],                    // iobjecttype
            level,                            // level
            parentstrings,                    // parentstrings
            FALSE,                            // keep system tbls state
            FALSE);                           // bWithChildren

   for (cpt=0; cpt<iter; cpt++)
       DOMUpdateDisplayDataAllUsers (
            lpDomData->psDomNode->nodeHandle, // hnodestruct
            aObjType[cpt],                    // iobjecttype
            level,                            // level
            parentstrings,                    // parentstrings
            FALSE,                            // bWithChildren
            ACT_ADD_OBJECT,                   // object added
            0L,                               // no item id
            0);                               // all mdi windows concerned

   // complimentary management of right pane: refresh it immediately
   UpdateRightPane(hwndMdi, lpDomData, TRUE, 0);

   DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, NULL);
}


VOID DomRevokeObject (HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam)
{
   int     objType;          // current selection type
   UCHAR   objName[MAXOBJECTNAME];
   UCHAR   objOwner[MAXOBJECTNAME];
   DWORD   dwCurSel;
   int     i,iret;             // return value of the dialog box
   int     level;
   LPUCHAR parentstrings [MAXPLEVEL];
   UCHAR   Parents[MAXPLEVEL][MAXOBJECTNAME];
   LPTREERECORD lpRecord;
   int     iter;
   int     aObjType[10];   // CAREFUL WITH THIS MAX VALUE!
   int     cpt;

   // dummy instructions to skip compiler warnings at level four
   wParam;
   lParam;

   objType = GetCurSelUnsolvedItemObjType (lpDomData);
   dwCurSel=(DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
   if (!dwCurSel)
       return;   // How did we happen here ?
   fstrncpy(objName,GetObjName(lpDomData, dwCurSel),MAXOBJECTNAME);

   lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                               LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
   fstrncpy(objOwner,lpRecord->ownerName,MAXOBJECTNAME);
   if (!GetCurSelObjParents (lpDomData, parentstrings))
       return;       // TESTER RETVAL
   for (i=0;i<3;i++) {
       fstrncpy(Parents[i],parentstrings[i],MAXOBJECTNAME);
       parentstrings[i]=Parents[i];
   }

   switch (objType)
   {
       case OT_TABLE:
       case OT_SCHEMAUSER_TABLE:
       {
           REVOKEPARAMS r;
           ZEROINIT (r);

           r.ObjectType = OT_TABLE;
           x_strcpy (r.DBName,          parentstrings [0]);   /* Database    */
           x_strcpy (r.PreselectObject, RemoveDisplayQuotesIfAny(StringWithoutOwner(objName)));             /* Table       */
           x_strcpy (r.PreselectObjectOwner,objOwner);    /* Table owner */
        
           iret = DlgRvkgtab(hwndMdi, &r);
           if (!iret)
               return;
       }
       objType = OTLL_GRANTEE;
       level   = 1;
       break;

       case OT_VIEW:
       case OT_SCHEMAUSER_VIEW:
       {
           REVOKEPARAMS r;
           ZEROINIT (r);

           r.ObjectType = OT_VIEW;
           x_strcpy (r.DBName,          parentstrings [0]);   /* Database    */
           x_strcpy (r.PreselectObject, RemoveDisplayQuotesIfAny(StringWithoutOwner(objName)));             /* view        */
           x_strcpy (r.PreselectObjectOwner,objOwner);        /* View owner  */
        
           iret = DlgRvkgview (hwndMdi, &r);
           if (!iret)
               return;
       }
       objType = OTLL_GRANTEE;
       level   = 1;
       break;

       case OT_PROCEDURE:
       case OT_SCHEMAUSER_PROCEDURE:
       {
           REVOKEPARAMS revoke;
           ZEROINIT    (revoke);

           revoke.ObjectType   = OT_PROCEDURE;

           x_strcpy (revoke.DBName,          parentstrings [0]); // Database name
           x_strcpy (revoke.PreselectObject, RemoveDisplayQuotesIfAny(StringWithoutOwner(objName)));
           x_strcpy(revoke.PreselectObjectOwner,objOwner);    /* Proc  owner */
           revoke.PreselectPrivileges [GRANT_EXECUTE]    = TRUE;

           iret = DlgRevokeOnTable (hwndMdi, &revoke);
           if (!iret)
               return;
       }
       objType = OTLL_GRANTEE;
       level   = 1;
       break;

       case OT_SEQUENCE:
       {
           REVOKEPARAMS revoke;
           ZEROINIT    (revoke);

           revoke.ObjectType   = OT_SEQUENCE;

           x_strcpy (revoke.DBName,          parentstrings [0]); // Database name
           x_strcpy (revoke.PreselectObject, RemoveDisplayQuotesIfAny(StringWithoutOwner(objName)));
           x_strcpy(revoke.PreselectObjectOwner,objOwner);    /* Proc  owner */
           revoke.PreselectPrivileges [GRANT_NEXT_SEQUENCE]    = TRUE;

           iret = DlgRevokeOnTable (hwndMdi, &revoke);
           if (!iret)
               return;
       }
       objType = OTLL_GRANTEE;
       level   = 1;
       break;

       case OT_DBEVENT:
       {
           REVOKEPARAMS r;
           ZEROINIT (r);

           x_strcpy (r.DBName,          parentstrings [0]);   /* Database     */
           x_strcpy (r.PreselectObject, RemoveDisplayQuotesIfAny(StringWithoutOwner(objName)));             /* DBE name     */
           x_strcpy (r.PreselectObjectOwner,objOwner);        /* DBEvent owner*/
        
           iret = DlgRvkgdbe (hwndMdi, &r);
           if (!iret)
               return;
       }
       objType = OTLL_GRANTEE;
       level   = 1;
       break;

       case OT_DATABASE:
       {
           REVOKEPARAMS r;
           ZEROINIT (r);

           r.ObjectType = OT_TABLE;
           x_strcpy (r.DBName,          objName);            /* Database    */
           
           iret = DlgRvkgdb (hwndMdi, &r);
           if (!iret)
               return;
       }
       objType = OTLL_DBGRANTEE;
       level   = 0;
       break;

       case OT_ROLE:
       {
           REVOKEPARAMS r;
           ZEROINIT (r);

           x_strcpy (r.PreselectObject,  objName);
           iret = DlgRvkgrol (hwndMdi, &r);
           parentstrings[0]=objName;
           if (!iret)
               return;
       }
       objType = OT_ROLEGRANT_USER;
       level   = 1;
       break;
       


       default:
       {
           char message [100];
           //
           // Revoke sys err: id = %d
           //
           wsprintf (message, ResourceString ((UINT)IDS_E_REVOKE_SYS_ERROR), objType);
           MessageBox( GetFocus(), message, NULL,
                       MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
           return;
           break;
       }
   }

   DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, NULL);

   switch (objType)
   {
       default:
           iter = 1;
           aObjType[0] = objType;
           break;
   }

   for (cpt=0; cpt<iter; cpt++)
       UpdateDOMDataAllUsers(
            lpDomData->psDomNode->nodeHandle, // hnodestruct
            aObjType[cpt],                    // iobjecttype
            level,                            // level
            parentstrings,                    // parentstrings
            FALSE,                            // keep system tbls state
            FALSE);                           // bWithChildren

   for (cpt=0; cpt<iter; cpt++)
       DOMUpdateDisplayDataAllUsers (
            lpDomData->psDomNode->nodeHandle, // hnodestruct
            aObjType[cpt],                    // iobjecttype
            level,                            // level
            parentstrings,                    // parentstrings
            FALSE,                            // bWithChildren
            ACT_ADD_OBJECT,                   // object added
            0L,                               // no item id
            0);                               // all mdi windows concerned

   // complimentary management of right pane: refresh it immediately
   UpdateRightPane(hwndMdi, lpDomData, TRUE, 0);

   DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, NULL);
}


VOID DomModifyObjectStruct (HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam)
{
   int     SesHndl;          // current session handle
   int     objType;          // current selection type
   UCHAR   objName[MAXOBJECTNAME];
   UCHAR   objOwner[MAXOBJECTNAME];
   DWORD   dwCurSel;
   int     i,iret;             // return value of the dialog box
   int     level;
   LPUCHAR parentstrings [MAXPLEVEL];
   LPUCHAR lpVirtNode;
   UCHAR   Parents[MAXPLEVEL][MAXOBJECTNAME];
   UCHAR   ParentTableOwner[MAXOBJECTNAME];
   int     iter;
   int     aObjType[10];   // CAREFUL WITH THIS MAX VALUE!
   int     cpt;
   LPTREERECORD  lpRecord;
   FINDCURSOR findcursor;
   TCHAR tchszNode[256];
   BOOL bHasGWSuffix;
   TCHAR tchszGateway[200];
   UINT nExistOpenCursor = 0;
   REQUESTMDIINFO* pArrayInfo = NULL;
   int nInfoSize = 0;
   BOOL bDoModify = TRUE;

   // dummy instructions to skip compiler warnings at level four
   wParam;
   lParam;

   objType = GetCurSelUnsolvedItemObjType (lpDomData);
   dwCurSel=(DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
   if (!dwCurSel)
       return;   // How did we happen here ?
   fstrncpy(objName,GetObjName(lpDomData, dwCurSel),MAXOBJECTNAME);

   lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                               LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

   fstrncpy(objOwner,lpRecord->ownerName,MAXOBJECTNAME);
   fstrncpy(ParentTableOwner,lpRecord->tableOwner,MAXOBJECTNAME);

   if (!GetCurSelObjParents (lpDomData, parentstrings))
       return;       // TESTER RETVAL
   for (i=0;i<3;i++) {
       fstrncpy(Parents[i],parentstrings[i],MAXOBJECTNAME);
       parentstrings[i]=Parents[i];
   }

   lpVirtNode = GetVirtNodeName(lpDomData->psDomNode->nodeHandle);

   bHasGWSuffix = GetGWClassNameFromString(lpVirtNode, (LPUCHAR)tchszGateway);
   lstrcpy (tchszNode, lpVirtNode);
   RemoveGWNameFromString((LPUCHAR)tchszNode);
   RemoveConnectUserFromString((LPUCHAR)tchszNode);

   memset(&findcursor, 0, sizeof(findcursor));
   findcursor.bCloseCursor = FALSE;
   lstrcpy (findcursor.tchszNode,       (LPCTSTR)tchszNode);
   lstrcpy (findcursor.tchszServer,     (LPCTSTR)tchszGateway);
   lstrcpy (findcursor.tchszDatabase,   (LPCTSTR)lpRecord->extra);
   lstrcpy (findcursor.tchszTable,      (LPCTSTR)RemoveDisplayQuotesIfAny(StringWithoutOwner (lpRecord->objName)));
   lstrcpy (findcursor.tchszTableOwner, (LPCTSTR)lpRecord->ownerName);
   nExistOpenCursor = GetExistOpenCursor (&findcursor, &pArrayInfo, &nInfoSize);

   if (nExistOpenCursor != 0 && nInfoSize > 0 && pArrayInfo != NULL)
   {
       int i = 0;
       HWND hParent = GetParent(hwndMdi);
       if (hParent)
           hParent = GetParent(hParent);
       ASSERT (hParent);
        nExistOpenCursor = 0;
       for (i=0; i<nInfoSize; i++)
       {
           if (pArrayInfo[i].hWndMDI == hParent)
               continue;
           if (pArrayInfo[i].nInfo == OPEN_CURSOR_SQLACT)
               nExistOpenCursor |= OPEN_CURSOR_SQLACT;
           else
           if (pArrayInfo[i].nInfo == OPEN_CURSOR_DOM)
               nExistOpenCursor |= OPEN_CURSOR_DOM;
       }

       if (nInfoSize == 1 && hParent == pArrayInfo->hWndMDI)
       {
           //
           // Only the current DOM has the opened cursor, just close the cursor
           // and continue the modify structure:
           findcursor.bCloseCursor = TRUE;
           SendMessage (pArrayInfo->hWnd, GetUserMessageQueryOpenCursor(), 0, (LPARAM)&findcursor);
           bDoModify = TRUE;
       }
       else
       if ((nExistOpenCursor & OPEN_CURSOR_SQLACT) && (nExistOpenCursor & OPEN_CURSOR_DOM))
       {
           //
           // There exist DOM and SQLTest Windows having the opened cursor on the table:
           // MSG: Unable to launch the Modify Structure Dialog, because both SQL/Test and other DOM windows have sessions open on the table.\n
           //      Please close the SQL/Test and DOM sessions to perform the Modify Structure.
           MessageBox(hwndMdi , ResourceString ((UINT)IDS_MODIFY_STRUCTURExOPENCURSOR_SQLTESTxDOM), NULL, MB_ICONEXCLAMATION | MB_OK);
           bDoModify = FALSE;
       }
       else
       if ((nExistOpenCursor & OPEN_CURSOR_DOM))
       {
           //
           // Other DOM Window has an opened cursor on the table:
           // MSG: Unable to launch the Modify Structure Dialog, because a DOM window has a session open on the table.\n 
           //      Please close the DOM session to perform the Modify Structure.
           MessageBox(hwndMdi , ResourceString ((UINT)IDS_MODIFY_STRUCTURExOPENCURSOR_DOM), NULL, MB_ICONEXCLAMATION | MB_OK);
           bDoModify = FALSE;
       }
       else
       if ((nExistOpenCursor & OPEN_CURSOR_SQLACT))
       {
           //
           // SQL Test Window has an opened cursor on the table:
           // MSG: Unable to launch the Modify Structure Dialog, because an SQL/Test window has a session open on the table.\n 
           //      Please close the SQL/Test session to perform the Modify Structure.
           MessageBox(hwndMdi , ResourceString ((UINT)IDS_MODIFY_STRUCTURExOPENCURSOR_SQLTEST), NULL, MB_ICONEXCLAMATION | MB_OK);
           bDoModify = FALSE;
       }
       free (pArrayInfo);
   }
   if (!bDoModify)
       return;


   switch (objType)
   {
       case OT_TABLE:
       case OT_SCHEMAUSER_TABLE:
       {
           TABLEPARAMS tableparams;
           int err;

           ZEROINIT (tableparams);

           x_strcpy (tableparams.DBName,     parentstrings [0]);
           x_strcpy (tableparams.objectname, RemoveDisplayQuotesIfAny(StringWithoutOwner(objName)));
           x_strcpy (tableparams.szSchema,   objOwner);

           err = GetDetailInfo (lpVirtNode, OT_TABLE, &tableparams, FALSE,
                                &SesHndl);
           if (err != RES_SUCCESS)
               return;

           tableparams.StorageParams.nObjectType      =  OT_TABLE;
           x_strcpy (tableparams.StorageParams.DBName,     parentstrings [0]);
           x_strcpy (tableparams.StorageParams.TableName,  RemoveDisplayQuotesIfAny(StringWithoutOwner(parentstrings [1])));
           x_strcpy (tableparams.StorageParams.TableOwner, ParentTableOwner);
           x_strcpy (tableparams.StorageParams.objectname, RemoveDisplayQuotesIfAny(StringWithoutOwner(objName)));
           x_strcpy (tableparams.StorageParams.objectowner,objOwner);
           tableparams.StorageParams.uPage_size = tableparams.uPage_size;
           tableparams.StorageParams.bReadOnly = tableparams.bReadOnly;
           if ((TCHAR)tableparams.StorageParams.objectname[0] == '$' || (TCHAR)tableparams.StorageParams.objectowner[0] == '$')
           {
               TCHAR tchszMessage [256];
               // "Cannot modify system object %s."
               wsprintf (tchszMessage, ResourceString(IDS_ERR_MODIFY_SYSTEM_OBJECT), tableparams.StorageParams.TableName);
               MessageBox( GetFocus(), tchszMessage, NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
               FreeAttachedPointers (&tableparams,  OT_TABLE);
               return;
           }
           iret = DlgModifyStructure (hwndMdi, &(tableparams.StorageParams));
           FreeAttachedPointers (&tableparams,  OT_TABLE);
           if (!iret)
               return;
       }
       break;  
       case OT_INDEX:
       {
           INDEXPARAMS indexparams;
           int err;

           ZEROINIT (indexparams);

           x_strcpy (indexparams.DBName,     parentstrings [0]);
           x_strcpy (indexparams.TableName,  RemoveDisplayQuotesIfAny(StringWithoutOwner(parentstrings [1])));
           x_strcpy (indexparams.TableOwner, ParentTableOwner);
           x_strcpy (indexparams.objectname, RemoveDisplayQuotesIfAny(StringWithoutOwner(objName)));
           x_strcpy (indexparams.objectowner,objOwner);

           err = GetDetailInfo (lpVirtNode, OT_INDEX, &indexparams, FALSE,
                                &SesHndl);
           if (err != RES_SUCCESS)
               return;
        
           indexparams.nObjectType      =  OT_INDEX;
           if ((TCHAR)indexparams.objectname[0] == '$' || (TCHAR)indexparams.objectowner[0] == '$')
           {
               TCHAR tchszMessage [256];
               wsprintf (tchszMessage, ResourceString(IDS_ERR_MODIFY_SYSTEM_OBJECT), indexparams.TableName);
               MessageBox( GetFocus(), tchszMessage, NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
               FreeAttachedPointers (&indexparams,  OT_INDEX);
               return;
           }
           iret = DlgModifyStructure (hwndMdi, (LPSTORAGEPARAMS)(&indexparams));
           FreeAttachedPointers (&indexparams,  OT_INDEX);

         /* TO BE CHANGED IF STORAGEPARAMS DOES'NT MAP ANY MORE TO INDEXPARAMS*/
           if (!iret)
               return;
       }

       objType = OT_INDEX;
       level   = 2;
       break;

       default:
       {
           char message [100];
           wsprintf (message, ResourceString ((UINT)IDS_E_REVOKE_SYS_ERROR), objType);
           MessageBox( GetFocus(), message, NULL,
                       MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
           return;
       }
   }

   // return; /* nothing to be refreshed. TO BE FINISHED FOR OT_TABLELOCATION*/
   // issue 74618 showed that it needed to be finished

   DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, NULL);

   switch (objType)
   {
       case OT_TABLE:
           iter = 2;
           aObjType[0] = OT_TABLELOCATION;
           aObjType[1] = OT_INDEX;
           level = 2;
           StringWithOwner(objName, objOwner, parentstrings[1]);
           break;

       default:
          iter = 0;
   }

   for (cpt=0; cpt<iter; cpt++)
       UpdateDOMDataAllUsers(
            lpDomData->psDomNode->nodeHandle, // hnodestruct
            aObjType[cpt],                    // iobjecttype
            level,                            // level
            parentstrings,                    // parentstrings
            FALSE,                            // keep system tbls state
            FALSE);                           // bWithChildren

   for (cpt=0; cpt<iter; cpt++)
       DOMUpdateDisplayDataAllUsers (
            lpDomData->psDomNode->nodeHandle, // hnodestruct
            aObjType[cpt],                    // iobjecttype
            level,                            // level
            parentstrings,                    // parentstrings
            FALSE,                            // bWithChildren
            ACT_ADD_OBJECT,                   // object added
            0L,                               // no item id
            0);                               // all mdi windows concerned
   DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, NULL);

   // complimentary management of right pane: refresh it immediately
   UpdateRightPane(hwndMdi, lpDomData, TRUE, 0);
}

//
// IVM unindexed tables - create index
//
VOID DomCreateIndex(HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam)
{
	LPUCHAR parentstrings[MAXPLEVEL];
	UCHAR ParentTableOwner[MAXOBJECTNAME];
	LPTREERECORD lpRecord;
	int objType = -1;
	DWORD dwCurSel = 0;
	int iret = -1;
	int tblType = 0;


	objType = GetCurSelUnsolvedItemObjType (lpDomData);
	dwCurSel=(DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
	if (!dwCurSel)
          return;             // How did we happen here ?
        lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                               LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
	fstrncpy(ParentTableOwner,lpRecord->ownerName,MAXOBJECTNAME);
	if (!GetCurSelObjParents (lpDomData, parentstrings))
          return;             // TESTER RETVAL
      
	switch (objType)
	{
	    case OT_TABLE:
	    {
		if (IsVW() && (tblType = getint(lpRecord->szComplim + STEPSMALLOBJ + STEPSMALLOBJ)) == IDX_VW)
		{
			if (GetOIVers == OIVERS_NOTOI)
			{
				INDEXPARAMS idx;
					
				ZEROINIT(idx);
				idx.bPersistence = TRUE;
				idx.bCreate = TRUE;
				x_strcpy (idx.DBName   , lpRecord->extra); 
				x_strcpy (idx.TableName, lpRecord->objName);
				x_strcpy (idx.TableOwner, ParentTableOwner);

				iret = DlgCreateIndex (hwndMdi, &idx);
				if (iret != IDOK)
					return;	
			}
			else 
			{
                    		DMLCREATESTRUCT cr;
				//char extra2[MAXOBJECTNAME];
				//x_strcat(extra2, lpRecord->ownerName);
				//x_strcat(extra2, lpRecord->objName);
                   		memset (&cr, 0, sizeof (cr));
                    		lstrcpy (cr.tchszDatabase,    (LPCTSTR)lpRecord->extra);
                    		lstrcpy (cr.tchszObject,      (LPCTSTR)RemoveDisplayQuotesIfAny(StringWithoutOwner(lpRecord->objName)));
                    		lstrcpy (cr.tchszObjectOwner, (LPCTSTR)ParentTableOwner);
				cr.tblType = tblType;

                    		iret = VDBA_CreateIndex(hwndMdi, &cr);
                    		if (iret != IDOK)
                        	  return;
                	}
		}
	    }
	    default:
		break;
	}
}


//
// Star management
//
VOID DomRegisterAsLink(HWND hwndCurDom, LPDOMDATA lpCurDomData, BOOL bDragDrop, HWND hwndDestDom, LPDOMDATA lpDestDomData, DWORD ItemUnderMouse)
{
  // parameters for multi-call to UpdateDomData/DomUpdateDisplayData
  // (ex:securities) - Not used at this time
  int   iter;
  int   aObjType[10];
  int   alevel  [10];
  int   cpt;

  int   objType = -1;
  int   level   =  -1;
  int   iret = IDCANCEL;    // compare to IDOK

  // parameters for UpdateDomData/DomUpdateDisplayData
  LPDOMDATA lpDomData = NULL;
  LPUCHAR   parentstrings [MAXPLEVEL];

  // Infos on source and destination
  char* destNodeName     = NULL;
  char* destDbName       = NULL;
  char* srcNodeName      = NULL;
  char* srcDbName        = NULL;
  char* srcObjName       = NULL;
  char* srcOwnerName     = NULL;
  LPTREERECORD lpRecord  = NULL;

  // prepare parameters
  if (bDragDrop) {
    destNodeName  = GetVirtNodeName(lpDestDomData->psDomNode->nodeHandle);
    lpRecord = (LPTREERECORD)SendMessage(lpDestDomData->hwndTreeLb,
                                         LM_GETITEMDATA, 0, (LPARAM)ItemUnderMouse);
    destDbName    = lpRecord->extra;

    srcNodeName   = GetVirtNodeName(lpCurDomData->psDomNode->nodeHandle);
    lpRecord = (LPTREERECORD)SendMessage(lpCurDomData->hwndTreeLb,
                                         LM_GETITEMDATA, 0, GetCurSel(lpCurDomData));
    srcDbName     = lpRecord->extra;
    srcObjName    = lpRecord->objName;
    srcOwnerName  = lpRecord->ownerName;
  }
  else {
    destNodeName  = GetVirtNodeName(lpCurDomData->psDomNode->nodeHandle);
    lpRecord = (LPTREERECORD)SendMessage(lpCurDomData->hwndTreeLb,
                                         LM_GETITEMDATA, 0, GetCurSel(lpCurDomData));
    destDbName    = lpRecord->extra;

    srcNodeName   = NULL;
    srcDbName     = NULL;
    srcObjName    = NULL;
    srcOwnerName  = NULL;
  }

  objType = GetCurSelObjType (lpCurDomData);
  switch (objType) {
    case OT_STATIC_TABLE:
    case OT_TABLE:
    case OT_SCHEMAUSER_TABLE:
      iret = DlgRegisterTableAsLink(hwndCurDom,   // parent window
                                    destNodeName, // destination node name
                                    destDbName,   // destination database name
                                    bDragDrop,    // Not a Drag-drop operation
                                    srcNodeName,  // source node name if drag-drop
                                    srcDbName,    // source db name if drag-drop
                                    srcObjName,   // source table name if drag-drop
                                    srcOwnerName  // owner of source table if drag-drop
                                    );
      if (iret != IDOK)
          return;
      objType = OT_TABLE;
      level = 1;
      break;

    case OT_STATIC_VIEW:
    case OT_VIEW:
    case OT_SCHEMAUSER_VIEW:
      iret = DlgRegisterViewAsLink(hwndCurDom,    // parent window
                                    destNodeName, // destination node name
                                    destDbName,   // destination database name
                                    bDragDrop,    // Not a Drag-drop operation
                                    srcNodeName,  // source node name if drag-drop
                                    srcDbName,    // source db name if drag-drop
                                    srcObjName,   // source view name if drag-drop
                                    srcOwnerName  // owner of source view if drag-drop
                                    );
      if (iret != IDOK)
          return;
      level   = 1;
      objType = OT_VIEW;
      break;
    case OT_STATIC_PROCEDURE:
    case OT_PROCEDURE:
    case OT_SCHEMAUSER_PROCEDURE:
      iret = DlgRegisterProcAsLink(hwndCurDom,    // parent window
                                    destNodeName, // destination node name
                                    destDbName,   // destination database name
                                    bDragDrop,    // Not a Drag-drop operation
                                    srcNodeName,  // source node name if drag-drop
                                    srcDbName,    // source db name if drag-drop
                                    srcObjName,   // source procedure name if drag-drop
                                    srcOwnerName  // owner of source table if drag-drop
                                    );
      if (iret != IDOK)
          return;
      level   = 1;
      objType = OT_PROCEDURE;
      break;

    default:
    {
        char message [100];
        // "Register as link not implemented yet for this type of object: id = %d"
        wsprintf ( message,
            ResourceString(IDS_ERR_REGISTER_LINK_NOT_IMPLEMENTED),
            objType);
        MessageBox (GetFocus(), message, NULL,
                    MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
        return;
    }
  }

  //
  //  Step 1 Plus: change active document and selection if successful register as link
  //
  if (bDragDrop) {
    SendMessage(hwndMDIClient, WM_MDIACTIVATE, (WPARAM)GetParent(hwndDestDom), 0L); // MAINLIB
    SendMessage(lpDestDomData->hwndTreeLb, LM_SETSEL, 0, ItemUnderMouse);
  }

  //
  // Step 2: calls to UpdateDOMData and DomUpdateDisplayData
  // to refresh internal data and display,
  // since we have added a 'registered as link' object
  //

  if (bDragDrop)
    lpDomData = lpDestDomData;
  else
    lpDomData = lpCurDomData;

  DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, NULL);

  switch (objType)
  {
      default:
          iter = 1;
          aObjType[0] = objType;
          alevel[0]=level;
          break;
  }

  parentstrings[0] = destDbName;
  parentstrings[1] = parentstrings[2] = NULL;

  for (cpt=0; cpt<iter; cpt++)
      UpdateDOMDataAllUsers(
           lpDomData->psDomNode->nodeHandle, // hnodestruct
           aObjType[cpt],                    // iobjecttype
           alevel[cpt],                      // level
           parentstrings,                    // parentstrings
           FALSE,                            // keep system tbls state
           FALSE);                           // bWithChildren

  for (cpt=0; cpt<iter; cpt++) {
      DOMUpdateDisplayDataAllUsers (
           lpDomData->psDomNode->nodeHandle, // hnodestruct
           aObjType[cpt],                    // iobjecttype
           alevel[cpt],                      // level
           parentstrings,                    // parentstrings
           FALSE,                            // bWithChildren
           ACT_ADD_OBJECT,                   // object added
           0L,                               // no item id
           0);                               // all mdi windows concerned
  }
  DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, NULL);
}

VOID DomRefreshLink(HWND hwndCurDom, LPDOMDATA lpCurDomData, BOOL bDragDrop, HWND hwndDestDom, LPDOMDATA lpDestDomData, DWORD ItemUnderMouse)
{
  int answer;
  char buf[3*MAXOBJECTNAME];
  int ilocsession;
  int iret;

  LPTREERECORD lpRecord  = NULL;
  char* NodeName     = NULL;
  char* DbName       = NULL;
  char* objName      = NULL;

  NodeName  = GetVirtNodeName(lpCurDomData->psDomNode->nodeHandle);
  lpRecord = (LPTREERECORD)SendMessage(lpCurDomData->hwndTreeLb,
                                         LM_GETITEMDATA, 0, GetCurSel(lpCurDomData));
  DbName    = lpRecord->extra;
  objName   = lpRecord->objName;

  // verifications
  if (lpRecord->parentDbType != DBTYPE_DISTRIBUTED) {
    // "Cannot Refresh Link: not a Distributed Database"
    MessageBox(hwndCurDom, ResourceString(IDS_ERR_REFRESH_LINK), NULL, MB_ICONEXCLAMATION | MB_OK);
    return;
  }
  // ask for confirmation
  //"Refresh link for object %s ?"
  wsprintf(buf, ResourceString(IDS_ERR_CONFIRM_REFRESH_LINK), objName);
  answer = MessageBox (hwndCurDom, buf, ResourceString(IDS_TITLE_CONFIRM), MB_ICONQUESTION|MB_YESNO);
  if (answer != IDYES)
    return;

  ShowHourGlass ();

  // Get a session on the ddb (note: accepts "(local)" as a node name)
  wsprintf(buf, "%s::%s", NodeName, DbName);
  iret = Getsession(buf, SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
  if (iret !=RES_SUCCESS) {
    RemoveHourGlass ();
    // "Cannot get a Session"
    MessageBox(hwndCurDom,ResourceString(IDS_ERR_GET_SESSION) , NULL, MB_OK|MB_ICONSTOP);
    return;
  }

  // execute the statement
  wsprintf(buf, ResourceString(IDS_ERR_REGISTER_AS_LINK_REFRESH), objName);//"register %s as link with refresh"
  iret=ExecSQLImm(buf, FALSE, NULL, NULL, NULL);

  RemoveHourGlass ();

  // release the session
  ReleaseSession(ilocsession, iret==RES_SUCCESS ? RELEASE_COMMIT : RELEASE_ROLLBACK);
  if (iret !=RES_SUCCESS) {
    //_T("Cannot Refresh Link")
    MessageWithHistoryButton(hwndCurDom, ResourceString(IDS_ERR_TWO_REFRESH_LINK));
    return;
  }

  // FINIR : QUESTION: voir rafraichissement sous-branche indexes si table ???
}
