/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Project  : CA/OpenIngres Visual DBA
**
**    Source   : crview.c
**
**    Dialog box for Creating a View
**
** History after 23-may-2000
**
** 22-May-2000 (uk$so01)
**    bug 99242 (DBCS) modify the functions:
**    AnalyseString()
** 26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 14-Fev-2002 (uk$so01)
**    SIR #106648, Integrate SQL Assistant In-Process Server
** 09-Mar-2009 (smeke01) b121764
**    Pass parameters to CreateSelectStmWizard as UCHAR so that we don't need  
**    to use the T2A macro.
*/

#include <ctype.h>
#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "getdata.h"
#include "lexical.h"
#include "msghandl.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "tchar.h"
#include "sqlasinf.h"
#include "extccall.h"


//#define MAX_TEXT_EDIT 2000

static BOOL CreateObject              (HWND hwnd, LPVIEWPARAMS lpview);
static BOOL AlterObject               (HWND hwnd);
static void OnDestroy                 (HWND hwnd);
static void OnCommand                 (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog              (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange          (HWND hwnd);
static void EnableDisableOKButton     (HWND hwnd);
static BOOL FillControlsFromStructure (HWND hwnd, LPVIEWPARAMS lpviewparams);
static BOOL FillStructureFromControls (HWND hwnd, LPVIEWPARAMS lpviewparams);
static BOOL NEAR SelectWizard         (HWND hDlg, LPVIEWPARAMS lpviewparams);
static int  AnalyseString             (HWND hwnd, TCHAR* listCol);


BOOL CALLBACK __export DlgCreateViewDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI __export DlgCreateView (HWND hwndOwner, LPVIEWPARAMS lpviewparams)

/*
// Function:
// Shows the Create/Alter user dialog.
//
// Paramters:
//     1) hwndOwner  :  Handle of the parent window for the dialog.
//     2) lpviewparam:  Points to structure containing information used to 
//                      initialise the dialog. The dialog uses the same
//                      structure to return the result of the dialog.
//
// Returns: TRUE if successful.
//
*/
{
   FARPROC lpProc;
   int     retVal;
   
   if (!IsWindow(hwndOwner) || !lpviewparams)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgCreateViewDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_VIEW),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpviewparams
       );

   FreeProcInstance (lpProc);
   return (retVal);
}

// ____________________________________________________________________________%
//

BOOL CALLBACK __export DlgCreateViewDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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

// ____________________________________________________________________________%
//

static BOOL OnInitDialog (HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   LPVIEWPARAMS lpview  = (LPVIEWPARAMS)lParam;
   char szTitle [MAX_TITLEBAR_LEN];
   char szFormat[100];

   if (!AllocDlgProp (hwnd, lpview))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();
   if (lpview->bCreate)
   {
       LoadString (hResource, (UINT)IDS_T_CREATE_VIEW, szFormat, sizeof (szFormat));
       lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_VIEW));
   }
   //else
   //    LoadString (hResource, (UINT)IDS_T_ALTER_VIEW,  szFormat, sizeof (szFormat));
   wsprintf (szTitle,
       szFormat,
       GetVirtNodeName (GetCurMdiNodeHandle ()),
       lpview->DBName);

   SetWindowText (hwnd, szTitle);
   FillControlsFromStructure (hwnd, lpview);
   EnableDisableOKButton  (hwnd);

   // Restricted features if gateway
   if (GetOIVers() == OIVERS_NOTOI)
     ShowWindow(GetDlgItem(hwnd, IDC_VIEW_CHECK_OPTION), SW_HIDE);

   richCenterDialog(hwnd);
   return TRUE;
}

// ____________________________________________________________________________%
//

static void OnDestroy (HWND hwnd)
{
   DeallocDlgProp (hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}

// ____________________________________________________________________________%
//

static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPVIEWPARAMS lpview     = GetDlgProp (hwnd);

   switch (id)
   {
       case IDOK:
       {
           BOOL Success = FALSE;

           if (lpview->bCreate)
           {
               Success = FillStructureFromControls (hwnd, lpview);
               if (!Success)
               {
                   FreeAttachedPointers (lpview, OT_VIEW);
                   break;
               }

               Success = CreateObject (hwnd, lpview);
               FreeAttachedPointers (lpview, OT_VIEW);
           }
           else
           {
               Success = AlterObject (hwnd);
           }

           if (!Success)
               break;
           else
               EndDialog (hwnd, TRUE);
       }    
       break;

       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           break;

       case IDC_VIEW_NAME:
       case IDC_VIEW_SELECT_STATEMENT:
           EnableDisableOKButton (hwnd);
           break;

       case IDC_VIEW_SELECT_WIZ:
        SelectWizard(hwnd, lpview);
   }
}

// ____________________________________________________________________________%
//

static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}


// ____________________________________________________________________________%
//

