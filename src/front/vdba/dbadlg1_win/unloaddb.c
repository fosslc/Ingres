/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project  : Visual DBA
**
**    Source   : unloaddb.c
**
**    Dialog box for Generating unloaddb command string
**
**  History after 04-May-1999:
**
**    21-Sep-1999 (schph01)
**      SIR #98835
**      Add warning message when unloaddb is used on STAR database.
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
#include "subedit.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "dom.h"
//#include "getvnode.h"

#include "getdata.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "msghandl.h"
#include "lexical.h"
#include "resource.h"


BOOL CALLBACK __export DlgUnLoadDBDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK __export UnLoadDBEnumEditCntls (HWND hwnd, LPARAM lParam);

static void OnDestroy(HWND hwnd);
static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);
static BOOL SendCommand(HWND hwnd);

static void EnableDisableControls (HWND hwnd);


static void InitialiseEditControls (HWND hwnd);

BOOL WINAPI __export DlgUnLoadDB (HWND hwndOwner, LPCOPYDB lpcopydb)
/*
	Function:
		Shows the Copydb dialog.

	Paramters:
		hwndOwner	- Handle of the parent window for the dialog.

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

	lpProc = MakeProcInstance ((FARPROC) DlgUnLoadDBDlgProc, hInst);

	retVal = VdbaDialogBoxParam
             (hResource,
              MAKEINTRESOURCE(IDD_RELOADUNLOAD),
              hwndOwner,
             (DLGPROC) lpProc,
             (LPARAM)lpcopydb);

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgUnLoadDBDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
	lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_RELOADUNLOAD));

	lpcopydb->bPrintable = 0;

	// Change the title to 'Copydb'

	//Fill the windows title bar
	GetWindowText(hwnd,Title,GetWindowTextLength(hwnd)+1);
	x_strcat(Title, " ");
	x_strcat(Title, GetVirtNodeName ( GetCurMdiNodeHandle ()));
	x_strcat(Title, "::");
	x_strcat(Title,lpcopydb->DBName);

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
	switch (id)
	{
		case IDOK:
		{
			int nCount=0;
			int DirLen;
			HWND hwndDir       = GetDlgItem(hwnd, IDC_DIRECTORY_NAME);
			HWND hwndSource    = GetDlgItem(hwnd, IDC_SOURCE_DIRECTORY);
			HWND hwndDest      = GetDlgItem(hwnd, IDC_DESTINATION_DIRECTORY);
			LPCOPYDB lpcpydb   = GetDlgProp(hwnd);
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
			lpcpydb->bPrintable = IsDlgButtonChecked(hwnd, IDC_CREATE_PRINTABLE);

			// store in the structure the current user name   
			ZEROINIT (lpcpydb->UserName);
			DBAGetUserName(vnodeName,lpcpydb->UserName);

			if ( IsStarDatabase(GetCurMdiNodeHandle (),lpcpydb->DBName) ) {
				TCHAR tchMess[BUFSIZE*3];
				wsprintf(tchMess, ResourceString ((UINT)IDS_I_NO_ROWS_WITH_UNLOADDB),
							lpcpydb->DBName);
				MessageBox(hwnd,tchMess,ResourceString(IDS_TITLE_WARNING), MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
			}
			if (SendCommand(hwnd)==TRUE)
				EndDialog(hwnd, TRUE);
			break;
		}

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
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
   Edit_LimitText( GetDlgItem(hwnd, IDC_DIRECTORY_NAME)        , MAX_DIRECTORY_LEN-1);
   Edit_LimitText( GetDlgItem(hwnd, IDC_SOURCE_DIRECTORY)      , MAX_DIRECTORY_LEN-1);
   Edit_LimitText( GetDlgItem(hwnd, IDC_DESTINATION_DIRECTORY) , MAX_DIRECTORY_LEN-1);
}







// SendCommand

// create and execute remote command
static BOOL SendCommand(HWND hwnd)
{
	//create the command for remote 
	UCHAR buf2[(MAXOBJECTNAME*2)+40];
	BOOL      bFirst  = TRUE;
	LPCOPYDB  lpcpydb = GetDlgProp(hwnd);
	UCHAR buf[MAX_RMCMD_BUFSIZE];
	LPUCHAR vnodeName = GetVirtNodeName(GetCurMdiNodeHandle ());   
	char szgateway[200];
	BOOL bHasGWSuffix = GetGWClassNameFromString(vnodeName, szgateway);

	x_strcpy(buf,"unloaddb ");
	x_strcat(buf,lpcpydb->DBName);

	if ( IsStarDatabase(GetCurMdiNodeHandle (),lpcpydb->DBName) )
		x_strcat(buf,"/star");
	else {
		if (bHasGWSuffix){
			x_strcat(buf,"/");
			x_strcat(buf,szgateway);
		}
	}

	x_strcat(buf," ");

	// store the user name in command line
	DBAGetUserName (vnodeName , lpcpydb->UserName);
	x_strcat(buf,"-u");
	x_strcat(buf,lpcpydb->UserName);
	x_strcat(buf," ");

	//store create printable data file option in command line
	if (lpcpydb->bPrintable == 1)
	{
	x_strcat(buf,"-c");
	x_strcat(buf," ");
	}

	// store the directory name in command line
	if ( lpcpydb->DirName[0] != '\0')
	{
		x_strcat(buf,"-d");
		x_strcat(buf,lpcpydb->DirName);
		x_strcat(buf," ");
	}
	// store the source directory name in command line
	if ( lpcpydb->DirSource[0] != '\0')
	{
		x_strcat(buf,"-source=");
		x_strcat(buf,lpcpydb->DirSource);
		x_strcat(buf," ");
	}
	// store the destination directory name in command line
	if ( lpcpydb->DirDest[0] != '\0')
	{
		x_strcat(buf,"-dest=");
		x_strcat(buf,lpcpydb->DirDest);
	}

	wsprintf(buf2,
	ResourceString ((UINT)IDS_T_RMCMD_UNLOADDB), //Generate unload.ing and reload.ing for %s::%s
					vnodeName,
					lpcpydb->DBName);

	execrmcmd(vnodeName,buf,buf2);

	return(TRUE);
}
