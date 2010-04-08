/*****************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project  : Visual DBA
**
**    Source   : auditdb.c
**
**    Generate the auditdb command string
**
**
**  History after 04-May-1999:
**
**   13-Jan-2000 (schph01)
**     Bug #75260 Remove the schema before the table name, when the SQL
**     statement is generate for the option -table=.
**   14-Jan-2000 (schph01)
**     Bug #99133
**       - generate the default file name when the dialog is launched from
**         table branch.
**   19-Jan-2000 (schph01)
**     Bug #99993 
**       - Add function VerifyBufferLen()
**       - On "OK" button remove the object list only before the EndDialog()
**         function call.
**         If the remote command is not launched because the space buffer for
**         the generation of the remote command is too small, remain in
**         the dialog box without remove the object list.
**       - Reverse order for the files option list, (in lpfile).
**       - Generate the default file name without the schema and the
**         display quotes.
**    06-Mar_2000 (schph01)
**      Bug #100708
**          updated the new bRefuseTblWithDupName variable in the structure
**          for the new sub-dialog for selecting a table
**  26-Apr-2001 (noifr01)
**    (rmcmd counterpart of SIR 103299) max lenght of rmcmd command lines
**    has been increased to 800 against 2.6. now use a new constant for
**    remote command buffer sizes, because the other "constant", in fact a
**    #define, has been redefined and depends on the result of a function 
**    call (something that doesn't compile if you put it as the size of a char[]
**    array. The new constant, used for buffer sizes only, is the biggest of 
**    the possible values)
**  27-Nov-2003 (schph01)
**    (sir 111389) Replace the spin control increasing the checkpoint number by
**    a button displaying the list of existing checkpoints.
**  27-Nov-2003 (schph01)
**    (bug 111414) Generated the #c parameter for auditdb command only if the
**    checkbox is checked and remove test for verify the validity of the
**    checkpoint number(now the edit field for checkpoint number is numeric).
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include "dll.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "getdata.h"
#include "msghandl.h"
#include "domdata.h"
#include "rmcmd.h"
#include "dbaginfo.h"

extern int MfcDlgCheckPointLst(char *szDBname,char *szOwnerName,
                               char *szVnodeName, char *szCurrChkPtNum);

static AUDITDBTPARAMS table;
static LPTABLExFILE   lpfile;

BOOL CALLBACK __export DlgAuditDBDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy (HWND hwnd);
static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange (HWND hwnd);

static void InitializeControls (HWND hwnd);
static void EnableDisableOKButton (HWND hwnd);
static BOOL FillStructureFromControls (HWND hwnd);
static void SetDefaultMessageOption (HWND hwnd);
static void EnableDisableControls (HWND hwnd);

static LPTABLExFILE FindStringInListTableAndFile (LPTABLExFILE lpFile, char* aString);
static LPOBJECTLIST FindStringInListObject (LPOBJECTLIST lpList, char* Element);
static LPTABLExFILE AddElement (LPTABLExFILE lpFile, char* aTable);
static void AddFile (HWND hwnd, LPTABLExFILE lpFile);

static LPTABLExFILE FreeTableAndFile (LPTABLExFILE lpFile);
static LPTABLExFILE RemoveElement (LPTABLExFILE lpFile, char* aTable);
static BOOL Modify (LPTABLExFILE lptablexfile);

int WINAPI __export DlgAuditDB (HWND hwndOwner, LPAUDITDBPARAMS lpauditdb)
/*
// Function:
// Shows the Refresh dialog.
//
// Paramters:
//     1) hwndOwner:   Handle of the parent window for the dialog.
//     1) lpauditdb:   Points to structure containing information used to 
//                     initialise the dialog. 
//
// Returns:
//     TRUE if successful.
//
*/
{
   FARPROC lpProc;
   int     retVal;
   
   if (!IsWindow (hwndOwner) || !lpauditdb)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgAuditDBDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_AUDITDB),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpauditdb
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgAuditDBDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
       HANDLE_MSG (hwnd, WM_COMMAND, OnCommand);
       HANDLE_MSG (hwnd, WM_DESTROY, OnDestroy);

       case WM_INITDIALOG:
           return HANDLE_WM_INITDIALOG (hwnd, wParam, lParam, OnInitDialog);

       default:
           return HandleUserMessages (hwnd, message, wParam, lParam);
   }
   return TRUE;
}


