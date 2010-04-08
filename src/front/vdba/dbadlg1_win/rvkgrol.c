/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : rvkgrol.c
//
//    Dialog box for revoke privilege from role
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


BOOL CALLBACK __export DlgRvkgrolDlgProc
                            (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static void OnDestroy       (HWND hwnd);
static void OnCommand       (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog    (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);
static void AddGrantee      (HWND hwndCtl, char* item);
static BOOL OccupyObjectsControl  (HWND hwnd);
static void EnableDisableOKButton (HWND hwnd);
static LPCHECKEDOBJECTS GetGrantee(char* item);
static BOOL FillStructureFromControls (HWND hwnd, LPREVOKEPARAMS lprevoke);
static void RolesClick (HWND hwnd);



int WINAPI __export DlgRvkgrol (HWND hwndOwner, LPREVOKEPARAMS lprevoke)
/*
// Function:
// Shows the Revoke privilege on table dialog.
//
// Paramters:
//     1) hwndOwner:   Handle of the parent window for the dialog.
//     1) lprevoke :   Points to structure containing information used to 
//                     initialise the dialog. 
//
// Returns:
//     (TRUE, FALSE)
//
*/
{
   FARPROC lpProc;
   int     retVal;
   
   if (!IsWindow (hwndOwner) || !lprevoke)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgRvkgrolDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_RVKGROL),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lprevoke
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgRvkgrolDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
   char szTitle  [MAX_TITLEBAR_LEN];
   char szFormat [100];
   LPREVOKEPARAMS lprevoke = (LPREVOKEPARAMS)lParam;
   HWND hwndUsers  = GetDlgItem (hwnd, IDC_RVKGROL_USERS);
   HWND hwndRoles  = GetDlgItem (hwnd, IDC_RVKGROL_ROLES);
   
   if (!AllocDlgProp (hwnd, lprevoke))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_RVKGROL));
   LoadString (hResource, (UINT)IDS_T_RVKGROL, szFormat, sizeof (szFormat));
   wsprintf (
       szTitle,
       szFormat,
       GetVirtNodeName (GetCurMdiNodeHandle ()));
   
   SetWindowText (hwnd, szTitle);
   CAListBoxFillRoles  (GetDlgItem (hwnd, IDC_RVKGROL_ROLES));

   if (x_strlen (lprevoke->PreselectGrantee) > 0)
   {
       if (x_strcmp (lprevoke->PreselectGrantee, lppublicdispstring()) == 0)
           CAListBox_SelectString (hwndUsers, -1, lppublicsysstring());
       else
           CAListBox_SelectString (hwndUsers, -1, lprevoke->PreselectGrantee);
   }
   if (x_strlen (lprevoke->PreselectObject) > 0)
   {
       CAListBox_SelectString (hwndRoles,  -1, lprevoke->PreselectObject);
       RolesClick (hwnd);
   }


   //CAListBoxFillUsersP (GetDlgItem (hwnd, IDC_RVKGROL_USERS));
   
   richCenterDialog(hwnd);
   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   DeallocDlgProp (hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPREVOKEPARAMS lprevoke = GetDlgProp (hwnd);
   int  ires;

   switch (id)
   {
       case IDOK:
           FillStructureFromControls (hwnd, lprevoke);
           ires = DBADropObject
                   (GetVirtNodeName ( GetCurMdiNodeHandle ()),
                   OTLL_GRANT,
                   (void *) lprevoke);

           if (ires != RES_SUCCESS)
           {
               ErrorMessage ((UINT)IDS_E_REVOKE_ROLE_FAILED, RES_ERR);
           }
           else 
           {
               EndDialog (hwnd, TRUE);
           }
           FreeObjectList (lprevoke->lpgrantee);
           FreeObjectList (lprevoke->lpobject);
           lprevoke->lpobject = NULL;
           lprevoke->lpgrantee= NULL;

           break;

       case IDCANCEL:
           FreeObjectList (lprevoke->lpgrantee);
           FreeObjectList (lprevoke->lpobject);
           lprevoke->lpobject = NULL;
           lprevoke->lpgrantee= NULL;
           EndDialog (hwnd, FALSE);
           break;

       case IDC_RVKGROL_USERS:
           if (codeNotify == CALBN_CHECKCHANGE)
               EnableDisableOKButton  (hwnd);
           break;

       case IDC_RVKGROL_ROLES:
           if (codeNotify == CALBN_CHECKCHANGE)
               RolesClick (hwnd);
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
   if ((CAListBox_GetSelCount (GetDlgItem (hwnd, IDC_RVKGROL_USERS)) > 0) &&
       (CAListBox_GetSelCount (GetDlgItem (hwnd, IDC_RVKGROL_ROLES)) > 0))
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
}


static void AddGrantee (HWND hwndCtl, char* item)
{
   int     k, hdl, ires;
   BOOL    bwsystem;
   char    buf [MAXOBJECTNAME];
   char    buffilter [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];
   LPDOMDATA lpDomData = GetCurLpDomData ();

   ZEROINIT (buf);
   ZEROINIT (buffilter);
   hdl      = GetCurMdiNodeHandle ();
   bwsystem = GetSystemFlag ();
   
   if (!lpDomData)
       return;

   parentstrings [0] = item;
   parentstrings [1] = NULL;

   ires = DOMGetFirstObject (hdl, OT_ROLEGRANT_USER, 1,
             parentstrings, bwsystem, NULL, buf, NULL, NULL);

   while (ires == RES_SUCCESS)
   {
       k     = CAListBox_FindStringExact (hwndCtl, 0, item);
       if (k < 0)
           CAListBox_AddString (hwndCtl, buf);
       ires  = DOMGetNextObject (buf, buffilter, NULL);
   }
}

static LPCHECKEDOBJECTS GetGrantee (char* item)
{
   LPCHECKEDOBJECTS list = NULL;
   LPCHECKEDOBJECTS obj;
  
   int     hdl, ires;
   BOOL    bwsystem;
   char    buf [MAXOBJECTNAME];
   char    buffilter [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];
   LPDOMDATA lpDomData = GetCurLpDomData ();

   ZEROINIT (buf);
   ZEROINIT (buffilter);
   hdl      = GetCurMdiNodeHandle ();
   bwsystem = GetSystemFlag ();
   
   if (!lpDomData)
       return NULL;

   parentstrings [0] = item;
   parentstrings [1] = NULL;

   ires = DOMGetFirstObject (hdl, OT_ROLEGRANT_USER, 1,
             parentstrings, bwsystem, NULL, buf, NULL, NULL);

   while (ires == RES_SUCCESS)
   {
       obj  = ESL_AllocMem (sizeof (CHECKEDOBJECTS));
       if (obj)
       {
           x_strcpy (obj->dbname, buf);
           obj->pnext = NULL;
           list = AddCheckedObject (list, obj);
       }
       else
       {
           list = FreeCheckedObjects  (list);
           return NULL;
       }
       ires  = DOMGetNextObject (buf, buffilter, NULL);
   }
   return list;
}



static BOOL FillStructureFromControls (HWND hwnd, LPREVOKEPARAMS lprevoke)
{
   lprevoke->lpgrantee  = AddItemToListQuoted (GetDlgItem (hwnd, IDC_RVKGROL_USERS));
   lprevoke->lpobject   = AddItemToListQuoted (GetDlgItem (hwnd, IDC_RVKGROL_ROLES));
   lprevoke->ObjectType = OT_ROLE;
   lprevoke->GranteeType= OT_USER;

   return TRUE;
}

static void RolesClick (HWND hwnd)
{
   HWND hwndUsers  = GetDlgItem (hwnd, IDC_RVKGROL_USERS);
   HWND hwndRoles  = GetDlgItem (hwnd, IDC_RVKGROL_ROLES);
   char* item;
   LPOBJECTLIST
       ls   = NULL,
       list = NULL,
       list2= NULL;
   LPCHECKEDOBJECTS
       la   = NULL,
       l1   = NULL,
       l2   = NULL,
       lr   = NULL;

   list2= AddItemToList  (hwndUsers);
   list = AddItemToList  (hwndRoles);
   CAListBox_ResetContent(hwndUsers);
   ls   = list;
   
   if (ls)
   {
       item = (char *) ls->lpObject;
       l1   = GetGrantee (item);
       lr   = DupList    (l1);
       ls = ls->lpNext;
   }

   while (ls)
   {
       item = (char *) ls->lpObject;
       l2   = GetGrantee (item);
       lr   = FreeCheckedObjects  (lr);
       lr   = Union (l1, l2);
       l1   = FreeCheckedObjects  (l1);
       l2   = FreeCheckedObjects  (l2);
       l1   = DupList (lr);
       ls = ls->lpNext;
   }
   
   la = lr;
   while (la)
   {
       CAListBox_AddString (hwndUsers, la->dbname);
       la = la->pnext;
   }
   lr   = FreeCheckedObjects  (lr);
   l1   = FreeCheckedObjects  (l1);

   ls   = list2;
   while (ls)
   {
       item = (char *) ls->lpObject;
       CAListBox_SelectString (hwndUsers, -1, item);
       ls = ls->lpNext;
   }

   FreeObjectList (list );
   FreeObjectList (list2);
   list = NULL;
   list2= NULL;
   
}

