/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Project  : Ingres Visual DBA
**
**  Source   : procedur.c
**
**  Dialog box for Creating a Procedure 
**
**  History after 22-May-2000
**
**   23-May-2000 (noifr01)
**     (bug 99242)  cleanup for DBCS compliance
**   21-Jun-2001 (schph01)
**     (SIR 103694) STEP 2 support of UNICODE datatypes
** 14-Fev-2002 (uk$so01)
**    SIR #106648, Integrate SQL Assistant In-Process Server
** 12-May-2004 (schph01)
**    (SIR 111507) Add management for new column type bigint
** 12-Oct-2006 (wridu01)
**    (Sir 116835) Add support for Ansi Date/Time datatypes
** 09-Mar-2009 (smeke01) b121764
**    Pass parameters to CreateSQLWizard as UCHAR so that we don't 
**    need to use the T2A macro.
*/

#include <ctype.h>
#include <stdlib.h>
#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "catospin.h"
#include "getdata.h"
#include "lexical.h"
#include "msghandl.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "sqlasinf.h"
#include "extccall.h"


//#define  MAX_TEXT_EDIT    2000


static BOOL CreateObject     (HWND hwnd, LPPROCEDUREPARAMS lpprocedure);
static BOOL AlterObject      (HWND hwnd);
static void OnDestroy        (HWND hwnd);
static void OnCommand        (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog     (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange (HWND hwnd);

static BOOL OccupyObjectsControl      (HWND hwnd);
static void EnableDisableOKButton     (HWND hwnd);
static BOOL FillControlsFromStructure (HWND hwnd, LPPROCEDUREPARAMS lpprocedure);
static BOOL FillStructureFromControls (HWND hwnd, LPPROCEDUREPARAMS lpprocedure);

static BOOL NEAR AddProcedureParameter(HWND hDlg);
static BOOL NEAR AddProcedureDeclare(HWND hDlg);
static BOOL NEAR AddProcedureStatement(HWND hDlg, LPPROCEDUREPARAMS lpprocedure);

BOOL CALLBACK __export DlgCreateProcedureDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI __export DlgCreateProcedure (HWND hwndOwner, LPPROCEDUREPARAMS lpprocedure)
/*
// Function:
// Shows the Refresh dialog.
//
// Paramters:
//     1) hwndOwner    :   Handle of the parent window for the dialog.
//     1) lpprocedure  :   Points to structure containing information used to 
//                         initialise the dialog. 
//
// Returns:
//     TRUE if successful.
//
*/
{
   FARPROC lpProc;
   int     retVal;
   
   if (!IsWindow (hwndOwner) || !lpprocedure)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgCreateProcedureDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_PROCEDURE),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpprocedure
       );

   FreeProcInstance (lpProc);
   return (retVal);
}

// ____________________________________________________________________________%
//

BOOL CALLBACK __export DlgCreateProcedureDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
   LPPROCEDUREPARAMS lpprocedure   = (LPPROCEDUREPARAMS)lParam;
   char szTitle [MAX_TITLEBAR_LEN];
   char szFormat[100];

   if (!AllocDlgProp (hwnd, lpprocedure))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();
   // 23/05/96 Emb : since the dialog resource is also used
   // for Ingres Desktop, hide the unnecessary control(s)
   ShowWindow(GetDlgItem(hwnd, IDC_PROCEDURE_DYNAMIC), SW_HIDE);
   ShowWindow(GetDlgItem(hwnd, 105), SW_HIDE);  // static text

   if (lpprocedure->bCreate)
   {
       LoadString (hResource, (UINT)IDS_T_CREATE_PROCEDURE, szFormat, sizeof (szFormat));
       lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_PROCEDURE));
   }
   //else
   //    LoadString (hResource, (UINT)IDS_T_ALTER_PROCEDURE,  szFormat, sizeof (szFormat));
   wsprintf (szTitle,
       szFormat,
       GetVirtNodeName (GetCurMdiNodeHandle ()),
       lpprocedure->DBName);
   SetWindowText  (hwnd, szTitle);

   FillControlsFromStructure (hwnd, lpprocedure);
   EnableDisableOKButton     (hwnd);
   richCenterDialog(hwnd);
   return TRUE;
}

// ____________________________________________________________________________%
//

