/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : usr2grp.c
//
//    Add usr to group                                    
//    
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


static void OnDestroy       (HWND hwnd);
static void OnCommand       (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL CreateObject    (HWND hwnd, LPGROUPUSERPARAMS   lpgroupuserparams);
static BOOL OnInitDialog    (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);
static BOOL FillControlsFromStructure (HWND hwnd, LPGROUPUSERPARAMS lpgroupuserparams);
static BOOL FillStructureFromControls (HWND hwnd, LPGROUPUSERPARAMS lpgroupuserparams);
static void EnableDisableOKButton     (HWND hwnd);

BOOL CALLBACK __export DlgAddUser2GroupDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


                   
int WINAPI __export DlgAddUser2Group (HWND hwndOwner, LPGROUPUSERPARAMS lpgroupuserparams)

/*
// Function:
// Shows the Add user to group dialog.
//
// Paramters:
//     1) hwndOwner        :   Handle of the parent window for the dialog.
//     2) lpgroupuserparams:   Points to structure containing information used to 
//                             initialise the dialog. The dialog uses the same
//                             structure to return the result of the dialog.
//
// Returns:
// TRUE if successful.
*/
{
   FARPROC lpProc;
   int     retVal;

   if (!IsWindow (hwndOwner) || !lpgroupuserparams)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgAddUser2GroupDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_USR2GRP),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpgroupuserparams
       );

   FreeProcInstance (lpProc);
   return (retVal);
}



BOOL CALLBACK __export DlgAddUser2GroupDlgProc
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
   HWND hwndUser = GetDlgItem (hwnd, IDC_USR2GRP_USERBOX);
   LPGROUPUSERPARAMS lpusr2grp   = (LPGROUPUSERPARAMS)lParam;
   char szTitle  [MAX_TITLEBAR_LEN];
   char szFormat [150];

   if (!AllocDlgProp (hwnd, lpusr2grp))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();

   LoadString (hResource, (UINT)IDS_T_USR2GRP, szFormat, sizeof (szFormat));
   wsprintf   (
       szTitle,
       szFormat,
       lpusr2grp->GroupName,
       GetVirtNodeName ( GetCurMdiNodeHandle ()));

   SetWindowText (hwnd, szTitle);
   ComboBoxFillUsers (hwndUser);
   EnableDisableOKButton (hwnd);
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_USR2GRP));

   richCenterDialog (hwnd);
   return TRUE;
}


static void OnDestroy (HWND hwnd)
{
   DeallocDlgProp(hwnd);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPGROUPUSERPARAMS lpusr2grp = GetDlgProp (hwnd);

   switch (id)
   {
       case IDOK:
       {
           BOOL Success;
           HWND hwndUser = GetDlgItem (hwnd, IDC_USR2GRP_USERBOX);
           char szUser [MAXOBJECTNAME];

           ComboBox_GetText (hwndUser, szUser, sizeof (szUser));
           x_strcpy (lpusr2grp->ObjectName, szUser);
          
           Success = CreateObject (hwnd, lpusr2grp);

           if (!Success)
               break;
           else
               EndDialog (hwnd, TRUE);
       }
       break;
           

       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           break;

       case IDC_USR2GRP_USERBOX:
       {
           switch (codeNotify)
           {
               case CBN_SELCHANGE:
                   EnableDisableOKButton (hwnd);
           }
           break;
       }
   }
}


static void OnSysColorChange (HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}


static BOOL CreateObject (HWND hwnd, LPGROUPUSERPARAMS   lpusr2grp)
{
   int     ires;
   LPUCHAR vnodeName = GetVirtNodeName (GetCurMdiNodeHandle ());

   ires = DBAAddObject (vnodeName, OT_GROUPUSER, (void *) lpusr2grp );

   if (ires != RES_SUCCESS)
   {
       ErrorMessage ((UINT) IDS_E_ADD_USR2GRP_FAILED, ires);
       return FALSE;
   }

   return TRUE;
}


static void EnableDisableOKButton (HWND hwnd)
{
   HWND hwndUser = GetDlgItem (hwnd, IDC_USR2GRP_USERBOX);
   char szUser [MAXOBJECTNAME];

   ComboBox_GetText (hwndUser, szUser, sizeof (szUser));

   if (x_strlen (szUser) == 0)
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
}
