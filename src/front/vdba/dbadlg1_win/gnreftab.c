/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : gnreftab.c
//
//    Dialog box for Table grant (or View grant). (reference) 
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
#include "resource.h"

BOOL CALLBACK __export DlgGnrefTableDlgProc
               (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy       (HWND hwnd);
static void OnCommand       (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog    (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);
static BOOL OccupyObjectsControl       (HWND hwnd);
static void InitialiseEditControls     (HWND hwnd);
static void EnableDisableOKButton      (HWND hwnd);
static void ShowColumns                (HWND hwnd, LPGRANTPARAMS lpgrant);
static BOOL FillStructureFromControls  (HWND hwnd,LPGRANTPARAMS lpgrant);

int WINAPI __export DlgGnrefTable (HWND hwndOwner, LPGRANTPARAMS lpgrant)
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

   lpProc = MakeProcInstance ((FARPROC) DlgGnrefTableDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_GNREF_TABLE),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpgrant
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgGnrefTableDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
   HWND  hwndDatabases   = GetDlgItem (hwnd, IDC_GNREF_TABLE_DATABASES);
   HWND  hwndTables      = GetDlgItem (hwnd, IDC_GNREF_TABLE_TABLE);
   HWND  hwndGrantees    = GetDlgItem (hwnd, IDC_GNREF_TABLE_GRANTEES);
   HWND  hwndExcludings  = GetDlgItem (hwnd, IDC_GNREF_TABLE_EXCLUDINGS);
   UCHAR szDatabaseName [MAXOBJECTNAME];
   char  szTitle        [MAX_TITLEBAR_LEN];
   char  szFormat       [100];

   if (!AllocDlgProp (hwnd, lpgrant))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();
   if (lpgrant->ObjectType == OT_TABLE)
   {
       LoadString (hResource, (UINT)IDS_T_GNREF_TABLE, szFormat, sizeof (szFormat));
       lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_GNREF_TABLE));
   }
   else
   {
       LoadString (hResource, (UINT)IDS_T_GNREF_VIEW,  szFormat, sizeof (szFormat));
       lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_GNREF_VIEW));
   }
   wsprintf (
       szTitle,
       szFormat,
       GetVirtNodeName (GetCurMdiNodeHandle ()));
   SetWindowText (hwnd, szTitle);

   if (lpgrant->GranteeType == OT_GROUP)
   {
       Button_SetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_GROUP), TRUE);
       FillGrantees (hwndGrantees, lpgrant->GranteeType);
   }
   else
   if (lpgrant->GranteeType == OT_ROLE)
   {
       Button_SetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_ROLE),  TRUE);
       FillGrantees (hwndGrantees, lpgrant->GranteeType);
   }
   else
   {
       if  (x_strcmp (lpgrant->PreselectGrantee, lppublicdispstring()) == 0)
           Button_SetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_PUBLIC),  TRUE);
       else
       {
           Button_SetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_USER),  TRUE);
           lpgrant->GranteeType = OT_USER;
           FillGrantees (hwndGrantees, lpgrant->GranteeType);
       }
   }
   ComboBoxFillDatabases (hwndDatabases);
   ComboBoxSelectFirstStr(hwndDatabases);
   ComboBox_GetText (hwndDatabases, szDatabaseName, sizeof (szDatabaseName));
   x_strcpy (lpgrant->DBName, szDatabaseName);

   if (lpgrant->ObjectType == OT_TABLE)
   {
       if (!CAListBoxFillTables (hwndTables, lpgrant->DBName))
           CAListBoxDestroyItemData (hwndTables);
       if (x_strlen (lpgrant->PreselectObject) > 1)
       {
           CAListBoxSelectStringWithOwner (hwndTables, lpgrant->PreselectObject, lpgrant->PreselectObjectOwner);
           ShowColumns (hwnd, lpgrant);
       }
   }
   else if (lpgrant->ObjectType == OT_VIEW)
   {
       if (!CAListBoxFillViews (hwndTables, lpgrant->DBName))
           CAListBoxDestroyItemData (hwndTables);
       if (x_strlen (lpgrant->PreselectObject) > 1)
       {
           CAListBoxSelectStringWithOwner (hwndTables, lpgrant->PreselectObject, lpgrant->PreselectObjectOwner);
           ShowColumns (hwnd, lpgrant);
       }
   }
   
   if (x_strlen (lpgrant->PreselectGrantee) > 1)
   {
       CAListBox_SelectString (hwndGrantees, -1, lpgrant->PreselectGrantee);
   }        
   if (lpgrant->PreselectPrivileges [GRANT_SELECT])
       Button_SetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_SELECT), TRUE);
   if (lpgrant->PreselectPrivileges [GRANT_INSERT])
       Button_SetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_INSERT), TRUE);
   if (lpgrant->PreselectPrivileges [GRANT_UPDATE])
       Button_SetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_UPDATE), TRUE);
   if (lpgrant->PreselectPrivileges [GRANT_DELETE])
       Button_SetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_DELETE), TRUE);
   if (lpgrant->PreselectPrivileges [GRANT_COPY_INTO])
       Button_SetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_COPYINTO), TRUE);
   if (lpgrant->PreselectPrivileges [GRANT_COPY_FROM])
       Button_SetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_COPYFROM), TRUE);
   if (lpgrant->PreselectPrivileges [GRANT_REFERENCE])
       Button_SetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_REFERENCE),TRUE);

   //EnableWindow (GetDlgItem (hwnd, IDC_GNREF_TABLE_COPYINTO), FALSE);
   //EnableWindow (GetDlgItem (hwnd, IDC_GNREF_TABLE_COPYFROM), FALSE);
   if (lpgrant->ObjectType == OT_VIEW)
   {
       ShowWindow (GetDlgItem (hwnd, IDC_GNREF_TABLE_REFERENCE), SW_HIDE);
       ShowWindow (GetDlgItem (hwnd, IDC_GNREF_TABLE_COPYINTO),  SW_HIDE);
       ShowWindow (GetDlgItem (hwnd, IDC_GNREF_TABLE_COPYFROM),  SW_HIDE);

       SetWindowText(GetDlgItem (hwnd, IDC_GNREF_TABLEL_TABLE), ResourceString ((UINT)IDS_I_VIEWS));
   }

   EnableDisableOKButton (hwnd);
   richCenterDialog(hwnd);
   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   HWND hwndTables = GetDlgItem (hwnd, IDC_GNREF_TABLE_TABLE);

   CAListBoxDestroyItemData (hwndTables);
   DeallocDlgProp (hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPGRANTPARAMS lpgrant   = GetDlgProp (hwnd);
   HWND hwndDatabases   = GetDlgItem (hwnd, IDC_GNREF_TABLE_DATABASES);
   HWND hwndGrantees    = GetDlgItem (hwnd, IDC_GNREF_TABLE_GRANTEES);
   HWND hwndTables      = GetDlgItem (hwnd, IDC_GNREF_TABLE_TABLE);
   HWND hwndExcludings  = GetDlgItem (hwnd, IDC_GNREF_TABLE_EXCLUDINGS);
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
               if (lpgrant->ObjectType == OT_TABLE)
                   ErrorMessage ((UINT)IDS_E_GRANT_TABLE_FAILED, ires);
               else
                   ErrorMessage ((UINT)IDS_E_GRANT_VIEW_FAILED,  ires);
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

       case IDC_GNREF_TABLE_TABLE:
           if (codeNotify == CALBN_CHECKCHANGE)
           {
               ShowColumns (hwnd, lpgrant);
               EnableDisableOKButton (hwnd);
           }
           break;

       case IDC_GNREF_TABLE_GRANTEES:
           EnableDisableOKButton (hwnd);
           break;

       case IDC_GNREF_TABLE_USER:
           CAListBox_ResetContent (hwndGrantees);
           FillGrantees (hwndGrantees, OT_USER);
           EnableDisableOKButton (hwnd);
           break;

       case IDC_GNREF_TABLE_GROUP:
           CAListBox_ResetContent (hwndGrantees);
           FillGrantees (hwndGrantees, OT_GROUP);
           EnableDisableOKButton (hwnd);
           break;

       case IDC_GNREF_TABLE_ROLE:
           CAListBox_ResetContent (hwndGrantees);
           FillGrantees (hwndGrantees, OT_ROLE);
           EnableDisableOKButton (hwnd);
           break;

       case IDC_GNREF_TABLE_PUBLIC:
           CAListBox_ResetContent (hwndGrantees);
           EnableDisableOKButton (hwnd);
           break;

       case IDC_GNREF_TABLE_INSERT:
       case IDC_GNREF_TABLE_SELECT:
       case IDC_GNREF_TABLE_UPDATE:
       case IDC_GNREF_TABLE_DELETE:
       case IDC_GNREF_TABLE_COPYINTO:
       case IDC_GNREF_TABLE_COPYFROM:
       case IDC_GNREF_TABLE_REFERENCE:
           EnableDisableOKButton (hwnd);
           break;

       case IDC_GNREF_TABLE_DATABASES:
       {
           char selString [MAXOBJECTNAME +1];

           if (codeNotify == CBN_SELCHANGE)
           {
               ComboBox_GetText (hwndDatabases, selString, sizeof (selString));
               x_strcpy (lpgrant->DBName, selString);
               CAListBoxDestroyItemData (hwndTables);
               CAListBox_ResetContent  (hwndTables);
               if (lpgrant->ObjectType == OT_TABLE)
                   CAListBoxFillTables (hwndTables, lpgrant->DBName);
               else
                   CAListBoxFillViews (hwndTables, lpgrant->DBName);
               CAListBox_ResetContent (hwndExcludings);
           }
           EnableDisableOKButton (hwnd);
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


#define    CURRENT_PRIVILEGES_NUM 7

static void EnableDisableOKButton  (HWND hwnd)
{
   HWND hwndGrantees    = GetDlgItem (hwnd, IDC_GNREF_TABLE_GRANTEES);
   HWND hwndTables      = GetDlgItem (hwnd, IDC_GNREF_TABLE_TABLE);
   BOOL check [CURRENT_PRIVILEGES_NUM];
   BOOL B [3];
   BOOL C [2];
   
   check [0] = Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_SELECT));
   check [1] = Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_INSERT));
   check [2] = Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_UPDATE));
   check [3] = Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_DELETE));
   check [4] = Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_COPYINTO));
   check [5] = Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_COPYFROM));
   check [6] = Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_REFERENCE));

   B[0] = Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_PUBLIC));
   B[1] = (CAListBox_GetSelCount (hwndGrantees) >= 1);
   B[2] = (CAListBox_GetSelCount (hwndTables)   >= 1);

   C[0] = !B[0] && B[1] && B[2] && (check [0] || check [1] || check [2] || check [3] || check [4] || check [5] || check [6]);
   C[1] =  B[0] && B[2] && (check [0] || check [1] || check [2] || check [3] || check [4] || check [5] || check [6]);

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



