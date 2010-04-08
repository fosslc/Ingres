/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project  : Visual DBA
**
**    Source   : addrepl.c
**
**    Add the replication connection.
**
**
**  History after 04-May-1999:
**
**   07-May-1999 (schph01)
**    Limited the amount of text the user may enter into the edit control
**    "Description"
**    to 80 instead 81.
**   20-Jan-2000 (uk$so01)
**     (Bug #100063) Eliminate the undesired compiler's warning
**   26-Mar-2001 (noifr01)
**     (sir 104270) removal of code for managing Ingres/Desktop
**  20-Sep-2002 (schph01)
**    bug 108645 Replace the call of CGhostname() by the GetLocalVnodeName()
**    function.( XX.XXXXX.gcn.local_vnode parameter in config.dat)
********************************************************************/

#include "compat.h"
#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "catospin.h"
#include "getdata.h"
#include "msghandl.h"
#include "lexical.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "esltools.h"
#include "monitor.h"
#include "winutils.h" // ShowHourGlass(),RemoveHourGlass()
#include "resource.h"
#include "extccall.h"

/*
#define REPLIC_FULLPEER      1
#define REPLIC_PROT_RO       2
#define REPLIC_UNPROT_RO     3
#define REPLIC_EACCESS       4
*/
#define MAX_MOBILE_USER       25  // max len for Mobile user

CTLDATA typeTypes[] =
{
   REPLIC_FULLPEER,    IDS_FULLPEER,
   REPLIC_PROT_RO,     IDS_PROTREADONLY,
   REPLIC_UNPROT_RO,   IDS_UNPROTREADONLY,
   REPLIC_EACCESS,     IDS_GATEWAY,
   -1                  // terminator
};

// Structure describing the numeric edit controls
static EDITMINMAX limits[] =
{
   IDC_REPL_CONN_NUMBER, IDC_REPL_CONN_SPIN_NUMBER, REPL_MIN_NUMBER, REPL_MAX_NUMBER,
   IDC_REPL_CONN_SERVER, IDC_REPL_CONN_SPIN_SERVER, REPL_MIN_SERVER, REPL_MAX_SERVER,
   -1// terminator
};

BOOL CALLBACK __export DlgAddReplConnectionDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy                  (HWND hwnd);
static void OnCommand                  (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog               (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange           (HWND hwnd);

static void InitialiseSpinControls     (HWND hwnd);
static void InitialiseEditControls     (HWND hwnd);
static BOOL OccupyTypesControl         (HWND hwnd);
static void EnableDisableOKButton      (HWND hwnd);
static void FillDatabasesOfVnode       (HWND hwndCtrlDB, LPUCHAR nodeName,HWND hwndCtrlType);
static void DisableEnableControl       (HWND hwnd);
static BOOL AlterObject                (HWND hwnd);
static BOOL FillStructureFromControls  (HWND hwnd, LPREPLCONNECTPARAMS lpaddrepl);
static BOOL FillControlsFromStructure  (HWND hwnd, LPREPLCONNECTPARAMS lpaddrepl);
static int  FindItemData( HWND hwndCtl, char * ItemName);
static int  FindData( HWND hwndCtl, char * ItemName);
static void HideShowDBname(HWND hwnd,BOOL bState);
static int  SelectDefaultType( HWND hwndCur);


BOOL WINAPI __export DlgAddReplConnection (HWND hwndOwner, LPREPLCONNECTPARAMS lpaddrepl)
/*
//   Function:
//   Shows the Add Replication Connection dialog.
//
//   Paramters:
//       hwndOwner - Handle of the parent window for the dialog.
//       lpaddrepl - Points to structure containing information used to 
//                   initialise the dialog. The dialog uses the same
//                   structure to return the result of the dialog.
//
//   Returns:
//       TRUE if successful.
//
//   Note:
//       If invalid parameters are passed in they are reset to a default
//       so the dialog can be safely displayed.
*/
{
   FARPROC lpProc;
   int     retVal;

   if (!IsWindow(hwndOwner) || !lpaddrepl)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgAddReplConnectionDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_REPL_CONN),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpaddrepl
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgAddReplConnectionDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
       HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
       HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);

       case WM_INITDIALOG:
           return HANDLE_WM_INITDIALOG (hwnd, wParam, lParam, OnInitDialog);

       case LM_GETEDITMINMAX:
       {
           LPEDITMINMAX lpdesc = (LPEDITMINMAX) lParam;
           HWND hwndCtl = (HWND) wParam;

           ASSERT(IsWindow(hwndCtl));
           ASSERT(lpdesc);

           return GetEditCtrlMinMaxValue (hwnd, hwndCtl, limits, &lpdesc->dwMin, &lpdesc->dwMax);
       }

       case LM_GETEDITSPINCTRL:
       {
           HWND hwndEdit = (HWND) wParam;
           HWND FAR * lphwnd = (HWND FAR *)lParam;
           *lphwnd = GetEditSpinControl(hwndEdit, limits);
           break;
       }

       default:
           return HandleUserMessages(hwnd, message, wParam, lParam);
   }
   return TRUE;
}


