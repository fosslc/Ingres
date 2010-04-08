/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : verifdb.c
//
//    Dialog box for generating the verifydb  command string.
//
//  History after 04-May-1999:
//
//   13-Jan-2000 (schph01)
//     Bug #75260 Remove the schema before the table name, when the SQL
//     statement is generate for the operations, xtable,drop_table and table.
//
********************************************************************/

#include "dll.h"
#include "catolist.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "getdata.h"
#include "msghandl.h"
#include "domdata.h"
#include "rmcmd.h"
#include "resource.h"

#define VERIFDB_MAXMODE 4
static char* MODE [VERIFDB_MAXMODE] = {
   "report",
   "run",
   "runinteractive",
   "runsilent"
};

#define VERIFDB_MAXOPERATION 10
static char* OPERATION [VERIFDB_MAXOPERATION] = {
   "accesscheck",
   "purge",
   "temp_purge",
   "expired_purge",
   "drop_table",
   "table",
   "dbms_catalogs",
   "refresh_ldbs",
   "force_consistent",
   "xtable"
};

#define VERIFDB_MAXSCOPE 3
static char* SCOPE [VERIFDB_MAXSCOPE] = {
   "dba",
   "installation",
   "dbname"
};


BOOL CALLBACK __export DlgVerifyDBDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy(HWND hwnd);
static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);
static void FillAllComboBoxes (HWND hwnd);
static void InitialiseEditControls (HWND hwnd);
static void EnableDisableTableControl (HWND hwnd);
static BOOL FillStructureFromControls (HWND hwnd);



BOOL WINAPI __export DlgVerifyDB (HWND hwndOwner, LPVERIFYDBPARAMS lpverifydb)
/*
// Function:
//     Shows the Checkpoint dialog.
//
// Paramters:
//     1) hwndOwner:  Handle of the parent window for the dialog.
//     2) lpverifydb: Points to structure containing information used to 
//                    initialise the dialog. The dialog uses the same
//                    structure to return the result of the dialog.
//
// Returns:
//     TRUE if successful.
//
// Note:
//     If invalid parameters are passed in they are reset to a default
// so the dialog can be safely displayed.
*/
{
   FARPROC lpProc;
   int     retVal;

   if (!IsWindow(hwndOwner) || !lpverifydb)
   {
       ASSERT (NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgVerifyDBDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_VERIFDB),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpverifydb
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgVerifyDBDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
       HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
       HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);

       case WM_INITDIALOG:
           return HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, OnInitDialog);

       default:
           return HandleUserMessages (hwnd, message, wParam, lParam);

   }
   return TRUE;
}