static BOOL FillStructureFromControls (HWND hwnd,LPGRANTPARAMS lpgrant)
{
   HWND hwndDatabases   = GetDlgItem (hwnd, IDC_GNREF_TABLE_DATABASES);
   HWND hwndGrantees    = GetDlgItem (hwnd, IDC_GNREF_TABLE_GRANTEES);
   HWND hwndTables      = GetDlgItem (hwnd, IDC_GNREF_TABLE_TABLE);
   HWND hwndExcludings  = GetDlgItem (hwnd, IDC_GNREF_TABLE_EXCLUDINGS);
   int  max_item_number;

  int chksel;
  int chkins; 
  int chkupd; 
  int chkdel; 
  int chkcpin;
  int chkcpfr;
  int chkref; 
   
   max_item_number = CAListBox_GetSelCount (hwndTables);
   if (max_item_number >= 1)
   {
       lpgrant->lpobject   = AddItemToListWithOwner (hwndTables);
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
       lpgrant->lpgrantee = AddItemToList (hwndGrantees);
       if (!lpgrant->lpgrantee)
       {
           FreeAttachedPointers (lpgrant, OTLL_GRANT);
           ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
           return FALSE;
       }
   }

   lpgrant->Privileges [GRANT_SELECT]    = chksel  = Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_SELECT));
   lpgrant->Privileges [GRANT_INSERT]    = chkins  = Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_INSERT));
   lpgrant->Privileges [GRANT_UPDATE]    = chkupd  = Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_UPDATE));
   lpgrant->Privileges [GRANT_DELETE]    = chkdel  = Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_DELETE));
   lpgrant->Privileges [GRANT_COPY_INTO] = chkcpin = Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_COPYINTO));
   lpgrant->Privileges [GRANT_COPY_FROM] = chkcpfr = Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_COPYFROM));
   lpgrant->Privileges [GRANT_REFERENCE] = chkref  = Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_REFERENCE));
   lpgrant->grant_option = Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_OPTION));

   max_item_number = CAListBox_GetSelCount (hwndExcludings);
   if (max_item_number >= 1)
   {
     // Several cases to manage for columns exclusion
     if (chksel || chkins || chkdel || chkcpin || chkcpfr) {
       if (lpgrant->ObjectType == OT_TABLE)
         //"The Excludings List only applies to Update and Reference Grants."
         MessageBox(hwnd, ResourceString(IDS_ERR_EXCLUDING_REFERENCE), NULL, MB_ICONEXCLAMATION | MB_OK);
       else
         //"The Excludings List only applies to Update Grants."
         MessageBox(hwnd, ResourceString(IDS_ERR_EXCLUDING_GRANT), NULL, MB_ICONEXCLAMATION | MB_OK);
       FreeAttachedPointers (lpgrant, OTLL_GRANT);
       return FALSE;
     }
       lpgrant->lpcolumn = AddItemToList (hwndExcludings);
       if (!lpgrant->lpcolumn)
       {
           FreeAttachedPointers (lpgrant, OTLL_GRANT);
           ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
           return FALSE;
       }
   }
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_USER)))
       lpgrant->GranteeType = OT_USER;
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_GROUP)))
       lpgrant->GranteeType = OT_GROUP;
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_ROLE)))
       lpgrant->GranteeType = OT_ROLE;
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_GNREF_TABLE_PUBLIC)))
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


   return TRUE;
}