static void OnDestroy(HWND hwnd)
{
   DeallocDlgProp(hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}

// ____________________________________________________________________________%
//

static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPPROCEDUREPARAMS lpprocedure   = GetDlgProp (hwnd);
   switch (id)
   {
       case IDOK:
       {
           BOOL Success = FALSE;

           if (lpprocedure->bCreate)
           {
               Success = FillStructureFromControls (hwnd, lpprocedure);
               if (!Success)
               {
                   FreeAttachedPointers (lpprocedure, OT_PROCEDURE);
                   break;
               }

               Success = CreateObject (hwnd, lpprocedure);
               FreeAttachedPointers (lpprocedure, OT_PROCEDURE);
           }
           /*
           No Alter procedure allowed

           else
           {
               Success = AlterObject (hwnd);
           }
           */

           if (!Success)
               break;
           else
               EndDialog (hwnd, TRUE);
       }
       break;

       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           break;

       case IDC_PROCEDURE_NAME:
       case IDC_PROCEDURE_STATEMENT:
           EnableDisableOKButton (hwnd);
           break;

       case IDC_ADD_PROCEDURE_PARAMETER:
          AddProcedureParameter(hwnd);
          break;
       case IDC_ADD_PROCEDURE_DECLARE:
          AddProcedureDeclare(hwnd);
          break;
       case IDC_ADD_PROCEDURE_STATEMENT:
          AddProcedureStatement(hwnd, lpprocedure);
          break;
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


static void EnableDisableOKButton (HWND hwnd)
{
   HWND    hwndProcName    = GetDlgItem (hwnd, IDC_PROCEDURE_NAME);
   HWND    hwndStatement   = GetDlgItem (hwnd, IDC_PROCEDURE_STATEMENT);

   if ((Edit_GetTextLength (hwndProcName) == 0) || (Edit_GetTextLength (hwndStatement) == 0))
   {
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   }
   else
   {
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
   }
}

// ____________________________________________________________________________%
//

static BOOL CreateObject (HWND hwnd, LPPROCEDUREPARAMS lpprocedure)
{
   int     ires;
   LPUCHAR vnodeName = GetVirtNodeName (GetCurMdiNodeHandle ());

   ires = DBAAddObject (vnodeName, OT_PROCEDURE, (void *) lpprocedure );

   if (ires != RES_SUCCESS)
   {
       ErrorMessage ((UINT) IDS_E_CREATE_PROCEDURE_FAILED, ires);
       return FALSE;
   }

   return TRUE;
}

// ____________________________________________________________________________%
//                                                 

static BOOL AlterObject  (HWND hwnd)
{
   return FALSE;
}

// ____________________________________________________________________________%
//

static BOOL FillControlsFromStructure (HWND hwnd, LPPROCEDUREPARAMS lpprocedure)
{
   HWND hwndProcName  = GetDlgItem (hwnd, IDC_PROCEDURE_NAME);
   HWND hwndParameter = GetDlgItem (hwnd, IDC_PROCEDURE_PARAMETER);
   HWND hwndDeclare   = GetDlgItem (hwnd, IDC_PROCEDURE_DECLARE);
   HWND hwndStatement = GetDlgItem (hwnd, IDC_PROCEDURE_STATEMENT);

   Edit_LimitText (hwndProcName,  MAXOBJECTNAME -1);
   Edit_LimitText (hwndParameter, MAX_TEXT_EDIT -1);
   Edit_LimitText (hwndDeclare,   MAX_TEXT_EDIT -1);
   Edit_LimitText (hwndStatement, MAX_TEXT_EDIT -1);

   Edit_SetText   (hwndProcName,  lpprocedure->objectname );
   Edit_SetText   (hwndParameter, lpprocedure->lpProcedureParams );
   Edit_SetText   (hwndDeclare,   lpprocedure->lpProcedureDeclare );
   Edit_SetText   (hwndStatement, lpprocedure->lpProcedureStatement );

   return TRUE;
}

// ____________________________________________________________________________%
//

static BOOL FillStructureFromControls (HWND hwnd, LPPROCEDUREPARAMS lpprocedure)
{
   HWND hwndProcName  = GetDlgItem (hwnd, IDC_PROCEDURE_NAME);
   HWND hwndParameter = GetDlgItem (hwnd, IDC_PROCEDURE_PARAMETER);
   HWND hwndDeclare   = GetDlgItem (hwnd, IDC_PROCEDURE_DECLARE);
   HWND hwndStatement = GetDlgItem (hwnd, IDC_PROCEDURE_STATEMENT);
   char    szProcName  [MAXOBJECTNAME];
   char    szParameter [MAX_TEXT_EDIT];
   char    szDeclare   [MAX_TEXT_EDIT];
   char    szStatement [MAX_TEXT_EDIT];

   Edit_GetText (hwndProcName, szProcName, sizeof (szProcName));
   if (!IsObjectNameValid (szProcName, OT_UNKNOWN))
   {
       MsgInvalidObject ((UINT)IDS_E_NOT_A_VALIDE_OBJECT);
       Edit_SetSel (hwndProcName, 0, -1);
       SetFocus (hwndProcName);
       return FALSE;
   }
   x_strcpy (lpprocedure->objectname, szProcName);

   Edit_GetText (hwndParameter, szParameter,   sizeof (szParameter));
   Edit_GetText (hwndDeclare,   szDeclare,     sizeof (szDeclare));
   Edit_GetText (hwndStatement, szStatement,   sizeof (szStatement));

   if (x_strlen (szDeclare) > 0)
   {
       lpprocedure->lpProcedureDeclare   = ESL_AllocMem (x_strlen (szDeclare)+1);
       if (!lpprocedure->lpProcedureDeclare)
       {
           ErrorMessage ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
           return FALSE;
       }
       x_strcpy (lpprocedure->lpProcedureDeclare, szDeclare);
   }
   if (x_strlen (szParameter) > 0)
   {
       lpprocedure->lpProcedureParams    = ESL_AllocMem (x_strlen (szParameter)+1);
       if (!lpprocedure->lpProcedureParams)
       {
           FreeAttachedPointers (lpprocedure, OT_PROCEDURE);
           ErrorMessage ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
           return FALSE;
       }
       x_strcpy (lpprocedure->lpProcedureParams,  szParameter);
   }
   if (x_strlen (szStatement) > 0)
   {
       lpprocedure->lpProcedureStatement = ESL_AllocMem (x_strlen (szStatement)+1);
       if (!lpprocedure->lpProcedureStatement)
       {
           FreeAttachedPointers (lpprocedure, OT_PROCEDURE);
           ErrorMessage ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
           return FALSE;
       }
       x_strcpy (lpprocedure->lpProcedureStatement, szStatement);
   }

   return TRUE;
}

//
// New section for sub-boxes Add/wizard - added Emb June 9, 95
// Structures taken from creattbl.c
//

// Structure describing the data types available
static CTLDATA typeTypes[] =
{
   INGTYPE_C,            IDS_INGTYPE_C,
   INGTYPE_CHAR,         IDS_INGTYPE_CHAR,
   INGTYPE_TEXT,         IDS_INGTYPE_TEXT,
   INGTYPE_VARCHAR,      IDS_INGTYPE_VARCHAR,
   INGTYPE_LONGVARCHAR,  IDS_INGTYPE_LONGVARCHAR,
   INGTYPE_INT4,         IDS_INGTYPE_INT4,
   INGTYPE_INT2,         IDS_INGTYPE_INT2,
   INGTYPE_INT1,         IDS_INGTYPE_INT1,
   INGTYPE_DECIMAL,      IDS_INGTYPE_DECIMAL,
   INGTYPE_FLOAT,        IDS_INGTYPE_FLOAT,
   INGTYPE_FLOAT8,       IDS_INGTYPE_FLOAT8,
   INGTYPE_FLOAT4,       IDS_INGTYPE_FLOAT4,
   INGTYPE_DATE,         IDS_INGTYPE_DATE,
   INGTYPE_MONEY,        IDS_INGTYPE_MONEY,
   INGTYPE_BYTE,         IDS_INGTYPE_BYTE,
   INGTYPE_BYTEVAR,      IDS_INGTYPE_BYTEVAR,
   INGTYPE_LONGBYTE,     IDS_INGTYPE_LONGBYTE,
   INGTYPE_INTEGER,      IDS_INGTYPE_INTEGER,   // equivalent to INT4
   -1    // terminator
};

static CTLDATA typeTypesUnicode[] =
{
   INGTYPE_C,              IDS_INGTYPE_C,
   INGTYPE_CHAR,           IDS_INGTYPE_CHAR,
   INGTYPE_TEXT,           IDS_INGTYPE_TEXT,
   INGTYPE_VARCHAR,        IDS_INGTYPE_VARCHAR,
   INGTYPE_LONGVARCHAR,    IDS_INGTYPE_LONGVARCHAR,
   INGTYPE_INT4,           IDS_INGTYPE_INT4,
   INGTYPE_INT2,           IDS_INGTYPE_INT2,
   INGTYPE_INT1,           IDS_INGTYPE_INT1,
   INGTYPE_DECIMAL,        IDS_INGTYPE_DECIMAL,
   INGTYPE_FLOAT,          IDS_INGTYPE_FLOAT,
   INGTYPE_FLOAT8,         IDS_INGTYPE_FLOAT8,
   INGTYPE_FLOAT4,         IDS_INGTYPE_FLOAT4,
   INGTYPE_DATE,           IDS_INGTYPE_DATE,
   INGTYPE_MONEY,          IDS_INGTYPE_MONEY,
   INGTYPE_BYTE,           IDS_INGTYPE_BYTE,
   INGTYPE_BYTEVAR,        IDS_INGTYPE_BYTEVAR,
   INGTYPE_LONGBYTE,       IDS_INGTYPE_LONGBYTE,
   INGTYPE_INTEGER,        IDS_INGTYPE_INTEGER,   // equivalent to INT4
   INGTYPE_OBJKEY,         IDS_INGTYPE_OBJECTKEY,
   INGTYPE_TABLEKEY,       IDS_INGTYPE_TABLEKEY,
   INGTYPE_UNICODE_NCHR,   IDS_INGTYPE_UNICODE_NCHR,     // unicode nchar
   INGTYPE_UNICODE_NVCHR,  IDS_INGTYPE_UNICODE_NVCHR,   // unicode nvarchar
   INGTYPE_UNICODE_LNVCHR, IDS_INGTYPE_UNICODE_LNVCHR, // unicode long nvarchar
   -1    // terminator
};

static CTLDATA typeTypesBigInt[] =
{
   INGTYPE_C,              IDS_INGTYPE_C,
   INGTYPE_CHAR,           IDS_INGTYPE_CHAR,
   INGTYPE_TEXT,           IDS_INGTYPE_TEXT,
   INGTYPE_VARCHAR,        IDS_INGTYPE_VARCHAR,
   INGTYPE_LONGVARCHAR,    IDS_INGTYPE_LONGVARCHAR,
   INGTYPE_BIGINT,         IDS_INGTYPE_BIGINT,
   INGTYPE_INT8,           IDS_INGTYPE_INT8,
   INGTYPE_INT4,           IDS_INGTYPE_INT4,
   INGTYPE_INT2,           IDS_INGTYPE_INT2,
   INGTYPE_INT1,           IDS_INGTYPE_INT1,
   INGTYPE_DECIMAL,        IDS_INGTYPE_DECIMAL,
   INGTYPE_FLOAT,          IDS_INGTYPE_FLOAT,
   INGTYPE_FLOAT8,         IDS_INGTYPE_FLOAT8,
   INGTYPE_FLOAT4,         IDS_INGTYPE_FLOAT4,
   INGTYPE_DATE,           IDS_INGTYPE_DATE,
   INGTYPE_MONEY,          IDS_INGTYPE_MONEY,
   INGTYPE_BYTE,           IDS_INGTYPE_BYTE,
   INGTYPE_BYTEVAR,        IDS_INGTYPE_BYTEVAR,
   INGTYPE_LONGBYTE,       IDS_INGTYPE_LONGBYTE,
   INGTYPE_INTEGER,        IDS_INGTYPE_INTEGER,   // equivalent to INT4
   INGTYPE_OBJKEY,         IDS_INGTYPE_OBJECTKEY,
   INGTYPE_TABLEKEY,       IDS_INGTYPE_TABLEKEY,
   INGTYPE_UNICODE_NCHR,   IDS_INGTYPE_UNICODE_NCHR,     // unicode nchar
   INGTYPE_UNICODE_NVCHR,  IDS_INGTYPE_UNICODE_NVCHR,   // unicode nvarchar
   INGTYPE_UNICODE_LNVCHR, IDS_INGTYPE_UNICODE_LNVCHR, // unicode long nvarchar
   -1    // terminator
};

static CTLDATA typeTypesAnsiDate[] =
{
   INGTYPE_C,              IDS_INGTYPE_C,
   INGTYPE_CHAR,           IDS_INGTYPE_CHAR,
   INGTYPE_TEXT,           IDS_INGTYPE_TEXT,
   INGTYPE_VARCHAR,        IDS_INGTYPE_VARCHAR,
   INGTYPE_LONGVARCHAR,    IDS_INGTYPE_LONGVARCHAR,
   INGTYPE_BIGINT,         IDS_INGTYPE_BIGINT,
   INGTYPE_INT8,           IDS_INGTYPE_INT8,
   INGTYPE_INT4,           IDS_INGTYPE_INT4,
   INGTYPE_INT2,           IDS_INGTYPE_INT2,
   INGTYPE_INT1,           IDS_INGTYPE_INT1,
   INGTYPE_DECIMAL,        IDS_INGTYPE_DECIMAL,
   INGTYPE_FLOAT,          IDS_INGTYPE_FLOAT,
   INGTYPE_FLOAT8,         IDS_INGTYPE_FLOAT8,
   INGTYPE_FLOAT4,         IDS_INGTYPE_FLOAT4,
   INGTYPE_DATE,           IDS_INGTYPE_DATE,
   INGTYPE_ADTE,           IDS_INGTYPE_ADTE,             // ansidate
   INGTYPE_TMWO,           IDS_INGTYPE_TMWO,             // time without time zone
   INGTYPE_TMW,            IDS_INGTYPE_TMW,              // time with time zone
   INGTYPE_TME,            IDS_INGTYPE_TME,              // time with local time zone
   INGTYPE_TSWO,           IDS_INGTYPE_TSWO,             // timestamp without time zone
   INGTYPE_TSW,            IDS_INGTYPE_TSW,              // timestamp with time zone
   INGTYPE_TSTMP,          IDS_INGTYPE_TSTMP,            // timestamp with local time zone
   INGTYPE_INYM,           IDS_INGTYPE_INYM,             // interval year to month
   INGTYPE_INDS,           IDS_INGTYPE_INDS,             // interval day to second
   INGTYPE_IDTE,           IDS_INGTYPE_IDTE,             // ingresdate
   INGTYPE_MONEY,          IDS_INGTYPE_MONEY,
   INGTYPE_BYTE,           IDS_INGTYPE_BYTE,
   INGTYPE_BYTEVAR,        IDS_INGTYPE_BYTEVAR,
   INGTYPE_LONGBYTE,       IDS_INGTYPE_LONGBYTE,
   INGTYPE_INTEGER,        IDS_INGTYPE_INTEGER,          // equivalent to INT4
   INGTYPE_OBJKEY,         IDS_INGTYPE_OBJECTKEY,
   INGTYPE_TABLEKEY,       IDS_INGTYPE_TABLEKEY,
   INGTYPE_UNICODE_NCHR,   IDS_INGTYPE_UNICODE_NCHR,     // unicode nchar
   INGTYPE_UNICODE_NVCHR,  IDS_INGTYPE_UNICODE_NVCHR,    // unicode nvarchar
   INGTYPE_UNICODE_LNVCHR, IDS_INGTYPE_UNICODE_LNVCHR,   // unicode long nvarchar
   -1    // terminator
};

typedef struct tagPROCADD {
  LPSTR lpStatement;        // returned statement
  BOOL  bParameter;         // TRUE for parameter, FALSE for declare section
} PROCADD, FAR *LPPROCADD;

BOOL CALLBACK __export ProcedureSubAddDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static void OnSubAddDestroy        (HWND hwnd);
static void OnSubAddCommand        (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnSubAddInitDialog     (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static BOOL BuildVariableDeclaration(HWND hwnd, LPPROCADD lpProcAdd);

static BOOL NEAR AddProcedureParameter(HWND hDlg)
{
  FARPROC lpProc;
  int     retVal;
  PROCADD procAdd;

  procAdd.lpStatement = NULL;
  procAdd.bParameter  = TRUE;
  lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_PROC_SUBADD));
  lpProc = MakeProcInstance ((FARPROC) ProcedureSubAddDlgProc, hInst);
  retVal = VdbaDialogBoxParam(hResource,
                          MAKEINTRESOURCE (IDD_PROC_SUBADD),
                          hDlg,
                          (DLGPROC) lpProc,
                          (LPARAM)(LPPROCADD)&procAdd);
  FreeProcInstance (lpProc);
  lpHelpStack = StackObject_POP (lpHelpStack);
  if (procAdd.lpStatement) 
  {
    HWND   hwndParam = GetDlgItem(hDlg, IDC_PROCEDURE_PARAMETER);
    DWORD  len;
    DWORD  dwResult;
    WPARAM wStart;
    int    i;
    char   ch [3];
    char*  txt;
    ch [0] = 0x0D;
    ch [1] = 0x0A;
    ch [2] = '\0';


    dwResult = Edit_GetSel (hwndParam);
    wStart   = LOWORD (dwResult);
    len = Edit_GetTextLength (hwndParam);
    txt = ESL_AllocMem (len +1);
    if (!txt)
    {
       ErrorMessage ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
       return FALSE;
    }
    Edit_GetText (hwndParam, txt, len);

    if (len >0)
    {
       if (wStart == 0)
       {
           Edit_ReplaceSel (hwndParam, procAdd.lpStatement);
           Edit_ReplaceSel (hwndParam, ", ");
           Edit_ReplaceSel (hwndParam, ch);
       }
       else
       if (wStart > 0 && wStart < len)
       {
           Edit_ReplaceSel (hwndParam, procAdd.lpStatement);
           Edit_ReplaceSel (hwndParam, ", ");
           Edit_ReplaceSel (hwndParam, ch);
       }
       else
       if (wStart == len)
       {
	   	   int l = x_strlen (txt);

           for (i=0;i<l;i+=CMbytecnt(txt+i)) /* this is just a check for a non space character, wherever it is. order of scan is not significant*/
           {
               if (txt [i] != ' ')
               {
                   Edit_ReplaceSel (hwndParam, ", ");
				   Edit_ReplaceSel (hwndParam, ch);
                   break;
               }
           }        
           //Edit_ReplaceSel (hwndParam, ", ");
           //Edit_ReplaceSel (hwndParam, ch);
           Edit_ReplaceSel (hwndParam, procAdd.lpStatement);
       }
    }
    else
    {
       Edit_ReplaceSel (hwndParam, procAdd.lpStatement);
    }
    //Edit_ReplaceSel(GetDlgItem(hDlg, IDC_PROCEDURE_PARAMETER),
    //                procAdd.lpStatement);
    ESL_FreeMem (procAdd.lpStatement);
    return TRUE;
  }
  return FALSE;
}
static BOOL NEAR AddProcedureDeclare(HWND hDlg)
{
  FARPROC lpProc;
  int     retVal;
  PROCADD procAdd;

  procAdd.lpStatement = NULL;
  procAdd.bParameter  = FALSE;    // Declare
  lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_ADDDECLAREBOX));
  lpProc = MakeProcInstance ((FARPROC) ProcedureSubAddDlgProc, hInst);
  retVal = VdbaDialogBoxParam(hResource,
                          MAKEINTRESOURCE (IDD_PROC_SUBADD),
                          hDlg,
                          (DLGPROC) lpProc,
                          (LPARAM)(LPPROCADD)&procAdd);
  FreeProcInstance (lpProc);
  lpHelpStack = StackObject_POP (lpHelpStack);
  if (procAdd.lpStatement) {
    Edit_ReplaceSel(GetDlgItem(hDlg, IDC_PROCEDURE_DECLARE),
                    procAdd.lpStatement);
    ESL_FreeMem(procAdd.lpStatement);
    return TRUE;
  }
  return FALSE;
}

