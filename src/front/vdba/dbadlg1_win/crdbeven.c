/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : crdbeven.c
//
//    Dialog box for creating Database Event (DBevent)
//
********************************************************************/


#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "getdata.h"
#include "msghandl.h"
#include "lexical.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "dom.h"
#include "esltools.h"
#include "winutils.h"


BOOL CALLBACK __export DlgCreateDBeventDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy (HWND hwnd);
static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

static BOOL OnInitDialog (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange (HWND hwnd);
static void InitialiseEditControls (HWND hwnd);

int WINAPI __export DlgCreateDBevent (HWND hwndOwner, LPDBEVENTPARAMS lpdbeventparams)
{
   FARPROC lpProc;
   int     retVal;

	if (!IsWindow (hwndOwner) || !lpdbeventparams)
	{
		ASSERT(NULL);
		return FALSE;
	}
   lpProc = MakeProcInstance ((FARPROC) DlgCreateDBeventDlgProc, hInst);
   retVal = VdbaDialogBoxParam
               (hResource,
                MAKEINTRESOURCE (IDD_DBEVENT),
                hwndOwner,
                (DLGPROC) lpProc,
                (LPARAM)  lpdbeventparams
               );

   FreeProcInstance (lpProc);
   return (retVal);
}





BOOL CALLBACK __export DlgCreateDBeventDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
       HANDLE_MSG (hwnd, WM_COMMAND, OnCommand);
       HANDLE_MSG (hwnd, WM_DESTROY, OnDestroy);
           
       case WM_INITDIALOG:
           return HANDLE_WM_INITDIALOG (hwnd, wParam, lParam, OnInitDialog);
               
       default:
           return FALSE;
   }
   return TRUE;
}


static BOOL OnInitDialog (HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   LPDBEVENTPARAMS lpdbevent = (LPDBEVENTPARAMS)lParam;
   char szTitle [MAX_TITLEBAR_LEN];
   char szFormat [100];

   if (!AllocDlgProp (hwnd, lpdbevent))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();

   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_DBEVENT));
   InitialiseEditControls (hwnd);

   LoadString (hResource, (UINT)IDS_T_CREATE_DBEVENT, szFormat, sizeof (szFormat));
   
   wsprintf (szTitle, szFormat,
       GetVirtNodeName ( GetCurMdiNodeHandle ()),
       lpdbevent->DBName);
   SetWindowText (hwnd, szTitle);

   //
   // Disable the OK button if there is no text in the DB event name control
   //

   if (Edit_GetTextLength (GetDlgItem (hwnd, IDC_DBEVENT_NAME)) == 0)
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);

   richCenterDialog (hwnd);

   return TRUE;
}


static void OnDestroy (HWND hwnd)
{
   DeallocDlgProp (hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPDBEVENTPARAMS lpdbevent = GetDlgProp (hwnd);

   switch (id)
   {
       case IDOK:
       {
           HWND hwndDBevent = GetDlgItem (hwnd, IDC_DBEVENT_NAME);
           char szEventName  [MAXOBJECTNAME -1];

           Edit_GetText (hwndDBevent, szEventName, sizeof (szEventName));
           if (!IsObjectNameValid (szEventName, OT_UNKNOWN))
           {
               MsgInvalidObject ((UINT)IDS_E_NOT_A_VALIDE_OBJECT);
               Edit_SetSel (hwndDBevent, 0, -1);
               SetFocus (hwndDBevent);
               break;
           }
           x_strcpy (lpdbevent->ObjectName, QuoteIfNeeded(szEventName));
           //
           // Call the low level function to write/update data on disk
           //
           if (lpdbevent->bCreate)
           {
               //
               // Add object
               //
               int ires;
               ires = DBAAddObject
                       ( GetVirtNodeName ( GetCurMdiNodeHandle ()),
                         OT_DBEVENT,
                         (void *) lpdbevent );

               if (ires != RES_SUCCESS)
               {
                   ErrorMessage ((UINT) IDS_E_CREATE_DBEVENT_FAILED, ires);
                   break;
               }
               else
               {
                   EndDialog (hwnd, TRUE);
               }
           }
       }    
       break;

       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           break;

       case IDC_DBEVENT_NAME:
       {
           switch (codeNotify)
           {
               case EN_CHANGE:
               {
                   BOOL bEnable = TRUE;
                   if (Edit_GetTextLength (hwndCtl) == 0)
                       bEnable = FALSE;
                   EnableWindow(GetDlgItem(hwnd, IDOK), bEnable);
                   break;
               }
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
   HWND hwndDBeventName        = GetDlgItem (hwnd, IDC_DBEVENT_NAME);
   LPDBEVENTPARAMS lpdbevent   = GetDlgProp (hwnd);

   Edit_LimitText (hwndDBeventName , MAXOBJECTNAME -1);
   SetDlgItemText (hwnd, IDC_DBEVENT_NAME, lpdbevent->ObjectName);
}