static void EnableDisableOKButton  (HWND hwnd)
{
   HWND    hwndViewName    = GetDlgItem (hwnd, IDC_VIEW_NAME);
   HWND    hwndStatement   = GetDlgItem (hwnd, IDC_VIEW_SELECT_STATEMENT);

   if ((Edit_GetTextLength (hwndViewName) == 0) || (Edit_GetTextLength (hwndStatement) == 0))
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
}

// ____________________________________________________________________________%
//

static BOOL CreateObject (HWND hwnd, LPVIEWPARAMS lpview)
{
   int     ires;
   LPUCHAR vnodeName = GetVirtNodeName (GetCurMdiNodeHandle ());

   ires = DBAAddObject (vnodeName, OT_VIEW, (void *) lpview );

   if (ires != RES_SUCCESS)
   {
       ErrorMessage ((UINT) IDS_E_CREATE_VIEW_FAILED, ires);
       return FALSE;
   }

   return TRUE;
}

// ____________________________________________________________________________%
//

static BOOL AlterObject  (HWND hwnd)
{

   return TRUE;
}

// ____________________________________________________________________________%
//

static BOOL FillControlsFromStructure (HWND hwnd, LPVIEWPARAMS lpviewparams)
{
   HWND hwndViewName    = GetDlgItem (hwnd, IDC_VIEW_NAME);
   HWND hwndStatement   = GetDlgItem (hwnd, IDC_VIEW_SELECT_STATEMENT);
   HWND hwndCheckOption = GetDlgItem (hwnd, IDC_VIEW_CHECK_OPTION);

   Edit_LimitText (hwndViewName,  MAXOBJECTNAME -1);
   Edit_LimitText (hwndStatement, MAX_TEXT_EDIT -1);

   Edit_SetText   (hwndViewName,   lpviewparams->objectname);
   Edit_SetText   (hwndStatement,  lpviewparams->lpSelectStatement);
   Button_SetCheck(hwndCheckOption,lpviewparams->WithCheckOption);

   return TRUE;
}

// ____________________________________________________________________________%
//

static BOOL FillStructureFromControls (HWND hwnd, LPVIEWPARAMS lpviewparams)
{
   HWND hwndViewName    = GetDlgItem (hwnd, IDC_VIEW_NAME);
   HWND hwndCheckOption = GetDlgItem (hwnd, IDC_VIEW_CHECK_OPTION);
   HWND hwndStatement   = GetDlgItem (hwnd, IDC_VIEW_SELECT_STATEMENT);
   HWND hwndViewColumns = GetDlgItem (hwnd, IDC_VIEW_COLUMNS);
   char szViewName [MAXOBJECTNAME];
   char szStatement[MAX_TEXT_EDIT];
   int  len;

   Edit_GetText (hwndViewName, szViewName, sizeof (szViewName));
   if (!IsObjectNameValid (szViewName, OT_UNKNOWN))
   {
       MsgInvalidObject ((UINT)IDS_E_NOT_A_VALIDE_OBJECT);
       Edit_SetSel (hwndViewName, 0, -1);
       SetFocus (hwndViewName);
       return FALSE;
   }
   
   Edit_GetText (hwndStatement, szStatement, sizeof (szStatement));
   len = Edit_GetTextLength (hwndViewColumns);

   if (x_strlen (szStatement) >= 1)
   {
       lpviewparams->lpSelectStatement = ESL_AllocMem (x_strlen (szStatement)+10+len);
       if (!lpviewparams->lpSelectStatement)
       {
           ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
           return FALSE;
       }
       else
       {
           int final_state;

           if (len > 0)
           {
               char* bufCol;

               bufCol = ESL_AllocMem (len+3);
               if (bufCol)
                   Edit_GetText (hwndViewColumns, bufCol, len +1);
               else
               {
                   ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
                   return FALSE;
               }

               final_state = AnalyseString (hwnd, (TCHAR*)bufCol);
               switch (final_state)
               {
                   case 4:
                       x_strcpy (lpviewparams->lpSelectStatement, bufCol);
                       ESL_FreeMem (bufCol);
                       bufCol = NULL;
                       break;
                   case 6:
                   case 8:
                       x_strcpy (lpviewparams->lpSelectStatement, "(");
                       x_strcat (lpviewparams->lpSelectStatement, bufCol);
                       x_strcat (lpviewparams->lpSelectStatement, ")");
                       ESL_FreeMem (bufCol);
                       bufCol = NULL;
                       break;
                   default:
                       ESL_FreeMem (bufCol);
                       bufCol = NULL;
                       SetFocus (hwndViewColumns);
                       return FALSE;
               }
           }
           
           x_strcat (lpviewparams->lpSelectStatement, " as ");
           x_strcat (lpviewparams->lpSelectStatement, szStatement);
       }
   }
   lpviewparams->WithCheckOption = Button_GetCheck (hwndCheckOption);
   x_strcpy (lpviewparams->objectname, szViewName);

   return TRUE;
}

