/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : session.c
//
//    Session setting dialog box
//
********************************************************************/


#include <ctype.h>
#include <stdlib.h>
#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "msghandl.h"
#include "lexical.h"
#include "catospin.h"
#include "dbaginfo.h"

#define SESSION_TIMEOUT_MIN 0
#define SESSION_TIMEOUT_MAX 65536
#define SESSION_WIDTH       10 
#define SessionMin          3           
#define SessionMax          MAXSESSIONS



// Structure describing the numeric edit controls
static EDITMINMAX limits[] =
{
   IDC_SESSION_SESSIONNUMBER, IDC_SESSION_SESSIONSPIN, SessionMin, SessionMax,
   IDC_SESSION_TIMEOUTNUMBER, IDC_SESSION_TIMEOUTSPIN, SESSION_TIMEOUT_MIN, SESSION_TIMEOUT_MAX,
   -1// terminator
};


BOOL CALLBACK __export DlgSessionDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy (HWND hwnd);
static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange (HWND hwnd);
static void InitializeControls (HWND hwnd);
static void InitialiseSpinControls (HWND hwnd);
static void EnableDisableOKButton (HWND hwnd);



int WINAPI __export DlgSession (HWND hwndOwner, LPSESSIONPARAMS lpsession)
/*
// Function:
// Shows the Session dialog.
//
// Paramters:
//     1) hwndOwner:   Handle of the parent window for the dialog.
//     1) lpsession:   Points to structure containing information used to 
//                     initialise the dialog. 
//
// Returns:
//     (IDOK, IDCANCEL) -1 if fail.
//
*/
{
   FARPROC lpProc;
   int     retVal;
   
   if (!IsWindow (hwndOwner) || !lpsession)
   {
       ASSERT (NULL);
       return -1;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgSessionDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_SESSION),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpsession
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgSessionDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
       HANDLE_MSG (hwnd, WM_COMMAND, OnCommand);
       HANDLE_MSG (hwnd, WM_DESTROY, OnDestroy);

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


static BOOL OnInitDialog (HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   LPSESSIONPARAMS lpsession = (LPSESSIONPARAMS)lParam;
   HWND hwndSession = GetDlgItem (hwnd, IDC_SESSION_SESSIONNUMBER);
   HWND hwndTimeOut = GetDlgItem (hwnd, IDC_SESSION_TIMEOUTNUMBER);
   char szTitle [MAX_TITLEBAR_LEN];
   char str [10];

   if (!AllocDlgProp (hwnd, lpsession))
       return FALSE;
   
   SpinGetVersion();
   LoadString (hResource, (UINT)IDS_T_SESSION_SETTING, szTitle, sizeof (szTitle));
   SetWindowText (hwnd, szTitle);

   //Edit_LimitText (hwndSession, SESSION_WIDTH);
   InitialiseSpinControls (hwnd);
   InitializeControls (hwnd);

   if (lpsession && (lpsession->number >0))
   {
       wsprintf (str, "%d", lpsession->number);
       Edit_SetText (hwndSession, str);
   }
   else
   {
       Edit_SetSel (hwndSession, 0, -1);
   }

   if (lpsession && (lpsession->time_out >0))
   {
       wsprintf (str, "%d", lpsession->time_out);
       Edit_SetText (hwndTimeOut, str);
   }
   else
   {
       Edit_SetSel (hwndTimeOut, 0, -1);
   }

   EnableDisableOKButton (hwnd);
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_SESSION));
       
   richCenterDialog(hwnd);
   return TRUE;
}


