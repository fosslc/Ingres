/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : chkpt.c
//
//    Generate the checkpoint command string
//
//    table list management cloned from copydb source
//
**  26-Apr-2001 (noifr01)
**    (rmcmd counterpart of SIR 103299) max lenght of rmcmd command lines
**    has been increased to 800 against 2.6. now use a new constant for
**    remote command buffer sizes, because the other "constant", in fact a
**    #define, has been redefined and depends on the result of a function 
**    call (something that doesn't compile if you put it as the size of a char[]
**    array. The new constant, used for buffer sizes only, is the biggest of 
**    the possible values)
**  12-Sep-2002 (schph01)
**     bug 108684 Remove the space between the comma and the next table
**     name in "-table=" parameter, when more than one tables has been
**     selected.
**  09-Mar-2010 (drivi01) SIR 123397
**     Update the dialog to be more user friendly.  Minimize the amount
**     of controls exposed to the user when dialog is initially displayed.
**     Add "Show/Hide Advanced" button to see more/less options in the 
**     dialog.     
********************************************************************/



#include "dll.h"
#include "subedit.h"
#include "rmcmd.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "getdata.h"
#include "domdata.h"
#include "msghandl.h"
#include "dbaginfo.h"

static AUDITDBTPARAMS table;
static BOOL bExpanded;

BOOL CALLBACK __export DlgCheckpointDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy(HWND hwnd);
static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);

static void InitialiseEditControls (HWND hwnd);
static void EnableDisableTableSpecific (HWND hwnd);

static BOOL SendCommand(HWND hwnd);