static BOOL OnInitDialog (HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   HWND hwndActionUser = GetDlgItem (hwnd, IDC_AUDITDB_ACTION_USER);
   LPAUDITDBPARAMS lpauditdb  = (LPAUDITDBPARAMS)lParam;
   char szFormat [100];
   char szTitle  [MAX_TITLEBAR_LEN];

   if (!AllocDlgProp (hwnd, lpauditdb))
       return FALSE;

   ZEROINIT (table);
   table.bRefuseTblWithDupName = TRUE;
   table.lpTable = NULL;
   lpfile        = NULL;

   LoadString (hResource, (UINT)IDS_T_AUDITDB, szFormat, sizeof (szFormat));
   wsprintf (szTitle, szFormat,
       GetVirtNodeName ( GetCurMdiNodeHandle ()),
       lpauditdb->DBName);
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_AUDITDB));

   if (lpauditdb->bStartSinceTable)
   {
     LPOBJECTLIST obj;
     char sztemp[MAXOBJECTNAME];
     // Title
     wsprintf( sztemp ," , table %s",lpauditdb->szDisplayTableName);
     lstrcat(szTitle,sztemp);
     // Fill Object List
     obj = AddListObject (table.lpTable, x_strlen (lpauditdb->szCurrentTableName) +1);
     if (obj)
     {
       lstrcpy ((UCHAR *)obj->lpObject,lpauditdb->szCurrentTableName );
       table.lpTable = obj;
     }
     else
     {
       ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
       table.lpTable = NULL;
     }
     lpfile = AddElement (lpfile,lpauditdb->szCurrentTableName );
     AddFile (hwnd, lpfile);

     ShowWindow      (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME)  ,SW_SHOW);
     ShowWindow      (GetDlgItem (hwnd, IDC_STATIC_TABLE_NAME),SW_SHOW);
     ShowWindow      (GetDlgItem (hwnd, IDC_AUDITDB_IDTABLE)  ,SW_HIDE);
     SetWindowText   (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME)  ,lpauditdb->szDisplayTableName );
     EnableWindow    (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME)  ,FALSE );
     Button_SetCheck (GetDlgItem (hwnd, IDC_AUDITDB_TABLES)   ,BST_CHECKED);
     EnableWindow    (GetDlgItem (hwnd, IDC_AUDITDB_TABLES)   ,FALSE );
   }
   SetWindowText (hwnd, szTitle);

   InitializeControls     (hwnd);

   ComboBoxFillUsers (hwndActionUser);
   EnableDisableOKButton (hwnd);
   EnableDisableControls (hwnd);

   richCenterDialog(hwnd);
   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   SubclassAllNumericEditControls (hwnd, EC_RESETSUBCLASS);
   DeallocDlgProp(hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPAUDITDBPARAMS lpauditdb = GetDlgProp (hwnd);
   BOOL Success;

   switch (id)
   {
       case IDOK:
           if (lpauditdb->bStartSinceTable)
           {
             char szTblOwner[MAXOBJECTNAME];
             OwnerFromString(table.lpTable->lpObject, szTblOwner);
             if ( IsTheSameOwner(GetCurMdiNodeHandle(),szTblOwner) == FALSE )
               break;
             if ( table.bRefuseTblWithDupName && IsTableNameUnique(GetCurMdiNodeHandle(),lpauditdb->DBName,table.lpTable->lpObject) == FALSE)
               break;
           }
           Success = FillStructureFromControls (hwnd);

           if (!Success)
               break;
           else
           {
               FreeObjectList (table.lpTable);
               table.lpTable = NULL;
               lpfile = FreeTableAndFile (lpfile);
               EndDialog (hwnd, TRUE);
           }
           break;

       case IDCANCEL:
           FreeObjectList (table.lpTable);
           table.lpTable = NULL;
           lpfile = FreeTableAndFile (lpfile);

           EndDialog (hwnd, FALSE);
           break;

       case IDC_AUDITDB_CKP:
           EnableDisableControls (hwnd);
           break;

       case IDC_AUDITDB_CKP_BUTTON:
           {
               char szCurChkPtNum[MAXOBJECTNAME];
               char szUserName    [MAXOBJECTNAME];
               int ires;
               LPUCHAR vnodeName = GetVirtNodeName (GetCurMdiNodeHandle ());

               DBAGetUserName (GetVirtNodeName ( GetCurMdiNodeHandle ()), szUserName);
               ires = MfcDlgCheckPointLst(lpauditdb->DBName ,szUserName, vnodeName, szCurChkPtNum);
               if (ires != IDCANCEL)
                   Edit_SetText   (GetDlgItem (hwnd, IDC_AUDITDB_CKP_NUMBER), szCurChkPtNum);
               break;
           }
       case IDC_AUDITDB_IDTABLE:
           {
               int   iret;
               char* aString;
               char  szTable [MAXOBJECTNAME];
               LPOBJECTLIST ls;
               LPTABLExFILE lf;

               x_strcpy (table.DBName,  lpauditdb->DBName);
               table.bRefuseTblWithDupName = TRUE;
               iret = DlgAuditDBTable (hwnd, &table);

               ls = table.lpTable;
               EnableDisableControls (hwnd);

               while (ls)
               {
                   aString = (char *)ls->lpObject;
                   if (!FindStringInListTableAndFile (lpfile, aString))
                   {
                       //
                       // Add table  into TABLExFILE
                       //
                       lpfile = AddElement (lpfile, aString);
                   }
                   ls = ls->lpNext;
               }
               AddFile (hwnd, lpfile);

               lf = lpfile;
               while (lf)
               {
                   if (!FindStringInListObject (table.lpTable, lf->TableName))
                   {
                       //
                       // Delete table from TABLExFILE
                       //
                       x_strcpy (szTable, lf->TableName);
                       lf = lf->next;
                       lpfile = RemoveElement (lpfile, szTable);
                   }
                   else lf = lf->next;
               }
           }
           break;

       case IDC_AUDITDB_IDFILE:
           {
               int iret;
               AUDITDBFPARAMS file;
               ZEROINIT (file);

               x_strcpy (file.DBName,  lpauditdb->DBName);
               file.lpTableAndFile = lpfile;
               iret   = DlgAuditDBFile (hwnd, &file);
               if (iret)
                   lpfile = file.lpTableAndFile;
           }
           break;

       case IDC_AUDITDB_TABLES:
           EnableDisableControls (hwnd);
           break;
       case IDC_AUDITDB_FILES:
           EnableDisableControls (hwnd);
           break;
   }
}