static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   LPREPLCONNECTPARAMS lpadd = (LPREPLCONNECTPARAMS)lParam;
   char szFormat [90];
   char szTitle [MAX_TITLEBAR_LEN];

   if (!AllocDlgProp(hwnd, lpadd))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();
  
   switch (lpadd->nReplicVersion) {
      case REPLIC_V10:
        lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_REPL_CONN));
        break;
      case REPLIC_V105:
        lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_REPL_CONN_1_05));
        break;
      case REPLIC_V11:
        lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_REPL_CONN_1_1));
        break;
      default:
        // Should not pass here!
        lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_REPL_CONN));
        break;
   }

   if (lpadd->nReplicVersion == REPLIC_V10 ||
       lpadd->nReplicVersion == REPLIC_V105  ){
      if (lpadd->nNumber > REPL_MAX_NUMBER || lpadd->nNumber < REPL_MIN_NUMBER)
          lpadd->nNumber = REPL_MIN_NUMBER;
   }
   else {
      if (lpadd->nNumber > REPL_MAX_NUMBER_V11 || lpadd->nNumber < REPL_MIN_NUMBER)
          lpadd->nNumber = REPL_MIN_NUMBER;
      limits[0].dwMax = REPL_MAX_NUMBER_V11;
   }

   if (lpadd->nServer > REPL_MAX_SERVER || lpadd->nServer < REPL_MIN_SERVER)
       lpadd->nServer = REPL_MIN_SERVER;

   if (lpadd->nType > REPLIC_EACCESS)
       lpadd->nType = 0;

   // Force the catospin.dll to load
   SpinGetVersion();

   if (!OccupyTypesControl(hwnd))
   {
       ASSERT(NULL);
       return FALSE;
   }
   SelectDefaultType(hwnd);
   if (lpadd->bAlter == TRUE && lpadd->bLocalDb == FALSE)
       LoadString (hResource, (UINT)IDS_T_REPL_ALTER_CONN, szFormat, sizeof (szFormat));
   if (lpadd->bAlter == TRUE && lpadd->bLocalDb == TRUE)
       LoadString (hResource, (UINT)IDS_T_REPL_ALTER_LOCAL_CONN, szFormat, sizeof (szFormat));
   if (lpadd->bAlter == FALSE && lpadd->bLocalDb == TRUE)
       LoadString (hResource, (UINT)IDS_T_REPL_INST_CONN, szFormat, sizeof (szFormat));
   if (lpadd->bAlter == FALSE && lpadd->bLocalDb == FALSE)
       LoadString (hResource, (UINT)IDS_T_REPL_CONN, szFormat, sizeof (szFormat));

   wsprintf (
       szTitle,
       szFormat,
       GetVirtNodeName (GetCurMdiNodeHandle ()),
       lpadd->DBName);
   SetWindowText (hwnd, szTitle);

   InitialiseSpinControls (hwnd);
   InitialiseEditControls (hwnd);

   FillControlsFromStructure (hwnd, lpadd);

   DisableEnableControl(hwnd);

   EnableDisableOKButton (hwnd);
   richCenterDialog(hwnd);

   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   HWND hwndCtl = GetDlgItem (hwnd, IDC_REPL_CONN_TYPE);
   LPREPLCONNECTPARAMS lpadd = GetDlgProp(hwnd);

   if (lpadd->nReplicVersion == REPLIC_V11) {
      ComboBoxDestroyItemData (hwndCtl);
   }
   SubclassAllNumericEditControls (hwnd, EC_RESETSUBCLASS);

   ResetSubClassEditCntl(hwnd, IDC_REPL_CONN_VNODE);
   ResetSubClassEditCntl(hwnd, IDC_REPL_CONN_DBNAME);

   if (lpadd->bAlter) {
     int type = GetRepObjectType4ll(OT_REPLIC_CONNECTION, lpadd->nReplicVersion);
     FreeAttachedPointers (lpadd, type);
   }

   DeallocDlgProp(hwnd);
   
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   HWND hwndDBName = GetDlgItem(hwnd, IDC_REPL_CONN_DBNAME);
   HWND hwndVNode  = GetDlgItem(hwnd, IDC_REPL_CONN_VNODE);
   HWND hwndType   = GetDlgItem(hwnd, IDC_REPL_CONN_TYPE);
   LPREPLCONNECTPARAMS lpadd = GetDlgProp(hwnd);
   BOOL Success;
   int  ires;
   
  switch (id)
  {
    case IDOK:
    {
    // create connection replication
    if (lpadd->bLocalDb == FALSE && lpadd->bAlter == FALSE) {
      Success = FillStructureFromControls (hwnd, lpadd);
      if (!Success)
        break;

      ires = DBAAddObject ( GetVirtNodeName ( GetCurMdiNodeHandle ()),
                            GetRepObjectType4ll(OT_REPLIC_CONNECTION,lpadd->nReplicVersion),
                            (void *) lpadd);

      if (ires != RES_SUCCESS)
      {
        ErrorMessage ((UINT) IDS_E_REPL_CONN_FAILED, ires);
        break;
      }
    }
    // create local connection replication called only in the installation 
    // replication object
    if (lpadd->bLocalDb == TRUE && lpadd->bAlter == FALSE )  {
      Success = FillStructureFromControls (hwnd, lpadd);
      if (!Success)
        break;

     // wsprintf(buf,"repcat %s::%s", lpadd->szVNode,lpadd->szDBName);
     // wsprintf(
     //   buf2,
     //   ResourceString ((UINT)IDS_I_INSTALL_REPLOBJ), // Installing Replication objects for %s::%s
     //   lpadd->szVNode,
     //   lpadd->szDBName);

     // execrmcmd(lpadd->szVNode,buf,buf2);
     // SetFocus(hwnd);

      ires = DBAAddObject( GetVirtNodeName ( GetCurMdiNodeHandle ()),
                                             GetRepObjectType4ll(OT_REPLIC_CONNECTION,lpadd->nReplicVersion),
                                             (void *) lpadd);
      if (ires != RES_SUCCESS)
        {
        ErrorMessage ((UINT) IDS_E_REPL_CONN_FAILED, ires);
        break;
        }
      }
  
      // Alter connection replication  
      if (lpadd->bLocalDb == FALSE && lpadd->bAlter == TRUE ||
          lpadd->bLocalDb == TRUE  && lpadd->bAlter == TRUE)
          {
          Success = AlterObject (hwnd);
          if (!Success)
            break;
          }
      EndDialog (hwnd, TRUE);
    }    
    break;

      case IDCANCEL:
          EndDialog (hwnd, FALSE);
          break;

      case IDC_REPL_CONN_SERVER:
      case IDC_REPL_CONN_NUMBER:
      {
          switch (codeNotify)
          {
              case EN_CHANGE:
              {
                  int nCount;

                  if (!IsWindowVisible(hwnd))
                      break;

                  // Test to see if the control is empty. If so then
                  // break out.  It becomes tiresome to edit
                  // if you delete all characters and an error box pops up.

                  nCount = Edit_GetTextLength(hwndCtl);
                  if (nCount == 0)
                      break;

                  VerifyAllNumericEditControls(hwnd, TRUE, TRUE);
                  break;
              }

              case EN_KILLFOCUS:
              {
                  HWND hwndNew = GetFocus();
                  int nNewCtl = GetDlgCtrlID(hwndNew);

                  if (nNewCtl == IDCANCEL
                      || nNewCtl == IDOK
                      || !IsChild(hwnd, hwndNew))
                      // Dont verify on any button hits
                      break;

                  if (!VerifyAllNumericEditControls(hwnd, TRUE, TRUE))
                      break;

                  UpdateSpinButtons(hwnd);
                  break;
              }
          }
      break;
      }

      case IDC_REPL_CONN_SPIN_NUMBER:
      case IDC_REPL_CONN_SPIN_SERVER:
      {
          ProcessSpinControl(hwndCtl, codeNotify, limits);
          break;
      }

      case IDC_REPL_CONN_DBNAME:
      {
          switch (codeNotify)
          {
              case CBN_SELCHANGE:
              {
                  EnableDisableOKButton (hwnd);
                  break;
              }
          }
          break;
      }

      case IDC_REPL_CONN_VNODE:
      {
        UCHAR vnode_name [MAXOBJECTNAME];

        switch (codeNotify)
        {
          case CBN_SELCHANGE:
          {
              ComboBox_GetText (hwndVNode, vnode_name, sizeof(vnode_name));
              if (x_strlen (vnode_name))
              {
                  if (x_strcmp(vnode_name,"mobile")==0)
                    HideShowDBname(hwnd,TRUE);
                  else  {
                    HideShowDBname(hwnd,FALSE); 
                    ComboBox_ResetContent (hwndDBName);
                    FillDatabasesOfVnode  (hwndDBName, vnode_name,hwndType);
                    ComboBoxSelectFirstStr(hwndDBName);
                    EnableDisableOKButton (hwnd);
                  }
              }
              EnableDisableOKButton (hwnd);
              break;
          }
        }
        break;
      }
      case IDC_REPL_CONN_TYPE:
      {
        switch (codeNotify)
        {
          case CBN_SELCHANGE:
          {
            int nbIndex,nSel;
            char type[MAXOBJECTNAME];

            if (lpadd->nReplicVersion == REPLIC_V11  )   {
               nSel  = ComboBox_GetCurSel(hwndType);

               x_strcpy(type, (char *)ComboBox_GetItemData(hwndType, nSel));

               if (x_strlen (type) && x_strcmp(type,"opingdt")==0 && lpadd->bLocalDb == FALSE) {
                  nbIndex = FindItemData( hwndType, "opingdt");
                  if (nbIndex != CB_ERR)  {
                     nbIndex = FindData( hwndVNode, "mobile");
                     if (nbIndex != CB_ERR)  {
                        ComboBox_SetCurSel(hwndVNode, nbIndex);
                        HideShowDBname(hwnd,TRUE);
                     }
                  }
               }
               else  {
                  UCHAR vnode_name [MAXOBJECTNAME];
                  ComboBox_GetText (hwndVNode, vnode_name, sizeof(vnode_name));
                  if (x_strlen (vnode_name)) {
                     HideShowDBname(hwnd,FALSE); 
                     ComboBox_ResetContent (hwndDBName);
                     FillDatabasesOfVnode  (hwndDBName, vnode_name,hwndType);
                     ComboBoxSelectFirstStr(hwndDBName);
                     EnableDisableOKButton (hwnd);
                  }
               }
            }
          break;
          }
        }
        break;
      }
      case IDC_DB_NAME_EDIT:
      case IDC_MOBILE_USER_NAME:
      {
        switch (codeNotify)
        {
        case EN_KILLFOCUS:
        case EN_CHANGE:
          EnableDisableOKButton (hwnd);
        }
      }
      break;

  }
}