static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   LPVERIFYDBPARAMS lpverifydb = (LPVERIFYDBPARAMS)lParam;
   char szFormat [100];
   char szTitle  [MAX_TITLEBAR_LEN];
   HWND hwndDatabases = GetDlgItem (hwnd, IDC_VERIFDB_DATABASE);
   HWND hwndTables    = GetDlgItem (hwnd, IDC_VERIFDB_TABLE);
   HWND hwndScope     = GetDlgItem (hwnd, IDC_VERIFDB_SCOPE);

   //
   // force catolist.dll to load
   //
   CATOListDummy();
   if (!AllocDlgProp(hwnd, lpverifydb))
       return FALSE;

   LoadString (hResource, (UINT)IDS_T_VERIFYDB, szFormat, sizeof (szFormat));
   wsprintf (szTitle, szFormat,
       GetVirtNodeName ( GetCurMdiNodeHandle ()));
   SetWindowText (hwnd, szTitle);

   FillAllComboBoxes (hwnd);
   CAListBoxFillDatabases (hwndDatabases);
   if (x_strlen (lpverifydb->DBName) > 0)
   {
       CAListBox_SelectString (hwndDatabases, -1, lpverifydb->DBName);
       if (!ComboBoxFillTables (hwndTables, lpverifydb->DBName))
           ComboBoxDestroyItemData (hwndTables);
       if (ComboBox_GetCount(hwndTables) >= 1)  {
           int iret;
           if (lpverifydb->bStartSinceTable) {
               iret = ComboBox_SelectString (hwndTables, -1, lpverifydb->szDisplayTableName);
               if (iret != CB_ERR)
                   EnableWindow(hwndTables,FALSE);
               else
                   ComboBox_SetCurSel (hwndTables, 0);
           }
           else
               ComboBox_SetCurSel (hwndTables, 0);
           }
       ComboBox_SelectString (hwndScope, -1, SCOPE [2]);
   }
   EnableDisableTableControl(hwnd);
   InitialiseEditControls(hwnd);
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_VERIFDB));
   richCenterDialog(hwnd);

   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   HWND hwndTables    = GetDlgItem (hwnd, IDC_VERIFDB_TABLE);

   ComboBoxDestroyItemData (hwndTables);
   DeallocDlgProp(hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   HWND hwndDatabases = GetDlgItem (hwnd, IDC_VERIFDB_DATABASE);
   HWND hwndTables    = GetDlgItem (hwnd, IDC_VERIFDB_TABLE);
   HWND hwndOperation = GetDlgItem (hwnd, IDC_VERIFDB_OPERATION);
   HWND hwndNologmode = GetDlgItem (hwnd, IDC_VERIFDB_NOLOGMODE);
   HWND hwndLogto     = GetDlgItem (hwnd, IDC_VERIFDB_LOGTO);
   HWND hwndLogfile   = GetDlgItem (hwnd, IDC_VERIFDB_LOGTO_EDIT);
 
   switch (id)
   {
       case IDOK:
           if (FillStructureFromControls (hwnd))
               EndDialog(hwnd, TRUE);
           break;

       case IDCANCEL:
           EndDialog(hwnd, FALSE);
           break;

       case IDC_VERIFDB_DATABASE:
           switch (codeNotify)
           {
               case CALBN_CHECKCHANGE:
                   EnableDisableTableControl (hwnd);
                   if (CAListBox_GetSelCount (hwndDatabases) > 10)
                   {   int cn = CAListBox_GetCurSel (hwndDatabases);
                       CAListBox_SetSel (hwndDatabases, FALSE, cn);
                       WarningMessage   ((UINT) IDS_I_UPTO10DATABASES);
                   }
                   if (CAListBox_GetSelCount (hwndDatabases) == 1)
                   {
                       LPOBJECTLIST Database = AddItemToList (hwndDatabases);
                       char* dbname;

                       if (Database)
                       {
                           ComboBoxDestroyItemData (hwndTables);
                           ComboBox_ResetContent   (hwndTables);
                           dbname = (char *)Database->lpObject;
                           ComboBoxFillTables (hwndTables, dbname);
                           if (ComboBox_GetCount(hwndTables) >= 1)  {
                              ComboBox_SetCurSel (hwndTables, 0);
                           }
                           FreeObjectList (Database);
                           Database = NULL;
                       }
                   }
                   break;
           }
           break;

       case IDC_VERIFDB_OPERATION:
           if (codeNotify = CBN_SELCHANGE)
               EnableDisableTableControl (hwnd);
           break;

       case IDC_VERIFDB_NOLOGMODE:
       case IDC_VERIFDB_LOGTO:
           if (Button_GetCheck (hwndNologmode))
           {
               EnableWindow (hwndLogto,  FALSE);
               EnableWindow (hwndLogfile,FALSE);
               Edit_SetText (hwndLogfile, "");
           }
           else
           {
               EnableWindow (hwndLogto,  TRUE);
               EnableWindow (hwndLogfile,TRUE);
               Edit_SetText (hwndLogfile, "");
           }


           if (Button_GetCheck (hwndLogto))
               EnableWindow (hwndNologmode,  FALSE);
           else
               EnableWindow (hwndNologmode,  TRUE);

           break;
   }
}


static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}



static void InitialiseEditControls (HWND hwnd)
{
   HWND hwndLogfile    = GetDlgItem (hwnd, IDC_VERIFDB_LOGTO_EDIT);

   Edit_LimitText (hwndLogfile, 2*MAXOBJECTNAME -1);
}

static void FillAllComboBoxes (HWND hwnd)
{
   int  i;
   HWND hwndMode       = GetDlgItem (hwnd, IDC_VERIFDB_MODE);
   HWND hwndScope      = GetDlgItem (hwnd, IDC_VERIFDB_SCOPE);
   HWND hwndOperation  = GetDlgItem (hwnd, IDC_VERIFDB_OPERATION);

   for (i = 0; i < VERIFDB_MAXMODE; i++)
       ComboBox_AddString (hwndMode, MODE [i]);
   ComboBox_SelectString  (hwndMode, -1, MODE [0]);
  
   for (i = 0; i < VERIFDB_MAXSCOPE; i++)
       ComboBox_AddString (hwndScope, SCOPE [i]);
   ComboBox_SelectString  (hwndScope, -1, SCOPE [0]);

   for (i = 0; i < VERIFDB_MAXOPERATION; i++)
       ComboBox_AddString (hwndOperation, OPERATION [i]);
   ComboBox_SelectString  (hwndOperation, -1, OPERATION [6]);
}