static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}


static void InitializeControls (HWND hwnd)
{
   char *szMin = "0";
   HWND hwndCkpNumber = GetDlgItem (hwnd, IDC_AUDITDB_CKP_NUMBER);
   SubclassAllNumericEditControls (hwnd, EC_SUBCLASS);

   Edit_LimitText (hwndCkpNumber, 10);
   Edit_SetText   (hwndCkpNumber, szMin);
   LimitNumericEditControls (hwnd);
}

static void EnableDisableOKButton (HWND hwnd)
{

}

static BOOL VerifyBufferLen(HWND hwnd)
{
   char szSyscat      [22];
   char szBefore      [22];
   char szAfter       [22];
   char szCn          [10];
   char szActUser     [MAXOBJECTNAME];
   char szUserName    [MAXOBJECTNAME];
   int nTotalLen = 0;
   HWND hwndSyscat         = GetDlgItem (hwnd, IDC_AUDITDB_SYSCAT);
   HWND hwndBefore         = GetDlgItem (hwnd, IDC_AUDITDB_BEFORE);
   HWND hwndAfter          = GetDlgItem (hwnd, IDC_AUDITDB_AFTER);
   HWND hwndCn             = GetDlgItem (hwnd, IDC_AUDITDB_CKP_NUMBER);
   HWND hwndActUser        = GetDlgItem (hwnd, IDC_AUDITDB_ACTION_USER);
   HWND hwndWait           = GetDlgItem (hwnd, IDC_AUDITDB_WAIT);
   HWND hwndInconsistent   = GetDlgItem (hwnd, IDC_AUDITDB_INCONSISTENT);
   HWND hwndTable          = GetDlgItem (hwnd, IDC_AUDITDB_TABLES);
   HWND hwndFile           = GetDlgItem (hwnd, IDC_AUDITDB_FILES);

   LPAUDITDBPARAMS     lpauditdb  = GetDlgProp (hwnd);
   LPTABLExFILE ls   = lpfile;

   ZEROINIT (szSyscat);
   ZEROINIT (szBefore);
   ZEROINIT (szAfter);
   ZEROINIT (szCn);
   ZEROINIT (szActUser);

   if (Button_GetCheck (hwndFile) && !ls)
       nTotalLen = x_strlen (" -file");

   if (Button_GetCheck (hwndTable))
   {
       char*  aTable;
       LPOBJECTLIST list = table.lpTable;

       if (list)
       {
           aTable = (LPTSTR)RemoveDisplayQuotesIfAny((LPTSTR)StringWithoutOwner(list->lpObject));
           nTotalLen += x_strlen (" -table=");
           nTotalLen += x_strlen (aTable);
           list = list->lpNext;
       }
       while (list)
       {
           nTotalLen += x_strlen (",");
           aTable = (LPTSTR)RemoveDisplayQuotesIfAny((LPTSTR)StringWithoutOwner(list->lpObject));
           nTotalLen += x_strlen ( aTable);
           list = list->lpNext;
       }
   }

   if (Button_GetCheck (hwndFile) && ls && Modify (ls))
   {
       if (ls)
       {
           nTotalLen += x_strlen (" -file=");
           nTotalLen += x_strlen (ls->FileName);
           ls = ls->next;
       }

       while (ls)
       {
           nTotalLen += x_strlen(",");
           nTotalLen += x_strlen (ls->FileName);
           ls = ls->next;
       }
   }

   if (Button_GetCheck (hwndSyscat))
       nTotalLen += x_strlen ("-a");

   nTotalLen += x_strlen ("auditdb    ");
   nTotalLen += x_strlen (lpauditdb->DBName);

   Edit_GetText (hwndAfter,  szAfter,  sizeof (szAfter ));
   Edit_GetText (hwndBefore, szBefore, sizeof (szBefore));
   Edit_GetText (hwndCn,     szCn,     sizeof (szCn));

   ComboBox_GetText (hwndActUser, szActUser, sizeof (szActUser));

   if (x_strlen (szAfter) > 0)
   {
       nTotalLen += x_strlen (" -b");
       nTotalLen += x_strlen (szAfter);
   }

   if (x_strlen (szBefore) > 0)
   {
       nTotalLen += x_strlen (" -e");
       nTotalLen += x_strlen (szBefore);
   }

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_AUDITDB_CKP)))
   {
       nTotalLen += x_strlen (" #c");
       if (x_strlen (szCn) > 0)
           nTotalLen += x_strlen (szCn);
   }

   if (x_strlen (szActUser) > 0)
   {
       nTotalLen += x_strlen (" -i");
       nTotalLen += x_strlen (szActUser);
   }

   if (Button_GetCheck (hwndWait))
       nTotalLen += x_strlen (" -wait");

   if (Button_GetCheck (hwndInconsistent))
       nTotalLen += x_strlen (" -inconsistent");
  
   ZEROINIT (szUserName);
   DBAGetUserName (GetVirtNodeName ( GetCurMdiNodeHandle ()), szUserName);
   nTotalLen += x_strlen (" -u");
   nTotalLen += x_strlen (szUserName);

   if (nTotalLen<MAX_LINE_COMMAND)
       return TRUE;
   else
       return FALSE;
}