//
// New section for select Wizard - added Emb June 13, 95
//

static BOOL NEAR SelectWizard(HWND hDlg, LPVIEWPARAMS lpviewparams)
{
	LPTSTR lpszSQL = NULL;
	LPUCHAR lpVirtNodeName = GetVirtNodeName(GetCurMdiNodeHandle());

	lpszSQL = CreateSelectStmWizard(hDlg, lpVirtNodeName, lpviewparams->DBName);
	if (lpszSQL)  
	{
		Edit_ReplaceSel(GetDlgItem(hDlg, IDC_VIEW_SELECT_STATEMENT), lpszSQL);
		free(lpszSQL);
		return TRUE;
	}
	return FALSE;
}


static int AnalyseString (HWND hwnd, TCHAR* listCol)
{
   int state, i, len;
   int iPrev1 = 0;
   int iPrev2 = 0;
   int nCount = 0;

   if (!listCol)
       return 0;
   len = x_strlen (listCol);

   i     = 0;
   state = 0;

   while (i < len)
   {
       TCHAR ch = listCol [i];

       switch (state)
       {
           case 0:
               if (ch == 0x0D || ch == 0x0A || ch == _T(' '))
                   state = 0;
               else
               if (_istalpha (ch))
                   state = 6;
               else
               if (ch == _T('('))
                   state = 1;
               else
                   state = -1;
               break;

           case 1:
               if (_istalpha (ch))
                   state = 2;
               else
                   state = -1;
               break;
           
           case 2:
               if (_istalnum (ch) || ch == _T('_'))
                   state = 2;
               else
               if (ch == _T(','))
                   state = 3;
               else
               if (ch == _T(')'))
                   state = 4;
               else
               if (ch == 0x0D || ch == 0x0A || ch == _T(' '))
                   state = 5;
               else
                   state = -1;

               break;

           case 3:
               if (_istalpha (ch))
                   state = 2;
               else
               if (ch == 0x0D || ch == 0x0A || ch == _T(' '))
                   state = 3;
               else
                   state = -1;

               break;

           case 4:
               if (ch == 0x0D || ch == 0x0A || ch == _T(' '))
                   state = 4;
               else
               if (i != len -1)
                   state = -1;

               break;

           case 5:
               if (ch == _T(')'))
                   state = 4;
               else
               if (ch == 0x0D || ch == 0x0A || ch == _T(' '))
                   state = 5;
               else
               if (_istalpha (ch))
               {
                   state = 2;
                   if (listCol [iPrev1] == 0x0A)
                   {
                       listCol [iPrev1] = _T(',');
                       listCol [iPrev2] = _T(' ');
                   }
                   else
                       listCol [iPrev1] = _T(',');

               }
               else
                   state = -1;

               break;

           case 6:
               if (_istalnum (ch) || ch == _T('_'))
                   state = 6;
               else
               if (ch == _T(','))
                   state = 7;
               else
               if (ch == 0x0D || ch == 0x0A || ch == _T(' '))
                   state = 8;
               else
                   state = -1;

               break;
       
           case 7:
               if (_istalpha (ch))
                   state = 6;
               else
               if (ch == 0x0D || ch == 0x0A || ch == _T(' '))
                   state = 7;
               else
                   state = -1;

               break;

           case 8:
               if (ch == 0x0D || ch == 0x0A || ch == _T(' '))
                   state = 8;
               else
               if (_istalpha (ch))
               {
                   state = 6;
                   if (listCol [iPrev1] == 0x0A)
                   {
                       listCol [iPrev1] = _T(',');
                       listCol [iPrev2] = _T(' ');
                   }
                   else
                       listCol [iPrev1] = _T(',');
               }
               else
               if (i != len -1)
                   state = -1;

               break;
       }
       nCount++;
       if (nCount > 1)
            iPrev2 = iPrev1;
       iPrev1 = i;
       //
       // DBCS, i++ will not work
       i += _tclen((const TCHAR*)listCol + i);
   }
   switch (state)
   {
       case 4:
       case 6:
       case 8:
           return state;

       case 1:
           //
           // Syntax error: At least one column must be specified
           //
           ErrorMessage ((UINT)IDS_E_SYNTAX_ONE_COLUMN, RES_ERR);
           break;
       case 2:
       case 5:
           //
           // Syntax error: ')' was expected
           //
           ErrorMessage ((UINT)IDS_E_SYNTAX_CLOSEPARENTHESE, RES_ERR);
           break;
       case 3:
       case 7:
           //
           // Syntax error: Illegal ',' or another column is expected
           //
           ErrorMessage ((UINT)IDS_E_SYNTAX_ILLEGAL_CHAR_OR_COL, RES_ERR);
           break;
       case -1:
           //
           // Syntax error: Illegal characters
           //
           ErrorMessage ((UINT)IDS_E_SYNTAX_ILLEGAL_CHAR, RES_ERR);
   }


   return 0;
}