static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}


static void InitialiseEditControls (HWND hwnd)
{
   char szNumber[20];
   LPREPLCONNECTPARAMS lpadd = GetDlgProp(hwnd);

   HWND hwndDBName = GetDlgItem(hwnd, IDC_REPL_CONN_DBNAME);
   HWND hwndDesc   = GetDlgItem(hwnd, IDC_REPL_CONN_DESCRIPTION);
   HWND hwndVNode  = GetDlgItem(hwnd, IDC_REPL_CONN_VNODE);
   HWND hwndNumber = GetDlgItem(hwnd, IDC_REPL_CONN_NUMBER);
   HWND hwndServer = GetDlgItem(hwnd, IDC_REPL_CONN_SERVER);

   // Subclass the edit controls

   SubclassAllNumericEditControls(hwnd, EC_SUBCLASS);

   // Limit the text in the edit controls

   Edit_LimitText (hwndDesc, MAX_DESCRIPTION_LEN-1);
   LimitNumericEditControls (hwnd);

   // Initialise edit controls with strings
   if ( lpadd->bAlter==TRUE || lpadd->bLocalDb==TRUE)
     {
     SetDlgItemText(hwnd, IDC_REPL_CONN_DBNAME, lpadd->szDBName);
     SetDlgItemText(hwnd, IDC_REPL_CONN_DESCRIPTION, lpadd->szDescription);
     SetDlgItemText(hwnd, IDC_REPL_CONN_VNODE, lpadd->szVNode);
     }
   my_itoa(lpadd->nNumber, szNumber, 10);
   SetDlgItemText(hwnd, IDC_REPL_CONN_NUMBER, szNumber);
   if (lpadd->nReplicVersion == REPLIC_V10 || 
       lpadd->nReplicVersion == REPLIC_V105) {
      my_itoa(lpadd->nServer, szNumber, 10);
      SetDlgItemText(hwnd, IDC_REPL_CONN_SERVER, szNumber);
   }
   if (lpadd->nReplicVersion == REPLIC_V11) {
      ShowWindow(hwndServer,SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, IDC_REPL_STATIC_SERVER),SW_HIDE);
   }
}



static void InitialiseSpinControls (HWND hwnd)
{
   DWORD dwMin, dwMax;
   LPREPLCONNECTPARAMS lpadd = GetDlgProp(hwnd);

   GetEditCtrlMinMaxValue  (hwnd, GetDlgItem(hwnd, IDC_REPL_CONN_NUMBER), limits, &dwMin, &dwMax);
   SpinRangeSet(GetDlgItem (hwnd, IDC_REPL_CONN_SPIN_NUMBER), dwMin, dwMax);

   if (lpadd->nReplicVersion == REPLIC_V10 ||
		 lpadd->nReplicVersion == REPLIC_V105 ) {
     GetEditCtrlMinMaxValue  (hwnd, GetDlgItem(hwnd, IDC_REPL_CONN_SERVER), limits, &dwMin, &dwMax);
     SpinRangeSet(GetDlgItem (hwnd, IDC_REPL_CONN_SPIN_SERVER), dwMin, dwMax);
     SpinPositionSet (GetDlgItem (hwnd, IDC_REPL_CONN_SPIN_SERVER), lpadd->nServer);
   }
   if (lpadd->nReplicVersion == REPLIC_V11) {
      ShowWindow(GetDlgItem (hwnd, IDC_REPL_CONN_SPIN_SERVER),SW_HIDE);
   }
   SpinPositionSet (GetDlgItem (hwnd, IDC_REPL_CONN_SPIN_NUMBER), lpadd->nNumber);
}



