/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project  : Ingres Visual DBA
**
**  Source   : rvkgdb.c
**
**  Dialog box for revoke privilege from database. (General) 
**
**  History after 25-Mar-2003
**
**  26-Mar-2003 (noifr01)
**   (sir 107523) management of sequences
**  23-Jan-2004 (schph01)
**   (sir 104378) detect version 3 of Ingres, for managing
**    new features provided in this version. replaced references
**    to 2.65 with refereces to 3  in #define definitions for
**    better readability in the future
**
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


BOOL CALLBACK __export DlgRvkgdbDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy       (HWND hwnd);
static void OnCommand       (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog    (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);
static BOOL OccupyObjectsControl   (HWND hwnd);
static void EnableDisableOKButton  (HWND hwnd);
static void EnableDisableOKButton2 (HWND hwnd);
static void FillGrantedUsers (HWND hwnd, LPREVOKEPARAMS lprevoke);
static void FillNoPrivileges (HWND hwndCtl);
static void FillPrivileges   (HWND hwndCtl);

static LPCHECKEDOBJECTS GetGrantee      (LPREVOKEPARAMS lprevoke, int objectToFind, int grantee);
static LPOBJECTLIST     InsertTableName (LPUCHAR tableName);



int WINAPI __export DlgRvkgdb (HWND hwndOwner, LPREVOKEPARAMS lprevoke)
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

   lpProc = MakeProcInstance ((FARPROC) DlgRvkgdbDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_RVKGDB),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lprevoke
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgRvkgdbDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
   HWND hwndGrantees = GetDlgItem (hwnd, IDC_RVKGDB_GRANTEES);
   HWND hwndPriv     = GetDlgItem (hwnd, IDC_RVKGDB_PRIVILEGES);
   HWND hwndNoPriv   = GetDlgItem (hwnd, IDC_RVKGDB_NOPRIVILEGES);
   char szFormat [90];
   char szTitle  [MAX_TITLEBAR_LEN];
   //int  i, positive_privilege;

   
   if (!AllocDlgProp (hwnd, lprevoke))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();
   
   LoadString (hResource, (UINT)IDS_T_RVKGDB, szFormat, sizeof (szFormat));
   wsprintf (
       szTitle,
       szFormat,
       GetVirtNodeName (GetCurMdiNodeHandle ()),
       lprevoke->DBName);
   SetWindowText (hwnd, szTitle);
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_RVKGDB));

   Button_SetCheck (GetDlgItem (hwnd, IDC_RVKGDB_PRIV   ), TRUE);
   Button_SetCheck (GetDlgItem (hwnd, IDC_RVKGDB_CASCADE), TRUE);
   Button_SetCheck (GetDlgItem (hwnd, IDC_RVKGDB_USERS  ), TRUE);
   FillNoPrivileges(hwndNoPriv);
   FillPrivileges  (hwndPriv  );

   FillGrantedUsers(hwnd, lprevoke);
   EnableDisableOKButton  (hwnd);
   richCenterDialog(hwnd);
   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   DeallocDlgProp (hwnd);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   HWND hwndPrivileges     = GetDlgItem (hwnd, IDC_RVKGDB_PRIVILEGES);
   HWND hwndNoPrivileges   = GetDlgItem (hwnd, IDC_RVKGDB_NOPRIVILEGES);
   HWND hwndGrantees       = GetDlgItem (hwnd, IDC_RVKGDB_GRANTEES);
   LPREVOKEPARAMS lprevoke = GetDlgProp (hwnd);

   switch (id)
   {
       case IDOK:
       {
           int ires, max_item_number;
           
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
           lprevoke->grant_option = Button_GetCheck (GetDlgItem (hwnd, IDC_RVKGDB_GOPT));
           lprevoke->cascade      = Button_GetCheck (GetDlgItem (hwnd, IDC_RVKGDB_CASCADE));
           lprevoke->lpobject     = InsertTableName (lprevoke->DBName);
           lprevoke->ObjectType   = OT_DATABASE;
           
           ires = DBADropObject
                   ( GetVirtNodeName (GetCurMdiNodeHandle ()),
                     OTLL_GRANT,
                     (void *) lprevoke);

           if (ires != RES_SUCCESS)
           {
               FreeObjectList (lprevoke->lpgrantee);
               FreeObjectList (lprevoke->lpobject);
               lprevoke->lpobject = NULL;
               lprevoke->lpgrantee= NULL;
               ErrorMessage ((UINT)IDS_E_REVOKE_DATABASE_FAILED, ires);
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

       case IDC_RVKGDB_USERS:
           FillGrantedUsers (hwnd, lprevoke);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_RVKGDB_GROUPS:
           FillGrantedUsers (hwnd, lprevoke);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_RVKGDB_ROLES:
           FillGrantedUsers (hwnd, lprevoke);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_RVKGDB_PUBLIC:
           FillGrantedUsers (hwnd, lprevoke);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_RVKGDB_GRANTEES:
           if (codeNotify == CALBN_CHECKCHANGE)
               EnableDisableOKButton  (hwnd);
           break;


       case IDC_RVKGDB_NOPRIVILEGES:
           if (codeNotify == CALBN_CHECKCHANGE)
           {
               int  k, f, cursel;
               char item [MAXOBJECTNAME];
               
               ZEROINIT  (item);
               cursel = CAListBox_GetCurSel (hwndCtl);
               if (CAListBox_GetSel (hwndCtl, cursel))
               {
                   CAListBox_GetText  (hwndCtl, cursel, item);
                   for (k = 0; k < MAXPRIVNOPRIV; k++)
                   {
                       if (x_stricmp (GetPrivilegeString2 (PrivNoPriv[k].NoPriv), item) == 0)
                       {
                           f = CAListBox_FindString (hwndPrivileges, 0, GetPrivilegeString2 (PrivNoPriv[k].Priv));
                           CAListBox_SetSel (hwndPrivileges, FALSE, f);
                           break;
                       }
                   }
               }
               FillGrantedUsers (hwnd, lprevoke);
               EnableDisableOKButton (hwnd);
           }
           break;

       case IDC_RVKGDB_PRIVILEGES:
           if (codeNotify == CALBN_CHECKCHANGE)
           {
               int  k, f, cursel;
               char item [MAXOBJECTNAME];
               
               ZEROINIT  (item);
               cursel = CAListBox_GetCurSel (hwndCtl);
               if (CAListBox_GetSel (hwndCtl, cursel))
               {
                   CAListBox_GetText  (hwndCtl, cursel, item);
                   for (k = 0; k < MAXPRIVNOPRIV; k++)
                   {
                       if (x_stricmp (GetPrivilegeString2 (PrivNoPriv[k].Priv), item) == 0)
                       {
                           f = CAListBox_FindString (hwndNoPrivileges, 0, GetPrivilegeString2 (PrivNoPriv[k].NoPriv));
                           CAListBox_SetSel (hwndNoPrivileges, FALSE, f);
                           break;
                       }
                   }
               }
               FillGrantedUsers (hwnd, lprevoke);
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



static void EnableDisableOKButton (HWND hwnd)
{

   HWND hwndPrivileges   = GetDlgItem (hwnd, IDC_RVKGDB_PRIVILEGES);
   HWND hwndNoPrivileges = GetDlgItem (hwnd, IDC_RVKGDB_NOPRIVILEGES);
   HWND hwndGrantees     = GetDlgItem (hwnd, IDC_RVKGDB_GRANTEES);
   BOOL B [3];

   B[0] = (CAListBox_GetSelCount (hwndPrivileges) > 0) || (CAListBox_GetSelCount (hwndNoPrivileges) > 0);
   B[1] = (CAListBox_GetSelCount (hwndGrantees)   > 0) ;

   if (B[0] && B[1])
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
}


static void FillGrantedUsers (HWND hwnd, LPREVOKEPARAMS lprevoke)
{
   char*item;
   int  i, Who;
   int  first = -1;
   LPCHECKEDOBJECTS
       resl = NULL,
       temp = NULL,
       ls   = NULL,
       Lgtee= NULL;
   LPOBJECTLIST
       PrivilegeList   = NULL,  
       NoPrivilegeList = NULL,
       lp              = NULL,
       lx, lch         = NULL;
   HWND hwndGrantee        = GetDlgItem (hwnd, IDC_RVKGDB_GRANTEES);
   HWND hwndPrivileges     = GetDlgItem (hwnd, IDC_RVKGDB_PRIVILEGES);
   HWND hwndNoPrivileges   = GetDlgItem (hwnd, IDC_RVKGDB_NOPRIVILEGES);

   lch = AddItemToList    (hwndGrantee);
   CAListBox_ResetContent (hwndGrantee);
   PrivilegeList   = AddItemToList (hwndPrivileges);
   NoPrivilegeList = AddItemToList (hwndNoPrivileges);

   //
   // Reinitialize the privileges
   //
   for (i = 0; i<GRANT_MAX_PRIVILEGES; i++)
       lprevoke->Privileges [i] = FALSE;

   lp = PrivilegeList;
   while (lp)
   {
       item = (char* )lp->lpObject;
       lprevoke->Privileges [GetIdentifier (item)] = TRUE;
       lp = lp->lpNext;
   }
   lp = NoPrivilegeList;
   while (lp)
   {
       item = (char* )lp->lpObject;
       lprevoke->Privileges [GetIdentifier (item)] = TRUE;
       lp = lp->lpNext;
   }


   if (Button_GetCheck (GetDlgItem (hwnd, IDC_RVKGDB_USERS)))
   {
       Who = OT_USER;
       //CAListBoxFillUsers (hwndGrantee);
   }
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_RVKGDB_GROUPS)))
   {
       Who = OT_GROUP;
       //CAListBoxFillGroups (hwndGrantee);
   }
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_RVKGDB_ROLES)))
   {
       Who = OT_ROLE;
       //CAListBoxFillRoles (hwndGrantee);
   }
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_RVKGDB_PUBLIC)))
       Who = OT_PUBLIC;
   lprevoke->GranteeType = Who;
   
   for (i=0; i<GRANT_MAX_PRIVILEGES; i++)
   {
       if (lprevoke->Privileges [i])
       {
           first = i;
           resl  = GetGrantee (lprevoke, first, Who);    
           break;
       }
   }
   i = first +1;

   while (i < GRANT_MAX_PRIVILEGES)
   {
       if (lprevoke->Privileges [i])
       {
           Lgtee = GetGrantee (lprevoke, i, Who);    
           temp  = Union (resl, Lgtee);
           resl  = FreeCheckedObjects  (resl);
           resl  = DupList (temp);
           temp  = FreeCheckedObjects  (temp);
           Lgtee = FreeCheckedObjects  (Lgtee);
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
   FreeObjectList (PrivilegeList);
   FreeObjectList (NoPrivilegeList);
   lch             = NULL;
   PrivilegeList   = NULL;
   NoPrivilegeList = NULL;
   resl            = FreeCheckedObjects  (resl);
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

   switch (objectToFind)
   {
       case GRANT_ACCESS               :
           grant_user = OT_DBGRANT_ACCESY_USER;
           break; 
       case GRANT_CREATE_PROCEDURE     :
           grant_user = OT_DBGRANT_CREPRY_USER;
           break; 
       case GRANT_CREATE_TABLE         :
           grant_user = OT_DBGRANT_CRETBY_USER;
           break; 
       case GRANT_CONNECT_TIME_LIMIT   :
           grant_user = OT_DBGRANT_CNCTLY_USER;
           break; 
       case GRANT_DATABASE_ADMIN       :
           grant_user = OT_DBGRANT_DBADMY_USER;
           break; 
       case GRANT_LOCKMOD              :
           grant_user = OT_DBGRANT_LKMODY_USER;
           break; 
       case GRANT_QUERY_IO_LIMIT       :
           grant_user = OT_DBGRANT_QRYIOY_USER;
           break; 
       case GRANT_QUERY_ROW_LIMIT      :
           grant_user = OT_DBGRANT_QRYRWY_USER;
           break;
       case GRANT_SELECT_SYSCAT        :
           grant_user = OT_DBGRANT_SELSCY_USER;
           break; 
       case GRANT_SESSION_PRIORITY     :
           grant_user = OT_DBGRANT_SESPRY_USER;
           break; 
       case GRANT_UPDATE_SYSCAT        :
           grant_user = OT_DBGRANT_UPDSCY_USER;
           break; 
       case GRANT_TABLE_STATISTICS     :
           grant_user = OT_DBGRANT_TBLSTY_USER;
           break; 
       case GRANT_IDLE_TIME_LIMIT      :
           grant_user = OT_DBGRANT_IDLTLY_USER;
           break; 

       case GRANT_NO_ACCESS            :
           grant_user = OT_DBGRANT_ACCESN_USER;
           break; 
       case GRANT_NO_CREATE_PROCEDURE  :
           grant_user = OT_DBGRANT_CREPRN_USER;
           break; 
       case GRANT_NO_CREATE_TABLE      :
           grant_user = OT_DBGRANT_CRETBN_USER;
           break; 
       case GRANT_NO_CONNECT_TIME_LIMIT:
           grant_user = OT_DBGRANT_CNCTLN_USER;
           break; 
       case GRANT_NO_DATABASE_ADMIN    :
           grant_user = OT_DBGRANT_DBADMN_USER;
           break; 
       case GRANT_NO_LOCKMOD           :
           grant_user = OT_DBGRANT_LKMODN_USER;
           break; 
       case GRANT_NO_QUERY_IO_LIMIT    :
           grant_user = OT_DBGRANT_QRYION_USER;
           break; 
       case GRANT_NO_QUERY_ROW_LIMIT   :
           grant_user = OT_DBGRANT_QRYRWN_USER;
           break; 
       case GRANT_NO_SELECT_SYSCAT     :
           grant_user = OT_DBGRANT_SELSCN_USER;
           break; 
       case GRANT_NO_SESSION_PRIORITY  :
           grant_user = OT_DBGRANT_SESPRN_USER;
           break; 
       case GRANT_NO_UPDATE_SYSCAT     :
           grant_user = OT_DBGRANT_UPDSCN_USER; 
           break;
       case GRANT_NO_TABLE_STATISTICS  :
           grant_user = OT_DBGRANT_TBLSTN_USER;
           break; 
       case GRANT_NO_IDLE_TIME_LIMIT      :
           grant_user = OT_DBGRANT_IDLTLN_USER;
           break; 
       //
       // New Dlg Grant Database -> Modif revoke.
       case GRANT_QUERY_CPU_LIMIT:
           grant_user = OT_DBGRANT_QRYCPY_USER;
           break; 
       case GRANT_QUERY_PAGE_LIMIT:
           grant_user = OT_DBGRANT_QRYPGY_USER;
           break; 
       case GRANT_QUERY_COST_LIMIT:
           grant_user = OT_DBGRANT_QRYCOY_USER;
           break; 
       case GRANT_NO_QUERY_CPU_LIMIT:
           grant_user = OT_DBGRANT_QRYCPN_USER;
           break; 
       case GRANT_NO_QUERY_PAGE_LIMIT:
           grant_user = OT_DBGRANT_QRYPGN_USER;
           break; 
       case GRANT_NO_QUERY_COST_LIMIT:
           grant_user = OT_DBGRANT_QRYCON_USER;
           break; 
       case GRANT_CREATE_SEQUENCE:
           grant_user = OT_DBGRANT_SEQCRY_USER;
           break; 
       case GRANT_NO_CREATE_SEQUENCE:
           grant_user = OT_DBGRANT_SEQCRN_USER;
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




static void FillPrivileges  (HWND hwndCtl)
{
   int     i;
   char*   str;
   UINT    PrivTable [GRANT_MAXDB_PRIVILEGES] =
   {   GRANT_ACCESS,           
       GRANT_CREATE_PROCEDURE, 
       GRANT_CREATE_TABLE,     
       GRANT_CONNECT_TIME_LIMIT,  
       GRANT_DATABASE_ADMIN,   
       GRANT_LOCKMOD,          
       GRANT_QUERY_IO_LIMIT,   
       GRANT_QUERY_ROW_LIMIT,  
       GRANT_SELECT_SYSCAT,    
       GRANT_SESSION_PRIORITY, 
       GRANT_UPDATE_SYSCAT,    
       GRANT_TABLE_STATISTICS, 
       GRANT_IDLE_TIME_LIMIT,
       GRANT_QUERY_CPU_LIMIT, 
       GRANT_QUERY_PAGE_LIMIT,
       GRANT_QUERY_COST_LIMIT,
	   GRANT_CREATE_SEQUENCE
   };


   for (i = 0; i<GRANT_MAXDB_PRIVILEGES; i++)
   {
       if (PrivTable[i]==GRANT_CREATE_SEQUENCE && GetOIVers()<OIVERS_30)
          continue;
       str = GetPrivilegeString2 (PrivTable [i]);
       if (str)
       {
           CAListBox_AddString (hwndCtl, str);
       }
   }
}

static void FillNoPrivileges (HWND hwndCtl)
{
   int     i;
   char*   str;
   UINT    NoPrivTable [GRANT_MAXDB_NO_PRIVILEGES] =
   {   GRANT_NO_ACCESS,           
       GRANT_NO_CREATE_PROCEDURE, 
       GRANT_NO_CREATE_TABLE,     
       GRANT_NO_CONNECT_TIME_LIMIT,  
       GRANT_NO_DATABASE_ADMIN,   
       GRANT_NO_LOCKMOD,          
       GRANT_NO_QUERY_IO_LIMIT,   
       GRANT_NO_QUERY_ROW_LIMIT,  
       GRANT_NO_SELECT_SYSCAT,    
       GRANT_NO_SESSION_PRIORITY, 
       GRANT_NO_UPDATE_SYSCAT,    
       GRANT_NO_TABLE_STATISTICS, 
       GRANT_NO_IDLE_TIME_LIMIT,
       GRANT_NO_QUERY_CPU_LIMIT,
       GRANT_NO_QUERY_PAGE_LIMIT,
       GRANT_NO_QUERY_COST_LIMIT,
	   GRANT_NO_CREATE_SEQUENCE
   };
   
   for (i = 0; i<GRANT_MAXDB_NO_PRIVILEGES; i++)
   {
       if (NoPrivTable[i]==GRANT_NO_CREATE_SEQUENCE && GetOIVers()<OIVERS_30)
          continue;
       str = GetPrivilegeString2 (NoPrivTable [i]);
       if (str)
       {
           CAListBox_AddString (hwndCtl, str);
       }
   }
}

