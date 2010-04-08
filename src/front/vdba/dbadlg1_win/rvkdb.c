/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project  : Ingres Visual DBA
**
**  Source   : rvkdb.c
**
**  Dialog box for revoke privilege from DB
**
**  History after 25-Mar-2003
**
**  26-Mar-2003 (noifr01)
**   (sir 107523) management of sequences
**
********************************************************************/
#include <ctype.h>
#include <stdlib.h>
#include "dll.h"
#include "subedit.h"
#include "dlgres.h"
#include "catolist.h"
#include "catospin.h"
#include "lexical.h"
#include "getdata.h"
#include "msghandl.h"
#include "domdata.h"
#include "esltools.h"
#include "resource.h"

//
// Global variable:  HaveBeenGranted is to be useed when retrieving the grantees.
// It indicates what type of grantee (USER, GROUP, ROLE) with a privilege. ie:
// User with privilege ACCESS (this means: HaveBeenGranted = OT_STATIC_R_DBGRANT_ACCESY).
// 

static int HaveBeenGranted;




BOOL CALLBACK __export DlgRevokeOnDatabaseDlgProc
               (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy              (HWND hwnd);
static void OnCommand              (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog           (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange       (HWND hwnd);
static BOOL OccupyObjectsControl   (HWND hwnd);
static void EnableDisableOKButton  (HWND hwnd);
static void FillGrantedUsers       (HWND hwndCtl, LPREVOKEPARAMS lprevoke, int objectToFind);
static LPOBJECTLIST InsertTableName(LPUCHAR tableName);



int WINAPI __export DlgRevokeOnDatabase (HWND hwndOwner, LPREVOKEPARAMS lprevoke)
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

   lpProc = MakeProcInstance ((FARPROC) DlgRevokeOnDatabaseDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_REVOKE_DATABASE),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lprevoke
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgRevokeOnDatabaseDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
   HWND    hwndGrantees    = GetDlgItem (hwnd, IDC_REVOKE_DATABASE_GRANTEES);
   char    szTitle [90];
   char    szStr   [MAX_TITLEBAR_LEN];
   int     i, positive_privilege;
   char    *pc;

   if (!AllocDlgProp (hwnd, lprevoke))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();
   // The value 9040 is defined in MAINMFC.H but is not accessible from this file:
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT (lprevoke->bInstallLevel? 9040: (UINT)IDD_REVOKE_DATABASE));

   for (i = 0; i < GRANT_MAX_PRIVILEGES; i++)
   {
       if (lprevoke->PreselectPrivileges [i])
       {
           positive_privilege = i;
           break;
       }
   }

   switch (positive_privilege)
   {
       case GRANT_ACCESS:
           LoadString (hResource, (UINT)IDS_T_REVOKE_ACCESS_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_ACCESS] = TRUE;
           HaveBeenGranted = OT_DBGRANT_ACCESY_USER;
           break;
       case GRANT_CREATE_PROCEDURE:
           LoadString (hResource, (UINT)IDS_T_REVOKE_CREATE_PROCEDURE_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_CREATE_PROCEDURE] = TRUE;
           HaveBeenGranted = OT_DBGRANT_CREPRY_USER;
           break;
       case GRANT_CREATE_TABLE:         
           LoadString (hResource, (UINT)IDS_T_REVOKE_CREATE_TABLE_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_CREATE_TABLE] = TRUE;
           HaveBeenGranted = OT_DBGRANT_CRETBY_USER;
           break;
       case GRANT_CONNECT_TIME_LIMIT:   
           LoadString (hResource, (UINT)IDS_T_REVOKE_CONNECT_TIME_LIMIT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_CONNECT_TIME_LIMIT] = TRUE;
           HaveBeenGranted = OT_DBGRANT_CNCTLY_USER ;
           break;
       case GRANT_DATABASE_ADMIN:       
           LoadString (hResource, (UINT)IDS_T_REVOKE_DATABASE_ADMIN_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_DATABASE_ADMIN] = TRUE;
           HaveBeenGranted = OT_DBGRANT_DBADMY_USER;
           break;
       case GRANT_LOCKMOD:              
           LoadString (hResource, (UINT)IDS_T_REVOKE_LOCKMOD_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_LOCKMOD] = TRUE;
           HaveBeenGranted = OT_DBGRANT_LKMODY_USER;
           break;
       case GRANT_QUERY_IO_LIMIT:       
           LoadString (hResource, (UINT)IDS_T_REVOKE_QUERY_IO_LIMIT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_QUERY_IO_LIMIT] = TRUE;
           HaveBeenGranted = OT_DBGRANT_QRYIOY_USER;
           break;
       case GRANT_QUERY_ROW_LIMIT:      
           LoadString (hResource, (UINT)IDS_T_REVOKE_QUERY_ROW_LIMIT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_QUERY_ROW_LIMIT] = TRUE;
           HaveBeenGranted = OT_DBGRANT_QRYRWY_USER;
           break;
       case GRANT_SELECT_SYSCAT:        
           LoadString (hResource, (UINT)IDS_T_REVOKE_SELECT_SYSCAT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_SELECT_SYSCAT] = TRUE;
           HaveBeenGranted = OT_DBGRANT_SELSCY_USER;
           break;
       case GRANT_SESSION_PRIORITY:     
           LoadString (hResource, (UINT)IDS_T_REVOKE_SESSION_PRIORITY_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_SESSION_PRIORITY] = TRUE;
           HaveBeenGranted = OT_DBGRANT_SESPRY_USER;
           break;
       case GRANT_UPDATE_SYSCAT:        
           LoadString (hResource, (UINT)IDS_T_REVOKE_UPDATE_SYSCAT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_UPDATE_SYSCAT] = TRUE;
           HaveBeenGranted = OT_DBGRANT_UPDSCY_USER;
           break;
       case GRANT_TABLE_STATISTICS:     
           LoadString (hResource, (UINT)IDS_T_REVOKE_TABLE_STATISTICS_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_TABLE_STATISTICS] = TRUE;
           HaveBeenGranted = OT_DBGRANT_TBLSTY_USER;
           break;
       case GRANT_IDLE_TIME_LIMIT:     
           LoadString (hResource, (UINT)IDS_T_REVOKE_IDLE_TIME_LIMIT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_IDLE_TIME_LIMIT] = TRUE;
           HaveBeenGranted = OT_DBGRANT_IDLTLY_USER;
           break;
   
           
       case GRANT_NO_ACCESS:
           LoadString (hResource, (UINT)IDS_T_REVOKE_NO_ACCESS_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_NO_ACCESS] = TRUE;
           HaveBeenGranted = OT_DBGRANT_ACCESN_USER;
           break;
       case GRANT_NO_CREATE_PROCEDURE:
           LoadString (hResource, (UINT)IDS_T_REVOKE_NO_CREATE_PROCEDURE_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_NO_CREATE_PROCEDURE] = TRUE;
           HaveBeenGranted = OT_DBGRANT_CREPRN_USER;
           break;
       case GRANT_NO_CREATE_TABLE:         
           LoadString (hResource, (UINT)IDS_T_REVOKE_NO_CREATE_TABLE_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_NO_CREATE_TABLE] = TRUE;
           HaveBeenGranted = OT_DBGRANT_CRETBN_USER;
           break;
       case GRANT_NO_CONNECT_TIME_LIMIT:   
           LoadString (hResource, (UINT)IDS_T_REVOKE_NO_CONNECT_TIME_LIMIT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_NO_CONNECT_TIME_LIMIT] = TRUE;
           HaveBeenGranted = OT_DBGRANT_CNCTLN_USER;
           break;
       case GRANT_NO_DATABASE_ADMIN:       
           LoadString (hResource, (UINT)IDS_T_REVOKE_NO_DATABASE_ADMIN_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_NO_DATABASE_ADMIN] = TRUE;
           HaveBeenGranted = OT_DBGRANT_DBADMN_USER;
           break;
       case GRANT_NO_LOCKMOD:              
           LoadString (hResource, (UINT)IDS_T_REVOKE_NO_LOCKMOD_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_NO_LOCKMOD] = TRUE;
           HaveBeenGranted = OT_DBGRANT_LKMODN_USER;
           break;
       case GRANT_NO_QUERY_IO_LIMIT:       
           LoadString (hResource, (UINT)IDS_T_REVOKE_NO_QUERY_IO_LIMIT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_NO_QUERY_IO_LIMIT] = TRUE;
           HaveBeenGranted = OT_DBGRANT_QRYION_USER;
           break;
       case GRANT_NO_QUERY_ROW_LIMIT:      
           LoadString (hResource, (UINT)IDS_T_REVOKE_NO_QUERY_ROW_LIMIT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_NO_QUERY_ROW_LIMIT] = TRUE;
           HaveBeenGranted = OT_DBGRANT_QRYRWN_USER;
           break;
       case GRANT_NO_SELECT_SYSCAT:        
           LoadString (hResource, (UINT)IDS_T_REVOKE_NO_SELECT_SYSCAT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_NO_SELECT_SYSCAT] = TRUE;
           HaveBeenGranted = OT_DBGRANT_SELSCN_USER;
           break;
       case GRANT_NO_SESSION_PRIORITY:     
           LoadString (hResource, (UINT)IDS_T_REVOKE_NO_SESSION_PRIORITY_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_NO_SESSION_PRIORITY] = TRUE;
           HaveBeenGranted = OT_DBGRANT_SESPRN_USER;
           break;
       case GRANT_NO_UPDATE_SYSCAT:        
           LoadString (hResource, (UINT)IDS_T_REVOKE_NO_UPDATE_SYSCAT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_NO_UPDATE_SYSCAT] = TRUE;
           HaveBeenGranted = OT_DBGRANT_UPDSCN_USER ;
           break;
       case GRANT_NO_TABLE_STATISTICS:     
           LoadString (hResource, (UINT)IDS_T_REVOKE_NO_TABLE_STATISTICS_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_NO_TABLE_STATISTICS] = TRUE;
           HaveBeenGranted = OT_DBGRANT_TBLSTN_USER;
           break;
       case GRANT_NO_IDLE_TIME_LIMIT:     
           LoadString (hResource, (UINT)IDS_T_REVOKE_NO_IDLE_TIME_LIMIT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_NO_IDLE_TIME_LIMIT] = TRUE;
           HaveBeenGranted = OT_DBGRANT_IDLTLN_USER;
           break;

       /* 26-Mar-2003. Added grants for sequences */
       case GRANT_CREATE_SEQUENCE:
           LoadString (hResource, (UINT)IDS_T_REVOKE_CREATE_SEQUENCE, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_CREATE_SEQUENCE] = TRUE;
           HaveBeenGranted = OT_DBGRANT_SEQCRY_USER;
           break;
       case GRANT_NO_CREATE_SEQUENCE:
           LoadString (hResource, (UINT)IDS_T_REVOKE_NO_CREATE_SEQUENCE, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_NO_CREATE_SEQUENCE] = TRUE;
           HaveBeenGranted = OT_DBGRANT_SEQCRN_USER;
           break;

       case GRANT_ALL       :
           LoadString (hResource, (UINT)IDS_T_REVOKE_ALL_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_ALL] = TRUE;
           HaveBeenGranted = -1;
           break;
       //
       // New Dlg Grant Database -> Modif revoke.
       case GRANT_QUERY_CPU_LIMIT       :
           LoadString (hResource, (UINT)IDS_T_REVOKE_QUERY_CPU_LIMIT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_QUERY_CPU_LIMIT] = TRUE;
           HaveBeenGranted = OT_DBGRANT_QRYCPY_USER;
           break;

       case GRANT_QUERY_PAGE_LIMIT:
           LoadString (hResource, (UINT)IDS_T_REVOKE_QUERY_PAGE_LIMIT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_QUERY_PAGE_LIMIT] = TRUE;
           HaveBeenGranted = OT_DBGRANT_QRYPGY_USER;
           break;

       case GRANT_QUERY_COST_LIMIT:
           LoadString (hResource, (UINT)IDS_T_REVOKE_QUERY_COST_LIMIT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_QUERY_COST_LIMIT] = TRUE;
           HaveBeenGranted = OT_DBGRANT_QRYCOY_USER;
           break;

       case GRANT_NO_QUERY_CPU_LIMIT:
           LoadString (hResource, (UINT)IDS_T_REVOKE_NO_QUERY_CPU_LIMIT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_NO_QUERY_CPU_LIMIT] = TRUE;
           HaveBeenGranted = OT_DBGRANT_QRYCPN_USER;
           break;

       case GRANT_NO_QUERY_PAGE_LIMIT:
           LoadString (hResource, (UINT)IDS_T_REVOKE_NO_QUERY_PAGE_LIMIT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_NO_QUERY_PAGE_LIMIT] = TRUE;
           HaveBeenGranted = OT_DBGRANT_QRYPGN_USER;
           break;

       case GRANT_NO_QUERY_COST_LIMIT:
           LoadString (hResource, (UINT)IDS_T_REVOKE_NO_QUERY_COST_LIMIT_ON, szTitle, sizeof (szTitle));
           lprevoke->Privileges [GRANT_NO_QUERY_COST_LIMIT] = TRUE;
           HaveBeenGranted = OT_DBGRANT_QRYCON_USER;
           break;
   }

   if (lprevoke->bInstallLevel)
       pc=ResourceString(IDS_CURRENT_INSTALLATION);
   else
       pc=lprevoke->DBName;
   wsprintf (szStr,
       szTitle,
       GetVirtNodeName ( GetCurMdiNodeHandle ()),
       pc);
   SetWindowText (hwnd, szStr);


   if (lprevoke->GranteeType == OT_GROUP)
   {
       Button_SetCheck (GetDlgItem (hwnd, IDC_REVOKE_DATABASE_GROUP), TRUE);
       FillGrantedUsers(hwndGrantees, lprevoke, HaveBeenGranted);
   }
   else
   if (lprevoke->GranteeType == OT_ROLE)
   {
       Button_SetCheck (GetDlgItem (hwnd, IDC_REVOKE_DATABASE_ROLE),  TRUE);
       FillGrantedUsers(hwndGrantees, lprevoke, HaveBeenGranted);
   }
   else
   {
       if  (x_strcmp (lprevoke->PreselectGrantee, lppublicdispstring()) == 0)
           Button_SetCheck (GetDlgItem (hwnd, IDC_REVOKE_DATABASE_PUBLIC),  TRUE);
       else
       {
           lprevoke->GranteeType = OT_USER;
           Button_SetCheck (GetDlgItem (hwnd, IDC_REVOKE_DATABASE_USER),  TRUE);
           FillGrantedUsers(hwndGrantees, lprevoke, HaveBeenGranted);
       }
   }

   if (x_strlen (lprevoke->PreselectGrantee) > 0)
   {
       CAListBox_SelectString (hwndGrantees, -1, lprevoke->PreselectGrantee);
   }        
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
   HWND    hwndGrantees  = GetDlgItem (hwnd, IDC_REVOKE_DATABASE_GRANTEES);

   switch (id)
   {
       case IDOK:
       {
           int ires;
           int max_item_number;
           LPOBJECTLIST obj;
           
           max_item_number = CAListBox_GetSelCount (hwndGrantees);
           if (max_item_number >= 1)
           {
               lprevoke->lpgrantee = AddItemToList (hwndGrantees);
               if (!lprevoke->lpgrantee)
               {
                   ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
                   break;
               }
           }
           if (Button_GetCheck (GetDlgItem (hwnd, IDC_REVOKE_DATABASE_USER)))
               lprevoke->GranteeType = OT_USER;
           else
           if (Button_GetCheck (GetDlgItem (hwnd, IDC_REVOKE_DATABASE_GROUP)))
               lprevoke->GranteeType = OT_GROUP;
           else
           if (Button_GetCheck (GetDlgItem (hwnd, IDC_REVOKE_DATABASE_ROLE)))
               lprevoke->GranteeType = OT_ROLE;
           else
           if (Button_GetCheck (GetDlgItem (hwnd, IDC_REVOKE_DATABASE_PUBLIC)))
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
           
           obj = AddListObject (lprevoke->lpobject, x_strlen (lprevoke->DBName) +1);
           if (obj)
           {
               x_strcpy ((UCHAR *)obj->lpObject, lprevoke->DBName);
               lprevoke->lpobject = obj;
           }
           else
           {
               ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
               FreeObjectList (lprevoke->lpgrantee);
               lprevoke->lpgrantee = NULL;
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
               lprevoke->lpgrantee = NULL;
               lprevoke->lpobject  = NULL;
               ErrorMessage ((UINT)IDS_E_REVOKE_DATABASE_FAILED, ires);
               break;
           }
           else 
           {
               EndDialog (hwnd, TRUE);
           }
           FreeObjectList (lprevoke->lpobject);
           FreeObjectList (lprevoke->lpgrantee);
           lprevoke->lpgrantee = NULL;
           lprevoke->lpobject  = NULL;
       }
       break;

       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           break;

       case IDC_REVOKE_DATABASE_USER:
           CAListBox_ResetContent (hwndGrantees);
           lprevoke->GranteeType = OT_USER;
           FillGrantedUsers (hwndGrantees, lprevoke, HaveBeenGranted);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_REVOKE_DATABASE_GROUP:
           CAListBox_ResetContent (hwndGrantees);
           lprevoke->GranteeType = OT_GROUP;
           FillGrantedUsers (hwndGrantees, lprevoke, HaveBeenGranted);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_REVOKE_DATABASE_ROLE:
           CAListBox_ResetContent (hwndGrantees);
           lprevoke->GranteeType = OT_ROLE;
           FillGrantedUsers (hwndGrantees, lprevoke, HaveBeenGranted);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_REVOKE_DATABASE_PUBLIC:
           CAListBox_ResetContent (hwndGrantees);
           lprevoke->GranteeType = OT_PUBLIC;
           FillGrantedUsers (hwndGrantees, lprevoke, HaveBeenGranted);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_REVOKE_DATABASE_GRANTEES:
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
   HWND    hwndGrantees  = GetDlgItem (hwnd, IDC_REVOKE_DATABASE_GRANTEES);
   BOOL    B [3];
   BOOL    C [2];

   B[0] = Button_GetCheck (GetDlgItem (hwnd, IDC_REVOKE_DATABASE_PUBLIC));
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

static void FillGrantedUsers (HWND hwndCtl, LPREVOKEPARAMS lprevoke, int objectToFind)
{
   int     hdl, ires, gtype;
   BOOL    bwsystem;
   char    buf [MAXOBJECTNAME];
   char    buffilter [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];
   LPDOMDATA lpDomData =GetCurLpDomData ();

   ZEROINIT (buf);
   ZEROINIT (buffilter);
   hdl      = GetCurMdiNodeHandle ();
   bwsystem = GetSystemFlag ();
   
   if (!lpDomData)
       return;

   parentstrings [0] = lprevoke->DBName;           // Database name
   parentstrings [1] = lprevoke->PreselectObject;  // Table name

   ires = DOMGetFirstObject (hdl, objectToFind, 2,
             parentstrings, bwsystem, NULL, buf, NULL, NULL);

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

