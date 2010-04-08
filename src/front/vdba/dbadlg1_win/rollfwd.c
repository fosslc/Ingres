/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project  : Visual DBA
**
**    Source   : rollfwd.c
**
**    Generate the roll forward DB command string 
**
**    table list management cloned from copydb source
**
** History:
**      Bug #100708
**          updated the new bRefuseTblWithDupName variable in the structure
**          for the new sub-dialog for selecting a table
**      02-Aug-2000 (schph01)
**          bug 102239 Remove the management of IDC_ROLLFWD_NOBLOB checkbox,
**          because this functionality is no more supported.
**      12-Sep-2002 (schph01)
**          bug 108684 Remove the space between the comma and the next table
**          name in "-table=" parameter, when more than one tables has been
**          selected.
**      27-Nov-2003 (schph01)
**          (sir 111389) Replace the spin control increasing the checkpoint number by
**          a button displaying the list of existing checkpoints.
**	    09-Mar-2010 (drivi01) SIR 123397
**          Update the dialog to be more user friendly.  Minimize the amount
**          of controls exposed to the user when dialog is initially displayed.
**          Add "Show/Hide Advanced" button to see more/less options in the 
**          dialog. Added dmf_cache control to allow to set -dmf_cache flag
**          for rollforwarddb.
********************************************************************/

#include <stdlib.h>
#include "dll.h"                                  
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "getdata.h"
#include "msghandl.h"
#include "domdata.h"
#include "esltools.h"
#include "rmcmd.h"
#include "msghandl.h"
#include "dbaginfo.h"
#include "resource.h"
#include "domdisp.h" //To Refresh the Tree

extern int MfcDlgCheckPointLst(char *szDBname,char *szOwnerName,
                               char *szVnodeName, char *szCurrChkPtNum);

static AUDITDBTPARAMS table;
static BOOL bExpanded;