static void EnableDisableTableControl (HWND hwnd)
{
   char szBuffer1 [MAXOBJECTNAME];
   LPVERIFYDBPARAMS      lpverifydb  = GetDlgProp (hwnd);

   HWND hwndDatabases = GetDlgItem (hwnd, IDC_VERIFDB_DATABASE);
   HWND hwndTables    = GetDlgItem (hwnd, IDC_VERIFDB_TABLE);
   HWND hwndLTables   = GetDlgItem (hwnd, IDC_VERIFDB_LTABLE);
   HWND hwndOperation = GetDlgItem (hwnd, IDC_VERIFDB_OPERATION);
   BOOL B [4];

   ComboBox_GetText (hwndOperation, szBuffer1, sizeof (szBuffer1));
   B [0] = (x_strcmp (szBuffer1, OPERATION [4]) == 0) ||
           (x_strcmp (szBuffer1, OPERATION [5]) == 0) ||
           (x_strcmp (szBuffer1, OPERATION [9]) == 0);
   B [1] = CAListBox_GetSelCount (hwndDatabases) == 1;

   if (B [0] && B [1])
   {
       ShowWindow (hwndTables,  SW_SHOW);
       ShowWindow (hwndLTables, SW_SHOW);
       if (lpverifydb->bStartSinceTable)
           EnableWindow(hwndTables,FALSE);
   }
   else
   {
       ShowWindow (hwndTables,  SW_HIDE);
       ShowWindow (hwndLTables, SW_HIDE);
   }
}