static BOOL FillStructureFromControls (HWND hwnd)
{
   char szGreatBuffer [MAX_RMCMD_BUFSIZE];
   char szBufferT     [MAX_RMCMD_BUFSIZE];
   char szBufferF     [MAX_RMCMD_BUFSIZE];
   char szSyscat      [22];
   char szBefore      [22];
   char szAfter       [22];
   char szCn          [10];
   char szActUser     [MAXOBJECTNAME];
   char szUserName    [MAXOBJECTNAME];
   char buftemp[200];

   HWND hwndSyscat         = GetDlgItem (hwnd, IDC_AUDITDB_SYSCAT);
   HWND hwndBefore         = GetDlgItem (hwnd, IDC_AUDITDB_BEFORE);
   HWND hwndAfter          = GetDlgItem (hwnd, IDC_AUDITDB_AFTER);
   HWND hwndCn             = GetDlgItem (hwnd, IDC_AUDITDB_CKP_NUMBER);
   HWND hwndActUser        = GetDlgItem (hwnd, IDC_AUDITDB_ACTION_USER);
   HWND hwndWait           = GetDlgItem (hwnd, IDC_AUDITDB_WAIT);
   HWND hwndInconsistent   = GetDlgItem (hwnd, IDC_AUDITDB_INCONSISTENT);
   HWND hwndTable          = GetDlgItem (hwnd, IDC_AUDITDB_TABLES);
   HWND hwndFile           = GetDlgItem (hwnd, IDC_AUDITDB_FILES);

   LPUCHAR vnodeName = GetVirtNodeName (GetCurMdiNodeHandle ());
   LPAUDITDBPARAMS     lpauditdb  = GetDlgProp (hwnd);
   LPTABLExFILE ls   = lpfile;

   ZEROINIT (szGreatBuffer);
   ZEROINIT (szBufferT);
   ZEROINIT (szBufferF);
   ZEROINIT (szSyscat);
   ZEROINIT (szBefore);
   ZEROINIT (szAfter);
   ZEROINIT (szCn);
   ZEROINIT (szActUser);

   // Verify if the length of the remote command do not exceed MAX_LINE_COMMAND.
   if (!VerifyBufferLen(hwnd))
   {
       ErrorMessage ((UINT) IDS_E_TOO_MANY_TABLES_SELECTED, RES_ERR);
       return FALSE;
   }

   if (Button_GetCheck (hwndFile))
       x_strcpy (szBufferF, " -file");

   if (Button_GetCheck (hwndTable))
   {
       char*  aTable;
       LPOBJECTLIST list = table.lpTable;

       if (list)
       {
           aTable = (LPTSTR)RemoveDisplayQuotesIfAny((LPTSTR)StringWithoutOwner(list->lpObject));
           x_strcpy (szBufferT, " -table=");
           x_strcat (szBufferT, aTable);
           list = list->lpNext;
       }
       while (list)
       {
           x_strcat (szBufferT, ",");
           aTable = (LPTSTR)RemoveDisplayQuotesIfAny((LPTSTR)StringWithoutOwner(list->lpObject));
           x_strcat (szBufferT, aTable);
           list = list->lpNext;
       }
   }

   if (Button_GetCheck (hwndFile) && ls && Modify (ls))
   {
       x_strcpy (szBufferF, " -file=");
       x_strcat (szBufferF, ls->FileName);
       ls = ls->next;

       while (ls)
       {
           x_strcat (szBufferF, ",");
           x_strcat (szBufferF, ls->FileName);
           ls = ls->next;
       }
   }

   if (Button_GetCheck (hwndSyscat))
       x_strcpy (szSyscat, "-a");
   else
       x_strcpy (szSyscat, "");

   wsprintf (szGreatBuffer,
       "auditdb %s %s %s %s",
       szSyscat,
       lpauditdb->DBName,
       szBufferT,
       szBufferF);
 
   Edit_GetText (hwndAfter,  szAfter,  sizeof (szAfter ));
   Edit_GetText (hwndBefore, szBefore, sizeof (szBefore));
   Edit_GetText (hwndCn,     szCn,     sizeof (szCn));

   ComboBox_GetText (hwndActUser, szActUser, sizeof (szActUser));

   if (x_strlen (szAfter) > 0)
   {
       x_strcat (szGreatBuffer, " -b");
       x_strcat (szGreatBuffer, szAfter);
   }

   if (x_strlen (szBefore) > 0)
   {
       x_strcat (szGreatBuffer, " -e");
       x_strcat (szGreatBuffer, szBefore);
   }

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_AUDITDB_CKP)))
   {
       x_strcat (szGreatBuffer, " #c");
       if (x_strlen (szCn) > 0)
           x_strcat (szGreatBuffer, szCn);
   }

   if (x_strlen (szActUser) > 0)
   {
       x_strcat (szGreatBuffer, " -i");
       x_strcat (szGreatBuffer, szActUser);
   }

   if (Button_GetCheck (hwndWait))
       x_strcat (szGreatBuffer, " -wait");

   if (Button_GetCheck (hwndInconsistent))
       x_strcat (szGreatBuffer, " -inconsistent");
  
   ZEROINIT (szUserName);
   DBAGetUserName (vnodeName, szUserName);
   x_strcat (szGreatBuffer, " -u");
   x_strcat (szGreatBuffer, szUserName);


   wsprintf(buftemp,
       ResourceString ((UINT)IDS_T_RMCMD_AUDITDB), //"auditing database %s::%s",
       vnodeName,
       lpauditdb->DBName);
   execrmcmd(vnodeName,szGreatBuffer,buftemp);
   return TRUE;
}