static BOOL NEAR AddProcedureStatement(HWND hDlg, LPPROCEDUREPARAMS lpprocedure)
{
	LPTSTR lpszSQL = NULL, lpszSQL2 = NULL;
	LPUCHAR lpVirtNodeName = GetVirtNodeName(GetCurMdiNodeHandle());

	lpszSQL = CreateSQLWizard(hDlg, lpVirtNodeName, lpprocedure->DBName);
	if (lpszSQL)
	{
		lpszSQL2 = malloc(lstrlen(lpszSQL) + 3 + 1);
		if (lpszSQL2) 
		{
			lstrcpy(lpszSQL2, lpszSQL);
			lstrcat(lpszSQL2, TEXT(" ; "));
			Edit_ReplaceSel(GetDlgItem(hDlg, IDC_PROCEDURE_STATEMENT), lpszSQL2);
			free(lpszSQL2);
		}
		else
			Edit_ReplaceSel(GetDlgItem(hDlg, IDC_PROCEDURE_STATEMENT), lpszSQL);
		free(lpszSQL);
		
		return TRUE;
	}

	return FALSE;
}


BOOL CALLBACK __export ProcedureSubAddDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
       HANDLE_MSG (hwnd, WM_COMMAND, OnSubAddCommand);
       HANDLE_MSG (hwnd, WM_DESTROY, OnSubAddDestroy);

       case WM_INITDIALOG:
           return HANDLE_WM_INITDIALOG (hwnd, wParam, lParam, OnSubAddInitDialog);

       default:
           return FALSE;
   }
   return TRUE;
}

