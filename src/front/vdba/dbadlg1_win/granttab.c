/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : granttab.c
//
//    Dialog box for table (or view) grant.            
//
//    History:
//	20-Aug-2010 (drivi01)
//	 Updated a call to CAListBoxFillTables function which
//	 was updated to include the 3rd parameter.
//	 The 3rd parameter is set to FALSE here to specify that
//	 only Ingres tables should be displayed.
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

// values for iAction parameter to ShowColumns
#define ACT_DEFAULT           0
#define ACT_INITDIALOG        1
#define ACT_TABLECHECKCHANGE  2

BOOL CALLBACK __export DlgGrantTableDlgProc
               (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static void OnDestroy       (HWND hwnd);
static void OnCommand       (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog    (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);
static BOOL OccupyObjectsControl       (HWND hwnd);
static void InitialiseEditControls     (HWND hwnd);
static void EnableDisableOKButton      (HWND hwnd);
static void ShowColumns                (HWND hwnd, LPGRANTPARAMS lpgrant, int iAction);
static BOOL FillStructureFromControls  (HWND hwnd, LPGRANTPARAMS lpgrant);

int WINAPI __export DlgGrantTable (HWND hwndOwner, LPGRANTPARAMS lpgrant)
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

   lpProc = MakeProcInstance ((FARPROC) DlgGrantTableDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_GRANT_TABLE),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpgrant
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgGrantTableDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
   HWND hwndGrantees    = GetDlgItem (hwnd, IDC_GRANT_TABLE_GRANTEES);
   HWND hwndTables      = GetDlgItem (hwnd, IDC_GRANT_TABLE_TABLE);
   HWND hwndExcludings  = GetDlgItem (hwnd, IDC_GRANT_TABLE_EXCLUDINGS);
   char szTitle [MAX_TITLEBAR_LEN];
   char szFormat[100];

   if (!AllocDlgProp (hwnd, lpgrant))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();

   if (lpgrant->ObjectType == OT_TABLE)
   {
       LoadString (hResource, (UINT)IDS_T_GRANT_TABLE, szFormat, sizeof (szFormat));
       lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_GRANT_TABLE));
   }
   else
   {
       LoadString (hResource, (UINT)IDS_T_GRANT_VIEW, szFormat, sizeof (szFormat));
       lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_GRANT_VIEW));
   }
   wsprintf (
       szTitle,
       szFormat,
       GetVirtNodeName (GetCurMdiNodeHandle ()),
       lpgrant->DBName);
   SetWindowText (hwnd, szTitle);

   Button_SetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_USER), TRUE);
   FillGrantees (hwndGrantees, OT_USER);

   if (lpgrant->ObjectType == OT_TABLE)
   {
       if (!CAListBoxFillTables (hwndTables, lpgrant->DBName, FALSE))
           CAListBoxDestroyItemData (hwndTables);
       if (x_strlen (lpgrant->PreselectObject) > 0)
       {
           CAListBoxSelectStringWithOwner  (hwndTables, lpgrant->PreselectObject, lpgrant->PreselectObjectOwner);
           ShowColumns (hwnd, lpgrant, ACT_INITDIALOG);
       }
   }
   else if (lpgrant->ObjectType == OT_VIEW)
   {
       if (!CAListBoxFillViews (hwndTables, lpgrant->DBName))
           CAListBoxDestroyItemData (hwndTables);
       if (x_strlen (lpgrant->PreselectObject) > 0)
       {
           CAListBoxSelectStringWithOwner   (hwndTables, lpgrant->PreselectObject, lpgrant->PreselectObjectOwner);
           ShowColumns (hwnd, lpgrant, ACT_INITDIALOG);
       }
   }
   
   if (x_strlen (lpgrant->PreselectGrantee) > 0)
   {
       CAListBox_SelectString (hwndGrantees, -1, lpgrant->PreselectGrantee);
   }        

   if (lpgrant->PreselectPrivileges [GRANT_SELECT])
       Button_SetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_SELECT),     TRUE);
   if (lpgrant->PreselectPrivileges [GRANT_INSERT])
       Button_SetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_INSERT),     TRUE);
   if (lpgrant->PreselectPrivileges [GRANT_UPDATE])
       Button_SetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_UPDATE),     TRUE);
   if (lpgrant->PreselectPrivileges [GRANT_DELETE])
       Button_SetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_DELETE),     TRUE);
   if (lpgrant->PreselectPrivileges [GRANT_COPY_INTO])
       Button_SetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_COPYINTO),   TRUE);
   if (lpgrant->PreselectPrivileges [GRANT_COPY_FROM])
       Button_SetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_COPYFROM),   TRUE);
   if (lpgrant->PreselectPrivileges [GRANT_REFERENCE])
       Button_SetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_REFERENCE),  TRUE);

   //EnableWindow (GetDlgItem (hwnd, IDC_GRANT_TABLE_COPYINTO), FALSE);
   //EnableWindow (GetDlgItem (hwnd, IDC_GRANT_TABLE_COPYFROM), FALSE);
   if (lpgrant->ObjectType == OT_VIEW)
   {
       ShowWindow (GetDlgItem (hwnd, IDC_GRANT_TABLE_REFERENCE), SW_HIDE);
       ShowWindow (GetDlgItem (hwnd, IDC_GRANT_TABLE_COPYINTO),  SW_HIDE);
       ShowWindow (GetDlgItem (hwnd, IDC_GRANT_TABLE_COPYFROM),  SW_HIDE);

       SetWindowText(GetDlgItem (hwnd, IDC_GRANT_TABLEL_TABLE), ResourceString ((UINT)IDS_I_VIEWS));
   }

   EnableDisableOKButton (hwnd);
   
   richCenterDialog(hwnd);
   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   HWND hwndTables = GetDlgItem (hwnd, IDC_GRANT_TABLE_TABLE);

   DeallocDlgProp (hwnd);
   CAListBoxDestroyItemData (hwndTables);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPGRANTPARAMS lpgrant   = GetDlgProp (hwnd);
   HWND hwndGrantees       = GetDlgItem (hwnd, IDC_GRANT_TABLE_GRANTEES);
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


       case IDC_GRANT_TABLE_TABLE:
           if (codeNotify == CALBN_CHECKCHANGE)
           {
               ShowColumns (hwnd, lpgrant, ACT_TABLECHECKCHANGE);
               EnableDisableOKButton (hwnd);
           }
           break;

       case IDC_GRANT_TABLE_GRANTEES:
           EnableDisableOKButton (hwnd);
           break;

       case IDC_GRANT_TABLE_USER:
           CAListBox_ResetContent (hwndGrantees);
           FillGrantees (hwndGrantees, OT_USER);
           EnableDisableOKButton (hwnd);
           break;

       case IDC_GRANT_TABLE_GROUP:
           CAListBox_ResetContent (hwndGrantees);
           FillGrantees (hwndGrantees, OT_GROUP);
           EnableDisableOKButton (hwnd);
           break;

       case IDC_GRANT_TABLE_ROLE:
           CAListBox_ResetContent (hwndGrantees);
           FillGrantees (hwndGrantees, OT_ROLE);
           EnableDisableOKButton (hwnd);
           break;
       
       case IDC_GRANT_TABLE_PUBLIC:
           CAListBox_ResetContent (hwndGrantees);
           EnableDisableOKButton (hwnd);
           break;

       case IDC_GRANT_TABLE_INSERT:
       case IDC_GRANT_TABLE_DELETE:
       case IDC_GRANT_TABLE_SELECT:
       case IDC_GRANT_TABLE_UPDATE:
       case IDC_GRANT_TABLE_COPYINTO:
       case IDC_GRANT_TABLE_COPYFROM:
       case IDC_GRANT_TABLE_REFERENCE:
           ShowColumns (hwnd, lpgrant, ACT_DEFAULT);
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