static BOOL OccupyTypesControl (HWND hwnd)
/*
// Function:
//     Fills the types drop down box with the type names.Replicator 1.0
//     Fills the DBMS type. Replicator version 1.1
//
// Parameters:
//     hwnd - Handle to the dialog window.
// 
// Returns:
//     TRUE if successful.
*/
{
   UCHAR buf      [MAXOBJECTNAME];  
   UCHAR buffilter[MAXOBJECTNAME];
   UCHAR extradata[EXTRADATASIZE];
   LPUCHAR buffowner;
   LPUCHAR aparents[MAXPLEVEL];
   int iret;
   int Index;

   HWND hwndCtl = GetDlgItem (hwnd, IDC_REPL_CONN_TYPE);
   LPREPLCONNECTPARAMS lpadd = GetDlgProp(hwnd);

   if (lpadd->nReplicVersion == REPLIC_V10 || lpadd->nReplicVersion == REPLIC_V105)
    return OccupyComboBoxControl(hwndCtl, typeTypes);
   else  {
      aparents[0] = lpadd->DBName;
      aparents[1] = 0;
      for ( iret  = DBAGetFirstObject (
                          GetVirtNodeName ( GetCurMdiNodeHandle ()),
                          OT_REPLIC_DBMS_TYPE_V11,
                          1,
                          aparents,
                          TRUE,
                          buf,
                          buffilter,extradata);
            iret == RES_SUCCESS ;
            iret = DBAGetNextObject(buf,buffilter,extradata)
      )
        {
        Index = ComboBox_AddString (hwndCtl, extradata);
        if (Index != CB_ERR || Index != CB_ERRSPACE)  {
          buffowner = ESL_AllocMem (x_strlen (buf) +1);
          x_strcpy ( buffowner,buf);
          ComboBox_SetItemData(hwndCtl, Index, buffowner);
        }
        }
        return TRUE;
   }
	return FALSE;
}

static void FillVnodeName (HWND hwndCtl)
{
   NODEDATAMIN nodedata;
   int ires;
   char BaseHostName[MAXOBJECTNAME];

   ires=GetFirstMonInfo(0,0,NULL,OT_NODE,(void * )&nodedata,NULL);
   while (ires==RES_SUCCESS) {
          if (nodedata.bIsLocal == TRUE) {
             memset (BaseHostName,'\0',MAXOBJECTNAME);
             GetLocalVnodeName (BaseHostName, MAXOBJECTNAME);
             if (BaseHostName[0] != '\0') {
                x_strcat(nodedata.NodeName," ");
                x_strcat(nodedata.NodeName,BaseHostName);
             }
          }
          ComboBox_AddString (hwndCtl, nodedata.NodeName);
       ires=GetNextMonInfo((void * )&nodedata);
   }
}

static void EnableDisableOKButton (HWND hwnd)
{
   DWORD nSel, nSel1, nbVnode, nbData ;
   char szBuffer[MAX_MOBILE_USER];
   BOOL bValid = TRUE;
   HWND hwndVnode     = GetDlgItem (hwnd, IDC_REPL_CONN_VNODE);
   HWND hwndDatabases = GetDlgItem (hwnd, IDC_REPL_CONN_DBNAME);
   HWND hwndEditData  = GetDlgItem (hwnd, IDC_DB_NAME_EDIT);
   HWND hwndEditOwner = GetDlgItem (hwnd, IDC_MOBILE_USER_NAME);

   LPREPLCONNECTPARAMS lpadd = GetDlgProp(hwnd);
   ZEROINIT(szBuffer);
   
   nSel  = ComboBox_GetCurSel(hwndVnode);
   nSel1 = ComboBox_GetCurSel(hwndDatabases);
 
   nbVnode = ComboBox_GetLBTextLen(hwndVnode, nSel);
   nbData  = ComboBox_GetLBTextLen(hwndDatabases,nSel1);

   if ((lpadd->nReplicVersion == REPLIC_V105 ||
      lpadd->nReplicVersion == REPLIC_V11) &&
      nSel != CB_ERR )  {
      ComboBox_GetLBText(hwndVnode, nSel, szBuffer);
      if (x_stricmp(szBuffer,"mobile") == 0)  {
         if (Edit_GetTextLength(hwndEditData) == 0 ||
             Edit_GetTextLength(hwndEditOwner) == 0 )
               bValid = FALSE;
      }
   }
   if ((nbVnode == 0)      ||
       (nbVnode == CB_ERR) ||
       ((nbData == 0 || nbData == CB_ERR) && x_stricmp(szBuffer,"mobile")!=0) ||
       (bValid == FALSE)
      )
      EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   else
      EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
}