// ____________________________________________________________________________%
//


static void SubAdd_EnableButton (HWND hwnd)
{
   HWND hwndParam = GetDlgItem (hwnd, IDC_PROC_SUBADD_NAME);
   HWND hwndWidth = GetDlgItem (hwnd, IDC_PROC_SUBADD_WIDTH);
   BOOL bWidth    = FALSE;
 
   if (IsWindowVisible (hwndWidth))
   {
       if (Edit_GetTextLength (hwndWidth) == 0)
           bWidth = TRUE;
       else
           bWidth = FALSE;
   }

   if (Edit_GetTextLength (hwndParam) == 0 || bWidth)
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
}

static void UpdateLengthField(HWND hwnd, HWND hwndCtl)
{
   char      buf[BUFSIZE];
   int       nStringId;
   int       nIdx;           // combo box index
   UINT      uDataType;      // combo box index type
// update length field caption according to selection
   nIdx = ComboBox_GetCurSel(hwndCtl);
   uDataType = (UINT)ComboBox_GetItemData(hwndCtl, nIdx);
   switch (uDataType)
   {
       case INGTYPE_DECIMAL:
         nStringId = IDS_PRESEL;
         break;
       case INGTYPE_FLOAT:
         nStringId = IDS_SIGDIG;
         break;
       default:
         nStringId = IDS_LENGTH;
   }
   LoadString(hResource, nStringId,  buf, sizeof(buf));
   SetDlgItemText(hwnd, IDC_PROC_SUBADD_WIDTH_CAPT, buf);

   // reset length field according to selection
   switch (uDataType) {
     case INGTYPE_DECIMAL:
       SetDlgItemText(hwnd, IDC_PROC_SUBADD_WIDTH, "5,0");
       break;
     default:
       SetDlgItemText(hwnd, IDC_PROC_SUBADD_WIDTH, "0");
       break;
   }

   // Enable/disable length field according to selection
   switch (uDataType) {
     case INGTYPE_C      :     // " c("
     case INGTYPE_CHAR   :     // " char("
     case INGTYPE_TEXT   :     // " text("
     case INGTYPE_VARCHAR:     // " varchar("
     case INGTYPE_LONGVARCHAR:
     case INGTYPE_DECIMAL:     // " decimal("
     case INGTYPE_BYTE   :     // " byte("
     case INGTYPE_UNICODE_NCHR:
     case INGTYPE_UNICODE_NVCHR:

     case INGTYPE_BYTEVAR:  
     case INGTYPE_LONGBYTE: 
     case INGTYPE_FLOAT:    

       ShowWindow(GetDlgItem(hwnd, IDC_PROC_SUBADD_WIDTH), SW_SHOW);
       break;
     default:
       SetDlgItemText(hwnd, IDC_PROC_SUBADD_WIDTH, "");
       ShowWindow(GetDlgItem(hwnd, IDC_PROC_SUBADD_WIDTH), SW_HIDE);
       break;
   }
}

