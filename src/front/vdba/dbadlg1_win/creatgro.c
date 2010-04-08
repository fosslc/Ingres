/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : creatgro.c
//
//    Dialog box for creating Group
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


static void OnDestroy                 (HWND hwnd);
static void OnCommand                 (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL CreateObject              (HWND hwnd, LPGROUPPARAMS   lpgroup);
static BOOL OnInitDialog              (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange          (HWND hwnd);
static BOOL FillControlsFromStructure (HWND hwnd, LPGROUPPARAMS lpgroupparams);
static BOOL FillStructureFromControls (HWND hwnd, LPGROUPPARAMS lpgroupparams);
static void EnableDisableOKButton     (HWND hwnd);
static void LocalCAListBoxFillDatabases    (HWND hwndCtl);

BOOL CALLBACK __export DlgCreateGroupDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


                   
int WINAPI __export DlgCreateGroup (HWND hwndOwner, LPGROUPPARAMS lpgroupparams)

/*
// Function:
// Shows the Create/Alter group dialog.
//
// Paramters:
//     1) hwndOwner    :   Handle of the parent window for the dialog.
//     2) lpgroupparams:   Points to structure containing information used to 
//                         initialise the dialog. The dialog uses the same
//                         structure to return the result of the dialog.
//
// Returns:
// TRUE if successful.
*/
{
   FARPROC lpProc;
   int     retVal;

   if (!IsWindow (hwndOwner) || !lpgroupparams)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgCreateGroupDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_GROUP),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpgroupparams
       );

   FreeProcInstance (lpProc);
   return (retVal);
}



