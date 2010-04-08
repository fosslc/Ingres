/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : rvkgdbe.c
//
//    Dialog box for revoke privilege from dbevent.  (General)     
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


BOOL CALLBACK __export DlgRvkgdbeDlgProc
                            (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static void OnDestroy              (HWND hwnd);
static void OnCommand              (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog           (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange       (HWND hwnd);
static BOOL OccupyObjectsControl   (HWND hwnd);
static void EnableDisableOKButton  (HWND hwnd);
static void EnableDisableOKButton2 (HWND hwnd);
static void FillGrantedUsers       (HWND hwnd, LPREVOKEPARAMS lprevoke);
static LPOBJECTLIST InsertTableName(LPUCHAR  tableName);
static LPCHECKEDOBJECTS GetGrantee (LPREVOKEPARAMS lprevoke, int objectToFind, int grantee);



int WINAPI __export DlgRvkgdbe (HWND hwndOwner, LPREVOKEPARAMS lprevoke)
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

   lpProc = MakeProcInstance ((FARPROC) DlgRvkgdbeDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_RVKGDBE),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lprevoke
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgRvkgdbeDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
   LPREVOKEPARAMS lprevoke = (LPREVOKEPARAMS)lParam;
   char szTitle  [MAX_TITLEBAR_LEN];
   char szFormat [100];
   char buffer   [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];
   
   if (!AllocDlgProp (hwnd, lprevoke))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();
   LoadString (hResource, (UINT)IDS_T_RVKGDBE,  szFormat, sizeof (szFormat));
   parentstrings [0] = lprevoke->DBName;
   GetExactDisplayString (
       lprevoke->PreselectObject,
       lprevoke->PreselectObjectOwner,
       OT_DBEVENT,
       parentstrings,
       buffer);

   wsprintf (
       szTitle,
       szFormat,
       GetVirtNodeName (GetCurMdiNodeHandle ()),
       lprevoke->DBName,
       buffer);
   SetWindowText (hwnd, szTitle);
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_RVKGDBE));
   
   Button_SetCheck (GetDlgItem (hwnd, IDC_RVKGDBE_PRIV   ), TRUE);
   Button_SetCheck (GetDlgItem (hwnd, IDC_RVKGDBE_CASCADE), TRUE);
   Button_SetCheck (GetDlgItem (hwnd, IDC_RVKGDBE_USERS  ), TRUE);
   Button_SetCheck (GetDlgItem (hwnd, IDC_RVKGDBE_USERS),   TRUE);

   FillGrantedUsers      (hwnd,    lprevoke);
   EnableDisableOKButton (hwnd);
   richCenterDialog      (hwnd);
   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   DeallocDlgProp (hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   HWND hwndGrantees       = GetDlgItem (hwnd, IDC_RVKGDBE_GRANTEES);
   LPREVOKEPARAMS lprevoke = GetDlgProp (hwnd);

   switch (id)
   {
       case IDOK:
       {
           TCHAR tchNameObject[MAXOBJECTNAME];
           int ires, max_item_number = 0;
           max_item_number = CAListBox_GetSelCount (hwndGrantees);
           if (max_item_number >= 1)
           {
               lprevoke->lpgrantee = AddItemToListQuoted (hwndGrantees);
               if (!lprevoke->lpgrantee)
               {
                   ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
                   break;
               }
           }
           lprevoke->grant_option = Button_GetCheck (GetDlgItem (hwnd, IDC_RVKGDBE_GOPT));
           lprevoke->cascade      = Button_GetCheck (GetDlgItem (hwnd, IDC_RVKGDBE_CASCADE));
           wsprintf(tchNameObject,"%s.%s",QuoteIfNeeded(lprevoke->PreselectObjectOwner),QuoteIfNeeded(lprevoke->PreselectObject));
           lprevoke->lpobject     = InsertTableName (tchNameObject);
           lprevoke->ObjectType   = OT_DBEVENT;
           
           ires = DBADropObject
                    (GetVirtNodeName (GetCurMdiNodeHandle ()),
                     OTLL_GRANT,
                     (void *) lprevoke);

           if (ires != RES_SUCCESS)
           {
               FreeObjectList (lprevoke->lpgrantee);
               FreeObjectList (lprevoke->lpobject);
               lprevoke->lpobject = NULL;
               lprevoke->lpgrantee= NULL;
               ErrorMessage ((UINT)IDS_E_REVOKE_DBEVENT_FAILED, ires);
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

       case IDC_RVKGDBE_USERS:
           lprevoke->GranteeType = OT_USER;
           FillGrantedUsers (hwnd, lprevoke);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_RVKGDBE_GROUPS:
           lprevoke->GranteeType = OT_GROUP;
           FillGrantedUsers (hwnd, lprevoke);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_RVKGDBE_ROLES:
           lprevoke->GranteeType = OT_ROLE;
           FillGrantedUsers (hwnd, lprevoke);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_RVKGDBE_PUBLIC:
           lprevoke->GranteeType = OT_PUBLIC;
           FillGrantedUsers (hwnd, lprevoke);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_RVKGDBE_GRANTEES:
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_RVKGDBE_RAISE    :
       case IDC_RVKGDBE_REGISTER :
           FillGrantedUsers       (hwnd, lprevoke);
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
   HWND hwndGrantees  = GetDlgItem (hwnd, IDC_RVKGDBE_GRANTEES);
   LPREVOKEPARAMS lprevoke  = GetDlgProp (hwnd);
   BOOL B [3];
   BOOL D = FALSE;
   int  i;

   for (i = 0; i < GRANT_MAX_PRIVILEGES; i++)
   {
       if (lprevoke->Privileges [i])
       {
           D = TRUE;
           break;
       }
   }

   B[1] = (CAListBox_GetSelCount (hwndGrantees) >= 1);

   if (D && B[1])
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
}


static void FillGrantedUsers (HWND hwnd, LPREVOKEPARAMS lprevoke)
{
   int  i, Who;
   int  first = -1;
   HWND hwndGrantee=GetDlgItem (hwnd, IDC_RVKGDBE_GRANTEES);

   LPCHECKEDOBJECTS
       resl = NULL,
       temp = NULL,
       lgtee= NULL,
       ls   = NULL;
   LPOBJECTLIST
       lch  = NULL,
       lx   = NULL;

   for (i=0; i<GRANT_MAX_PRIVILEGES; i++)
       lprevoke->Privileges [i] = FALSE;

   lprevoke->Privileges [GRANT_RAISE   ] = Button_GetCheck (GetDlgItem (hwnd, IDC_RVKGDBE_RAISE   ));
   lprevoke->Privileges [GRANT_REGISTER] = Button_GetCheck (GetDlgItem (hwnd, IDC_RVKGDBE_REGISTER));

   lch = AddItemToList    (hwndGrantee);
   CAListBox_ResetContent (hwndGrantee);

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_RVKGDBE_USERS)))
   {
       Who = OT_USER;
       //CAListBoxFillUsers (hwndGrantee);
   }
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_RVKGDBE_GROUPS)))
   {
       Who = OT_GROUP;
       //CAListBoxFillGroups (hwndGrantee);
   }
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_RVKGDBE_ROLES)))
   {
       Who = OT_ROLE;
       //CAListBoxFillRoles (hwndGrantee);
   }
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_RVKGDBE_PUBLIC)))
       Who = OT_PUBLIC;
   lprevoke->GranteeType = Who;

   for (i=0; i<GRANT_MAX_PRIVILEGES; i++)
   {
       if (lprevoke->Privileges [i])
       {
           first = i;
           resl  = GetGrantee (lprevoke, i, Who);
           break;
       }
   }
   i = first +1;

   while (i < GRANT_MAX_PRIVILEGES)
   {
       if (lprevoke->Privileges [i])
       {
           lgtee= GetGrantee           (lprevoke, i, Who);
           temp = Union         (resl, lgtee);
           lgtee= FreeCheckedObjects   (lgtee);
           resl = FreeCheckedObjects   (resl);
           resl = temp;
           temp = NULL;
       }
       i++;
   }

   ls = resl;
   while (ls)
   {
       CAListBox_AddString (hwndGrantee, ls->dbname);
       ls = ls->pnext;
   }
 
   lx = lch;
   while (lx)
   {
       char* item = (char *)lx->lpObject;
       CAListBox_SelectString (hwndGrantee, -1, item);
       lx = lx->lpNext;
   }
   FreeObjectList (lch);
   lch  = NULL;
   resl = FreeCheckedObjects  (resl);
}

           
static LPOBJECTLIST InsertTableName (LPUCHAR tableName)
{
   LPOBJECTLIST list = NULL;
   LPOBJECTLIST obj;

   obj = AddListObject (list, x_strlen (tableName) +1);
   if (obj)
   {
       x_strcpy ((UCHAR *)obj->lpObject, tableName);
       list = obj;
   }
   else
   {
       ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
       list = NULL;
   }
   return (list);
}



