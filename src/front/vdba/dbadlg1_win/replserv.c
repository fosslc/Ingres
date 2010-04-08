/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : replserv.c
//
//    Modify the Target Type and server number associate with database.
//
********************************************************************/

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


/*
#define REPLIC_FULLPEER      1
#define REPLIC_PROT_RO       2
#define REPLIC_UNPROT_RO     3
#define REPLIC_EACCESS       4
*/

#define MIN_SERVER 0
#define MAX_SERVER 999

// Structure describing the numeric edit controls
static EDITMINMAX limits[] =
{
   IDC_SERVER_NUMBER, IDC_CA_SERVER, MIN_SERVER, MAX_SERVER,
   -1// terminator
};
static EDITMINMAX limits2[] =
{
   IDC_SERVER_NUMBER, IDC_CA_SERVER, 1, MAX_SERVER,
   -1// terminator
};

CTLDATA typeTypesTarget[] =
{
   REPLIC_FULLPEER,    IDS_FULLPEER,
   REPLIC_PROT_RO,     IDS_PROTREADONLY,
   REPLIC_UNPROT_RO,   IDS_UNPROTREADONLY,
   -1                  // terminator
};

BOOL CALLBACK __export DlgReplServerDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy                  (HWND hwnd);
static void OnCommand                  (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog               (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange           (HWND hwnd);

static void InitialiseSpinControls     (HWND hwnd);
static void InitialiseEditControls     (HWND hwnd);
static BOOL OccupyTypesControl         (HWND hwnd);

BOOL WINAPI __export DlgReplServer (HWND hwndOwner, LPDD_CONNECTION lpConn)
/*
//   Function:
//   Shows the Type and server dialog.
//
//   Paramters:
//       hwndOwner - Handle of the parent window for the dialog.
//       lpConn    - Points to structure containing information used to 
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

   if (!IsWindow(hwndOwner) || !lpConn)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgReplServerDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_REPL_TYPE_AND_SERVER),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpConn
       );

   FreeProcInstance (lpProc);
   return (retVal);
}

BOOL CALLBACK __export DlgReplServerDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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

           if (GetOIVers() == OIVERS_12) 
              return GetEditCtrlMinMaxValue (hwnd, hwndCtl, limits, &lpdesc->dwMin, &lpdesc->dwMax);
           else 
              return GetEditCtrlMinMaxValue (hwnd, hwndCtl, limits2, &lpdesc->dwMin, &lpdesc->dwMax);
       }

       case LM_GETEDITSPINCTRL:
       {
           HWND hwndEdit = (HWND) wParam;
           HWND FAR * lphwnd = (HWND FAR *)lParam;
           if (GetOIVers() == OIVERS_12) 
              *lphwnd = GetEditSpinControl(hwndEdit, limits);
           else 
              *lphwnd = GetEditSpinControl(hwndEdit, limits2);
           break;
       }

       default:
           return HandleUserMessages(hwnd, message, wParam, lParam);
   }
   return TRUE;
}
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   LPDD_CONNECTION lpadd = (LPDD_CONNECTION)lParam;
   char szTitle [MAX_TITLEBAR_LEN];

   if (!AllocDlgProp(hwnd, lpadd))
       return FALSE;

   lpHelpStack = StackObject_PUSH (lpHelpStack,
                                   StackObject_INIT ((UINT)IDD_REPL_TYPE_AND_SERVER));
   
   // Force the catospin.dll to load
   SpinGetVersion();

   if (!OccupyTypesControl(hwnd))
   {
       ASSERT(NULL);
       return FALSE;
   }

   wsprintf (
       szTitle,
       ResourceString ((UINT)IDS_T_REPL_TYPE_AND_SERVER),
       lpadd->szVNode,
       lpadd->szDBName);
   SetWindowText (hwnd, szTitle);

   InitialiseSpinControls (hwnd);
   InitialiseEditControls (hwnd);

   if (lpadd->bMustRemainFullPeer)
      EnableWindow(GetDlgItem(hwnd,IDC_TARGET_TYPE),FALSE);
   richCenterDialog(hwnd);

   return TRUE;
}
static void OnDestroy(HWND hwnd)
{
   SubclassAllNumericEditControls (hwnd, EC_RESETSUBCLASS);

   ResetSubClassEditCntl(hwnd, IDC_SERVER_NUMBER);

   DeallocDlgProp(hwnd);
   
   lpHelpStack = StackObject_POP (lpHelpStack);

}

static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
  int iDx;
  char szNumber [20];
  LPDD_CONNECTION lpconn = GetDlgProp(hwnd);
  HWND hwndTarget        = GetDlgItem (hwnd, IDC_TARGET_TYPE);
  HWND hwndServer        = GetDlgItem(hwnd, IDC_SERVER_NUMBER);

  switch (id)
  {
    case IDOK:
    {
        iDx = ComboBox_GetCurSel(hwndTarget);
        if (iDx==LB_ERR)
          break;
        lpconn->nTypeRepl = ComboBox_GetItemData(hwndTarget, iDx);
        
        GetWindowText(hwndServer, szNumber, sizeof(szNumber));
        lpconn->nServerNo = my_atoi(szNumber);
        EndDialog (hwnd, TRUE);
    }
    break;
    case IDCANCEL:
        EndDialog (hwnd, FALSE);
        break;

    case IDC_SERVER_NUMBER:
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
    }
    break;
    case IDC_CA_SERVER:
    {
        if (GetOIVers() == OIVERS_12) 
           ProcessSpinControl(hwndCtl, codeNotify, limits);
        else 
           ProcessSpinControl(hwndCtl, codeNotify, limits2);
        break;
    }
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
   LPDD_CONNECTION lpconn = GetDlgProp(hwnd);
   HWND hwndServer = GetDlgItem(hwnd, IDC_SERVER_NUMBER);

   // Subclass the edit controls
   SubclassAllNumericEditControls(hwnd, EC_SUBCLASS);
   
   // Limit the text in the edit controls
   LimitNumericEditControls(hwnd);

   my_itoa(lpconn->nServerNo, szNumber, 10);
   SetDlgItemText(hwnd, IDC_SERVER_NUMBER, szNumber);

}


static void InitialiseSpinControls (HWND hwnd)
{
   DWORD dwMin, dwMax;
   LPDD_CONNECTION lpadd = GetDlgProp(hwnd);


   if (GetOIVers() == OIVERS_12) 
      GetEditCtrlMinMaxValue  (hwnd, GetDlgItem(hwnd, IDC_SERVER_NUMBER), limits, &dwMin, &dwMax);
   else 
      GetEditCtrlMinMaxValue  (hwnd, GetDlgItem(hwnd, IDC_SERVER_NUMBER), limits2, &dwMin, &dwMax);
   SpinRangeSet(GetDlgItem (hwnd, IDC_CA_SERVER), dwMin, dwMax);

   SpinPositionSet (GetDlgItem (hwnd, IDC_CA_SERVER), lpadd->nServerNo);
}



static BOOL OccupyTypesControl (HWND hwnd)
/*
// Function:
//     Fills the types drop down box with the type names.
//
// Parameters:
//     hwnd - Handle to the dialog window.
// 
// Returns:
//     TRUE if successful.
*/
{
  int iret;
  HWND hwndCtl = GetDlgItem (hwnd, IDC_TARGET_TYPE);
  LPDD_CONNECTION lpconn = GetDlgProp(hwnd);
  
  iret = OccupyComboBoxControl(hwndCtl, typeTypesTarget);
  ComboBox_SetCurSel(hwndCtl, (lpconn->nTypeRepl-1));
  return iret;
}
