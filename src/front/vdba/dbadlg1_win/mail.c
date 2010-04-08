/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : mail.c
//
//    Add replication error alert user dialog
//
********************************************************************/


#include <ctype.h>
#include <stdlib.h>
#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "getdata.h"
#include "msghandl.h"
#include "lexical.h"
#include "dba.h"
#include "domdata.h"
#include "dbaginfo.h"





BOOL CALLBACK __export DlgMailDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy (HWND hwnd);
static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange (HWND hwnd);
static void EnableDisableOKButton (HWND hwnd);



int WINAPI __export DlgMail (HWND hwndOwner, LPREPLMAILPARAMS lpmail)
/*
// Function:
// Shows the Session dialog.
//
// Paramters:
//     1) hwndOwner:   Handle of the parent window for the dialog.
//     1) lpmail:      Points to structure containing information used to 
//                     initialise the dialog. 
//
// Returns:
//     TRUE if successful.
//
*/
{
   FARPROC lpProc;
   int     retVal;
   
   if (!IsWindow (hwndOwner) || !lpmail)
   {
       ASSERT (NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgMailDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_MAIL),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpmail
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgMailDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
   LPREPLMAILPARAMS lpmail = (LPREPLMAILPARAMS)lParam;
   HWND hwndMailText   = GetDlgItem (hwnd, IDC_MAIL_TEXT);
   char szFormat[100];
   char szTitle [MAX_TITLEBAR_LEN];

   if (!AllocDlgProp (hwnd, lpmail))
       return FALSE;

   LoadString (hResource, (UINT)IDS_T_MAIL, szFormat, sizeof (szFormat));
   wsprintf (szTitle,
       szFormat,
       GetVirtNodeName (GetCurMdiNodeHandle ()),
       lpmail->DBName);
   SetWindowText (hwnd, szTitle);
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_MAIL));

   Edit_LimitText (hwndMailText, MAX_MAIL_TEXTLEN-1);
   EnableDisableOKButton (hwnd);
       
   richCenterDialog(hwnd);
   return TRUE;
}


static void OnDestroy (HWND hwnd)
{
   DeallocDlgProp (hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPREPLMAILPARAMS lpmail  = GetDlgProp (hwnd);
   HWND    hwndMailText = GetDlgItem (hwnd, IDC_MAIL_TEXT);
   char    szText  [MAX_MAIL_TEXTLEN];

   switch (id)
   {
       case IDOK:
       {
           int ires;

           Edit_GetText (hwndMailText, szText, sizeof (szText));
           x_strcpy (lpmail->szMailText, szText);
           ires = DBAAddObject
               ( GetVirtNodeName ( GetCurMdiNodeHandle ()),
                 OT_REPLIC_MAILUSER,
                 (void *) lpmail);

           if (ires != RES_SUCCESS)
           {
               ErrorMessage ((UINT) IDS_E_REPLMAIL_FAILED, ires);
               break;
           }
           else
           {
               EndDialog (hwnd, TRUE);
           }
       }
       break;

       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           break;

       case IDC_MAIL_TEXT:
           EnableDisableOKButton (hwnd);
           break;
   }
}


static void OnSysColorChange (HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}


static void EnableDisableOKButton (HWND hwnd)
{
   HWND hwndMailTaxt = GetDlgItem (hwnd, IDC_MAIL_TEXT);
   char szText [MAX_MAIL_TEXTLEN];

   Edit_GetText (hwndMailTaxt, szText, sizeof (szText));

   if (x_strlen (szText) == 0)
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
}




