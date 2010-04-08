/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project  : CA/OpenIngres Visual DBA
**
**    Source   : auditdbt.c
**
**    Specify table for AuditDB dialog
**
** History:
**    06-Mar_2000 (schph01)
**      Bug #100708
**          Add function IsTableNameUnique()
**          updated the new bRefuseTblWithDupName variable in the structure
**          for the new sub-dialog for selecting a table
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
#include "error.h"
#include "dbaginfo.h"
#include "resource.h"

static void OnDestroy                  (HWND hwnd);
static void OnCommand                  (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog               (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange           (HWND hwnd);
static void EnableDisableOKButton      (HWND hwnd);
static BOOL FillStructureFromControls  (HWND hwnd);
static void CheckTables (HWND hwnd);


BOOL CALLBACK __export DlgAuditDBTableDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI __export DlgAuditDBTable (HWND hwndOwner, LPAUDITDBTPARAMS lpauditdbtparams)

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
   
   if (!IsWindow(hwndOwner) || !lpauditdbtparams)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgAuditDBTableDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_AUDITDBT),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpauditdbtparams
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgAuditDBTableDlgProc
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
   LPAUDITDBTPARAMS lpauditdbt = (LPAUDITDBTPARAMS)lParam;
   HWND hwndTables  = GetDlgItem (hwnd, IDC_AUDITDBT_TABLES);

   if (!AllocDlgProp (hwnd, lpauditdbt))
       return FALSE;
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_AUDITDBT));

   //
   // force catolist.dll to load
   //
   CATOListDummy();
   CAListBoxFillTables (hwndTables, lpauditdbt->DBName);
   CheckTables (hwnd);

   EnableDisableOKButton (hwnd);
   richCenterDialog (hwnd);

   return TRUE;
}


static void OnDestroy (HWND hwnd)
{
   HWND hwndTables  = GetDlgItem (hwnd, IDC_AUDITDBT_TABLES);

   CAListBoxDestroyItemData (hwndTables);
   DeallocDlgProp (hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   switch (id) {
      case IDOK:
         if (FillStructureFromControls (hwnd))
             EndDialog (hwnd, TRUE);
         else
             break;
         break;

      case IDCANCEL:
         EndDialog (hwnd, FALSE);
         break;

      case IDC_AUDITDBT_TABLES:
         switch (codeNotify)  {
            case CALBN_CHECKCHANGE:
            {
               LPAUDITDBTPARAMS lpauditdbt;
               int id = CAListBox_GetCurSel(hwndCtl);
               if (id == CALB_ERR ) {
                  myerror(ERR_GW);
                  break;
               }
               if (CAListBox_GetSel(hwndCtl, id)) {  // table checked
                   char mybuf[MAXOBJECTNAME];
                   char tblName[MAXOBJECTNAME];
                   x_strcpy (mybuf, (char *) CAListBox_GetItemData (hwndCtl, id));
                   if ( IsTheSameOwner(GetCurMdiNodeHandle(),mybuf) == FALSE )
                   {
                       CAListBox_SetSel(hwndCtl, FALSE,id);   // unchecks the table 
                       break;
                   }
                   CAListBox_GetText(hwndCtl, id, tblName);
                   /* now verifie that there have only one table with the same name. */
                   lpauditdbt = GetDlgProp (hwnd);
                   if (lpauditdbt->bRefuseTblWithDupName && IsTableNameUnique(GetCurMdiNodeHandle(),lpauditdbt->DBName,tblName) == FALSE)
                   {
                       CAListBox_SetSel(hwndCtl, FALSE,id);   // unchecks the table 
                       break;
                   }
               }
            }
         }
         EnableDisableOKButton (hwnd);
         break;
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
   LPAUDITDBTPARAMS lpauditdbt = GetDlgProp (hwnd);
   HWND hwndTables  = GetDlgItem (hwnd, IDC_AUDITDBT_TABLES);
   FreeObjectList (lpauditdbt->lpTable);
   lpauditdbt->lpTable  = NULL;
   lpauditdbt->lpTable  = AddItemToListWithOwner (hwndTables);
   return TRUE;
}


static void EnableDisableOKButton (HWND hwnd)
{
   /*
   HWND hwndTables  = GetDlgItem (hwnd, IDC_AUDITDBT_TABLES);

   int i = CAListBox_GetSelCount (hwndTables);
   if ( i > 0)
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   */
}



void  CheckTables (HWND hwnd)
{
   char* textStr; 
   HWND  hwndTables  = GetDlgItem (hwnd, IDC_AUDITDBT_TABLES);
   LPAUDITDBTPARAMS lpauditdbt = GetDlgProp (hwnd);


   LPOBJECTLIST ls  = lpauditdbt->lpTable;

   while (ls)
   {
       textStr = (char *)ls->lpObject;
       CAListBoxSelectStringWithOwner (hwndTables, textStr, "NULL");
       ls = ls->lpNext;
   }
}

// 
// This fonction should be remove when the remote command :
//  Rollforward accept the option -table=  with the schema name.
//  ckpdb      idem
//  Optimizedb idem
//  statdump   idem
//  auditdb    idem
//  copydb     idem
// 
// Return TRUE : if the connect Owner if the same with the owner table.
//        FALSE: The owner is different and you cannot launch the remote command.
BOOL IsTheSameOwner(int hNode,char *szTblOwner)
{
    char szMsg[200];
    char szUserName[MAXOBJECTNAME];
    DBAGetUserName(GetVirtNodeName(hNode),szUserName);
    if (lstrcmpi(szTblOwner,szUserName)) {
       wsprintf(szMsg,ResourceString(IDS_ERR_LOG_USER),szTblOwner,szTblOwner);
       MessageBox(GetFocus(),szMsg,NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);
       return FALSE;
    }
    return TRUE;
}

/* 
** This fonction should be remove when the remote command :
**  Rollforward
**  checkpoint
**  ckpdb
** does not apply to all tables that have the same name.
**
**
**
** Return TRUE : if the Table Name is unique.
**        FALSE: if two tables have the same name you cannot launch the remote command.
*/
BOOL IsTableNameUnique(int hNode, char *DbName, char *tblName)
{
   int     ires,nCount;
   char    buf [MAXOBJECTNAME*2];
   char    buffilter [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];
   char *lpTbl;

   parentstrings [0] = DbName;
   parentstrings [1] = NULL;

   lpTbl = StringWithoutOwner(tblName);
   nCount = 0;

   ires = DOMGetFirstObject (hNode, OT_TABLE, 1, parentstrings, FALSE, NULL, buf,
                             buffilter, NULL);
   while (ires == RES_SUCCESS)
   {
      if (x_strcmp(StringWithoutOwner(buf), lpTbl) == 0)
         nCount++;
      ires = DOMGetNextObject (buf, buffilter, NULL);
   }

   if (nCount==1)
      return TRUE;
   else
   {
      MessageBox(GetFocus(),ResourceString(IDS_ERR_TABLE_NAME_NOT_UNIQUE),
                                           NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);
      return FALSE;
   }

}