static BOOL OnSubAddInitDialog (HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   LPPROCADD  lpProcAdd = (LPPROCADD)lParam;
   char       szTitle [MAX_TITLEBAR_LEN];


   if (!AllocDlgProp (hwnd, lpProcAdd))
       return FALSE;
   if (lpProcAdd->bParameter)
       LoadString (hResource, (UINT)IDS_T_PROCEDURE_PARAM_CAPT, szTitle, sizeof (szTitle));
   else
       LoadString (hResource, (UINT)IDS_T_PROCEDURE_DECLARE_CAPT, szTitle, sizeof (szTitle));
   SetWindowText  (hwnd, szTitle);

   // Initialize controls
   if (GetOIVers() >= OIVERS_91)
      OccupyComboBoxControl(GetDlgItem(hwnd, IDC_PROC_SUBADD_TYPE), typeTypesAnsiDate);
   else if (GetOIVers() >= OIVERS_30)
      OccupyComboBoxControl(GetDlgItem(hwnd, IDC_PROC_SUBADD_TYPE), typeTypesBigInt);
   else if (GetOIVers() >= OIVERS_26)
      OccupyComboBoxControl(GetDlgItem(hwnd, IDC_PROC_SUBADD_TYPE), typeTypesUnicode);
   else
      OccupyComboBoxControl(GetDlgItem(hwnd, IDC_PROC_SUBADD_TYPE), typeTypes);

   Edit_LimitText(GetDlgItem(hwnd, IDC_PROC_SUBADD_NAME), MAXOBJECTNAME);
   Edit_LimitText(GetDlgItem(hwnd, IDC_PROC_SUBADD_WIDTH), 10);



   // FINIR : remplissage controls
  
   SubAdd_EnableButton (hwnd);
   UpdateLengthField(hwnd, GetDlgItem (hwnd, IDC_PROC_SUBADD_TYPE));

   richCenterDialog(hwnd);
   return TRUE;
}

