/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : grantdbe.c
//
//    Dialog box for Database event grant.
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


BOOL CALLBACK __export DlgGrantDBeventDlgProc
               (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy       (HWND hwnd);
static void OnCommand       (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog    (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);
static void EnableDisableOKButton      (HWND hwnd);
static BOOL FillStructureFromControls  (HWND hwnd, LPGRANTPARAMS lpgrant);


int WINAPI __export DlgGrantDBevent    (HWND hwndOwner, LPGRANTPARAMS lpgrant)
/*
// Function:
// Shows the Grant (Table or View) dialog.
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

   lpProc = MakeProcInstance ((FARPROC) DlgGrantDBeventDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_GRANT_DBEVENT),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpgrant
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgGrantDBeventDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
   HWND hwndGrantees  = GetDlgItem (hwnd, IDC_GRANT_DBEVENT_GRANTEES);
   HWND hwndDBevents  = GetDlgItem (hwnd, IDC_GRANT_DBEVENT_DBEVENT);
   char szTitle [MAX_TITLEBAR_LEN];
   char szFormat[100];
   //
   // force catolist.dll to load
   //
   CATOListDummy();

   if (!AllocDlgProp (hwnd, lpgrant))
       return FALSE;

   LoadString (hResource, (UINT)IDS_T_GRANT_DBEVENT, szFormat, sizeof (szFormat));
   wsprintf (szTitle,
       szFormat,
       GetVirtNodeName (GetCurMdiNodeHandle ()),
       lpgrant->DBName);
   SetWindowText (hwnd, szTitle);
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_GRANT_DBEVENT));

   Button_SetCheck (GetDlgItem (hwnd, IDC_GRANT_DBEVENT_USER), TRUE);
   FillGrantees (hwndGrantees, OT_USER);

   if (!CAListBoxFillDBevents (hwndDBevents, lpgrant->DBName))
       CAListBoxDestroyItemData (hwndDBevents);

   if (lpgrant->PreselectPrivileges [GRANT_RAISE])
       Button_SetCheck (GetDlgItem (hwnd, IDC_GRANT_DBEVENT_RAISE),    TRUE);
   if (lpgrant->PreselectPrivileges [GRANT_REGISTER])
       Button_SetCheck (GetDlgItem (hwnd, IDC_GRANT_DBEVENT_REGISTER), TRUE);

   if (x_strlen (lpgrant->PreselectGrantee) > 0)
       CAListBox_SelectString (hwndGrantees, -1, lpgrant->PreselectGrantee);
   if (x_strlen (lpgrant->PreselectObject) > 0)
       CAListBoxSelectStringWithOwner (hwndDBevents, lpgrant->PreselectObject, lpgrant->PreselectObjectOwner);
   EnableDisableOKButton  (hwnd);
   
   richCenterDialog(hwnd);
   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   HWND hwndDBevents  = GetDlgItem (hwnd, IDC_GRANT_DBEVENT_DBEVENT);

   CAListBoxDestroyItemData (hwndDBevents);
   DeallocDlgProp (hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPGRANTPARAMS lpgrant = GetDlgProp (hwnd);
   HWND hwndGrantees  = GetDlgItem (hwnd, IDC_GRANT_DBEVENT_GRANTEES);
   HWND hwndDBevents  = GetDlgItem (hwnd, IDC_GRANT_DBEVENT_DBEVENT);
   int  ires;

   switch (id)
   {
       case IDOK:
           if (!FillStructureFromControls (hwnd, lpgrant))
               break;
           
           ires = DBAAddObject
                (GetVirtNodeName (GetCurMdiNodeHandle ()),
                 OTLL_GRANT,
                 (void *) lpgrant);

           if (ires != RES_SUCCESS)
           {
               FreeAttachedPointers (lpgrant, OTLL_GRANT);
               ErrorMessage ((UINT)IDS_E_GRANT_DBEVENT_FAILED, RES_ERR);
               break;
           }
           else 
               EndDialog (hwnd, TRUE);
           
           FreeAttachedPointers (lpgrant, OTLL_GRANT);
           break;

       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           break;

       case IDC_GRANT_DBEVENT_RAISE:
       case IDC_GRANT_DBEVENT_REGISTER:
           EnableDisableOKButton (hwnd);
           break;

       case IDC_GRANT_DBEVENT_GRANTEES:
       case IDC_GRANT_DBEVENT_DBEVENT:
           EnableDisableOKButton (hwnd);
           break;

       case IDC_GRANT_DBEVENT_USER:
           CAListBox_ResetContent (hwndGrantees);
           FillGrantees (hwndGrantees, OT_USER);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_GRANT_DBEVENT_GROUP:
           CAListBox_ResetContent (hwndGrantees);
           FillGrantees (hwndGrantees, OT_GROUP);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_GRANT_DBEVENT_ROLE:
           CAListBox_ResetContent (hwndGrantees);
           FillGrantees (hwndGrantees, OT_ROLE);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_GRANT_DBEVENT_PUBLIC:
           CAListBox_ResetContent (hwndGrantees);
           EnableDisableOKButton  (hwnd);
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
   HWND    hwndGrantees  = GetDlgItem (hwnd, IDC_GRANT_DBEVENT_GRANTEES);
   HWND    hwndDBevents  = GetDlgItem (hwnd, IDC_GRANT_DBEVENT_DBEVENT);
   BOOL    check [2];
   BOOL    B [3];
   BOOL    C [2];

   check [0] = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_DBEVENT_RAISE   ));
   check [1] = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_DBEVENT_REGISTER));
   B[0] = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_DBEVENT_PUBLIC));
   B[1] = (CAListBox_GetSelCount (hwndGrantees) >= 1);
   B[2] = (CAListBox_GetSelCount (hwndDBevents) >= 1);

   C[0] = !B[0] && B[1] && B[2] && (check [0] || check [1]);
   C[1] =  B[0] && B[2] && (check [0] || check [1]);

   if (C[0] || C[1])
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);

   if (B[0])
   {
       CAListBox_ResetContent (hwndGrantees);
       EnableWindow (hwndGrantees, FALSE);
   }
   else
       EnableWindow (hwndGrantees, TRUE);
}

static BOOL FillStructureFromControls (HWND hwnd, LPGRANTPARAMS lpgrant)
{
   HWND hwndGrantees    = GetDlgItem (hwnd, IDC_GRANT_DBEVENT_GRANTEES);
   HWND hwndDBevents    = GetDlgItem (hwnd, IDC_GRANT_DBEVENT_DBEVENT);
   int  max_item_number = CAListBox_GetSelCount (hwndDBevents);

   if (max_item_number >= 1)
   {
       lpgrant->lpobject   = AddItemToListWithOwner (hwndDBevents);
       if (!lpgrant->lpobject)
       {
           FreeAttachedPointers (lpgrant, OTLL_GRANT);
           ErrorMessage   ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
           return FALSE;
       }
   }

   max_item_number = CAListBox_GetSelCount (hwndGrantees);
   if (max_item_number >= 1)
   {
       lpgrant->lpgrantee = AddItemToListQuoted (hwndGrantees);
       if (!lpgrant->lpgrantee)
       {
           FreeAttachedPointers (lpgrant, OTLL_GRANT);
           ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
           EndDialog (hwnd, FALSE);
           return FALSE;
       }
   }
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_DBEVENT_USER)))
       lpgrant->GranteeType = OT_USER;
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_DBEVENT_GROUP)))
       lpgrant->GranteeType = OT_GROUP;
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_DBEVENT_ROLE)))
       lpgrant->GranteeType = OT_ROLE;
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_DBEVENT_PUBLIC)))
   {
       lpgrant->GranteeType = OT_PUBLIC;     // Public
       FreeObjectList (lpgrant->lpgrantee);
       lpgrant->lpgrantee= NULL;
       lpgrant->lpgrantee = APublicUser ();
       if (!lpgrant->lpgrantee)
       {
           FreeAttachedPointers (lpgrant, OTLL_GRANT);
           ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
           return FALSE;
       }
   }
   
   lpgrant->Privileges [GRANT_RAISE]    = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_DBEVENT_RAISE));
   lpgrant->Privileges [GRANT_REGISTER] = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_DBEVENT_REGISTER));
   lpgrant->grant_option = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_DBEVENT_OPTION));

   return TRUE;
}