static void FillDatabasesOfVnode  (HWND hwndCtrlDB, LPUCHAR nodeName,HWND hwndCtrlType)
{
   int     hdl, ires,nIdx,oldOIVers;
   char    buf [MAXOBJECTNAME];
   CHAR    extradata[EXTRADATASIZE];
   char    szDBMStype[MAXOBJECTNAME];
   char    szConnectName[MAXOBJECTNAME*2];
   // Connect With the server class selected in Combobox "Gateway Type".
   nIdx = ComboBox_GetCurSel(hwndCtrlType);
   if (nIdx == CB_ERR)
      return;
   x_strcpy(szDBMStype, (LPUCHAR) ComboBox_GetItemData(hwndCtrlType, nIdx));
   x_strcpy(szConnectName,nodeName);
   x_strcat(szConnectName,LPCLASSPREFIXINNODENAME);
   x_strcat(szConnectName,CharUpper(szDBMStype));
   x_strcat(szConnectName,LPCLASSSUFFIXINNODENAME);

   oldOIVers = GetOIVers();
   if (!(x_stricmp(nodeName,GetVirtNodeName (GetCurMdiNodeHandle ())) == 0 &&
       x_stricmp(szDBMStype,"ingres")==0 &&
       oldOIVers != OIVERS_NOTOI)) {
         if ( !CheckConnection(szConnectName,FALSE,FALSE) ) {
            SetOIVers(oldOIVers);
            return;
         }
   }
   ShowHourGlass ();
   hdl = OpenNodeStruct (szConnectName);

   ires = DOMGetFirstObject (hdl, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, extradata);
   while (ires == RES_SUCCESS)
   {
       if ( getint(extradata+STEPSMALLOBJ) == DBTYPE_REGULAR)
          ComboBox_AddString (hwndCtrlDB, buf);
       ires  = DOMGetNextObject (buf, NULL, extradata);
   }

   CloseNodeStruct (hdl, FALSE);
   RemoveHourGlass ();
   SetOIVers(oldOIVers);
}
static int SelectDefaultType( HWND hwndCur)
{
   int nbInd;
   HWND hwndType = GetDlgItem(hwndCur, IDC_REPL_CONN_TYPE);
   if (GetOIVers () !=OIVERS_12)
      nbInd = FindData( hwndType, "OpenIngres");
   else
      if (GetOIVers () == OIVERS_12)
         nbInd = FindData( hwndType, "CA-OpenIngres");
   
   if (nbInd == CB_ERR)
         nbInd = FindData( hwndType, "Ingres");

   if (nbInd != CB_ERR)
      ComboBox_SetCurSel(hwndType, nbInd);
   else
      ComboBox_SetCurSel(hwndType, 0);
  
   return nbInd;
}

static void DisableEnableControl(HWND hwndCtl)
{
   int nbIndex;
   LPREPLCONNECTPARAMS lpadd = GetDlgProp(hwndCtl);

   if ( lpadd->bLocalDb==TRUE && lpadd->bAlter == TRUE )
   {
       if (lpadd->nReplicVersion == REPLIC_V10 ||
           lpadd->nReplicVersion == REPLIC_V105) {
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_VNODE )      , FALSE);
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_TYPE  )      , TRUE );
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_NUMBER)      , TRUE );
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SPIN_NUMBER) , TRUE );
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_DESCRIPTION) , TRUE );
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_DBNAME)      , FALSE);
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SERVER)      , FALSE);
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SPIN_SERVER) , FALSE);
           return;
       }
       if (lpadd->nReplicVersion == REPLIC_V11) {
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_TYPE  )      , FALSE);
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_DESCRIPTION) , TRUE );
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_NUMBER)      , FALSE);
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SPIN_NUMBER) , FALSE);
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_VNODE )      , FALSE);
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_DBNAME)      , FALSE);
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SERVER)      , FALSE);
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SPIN_SERVER) , FALSE);
            return;
        }
   }

   if ( lpadd->bLocalDb==FALSE && lpadd->bAlter == TRUE )
   {
       if (lpadd->nReplicVersion == REPLIC_V10 ||
           lpadd->nReplicVersion == REPLIC_V105) {
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SERVER)      , TRUE);
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SPIN_SERVER) , TRUE);
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_DESCRIPTION) , TRUE );
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_VNODE )      , FALSE);
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_DBNAME)      , FALSE);
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_TYPE  )      , FALSE);
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_NUMBER)      , FALSE);
           EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SPIN_NUMBER) , FALSE);
           if (lpadd->nReplicVersion == REPLIC_V105) {
              EnableWindow (GetDlgItem (hwndCtl, IDC_DB_NAME_EDIT) , FALSE);
              EnableWindow (GetDlgItem (hwndCtl, IDC_MOBILE_USER_NAME)  , FALSE);
              if (x_strcmp(lpadd->szVNode,"mobile")==0)   {
                 EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SPIN_SERVER) , FALSE);
                 EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SERVER) , FALSE);
              }
            }

            return;
        }
        if (lpadd->nReplicVersion == REPLIC_V11) {
            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_DESCRIPTION) , TRUE );
            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_TYPE  )      , FALSE );
            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_VNODE )      , FALSE);
            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_DBNAME)      , FALSE);
            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_NUMBER)      , FALSE);
            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SPIN_NUMBER) , FALSE);
            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SERVER)      , FALSE);
            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SPIN_SERVER) , FALSE);
            return;
        }
     }


   if ( lpadd->bLocalDb == TRUE &&  lpadd->bAlter == FALSE )
   {
        if (lpadd->nReplicVersion == REPLIC_V10 ||
            lpadd->nReplicVersion == REPLIC_V105) {
            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_VNODE)       , FALSE);
            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_DBNAME)      , FALSE);
            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SERVER)      , FALSE);
            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SPIN_SERVER) , FALSE);
        }
        if (lpadd->nReplicVersion == REPLIC_V11) {

            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_DESCRIPTION) , TRUE );
            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_NUMBER)      , TRUE);
            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SPIN_NUMBER) , TRUE);
            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_VNODE )      , FALSE);
            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_DBNAME)      , FALSE);
            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SERVER)      , FALSE);
            EnableWindow (GetDlgItem (hwndCtl, IDC_REPL_CONN_SPIN_SERVER) , FALSE);

            nbIndex = SelectDefaultType(hwndCtl);
            if (nbIndex != CB_ERR)
               EnableWindow ( GetDlgItem(hwndCtl, IDC_REPL_CONN_TYPE) , FALSE );
            else
               EnableWindow ( GetDlgItem(hwndCtl, IDC_REPL_CONN_TYPE) , TRUE );
        }
   }
   if ( lpadd->bLocalDb == FALSE &&  lpadd->bAlter == FALSE &&
        lpadd->nReplicVersion == REPLIC_V11)
      SelectDefaultType(hwndCtl);
}