static void OnSubAddDestroy(HWND hwnd)
{
  DeallocDlgProp(hwnd);
}

// FINIR
static void OnSubAddCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
  LPPROCADD lpProcAdd;

  lpProcAdd = GetDlgProp (hwnd);
  switch (id) {
    case IDOK:
      // FINIR
      if (BuildVariableDeclaration(hwnd, lpProcAdd)) {
        EndDialog (hwnd, TRUE);
      }
      else
        MessageBeep((UINT)-1);
      break;
    case IDCANCEL:
      EndDialog (hwnd, FALSE);
      break;

    case IDC_PROC_SUBADD_TYPE:
      switch (codeNotify) {
        case CBN_SELCHANGE:
          UpdateLengthField( hwnd, hwndCtl);
          SubAdd_EnableButton (hwnd);
          break;
      }
      break;

    case IDC_PROC_SUBADD_NAME:
       SubAdd_EnableButton (hwnd);
       break;
    case IDC_PROC_SUBADD_WIDTH:
       if (codeNotify == EN_CHANGE)
       {
           if (Edit_GetTextLength (hwndCtl) == 1) 
           {
               char width [10];

               Edit_GetText (hwndCtl, width, 9);
               if (x_stricmp (width, "0") == 0)
                   Edit_SetText (hwndCtl, "1");
           }
       }
       SubAdd_EnableButton (hwnd);
       break;

    case IDC_PROC_SUBADD_NULL:
      if (Button_GetCheck(hwndCtl)) {
        // uncheck and gray 'with default' button
        Button_SetCheck(GetDlgItem(hwnd, IDC_PROC_SUBADD_DEFAULT), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_PROC_SUBADD_DEFAULT), FALSE);
      }
      else 
        EnableWindow(GetDlgItem(hwnd, IDC_PROC_SUBADD_DEFAULT), TRUE);
      break;
  }
}

