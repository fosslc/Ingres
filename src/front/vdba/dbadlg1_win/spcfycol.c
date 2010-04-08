/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : spcfycol.c
//
//    Sub dialog box for specifying the columns of a table
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

static void OnDestroy        (HWND hwnd);
static void OnCommand        (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog     (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange (HWND hwnd);

static void EnableDisableOKButton     (HWND hwnd);
static BOOL FillStructureFromControls (HWND hwnd);
static BOOL ComboBoxFillTables2 (HWND hwnd);
static LPTABLExCOLS FindStringInListTableAndColumns (LPTABLExCOLS lpTablexCol, char* aString);


BOOL CALLBACK __export DlgSpecifyColumnsDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI __export DlgSpecifyColumns (HWND hwndOwner, LPSPECIFYCOLPARAMS lpspecifycolparams)

/*
// Function:
// Shows the Specify table dialog for AuditDB.
//
// Paramters:
//     1) hwndOwner:    Handle of the parent window for the dialog.
//     2) lpspecifycolparams: 
//                      Points to structure containing information used to 
//                      initialise the dialog. The dialog uses the same
//                      structure to return the result of the dialog.
//
// Returns: TRUE if Successful.
//
*/
{
   FARPROC lpProc;
   int     retVal;
   
   if (!IsWindow(hwndOwner) || !lpspecifycolparams)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgSpecifyColumnsDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_SPECIFY_COL),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpspecifycolparams
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgSpecifyColumnsDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM  lParam)
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
   LPSPECIFYCOLPARAMS lpspecifycol = (LPSPECIFYCOLPARAMS)lParam;
   HWND hwndTables  = GetDlgItem (hwnd, IDC_SPECIFY_COL_TABLE);
   HWND hwndColumns = GetDlgItem (hwnd, IDC_SPECIFY_COL_COL);
   char szTable [MAXOBJECTNAME];
   LPTABLExCOLS ls;

   if (!AllocDlgProp (hwnd, lpspecifycol))
       return FALSE;

   if (!ComboBoxFillTables2 (hwnd))
       ComboBoxDestroyItemData (hwndTables);

   if (ComboBox_GetCount (hwndTables) > 0)
   {
       char szOwner [MAXOBJECTNAME];
       char szAll   [MAXOBJECTNAME];
       int  nSel;

       ComboBoxSelectFirstStr (hwndTables);
       ComboBox_GetText (hwndTables, szTable, sizeof (szTable));
       nSel = ComboBox_GetCurSel (hwndTables);
       x_strcpy (szOwner, (char *)ComboBox_GetItemData (hwndTables, nSel));
       StringWithOwner  (szTable, szOwner, szAll);

       ls = lpspecifycol->lpTableAndColumns;
       while (ls)
       {
           if (x_strcmp (ls->TableName, szAll) == 0)
           {
               char* ColStr;
               LPOBJECTLIST lo = ls->lpCol;

               CAListBoxFillTableColumns
                   (hwndColumns,
                   lpspecifycol->DBName,
                   ls->TableName);
               
               while (lo)
               {
                   ColStr = (char *)lo->lpObject;
                   CAListBox_SelectString (hwndColumns, -1, ColStr);
                   lo = lo->lpNext;
               }

               break;
           }
           ls = ls->next;
       }
   }
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_SPECIFY_COL));

   EnableDisableOKButton (hwnd);
   richCenterDialog (hwnd);

   return TRUE;
}