static BOOL AlterObject (HWND hwnd)
{
   REPLCONNECTPARAMS lpadd2;
   REPLCONNECTPARAMS lpadd3;
   BOOL Success, bOverwrite;
   int  ires;
   HWND myFocus;
   int  hdl                    = GetCurMdiNodeHandle ();
   LPUCHAR vnodeName           = GetVirtNodeName (hdl);
   LPREPLCONNECTPARAMS lpadd1  = GetDlgProp (hwnd);
   BOOL bChangedbyOtherMessage = FALSE;
   int irestmp;
   ZEROINIT (lpadd2);
   ZEROINIT (lpadd3);

   x_strcpy (lpadd2.DBName, lpadd1->DBName);
   x_strcpy (lpadd3.DBName, lpadd1->DBName);
   x_strcpy (lpadd2.szOwner, lpadd1->szOwner);
   x_strcpy (lpadd3.szOwner, lpadd1->szOwner);
   x_strcpy (lpadd2.szDBOwner, lpadd1->szDBOwner);
   x_strcpy (lpadd3.szDBOwner, lpadd1->szDBOwner);
   lpadd2.nNumber  = lpadd1->nNumber;
   lpadd3.nNumber  = lpadd1->nNumber;
   lpadd2.bLocalDb = lpadd1->bLocalDb;
   lpadd3.bLocalDb = lpadd1->bLocalDb;
   lpadd2.bAlter   = lpadd1->bAlter;
   lpadd3.bAlter   = lpadd1->bAlter;
   lpadd2.nReplicVersion = lpadd1->nReplicVersion;
   lpadd3.nReplicVersion = lpadd1->nReplicVersion;

   if (lpadd1->nReplicVersion == REPLIC_V11)  {
      x_strcpy (lpadd2.szDBMStype, lpadd1->szDBMStype);
      x_strcpy (lpadd3.szDBMStype, lpadd1->szDBMStype);
   }
   Success = TRUE;
   myFocus = GetFocus();
   ires    = GetDetailInfo (vnodeName,
                            GetRepObjectType4ll(OT_REPLIC_CONNECTION,lpadd2.nReplicVersion),
                            &lpadd2,
                            TRUE,
                            &hdl);
   if (ires != RES_SUCCESS)
   {
   ReleaseSession(hdl,RELEASE_ROLLBACK);
   switch (ires)
   {
   case RES_ENDOFDATA:
     ires = ConfirmedMessage ((UINT)IDS_C_ALTER_CONFIRM_CREATE);
     SetFocus (myFocus);
     if (ires == IDYES)
     {
       Success = FillStructureFromControls (hwnd, &lpadd3);
       if (Success)
       {
         ires = DBAAddObject (vnodeName,
                              GetRepObjectType4ll(OT_REPLIC_CONNECTION,lpadd3.nReplicVersion),
                              (void *) &lpadd3 );
         if (ires != RES_SUCCESS)
         {
           ErrorMessage ((UINT) IDS_E_REPL_CONN_FAILED, ires);
           Success=FALSE;
         }
       }
     }
     break;
   default:
     Success=FALSE;
     ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
     break;
   }
       return Success;
   }

   if (!AreObjectsIdentical (lpadd1, &lpadd2,
                             GetRepObjectType4ll(OT_REPLIC_CONNECTION,lpadd1->nReplicVersion)))
   {
       ReleaseSession(hdl,RELEASE_ROLLBACK);
       ires   = ConfirmedMessage ((UINT)IDS_C_ALTER_RESTART_LASTCHANGES);
       bChangedbyOtherMessage = TRUE;
       irestmp=ires;
       if (ires == IDYES)
       {
         if (lpadd1->nNumber != lpadd2.nNumber)
            lpadd1->nNumber = lpadd2.nNumber;
         ires = GetDetailInfo (vnodeName,
                               GetRepObjectType4ll(OT_REPLIC_CONNECTION,lpadd1->nReplicVersion),
                               lpadd1,
                               FALSE,
                               &hdl);
         if (ires != RES_SUCCESS)
         {
             Success =FALSE;
             ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
         }
         else
         {
            //FillStructureWithNewValues(hwnd,lpadd1);
            FillControlsFromStructure (hwnd, lpadd1);
            return FALSE;  
         }
       }
       else
       {
           // Double Confirmation ?
           ires   = ConfirmedMessage ((UINT)IDS_C_ALTER_CONFIRM_OVERWRITE);
           if (ires != IDYES)
               Success=FALSE;
           else
           {
           ires = GetDetailInfo (vnodeName,
                                 GetRepObjectType4ll(OT_REPLIC_CONNECTION,lpadd1->nReplicVersion),
                                 lpadd1,
                                 TRUE,
                                 &hdl);
              if (ires != RES_SUCCESS)
              {
              ReleaseSession(hdl,RELEASE_ROLLBACK);
              Success=FALSE;
              }
           }
       }
       if (!Success)
           return Success;
   }

   Success = FillStructureFromControls (hwnd, &lpadd3);
   if (Success)
     {
     ires = DBAAlterObject (lpadd1, &lpadd3,
                            GetRepObjectType4ll(OT_REPLIC_CONNECTION,lpadd3.nReplicVersion),
                            hdl);
     if (ires != RES_SUCCESS)
       {
       Success=FALSE;
       ErrorMessage ((UINT) IDS_E_ALTER_REPL_CONNECT_FAILED, ires);
       ires = GetDetailInfo (vnodeName,
                             GetRepObjectType4ll(OT_REPLIC_CONNECTION,lpadd2.nReplicVersion),
                             &lpadd2, FALSE, &hdl);
       if (ires != RES_SUCCESS)
           ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
       else
         {
         if (!AreObjectsIdentical (lpadd1, &lpadd2,
                                   GetRepObjectType4ll(OT_REPLIC_CONNECTION,lpadd1->nReplicVersion)
                                   ))
           {
           if (!bChangedbyOtherMessage)
             {
               ires  = ConfirmedMessage ((UINT)IDS_C_ALTER_RESTART_LASTCHANGES);
             }
           else
               ires=irestmp;

           if (ires == IDYES)
             {
               ires = GetDetailInfo (vnodeName,
                                     GetRepObjectType4ll(OT_REPLIC_CONNECTION,lpadd1->nReplicVersion),
                                     lpadd1,
                                     FALSE,
                                     &hdl);
               if (ires == RES_SUCCESS)
                 FillStructureFromControls (hwnd, lpadd1);
             }
           else
               bOverwrite = TRUE;
           }
         }
     }
   }
   return Success;
}