static char *MakeVariableTypeString(UINT uDataType);

static char szOneSpace[]          = " ";
static char szCloseParenthesis[]  = ") ";
static char szComma[]             = " , ";
static char szSemiColon[]         = " ; ";
static char szWith[]              = "with ";    // trailing space mandatory!
static char szNot[]               = "not ";     // trailing space mandatory!
static char szNull[]              = "null ";    // trailing space mandatory!
static char szDefault[]           = "default "; // trailing space mandatory!

static BOOL BuildVariableDeclaration(HWND hwnd, LPPROCADD lpProcAdd)
{
  char *pString;
  char *pCur;
  int   size;
  int   nIdx;           // combo box index
  UINT  uDataType;      // combo box index type

  size = MAXOBJECTNAME * 5 + 1;
  pString = ESL_AllocMem(size);
  if (!pString)
    return FALSE;
  pCur = pString;

  // variable name
  GetDlgItemText(hwnd, IDC_PROC_SUBADD_NAME , pCur, size - (pCur-pString));
  pCur += x_strlen(pCur);
  x_strcpy(pCur, szOneSpace);
  pCur += x_strlen(pCur);

  // get variable type from combo
  nIdx = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_PROC_SUBADD_TYPE));
  uDataType = (UINT)ComboBox_GetItemData(GetDlgItem(hwnd, IDC_PROC_SUBADD_TYPE), nIdx);

  // variable type and length
  x_strcpy(pCur, MakeVariableTypeString(uDataType));
  pCur += x_strlen(pCur);
  switch (uDataType) {
    case INGTYPE_C      :     // " c("
    case INGTYPE_CHAR   :     // " char("
    case INGTYPE_TEXT   :     // " text("
    case INGTYPE_VARCHAR:     // " varchar("
    case INGTYPE_LONGVARCHAR:
    case INGTYPE_DECIMAL:     // " decimal("
    case INGTYPE_BYTE   :     // " byte("
    case INGTYPE_UNICODE_NCHR:
    case INGTYPE_UNICODE_NVCHR:

    case INGTYPE_BYTEVAR:  
    case INGTYPE_LONGBYTE: 
    case INGTYPE_FLOAT:    

      GetDlgItemText(hwnd, IDC_PROC_SUBADD_WIDTH, pCur, size - (pCur-pString));
      pCur += x_strlen(pCur);
      x_strcpy(pCur, szCloseParenthesis);
      pCur += x_strlen(pCur);
      break;
    default:
      x_strcpy(pCur, szOneSpace);
      pCur += x_strlen(pCur);
      break;
  }

  // null/default fields
  if (Button_GetCheck(GetDlgItem(hwnd, IDC_PROC_SUBADD_NULL))) {
    x_strcpy(pCur, szWith);
    pCur += x_strlen(pCur);
    x_strcpy(pCur, szNull);
    pCur += x_strlen(pCur);
  }
  else {
    x_strcpy(pCur, szNot);
    pCur += x_strlen(pCur);
    x_strcpy(pCur, szNull);
    pCur += x_strlen(pCur);
    if (Button_GetCheck(GetDlgItem(hwnd, IDC_PROC_SUBADD_DEFAULT)))
      x_strcpy(pCur, szWith);
    else 
      x_strcpy(pCur, szNot);
    pCur += x_strlen(pCur);
    x_strcpy(pCur, szDefault);
    pCur += x_strlen(pCur);
  }
  // terminator
  /*
  if (lpProcAdd->bParameter)
    x_strcpy(pCur, szComma);
  else 
    x_strcpy(pCur, szSemiColon);
  */

  if (!lpProcAdd->bParameter)
    x_strcpy(pCur, szSemiColon);

  lpProcAdd->lpStatement = pString;
  return TRUE;
}

