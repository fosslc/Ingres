/*****************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project  : Visual DBA
**
**    Source   : creatsyn.c
**
**    Dialog box for Creating a Synonym
**
**    History:
**
**    24-Mar-2000 (schph01)
**      (bug #101000)
**      Since the change for bug #97680, the structure used to generate the
**      SQL syntax cannot be filled with the StringWithOwner() function.
**      Since this structure has two fields, one for the "object owner" and
**      another for the"object name", each field needs to be filled
**      separately.
**    21-Mar-2001 (noifr01)
**      (sir 104270) removal of code for managing Ingres/Desktop
*****************************************************************************/

#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "getdata.h"
#include "lexical.h"
#include "msghandl.h"
#include "domdata.h"
#include "tools.h"

#include "dbaginfo.h"   // GetGWType()

static int XXX_OLDTYPE = OT_UNKNOWN;

static void OnDestroy                 (HWND hwnd);
static void OnCommand                 (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog              (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange          (HWND hwnd);
static void EnableDisableOKButton     (HWND hwnd);       
static BOOL FillComboBoxWithObject    (HWND hwndCtl, int ObjectType, LPSYNONYMPARAMS lpsynonymparams);
static void InitialiseEditControls    (HWND hwnd);


BOOL CALLBACK __export DlgCreateSynonymDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI __export DlgCreateSynonym (HWND hwndOwner, LPSYNONYMPARAMS lpsynonymparams)

/*
// Function:
// Shows the Create synonym dialog.
//
// Paramters:
//     1) hwndOwner:   Handle of the parent window for the dialog.
//     1) lpsynonym:   Points to structure containing information used to 
//                     initialise the dialog. 
//
// Returns:
//     (TRUE, FALSE)
//
*/
{
   FARPROC lpProc;
   int     retVal;

   if (!IsWindow (hwndOwner) || !lpsynonymparams)
   {
       ASSERT(NULL);
       return FALSE;
   }
   
   lpProc = MakeProcInstance ((FARPROC) DlgCreateSynonymDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_SYNONYM),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpsynonymparams
       );

   FreeProcInstance (lpProc);
   return (retVal);
}



BOOL CALLBACK __export DlgCreateSynonymDlgProc
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
   LPSYNONYMPARAMS lpsynonym   = (LPSYNONYMPARAMS)lParam;
   HWND    hwndObject          = GetDlgItem (hwnd, IDC_CS_OBJECT );
   LPUCHAR parentstrings [MAXPLEVEL];
   char    szFormat [100];
   char    szTitle  [MAX_TITLEBAR_LEN];

   if (!AllocDlgProp (hwnd, lpsynonym))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();

   if (lpsynonym->bCreate)
   {
       LoadString (hResource, (UINT)IDS_T_CREATE_SYNONYM, szFormat, sizeof (szFormat));
       lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_SYNONYM));
   }
   //else
   //    LoadString (hResource, (UINT)IDS_T_ALTER_SYNONYM,  szFormat, sizeof (szFormat));
   wsprintf (szTitle, szFormat,
       GetVirtNodeName ( GetCurMdiNodeHandle ()),
       lpsynonym->DBName);
   SetWindowText (hwnd, szTitle);
   InitialiseEditControls (hwnd);

   //
   // Set the object type by default to 'TABLE'
   //
   Button_SetCheck (GetDlgItem (hwnd, IDC_CS_TABLE), TRUE);
   //
   // Get the available tables names and insert them into the combo box
   // 
   parentstrings [0] = lpsynonym->DBName;              
   if (!FillComboBoxWithObject (hwndObject, OT_TABLE, lpsynonym))
       ComboBoxDestroyItemData (hwndObject);
   //
   // Disable the OK button if there is neither text in the 
   // synonym name control nor object is selected
   //
   EnableDisableOKButton (hwnd);       

   richCenterDialog (hwnd);
   return TRUE;
}



