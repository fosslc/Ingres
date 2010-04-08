/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : findobj.c
//
//    Dialog box for locating an object
**
**   26-Mar-2001 (noifr01)
**     (sir 104270) removal of code for managing Ingres/Desktop
**   18-Mar-2003 (noifr01)
**     sir 107523 management of sequences
********************************************************************/


#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "lexical.h"
#include "msghandl.h"
#include "getdata.h"
#include "esltools.h"
#include "domdata.h"
#include "dbaginfo.h"

static char String_all [MAXOBJECTNAME];
static int IngresObj4locate[]=   { OT_DATABASE          ,
                                   OT_USER              ,
                                   OT_GROUP             ,
                                   OT_ROLE              ,
                                   OT_LOCATION          ,
                                   OT_TABLE             ,
                                   OT_VIEW              ,
                                   OT_PROCEDURE         ,
                                   OT_SCHEMAUSER        ,
                                   OT_SYNONYM           ,
                                   OT_DBEVENT           ,
								   OT_SEQUENCE};

static int *Obj4locate()
{
   return IngresObj4locate;
}

static int nbObj4locate()
{
         return (sizeof (IngresObj4locate) / sizeof (int));
}


BOOL CALLBACK __export DlgLocateDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy       (HWND hwnd);
static void OnCommand       (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog    (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);

static BOOL FillStructureFromControls   (HWND hwnd);
static void FillObjectTypes             (HWND hwnd);
static void EnableDisableOKButton       (HWND hwnd);
static void ProhibitString_all          (HWND hwnd);
static BOOL ProhibitString_all2         (HWND hwnd);
static BOOL ObjectFound                 (HWND hwnd, int  objtype);


BOOL WINAPI __export DlgLocate (HWND hwndOwner, LPLOCATEPARAMS lplocate)
/*
// Function:
// Shows the specifyin database privileges dialog.
//
// Paramters:
//     1) hwndOwner:   Handle of the parent window for the dialog.
//     1) lplocate  :   Points to structure containing information used to 
//                     initialise the dialog. 
//
// Returns:
//     (TRUE, FALSE)
//
*/
{
   FARPROC lpProc;
   int     retVal;
   
   if (!IsWindow (hwndOwner) || !lplocate)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgLocateDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_LOCATE),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lplocate
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgLocateDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
   char    szFormat [100];
   char    szTitle  [MAX_TITLEBAR_LEN];

   LPLOCATEPARAMS lplocate = (LPLOCATEPARAMS)lParam;

   if (!AllocDlgProp (hwnd, lplocate))
       return FALSE;

   LoadString (hResource, (UINT)IDS_T_LOCATE, szFormat, sizeof (szFormat));
   wsprintf (szTitle, szFormat,  GetVirtNodeName (GetCurMdiNodeHandle ()));
   SetWindowText (hwnd, szTitle);
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_LOCATE));

   //
   // Load the string "(all)"
   //
   if (LoadString (hResource, (UINT)IDS_I_LOCATE_ALL, String_all, sizeof (String_all)) == 0)
       x_strcpy (String_all, "(all)");

   FillObjectTypes (hwnd);
   ComboBoxFillDatabases (GetDlgItem (hwnd, IDC_LOCATE_DATABASE));

   if (ComboBox_GetCount (GetDlgItem (hwnd, IDC_LOCATE_OBJECTTYPE)) > 0)
       ComboBox_SetCurSel(GetDlgItem (hwnd, IDC_LOCATE_OBJECTTYPE), 0);

   Edit_LimitText (GetDlgItem (hwnd, IDC_LOCATE_FIND), MAXOBJECTNAME -1);
   Edit_SetText   (GetDlgItem (hwnd, IDC_LOCATE_FIND), String_all);
   EnableDisableOKButton (hwnd);

   richCenterDialog(hwnd);
   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   DeallocDlgProp(hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPLOCATEPARAMS lplocate   = GetDlgProp (hwnd);

   switch (id)
   {
       case IDOK:
           if (FillStructureFromControls (hwnd))
               EndDialog (hwnd, TRUE);
           else
           {
               break;
           }
               
           break;

       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           break;

       case IDC_LOCATE_FIND:
           if (codeNotify == EN_CHANGE)
           {
               //ProhibitString_all    (hwnd);
               EnableDisableOKButton (hwnd);
           }
           break;

       case IDC_LOCATE_OBJECTTYPE:
           if (codeNotify == CBN_SELCHANGE)
           {
               /* Comment on Sept 29'95
               ProhibitString_all    (hwnd);
               */
               EnableDisableOKButton (hwnd);
           }
           break;

       case IDC_LOCATE_DATABASE:
           if (codeNotify == CBN_SELCHANGE)
           {    
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
   HWND hwndObjectType = GetDlgItem (hwnd, IDC_LOCATE_OBJECTTYPE);
   HWND hwndDatabase   = GetDlgItem (hwnd, IDC_LOCATE_DATABASE);
   HWND hwndLDatabase  = GetDlgItem (hwnd, IDC_LOCATE_LDATABASE);
   int  nSel;

   if (Edit_GetTextLength (GetDlgItem (hwnd, IDC_LOCATE_FIND)) == 0)
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);

   if ((nSel = ComboBox_GetCurSel (hwndObjectType)) != CB_ERR)
   {
       nSel = Obj4locate() [nSel];
       switch (nSel)
       {
           case OT_DATABASE:
           case OT_USER:
           case OT_GROUP:
           case OT_ROLE:
           case OT_LOCATION:
               ComboBox_SetCurSel (hwndDatabase, -1);
               EnableWindow    (hwndDatabase,  FALSE);
               EnableWindow    (hwndLDatabase, FALSE);
               break;

           default:
           {
               int idx;
               EnableWindow    (hwndDatabase,  TRUE);
               EnableWindow    (hwndLDatabase, TRUE);

               idx = ComboBox_GetCurSel (hwndDatabase);
               if (idx != CB_ERR)
                   EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
               else
                   EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);


               //if (ComboBox_GetCount (hwndDatabase) > 0)
               //{
               //    ComboBox_SetCurSel (hwndDatabase, 0);
               //}
               break;
           }
       }
   }
}