static BOOL FillStructureFromControls (HWND hwnd)
{
   char szGreatBuffer [3000];
   char szBuffer      [2*MAXOBJECTNAME];
   char szBuffer2     [2*MAXOBJECTNAME];
   char buftemp       [200];
   BOOL bReportMode = FALSE;

   HWND hwndMode       = GetDlgItem (hwnd, IDC_VERIFDB_MODE);
   HWND hwndScope      = GetDlgItem (hwnd, IDC_VERIFDB_SCOPE);
   HWND hwndOperation  = GetDlgItem (hwnd, IDC_VERIFDB_OPERATION);
   HWND hwndDatabases  = GetDlgItem (hwnd, IDC_VERIFDB_DATABASE);
   HWND hwndTables     = GetDlgItem (hwnd, IDC_VERIFDB_TABLE);
   HWND hwndLogfile    = GetDlgItem (hwnd, IDC_VERIFDB_LOGTO_EDIT);
   HWND hwndVerbose    = GetDlgItem (hwnd, IDC_VERIFDB_VERBOSE);
   HWND hwndNologmode  = GetDlgItem (hwnd, IDC_VERIFDB_NOLOGMODE);
   HWND hwndLogto      = GetDlgItem (hwnd, IDC_VERIFDB_LOGTO);

   LPUCHAR vnodeName   = GetVirtNodeName (GetCurMdiNodeHandle ());
   LPVERIFYDBPARAMS      lpverifydb  = GetDlgProp (hwnd);
   ZEROINIT (szGreatBuffer);
   ZEROINIT (szBuffer);

   x_strcpy (szGreatBuffer, "verifydb ");

   ZEROINIT (szBuffer);
   if (DBAGetSessionUser(vnodeName, buftemp)) {
      x_strcat (szGreatBuffer, " -u");
      suppspace(buftemp);
      x_strcat (szGreatBuffer, buftemp);
   }

   //
   // Generate -mmode
   //
   ZEROINIT (szBuffer);
   ComboBox_GetText (hwndMode, szBuffer, sizeof (szBuffer));
   if (x_strlen (szBuffer) > 0)
   {
       x_strcat (szGreatBuffer, " -m");
       x_strcat (szGreatBuffer, szBuffer);
       if ((x_strcmp (szBuffer, MODE [0]) == 0))
           bReportMode = TRUE;
   }

   //
   // Generate -sscope
   //
   ZEROINIT (szBuffer);
   ComboBox_GetText (hwndScope, szBuffer, sizeof (szBuffer));
   if (x_strlen (szBuffer) > 0)
   {
       LPOBJECTLIST lsdatabase = NULL;

       if ((x_strcmp (szBuffer, SCOPE [2]) == 0) && (CAListBox_GetSelCount (hwndDatabases) > 0))
       {
           //
           // scope = dbname
           //
           char* dbName;
           LPOBJECTLIST ls;
           lsdatabase = AddItemToList (hwndDatabases);
           x_strcat (szGreatBuffer, " -s");
           x_strcat (szGreatBuffer, szBuffer);
           
           ls = lsdatabase;
           x_strcat (szGreatBuffer, " \"");
           while (ls)
           {
               dbName = (char *)ls->lpObject;
               x_strcat (szGreatBuffer, dbName);

               ls = ls->lpNext;
               if (ls)
                   x_strcat (szGreatBuffer, " ");
           }
           x_strcat (szGreatBuffer, "\"");

           FreeObjectList (lsdatabase);
           lsdatabase = NULL;
       }
       else
       {
           x_strcat (szGreatBuffer, " -s");
           x_strcat (szGreatBuffer, szBuffer);
       }
   }

   //
   // Generate -ooperation
   //
   ZEROINIT (szBuffer);
   ZEROINIT (szBuffer2);

   ComboBox_GetText (hwndOperation, szBuffer, sizeof (szBuffer));
   if (IsWindowVisible (hwndTables))
   {
       int  nSel;
       char xbuff      [MAXOBJECTNAME];
       char xbuffowner [MAXOBJECTNAME];
       
       nSel = ComboBox_GetCurSel (hwndTables);
       if ( nSel != CB_ERR )    {
           ComboBox_GetText (hwndTables, xbuff, sizeof (xbuff));
           x_strcpy (xbuffowner, (char *)ComboBox_GetItemData (hwndTables, nSel));
           StringWithOwner (xbuff, xbuffowner, szBuffer2);
       }
   }

   if (((x_strcmp (szBuffer, OPERATION [5]) == 0) ||
        (x_strcmp (szBuffer, OPERATION [6]) == 0) ||
        (x_strcmp (szBuffer, OPERATION [8]) == 0) ||
        (x_strcmp (szBuffer, OPERATION [9]) == 0) ) && bReportMode == FALSE)
   {
       //"Using this option when you are in any run mode is not supported;\nit can have severe, unexpected results."
       MessageBox(GetFocus(), ResourceString(IDS_ERR_OPTION_RUN_MODE), NULL,
                  MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
       return FALSE;
   }
   if (x_strlen (szBuffer) > 0)
   {
       if (((x_strcmp (szBuffer, OPERATION [4]) == 0) ||
           (x_strcmp (szBuffer, OPERATION [5]) == 0) ||
           (x_strcmp (szBuffer, OPERATION [9]) == 0))
           && (x_strlen (szBuffer2) > 0))
       {
         char szTblOwner[MAXOBJECTNAME];
         OwnerFromString(szBuffer2, szTblOwner);
         if ( IsTheSameOwner(GetCurMdiNodeHandle(),szTblOwner) == FALSE )
           return FALSE;
           //
           // operation = drop_table or table or xtable
           //
           x_strcat (szGreatBuffer, " -o");
           x_strcat (szGreatBuffer, szBuffer);

           x_strcat (szGreatBuffer, " \"");
           x_strcat (szGreatBuffer, (LPTSTR)RemoveDisplayQuotesIfAny((LPTSTR)StringWithoutOwner (szBuffer2)));
           x_strcat (szGreatBuffer, "\"");
       }
       else
       {
           x_strcat (szGreatBuffer, " -o");
           x_strcat (szGreatBuffer, szBuffer);
       }
   }

   if (Button_GetCheck (hwndNologmode))
       x_strcat (szGreatBuffer, " -n");
   else
   if (Button_GetCheck (hwndLogto) && (Edit_GetTextLength (hwndLogfile) > 0))
   {
       ZEROINIT (szBuffer);

       Edit_GetText (hwndLogfile,  szBuffer,  sizeof (szBuffer));
       NoLeadingEndingSpace (szBuffer);
       x_strcat (szGreatBuffer, " -lf");
       x_strcat (szGreatBuffer, szBuffer);
   }

   if (Button_GetCheck (hwndVerbose))
       x_strcat (szGreatBuffer, " -v");

   wsprintf(buftemp,
       ResourceString ((UINT)IDS_T_RMCMD_VERIFDB), //"VerifyDB on node %s",
       vnodeName);
   execrmcmd(vnodeName,szGreatBuffer,buftemp);
   return TRUE;
}