static LPCHECKEDOBJECTS GetGrantee (LPREVOKEPARAMS lprevoke, int objectToFind, int grantee)
{
   LPCHECKEDOBJECTS list = NULL;
   LPCHECKEDOBJECTS obj;
   int     hdl, ires, gtype, grant_user;
   BOOL    bwsystem;
   char    buf [MAXOBJECTNAME];
   char    buffilter [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];
   LPDOMDATA lpDomData = GetCurLpDomData ();
   char    buffwithowner [MAXOBJECTNAME];

   switch (objectToFind)
   {
       case GRANT_RAISE   :
           grant_user = OT_DBEGRANT_RAISE_USER;
           break; 
       case GRANT_REGISTER:
           grant_user = OT_DBEGRANT_REGTR_USER;
           break;
       default:
           grant_user = -1;
           break;
   }

   ZEROINIT (buf);
   ZEROINIT (buffilter);
   hdl      = GetCurMdiNodeHandle ();
   bwsystem = GetSystemFlag ();
   
   if (!lpDomData)
       return NULL;

   parentstrings [0] = lprevoke->DBName;           // Database name
   parentstrings [1] = StringWithOwner ((LPTSTR)Quote4DisplayIfNeeded(lprevoke->PreselectObject), lprevoke->PreselectObjectOwner, buffwithowner);

   ires = DOMGetFirstObject (hdl, grant_user, 2,
             parentstrings, bwsystem, NULL, buf, NULL, NULL);

   while (ires == RES_SUCCESS)
   {
       gtype = DBA_GetObjectType  (buf, lpDomData);
       if (gtype == grantee)
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
       }
       ires  = DOMGetNextObject (buf, buffilter, NULL);
   }
   return list;
}