#define    CURRENT_PRIVILEGES_NUM 7

static void EnableDisableOKButton  (HWND hwnd)
{
   HWND hwndGrantees    = GetDlgItem (hwnd, IDC_GRANT_TABLE_GRANTEES);
   HWND hwndTables      = GetDlgItem (hwnd, IDC_GRANT_TABLE_TABLE);
   BOOL check [CURRENT_PRIVILEGES_NUM];
   BOOL B [3];
   BOOL C [2];
   
   check [0] = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_SELECT));
   check [1] = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_INSERT));
   check [2] = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_UPDATE));
   check [3] = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_DELETE));
   check [4] = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_COPYINTO));
   check [5] = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_COPYFROM));
   check [6] = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_REFERENCE));

   B[0] = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_PUBLIC));
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



static BOOL FillStructureFromControls (HWND hwnd, LPGRANTPARAMS lpgrant)
{
   HWND hwndGrantees    = GetDlgItem (hwnd, IDC_GRANT_TABLE_GRANTEES);
   HWND hwndTables      = GetDlgItem (hwnd, IDC_GRANT_TABLE_TABLE);
   HWND hwndExcludings  = GetDlgItem (hwnd, IDC_GRANT_TABLE_EXCLUDINGS);
   int max_item_number;

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
       lpgrant->lpgrantee = AddItemToListQuoted (hwndGrantees);
       if (!lpgrant->lpgrantee)
       {
           FreeAttachedPointers (lpgrant, OTLL_GRANT);
           ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
           return FALSE;
       }
   }

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_USER)))
       lpgrant->GranteeType = OT_USER;
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_GROUP)))
       lpgrant->GranteeType = OT_GROUP;
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_ROLE)))
       lpgrant->GranteeType = OT_ROLE;
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_PUBLIC)))
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

   lpgrant->Privileges [GRANT_SELECT]    = chksel  = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_SELECT));
   lpgrant->Privileges [GRANT_INSERT]    = chkins  = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_INSERT));
   lpgrant->Privileges [GRANT_UPDATE]    = chkupd  = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_UPDATE));
   lpgrant->Privileges [GRANT_DELETE]    = chkdel  = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_DELETE));
   lpgrant->Privileges [GRANT_COPY_INTO] = chkcpin = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_COPYINTO));
   lpgrant->Privileges [GRANT_COPY_FROM] = chkcpfr = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_COPYFROM));
   lpgrant->Privileges [GRANT_REFERENCE] = chkref  = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_REFERENCE));
   lpgrant->grant_option = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_OPTION));

   max_item_number = CAListBox_GetSelCount (hwndExcludings);
   if (max_item_number >= 1)
   {
     BOOL bStoreColumns = TRUE;

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
     // store columns if applies
     if (bStoreColumns)
     {
       lpgrant->lpcolumn = AddItemToList (hwndExcludings);
       if (!lpgrant->lpcolumn)
       {
           FreeAttachedPointers (lpgrant, OTLL_GRANT);
           ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
           return FALSE;
       }
     }
   }
  
   return TRUE;
}

