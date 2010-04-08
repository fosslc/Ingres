/*****************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project  : Visual DBA
**
**    Source   : auditdbf.c
**
**    Specify files for AuditDB dialog box
**
**
**  History after 04-May-1999:
**
**   19-Jan-2000 (schph01)
**     Bug #99993
**       - manage "Ok" and "Cancel" button for all modifications to do by the
**         user in the names of files.
**
******************************************************************************/

#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "getdata.h"
#include "msghandl.h"
#include "lexical.h"
#include "domdata.h"


static void OnDestroy                  (HWND hwnd);
static void OnCommand                  (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog               (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange           (HWND hwnd);
static BOOL ComboBoxFillTable          (HWND hwnd);
static LPTABLExFILE FindStringInListTableAndFiles (LPTABLExFILE lpTablexFile, char* aString);


BOOL CALLBACK __export DlgAuditDBFileDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI __export DlgAuditDBFile (HWND hwndOwner, LPAUDITDBFPARAMS lpauditdbfparams)

/*
// Function:
// Shows the Specify table dialog for AuditDB.
//
// Paramters:
//     1) hwndOwner:    Handle of the parent window for the dialog.
//     2) lpauditdbparams: 
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
   
   if (!IsWindow(hwndOwner) || !lpauditdbfparams)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgAuditDBFileDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_AUDITDBF),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpauditdbfparams
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgAuditDBFileDlgProc
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
   LPAUDITDBFPARAMS lpauditdbf = (LPAUDITDBFPARAMS)lParam;
   HWND hwndTables  = GetDlgItem (hwnd, IDC_AUDITDBF_TABLE);
   HWND hwndFile    = GetDlgItem (hwnd, IDC_AUDITDBF_FILE);
   LPTABLExFILE ls;

   if (!AllocDlgProp (hwnd, lpauditdbf))
       return FALSE;

   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_AUDITDBF));

   if (!ComboBoxFillTable (hwnd))
       ComboBoxDestroyItemData (hwndTables);

   if (ComboBox_GetCount (hwndTables) > 0)
   {
       char szTable [MAXOBJECTNAME];
       char szOwner [MAXOBJECTNAME];
       char szAll   [MAXOBJECTNAME];
       int  nSel;

       ComboBoxSelectFirstStr (hwndTables);
       ComboBox_GetText (hwndTables, szTable, sizeof (szTable));
       nSel = ComboBox_GetCurSel (hwndTables);
       x_strcpy (szOwner, (char *)ComboBox_GetItemData (hwndTables, nSel));
       StringWithOwner (szTable, szOwner, szAll);
       ls = FindStringInListTableAndFiles (lpauditdbf->lpTableAndFile, szAll);
       if (ls)
       {
           Edit_SetText (hwndFile, ls->FileName);
           Edit_LimitText ( hwndFile, sizeof(ls->FileName)-1);
       }
   }
   richCenterDialog (hwnd);

   return TRUE;
}


static void OnDestroy (HWND hwnd)
{
   HWND hwndTables  = GetDlgItem (hwnd, IDC_AUDITDBF_TABLE);
   ComboBoxDestroyItemData (hwndTables);
   DeallocDlgProp (hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   char szTable [MAXOBJECTNAME];
   char szOwner [MAXOBJECTNAMENOSCHEMA];
   char szAll   [MAXOBJECTNAME];
   int  nSel,nNbTables,index;
   HWND hwndTables  = GetDlgItem (hwnd, IDC_AUDITDBF_TABLE);
   HWND hwndFile    = GetDlgItem (hwnd, IDC_AUDITDBF_FILE);
   LPAUDITDBFPARAMS lpauditdbf = GetDlgProp (hwnd);
   LPTABLExFILE ls;

   switch (id)
   {
       case IDOK:
           nNbTables = ComboBox_GetCount (hwndTables);
           for (index = 0; index < nNbTables;index++)
           {
               ComboBox_GetLBText (hwndTables, index,szTable);
               x_strcpy (szOwner, (char *)ComboBox_GetItemData (hwndTables, index));
               StringWithOwner (szTable, szOwner, szAll);
               ls = FindStringInListTableAndFiles (lpauditdbf->lpTableAndFile, szAll);
               if (ls)
               {
                   if (x_strlen(ls->FileNameModifyByUser))
                   {
                       x_strcpy (ls->FileName, ls->FileNameModifyByUser);
                       x_strcpy (ls->FileNameModifyByUser,"");
                   }
               }
           }
           EndDialog (hwnd, TRUE);
           break;
       case IDCANCEL:
           nNbTables = ComboBox_GetCount (hwndTables);
           for (index = 0; index < nNbTables;index++)
           {
               ComboBox_GetLBText(hwndTables, index, szTable);
               x_strcpy (szOwner, (char *)ComboBox_GetItemData (hwndTables, index));
               StringWithOwner (szTable, szOwner, szAll);
               ls = FindStringInListTableAndFiles (lpauditdbf->lpTableAndFile, szAll);
               if (ls)
                   x_strcpy (ls->FileNameModifyByUser,"");
           }
           EndDialog (hwnd, FALSE);
           break;
       case IDC_AUDITDBF_TABLE:
           switch (codeNotify)
           {
               case CBN_SELCHANGE:
               {
                   ls = lpauditdbf->lpTableAndFile;
                   ComboBox_GetText (hwndTables, szTable, sizeof (szTable));
                   nSel = ComboBox_GetCurSel (hwndTables);
                   x_strcpy (szOwner, (char *)ComboBox_GetItemData (hwndTables, nSel));
                   StringWithOwner (szTable, szOwner, szAll);
                   ls = FindStringInListTableAndFiles (lpauditdbf->lpTableAndFile, szAll);
                   if (ls)
                   {
                       if (!x_strlen(ls->FileNameModifyByUser))
                           Edit_SetText (hwndFile, ls->FileName);
                       else
                           Edit_SetText (hwndFile, ls->FileNameModifyByUser);
                   }
                   break;
               }
           }
           break;
       case IDC_AUDITDBF_FILE:
           switch (codeNotify)
           {
               case EN_CHANGE:
               {
                   ComboBox_GetText (hwndTables, szTable, sizeof (szTable));
                   nSel = ComboBox_GetCurSel (hwndTables);
                   x_strcpy (szOwner, (char *)ComboBox_GetItemData (hwndTables, nSel));
                   StringWithOwner (szTable, szOwner, szAll);
                   ls = FindStringInListTableAndFiles (lpauditdbf->lpTableAndFile, szAll);
                   if (ls)
                       Edit_GetText (hwndFile, ls->FileNameModifyByUser, sizeof (ls->FileNameModifyByUser));
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


static BOOL ComboBoxFillTable (HWND hwnd)
{
   int     hdl, ires, idx;
   BOOL    bwsystem;
   char    buf       [MAXOBJECTNAME];
   char    buf2      [MAXOBJECTNAME];
   char    szOwner   [MAXOBJECTNAME];
   char*   buffowner;
   LPUCHAR parentstrings [MAXPLEVEL];
   LPAUDITDBFPARAMS lpauditdbf = GetDlgProp (hwnd);
   HWND hwndTables  = GetDlgItem (hwnd, IDC_AUDITDBF_TABLE);

   ZEROINIT (buf);
   parentstrings [0] = lpauditdbf->DBName;
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
       if (FindStringInListTableAndFiles (lpauditdbf->lpTableAndFile, buf2))
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


static LPTABLExFILE
FindStringInListTableAndFiles (LPTABLExFILE lpTablexFile, char* aString)
{
   LPTABLExFILE ls = lpTablexFile;


   while (ls)
   {
       if (x_strcmp (ls->TableName, aString) == 0)
           return (ls);

       ls = ls->next;
   }
   return NULL;
}