struct coltype {
   UINT uDataType;
   UCHAR *cDataType;
} ColTypes[] = {
      {INGTYPE_C           ," c("            },
      {INGTYPE_CHAR        ," char("         },
      {INGTYPE_TEXT        ," text("         },
      {INGTYPE_VARCHAR     ," varchar("      },
      {INGTYPE_LONGVARCHAR ," long varchar " },
      {INGTYPE_BIGINT      ," bigint "       },
      {INGTYPE_INT8        ," int8 "         },
      {INGTYPE_INT4        ," int "          },
      {INGTYPE_INT2        ," smallint"      },
      {INGTYPE_INT1        ," integer1"      },
      {INGTYPE_DECIMAL     ," decimal("      },
      {INGTYPE_FLOAT       ," float"         },
      {INGTYPE_FLOAT8      ," float"         },
      {INGTYPE_FLOAT4      ," real"          },
      {INGTYPE_DATE        ," date"          },
      {INGTYPE_MONEY       ," money"         },
      {INGTYPE_BYTE        ," byte("         },
      {INGTYPE_BYTEVAR     ," byte varying"  },
      {INGTYPE_INTEGER     ," int "          },   // equivalent to INT4
      {INGTYPE_UNICODE_NCHR," nchar("        },
      {INGTYPE_UNICODE_NVCHR," nvarchar("    },
      {INGTYPE_UNICODE_LNVCHR," long nvarchar"},
      {INGTYPE_ERROR       ,""               }};

static char *MakeVariableTypeString(UINT uDataType)
{
  int cpt;

  for (cpt=0; cpt< sizeof(ColTypes)/sizeof(ColTypes[0]); cpt++)
    if (ColTypes[cpt].uDataType == uDataType)
      return ColTypes[cpt].cDataType;
  return "";
}
