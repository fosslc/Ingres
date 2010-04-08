/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project  : Ingres Visual DBA
**
**    Source   : savefile.c
**
**    Dialog box for Saving the Environment
**
**  History after 01-Jan-2000
**
**   19-Jan-2000 (noifr01)
**    (SIR 100065) get the extension name from a (unique) resource
**    rather than having it hardcoded in the dialog
**	20-Jul-2000 (hanch04)
**	    The GetSaveFileName dlg does not append .vdbacfg onto the filename.
**	    This works on NT and may be a MAINWIN bug.  Manually append
**	    .vdbacfg onto the saved environment filename.
**	28-Aug-2001 (hanje04)
**	    Bug 105617
**	    Default extension (.vdbacfg) is too long for UNIX to add .Z.
**	    Revert back to using .cfg on UNIX only.
**      24-Apr-2003 (hanch04/noifr01)
**          For MAINWIN, don't used the customized save environment dialog.
**	    Fixed for bug 110132

********************************************************************/


#include "dll.h"
#include "dbadlg1.h"
#include "dlgres.h"

#ifndef MAINWIN
#include "error.h"
#endif

#include <commdlg.h>
#include <dlgs.h>         // common dialog box items ids

#include "msghandl.h"
#include "resource.h"
typedef struct tagFILTERS
{
    int nString;
    int nFilter;
} FILTERS, FAR * LPFILTERS;

static FILTERS filterTypes[] =
{
    IDS_CFGFILES,   IDS_CFGFILES_FILTER,
    IDS_ALLFILES,   IDS_ALLFILES_FILTER,
    -1      // terminator
};

static void CreateFilterString (LPSTR lpszFilter, int cMaxBytes);
static BOOL PasswordOnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void PasswordOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL SaveEnvOnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void SaveEnvOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

// variables used by hookProc and DlgSaveEnv and DlgPasswordSaveEnv
static BOOL bPassword;
static char password[PASSWORD_LEN];

//
// Dialog proc for password entry
//
BOOL CALLBACK EXPORT PasswordSaveEnvDlgProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
  switch(msg) {
    HANDLE_MSG(hwnd, WM_INITDIALOG, PasswordOnInitDialog);
    HANDLE_MSG(hwnd, WM_COMMAND, PasswordOnCommand);
  }
  return FALSE;
}

static BOOL PasswordOnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    Edit_LimitText(GetDlgItem(hwnd, IDC_SAVEENV_PASSWORD), PASSWORD_LEN - 1);
    Edit_LimitText(GetDlgItem(hwnd, IDC_SAVEENV_CONFIRMPASSWORD), PASSWORD_LEN - 1);
    return TRUE;
}

static void PasswordOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
  char psw1[PASSWORD_LEN];
  char psw2[PASSWORD_LEN];
  char rsBuf[BUFSIZE];

  switch (id) {
    case IDOK:
      GetDlgItemText(hwnd, IDC_SAVEENV_PASSWORD, psw1, sizeof(psw1)-1);
      GetDlgItemText(hwnd, IDC_SAVEENV_CONFIRMPASSWORD, psw2, sizeof(psw2)-1);
      if (x_strcmp(psw1, psw2)) {
        HWND currentFocus = GetFocus();
        LoadString(hResource, (UINT)IDS_E_PLEASE_RETYPE_PASSWORD,
                  rsBuf, sizeof(rsBuf)-1);
        MessageBox(NULL, rsBuf, NULL, MB_ICONEXCLAMATION | MB_OK| MB_TASKMODAL);
        SetFocus (currentFocus);
        return;    // Don't exit
      }
      if (!x_strlen(psw1)) {
        HWND currentFocus = GetFocus();
        LoadString(hResource, (UINT)IDS_E_PASSWORD_REQUIRED,
                  rsBuf, sizeof(rsBuf)-1);
        MessageBox(NULL, rsBuf, NULL, MB_ICONEXCLAMATION | MB_OK| MB_TASKMODAL);
        SetFocus (currentFocus);
        return;    // Don't exit
      }
      x_strcpy(password, psw1);
      EndDialog(hwnd, TRUE);
      break;

    case IDCANCEL:
      EndDialog(hwnd, FALSE);
      break;
  }
}

//
// Hook procedure for environment save
//
#ifndef MAINWIN
UINT CALLBACK EXPORT SaveEnvHookProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    switch(msg) {
        HANDLE_MSG(hwnd, WM_INITDIALOG, SaveEnvOnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, SaveEnvOnCommand);
    }
    return 0;
}
static BOOL SaveEnvOnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    CheckDlgButton(hwnd, IDC_SAVEENV_BPASSWORD, TRUE);  // default
    return TRUE;
}