static void FillObjectTypes (HWND hwnd)
{
   int  i;
   char item [MAXOBJECTNAME];
   HWND hwndObjectType = GetDlgItem (hwnd, IDC_LOCATE_OBJECTTYPE);

   for (i = 0; i < nbObj4locate(); i++)
   {
       x_strcpy (item, (char*) ObjectTypeString (Obj4locate()[i], FALSE));
       ComboBox_AddString (hwndObjectType, item);
   }
}

static BOOL FillStructureFromControls (HWND hwnd)
{
   HWND hwndObjectType = GetDlgItem (hwnd, IDC_LOCATE_OBJECTTYPE);
   HWND hwndDatabase   = GetDlgItem (hwnd, IDC_LOCATE_DATABASE);
   HWND hwndFind       = GetDlgItem (hwnd, IDC_LOCATE_FIND);
   LPLOCATEPARAMS lplocate   = GetDlgProp (hwnd);

   int  nSel;
   char szObjectType [MAXOBJECTNAME];
   char szDatabase   [MAXOBJECTNAME];
   char szFind       [MAXOBJECTNAME];

   ZEROINIT (szObjectType);
   ZEROINIT (szDatabase  );
   ZEROINIT (szFind      );

   ComboBox_GetText (hwndObjectType, szObjectType, sizeof (szObjectType));
   if (IsWindowEnabled (hwndDatabase))
       ComboBox_GetText (hwndDatabase  , szDatabase  , sizeof (szDatabase  ));
   Edit_GetText (hwndFind, szFind, sizeof (szFind));
   if ((nSel = ComboBox_GetCurSel (hwndObjectType)) != CB_ERR)
       lplocate->ObjectType = Obj4locate() [nSel];
   else
       lplocate->ObjectType = -1;

   x_strcpy (lplocate->DBName,     szDatabase);
   x_strcpy (lplocate->FindString, szFind);
   if (!ProhibitString_all2 (hwnd))
       return FALSE;

   return TRUE;
}

