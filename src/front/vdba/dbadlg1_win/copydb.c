/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project  : Visual DBA
**
**    Source   : copydb.c
**
**    Generate the Copydb command string.
**
** History:
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
********************************************************************/

#include "dll.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "dom.h"
#include "getdata.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "msghandl.h"

static AUDITDBTPARAMS table;

BOOL CALLBACK __export DlgCopyDBDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK __export CopyDBEnumEditCntls (HWND hwnd, LPARAM lParam);

static BOOL SendCommand(HWND hwnd);
static void OnDestroy(HWND hwnd);
static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);

static void EnableDisableControls (HWND hwnd);


static void InitialiseEditControls (HWND hwnd);

BOOL WINAPI __export DlgCopyDB (HWND hwndOwner, LPCOPYDB lpcopydb)
/*
   Function:
      Shows the Copydb dialog.

   Paramters:
      hwndOwner   - Handle of the parent window for the dialog.

   Returns:
      TRUE if successful.
*/
{
   FARPROC lpProc;
   int     retVal;

   if (!IsWindow(hwndOwner) || !lpcopydb)
   {
     ASSERT(NULL);
     return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgCopyDBDlgProc, hInst);

   retVal = VdbaDialogBoxParam
             (hResource,
             MAKEINTRESOURCE(IDD_COPYINOUT),
             hwndOwner,
             (DLGPROC) lpProc,
             (LPARAM)lpcopydb);

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgCopyDBDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
   char Title[200];
   LPCOPYDB lpcopydb = (LPCOPYDB) lParam;

   InitialiseEditControls(hwnd);

   if (!AllocDlgProp(hwnd, lpcopydb))
      return FALSE;

   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_COPYINOUT));
   EnableDisableControls (hwnd);

   lpcopydb->bPrintable = 0;

   ZEROINIT (table);
   table.lpTable = NULL;

   // Change the title to 'Copydb'
   // Fill the windows title bar
   GetWindowText(hwnd,Title,GetWindowTextLength(hwnd)+1);
   x_strcat(Title, " ");
   x_strcat(Title, GetVirtNodeName ( GetCurMdiNodeHandle ()));
   x_strcat(Title, "::");
   x_strcat(Title,lpcopydb->DBName);

   if (lpcopydb->bStartSinceTable)
   {
     LPOBJECTLIST obj;
     char sztemp[MAXOBJECTNAME];
     // Title
     wsprintf( sztemp ," , table %s",lpcopydb->szDisplayTableName);
     lstrcat(Title,sztemp);
     // Fill Object List
     obj = AddListObject (table.lpTable, x_strlen (lpcopydb->szCurrentTableName) +1);
     if (obj)
     {
       lstrcpy ((UCHAR *)obj->lpObject,lpcopydb->szCurrentTableName );
       table.lpTable = obj;
       lpcopydb->lpTable = table.lpTable;
     }
     else
     {
       ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
       lpcopydb->lpTable = NULL;
     }
     ShowWindow      (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME)  ,SW_SHOW);
     ShowWindow      (GetDlgItem (hwnd, IDC_STATIC_TABLE_NAME),SW_SHOW);
     ShowWindow      (GetDlgItem (hwnd, IDC_TABLES_LIST)      ,SW_HIDE);
     SetWindowText   (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME)  ,lpcopydb->szDisplayTableName );
     EnableWindow    (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME)  ,FALSE );
     Button_SetCheck (GetDlgItem (hwnd, IDC_SPECIFY_TABLE)    ,BST_CHECKED);
     EnableWindow    (GetDlgItem (hwnd, IDC_SPECIFY_TABLE)    ,FALSE );
   }
   SetWindowText(hwnd,Title);

   richCenterDialog(hwnd);

   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   DeallocDlgProp(hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPCOPYDB lpcpydb = GetDlgProp(hwnd);
   switch (id)
   {
     case IDOK:
     {
       int nCount=0;
       int DirLen;
       HWND hwndDir       = GetDlgItem(hwnd, IDC_DIRECTORYNAME);
       HWND hwndSource    = GetDlgItem(hwnd, IDC_SOURCE_DIR);
       HWND hwndDest      = GetDlgItem(hwnd, IDC_DESTINATION_DIR);
       LPUCHAR vnodeName  = GetVirtNodeName (GetCurMdiNodeHandle ());

       DirLen=Edit_GetTextLength(hwndDir)+1;
       if (DirLen >0)
         Edit_GetText(hwndDir, lpcpydb->DirName , DirLen);

       DirLen=Edit_GetTextLength(hwndSource)+1;
       if (DirLen >0)
         Edit_GetText(hwndSource, lpcpydb->DirSource , DirLen);

       DirLen=Edit_GetTextLength(hwndDest)+1;
       if (DirLen >0)
         Edit_GetText(hwndDest, lpcpydb->DirDest , DirLen);

       lpcpydb->bPrintable = IsDlgButtonChecked(hwnd, IDC_CREATE_PRINT);

       // store in the structure the current user name
       ZEROINIT (lpcpydb->UserName);
       DBAGetUserName(vnodeName,lpcpydb->UserName);

       if (lpcpydb->bStartSinceTable)
       {
         char szTblOwner[MAXOBJECTNAME];
         OwnerFromString(lpcpydb->lpTable->lpObject, szTblOwner);
         if ( IsTheSameOwner(GetCurMdiNodeHandle(),szTblOwner) == FALSE )
           break;
       }
       if (SendCommand(hwnd)==TRUE)
         {
         FreeObjectList (table.lpTable);
         table.lpTable = NULL;
         EndDialog(hwnd, TRUE);
         }
       else
         {
         lpcpydb->lpTable = table.lpTable; // reload anchor for linked list
         SetFocus(hwnd);
         }
       break;
     }

     case IDCANCEL:
       {
       FreeObjectList (table.lpTable);
       table.lpTable = NULL;
       EndDialog(hwnd, FALSE);
       break;
       }
     case IDC_SPECIFY_TABLE:
       EnableDisableControls (hwnd);
         break;
     
     case IDC_TABLES_LIST:
       {
       int   iret;
       x_strcpy(table.DBName , lpcpydb->DBName);
       table.bRefuseTblWithDupName = FALSE;
       iret = DlgAuditDBTable (hwnd, &table);

       lpcpydb->lpTable = table.lpTable;
       break;
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
   Edit_LimitText( GetDlgItem(hwnd, IDC_DIRECTORYNAME )   , MAX_DIRECTORY_LEN-1);
   Edit_LimitText( GetDlgItem(hwnd, IDC_SOURCE_DIR )      , MAX_DIRECTORY_LEN-1);
   Edit_LimitText( GetDlgItem(hwnd, IDC_DESTINATION_DIR ) , MAX_DIRECTORY_LEN-1);
}

static void EnableDisableControls (HWND hwnd)
{
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_SPECIFY_TABLE)))
     EnableWindow (GetDlgItem (hwnd, IDC_TABLES_LIST),  TRUE);
   else
     EnableWindow (GetDlgItem (hwnd, IDC_TABLES_LIST),  FALSE);
}

