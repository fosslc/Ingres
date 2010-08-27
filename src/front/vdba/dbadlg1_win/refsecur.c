/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : refsecur.c
//
//    Dialog box for Creating a Security alarm dialog   (Cross Reference)
// History:
// uk$so01 (20-Jan-2000, Bug #100063)
//         Eliminate the undesired compiler's warning
//	20-Aug-2010 (drivi01)
//	 Updated a call to CAListBoxFillTables function which
//	 was updated to include the 3rd parameter.
//	 The 3rd parameter is set to FALSE here to specify that
//	 only Ingres tables should be displayed.
********************************************************************/

#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "getdata.h"
#include "msghandl.h"
#include "dba.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "dom.h"
#include "resource.h"
#include "esltools.h"
#include "winutils.h"


BOOL CALLBACK __export DlgRefSecurityAlarmDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy (HWND hwnd);
static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog           (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange       (HWND hwnd);
static void PreCheckItem           (HWND hwnd);
static void EnableDisableOKButton  (HWND hwnd);
static BOOL FillStructureFromControls (HWND hwnd, LPSECURITYALARMPARAMS lpsecurity);


int WINAPI __export DlgRefSecurity
                    (HWND hwndOwner, LPSECURITYALARMPARAMS lpsecurityalarmparams)
/*
// Function:
// Shows the Create Security alarm dialog. (Reference)
//
// Paramters:
//     1) hwndOwner:               Handle of the parent window for the dialog.
//     2) lpsecurityalarmparams:   Points to structure containing information used to 
//                                 initialise the dialog. The dialog uses the same
//                                 structure to return the result of the dialog.
//
// Returns:
//    TRUE if successful
*/
{
   FARPROC lpProc;
   int     retVal;

   if (!IsWindow (hwndOwner) || !lpsecurityalarmparams)
   {
       ASSERT(NULL);
       return FALSE;
   }
   
   lpProc = MakeProcInstance ((FARPROC) DlgRefSecurityAlarmDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
       MAKEINTRESOURCE (IDD_REFALARM),
       hwndOwner,
       (WNDPROC) lpProc,
       (LPARAM)  lpsecurityalarmparams );

   FreeProcInstance (lpProc);
   return (retVal);
}

BOOL CALLBACK __export DlgRefSecurityAlarmDlgProc
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
   LPSECURITYALARMPARAMS lpsecurity   = (LPSECURITYALARMPARAMS)lParam;
   HWND    hwndUsers    = GetDlgItem (hwnd, IDC_REFALARM_BYUSER  );
   HWND    hwndDatabase = GetDlgItem (hwnd, IDC_REFALARM_DATABASE);
   HWND    hwndTables   = GetDlgItem (hwnd, IDC_REFALARM_ONTABLE );
   char    szTitle  [MAX_TITLEBAR_LEN];
   char    szFormat [100];
   char    szDatabaseName [MAXOBJECTNAME];

   if (!AllocDlgProp (hwnd, lpsecurity))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();

   LoadString (hResource, (UINT)IDS_T_CREATE_SECURITY, szFormat, sizeof (szFormat));

   wsprintf (szTitle, szFormat,
       GetVirtNodeName ( GetCurMdiNodeHandle ()),
       lpsecurity->DBName);
   SetWindowText (hwnd, szTitle);
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_REFALARM));

   //
   // Get the available database names and insert them into the combo box
   //
   ComboBoxFillDatabases (hwndDatabase);
   ComboBoxSelectFirstStr(hwndDatabase);
   ComboBox_GetText (hwndDatabase, szDatabaseName, sizeof (szDatabaseName));
   if (x_strlen (szDatabaseName) > 0)
       x_strcpy (lpsecurity->DBName, szDatabaseName);

   //
   // Get the available table names and insert them into the table list 
   //
   if (x_strlen (lpsecurity->DBName) > 0)
   {
       if (!CAListBoxFillTables (hwndTables, lpsecurity->DBName, FALSE))
       {
           CAListBoxDestroyItemData (hwndTables);
           return FALSE;
       }
   }
   //
   // Get the available users names and insert them into the CA list box 
   // 

   CAListBoxFillUsersP (hwndUsers);
   PreCheckItem (hwnd);
   EnableDisableOKButton (hwnd);

   richCenterDialog (hwnd);
   return TRUE;
}