BOOL WINAPI __export DlgCheckpoint (HWND hwndOwner, LPCHKPT lpchk)
/*
// Function:
//     Shows the Checkpoint dialog.
//
// Paramters:
//     1) hwndOwner: Handle of the parent window for the dialog.
//     2) lpchk:     Points to structure containing information used to 
//                   initialise the dialog. The dialog uses the same
//                   structure to return the result of the dialog.
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

   if (!IsWindow(hwndOwner) || !lpchk)
   {
       ASSERT (NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgCheckpointDlgProc, hInst);
   retVal = VdbaDialogBoxParam (hResource,
        MAKEINTRESOURCE (IDD_CHKPOINT),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpchk
       );

   FreeProcInstance (lpProc);
   // Return always FALSE,no refresh necessary. //PS 20/11/98
   return (FALSE);
}


BOOL CALLBACK __export DlgCheckpointDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
       HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
       HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);

       case WM_INITDIALOG:
           return HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, OnInitDialog);
       default:
           return FALSE;
   }
   return TRUE;
}


static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   LPCHKPT lpchk = (LPCHKPT)lParam;
   char    Title[200];
   UCHAR   UserName[MAXOBJECTNAME];
   RECT	rect;
   HWND hwndEditTable = GetDlgItem(hwnd, IDC_EDIT_TABLE_NAME);
   LPUCHAR vnodeName = GetVirtNodeName (GetCurMdiNodeHandle ());

   if (!AllocDlgProp(hwnd, lpchk))
       return FALSE;

   // initialize table list
   ZEROINIT (table);
   table.lpTable = NULL;
   // Disable Control if lpchk->bStartSinceTable is TRUE.
   if (lpchk->bStartSinceTable)
   {
     Button_SetCheck (GetDlgItem (hwnd, IDC_SPECIFY_TABLE),TRUE);
     EnableWindow    (GetDlgItem (hwnd, IDC_SPECIFY_TABLE),FALSE);
     ShowWindow      (GetDlgItem (hwnd, IDC_TABLES_LIST)  ,SW_HIDE);
     // Display static and edit for table name.
     ShowWindow      (GetDlgItem (hwnd, IDC_STATIC_TABLE_NAME),SW_SHOW);
     ShowWindow      (hwndEditTable,SW_SHOW);
     Edit_SetText    (hwndEditTable,lpchk->szDisplayTableName);
     EnableWindow    (hwndEditTable,FALSE);
     // Available only without -table option in ckpdb command.
     EnableWindow    (GetDlgItem (hwnd, IDC_CHKPOINT_EJOURNALING),FALSE);
     EnableWindow    (GetDlgItem (hwnd, IDC_CHKPOINT_DJOURNALING),FALSE);
     EnableWindow    (GetDlgItem (hwnd, IDC_CHKPOINT_CHGJOURNAL), FALSE);
     EnableWindow    (GetDlgItem (hwnd, IDC_CHKPOINT_DELETEPREV) ,FALSE);
   }
   else
   {
     Button_SetCheck (GetDlgItem (hwnd, IDC_SPECIFY_TABLE),FALSE);
     EnableWindow    (GetDlgItem (hwnd, IDC_SPECIFY_TABLE),TRUE);
     ShowWindow      (GetDlgItem (hwnd, IDC_TABLES_LIST)  ,SW_SHOW);
     EnableWindow    (GetDlgItem (hwnd, IDC_CHKPOINT_EJOURNALING),FALSE);
     EnableWindow    (GetDlgItem (hwnd, IDC_CHKPOINT_DJOURNALING),FALSE);
     EnableWindow    (GetDlgItem (hwnd, IDC_CHKPOINT_CHGJOURNAL), TRUE);
     EnableWindow    (GetDlgItem(hwnd, IDC_CHKPOINT_DELETEPREV),TRUE);
     // Hide static and edit for table name.
     ShowWindow      (GetDlgItem (hwnd, IDC_STATIC_TABLE_NAME)  ,SW_HIDE);
     ShowWindow      (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME)    ,SW_HIDE);
   }

   ZEROINIT (Title);

   DBAGetUserName(vnodeName,UserName);
   x_strcpy(lpchk->User,UserName);

   //Fill the windows title bar
   if (!lpchk->bStartSinceTable)
     GetWindowText(hwnd,Title,GetWindowTextLength(hwnd)+1);
   else
     x_strcat(Title, "Backup on");
   x_strcat(Title, " ");
   x_strcat(Title, vnodeName);
   x_strcat(Title, "::");
   x_strcat(Title,lpchk->DBName);
   if (lpchk->bStartSinceTable) {
     x_strcat(Title," ,table ");
     x_strcat(Title,lpchk->szDisplayTableName);
   }
   SetWindowText(hwnd,Title);

   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_CHKPOINT));
   InitialiseEditControls(hwnd);

   CheckDlgButton(hwnd, IDC_CHKPOINT_DELETEPREV, lpchk->bDelete);
   CheckDlgButton(hwnd, IDC_CHKPOINT_XLOCK, lpchk->bExclusive);
   CheckDlgButton(hwnd, IDC_CHKPOINT_EJOURNALING, lpchk->bEnable);
   CheckDlgButton(hwnd, IDC_CHKPOINT_WAIT, lpchk->bWait);

   EnableDisableTableSpecific(hwnd);

   // #m management
   SubClassEditCntl(hwnd, IDC_PARALLEL_LOCS, CSC_NUMERIC);
   EnableWindow(GetDlgItem(hwnd, IDC_PARALLEL_LOCS), FALSE);
   EnableWindow (GetDlgItem(hwnd, IDC_CHKPOINT_DEVICE), FALSE);

   //resize the window to hide advanced options
   GetWindowRect(hwnd, &rect);
   MoveWindow(hwnd, rect.left, rect.top, rect.right-rect.left, (int)((rect.bottom-rect.top)/2.05), TRUE);
   bExpanded = FALSE;

   richCenterDialog(hwnd);

   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   // #m management
   ResetSubClassEditCntl(hwnd, IDC_PARALLEL_LOCS);

   DeallocDlgProp(hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
  int iret;
  RECT rect;
  LPCHKPT lpchk = GetDlgProp(hwnd);

   switch (id)
   {
     case IDOK:
     {
       char szDevice[MAX_DEVICENAME_LEN + 1];
       HWND hwndDevice = GetDlgItem(hwnd, IDC_CHKPOINT_DEVICE);

       Edit_GetText(hwndDevice, szDevice, sizeof(szDevice));
       if (!VerifyDeviceName(hwnd, szDevice, TRUE))
       {
           Edit_SetSel(hwndDevice, 0, -1);
           SetFocus(hwndDevice);
           break;
       }


       lstrcpy(lpchk->Device, szDevice);

       lpchk->bDelete   = IsDlgButtonChecked(hwnd, IDC_CHKPOINT_DELETEPREV);
       lpchk->bExclusive= IsDlgButtonChecked(hwnd, IDC_CHKPOINT_XLOCK);
       lpchk->bEnable   = IsDlgButtonChecked(hwnd, IDC_CHKPOINT_EJOURNALING);
       lpchk->bDisable  = IsDlgButtonChecked(hwnd, IDC_CHKPOINT_DJOURNALING);
       lpchk->bWait     = IsDlgButtonChecked(hwnd, IDC_CHKPOINT_WAIT);
       lpchk->bVerbose  = IsDlgButtonChecked(hwnd, IDC_CHKPOINT_VERBOSE);

       // #m management
       lpchk->bMulti = IsDlgButtonChecked(hwnd, IDC_PARALLEL_CHECK);
       if (lpchk->bMulti)
         Edit_GetText(GetDlgItem(hwnd, IDC_PARALLEL_LOCS), lpchk->szMulti, sizeof(lpchk->szMulti));
       else
         lpchk->szMulti[0] = 0;

       if (lpchk->bStartSinceTable)
       {
         char szTblOwner[MAXOBJECTNAME];
         OwnerFromString(lpchk->szDisplayTableName, szTblOwner);
         if ( IsTheSameOwner(GetCurMdiNodeHandle(),szTblOwner) == FALSE )
           break;
         if ( IsTableNameUnique(GetCurMdiNodeHandle(),lpchk->DBName,lpchk->szDisplayTableName) == FALSE)
           break;
       }

       if ( SendCommand( hwnd) == TRUE) {
         FreeObjectList (table.lpTable);
         table.lpTable = NULL;
         EndDialog(hwnd, 1);
       }
       else {
         lpchk->lpTable = table.lpTable; // reload anchor for linked list
         SetFocus(hwnd);
       }

       break;
     }
     case IDC_CHKPOINT_TAPE_DEVICE:
	if (IsDlgButtonChecked(hwnd, IDC_CHKPOINT_TAPE_DEVICE) == BST_CHECKED)
	    EnableWindow (GetDlgItem(hwnd, IDC_CHKPOINT_DEVICE), TRUE);
	else
	    EnableWindow (GetDlgItem(hwnd, IDC_CHKPOINT_DEVICE), FALSE);
     case IDC_CHKPOINT_EJOURNALING:
        if (codeNotify == BN_CLICKED)  {
            if (IsDlgButtonChecked(hwnd, IDC_CHKPOINT_EJOURNALING) &&
                IsDlgButtonChecked(hwnd, IDC_CHKPOINT_DJOURNALING))
            {
                CheckDlgButton(hwnd, IDC_CHKPOINT_DJOURNALING, 0); // uncheck
                EnableWindow    (GetDlgItem (hwnd, IDC_SPECIFY_TABLE),FALSE);
            }
            if (IsDlgButtonChecked(hwnd, IDC_CHKPOINT_EJOURNALING) == BST_UNCHECKED &&
                IsDlgButtonChecked(hwnd, IDC_CHKPOINT_DJOURNALING) == BST_UNCHECKED &&
                IsDlgButtonChecked(hwnd, IDC_CHKPOINT_DELETEPREV)  == BST_UNCHECKED )
              EnableWindow    (GetDlgItem (hwnd, IDC_SPECIFY_TABLE),TRUE);
            else
              EnableWindow    (GetDlgItem (hwnd, IDC_SPECIFY_TABLE),FALSE);
        }
        break;
     case IDC_CHKPOINT_DJOURNALING:
        if (codeNotify == BN_CLICKED)  {
            if (IsDlgButtonChecked(hwnd, IDC_CHKPOINT_EJOURNALING) &&
                IsDlgButtonChecked(hwnd, IDC_CHKPOINT_DJOURNALING))
            {
                CheckDlgButton(hwnd, IDC_CHKPOINT_EJOURNALING, 0); // uncheck
                EnableWindow    (GetDlgItem (hwnd, IDC_SPECIFY_TABLE),FALSE);
            }
            if (IsDlgButtonChecked(hwnd, IDC_CHKPOINT_EJOURNALING) == BST_UNCHECKED &&
                IsDlgButtonChecked(hwnd, IDC_CHKPOINT_DJOURNALING) == BST_UNCHECKED  &&
                IsDlgButtonChecked(hwnd, IDC_CHKPOINT_DELETEPREV)  == BST_UNCHECKED )
              EnableWindow    (GetDlgItem (hwnd, IDC_SPECIFY_TABLE),TRUE);
            else
              EnableWindow    (GetDlgItem (hwnd, IDC_SPECIFY_TABLE),FALSE);
        }
        break;
     case IDCANCEL:
       FreeObjectList (table.lpTable);
       table.lpTable = NULL;
       EndDialog(hwnd, 0);
       break;

     case IDC_SPECIFY_TABLE:
       if (IsDlgButtonChecked(hwnd, IDC_SPECIFY_TABLE) == BST_UNCHECKED)
       {
         EnableWindow    (GetDlgItem (hwnd, IDC_CHKPOINT_DELETEPREV) ,TRUE);
		 EnableWindow	 (GetDlgItem (hwnd, IDC_CHKPOINT_CHGJOURNAL), TRUE);
       }
       else
       {
         EnableWindow    (GetDlgItem (hwnd, IDC_CHKPOINT_DELETEPREV) ,FALSE);
		 EnableWindow	 (GetDlgItem (hwnd, IDC_CHKPOINT_CHGJOURNAL), FALSE);
		 Button_SetCheck (GetDlgItem (hwnd, IDC_CHKPOINT_CHGJOURNAL), FALSE);
       }
       EnableDisableTableSpecific(hwnd);
         break;

     case IDC_TABLES_LIST:
       x_strcpy(table.DBName , lpchk->DBName);
       table.bRefuseTblWithDupName = TRUE;
       iret = DlgAuditDBTable (hwnd, &table);
       lpchk->lpTable = table.lpTable;
       if (lpchk->lpTable)
       {
       // Available only without -table option in ckpdb command.
       EnableWindow    (GetDlgItem (hwnd, IDC_CHKPOINT_EJOURNALING),FALSE);
       EnableWindow    (GetDlgItem (hwnd, IDC_CHKPOINT_DJOURNALING),FALSE);
       EnableWindow    (GetDlgItem (hwnd, IDC_CHKPOINT_DELETEPREV) ,FALSE);
       }
       else
       {
       EnableWindow    (GetDlgItem (hwnd, IDC_CHKPOINT_EJOURNALING),TRUE);
       EnableWindow    (GetDlgItem (hwnd, IDC_CHKPOINT_DJOURNALING),TRUE);
       EnableWindow    (GetDlgItem (hwnd, IDC_CHKPOINT_DELETEPREV) ,TRUE);
       }
       break;
     case IDC_CHKPOINT_DELETEPREV:
       if ( IsDlgButtonChecked(hwnd, IDC_CHKPOINT_DELETEPREV) == BST_CHECKED )
       {
         EnableWindow    (GetDlgItem (hwnd, IDC_SPECIFY_TABLE) ,FALSE);
       }
       else
       {
         if (IsDlgButtonChecked(hwnd, IDC_CHKPOINT_EJOURNALING) == BST_CHECKED ||
             IsDlgButtonChecked(hwnd, IDC_CHKPOINT_DJOURNALING) == BST_CHECKED)
           EnableWindow    (GetDlgItem (hwnd, IDC_SPECIFY_TABLE) ,FALSE);
         else
           EnableWindow    (GetDlgItem (hwnd, IDC_SPECIFY_TABLE) ,TRUE);
       }
     // #m management
     case IDC_PARALLEL_CHECK:
       if (codeNotify == BN_CLICKED) {
         BOOL bChecked = IsDlgButtonChecked(hwnd, IDC_PARALLEL_CHECK);
         EnableWindow(GetDlgItem(hwnd, IDC_PARALLEL_LOCS), bChecked);
       }
       break;
     case IDC_SHOW_ADVANCED:  //show/hide advanced options in the dialog
	if (!bExpanded)
	{
		GetWindowRect(hwnd, &rect);
		MoveWindow(hwnd, rect.left, rect.top, rect.right-rect.left, (int)((rect.bottom-rect.top)*2.05), TRUE);
		bExpanded = TRUE;	
		Button_SetText(GetDlgItem(hwnd, IDC_SHOW_ADVANCED), "<< Hide &Advanced");
	}
	else
	{
		GetWindowRect(hwnd, &rect);
		MoveWindow(hwnd, rect.left, rect.top, rect.right-rect.left, (int)((rect.bottom-rect.top)/2.05), TRUE);
		bExpanded = FALSE;
		Button_SetText(GetDlgItem(hwnd, IDC_SHOW_ADVANCED), "Show &Advanced >>");
	}
	break;
     case IDC_CHKPOINT_CHGJOURNAL:
	if (Button_GetCheck(GetDlgItem(hwnd, IDC_CHKPOINT_CHGJOURNAL)) == BST_CHECKED)
	{
		EnableWindow(GetDlgItem(hwnd, IDC_CHKPOINT_EJOURNALING), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_CHKPOINT_DJOURNALING), TRUE);
		Button_SetCheck(GetDlgItem(hwnd, IDC_CHKPOINT_EJOURNALING), TRUE);
             	EnableWindow(GetDlgItem (hwnd, IDC_SPECIFY_TABLE),FALSE);
	}
	else
	{
		EnableWindow(GetDlgItem(hwnd, IDC_CHKPOINT_EJOURNALING), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_CHKPOINT_DJOURNALING), FALSE);
		Button_SetCheck(GetDlgItem(hwnd, IDC_CHKPOINT_EJOURNALING), FALSE);
		Button_SetCheck(GetDlgItem(hwnd, IDC_CHKPOINT_DJOURNALING), FALSE);
             	EnableWindow(GetDlgItem (hwnd, IDC_SPECIFY_TABLE),TRUE);
	}
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
   HWND hwndDevice = GetDlgItem(hwnd, IDC_CHKPOINT_DEVICE);
   LPCHKPT lpchk   = GetDlgProp(hwnd);

   Edit_LimitText (hwndDevice, MAX_DEVICENAME_LEN);
   SetDlgItemText (hwnd, IDC_CHKPOINT_DEVICE, lpchk->Device);

}

static void EnableDisableTableSpecific(HWND hwnd)
{
  if (Button_GetCheck (GetDlgItem (hwnd, IDC_SPECIFY_TABLE)))
    EnableWindow (GetDlgItem (hwnd, IDC_TABLES_LIST),  TRUE);
  else
    EnableWindow (GetDlgItem (hwnd, IDC_TABLES_LIST),  FALSE);
}

static BOOL SendCommand(HWND hwnd)
{
   LPCHKPT lpchk = GetDlgProp(hwnd);
   UCHAR   buf2[MAXOBJECTNAME*3];
   UCHAR buf[MAX_RMCMD_BUFSIZE];
   int ObjectLen, BufLen;

   LPUCHAR   vnodeName=GetVirtNodeName(GetCurMdiNodeHandle ());   
      
   //create the command for remote 
   ZEROINIT (buf);

   x_strcpy(buf,"ckpdb ");
   x_strcat(buf,lpchk->DBName);
   x_strcat(buf," ");

   if (lpchk->User[0]!='\0') {
     x_strcat(buf,"-u");
     x_strcat(buf,lpchk->User);
     x_strcat(buf," ");
   }

   if (lpchk->bDelete)
      x_strcat(buf,"-d ");
   if (lpchk->bExclusive)
     x_strcat(buf,"-l ");

   if (lpchk->bWait) 
     x_strcat(buf,"+w ");

   if (lpchk->bEnable)
     x_strcat(buf,"+j ");

   if (lpchk->bDisable)
     x_strcat(buf,"-j ");

   if (lpchk->Device[0]!='\0') {
     x_strcat(buf,"-m");
     NoLeadingEndingSpace (lpchk->Device);
     x_strcat(buf,lpchk->Device);
     x_strcat(buf," ");
   }
   if (lpchk->bVerbose) {
     x_strcat(buf,"-v");
   }

   // tables list if applicable
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_SPECIFY_TABLE))) {
     if (lpchk->bStartSinceTable)
     {
         char * st = (char *)StringWithoutOwner(lpchk->szDisplayTableName);
         x_strcat(buf, " -table=");
         x_strcat(buf,st);
         x_strcat(buf, " ");
     }
     else
     {
       if (lpchk->lpTable) {
         x_strcat(buf, " -table=");
         while (lpchk->lpTable) {
         ObjectLen = x_strlen(lpchk->lpTable->lpObject );
         BufLen    = x_strlen(buf) ;
         if (ObjectLen >= MAX_LINE_COMMAND-BufLen) {
           ErrorMessage ((UINT) IDS_E_TOO_MANY_TABLES_SELECTED, RES_ERR);
           return FALSE;
         }
         else {
           char * st = (char *)StringWithoutOwner(lpchk->lpTable->lpObject);
           x_strcat(buf,st);
           lpchk->lpTable = lpchk->lpTable->lpNext;
         }
         if (lpchk->lpTable)
           x_strcat(buf, ",");
         else
           x_strcat(buf, " ");
         }
       }
     }
   }

   // #m management
   if (lpchk->bMulti) {
     x_strcat(buf, " #m");
     x_strcat(buf, lpchk->szMulti);
     x_strcat(buf, " ");
   }

   wsprintf(buf2,
       ResourceString ((UINT)IDS_T_RMCMD_CHKPT), //"checkpoints on database %s::%s",
       vnodeName,
       lpchk->DBName);
   // for remote command
   execrmcmd(vnodeName,buf,buf2);

   return (TRUE);

}