// SendCommand

// create and execute remote command
static BOOL SendCommand(HWND hwnd)
{
   UCHAR buf2[MAXOBJECTNAME*3];
   UCHAR buf[MAX_RMCMD_BUFSIZE];
   int ObjectLen = 0;
   int BufLen    = 0;
   BOOL      bFirst  = TRUE;
   LPCOPYDB  lpcpydb = GetDlgProp(hwnd);
   LPUCHAR vnodeName = GetVirtNodeName(GetCurMdiNodeHandle ());

	char szgateway[200];
	BOOL bHasGWSuffix = GetGWClassNameFromString(vnodeName, szgateway);
   x_strcpy(buf,"copydb ");
   x_strcat(buf,lpcpydb->DBName);

   if ( IsStarDatabase(GetCurMdiNodeHandle (),lpcpydb->DBName) )
     x_strcat(buf,"/star");
   else {
	   if (bHasGWSuffix) {
		   x_strcat(buf,"/");
		   x_strcat(buf,szgateway);
	   }
	}
   x_strcat(buf," ");



   //store the name of table
   while (lpcpydb->lpTable)
     {
     ObjectLen = x_strlen(lpcpydb->lpTable->lpObject );
     BufLen    = x_strlen(buf) ;

     if (ObjectLen >= MAX_LINE_COMMAND-BufLen)
       {
         ErrorMessage ((UINT) IDS_E_TOO_MANY_TABLES_SELECTED, RES_ERR);  
         return FALSE;
       }
     else
       {
         char * st = (char *)StringWithoutOwner(lpcpydb->lpTable->lpObject);
         x_strcat(buf,st);
         x_strcat(buf," ");
         lpcpydb->lpTable = lpcpydb->lpTable->lpNext;
       }
     }

   // store the user name in command line
   DBAGetUserName (vnodeName , lpcpydb->UserName);
   
   ObjectLen = x_strlen(lpcpydb->UserName)+4;
   BufLen    = x_strlen(buf);
   if (ObjectLen >= MAX_LINE_COMMAND-BufLen)
     {
       ErrorMessage ((UINT) IDS_E_TOO_MANY_TABLES_SELECTED, RES_ERR);  
       return FALSE;
     }
     else
     {
       x_strcat(buf,"-u");
       x_strcat(buf,lpcpydb->UserName);
       x_strcat(buf," ");
     }
   
   //store create printable data file option in command line
   if (lpcpydb->bPrintable == 1 )
     {
     ObjectLen = 4;
     BufLen    = x_strlen(buf);
   
     if (ObjectLen >= MAX_LINE_COMMAND-BufLen)
       {
         ErrorMessage ((UINT) IDS_E_TOO_MANY_TABLES_SELECTED, RES_ERR);  
         return FALSE;
       }
       else
       {
         x_strcat(buf,"-c");
         x_strcat(buf," ");
       }
     }
   // store the directory name in command line
   if ( lpcpydb->DirName[0] != '\0')
     {
     ObjectLen = x_strlen(lpcpydb->DirName)+4;
     BufLen    = x_strlen(buf);
   
     if (ObjectLen >= MAX_LINE_COMMAND-BufLen)
       {
         //MessageBox(GetFocus(),"too many tables selected or directory name too long",NULL, MB_ICONSTOP|MB_OK);
         ErrorMessage ((UINT)IDS_E_TOO_MANYTABLES_OR_DIR, RES_ERR);
         return FALSE;
       }
       else
         {
         x_strcat(buf,"-d");
         x_strcat(buf,lpcpydb->DirName);
         x_strcat(buf," ");
         }
     }

   // store the source directory name in command line
   if ( lpcpydb->DirSource[0] != '\0')
     {
     ObjectLen = x_strlen(lpcpydb->DirSource)+10;
     BufLen    = x_strlen(buf);
   
     if (ObjectLen >= MAX_LINE_COMMAND-BufLen)
       {
         //MessageBox(GetFocus(),"too many tables selected or source directory too long",NULL, MB_ICONSTOP|MB_OK);
         ErrorMessage ((UINT)IDS_E_TOO_MANYTABLES_OR_SDIR, RES_ERR);
         return FALSE;
       }
       else
       {
         x_strcat(buf,"-source=");
         x_strcat(buf,lpcpydb->DirSource);
         x_strcat(buf," ");
       }
     }
   // store the destination directory name in command line
   if ( lpcpydb->DirDest[0] != '\0')
     {
     ObjectLen = x_strlen(lpcpydb->DirDest)+7;
     BufLen    = x_strlen(buf);
   
     if (ObjectLen >= MAX_LINE_COMMAND-BufLen)
       {
         //MessageBox(GetFocus(),"too many tables selected or destination directory too long",NULL, MB_ICONSTOP|MB_OK);
         ErrorMessage ((UINT)IDS_E_TOO_MANYTABLES_OR_DDIR, RES_ERR);
         return FALSE;
       }
       else
         {
           x_strcat(buf,"-dest=");
           x_strcat(buf,lpcpydb->DirDest);
         }
     }

   wsprintf(buf2,
       ResourceString ((UINT) IDS_T_RMCMD_COPY_INOUT),
       vnodeName,
       lpcpydb->DBName);

   execrmcmd(vnodeName,buf,buf2);

   return(TRUE);
}