BOOL CALLBACK __export DlgRollForwardDlgProc
                   (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy                  (HWND hwnd);
static void OnCommand                  (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog               (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange           (HWND hwnd);
static void SetDefaultOptions          (HWND hwnd);
static void InitialiseEditControls     (HWND hwnd);
static BOOL FillStructureFromControls  (HWND hwnd);
static void EnableDisableTableSpecific (HWND hwnd);
static void EnableDisableRelocateSpecific (HWND hwnd);
static void EnableDisableCheckpointControls (HWND hwnd);

#define CACHE_SIZES 6
#define BUF_NUM 256
#define PCHK_DEF 2
static char* cache_size[][2] = { 
	{"2", " -dmf_cache_size="},
	{"4", " -dmf_cache_size_4k="},
	{"8", " -dmf_cache_size_8k="},
	{"16", " -dmf_cache_size_16k="},
	{"32", " -dmf_cache_size_32k="},
	{"64", " -dmf_cache_size_64k="},
	{'\0', '\0'}
};

BOOL WINAPI __export DlgRollForward (HWND hwndOwner, LPROLLFWDPARAMS lprollfwd)
/*
// Function:
//     Shows the Checkpoint dialog.
//
// Paramters:
//     1) hwndOwner: Handle of the parent window for the dialog.
//     2) lprollfwd:     Points to structure containing information used to 
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

   if (!IsWindow(hwndOwner) || !lprollfwd)
   {
       ASSERT (NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgRollForwardDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_ROLLFWD),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lprollfwd
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgRollForwardDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
   char szFormat [100];
   char szTitle  [MAX_TITLEBAR_LEN];
   RECT rect;
   HWND cHwnd;
   int i=0;
   LPROLLFWDPARAMS lprollfwd = (LPROLLFWDPARAMS)lParam;

   if (!AllocDlgProp(hwnd, lprollfwd))
       return FALSE;

   // initialize table list
   ZEROINIT (table);
   table.bRefuseTblWithDupName = TRUE;
   table.lpTable = NULL;

   LoadString (hResource, (UINT)IDS_T_ROLLFORWARD, szFormat, sizeof (szFormat));
   wsprintf (szTitle, szFormat,
       GetVirtNodeName ( lprollfwd->node ),
       lprollfwd->DBName);

   if (lprollfwd->bStartSinceTable)
   {
     LPOBJECTLIST obj;
     char sztemp[MAXOBJECTNAME];
     // Title
     wsprintf( sztemp ," , table %s",lprollfwd->szDisplayTableName);
     lstrcat(szTitle,sztemp);
     // Fill Object List
     obj = AddListObject (table.lpTable, x_strlen (lprollfwd->szDisplayTableName) +1);
     if (obj)
     {
       lstrcpy ((UCHAR *)obj->lpObject,lprollfwd->szDisplayTableName );
       table.lpTable = obj;
       lprollfwd->lpTable = table.lpTable;
     }
     else
     {
       ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
       lprollfwd->lpTable = NULL;
     }
     // Enable Button
     Button_SetCheck (GetDlgItem (hwnd, IDC_SPECIFY_TABLE)    ,BST_CHECKED);
     EnableWindow    (GetDlgItem (hwnd, IDC_SPECIFY_TABLE)    ,FALSE);
     ShowWindow      (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME)  ,SW_SHOW);
     ShowWindow      (GetDlgItem (hwnd, IDC_STATIC_TABLE_NAME),SW_SHOW);
     ShowWindow      (GetDlgItem (hwnd, IDC_TABLES_LIST)      ,SW_HIDE);
     SetWindowText   (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME)  ,lprollfwd->szDisplayTableName );
   }
   SetWindowText (hwnd, szTitle);
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_ROLLFWD));

   InitialiseEditControls (hwnd);

   EnableDisableTableSpecific(hwnd);
   EnableDisableRelocateSpecific(hwnd);
   EnableDisableCheckpointControls(hwnd);
   //
   // Set the default options
   //
   SetDefaultOptions (hwnd);
   richCenterDialog(hwnd);

   // #m management
   SubClassEditCntl(hwnd, IDC_PARALLEL_RFW_LOCS, CSC_NUMERIC);
   EnableWindow(GetDlgItem(hwnd, IDC_PARALLEL_RFW_LOCS), FALSE);

   //Fill the cache size

   cHwnd = GetDlgItem(hwnd,IDC_COMBO_ROLLFWD_CACHE);
   for (i=0; cache_size[i][0] != 0; i++)
	   ComboBox_AddString(cHwnd, cache_size[i][0]);
	ComboBox_SelectString  (cHwnd, -1, cache_size [0][0]);
	SubClassEditCntl(hwnd, IDC_EDIT_ROLLFWD_BUF, CSC_NUMERIC);
	sprintf(szFormat, "%d", BUF_NUM);
	Edit_SetText(GetDlgItem(hwnd, IDC_EDIT_ROLLFWD_BUF), (LPCSTR)szFormat);


   //resize the window to hide advanced options
   GetWindowRect(hwnd, &rect);
   MoveWindow(hwnd, rect.left, rect.top, rect.right-rect.left, (int)((rect.bottom-rect.top)/2.5), TRUE);
   bExpanded = FALSE;


   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   // #m management
   ResetSubClassEditCntl(hwnd, IDC_PARALLEL_RFW_LOCS);

   ResetSubClassEditCntl(hwnd, IDC_CHKPOINT_DEVICE);
   ResetSubClassEditCntl(hwnd, IDC_USER);
   DeallocDlgProp(hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
  int iret;
  RECT rect;
   LPROLLFWDPARAMS lprollfwd = GetDlgProp(hwnd);

   switch (id)
   {
       case IDOK:
       {
           if (lprollfwd->bStartSinceTable)
           {
             char szTblOwner[MAXOBJECTNAME];
             OwnerFromString(lprollfwd->lpTable->lpObject, szTblOwner);
             if ( IsTheSameOwner(GetCurMdiNodeHandle(),szTblOwner) == FALSE )
               break;
             if ( IsTableNameUnique(GetCurMdiNodeHandle(),lprollfwd->DBName,lprollfwd->lpTable->lpObject) == FALSE)
               break;
           }
			{
				BOOL busecheckpoint  = Button_GetCheck (GetDlgItem (hwnd, IDC_ROLLFWD_LASTCKP));
				BOOL bFromCheckpoint = Button_GetCheck (GetDlgItem (hwnd, IDC_ROLLFWD_SPECIFIEDCKP));
				if (!busecheckpoint && bFromCheckpoint) {
					//"The 'Using Checkpoint' option is required when using 'From Specified Checkpoint'"
					MessageBox (hwnd, ResourceString(IDS_ERR_USING_CHECKPOINT), NULL, MB_ICONSTOP|MB_OK);
					break;
				}
			}
           FillStructureFromControls (hwnd);
           FreeObjectList (table.lpTable);
           table.lpTable = NULL;
           StringList_Done (lprollfwd->lpOldLocations);
           StringList_Done (lprollfwd->lpNewLocations);
           EndDialog(hwnd, TRUE);
           break;
       }

       case IDCANCEL:
           FreeObjectList (table.lpTable);
           table.lpTable = NULL;
           StringList_Done (lprollfwd->lpOldLocations);
           StringList_Done (lprollfwd->lpNewLocations);
           EndDialog(hwnd, FALSE);
           break;

       case IDC_ROLLFWD_SPECIFIEDCKP:
           EnableDisableCheckpointControls(hwnd);
		   if (Button_GetCheck(GetDlgItem(hwnd, IDC_ROLLFWD_SPECIFIEDCKP)))
					Button_SetCheck(GetDlgItem(hwnd, IDC_ROLLFWD_LASTCKP), TRUE);
           break;

       case IDC_ROLLFWD_LASTCKP_BUTTON:
           {
               char szCurChkPtNum[MAXOBJECTNAME];
               char szUserName    [MAXOBJECTNAME];
               int ires;
               LPUCHAR vnodeName = GetVirtNodeName (GetCurMdiNodeHandle ());

               DBAGetUserName (GetVirtNodeName ( GetCurMdiNodeHandle ()), szUserName);
               ires = MfcDlgCheckPointLst(lprollfwd->DBName ,szUserName, vnodeName, szCurChkPtNum);
               if (ires != IDCANCEL)
                   Edit_SetText   (GetDlgItem (hwnd,IDC_ROLLFWD_LASTCKP_NUM ), szCurChkPtNum);
           }
           break;
	   case IDC_ROLLFWD_LASTCKP:
		   if (!Button_GetCheck(GetDlgItem(hwnd, IDC_ROLLFWD_LASTCKP)))
		   {
				Button_SetCheck(GetDlgItem(hwnd, IDC_ROLLFWD_SPECIFIEDCKP), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_ROLLFWD_SPECIFIEDCKP), FALSE);
		   }
		   else
				EnableWindow(GetDlgItem(hwnd, IDC_ROLLFWD_SPECIFIEDCKP), TRUE);
		   break;
     case IDC_SPECIFY_TABLE:
       EnableDisableTableSpecific(hwnd);
       EnableDisableRelocateSpecific(hwnd);
       break;

     case IDC_ROLLFWD_RELOCATE:
       EnableDisableRelocateSpecific(hwnd);
       break;

     case IDC_TABLES_LIST:
       x_strcpy(table.DBName , lprollfwd->DBName);
       table.bRefuseTblWithDupName = TRUE;
       iret = DlgAuditDBTable (hwnd, &table);
       lprollfwd->lpTable = table.lpTable;
       break;

     case IDC_ROLLFWD_LOCATIONS:
       DlgRollForwardRelocate (hwnd, lprollfwd);
       break;

      // #m management
     case IDC_PARALLEL_RFW:
       if (codeNotify == BN_CLICKED) {
	     char pchk[MAX_PATH];
         BOOL bChecked = IsDlgButtonChecked(hwnd, IDC_PARALLEL_RFW);
         EnableWindow(GetDlgItem(hwnd, IDC_PARALLEL_RFW_LOCS), bChecked);
		 if (bChecked)
			 sprintf(pchk, "%d", PCHK_DEF);
		 else
			 *pchk='\0';
		 Edit_SetText(GetDlgItem(hwnd, IDC_PARALLEL_RFW_LOCS), pchk);
	   }
       break;
     case IDC_SHOW_ADVANCED:
		 if (!bExpanded)
		 {
			GetWindowRect(hwnd, &rect);
   			MoveWindow(hwnd, rect.left, rect.top, rect.right-rect.left, (int)((rect.bottom-rect.top)*2.5), TRUE);
			SetWindowText(GetDlgItem(hwnd, IDC_SHOW_ADVANCED), "<< Hide &Advanced");
            bExpanded = TRUE;
		 }
		 else
		 {
			//resize the window to hide advanced options
			GetWindowRect(hwnd, &rect);
			MoveWindow(hwnd, rect.left, rect.top, rect.right-rect.left, (int)((rect.bottom-rect.top)/2.5), TRUE);
     		SetWindowText(GetDlgItem(hwnd, IDC_SHOW_ADVANCED), "Show &Advanced >>");
			bExpanded = FALSE;
		 }
		 break;
	 case IDC_ROLLFWD_TAPE:
		 if (Button_GetCheck(GetDlgItem(hwnd, IDC_ROLLFWD_TAPE)))
			 EnableWindow(GetDlgItem(hwnd, IDC_ROLLFWD_DEVICE), TRUE);
		 else
			 EnableWindow(GetDlgItem(hwnd, IDC_ROLLFWD_DEVICE), FALSE);
		 break;
	 case IDC_CHECK_ROLLFWD_AFTER:
		 if (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK_ROLLFWD_AFTER)))
			 EnableWindow(GetDlgItem(hwnd, IDC_ROLLFWD_AFTER), TRUE);
		 else
			 EnableWindow(GetDlgItem(hwnd, IDC_ROLLFWD_AFTER), FALSE);
		 break;
	 case IDC_CHECK_ROLLFWD_BEFORE:
		 if (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK_ROLLFWD_BEFORE)))
			 EnableWindow(GetDlgItem(hwnd, IDC_ROLLFWD_BEFORE), TRUE);
		 else
			 EnableWindow(GetDlgItem(hwnd, IDC_ROLLFWD_BEFORE), FALSE);
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
   HWND hwndLastCkp = GetDlgItem (hwnd, IDC_ROLLFWD_LASTCKP_NUM);
   HWND hwndDevice  = GetDlgItem(hwnd, IDC_ROLLFWD_DEVICE);
   HWND hwndAfter   = GetDlgItem(hwnd, IDC_ROLLFWD_AFTER);
   HWND hwndBefore  = GetDlgItem(hwnd, IDC_ROLLFWD_BEFORE);
   LPROLLFWDPARAMS  lprollfwd = GetDlgProp(hwnd);

   SubclassAllNumericEditControls (hwnd, EC_SUBCLASS);

   Edit_LimitText (hwndDevice,  MAX_DEVICENAME_LEN);
   Edit_LimitText (hwndAfter,   22);
   Edit_LimitText (hwndBefore,  22);
   Edit_LimitText (hwndLastCkp, 10);
   
   Edit_SetText   (hwndLastCkp, "0");
   LimitNumericEditControls (hwnd);
}

static void SetDefaultOptions (HWND hwnd)
{
   Button_SetCheck (GetDlgItem (hwnd, IDC_ROLLFWD_LASTCKP),    TRUE);
   Button_SetCheck (GetDlgItem (hwnd, IDC_ROLLFWD_JOURNALING), TRUE);
   Button_SetCheck (GetDlgItem (hwnd, IDC_ROLLFWD_LASTCKP), TRUE);
   Button_SetCheck (GetDlgItem (hwnd, IDC_ROLLFWD_WAIT), TRUE);
}


static void EnableDisableTableSpecific(HWND hwnd)
{
  BOOL bEnable;
  LPROLLFWDPARAMS  lprollfwd  = GetDlgProp (hwnd);
  if (Button_GetCheck (GetDlgItem (hwnd, IDC_SPECIFY_TABLE)))
    bEnable = TRUE;
  else
    bEnable = FALSE;
  if (lprollfwd->bStartSinceTable)
    EnableWindow (GetDlgItem (hwnd, IDC_TABLES_LIST),             FALSE);
  else
    EnableWindow (GetDlgItem (hwnd, IDC_TABLES_LIST),             bEnable);
  EnableWindow (GetDlgItem (hwnd, IDC_ROLLFWD_ONERRORCONTINUE),   bEnable);
  EnableWindow (GetDlgItem (hwnd, IDC_ROLLFWD_NOSECONDARY_INDEX), bEnable);
  EnableWindow (GetDlgItem (hwnd, IDC_ROLLFWD_RELOCATE),          bEnable);
  EnableWindow (GetDlgItem (hwnd, IDC_ROLLFWD_LOCATIONS),         bEnable);
}

static void EnableDisableRelocateSpecific (HWND hwnd)
{
  BOOL bEnable;
  if (Button_GetCheck (GetDlgItem (hwnd, IDC_ROLLFWD_RELOCATE)))
    bEnable = TRUE;
  else
    bEnable = FALSE;
  EnableWindow (GetDlgItem (hwnd, IDC_ROLLFWD_LOCATIONS), bEnable);
}

static void EnableDisableCheckpointControls (HWND hwnd)
{
   BOOL bEnable;
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_ROLLFWD_SPECIFIEDCKP)))
       bEnable = TRUE;
   else
       bEnable = FALSE;

       EnableWindow (GetDlgItem (hwnd, IDC_ROLLFWD_LASTCKP_BUTTON),  bEnable);
       EnableWindow (GetDlgItem (hwnd, IDC_ROLLFWD_LASTCKP_NUM),     bEnable);
}

static BOOL FillStructureFromControls (HWND hwnd)
{
   char szGreatBuffer   [3000];
   char szBuffer        [100];
   char buftemp[200];
   char buf_num[100];

   HWND hwndLastCkp = GetDlgItem (hwnd, IDC_ROLLFWD_LASTCKP_NUM);
   HWND hwndDevice  = GetDlgItem(hwnd, IDC_ROLLFWD_DEVICE);
   HWND hwndAfter   = GetDlgItem(hwnd, IDC_ROLLFWD_AFTER);
   HWND hwndBefore  = GetDlgItem(hwnd, IDC_ROLLFWD_BEFORE);

   LPUCHAR vnodeName = GetVirtNodeName (GetCurMdiNodeHandle ());
   LPROLLFWDPARAMS  lprollfwd  = GetDlgProp (hwnd);

   ZEROINIT (szGreatBuffer);
   ZEROINIT (szBuffer);


   x_strcpy (szGreatBuffer, "rollforwarddb ");
   x_strcat (szGreatBuffer, lprollfwd->DBName);

   //
   // Generate +c/-c
   //
   if (!Button_GetCheck (GetDlgItem (hwnd, IDC_ROLLFWD_LASTCKP)))
       x_strcat (szGreatBuffer, " -c");

   //
   // Generate +j/-j
   //
   if (!Button_GetCheck (GetDlgItem (hwnd, IDC_ROLLFWD_JOURNALING)))
       x_strcat (szGreatBuffer, " -j");

   //
   // Generate -mdevice
   // 
   ZEROINIT (szBuffer);
   Edit_GetText (hwndDevice, szBuffer, sizeof (szBuffer));
   if (x_strlen (szBuffer) > 0)
   {
       x_strcat (szGreatBuffer, " -m");
       NoLeadingEndingSpace (szBuffer);
       x_strcat (szGreatBuffer, szBuffer);
       x_strcat (szGreatBuffer, ":");
   }
   //
   // Generate -b
   // 
   ZEROINIT (szBuffer);
   Edit_GetText (hwndAfter, szBuffer, sizeof (szBuffer));
   if (x_strlen (szBuffer) > 0)
   {
       x_strcat (szGreatBuffer, " -b");
       x_strcat (szGreatBuffer, szBuffer);
   }
   //
   // Generate -e
   // 
   ZEROINIT (szBuffer);
   Edit_GetText (hwndBefore, szBuffer, sizeof (szBuffer));
   if (x_strlen (szBuffer) > 0)
   {
       x_strcat (szGreatBuffer, " -e");
       x_strcat (szGreatBuffer, szBuffer);
   }

   //
   // Generate #c[n]
   //
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_ROLLFWD_SPECIFIEDCKP)))
   {
       x_strcat (szGreatBuffer, " #c");
       
       if (Edit_GetTextLength (hwndLastCkp) > 0)
       {
           ZEROINIT (szBuffer);
           Edit_GetText (hwndLastCkp, szBuffer, sizeof (szBuffer));
           if (atoi (szBuffer) > 0)
               x_strcat (szGreatBuffer, szBuffer);
       }
   }

   //
   // Generate +w/-w
   //
   if (!Button_GetCheck (GetDlgItem (hwnd, IDC_ROLLFWD_WAIT)))
       x_strcat (szGreatBuffer, " -w");

   //
   // Generate -v
   //
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_ROLLFWD_VERBOSE)))
       x_strcat (szGreatBuffer, " -v");

   //
   // Generate the -uusername
   //
   ZEROINIT (szBuffer);
   DBAGetUserName (vnodeName, szBuffer);
   x_strcat (szGreatBuffer, " -u");
   x_strcat (szGreatBuffer, szBuffer);

   //
   // Generate the table list related commands, only if at least one table specified
   //
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_SPECIFY_TABLE))) {
     if (lprollfwd->lpTable) {
       x_strcat(szGreatBuffer, " -table=");
       while (lprollfwd->lpTable) {
         char * st = (char *)StringWithoutOwner(lprollfwd->lpTable->lpObject);
         x_strcat(szGreatBuffer, st);
         lprollfwd->lpTable = lprollfwd->lpTable->lpNext;
         if (lprollfwd->lpTable)
           x_strcat(szGreatBuffer, ",");
         else
           x_strcat(szGreatBuffer, " ");
       }
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_ROLLFWD_ONERRORCONTINUE)))
         x_strcat(szGreatBuffer, " on_error_continue");
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_ROLLFWD_NOSECONDARY_INDEX)))
         x_strcat(szGreatBuffer, " -nosecondary_index");
       // PS 25/11/98 do not generate the -location and -new_location
       // if the object list is empty.
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_ROLLFWD_RELOCATE)) &&
           lprollfwd->lpOldLocations && lprollfwd->lpNewLocations) {
         LPSTRINGLIST lpCur;
         x_strcat(szGreatBuffer, " -relocate -location=");
         lpCur = lprollfwd->lpOldLocations;
         while (lpCur) {
           x_strcat(szGreatBuffer, lpCur->lpszString);
           lpCur = lpCur->next;
           if (lpCur)
             x_strcat(szGreatBuffer, ", ");
           else
             x_strcat(szGreatBuffer, " ");
         }
         x_strcat(szGreatBuffer, " -new_location=");
         lpCur = lprollfwd->lpNewLocations;
         while (lpCur) {
           x_strcat(szGreatBuffer, lpCur->lpszString);
           lpCur = lpCur->next;
           if (lpCur)
             x_strcat(szGreatBuffer, ", ");
           else
             x_strcat(szGreatBuffer, " ");
         }
       }
     }
   }

   // #m management
   if (IsDlgButtonChecked(hwnd, IDC_PARALLEL_RFW)) {
     char szMulti[10];
     Edit_GetText(GetDlgItem(hwnd, IDC_PARALLEL_RFW_LOCS), szMulti, sizeof(szMulti));

     x_strcat(szGreatBuffer, " #m");
     x_strcat(szGreatBuffer, szMulti);
     x_strcat(szGreatBuffer, " ");
   }

   // -dmf_cache_size
   ZEROINIT(szBuffer);
   ZEROINIT(buftemp);
   ComboBox_GetText(GetDlgItem(hwnd, IDC_COMBO_ROLLFWD_CACHE), szBuffer, sizeof(szBuffer));
   Edit_GetText(GetDlgItem(hwnd, IDC_EDIT_ROLLFWD_BUF), buftemp, sizeof(buftemp));
   sprintf(buf_num, "%d", BUF_NUM);
   /*if ( x_strncmp(szBuffer, cache_size[0], x_strlen(szBuffer)) != 0)
   {
	   if (strncmp(szBuffer, "4", strlen(szBuffer)) == 0)
		   x_strcat(szGreatBuffer, " -dmf_cache_size_4k=");
	   else if (strncmp(szBuffer, "8", strlen(szBuffer)) == 0)
		   x_strcat(szGreatBuffer, " -dmf_cache_size_8k=");
	   else if (strncmp(szBuffer, "16", strlen(szBuffer)) == 0)
		   x_strcat(szGreatBuffer, " -dmf_cache_size_16k=");
	   else if (strncmp(szBuffer, "32", strlen(szBuffer)) == 0)
		   x_strcat(szGreatBuffer, " -dmf_cache_size_32k=");
	   else if (strncmp(szBuffer, "64", strlen(szBuffer)) == 0)
		   x_strcat(szGreatBuffer, " -dmf_cache_size_64k=");
	   x_strcat(szGreatBuffer, buftemp);

   }*/
   if (x_strncmp(szBuffer, cache_size[0][0], x_strlen(szBuffer)) != 0 ||
	   x_strncmp(buftemp, buf_num, x_strlen(buftemp)) != 0 )
   {
	   int i=0;
	   int bSet = 0;
	   for (i = 0; cache_size[i][0] != 0; i++)
	   {
		   if (x_strncmp(szBuffer, cache_size[i][0], x_strlen(szBuffer)) == 0)
		   {
			   x_strcat(szGreatBuffer, cache_size[i][1]);
			   bSet = 1;
			   break;
		   }
	   }
	   if (bSet)
		   x_strcat(szGreatBuffer, buftemp);
   }
   /*else if (x_strncmp(buftemp, buf_num, x_strlen(buftemp))!=0)
   {
	   x_strcat(szGreatBuffer, " -dmf_cache_size=");
	   x_strcat(szGreatBuffer, buftemp);
   }*/
   


   wsprintf(buftemp,
       ResourceString ((UINT)IDS_T_RMCMD_ROLLFWD), // Rollforward DB on database %s::%s
       vnodeName,
       lprollfwd->DBName);
   execrmcmd(vnodeName,szGreatBuffer,buftemp);

   return TRUE;
}