static BOOL FillStructureFromControls (HWND hwnd, LPREPLCONNECTPARAMS lpaddrepl)
{
   HWND hwndType     = GetDlgItem (hwnd, IDC_REPL_CONN_TYPE);
   HWND hwndNumber   = GetDlgItem (hwnd, IDC_REPL_CONN_NUMBER);
   HWND hwndServer   = GetDlgItem (hwnd, IDC_REPL_CONN_SERVER);
   HWND hwndDBName;
   HWND hwndVNode;
   HWND hwndUserName;
   char szNumber [20],szVnodeName[MAXOBJECTNAME];
   int  nIdx,nmax;
   char *pDest;
   hwndVNode   = GetDlgItem (hwnd, IDC_REPL_CONN_VNODE); 

   ComboBox_GetText (hwndVNode, szVnodeName,  sizeof( szVnodeName ) / sizeof( szVnodeName[0]));

   pDest=x_strstr(szVnodeName,ResourceString((UINT)IDS_I_LOCALNODE));
   if (pDest == szVnodeName)
     x_strcpy(lpaddrepl->szVNode,szVnodeName+x_strlen(ResourceString((UINT)IDS_I_LOCALNODE))+1);
   else
      x_strcpy(lpaddrepl->szVNode,szVnodeName);

   if (x_strcmp(lpaddrepl->szVNode,"mobile")==0) {
      hwndUserName  = GetDlgItem (hwnd, IDC_MOBILE_USER_NAME);
      hwndDBName    = GetDlgItem (hwnd, IDC_DB_NAME_EDIT);
      
      nmax = Edit_GetTextLength(hwndUserName);
      if (nmax != 0)
         Edit_GetText(hwndUserName,lpaddrepl->szDBOwner,nmax+1);
      else
         lpaddrepl->szDBOwner[0]=0;
      
      nmax = Edit_GetTextLength(hwndDBName);
      if (nmax != 0)
         Edit_GetText(hwndDBName,lpaddrepl->szDBName,nmax+1);
      else
         lpaddrepl->szDBName[0]=0;
   }
   else  {
      hwndDBName  = GetDlgItem (hwnd, IDC_REPL_CONN_DBNAME);
      ComboBox_GetText (hwndDBName, lpaddrepl->szDBName, sizeof (lpaddrepl->szDBName));
      if (!lpaddrepl->bAlter) {
         int  hdl = OpenNodeStruct (lpaddrepl->szVNode);
         if (hdl>=0) {
            DOMGetDBOwner(hdl, lpaddrepl->szDBName, lpaddrepl->szDBOwner);
            CloseNodeStruct (hdl, FALSE);
         }
      }
   }

   if (!VerifyAllNumericEditControls (hwnd, TRUE, TRUE))
     return FALSE;

   nIdx = ComboBox_GetCurSel(hwndType);
   if (lpaddrepl->nReplicVersion == REPLIC_V10 || 
       lpaddrepl->nReplicVersion == REPLIC_V105)
      lpaddrepl->nType = (int)ComboBox_GetItemData(hwndType, nIdx);
   else
      x_strcpy(lpaddrepl->szDBMStype, (LPUCHAR) ComboBox_GetItemData(hwndType, nIdx));
   GetDlgItemText(hwnd, IDC_REPL_CONN_DESCRIPTION, lpaddrepl->szDescription, sizeof(lpaddrepl->szDescription));

   GetWindowText(hwndNumber, szNumber, sizeof(szNumber));
   lpaddrepl->nNumber = my_atoi(szNumber);

   if (lpaddrepl->bLocalDb == FALSE)
     {
     GetWindowText(hwndServer, szNumber, sizeof(szNumber));
     lpaddrepl->nServer = my_atoi(szNumber);
     }

   return TRUE;

}
static BOOL FillControlsFromStructure (HWND hwnd, LPREPLCONNECTPARAMS lpaddrepl)
{
  HWND hwndVNode     = GetDlgItem (hwnd, IDC_REPL_CONN_VNODE);
  HWND hwndDatabases = GetDlgItem (hwnd, IDC_REPL_CONN_DBNAME);
  HWND hwndType      = GetDlgItem (hwnd, IDC_REPL_CONN_TYPE);
  HWND hwndDesc      = GetDlgItem (hwnd, IDC_REPL_CONN_DESCRIPTION);
  HWND hwndServer    = GetDlgItem (hwnd, IDC_REPL_CONN_SERVER);
  HWND hwndNumber    = GetDlgItem (hwnd, IDC_REPL_CONN_NUMBER);
  LPREPLCONNECTPARAMS lpadd = GetDlgProp(hwnd);
  char szNumber [20];
  int nIndex;
  UCHAR szVnodeWithoutUser[MAXOBJECTNAME*2];
  int Ret;

  // If a default type is supplied then select it.
  if (lpaddrepl->nReplicVersion == REPLIC_V10 || 
      lpaddrepl->nReplicVersion == REPLIC_V105 )  {
    if (lpaddrepl->nType != 0)
        SelectComboBoxItem(hwndType, typeTypes, lpaddrepl->nType);
  }
  else {
    if (lpaddrepl->szDBMStype[0] != '\0') {
      int nbIndex;
      nbIndex = FindItemData( hwndType, lpaddrepl->szDBMStype);
      if (nbIndex != CB_ERR)
         ComboBox_SetCurSel(hwndType, nbIndex);
    }
    //else
    //    ComboBox_SetCurSel(hwndType, 0);
  }

  FillVnodeName (hwndVNode);

  if (lpaddrepl->nReplicVersion == REPLIC_V105 || lpaddrepl->nReplicVersion == REPLIC_V11)  {
       ComboBox_AddString (hwndVNode, "mobile");
  }
  if (lpaddrepl->bLocalDb == TRUE || lpaddrepl->bAlter == TRUE)   {
      if (x_strcmp( lpaddrepl->szVNode,"mobile")==0){
         HideShowDBname(hwnd, TRUE);
         Edit_SetText(GetDlgItem (hwnd, IDC_DB_NAME_EDIT), lpaddrepl->szDBName);
         Edit_SetText(GetDlgItem (hwnd, IDC_MOBILE_USER_NAME), lpaddrepl->szDBOwner);
     }
      else  {
         FillDatabasesOfVnode  (hwndDatabases, lpaddrepl->szVNode,hwndType);
         nIndex=ComboBox_FindStringExact (hwndDatabases, -1, lpaddrepl->szDBName);
         if (nIndex!=CB_ERR)
            ComboBox_SetCurSel(hwndDatabases, nIndex);
         else
            ComboBox_SetCurSel(hwndDatabases, 0);
         Edit_SetText(hwndDatabases, lpaddrepl->szDBName);
      }
    ZEROINIT(szVnodeWithoutUser);
    x_strcpy(szVnodeWithoutUser,lpaddrepl->szVNode);
    RemoveConnectUserFromString(szVnodeWithoutUser);
    RemoveGWNameFromString(szVnodeWithoutUser);

    //nIndex=ComboBox_FindStringExact (hwndVNode, -1, szVnodeWithoutUser);
    nIndex=ComboBox_FindString (hwndVNode, -1, szVnodeWithoutUser);

    if (nIndex!=CB_ERR)
      ComboBox_SetCurSel(hwndVNode, nIndex);
    else
      ComboBox_SetCurSel(hwndVNode, 0);
   
    Edit_SetText(hwndDesc , lpaddrepl->szDescription);
    Edit_SetText(hwndVNode, lpaddrepl->szVNode);
  
    ZEROINIT(szNumber);
    my_itoa(lpaddrepl->nNumber, szNumber, 10);
    Edit_SetText(hwndNumber, szNumber);
    if (lpaddrepl->nReplicVersion == REPLIC_V10 ||
        lpaddrepl->nReplicVersion == REPLIC_V105 )  {
        ZEROINIT(szNumber);
        my_itoa(lpaddrepl->nServer, szNumber, 10);
        Edit_SetText(hwndServer, szNumber);
    }
  }
  else
  {
    LPUCHAR lpCurVnode = GetVirtNodeName (GetCurMdiNodeHandle ());
    ZEROINIT(szVnodeWithoutUser);
    x_strcpy(szVnodeWithoutUser,lpCurVnode);
    RemoveConnectUserFromString(szVnodeWithoutUser);
    RemoveGWNameFromString(szVnodeWithoutUser);
    Ret = ComboBox_SelectString  (hwndVNode, -1, szVnodeWithoutUser);
    if (Ret == CB_ERR)
        ComboBoxSelectFirstStr (hwndVNode);
    //ComboBoxFillDatabases  (hwndDatabases);
    // PS 03/19/98
    // Fill the combobox without the database star.
    FillDatabasesOfVnode  (hwndDatabases, GetVirtNodeName (GetCurMdiNodeHandle ()),hwndType);
    ComboBoxSelectFirstStr (hwndDatabases);
  }
   return TRUE;
}
static int FindItemData( HWND hwndCtl, char * ItemName)
{
  int ItemMax,i;
  char ItemData[9];
  ItemMax=ComboBox_GetCount(hwndCtl);
  for (i=0;i<ItemMax;i++) {
    x_strcpy(ItemData, (LPUCHAR) ComboBox_GetItemData(hwndCtl, i));
    if (x_strcmp(ItemData,ItemName)==0)
      return i;
  }
  return CB_ERR;
}
static int FindData( HWND hwndCtl, char * ItemName)
{
  int ItemMax,i;
  char ItemData[MAXOBJECTNAME];
  
  ItemMax=ComboBox_GetCount(hwndCtl);
  for (i=0;i<ItemMax;i++) {
    ComboBox_GetLBText(hwndCtl, i, ItemData);
    if (x_stricmp(ItemData,ItemName)==0)
      return i;
  }
  return CB_ERR;
}