static void OnDestroy (HWND hwnd)
{
   SubclassAllNumericEditControls (hwnd, EC_RESETSUBCLASS);
   DeallocDlgProp (hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPSESSIONPARAMS lpsession = GetDlgProp (hwnd);
   HWND    hwndSession = GetDlgItem (hwnd, IDC_SESSION_SESSIONNUMBER);
   HWND    hwndTimeOut = GetDlgItem (hwnd, IDC_SESSION_TIMEOUTNUMBER);
   char    szSession  [SESSION_WIDTH+1];
   char    szTimeOut  [SESSION_WIDTH+1];

   switch (id)
   {
       case IDOK:
       {
           int inewsessions;
           Edit_GetText (hwndSession, szSession, sizeof (szSession));
           inewsessions = atoi (szSession);
           if (inewsessions!=lpsession->number) {
              if (DBACloseNodeSessions("",FALSE,FALSE)!=RES_SUCCESS) {
                ErrorMessage ((UINT)IDS_E_CLOSING_SESSION, RES_ERR);
                break;
              }
           }
           lpsession->number  = inewsessions;

           Edit_GetText (hwndTimeOut, szTimeOut, sizeof (szTimeOut));
           lpsession->time_out = atoi (szTimeOut);
           EndDialog (hwnd, TRUE);
       }
       break;

       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           break;

       case IDC_SESSION_SESSIONNUMBER:
       case IDC_SESSION_TIMEOUTNUMBER:
           switch (codeNotify)
           {
               case EN_CHANGE:
               {
                   int  nCount;

                   if (!IsWindowVisible(hwnd))
                       break;

                   // Test to see if the control is empty. If so then
                   // break out.  It becomes tiresome to edit
                   // if you delete all characters and an error box pops up.
                   
                   EnableDisableOKButton (hwnd);
                   nCount = Edit_GetTextLength(hwndCtl);
                   if (nCount == 0)
                       break;

                   VerifyAllNumericEditControls (hwnd, TRUE, TRUE);
                   break;
               }

               case EN_KILLFOCUS:
               {
                   HWND hwndNew = GetFocus();
                   int nNewCtl  = GetDlgCtrlID (hwndNew);

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

       case IDC_SESSION_SESSIONSPIN:
       case IDC_SESSION_TIMEOUTSPIN:
           ProcessSpinControl (hwndCtl, codeNotify, limits);
           break;    
   }
}


static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}



BOOL LoadSessionSettings (LPSESSIONPARAMS lpsess)
{
   char session_number [10];
   char timeout_number [10];

   if (!lpsess)
       return FALSE;
   
   GetPrivateProfileString ("SESSION", "SessionNumber", "8", session_number, sizeof (session_number), inifilename());
   GetPrivateProfileString ("SESSION", "Time_out", "0", timeout_number, sizeof (timeout_number), inifilename());

   lpsess->number = atoi (session_number);
   if (lpsess->number<1 || lpsess->number>MAXSESSIONS)
     lpsess->number = MAXSESSIONS;
   imaxsessions=(int)lpsess->number;
   lpsess->time_out = atol (timeout_number);

   return TRUE;
}

BOOL WriteSessionSettings (LPSESSIONPARAMS lpsess)
{
   BOOL succ, succ2;
   char session_number [10];
   char timeout_number [10];

   if (!lpsess)
       return FALSE;

   wsprintf (session_number, "%d", lpsess->number);
   wsprintf (timeout_number, "%d", lpsess->time_out);

   succ  = WritePrivateProfileString ("SESSION", "SessionNumber", session_number, inifilename());
   succ2 = WritePrivateProfileString ("SESSION", "Time_out", timeout_number, inifilename());

   if (!(succ && succ2))
       return FALSE;

   return TRUE;
}

BOOL LoadCurrentSessionSettings (LPSESSIONPARAMS lpsess)
{

   return TRUE;
}


static void InitializeControls (HWND hwnd)
{
   HWND hwndSessionNumber = GetDlgItem (hwnd, IDC_SESSION_SESSIONNUMBER);
   HWND hwndTimeOutNumber = GetDlgItem (hwnd, IDC_SESSION_TIMEOUTNUMBER);

   SubclassAllNumericEditControls (hwnd, EC_SUBCLASS);
   
   Edit_LimitText (hwndSessionNumber, 5);
   Edit_LimitText (hwndTimeOutNumber, 5);
   LimitNumericEditControls (hwnd);
}

static void InitialiseSpinControls (HWND hwnd)
{
   DWORD dwMin, dwMax;

   GetEditCtrlMinMaxValue   (hwnd, GetDlgItem (hwnd, IDC_SESSION_SESSIONNUMBER), limits, &dwMin, &dwMax);
   SpinRangeSet (GetDlgItem (hwnd, IDC_SESSION_SESSIONSPIN), dwMin, dwMax);

   GetEditCtrlMinMaxValue   (hwnd, GetDlgItem (hwnd, IDC_SESSION_TIMEOUTNUMBER), limits, &dwMin, &dwMax);
   SpinRangeSet (GetDlgItem (hwnd, IDC_SESSION_TIMEOUTSPIN), dwMin, dwMax);

}

static void EnableDisableOKButton (HWND hwnd)
{
   HWND hwndSession = GetDlgItem (hwnd, IDC_SESSION_SESSIONNUMBER);
   HWND hwndTimeOut = GetDlgItem (hwnd, IDC_SESSION_TIMEOUTNUMBER);

   if (Edit_GetTextLength (hwndSession) == 0)
       //|| (Edit_GetTextLength (hwndTimeOut) == 0))
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
}