static LPTABLExFILE AddElement (LPTABLExFILE lpFile, char* aTable)
{
   LPTABLExFILE obj = (LPTABLExFILE) ESL_AllocMem (sizeof (TABLExFILE));
   if (!obj)
   {
       lpFile = FreeTableAndFile (lpFile);
       ErrorMessage ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
       return (lpFile);
   }
   else
   {
       LPTABLExFILE plistbegin = lpFile;
       x_strcpy (obj->TableName, aTable);  // Prefixed by owner
       obj->next = NULL;

       // find the last object in the list
       while (lpFile)
       {
           if (!lpFile->next)
               break;
           lpFile=lpFile->next;
       }

       if (lpFile)
           lpFile->next = obj;
       else
       {
           lpFile = obj; // At the present time no element in the list.
           plistbegin = lpFile;
       }

       return (plistbegin);
   }
}


static LPTABLExFILE FreeTableAndFile (LPTABLExFILE lpFile)
{
   LPTABLExFILE ls = lpFile, tmp;

   while (ls)
   {
       tmp = ls;
       ls  = ls->next;
       ESL_FreeMem (tmp);
   }
   return (ls);
}


static LPTABLExFILE
RemoveElement (LPTABLExFILE lpFile, char* aTable)
{
   LPTABLExFILE tm, prec;
   LPTABLExFILE ls = lpFile;

   prec = ls;
   while (ls)
   {
       if (x_strcmp (ls->TableName, aTable) == 0)
       {
           tm = ls;
           if (ls == lpFile)
           {
               ls = ls->next;
               ESL_FreeMem (tm);
               tm = NULL;
               return (ls);
           }
           else
           {
               prec->next = ls->next;
               ESL_FreeMem (tm);
               tm = NULL;
               return (lpFile);
           }
       }
       prec = ls;
       ls = ls->next;
   }
   return lpFile;
}