static void SaveEnvOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
  BOOL  status;
  char  buf[BUFSIZE];

      if (id==IDOK) {
        status = IsDlgButtonChecked(hwnd, IDC_SAVEENV_BPASSWORD);
        GetDlgItemText(hwnd, edt1, buf, sizeof(buf));
        bPassword    = status;
      }
}
#endif


BOOL WINAPI __export DlgSaveFile (HWND hwndOwner, LPTOOLSSAVEFILE lpToolsSave)
/*
    Function:
        Shows the File Save dialog for environment.

    Paramters:
        hwndOwner       - Handle of the parent window for the dialog.
        lpToolsSave     - Points to structure containing information used to 
                            - initialise the dialog. The dialog uses the same
                            - structure to return the result of the dialog.

    Returns:
        TRUE if successful.
*/
{
    OPENFILENAME  ofn;
    char          szFilter[256];

#ifdef MAINWIN
	char bufcaption[200];
#else
	char buftemp[200];
#endif
	char bufextension[200];

  BOOL          bRet;
  
    if (!IsWindow(hwndOwner) || !lpToolsSave)
    {
        ASSERT(NULL);
        return FALSE;
    }

    CreateFilterString(szFilter, sizeof(szFilter));
    lpToolsSave->szFile[0]='\0';

    ZEROINIT(ofn);

#ifdef MAINWIN
	x_strcpy(bufextension,"cfg");
#else
	fstrncpy(buftemp, ResourceString ((UINT)IDS_CFGFILES_FILTER),sizeof(buftemp)-1);
	if (buftemp[0]!='*' || buftemp[1]!='.' || x_strlen(buftemp)>20) {
		myerror(ERR_FILE); // internal error
		return FALSE;
	}
	x_strcpy(bufextension,buftemp+2);
#endif

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndOwner;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrDefExt = bufextension;
    ofn.lpstrFile = lpToolsSave->szFile;

    ofn.nMaxFile = sizeof(lpToolsSave->szFile) - (x_strlen(bufextension)+1);   // reserve ".<extension>" add

#ifdef MAINWIN
	fstrncpy(bufcaption, ResourceString ((UINT)IDS_ST_SAVE),sizeof(bufcaption)-1);
	ofn.lpstrTitle=bufcaption;
    ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    bPassword = FALSE;
#else
    ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT |
              OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_SAVEENV);
    ofn.lpfnHook = SaveEnvHookProc;
    ofn.hInstance = hResource;
    bPassword = TRUE;
    password[0] = '\0';
#endif


  // help : use the custom dialog box id
  lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_SAVEENV));
  bRet = GetSaveFileName(&ofn);
  lpHelpStack = StackObject_POP (lpHelpStack);

  if (bRet) {

    // add extension if necessary
#ifdef MAINWIN
    if (!x_strchr(lpToolsSave->szFile,'.'))
		x_strcat(lpToolsSave->szFile, ".cfg");
#endif /* MAINWIN */

    // request password if needed
    if (bPassword) 
    {
      lpHelpStack = StackObject_PUSH (lpHelpStack,
                          StackObject_INIT ((UINT)IDD_SAVEENVPSW));
      if (VdbaDialogBox(hResource, MAKEINTRESOURCE(IDD_SAVEENVPSW),
                           hwndOwner, PasswordSaveEnvDlgProc) == IDOK) 
      {
        x_strcpy(lpToolsSave->szPassword, password);
        lpToolsSave->bPassword = bPassword;
      }
      else 
      {
        bRet = FALSE;
        lpToolsSave->szPassword[0] = '\0';  // Security
        lpToolsSave->bPassword = FALSE;     // Security
      }
      lpHelpStack = StackObject_POP (lpHelpStack);
      
    }
    else {
      lpToolsSave->szPassword[0] = '\0';  // Security
      lpToolsSave->bPassword = FALSE;     // Security
    }
  }
  
  return bRet;
}

static void CreateFilterString (LPSTR lpszFilter, int cMaxBytes)
{
    int nFilter = 0;
    int nLen;

    while (filterTypes[nFilter].nString != -1)
    {
        if ((nLen = LoadString(hResource, filterTypes[nFilter].nString, lpszFilter, cMaxBytes)) != 0)
        {
            lpszFilter += nLen + 1;
            cMaxBytes -= (nLen + 1);

            if ((nLen = LoadString(hResource, filterTypes[nFilter].nFilter, lpszFilter, cMaxBytes)) != 0)
            {
                lpszFilter += nLen + 1;
                cMaxBytes -= (nLen + 1);
            }
        }
        nFilter++;
    }
    if (cMaxBytes > 0)
        *lpszFilter = (char)NULL;
}
