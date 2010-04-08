/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : confsvr.c
//
//    Select the option for build server for replicator 1.1
//
********************************************************************/

#include "dll.h"     
#include "dlgres.h"


BOOL CALLBACK __export DlgConfSvrDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnCommand        (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog     (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnDestroy        (HWND hwnd);
static void OnSysColorChange (HWND hwnd);

BOOL WINAPI __export DlgConfSvr (HWND hwndOwner)
/*
//   Function:
//   Shows the Type and server dialog.
//
//   Paramters:
//       hwndOwner - Handle of the parent window for the dialog.
//
//   Returns:
//       TRUE if successful.
//
//   Note:
//       If invalid parameters are passed in they are reset to a default
//       so the dialog can be safely displayed.
*/
{
  FARPROC lpProc;
  int     retVal;

  if (!IsWindow(hwndOwner))
  {
      ASSERT(NULL);
      return FALSE;
  }

  lpProc = MakeProcInstance ((FARPROC) DlgConfSvrDlgProc, hInst);
  retVal = VdbaDialogBox
      (hResource,
      MAKEINTRESOURCE (IDD_REPL_CONFSVR),
      hwndOwner,
      (DLGPROC) lpProc
      );

  FreeProcInstance (lpProc);
  return (retVal);
}

BOOL CALLBACK __export DlgConfSvrDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);

    case WM_INITDIALOG:
        return HANDLE_WM_INITDIALOG (hwnd, wParam, lParam, OnInitDialog);

    default:
        return HandleUserMessages(hwnd, message, wParam, lParam);
  }
  return TRUE;
}
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{

  lpHelpStack = StackObject_PUSH (lpHelpStack,
                                  StackObject_INIT ((UINT)IDD_REPL_CONFSVR));
  Button_SetCheck(GetDlgItem(hwnd, IDC_ALL_OPTION),TRUE);
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
  HWND hwndButton = GetDlgItem(hwnd, IDC_ALL_OPTION);

  switch (id)
  {
    case IDOK:
    {
        if (Button_GetCheck(hwndButton))
          EndDialog (hwnd, 2);
        else
          EndDialog (hwnd, 1);
    }
    break;
    case IDCANCEL:
        EndDialog (hwnd, FALSE);
        break;

  }

}

static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}