BOOL CALLBACK __export DlgCreateGroupDlgProc
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
   LPGROUPPARAMS lpgroup   = (LPGROUPPARAMS)lParam;
   char szTitle  [MAX_TITLEBAR_LEN];
   char szFormat [100];

   if (!AllocDlgProp (hwnd, lpgroup))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();

   if (lpgroup->bCreate) 
   {
       LoadString (hResource, (UINT)IDS_T_CREATE_GROUP, szFormat, sizeof (szFormat));
       lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_GROUP));
   }
   else
   {
       LoadString (hResource, (UINT)IDS_T_ALTER_GROUP, szFormat, sizeof (szFormat));
   }
   wsprintf (szTitle,
       szFormat,
       GetVirtNodeName (GetCurMdiNodeHandle ()));
   SetWindowText (hwnd, szTitle);
   FillControlsFromStructure (hwnd, lpgroup);
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
   LPGROUPPARAMS lpgroup = GetDlgProp (hwnd);

   switch (id)
   {
       case IDOK:
       {
           BOOL Success = FALSE;

           if (lpgroup->bCreate)
           {
               Success = FillStructureFromControls (hwnd, lpgroup);
               if (!Success)
               {
                   FreeAttachedPointers (lpgroup, OT_GROUP);
                   break;
               }
               Success = CreateObject (hwnd, lpgroup);
               FreeAttachedPointers (lpgroup, OT_GROUP);
           }

           if (!Success)
               break;
           else
               EndDialog (hwnd, TRUE);
       }
       break;
           

       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           break;

       case IDC_CG_GROUPNAME:
       {
           switch (codeNotify)
           {
               case EN_CHANGE:
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


static BOOL CreateObject (HWND hwnd, LPGROUPPARAMS   lpgroup)
{
   int     ires;
   LPUCHAR vnodeName = GetVirtNodeName (GetCurMdiNodeHandle ());

   ires = DBAAddObject (vnodeName, OT_GROUP, (void *) lpgroup );
   if (ires != RES_SUCCESS)
   {
       ErrorMessage ((UINT) IDS_E_CREATE_GROUP_FAILED, ires);
       return FALSE;
   }
   return TRUE;
}


static BOOL FillControlsFromStructure (HWND hwnd, LPGROUPPARAMS lpgroupparams)
{
   HWND hwndGroup     = GetDlgItem (hwnd, IDC_CG_GROUPNAME);
   HWND hwndUsers     = GetDlgItem (hwnd, IDC_CG_USERLIST );
   HWND hwndDatabases = GetDlgItem (hwnd, IDC_CG_DATABASES);
   int  index, max_database_number ;
   LPCHECKEDOBJECTS lsuser;
   LPCHECKEDOBJECTS lsdatabases;

   Edit_LimitText (hwndGroup, MAXOBJECTNAME -1);
   Edit_SetText   (hwndGroup, lpgroupparams->ObjectName);
   
   CAListBoxFillUsers     (hwndUsers);
   LocalCAListBoxFillDatabases (hwndDatabases);
   
   //
   // Checked the database's name where user has access 
   //
   max_database_number = CAListBox_GetCount (hwndDatabases);

   lsdatabases = (LPCHECKEDOBJECTS) lpgroupparams->lpfirstdb;
   while (lsdatabases)
   {
       index = CAListBox_FindString (hwndDatabases,-1,lsdatabases->dbname);
       if ((index >= 0) && (index <max_database_number))
           CAListBox_SetSel (hwndDatabases, TRUE, index);
       lsdatabases = lsdatabases->pnext;
   }
   //
   // Checked the user name if user is belong to the group 'ObjectName' 
   //
   max_database_number = CAListBox_GetCount (hwndUsers);
   lsuser = (LPCHECKEDOBJECTS) lpgroupparams->lpfirstuser;
   while (lsuser)
   {
       index = CAListBox_FindString (hwndUsers, -1, lsuser->dbname);
       {
           if ((index >= 0) && (index <max_database_number))
               CAListBox_SetSel (hwndUsers, TRUE, index);
       }
       lsuser = lsuser->pnext;
   }

   return TRUE;
}


static BOOL FillStructureFromControls (HWND hwnd, LPGROUPPARAMS lpgroupparams)
{
   HWND hwndGroup     = GetDlgItem (hwnd, IDC_CG_GROUPNAME);
   HWND hwndUsers     = GetDlgItem (hwnd, IDC_CG_USERLIST );
   HWND hwndDatabases = GetDlgItem (hwnd, IDC_CG_DATABASES);
   int  i, max_database_number ;
   LPCHECKEDOBJECTS list = NULL;
   LPCHECKEDOBJECTS obj;
   char szGroup  [MAXOBJECTNAME], buf  [MAXOBJECTNAME];
   BOOL checked;
   
   Edit_GetText (hwndGroup, szGroup, sizeof (szGroup));
   if (!IsObjectNameValid (szGroup, OT_UNKNOWN))
   {
       MsgInvalidObject ((UINT)IDS_E_NOT_A_VALIDE_OBJECT);
       Edit_SetSel (hwndGroup, 0, -1);
       SetFocus (hwndGroup);
       return FALSE;
   }
   x_strcpy (lpgroupparams->ObjectName, szGroup);

   //
   // Get the names of databases that have been checked and
   // insert into the list
   //
   max_database_number = CAListBox_GetCount (hwndDatabases);
   for (i = 0; i < max_database_number; i++)
   {
       checked = CAListBox_GetSel (hwndDatabases, i);
       if (checked == 1)
       {
           CAListBox_GetText (hwndDatabases, i, buf);
           obj = ESL_AllocMem (sizeof (CHECKEDOBJECTS));
           if (!obj)
           {
               list = FreeCheckedObjects (list);
               ErrorMessage ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
               return FALSE;
           }
           else
           {
               x_strcpy (obj->dbname, buf);
               obj->bchecked = TRUE;
               list = AddCheckedObject (list, obj);
           }
       }
   }
   lpgroupparams->lpfirstdb = list;

   //
   // Get the names of users that have been checked and
   // insert into the list
   //
   max_database_number = CAListBox_GetCount (hwndUsers);
   list = NULL;
   for (i = 0; i < max_database_number; i++)
   {
       checked = CAListBox_GetSel (hwndUsers, i);
       if (checked == 1)
       {
           CAListBox_GetText (hwndUsers, i, buf);
           obj = ESL_AllocMem (sizeof (CHECKEDOBJECTS));
           if (!obj)
           {
               lpgroupparams->lpfirstdb = FreeCheckedObjects (lpgroupparams->lpfirstdb);
               list = FreeCheckedObjects (list);
               ErrorMessage ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
               return FALSE;
           }
           else
           {
               x_strcpy (obj->dbname, QuoteIfNeeded(buf));
               obj->bchecked = TRUE;
               list = AddCheckedObject (list, obj);
           }
       }
   }
   lpgroupparams->lpfirstuser = list;

   return TRUE;
}

static void EnableDisableOKButton (HWND hwnd)
{
   HWND hwndGroup = GetDlgItem (hwnd, IDC_CG_GROUPNAME);

   if (Edit_GetTextLength (hwndGroup) == 0)
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
}

static void LocalCAListBoxFillDatabases (HWND hwndCtl)
{
   int     hdl, ires, ires1;
   BOOL    bwsystem;
   char    buf [MAXOBJECTNAME];
   char    buffilter [MAXOBJECTNAME];
     
   ZEROINIT (buf);
   ZEROINIT (buffilter);

   hdl      = GetCurMdiNodeHandle ();
   bwsystem = GetSystemFlag ();
       
   ires     = DOMGetFirstObject (hdl, OT_DATABASE, 0, NULL, bwsystem, NULL, buf, NULL, NULL);
   while (ires == RES_SUCCESS)
   {
       BOOL bOK=FALSE;
       BOOL bOK1=FALSE;
       ires1=isDBGranted  (hdl,buf,lppublicdispstring(),&bOK);
//       ires1=HasDBNoaccess(hdl,buf,lppublicdispstring(),&bOK1);
       if (!bOK)
         CAListBox_AddString (hwndCtl, buf);
       ires  = DOMGetNextObject (buf, buffilter, NULL);
   }
}

