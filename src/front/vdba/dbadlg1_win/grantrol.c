/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : grantrol.c
//
//    Dialog box for <granting role(s) to user(s)>.  
//
//  History after 04-May-1999:
//
//    22-Dec-1999 (schph01)
//       Bug #97680 remplacement de AddItemToList() by AddItemToListQuoted()
//                  for the correct generation of syntax SQL.
//
********************************************************************/

#include <ctype.h>
#include <stdlib.h>
#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "catospin.h"
#include "lexical.h"
#include "getdata.h"
#include "msghandl.h"
#include "domdata.h"
#include "esltools.h"




BOOL CALLBACK __export DlgGrantRole2UserDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy       (HWND hwnd);
static void OnCommand       (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog    (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);
static void EnableDisableOKButton     (HWND hwnd);
static BOOL FillStructureFromControls (HWND hwnd, LPGRANTPARAMS lpgrant);


int WINAPI __export DlgGrantRole2User (HWND hwndOwner, LPGRANTPARAMS lpgrant)
/*
// Function:
// Shows the Grant role to users dialog.
//
// Paramters:
//     1) hwndOwner:   Handle of the parent window for the dialog.
//     1) lpgrant  :   Points to structure containing information used to 
//                     initialise the dialog. 
//
// Returns:
//     (TRUE, FALSE)
//
*/
{
   FARPROC lpProc;
   int     retVal;
   
   if (!IsWindow (hwndOwner) || !lpgrant)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgGrantRole2UserDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_GRANTROL),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpgrant
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgGrantRole2UserDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
   LPGRANTPARAMS lpgrant = (LPGRANTPARAMS)lParam;
   HWND hwndUsers  = GetDlgItem (hwnd, IDC_GRANTROL_USERS);
   HWND hwndRoles  = GetDlgItem (hwnd, IDC_GRANTROL_ROLES);
   char szFormat [100];
   char szTitle  [MAX_TITLEBAR_LEN];
   
   //
   // force catolist.dll to load
   //
   CATOListDummy();

   if (!AllocDlgProp (hwnd, lpgrant))
       return FALSE;

   LoadString (hResource, (UINT)IDS_T_GRANT_ROLE, szFormat, sizeof (szFormat));
   wsprintf (
       szTitle,
       szFormat,
       GetVirtNodeName (GetCurMdiNodeHandle ()));
   SetWindowText (hwnd, szTitle);
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_GRANTROL));

   CAListBoxFillRoles  (GetDlgItem (hwnd, IDC_GRANTROL_ROLES));
   CAListBoxFillUsersP (GetDlgItem (hwnd, IDC_GRANTROL_USERS));

   if (x_strlen (lpgrant->PreselectGrantee) > 0)
   {
       if (x_strcmp (lpgrant->PreselectGrantee, lppublicdispstring()) == 0)
           CAListBox_SelectString (hwndUsers, -1, lppublicsysstring());
       else
           CAListBox_SelectString (hwndUsers, -1, lpgrant->PreselectGrantee);
   }
   if (x_strlen (lpgrant->PreselectObject) > 0)
       CAListBox_SelectString (hwndRoles,  -1, lpgrant->PreselectObject);

   EnableDisableOKButton  (hwnd);
   richCenterDialog (hwnd);
   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   DeallocDlgProp (hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPGRANTPARAMS lpgrant = GetDlgProp (hwnd);
   HWND hwndGrantees     = GetDlgItem (hwnd, IDC_GRANTROL_USERS);
   HWND hwndRoles        = GetDlgItem (hwnd, IDC_GRANTROL_ROLES);
   int  ires;

   switch (id)
   {
       case IDOK:
           FillStructureFromControls (hwnd, lpgrant);
           
           ires = DBAAddObject
                   ( GetVirtNodeName ( GetCurMdiNodeHandle ()),
                     OTLL_GRANT,
                     (void *) lpgrant);

           if (ires != RES_SUCCESS)
           {
               FreeObjectList (lpgrant->lpobject);
               FreeObjectList (lpgrant->lpgrantee);
               lpgrant->lpobject = NULL;
               lpgrant->lpgrantee= NULL;
               ErrorMessage ((UINT)IDS_E_GRANT_ROLE_FAILED, RES_ERR);
               break;
           }
           else 
           {
               EndDialog (hwnd, TRUE);
           }
           FreeObjectList (lpgrant->lpobject);
           FreeObjectList (lpgrant->lpgrantee);
           lpgrant->lpobject = NULL;
           lpgrant->lpgrantee= NULL;
           
           break;

       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           break;

       case IDC_GRANTROL_USERS:
       case IDC_GRANTROL_ROLES:
           if (codeNotify == CALBN_CHECKCHANGE)
               EnableDisableOKButton (hwnd);
           break;
   }
}


static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}


static void EnableDisableOKButton (HWND hwnd)
{
   if ((CAListBox_GetSelCount (GetDlgItem (hwnd, IDC_GRANTROL_USERS)) > 0) &&
       (CAListBox_GetSelCount (GetDlgItem (hwnd, IDC_GRANTROL_ROLES)) > 0))
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);

}

static BOOL FillStructureFromControls (HWND hwnd, LPGRANTPARAMS lpgrant)
{
   lpgrant->lpgrantee  = AddItemToListQuoted (GetDlgItem (hwnd, IDC_GRANTROL_USERS));
   lpgrant->lpobject   = AddItemToListQuoted (GetDlgItem (hwnd, IDC_GRANTROL_ROLES));
   lpgrant->ObjectType = OT_ROLE;
   lpgrant->GranteeType= OT_USER;

   return TRUE;
}

