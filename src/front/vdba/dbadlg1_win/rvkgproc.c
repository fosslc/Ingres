/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : rvkgproc.c
//
//    Dialog box for revoke execute procedure (General)
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


BOOL CALLBACK __export DlgRvkgprocDlgProc
                            (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static void OnDestroy       (HWND hwnd);
static void OnCommand       (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog    (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);
static void AddGrantee      (HWND hwndCtl, char* item);
static BOOL OccupyObjectsControl  (HWND hwnd);
static void EnableDisableOKButton (HWND hwnd);
static LPCHECKEDOBJECTS GetGrantee(HWND hwnd, char* item);



int WINAPI __export DlgRvkgproc (HWND hwndOwner, LPREVOKEPARAMS lprevoke)
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

   lpProc = MakeProcInstance ((FARPROC) DlgRvkgprocDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_RVKGPROC),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lprevoke
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgRvkgprocDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
   char buffer   [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];

   LPREVOKEPARAMS lprevoke = (LPREVOKEPARAMS)lParam;
   HWND hwndUsers  = GetDlgItem (hwnd, IDC_RVKGPROC_USERS);
   HWND hwndProcs  = GetDlgItem (hwnd, IDC_RVKGPROC_PROCS);
   
   if (!AllocDlgProp (hwnd, lprevoke))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();
   LoadString (hResource, (UINT)IDS_T_RVKGPROC, szFormat, sizeof (szFormat));
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_RVKGPROC));

   parentstrings [0] = lprevoke->DBName;
   GetExactDisplayString (
       lprevoke->PreselectObject,
       lprevoke->PreselectObjectOwner,
       OT_PROCEDURE,
       parentstrings,
       buffer);

   wsprintf (
       szTitle,
       szFormat,
       GetVirtNodeName (GetCurMdiNodeHandle ()),
       lprevoke->DBName,
       buffer);
   SetWindowText (hwnd, szTitle);
   CAListBoxFillProcedures  (hwndProcs, lprevoke->DBName);
   if (x_strlen (lprevoke->PreselectGrantee) >  0)
   {
       if (x_strcmp (lprevoke->PreselectGrantee, lppublicdispstring()) == 0)
           CAListBox_SelectString (hwndUsers, -1, lppublicsysstring());
       else
           CAListBox_SelectString (hwndUsers, -1, lprevoke->PreselectGrantee);
   }

   if (x_strlen (lprevoke->PreselectObject) > 0)
       CAListBoxSelectStringWithOwner (hwndProcs, lprevoke->PreselectObject, lprevoke->PreselectObjectOwner);

   
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
   HWND hwndProcs = GetDlgItem (hwnd, IDC_RVKGPROC_PROCS);
   HWND hwndUsers = GetDlgItem (hwnd, IDC_RVKGPROC_USERS);
   LPREVOKEPARAMS lprevoke = GetDlgProp (hwnd);

   switch (id)
   {
       case IDOK:
           {
               int ires;
               int max_item_number;

               max_item_number = CAListBox_GetSelCount (hwndUsers);
               if (max_item_number >= 1)
               {
                   lprevoke->lpgrantee = AddItemToList (hwndUsers);
                   if (!lprevoke->lpgrantee)
                   {
                       ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
                       break;
                   }
               }

               max_item_number = CAListBox_GetSelCount (hwndProcs);
               if (max_item_number >= 1)
               {
                   lprevoke->lpobject = AddItemToList (hwndProcs);
                   if (!lprevoke->lpobject)
                   {
                       ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
                       FreeObjectList (lprevoke->lpobject);
                       lprevoke->lpobject = NULL;
                       break;
                   }
               }
               lprevoke->ObjectType   = OT_PROCEDURE;

               ires = DBADropObject
                       (GetVirtNodeName ( GetCurMdiNodeHandle ()),
                       OTLL_GRANT,
                       (void *) lprevoke);

               if (ires != RES_SUCCESS)
               {
                   ErrorMessage ((UINT)IDS_E_REVOKE_PROCEDURE_FAILED, ires);
                   break;
               }
               else 
               {
                   EndDialog (hwnd, TRUE);
               }
               FreeObjectList (lprevoke->lpgrantee);
               FreeObjectList (lprevoke->lpobject);
               lprevoke->lpobject = NULL;
               lprevoke->lpgrantee= NULL;
           }
           break;

       case IDCANCEL:
           FreeObjectList (lprevoke->lpgrantee);
           FreeObjectList (lprevoke->lpobject);
           lprevoke->lpobject = NULL;
           lprevoke->lpgrantee= NULL;
           EndDialog (hwnd, FALSE);
           break;

       case IDC_RVKGPROC_USERS:
           if (codeNotify == CALBN_CHECKCHANGE)
               EnableDisableOKButton  (hwnd);
           break;

       case IDC_RVKGPROC_PROCS:
           if (codeNotify == CALBN_CHECKCHANGE)
           {
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
               list = AddItemToListWithOwner  (hwndProcs);
               CAListBox_ResetContent(hwndUsers);
               ls   = list;
             
               if (ls)
               {
                   item = (char *) ls->lpObject;
                   l1   = GetGrantee (hwnd, item);
                   lr   = DupList    (l1);
                   ls = ls->lpNext;
               }

               while (ls)
               {
                   item = (char *) ls->lpObject;
                   l2   = GetGrantee (hwnd, item);
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
   if ((CAListBox_GetSelCount (GetDlgItem (hwnd, IDC_RVKGPROC_USERS)) > 0) &&
       (CAListBox_GetSelCount (GetDlgItem (hwnd, IDC_RVKGPROC_PROCS)) > 0))
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

static LPCHECKEDOBJECTS GetGrantee (HWND hwnd, char* item)
{
   LPREVOKEPARAMS lprevoke = GetDlgProp (hwnd);
   LPCHECKEDOBJECTS list   = NULL;
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

   parentstrings [0] = lprevoke->DBName;
   parentstrings [1] = item;

   ires = DOMGetFirstObject (hdl, OT_PROCGRANT_EXEC_USER, 2,
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