static void ShowColumns (HWND hwnd, LPGRANTPARAMS lpgrant)
{
   BOOL checked;
   char str       [MAXOBJECTNAME];
   char table     [MAXOBJECTNAME]; // Table name or view name
   char buffowner [MAXOBJECTNAME]; // Owner.Tablename

   HWND hwndTables      = GetDlgItem (hwnd, IDC_GRANT_TABLE_TABLE);
   HWND hwndExcludings  = GetDlgItem (hwnd, IDC_GRANT_TABLE_EXCLUDINGS);
   int  i, max_item     = CAListBox_GetSelCount (hwndTables);

   if (max_item == 1)
   {
       max_item = CAListBox_GetCount (hwndTables);
       CAListBox_ResetContent (hwndExcludings);
       for (i = 0; i < max_item; i++)
       {
           checked = CAListBox_GetSel (hwndTables, i);
           if (checked == 1)
           {
               ZEROINIT (str);
               CAListBox_GetText (hwndTables, i, str);
               x_strcpy (buffowner, (char *) CAListBox_GetItemData (hwndTables, i));
               StringWithOwner (str, buffowner, table);

               if (lpgrant->ObjectType == OT_TABLE)
                   CAListBoxFillTableColumns (hwndExcludings, lpgrant->DBName, table);
               else
                   CAListBoxFillViewColumns  (hwndExcludings, lpgrant->DBName, table); // Table = View
               break;
           }
       }
   }
   else
       CAListBox_ResetContent (hwndExcludings);
}