static LPOBJECTLIST
FindStringInListObject (LPOBJECTLIST lpList, char* Element)
{
   LPOBJECTLIST ls = lpList;
   char* aString;

   while (ls)
   {
       aString = (char *) ls->lpObject;

       if (x_strcmp (aString, Element) == 0)
           return(ls);
       ls = ls->lpNext;
   }
   return NULL;
}

static LPTABLExFILE
FindStringInListTableAndFile (LPTABLExFILE lpFile, char* aString)
{
   LPTABLExFILE ls = lpFile;


   while (ls)
   {
       if (x_strcmp (ls->TableName, aString) == 0)
           return (ls);

       ls = ls->next;
   }
   return NULL;
}

static BOOL Modify (LPTABLExFILE lptablexfile)
{
   LPTABLExFILE ls = lptablexfile;

   char* table;
   char  file [2*MAXOBJECTNAME];

   while (ls)
   {
       table = ls->TableName;

       wsprintf (file, "%s.trl", table);
       if (x_strcmp (file, ls->FileName) != 0)
           return TRUE;
       ls = ls->next;
   }
   return FALSE;
}


static void EnableDisableControls (HWND hwnd)
{
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_AUDITDB_TABLES)))
       EnableWindow (GetDlgItem (hwnd, IDC_AUDITDB_IDTABLE),  TRUE);
   else
   {
       EnableWindow (GetDlgItem (hwnd, IDC_AUDITDB_IDTABLE),  FALSE);
       FreeObjectList (table.lpTable);
       table.lpTable = NULL;
       lpfile = FreeTableAndFile (lpfile);
   }

   if (table.lpTable)
   {
      EnableWindow (GetDlgItem (hwnd, IDC_AUDITDB_FILES),  TRUE);
      if (Button_GetCheck (GetDlgItem (hwnd, IDC_AUDITDB_FILES)))
         EnableWindow (GetDlgItem (hwnd, IDC_AUDITDB_IDFILE), TRUE);
      else
         EnableWindow (GetDlgItem (hwnd, IDC_AUDITDB_IDFILE), FALSE);
   }
   else
   {
       Button_SetCheck (GetDlgItem (hwnd, IDC_AUDITDB_FILES), FALSE);
       EnableWindow (GetDlgItem (hwnd, IDC_AUDITDB_FILES),  FALSE);
       EnableWindow (GetDlgItem (hwnd, IDC_AUDITDB_IDFILE), FALSE);
   }

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_AUDITDB_CKP)))   {
       EnableWindow (GetDlgItem (hwnd, IDC_AUDITDB_CKP_BUTTON),  TRUE);
       EnableWindow (GetDlgItem (hwnd, IDC_AUDITDB_CKP_NUMBER),  TRUE);
   }
   else  {
       EnableWindow (GetDlgItem (hwnd, IDC_AUDITDB_CKP_BUTTON),  FALSE);
       EnableWindow (GetDlgItem (hwnd, IDC_AUDITDB_CKP_NUMBER),  FALSE);
   }

}

static void AddFile (HWND hwnd, LPTABLExFILE lpFile)
{
   int     hdl, ires;
   BOOL    bwsystem;
   char    buf       [MAXOBJECTNAME];
   char    buf2      [MAXOBJECTNAME];
   char    szOwner   [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];
   LPAUDITDBPARAMS lpauditdb = GetDlgProp (hwnd);
   HWND hwndTables  = GetDlgItem (hwnd, IDC_AUDITDBF_TABLE);
   LPTABLExFILE
       ls  = lpFile,
       obj;
   char    szFileName[MAXOBJECTNAME];

   ZEROINIT (buf);
   parentstrings [0] = lpauditdb->DBName;
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
       obj = FindStringInListTableAndFile (ls, buf2);
       if (obj)
       {
           wsprintf (szFileName,   "%s.trl", RemoveDisplayQuotesIfAny((LPTSTR)StringWithoutOwner(buf)));
           x_strcpy (obj->FileName,  szFileName);
       }

       ires    = DOMGetNextObject (buf, szOwner, NULL);
   }
}