static void ShowColumns (HWND hwnd, LPGRANTPARAMS lpgrant, int iAction)
{
   BOOL checked;
   char str       [MAXOBJECTNAME];
   char table     [MAXOBJECTNAME]; // Table name or view name
   char buffowner [MAXOBJECTNAME]; // Owner.Tablename
   //BOOL chk1,chk2,chk3;
   //BOOL chk4, chk5;

   HWND hwndTables      = GetDlgItem (hwnd, IDC_GRANT_TABLE_TABLE);
   HWND hwndExcludings  = GetDlgItem (hwnd, IDC_GRANT_TABLE_EXCLUDINGS);
   int  i, max_item     = CAListBox_GetSelCount (hwndTables);

   //chk1 = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_INSERT));
   //chk2 = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_DELETE));
   //chk3 = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_SELECT));
   //chk4 = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_COPYINTO));
   //chk5 = Button_GetCheck (GetDlgItem (hwnd, IDC_GRANT_TABLE_COPYFROM));

   switch(iAction) {
      case ACT_INITDIALOG:
      case ACT_TABLECHECKCHANGE:
        //if (max_item==1 && chk1==0 && chk2==0 && chk3==0 && chk4==0 && chk5==0)
        if (max_item==1)
        {
            max_item  = CAListBox_GetCount (hwndTables);
            CAListBox_ResetContent (hwndExcludings);
            for (i = 0; i < max_item; i++)
            {
                checked = CAListBox_GetSel (hwndTables, i);
                if (checked == 1)
                {
                    ZEROINIT (str);
                    CAListBox_GetText  (hwndTables, i, str);
                    x_strcpy (buffowner, (char *) CAListBox_GetItemData (hwndTables, i));
                    StringWithOwner    (str, buffowner, table);
        
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
        break;

      case ACT_DEFAULT:
        //if (chk1==0 && chk2==0 && chk3==0 && chk4==0 && chk5==0)
        //  ShowWindow(hwndExcludings, SW_SHOW);
        //else
        //  ShowWindow(hwndExcludings, SW_HIDE);
        break;
   }
}