static void OnDestroy (HWND hwnd)
{
   DeallocDlgProp(hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPSECURITYALARMPARAMS lpsecurity = GetDlgProp (hwnd);
   int  ires;

   switch (id)
   {
       case IDOK:
           if (!FillStructureFromControls (hwnd, lpsecurity))
               break;
           //
           // Call the low level function to write data on disk
           //
           ires = DBAAddObject
               ( GetVirtNodeName ( GetCurMdiNodeHandle ()),
               OTLL_SECURITYALARM,
               (void *) lpsecurity );

           if (ires != RES_SUCCESS)
           {
               FreeAttachedPointers (lpsecurity, OTLL_SECURITYALARM);
               ErrorMessage ((UINT)IDS_E_CREATE_SECURITY_FAILED, ires);
               break;
           }
           else
               EndDialog (hwnd, TRUE);
           FreeAttachedPointers (lpsecurity, OTLL_SECURITYALARM);
           break;
           
       case IDCANCEL:
           FreeAttachedPointers (lpsecurity, OTLL_SECURITYALARM);
           EndDialog (hwnd, FALSE);
           break;
       
       case IDC_REFALARM_BYUSER:
           EnableDisableOKButton (hwnd);
           break;

       case IDC_REFALARM_DATABASE:
       {
           char szDatabaseName [MAXOBJECTNAME];
           HWND hwndDatabase = GetDlgItem (hwnd, IDC_REFALARM_DATABASE);
           HWND hwndTables   = GetDlgItem (hwnd, IDC_REFALARM_ONTABLE);
           
           switch (codeNotify)
           {
               case CBN_SELCHANGE:
                   ComboBox_GetText (hwndDatabase, szDatabaseName, sizeof (szDatabaseName));
                   if (x_strlen (szDatabaseName) > 0)
                   {
                       x_strcpy (lpsecurity->DBName, szDatabaseName);
                       CAListBoxDestroyItemData (hwndTables);
                       CAListBox_ResetContent   (hwndTables);
                       if (!CAListBoxFillTables (hwndTables, lpsecurity->DBName, FALSE))
                           CAListBoxDestroyItemData (hwndTables);
                   }
                   break;
           }
           EnableDisableOKButton (hwnd);
       }
       break;

       case IDC_REFALARM_ONTABLE:
           EnableDisableOKButton (hwnd);
           break;

       case IDC_REFALARM_SUCCESS:
       case IDC_REFALARM_FAILURE:
       case IDC_REFALARM_SELECT:
       case IDC_REFALARM_DELETE:
       case IDC_REFALARM_INSERT:
       case IDC_REFALARM_UPDATE:
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


static void EnableDisableOKButton  (HWND hwnd)
{
   HWND hwndUsers   = GetDlgItem (hwnd, IDC_REFALARM_BYUSER );
   HWND hwndTables  = GetDlgItem (hwnd, IDC_REFALARM_ONTABLE);
   HWND hwndSuccess = GetDlgItem (hwnd, IDC_REFALARM_SUCCESS);
   HWND hwndFailure = GetDlgItem (hwnd, IDC_REFALARM_FAILURE);
   HWND hwndSelect  = GetDlgItem (hwnd, IDC_REFALARM_SELECT);
   HWND hwndDelete  = GetDlgItem (hwnd, IDC_REFALARM_DELETE);
   HWND hwndInsert  = GetDlgItem (hwnd, IDC_REFALARM_INSERT);
   HWND hwndUpdate  = GetDlgItem (hwnd, IDC_REFALARM_UPDATE);

   int     table_sel_count = CAListBox_GetSelCount (hwndTables);
   int     user_sel_count  = CAListBox_GetSelCount (hwndUsers);
   BOOL    rb = Button_GetCheck (hwndSuccess) || Button_GetCheck (hwndFailure);
   BOOL    tb = Button_GetCheck (hwndSelect)  || Button_GetCheck (hwndDelete) \
             || Button_GetCheck (hwndInsert)  || Button_GetCheck (hwndUpdate);

   if ((table_sel_count == 0) || (user_sel_count == 0) || (!tb) || (!rb))
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
}


static void PreCheckItem (HWND hwnd)
{
   LPSECURITYALARMPARAMS lpsecurity = GetDlgProp (hwnd);
 
   HWND hwndUsers  = GetDlgItem (hwnd, IDC_REFALARM_BYUSER );
   HWND hwndTables = GetDlgItem (hwnd, IDC_REFALARM_ONTABLE);

   HWND hwndSuccess= GetDlgItem (hwnd, IDC_REFALARM_SUCCESS);
   HWND hwndFailure= GetDlgItem (hwnd, IDC_REFALARM_FAILURE);
   HWND hwndSelect = GetDlgItem (hwnd, IDC_REFALARM_SELECT);
   HWND hwndDelete = GetDlgItem (hwnd, IDC_REFALARM_DELETE);
   HWND hwndInsert = GetDlgItem (hwnd, IDC_REFALARM_INSERT);
   HWND hwndUpdate = GetDlgItem (hwnd, IDC_REFALARM_UPDATE);

   Button_SetCheck (hwndSuccess, lpsecurity->bsuccfail  [SECALARMSUCCESS]);
   Button_SetCheck (hwndFailure, lpsecurity->bsuccfail  [SECALARMFAILURE]);

   Button_SetCheck (hwndSelect, lpsecurity->baccesstype [SECALARMSEL]);
   Button_SetCheck (hwndDelete, lpsecurity->baccesstype [SECALARMDEL]);
   Button_SetCheck (hwndInsert, lpsecurity->baccesstype [SECALARMINS]);
   Button_SetCheck (hwndUpdate, lpsecurity->baccesstype [SECALARMUPD]);

   if (x_strlen (lpsecurity->PreselectUser) > 0)
       CAListBox_SelectString (hwndUsers, -1, lpsecurity->PreselectUser);
}



static BOOL FillStructureFromControls (HWND hwnd, LPSECURITYALARMPARAMS lpsecurity)
{
   char buf        [MAXOBJECTNAME];
   char buffowner  [MAXOBJECTNAME];
   char bufftable  [MAXOBJECTNAME];
   LPCHECKEDOBJECTS  obj;
   LPCHECKEDOBJECTS  list = NULL;
   int i, max_database_number, checked;
   HWND hwndUsers   = GetDlgItem (hwnd, IDC_REFALARM_BYUSER );
   HWND hwndTables  = GetDlgItem (hwnd, IDC_REFALARM_ONTABLE);
   HWND hwndSuccess = GetDlgItem (hwnd, IDC_REFALARM_SUCCESS);
   HWND hwndFailure = GetDlgItem (hwnd, IDC_REFALARM_FAILURE);
   HWND hwndSelect  = GetDlgItem (hwnd, IDC_REFALARM_SELECT);
   HWND hwndDelete  = GetDlgItem (hwnd, IDC_REFALARM_DELETE);
   HWND hwndInsert  = GetDlgItem (hwnd, IDC_REFALARM_INSERT);
   HWND hwndUpdate  = GetDlgItem (hwnd, IDC_REFALARM_UPDATE);

   lpsecurity->bsuccfail [SECALARMSUCCESS] = Button_GetCheck (hwndSuccess);
   lpsecurity->bsuccfail [SECALARMFAILURE] = Button_GetCheck (hwndFailure);
   //
   // Get selection from
   // toggle buttons (Select, Delete, Insert, Update)
   //
   lpsecurity->baccesstype [SECALARMSEL]   = Button_GetCheck (hwndSelect);
   lpsecurity->baccesstype [SECALARMDEL]   = Button_GetCheck (hwndDelete);
   lpsecurity->baccesstype [SECALARMINS]   = Button_GetCheck (hwndInsert);
   lpsecurity->baccesstype [SECALARMUPD]   = Button_GetCheck (hwndUpdate);

   //
   // Get the names of users that have been checked and
   // insert into the list
   //
   max_database_number = CAListBox_GetCount (hwndUsers);
   
   for (i=0; i<max_database_number; i++)
   {
       checked = CAListBox_GetSel (hwndUsers, i);
       if (checked)
       {
           CAListBox_GetText (hwndUsers, i, buf);
           obj = ESL_AllocMem (sizeof (CHECKEDOBJECTS));
           if (!obj)
           {
               FreeAttachedPointers (lpsecurity, OTLL_SECURITYALARM);
               ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
               break;
           }
           else
           {
               if (x_strcmp (buf, lppublicdispstring()) == 0)
                   x_strcpy (obj->dbname, lppublicsysstring());
               else
                   x_strcpy (obj->dbname, buf);
               obj->bchecked = TRUE;
               list = AddCheckedObject (list, obj);
           }
       }
   }
   lpsecurity->lpfirstUser = list;

   //
   // Get the names of Tables that have been checked and
   // insert into the list
   //
   max_database_number = CAListBox_GetCount (hwndTables);
   list = NULL;

   for (i = 0; i < max_database_number; i++)
   {
       checked = CAListBox_GetSel (hwndTables, i);
       if (checked)
       {
           CAListBox_GetText (hwndTables, i, buf);
           x_strcpy (buffowner, (char *) CAListBox_GetItemData (hwndTables, i));
           obj = ESL_AllocMem (sizeof (CHECKEDOBJECTS));
           if (!obj)
           {
               FreeAttachedPointers (lpsecurity, OTLL_SECURITYALARM);
               ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
               return FALSE;
           }
           else
           {
               StringWithOwner (buf, buffowner, bufftable);
               x_strcpy (obj->dbname, bufftable);
               obj->bchecked = TRUE;
               list = AddCheckedObject (list, obj);
           }
       }
   }
   lpsecurity->lpfirstTable = list;

   return TRUE;
}

