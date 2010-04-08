/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : grantpro.c
//
//    Dialog box for Executing procedure grant.
//
** 28-Mar-2003 (schph01)
**   SIR 107523 used this dialog also for "NEXT ON SEQUENCE" grant.
**
** 15-Apr-2003 (schph01)
**  bug 110072 Retrieve the "With Grant Option" radio button state
**  for fill the structure.
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

BOOL CALLBACK __export DlgGrantProcedureDlgProc
              (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy       (HWND hwnd);
static void OnCommand       (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog    (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);
static BOOL FillStructureFromControls  (HWND hwnd, LPGRANTPARAMS lpgrant);
static void EnableDisableOKButton      (HWND hwnd);

int WINAPI __export DlgGrantProcedure  (HWND hwndOwner, LPGRANTPARAMS lpgrant)
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
//     TRUE if successful.
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

   lpProc = MakeProcInstance ((FARPROC) DlgGrantProcedureDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_GRANT_PROCEDURE),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpgrant
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgGrantProcedureDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
   HWND hwndGrantees     = GetDlgItem (hwnd, IDC_GRANT_PROCEDURE_GRANTEES);
   HWND hwndProcedures   = GetDlgItem (hwnd, IDC_GRANT_PROCEDURE_PROCEDURES);
   char szTitle [MAX_TITLEBAR_LEN];
   char szFormat[100];

   if (!AllocDlgProp (hwnd, lpgrant))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();

   if (lpgrant->ObjectType == OT_PROCEDURE)
      LoadString (hResource, (UINT)IDS_T_GRANT_PROCEDURE, szFormat, sizeof (szFormat));
   else
      LoadString (hResource, (UINT)IDS_T_GRANT_SEQUENCE, szFormat, sizeof (szFormat));

   wsprintf (szTitle,
       szFormat,
       GetVirtNodeName (GetCurMdiNodeHandle ()),
       lpgrant->DBName);
   SetWindowText (hwnd, szTitle);

   if (lpgrant->ObjectType == OT_PROCEDURE)
      lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_GRANT_PROCEDURE));
   else
      lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_GRANT_SEQUENCE));


   Button_SetCheck (GetDlgItem (hwnd, IDC_GRANT_PROCEDURE_USER), TRUE);
   FillGrantees (hwndGrantees, OT_USER);

   if (lpgrant->ObjectType == OT_PROCEDURE) {
       if (!CAListBoxFillProcedures (hwndProcedures, lpgrant->DBName))
           CAListBoxDestroyItemData (hwndProcedures);
   }
   else {
       ShowWindow( GetDlgItem (hwnd,IDC_GRANT_PROCEDURE_OPTION) ,SW_HIDE);// Hided "With grant Option" control
       szFormat[0]= '\0';
       LoadString (hResource, (UINT)IDS_STATIC_SEQUENCE, szFormat, sizeof (szFormat));
       SetDlgItemText(  hwnd, IDC_STATIC_GRANT_PROCEDURE, szFormat ); 
 
       if (!CAListBoxFillSequences (hwndProcedures, lpgrant->DBName))
           CAListBoxDestroyItemData (hwndProcedures);
   }

   if (x_strlen (lpgrant->PreselectGrantee) > 0)
       CAListBox_SelectString (hwndGrantees, -1, lpgrant->PreselectGrantee);
   if (x_strlen (lpgrant->PreselectObject) > 0)
       CAListBoxSelectStringWithOwner (hwndProcedures, lpgrant->PreselectObject, lpgrant->PreselectObjectOwner);

   EnableDisableOKButton (hwnd);

   richCenterDialog(hwnd);
   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   HWND hwndProcedures= GetDlgItem (hwnd, IDC_GRANT_PROCEDURE_PROCEDURES);

   CAListBoxDestroyItemData (hwndProcedures);
   DeallocDlgProp (hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPGRANTPARAMS lpgrant   = GetDlgProp (hwnd);
   HWND hwndGrantees       = GetDlgItem (hwnd, IDC_GRANT_PROCEDURE_GRANTEES);
   HWND hwndProcedures     = GetDlgItem (hwnd, IDC_GRANT_PROCEDURE_PROCEDURES);
   int  ires;

   switch (id)
   {
       case IDOK:
           if (!FillStructureFromControls (hwnd, lpgrant))
               break;
           ires = DBAAddObject
               (GetVirtNodeName ( GetCurMdiNodeHandle ()),
               OTLL_GRANT,
               (void *) lpgrant);

           if (ires != RES_SUCCESS)
           {
               FreeAttachedPointers (lpgrant, OTLL_GRANT);
               ErrorMessage ((UINT)IDS_E_GRANT_PROCEDURE_FAILED, ires);
               break;
           }
           else 
               EndDialog (hwnd, TRUE);
           FreeAttachedPointers (lpgrant, OTLL_GRANT);
           break;

       case IDCANCEL:
           FreeAttachedPointers (lpgrant, OTLL_GRANT);
           EndDialog (hwnd, FALSE);
           break;

       case IDC_GRANT_PROCEDURE_PROCEDURES:
       case IDC_GRANT_PROCEDURE_GRANTEES:
           EnableDisableOKButton (hwnd);
           break;

       case IDC_GRANT_PROCEDURE_USER:
           CAListBox_ResetContent (hwndGrantees);
           FillGrantees (hwndGrantees, OT_USER);
           EnableDisableOKButton (hwnd);
           break;

       case IDC_GRANT_PROCEDURE_GROUP:
           CAListBox_ResetContent (hwndGrantees);
           FillGrantees (hwndGrantees, OT_GROUP);
           EnableDisableOKButton (hwnd);
           break;

       case IDC_GRANT_PROCEDURE_ROLE:
           CAListBox_ResetContent (hwndGrantees);
           FillGrantees (hwndGrantees, OT_ROLE);
           EnableDisableOKButton (hwnd);
           break;

       case IDC_GRANT_PROCEDURE_PUBLIC:
           CAListBox_ResetContent (hwndGrantees);
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
   HWND    hwndGrantees  = GetDlgItem (hwnd, IDC_GRANT_PROCEDURE_GRANTEES);
   HWND    hwndProcedure = GetDlgItem (hwnd, IDC_GRANT_PROCEDURE_PROCEDURES);
   BOOL    B [3];
   BOOL    C [2];

   B[0] = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_PROCEDURE_PUBLIC));
   B[1] = (CAListBox_GetSelCount (hwndGrantees)   >= 1);
   B[2] = (CAListBox_GetSelCount (hwndProcedure)  >= 1);

   C[0] = !B[0] && B[1] && B[2];
   C[1] =  B[0] && B[2];

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
   HWND hwndGrantees  = GetDlgItem (hwnd, IDC_GRANT_PROCEDURE_GRANTEES);
   HWND hwndProcedures= GetDlgItem (hwnd, IDC_GRANT_PROCEDURE_PROCEDURES);
   int  max_item_number;
   
   max_item_number = CAListBox_GetSelCount (hwndProcedures);
   if (max_item_number >= 1)
   {
       lpgrant->lpobject   = AddItemToListWithOwner (hwndProcedures);
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
           return FALSE;
       }
   }

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_PROCEDURE_USER)))
       lpgrant->GranteeType = OT_USER;
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_PROCEDURE_GROUP)))
       lpgrant->GranteeType = OT_GROUP;
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_PROCEDURE_ROLE)))
       lpgrant->GranteeType = OT_ROLE;
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_PROCEDURE_PUBLIC)))
   {
       lpgrant->GranteeType = OT_PUBLIC;     // Public
       FreeObjectList (lpgrant->lpgrantee);
       lpgrant->lpgrantee = APublicUser ();
       if (!lpgrant->lpgrantee)
       {
           FreeAttachedPointers (lpgrant, OTLL_GRANT);
           ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
           return FALSE;
       }
   }
   lpgrant->grant_option = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_PROCEDURE_OPTION));
   return TRUE;
}

