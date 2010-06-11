/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : rvktable.c
//
//    Dialog box for revoking privilege one by one from 
//                 (Table, View, Procedure, DBevent)
//
** 28-Mar-2003 (schph01)
**   SIR 107523 used this dialog also for "NEXT ON SEQUENCE" grant
**   sequences.
** 04-Jun-2010 (horda03) b123227
**   Syntax for drop grant on sequence incorrect.
**   Need to display the CASCASE *& RESTRICT buttons.
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


//
// Global variable:  HaveBeenGranted is to be useed when retrieving the grantees.
// It indicates what type of grantee (USER, GROUP, ROLE) with a privilege. ie:
// User with privilege SELECT (this means: HaveBeenGranted = OT_TABLEGRANT_SEL_USER).
// 
static int HaveBeenGranted;


BOOL CALLBACK __export DlgRevokeOnTableDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy       (HWND hwnd);
static void OnCommand       (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog    (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);

static BOOL OccupyObjectsControl   (HWND hwnd);
static void EnableDisableOKButton  (HWND hwnd);
static void EnableDisableOKButton2 (HWND hwnd);
static void FillGrantedUsers (HWND hwndCtl, LPREVOKEPARAMS lprevoke, int objectToFind);
static LPOBJECTLIST InsertTableName (LPUCHAR tableName);
static void HandleUnknounObject (HWND hwnd);



int WINAPI __export DlgRevokeOnTable (HWND hwndOwner, LPREVOKEPARAMS lprevoke)
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

   lpProc = MakeProcInstance ((FARPROC) DlgRevokeOnTableDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_REVOKE_TABLE),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lprevoke
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgRevokeOnTableDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
   HWND hwndGrantees    = GetDlgItem (hwnd, IDC_REVOKE_TABLE_GRANTEES);
   char szTitle [90];
   char szStr   [MAX_TITLEBAR_LEN];
   int  i, positive_privilege;
   BOOL bGrantAll = FALSE;
   char buffer  [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];

   if (!AllocDlgProp (hwnd, lprevoke))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();


   for (i = 0; i < GRANT_MAX_PRIVILEGES; i++)
   {
       if (lprevoke->PreselectPrivileges [i])
       {
           positive_privilege = i;
           break;
       }
   }

   switch (lprevoke->ObjectType)
   {
       case OT_TABLE:
           lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_REVOKE_TABLE));
           break;
       case OT_VIEW:
           lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_REVOKE_VIEW));
           break;
       case OT_DBEVENT:
           lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_REVOKE_DBEVENT));
           break;
       case OT_PROCEDURE:
           lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_RVKGPROC));
           break;
       case OT_SEQUENCE:
           lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_RVKGSEQUENCE));
           break;
   }

   switch (positive_privilege)
   {
       case GRANT_SELECT    :
           LoadString (hResource, (UINT)IDS_T_REVOKE_SELECT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_SELECT] = TRUE;
           if (lprevoke->ObjectType == OT_TABLE)
               HaveBeenGranted = OT_TABLEGRANT_SEL_USER;
           else
               HaveBeenGranted = OT_VIEWGRANT_SEL_USER;
           break;
       case GRANT_INSERT    :
           LoadString (hResource, (UINT)IDS_T_REVOKE_INSERT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_INSERT] = TRUE;
           if (lprevoke->ObjectType == OT_TABLE)
               HaveBeenGranted = OT_TABLEGRANT_INS_USER;
           else
               HaveBeenGranted = OT_VIEWGRANT_INS_USER;
           break;
       case GRANT_UPDATE    :
           LoadString (hResource, (UINT)IDS_T_REVOKE_UPDATE_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_UPDATE] = TRUE;
           if (lprevoke->ObjectType == OT_TABLE)
               HaveBeenGranted = OT_TABLEGRANT_UPD_USER;
           else
               HaveBeenGranted = OT_VIEWGRANT_UPD_USER;
           break;
       case GRANT_DELETE    :
           LoadString (hResource, (UINT)IDS_T_REVOKE_DELETE_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_DELETE] = TRUE;
           if (lprevoke->ObjectType == OT_TABLE)
               HaveBeenGranted = OT_TABLEGRANT_DEL_USER;
           else
               HaveBeenGranted = OT_VIEWGRANT_DEL_USER;
           break;
       case GRANT_COPY_INTO :
           LoadString (hResource, (UINT)IDS_T_REVOKE_COPYINTO_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_COPY_INTO] = TRUE;
           HaveBeenGranted = OT_TABLEGRANT_CPI_USER;
           break;
       case GRANT_COPY_FROM :
           LoadString (hResource, (UINT)IDS_T_REVOKE_COPYFROM_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_COPY_FROM] = TRUE; 
           HaveBeenGranted = OT_TABLEGRANT_CPF_USER;
           break;
       case GRANT_REFERENCE :
           LoadString (hResource, (UINT)IDS_T_REVOKE_REFERENCE_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_REFERENCE] = TRUE;
           HaveBeenGranted = OT_TABLEGRANT_REF_USER;
           break;
       case GRANT_RAISE     :
           LoadString (hResource, (UINT)IDS_T_REVOKE_RAISE_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_RAISE] = TRUE;
           HaveBeenGranted = OT_DBEGRANT_RAISE_USER;
           break;
       case GRANT_REGISTER  :
           LoadString (hResource, (UINT)IDS_T_REVOKE_REGISTER_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_REGISTER] = TRUE;
           HaveBeenGranted = OT_DBEGRANT_REGTR_USER;
           break;
       case GRANT_EXECUTE   :
           LoadString (hResource, (UINT)IDS_T_REVOKE_EXECUTE_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_EXECUTE] = TRUE;
           HaveBeenGranted = OT_PROCGRANT_EXEC_USER;
           break;
       case GRANT_NEXT_SEQUENCE   :
           LoadString (hResource, (UINT)IDS_T_REVOKE_NEXT_SEQUENCE_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_NEXT_SEQUENCE] = TRUE;
           HaveBeenGranted = OT_SEQUGRANT_NEXT_USER;
           break;
       case GRANT_ALL       :
           LoadString (hResource, (UINT)IDS_T_REVOKE_ALL_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_ALL] = TRUE;
           bGrantAll = TRUE;
           switch (lprevoke->ObjectType)
           {
               case OT_TABLE:
                   break;
               case OT_PROCEDURE:
                   break;
               case OT_DBEVENT:
                   break;

           }
           HaveBeenGranted = -1;
           break;
   }

   parentstrings [0] = lprevoke->DBName;
   GetExactDisplayString (
       lprevoke->PreselectObject,
       lprevoke->PreselectObjectOwner,
       lprevoke->ObjectType,
       parentstrings,
       buffer);

   wsprintf (
       szStr,
       szTitle,
       GetVirtNodeName ( GetCurMdiNodeHandle ()),
       lprevoke->DBName,
       buffer);
   SetWindowText (hwnd, szStr);

   if ( lprevoke->ObjectType == OT_SEQUENCE)
   {
      //Hide control the controls not used for sequences
      ShowWindow( GetDlgItem (hwnd,IDC_REVOKE_TABLE_GRANT_OPTION) ,SW_HIDE); // Hided "With grant Option" control
      ShowWindow( GetDlgItem (hwnd,IDC_REVOKE_TABLE_PRIVILEGE)    ,SW_HIDE); 
      ShowWindow( GetDlgItem (hwnd,102)                           ,SW_HIDE);
      ShowWindow( GetDlgItem (hwnd,5020)                          ,SW_HIDE);
      // Unchecked the controls not used
      Button_SetCheck (GetDlgItem (hwnd, IDC_REVOKE_TABLE_PRIVILEGE),    FALSE);
      Button_SetCheck (GetDlgItem (hwnd, IDC_REVOKE_TABLE_CASCADE),      TRUE);
      Button_SetCheck (GetDlgItem (hwnd, IDC_REVOKE_TABLE_RESTRICT),     FALSE);
      Button_SetCheck (GetDlgItem (hwnd, IDC_REVOKE_TABLE_GRANT_OPTION), FALSE);
   }
   else
   {
      Button_SetCheck (GetDlgItem (hwnd, IDC_REVOKE_TABLE_PRIVILEGE),TRUE);
      Button_SetCheck (GetDlgItem (hwnd, IDC_REVOKE_TABLE_CASCADE),  TRUE);
   }

   if (lprevoke->GranteeType == OT_UNKNOWN)
   {
       HandleUnknounObject (hwnd);
   }
   else   
   if (lprevoke->GranteeType == OT_GROUP)
   {
       Button_SetCheck (GetDlgItem (hwnd, IDC_REVOKE_TABLE_GROUP), TRUE);
       FillGrantedUsers(hwndGrantees, lprevoke, HaveBeenGranted);
   }
   else
   if (lprevoke->GranteeType == OT_ROLE)
   {
       Button_SetCheck (GetDlgItem (hwnd, IDC_REVOKE_TABLE_ROLE),  TRUE);
       FillGrantedUsers(hwndGrantees, lprevoke, HaveBeenGranted);
   }
   else
   {
       if  (x_strcmp (lprevoke->PreselectGrantee, lppublicdispstring()) == 0)
           Button_SetCheck (GetDlgItem (hwnd, IDC_REVOKE_TABLE_PUBLIC),  TRUE);
       else
       {
           lprevoke->GranteeType = OT_USER;
           Button_SetCheck (GetDlgItem (hwnd, IDC_REVOKE_TABLE_USER),  TRUE);
           FillGrantedUsers(hwndGrantees, lprevoke, HaveBeenGranted);
       }
   }

   if (x_strlen (lprevoke->PreselectGrantee) > 0)
   {
       CAListBox_SelectString (hwndGrantees, -1, lprevoke->PreselectGrantee);
   }

   if (lprevoke->GranteeType != OT_UNKNOWN)
       EnableDisableOKButton  (hwnd);
   
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
   HWND    hwndGrantees  = GetDlgItem (hwnd, IDC_REVOKE_TABLE_GRANTEES);

   switch (id)
   {
       case IDOK:
       {
           TCHAR tchNameObject[MAXOBJECTNAME];
           int ires;
           int max_item_number;
           
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
           if (Button_GetCheck (GetDlgItem (hwnd, IDC_REVOKE_TABLE_USER)))
               lprevoke->GranteeType = OT_USER;
           else
           if (Button_GetCheck (GetDlgItem (hwnd, IDC_REVOKE_TABLE_GROUP)))
               lprevoke->GranteeType = OT_GROUP;
           else
           if (Button_GetCheck (GetDlgItem (hwnd, IDC_REVOKE_TABLE_ROLE)))
               lprevoke->GranteeType = OT_ROLE;
           else
           if (Button_GetCheck (GetDlgItem (hwnd, IDC_REVOKE_TABLE_PUBLIC)))
           {
               lprevoke->GranteeType = OT_PUBLIC;     // Public
               FreeObjectList (lprevoke->lpgrantee);
               lprevoke->lpgrantee = NULL;
               lprevoke->lpgrantee = APublicUser ();
               if (!lprevoke->lpgrantee)
               {
                   ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
                   break;
               }
           }
           
           lprevoke->grant_option = Button_GetCheck (GetDlgItem (hwnd, IDC_REVOKE_TABLE_GRANT_OPTION));
           lprevoke->cascade      = Button_GetCheck (GetDlgItem (hwnd, IDC_REVOKE_TABLE_CASCADE));
           wsprintf(tchNameObject,"%s.%s",QuoteIfNeeded(lprevoke->PreselectObjectOwner),QuoteIfNeeded(lprevoke->PreselectObject));
           lprevoke->lpobject     = InsertTableName (tchNameObject);
           if (!lprevoke->lpobject)
           {
               FreeObjectList (lprevoke->lpobject);
               lprevoke->lpobject = NULL;
               ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
               break;
           }
           
           ires = DBADropObject
                   ( GetVirtNodeName ( GetCurMdiNodeHandle ()),
                     OTLL_GRANT,
                     (void *) lprevoke);

           if (ires != RES_SUCCESS)
           {
               FreeObjectList (lprevoke->lpgrantee);
               FreeObjectList (lprevoke->lpobject);
               lprevoke->lpobject = NULL;
               lprevoke->lpgrantee= NULL;
               switch (lprevoke->ObjectType)
               {
                   case OT_TABLE:
                       ErrorMessage ((UINT)IDS_E_REVOKE_TABLE_FAILED, ires);
                       break;
                   case OT_PROCEDURE:
                       ErrorMessage ((UINT)IDS_E_REVOKE_PROCEDURE_FAILED, ires);
                       break;
                   case OT_DBEVENT:
                       ErrorMessage ((UINT)IDS_E_REVOKE_DBEVENT_FAILED, ires);
                       break;
                   case OT_SEQUENCE:
                       ErrorMessage ((UINT)IDS_E_REVOKE_SEQUENCE_FAILED, ires);
                       break;
               }
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

       case IDC_REVOKE_TABLE_USER:
           CAListBox_ResetContent (hwndGrantees);
           lprevoke->GranteeType = OT_USER;
           FillGrantedUsers (hwndGrantees, lprevoke, HaveBeenGranted);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_REVOKE_TABLE_GROUP:
           CAListBox_ResetContent (hwndGrantees);
           lprevoke->GranteeType = OT_GROUP;
           FillGrantedUsers (hwndGrantees, lprevoke, HaveBeenGranted);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_REVOKE_TABLE_ROLE:
           CAListBox_ResetContent (hwndGrantees);
           lprevoke->GranteeType = OT_ROLE;
           FillGrantedUsers (hwndGrantees, lprevoke, HaveBeenGranted);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_REVOKE_TABLE_PUBLIC:
           CAListBox_ResetContent (hwndGrantees);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_REVOKE_TABLE_GRANTEES:
           if (lprevoke->GranteeType != OT_UNKNOWN)
               EnableDisableOKButton  (hwnd);
           else
               EnableDisableOKButton2 (hwnd);

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
   HWND    hwndGrantees  = GetDlgItem (hwnd, IDC_REVOKE_TABLE_GRANTEES);
   BOOL    B [3];
   BOOL    C [2];

   B[0] = Button_GetCheck (GetDlgItem (hwnd, IDC_REVOKE_TABLE_PUBLIC));
   B[1] = (CAListBox_GetSelCount (hwndGrantees) >= 1);

   C[0] = !B[0] && B[1];
   C[1] =  B[0];

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

static void EnableDisableOKButton2 (HWND hwnd)
{
   HWND hwndGrantees  = GetDlgItem (hwnd, IDC_REVOKE_TABLE_GRANTEES);

   if (CAListBox_GetSelCount (hwndGrantees) >= 1)
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
}


static void FillGrantedUsers (HWND hwndCtl, LPREVOKEPARAMS lprevoke, int objectToFind)
{
   int     hdl, ires, gtype;
   BOOL    bwsystem;
   char    buf [MAXOBJECTNAME];
   char    buffilter [MAXOBJECTNAME];
   char    bufwithparent [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];
   LPDOMDATA lpDomData =GetCurLpDomData ();

   ZEROINIT (buf);
   ZEROINIT (buffilter);
   hdl      = GetCurMdiNodeHandle ();
   bwsystem = GetSystemFlag ();
   
   if (!lpDomData)
       return;

   parentstrings [0] = lprevoke->DBName;   // Database name
   if (lprevoke->ObjectType != OT_VIEW)
       parentstrings [1] = StringWithOwner ((LPTSTR)Quote4DisplayIfNeeded(lprevoke->PreselectObject),
                                            lprevoke->PreselectObjectOwner, bufwithparent);
   else
       parentstrings [1] = StringWithOwner ((LPTSTR)Quote4DisplayIfNeeded(lprevoke->PreselectObject),
                                            lprevoke->PreselectObjectOwner, bufwithparent);

   ires = DOMGetFirstObject (hdl, objectToFind, 2, parentstrings, bwsystem, NULL, buf, NULL, NULL);
   while (ires == RES_SUCCESS)
   {
       gtype = DBA_GetObjectType  (buf, lpDomData);
       if (gtype == lprevoke->GranteeType)
       {
           CAListBox_AddString (hwndCtl, buf);
       }
       ires  = DOMGetNextObject (buf, buffilter, NULL);
   }
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
       //
       // Cannot allocate memory
       //
       list = NULL;
   }
   return (list);
}


static void HandleUnknounObject (HWND hwnd)
{
   char message [100];
   HWND hwndGrantees    = GetDlgItem (hwnd, IDC_REVOKE_TABLE_GRANTEES);
   LPREVOKEPARAMS lprevoke = GetDlgProp (hwnd);
   HWND hwndLabel = GetDlgItem (hwnd, IDC_REVOKE_TABLE_LRGDG);  
   ShowWindow (GetDlgItem (hwnd, IDC_REVOKE_TABLE_USER  ), SW_HIDE);
   ShowWindow (GetDlgItem (hwnd, IDC_REVOKE_TABLE_GROUP ), SW_HIDE);
   ShowWindow (GetDlgItem (hwnd, IDC_REVOKE_TABLE_ROLE  ), SW_HIDE);
   ShowWindow (GetDlgItem (hwnd, IDC_REVOKE_TABLE_PUBLIC), SW_HIDE);

   LoadString    (hResource, (UINT)IDS_I_RG_on_DG, message, sizeof(message));
   SetWindowText (hwndLabel, message);
   ShowWindow (hwndLabel, SW_SHOW);
   CAListBox_AddString   (hwndGrantees, lprevoke->PreselectGrantee);
   CAListBox_SelectString(hwndGrantees, -1, lprevoke->PreselectGrantee); 
}