static void OnDestroy (HWND hwnd)
{
   HWND hwndTables  = GetDlgItem (hwnd, IDC_SPECIFY_COL_TABLE);

   ComboBoxDestroyItemData (hwndTables);
   DeallocDlgProp (hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   HWND hwndTables  = GetDlgItem (hwnd, IDC_SPECIFY_COL_TABLE);
   HWND hwndColumns = GetDlgItem (hwnd, IDC_SPECIFY_COL_COL);
   LPSPECIFYCOLPARAMS lpspecifycol = GetDlgProp (hwnd);
   LPTABLExCOLS ls;

   switch (id)
   {
       case IDOK:
           {
               EndDialog (hwnd, TRUE);
           }
           break;

       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           break;

       case IDC_SPECIFY_COL_TABLE:
           switch (codeNotify)
           {
               case CBN_SELCHANGE:
               {
                   char szTable [MAXOBJECTNAME];
                   char szOwner [MAXOBJECTNAME];
                   char szAll   [MAXOBJECTNAME];
                   int  nSel;
                   
                   ls = lpspecifycol->lpTableAndColumns;

                   ComboBox_GetText (hwndTables, szTable, sizeof (szTable));
                   nSel = ComboBox_GetCurSel (hwndTables);
                   x_strcpy (szOwner, (char *)ComboBox_GetItemData (hwndTables, nSel));
                   StringWithOwner  (szTable, szOwner, szAll);
                   CAListBox_ResetContent    (hwndColumns);
                   CAListBoxFillTableColumns (hwndColumns, lpspecifycol->DBName, szAll);

                   while (ls)
                   {
                       if (x_strcmp (ls->TableName, szAll) == 0)
                       {
                           char* ColStr;
                           
                           LPOBJECTLIST lo = ls->lpCol;
                           while (lo)
                           {
                               ColStr = (char *)lo->lpObject;
                               CAListBox_SelectString (hwndColumns, -1, ColStr);
                               lo = lo->lpNext;
                           }
                           break;
                       }
                       ls = ls->next;
                   }
                   break;
               }
           }

       case IDC_SPECIFY_COL_COL:
           switch (codeNotify)
           {
               case CALBN_CHECKCHANGE:
               {
                   char szTable [MAXOBJECTNAME];
                   char szOwner [MAXOBJECTNAME];
                   char szAll   [MAXOBJECTNAME];
                   int  nSel;

                   ls = lpspecifycol->lpTableAndColumns;
                   ComboBox_GetText (hwndTables, szTable, sizeof (szTable));
                   nSel = ComboBox_GetCurSel (hwndTables);
                   x_strcpy (szOwner, (char *)ComboBox_GetItemData (hwndTables, nSel));
                   StringWithOwner  (szTable, szOwner, szAll);
                  
                   while (ls)
                   {
                       if (x_strcmp (ls->TableName, szAll) == 0)
                       {
                           FreeObjectList (ls->lpCol);
                           ls->lpCol = AddItemToList (hwndColumns);
                           break;
                       }
                       ls = ls->next;
                   }
                   break;
               }
           }
   }
}


static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}


static BOOL FillStructureFromControls (HWND hwnd)
{
   return TRUE;
}


static void EnableDisableOKButton (HWND hwnd)
{
   /*
   if (Edit_GetTextLength (GetDlgItem (hwnd, IDC_USERNAME)) == 0)
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
   */
}


static BOOL ComboBoxFillTables2 (HWND hwnd)
{
   int     hdl, ires, idx;
   BOOL    bwsystem;
   char    buf       [MAXOBJECTNAME];
   char    buf2      [MAXOBJECTNAME];
   char    szOwner   [MAXOBJECTNAME];
   char*   buffowner;
   LPUCHAR parentstrings [MAXPLEVEL];
   LPSPECIFYCOLPARAMS lpspecifycol = GetDlgProp (hwnd);
   HWND hwndTables  = GetDlgItem (hwnd, IDC_SPECIFY_COL_TABLE);

   ZEROINIT (buf);
   parentstrings [0] = lpspecifycol->DBName;
   parentstrings [1] = NULL;

   hdl      = GetCurMdiNodeHandle ();
   bwsystem = GetSystemFlag ();
   
   ires = DOMGetFirstObject (
       hdl,
       OT_TABLE,
       1,
       parentstrings,
       bwsystem,
       NULL,
       buf,
       szOwner,
       NULL);
   while (ires == RES_SUCCESS)
   {
       StringWithOwner (buf, szOwner, buf2);
       if (FindStringInListTableAndColumns (lpspecifycol->lpTableAndColumns, buf2))
       {
           idx = ComboBox_AddString   (hwndTables, buf);
           buffowner = ESL_AllocMem (x_strlen (szOwner) +1);
           if (!buffowner)
               return FALSE;
           x_strcpy (buffowner, szOwner);
           ComboBox_SetItemData (hwndTables, idx, buffowner);
       }

       ires    = DOMGetNextObject (buf, szOwner, NULL);
   }
   return TRUE;
}

static LPTABLExCOLS
FindStringInListTableAndColumns (LPTABLExCOLS lpTablexCol, char* aString)
{
   LPTABLExCOLS ls = lpTablexCol;


   while (ls)
   {
       if (x_strcmp (ls->TableName, aString) == 0)
           return (ls);

       ls = ls->next;
   }
   return NULL;
}