static void OnDestroy (HWND hwnd)
{
   HWND hwndObject = GetDlgItem (hwnd, IDC_CS_OBJECT);

   ComboBoxDestroyItemData (hwndObject);
   DeallocDlgProp(hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   HWND    hwndObject        = GetDlgItem (hwnd, IDC_CS_OBJECT);
   LPSYNONYMPARAMS lpsynonym = GetDlgProp (hwnd);
   int     ires;
   char    szBuffer [2*MAXOBJECTNAME];

   switch (id)
   {
       case IDOK:
       {
           char szSynonymName [MAXOBJECTNAME];
           char szObjectName  [MAXOBJECTNAME];

           HWND hwndSynonymName = GetDlgItem (hwnd, IDC_CS_NAME);
           int  nSel;

           Edit_GetText (hwndSynonymName, szSynonymName, MAXOBJECTNAME);
           if (!IsObjectNameValid (szSynonymName, OT_UNKNOWN))
           {
               MsgInvalidObject ((UINT)IDS_E_NOT_A_VALIDE_OBJECT);
               Edit_SetSel (hwndSynonymName, 0, -1);
               SetFocus (hwndSynonymName);
               break;
           }
           else
               x_strcpy (lpsynonym->ObjectName, szSynonymName);
           //
           // Get the object selected
           //
           ComboBox_GetText   (hwndObject,   szBuffer, sizeof (szBuffer));
           nSel = ComboBox_GetCurSel (hwndObject);
           if (nSel != CB_ERR)
               x_strcpy (lpsynonym->RelatedObjectOwner, (char *)ComboBox_GetItemData (hwndObject, nSel));
           if (Button_GetCheck (GetDlgItem (hwnd, IDC_CS_INDEX)) )
           {
               ExcludeBraceString (szObjectName, szBuffer);
               x_strcpy (lpsynonym->RelatedObject, (LPTSTR)RemoveDisplayQuotesIfAny((LPTSTR)StringWithoutOwner (szObjectName)));
           }
           else
               x_strcpy (lpsynonym->RelatedObject, (LPTSTR)RemoveDisplayQuotesIfAny((LPTSTR)StringWithoutOwner (szBuffer)));

           //
           // Create and add object
           //
           ires = DBAAddObject
               ( GetVirtNodeName ( GetCurMdiNodeHandle ()),
                 OT_SYNONYM,
                 (void *) lpsynonym );

           if (ires != RES_SUCCESS)
           {
               ErrorMessage ((UINT) IDS_E_CREATE_SYNONYM_FAILED, ires);
               break;
           }
           else
           {
               EndDialog (hwnd, TRUE);
           }
       }
       break;


       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           break;

       case IDC_CS_OBJECT:
       {
           switch (codeNotify)
           {
               case CBN_SELCHANGE:
                   EnableDisableOKButton (hwnd);
                   break;
           }
       }
       break;

       case IDC_CS_TABLE:
           FillComboBoxWithObject (hwndObject, OT_TABLE, lpsynonym);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_CS_VIEW:
           FillComboBoxWithObject (hwndObject, OT_VIEW, lpsynonym);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_CS_INDEX:
           FillComboBoxWithObject (hwndObject, OT_INDEX, lpsynonym);
           EnableDisableOKButton  (hwnd);
           break;

       case IDC_CS_NAME:
           EnableDisableOKButton  (hwnd);
           break;
   }
}


static void OnSysColorChange (HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}



static void InitialiseEditControls (HWND hwnd)
{
   HWND hwndSynonymName  = GetDlgItem (hwnd, IDC_CS_NAME);

   Edit_LimitText (hwndSynonymName , MAXOBJECTNAME-1);
}



static void EnableDisableOKButton (HWND hwnd)
{
   HWND hwndSynonymName = GetDlgItem (hwnd, IDC_CS_NAME);
   HWND hwndObjectName  = GetDlgItem (hwnd, IDC_CS_OBJECT);

   if ((Edit_GetTextLength (hwndSynonymName) == 0) || 
       (ComboBox_GetCount (hwndObjectName) == 0))
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
}



static BOOL FillComboBoxWithObject (HWND hwndCtl, int ObjectType, LPSYNONYMPARAMS lpsynonymparams)
{
   UCHAR   buf         [MAXOBJECTNAME];
   UCHAR   buf2        [MAXOBJECTNAME];
   UCHAR   buf3        [MAXOBJECTNAME];
   UCHAR   buffowner   [MAXOBJECTNAME];

   LPUCHAR buffowner2;
   char*   result;

   int     hdl      = GetCurMdiNodeHandle ();
   BOOL    bwsystem = GetSystemFlag ();
   int     ires, idx;
   UCHAR   xbuff  [2*MAXOBJECTNAME +1];
   LPUCHAR parentstrings [MAXPLEVEL];

   parentstrings [0] = lpsynonymparams->DBName;
   parentstrings [1] = NULL;

   switch (ObjectType)
   {
       case OT_TABLE:
           if (XXX_OLDTYPE != OT_UNKNOWN)
           {
               ComboBoxDestroyItemData (hwndCtl);
               ComboBox_ResetContent   (hwndCtl);
           }
           ComboBoxFillTables (hwndCtl, lpsynonymparams->DBName);
           ComboBox_SetCurSel (hwndCtl, 0);
           XXX_OLDTYPE = OT_TABLE;
           break;

       case OT_VIEW:
           if (XXX_OLDTYPE != OT_UNKNOWN)
           {
               ComboBoxDestroyItemData (hwndCtl);
               ComboBox_ResetContent   (hwndCtl);
           }
           ComboBoxFillViews (hwndCtl, lpsynonymparams->DBName);
           ComboBox_SetCurSel (hwndCtl, 0);
           XXX_OLDTYPE = OT_VIEW;
           break;

       case OT_INDEX:
       {
           LPCHECKEDOBJECTS  obj, lo;
           LPCHECKEDOBJECTS  list = NULL;

           if (XXX_OLDTYPE != OT_UNKNOWN)
           {
               ComboBoxDestroyItemData (hwndCtl);
               ComboBox_ResetContent   (hwndCtl);
           }

           //
           // Get the table names from the database :'parent'
           //
           ires = DOMGetFirstObject (
               hdl,
               OT_TABLE,
               1,
               parentstrings,
               bwsystem,
               NULL,
               buf,
               buf2,
               NULL);
           while (ires == RES_SUCCESS)
           {
               obj  = ESL_AllocMem (sizeof (CHECKEDOBJECTS));
               result = x_strchr (buf, '.');
               if (result)
                   obj->bchecked = TRUE;
               else
                   obj->bchecked = FALSE;
               StringWithOwner (buf, buf2, buffowner);
               x_strcpy (obj->dbname,  buffowner);

               list = AddCheckedObjectTail (list, obj);
               ires = DOMGetNextObject (buf, buf2, NULL);
           }
           //
           // For each table, we loop to get all index
           //
           lo =  list;
           parentstrings [0] = lpsynonymparams->DBName;
           while (lo)
           {
               parentstrings [1] = lo->dbname; // dbname is a table name ...!
               ires = DOMGetFirstObject (
                   hdl,
                   OT_INDEX,
                   2,
                   parentstrings,
                   bwsystem,
                   NULL,
                   buf,
                   buf2,
                   NULL);

               while (ires == RES_SUCCESS)
               {
                   if (!lo->bchecked)
                   {
                       x_strcpy (buf3,    StringWithoutOwner (lo->dbname));
                       wsprintf (xbuff, "[%s] ", buf3);
                   }
                   else
                       wsprintf (xbuff, "[%s] ", lo->dbname);
                   x_strcat   (xbuff, buf);  // [table] index.
                   idx  =   ComboBox_AddString (hwndCtl, xbuff);

                   buffowner2 = ESL_AllocMem (x_strlen (buf2) +1);
                   if (!buffowner2)
                       return FALSE;
                   x_strcpy (buffowner2, buf2);
                   ComboBox_SetItemData (hwndCtl, idx, buffowner2);

                   ires =   DOMGetNextObject (buf, buf2, NULL);
               }
               lo = lo->pnext;
           }
           ComboBox_SetCurSel (hwndCtl, 0);
           list = FreeCheckedObjects (list);
       }
       break;
   }
   return TRUE;
}