static BOOL ObjectFound (HWND hwnd, int  objtype)
{
   int     iresulttype, ires;
   char    szFindStr [MAXOBJECTNAME];
   UCHAR   buf1 [MAXOBJECTNAME],
           buf2 [MAXOBJECTNAME],
           buf3 [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];

   HWND hwndDB   = GetDlgItem (hwnd, IDC_LOCATE_DATABASE);
   HWND hwndFind = GetDlgItem (hwnd, IDC_LOCATE_FIND);
   int  hdl      = GetCurMdiNodeHandle ();
   BOOL bwsystem = GetSystemFlag ();

   if (objtype < 0)
       return FALSE;
   else
   {
       switch (objtype)
       {
           case OT_DATABASE: 
           case OT_USER    : 
           case OT_GROUP   : 
           case OT_ROLE    : 
           case OT_LOCATION:
               Edit_GetText (hwndFind, szFindStr, sizeof (szFindStr));
               if (x_strcmp (szFindStr, String_all) == 0)
                   return TRUE;
               else
               {
                   ires = DOMGetObjectLimitedInfo
                           (hdl,
                            szFindStr,             // Object name to find
                            "",             // Object owner
                            objtype,               // Object type    
                            0,                     // Level
                            NULL,                  // Parent string
                            bwsystem,
                            &iresulttype,
                            buf1,
                            buf2,
                            buf3);
                   if (ires == RES_SUCCESS)
                       return TRUE;
                   else
                       return FALSE;
               }

           case OT_TABLE      :
           case OT_VIEW       :
           case OT_PROCEDURE  :
           case OT_SCHEMAUSER :
           case OT_SYNONYM    :
           case OT_DBEVENT    :
           case OT_SEQUENCE   :
               Edit_GetText (hwndFind, szFindStr, sizeof (szFindStr));
               if (x_strcmp (szFindStr, String_all) == 0)
                   return TRUE;
               else
               {
                   char szDbName [MAXOBJECTNAME];
                   Edit_GetText (hwndDB, szDbName, sizeof (szDbName));
                   parentstrings [0] = szDbName;

                   ires = DOMGetObjectLimitedInfo
                           (hdl,
                            szFindStr,             // Object name to find
                            "",             // Object owner
                            objtype,               // Object type    
                            1,                     // Level
                            parentstrings,         // Parent string
                            bwsystem,
                            &iresulttype,
                            buf1,
                            buf2,
                            buf3);
                   if (ires == RES_SUCCESS)
                       return TRUE;
                   else
                       return FALSE;
               }

           case OT_RULE       :
           case OT_INDEX      :
           case OT_INTEGRITY  :
               {
                   char szDbName [MAXOBJECTNAME];
                   Edit_GetText (hwndFind, szFindStr, sizeof (szFindStr));
                   if (x_strcmp (szFindStr, String_all) == 0)
                      return FALSE;
                   Edit_GetText (hwndDB, szDbName, sizeof (szDbName));
                   parentstrings [0] = szDbName;
          
                   ires = DOMGetObjectLimitedInfo
                           (hdl,
                            szFindStr,             // Object name to find
                            "",             // Object owner
                            objtype,               // Object type    
                            2,                     // Level
                            parentstrings,         // Parent string
                            bwsystem,
                            &iresulttype,
                            buf1,
                            buf2,
                            buf3);
                   if (ires == RES_SUCCESS)
                       return TRUE;
                   else
                       return FALSE;
               }


           default:
               return FALSE;
       }
   }
}

static void ProhibitString_all (HWND hwnd)
{
   int  nSel, i;
   HWND hwndObjectType = GetDlgItem (hwnd, IDC_LOCATE_OBJECTTYPE);
   HWND hwndFind       = GetDlgItem (hwnd, IDC_LOCATE_FIND);

   if ((nSel = ComboBox_GetCurSel (hwndObjectType)) != CB_ERR)
       i = Obj4locate() [nSel];
   else
       i = -1;

   //int iresulttype;
   //UCHAR buf1[MAXOBJECTNAME],buf2[MAXOBJECTNAME],buf3[MAXOBJECTNAME];
   //int DOMGetObjectLimitedInfo(hnodestruct,lpobjectname,iobjecttype,level,
   //           parentstrings, bwithsystem, &iresulttype,buf1,buf2,buf3);

   if (i == -1)
       return;
   switch (i)
   {
       case OT_RULE       :
       case OT_INDEX      :
       case OT_INTEGRITY  :
           Edit_SetText (hwndFind, "");
           break;

       default:
           Edit_SetText (hwndFind, String_all);
           break;
   }
}




static BOOL ProhibitString_all2 (HWND hwnd)
{
   int  nSel, i;
   char szFindStr [MAXOBJECTNAME];
   HWND hwndObjectType = GetDlgItem (hwnd, IDC_LOCATE_OBJECTTYPE);
   HWND hwndFind       = GetDlgItem (hwnd, IDC_LOCATE_FIND);

   if ((nSel = ComboBox_GetCurSel (hwndObjectType)) != CB_ERR)
       i = Obj4locate() [nSel];
   else
       i = -1;

   if (i == -1)
       return TRUE;
   switch (i)
   {
       case OT_RULE       :
       case OT_INDEX      :
       case OT_INTEGRITY  :
           Edit_GetText (hwndFind, szFindStr, sizeof (szFindStr));
           if (x_strcmp (szFindStr, String_all) == 0)
           {
               char* mess;
               char  mess2 [200];
               HWND currentFocus = GetFocus();

               mess = ResourceString ((UINT)IDS_E_CANNOT_USE_THIS);
               wsprintf (mess2, mess, String_all);
               MessageBox (NULL, mess2, NULL, MB_OK| MB_TASKMODAL);
               SetFocus (currentFocus);
               return FALSE;
           }

       default:
           return TRUE;
   }
}