static void HideShowDBname(HWND hwnd,BOOL bState)
{
   char szNumber[20];
   HWND hwndServer = GetDlgItem(hwnd, IDC_REPL_CONN_SERVER);
   LPREPLCONNECTPARAMS lpaddrepl = GetDlgProp(hwnd);

    if ( bState == TRUE) {
       ShowWindow( GetDlgItem (hwnd,IDC_REPL_CONN_DBNAME)  ,SW_HIDE);
       ShowWindow( GetDlgItem (hwnd,IDC_DB_NAME_EDIT)      ,SW_SHOW);
       Edit_LimitText(GetDlgItem (hwnd,IDC_DB_NAME_EDIT),MAX_MOBILE_USER);

       ShowWindow( GetDlgItem (hwnd,IDC_MOBILE_USER_NAME)  ,SW_SHOW);
       ShowWindow( GetDlgItem (hwnd,IDC_MOBILE_USER_STATIC),SW_SHOW);

       Edit_GetText(hwndServer, szNumber, sizeof(szNumber));
       lpaddrepl->nServer = my_atoi(szNumber);
       if (lpaddrepl->nServer >0){
         MessageBox(hwnd,ResourceString ((UINT)IDS_ERR_SELECTED_MOBILE),//"You have selected mobile: server is reset to 0"
                         NULL, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);

         Edit_SetText(hwndServer, "0");
       }
       EnableWindow (GetDlgItem (hwnd, IDC_REPL_CONN_SPIN_SERVER) , FALSE);
       EnableWindow (GetDlgItem (hwnd, IDC_REPL_CONN_SERVER) , FALSE);
       Edit_LimitText(GetDlgItem (hwnd,IDC_MOBILE_USER_NAME),MAX_MOBILE_USER);
       if (lpaddrepl->nReplicVersion == REPLIC_V11)  {
         int nbIndex;
         HWND hwndType = GetDlgItem (hwnd, IDC_REPL_CONN_TYPE);

         nbIndex = FindItemData( hwndType, "opingdt");
         if (nbIndex != CB_ERR)
            ComboBox_SetCurSel(hwndType, nbIndex);

         EnableWindow (hwndType , FALSE);
       }
    }
    else {
       ShowWindow( GetDlgItem (hwnd,IDC_REPL_CONN_DBNAME)  ,SW_SHOW);
       ShowWindow( GetDlgItem (hwnd,IDC_DB_NAME_EDIT)      ,SW_HIDE);

       ShowWindow( GetDlgItem (hwnd,IDC_MOBILE_USER_NAME)  ,SW_HIDE);
       ShowWindow( GetDlgItem (hwnd,IDC_MOBILE_USER_STATIC),SW_HIDE);
       ZEROINIT(szNumber);

       my_itoa(lpaddrepl->nServer, szNumber, 10);
       Edit_SetText(hwndServer, szNumber);
       EnableWindow (GetDlgItem (hwnd, IDC_REPL_CONN_SPIN_SERVER) , TRUE);
       EnableWindow (GetDlgItem (hwnd, IDC_REPL_CONN_SERVER) , TRUE);
       if (lpaddrepl->nReplicVersion == REPLIC_V11)  {
         //SelectDefaultType(hwnd);
         EnableWindow (GetDlgItem (hwnd, IDC_REPL_CONN_TYPE) , TRUE);
       }
    }
}
