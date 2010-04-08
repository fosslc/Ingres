/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project  : CA/OpenIngres Visual DBA
**
**    Source   : changest.c
**
**    Change storage structur of Table/Index
**
**  28-Mar-2002 (noifr01)
**    (sir 99596) removed additional unused resources/sources
**  01-Jul-2004 (schph01)
**    (sir 112590) manage new check box control "Concurrent Updates"
**  07-Apr-2005 (komve01)
**    Bug# 114240/Issue# 14082209
**	  In CheckColumns, changed the SelectString, which was selecting 
**	  columns prefixed with a search string to FindStringExact and 
**	  SetSel, to check the exact column that is looked for.
********************************************************************/

#include "dll.h"
#include <stddef.h>
#include "dbadlg1.h"
#include "dlgres.h"
#include "msghandl.h"
#include "subedit.h"
#include "getdata.h"
#include "domdata.h"
#include "dom.h"
#include "catolist.h"
#include "catospin.h"
#include "catocntr.h"

static int XX_MODIFY_TYPE;
static int XX_STRUCT_TYPE;
static void ComboBox_InitPageSize (HWND hwnd, LONG Page_size);

// Structure describing the available table structures for table
static CTLDATA typeStructs[] =
{
	IDX_BTREE,    IDS_STRUCT_BTREE,
	IDX_ISAM,     IDS_STRUCT_ISAM,
	IDX_HASH,     IDS_STRUCT_HASH,
	IDX_HEAP,     IDS_STRUCT_HEAP,
	IDX_HEAPSORT, IDS_STRUCT_HEAPSORT,
	-1// terminator
};

// Structure describing the available table structures for index
static CTLDATA typeStructs2[] =
{
	IDX_BTREE,    IDS_STRUCT_BTREE,
	IDX_ISAM,     IDS_STRUCT_ISAM,
	IDX_HASH,     IDS_STRUCT_HASH,
	-1// terminator
};


// Structure describing the numeric edit controls
static EDITMINMAX limits[] =
{
	IDC_MINPAGES,    IDC_SPIN_MINPAGES,    IDX_MIN_MINPAGES,    IDX_MAX_MINPAGES,
	IDC_MAXPAGES,    IDC_SPIN_MAXPAGES,    IDX_MIN_MAXPAGES,    IDX_MAX_MAXPAGES,
	IDC_LEAFFILL,    IDC_SPIN_LEAFFILL,    IDX_MIN_LEAFFILL,    IDX_MAX_LEAFFILL,
	IDC_NONLEAFFILL, IDC_SPIN_NONLEAFFILL, IDX_MIN_NONLEAFFILL, IDX_MAX_NONLEAFFILL,
	IDC_FILLFACTOR,  IDC_SPIN_FILLFACTOR,  IDX_MIN_FILLFACTOR,  IDX_MAX_FILLFACTOR,
	IDC_ALLOCATION,  IDC_SPIN_ALLOCATION,  IDX_MIN_ALLOCATION,  IDX_MAX_ALLOCATION,
	IDC_EXTEND,      IDC_SPIN_EXTEND,      IDX_MIN_EXTEND,      IDX_MAX_EXTEND,
	-1// terminator
};

typedef struct tagCNTLENABLE
{
	int nCntlId;      // Control ID
	BOOL bEnabled;    // TRUE if control enabled for this structure type
	long lDefault;    // Default without compression
	long lCDefault;   // Default with compression
} CNTLENABLE, FAR * LPCNTLENABLE;

// Number of controls listed in the controls enable structure
#define CTL_ENABLE   21

typedef struct tagSTRUCTCNTLS
{
	int nStructType;     // Structure type
	CNTLENABLE cntl[CTL_ENABLE];  // Define enabled controls
} STRUCTCNTLS, FAR * LPSTRUCTCNTLS;


// FOR TABLE:
// Structure describing the controls enabled for each structure type
// and the default for the control if enabled
static STRUCTCNTLS cntlEnable[] =
{
	IDX_BTREE,
		IDC_MINPAGES,           FALSE,   -1,   -1,
		IDC_MAXPAGES,           FALSE,   -1,   -1,
		IDC_FILLFACTOR,         TRUE,    80,  100,
		IDC_LEAFFILL,           TRUE,    80,   80,
		IDC_NONLEAFFILL,        TRUE,    70,   70,
		IDC_TXT_MINPAGES,       FALSE,   -1,   -1,
		IDC_TXT_MAXPAGES,       FALSE,   -1,   -1,
		IDC_TXT_FILLFACTOR,     TRUE,    -1,   -1,
		IDC_TXT_LEAFFILL,       TRUE,    -1,   -1,
		IDC_TXT_NONLEAFFILL,    TRUE,    -1,   -1,
		IDC_SPIN_MINPAGES,      FALSE,   -1,   -1,
		IDC_SPIN_MAXPAGES,      FALSE,   -1,   -1,
		IDC_SPIN_FILLFACTOR,    TRUE,    -1,   -1,
		IDC_SPIN_LEAFFILL,      TRUE,    -1,   -1,
		IDC_SPIN_NONLEAFFILL,   TRUE,    -1,   -1,
		IDC_KEY_COMPRESSION,    TRUE,    -1,   -1,
		IDC_DATA_COMPRESSION,   TRUE,    -1,   -1,
		IDC_CHANGEST_HIDATA,    TRUE,    -1,   -1,
		IDC_UNIQUE_NO,          TRUE,    -1,   -1,
		IDC_UNIQUE_ROW,         TRUE,    -1,   -1,
		IDC_UNIQUE_STATEMENT,   TRUE,    -1,   -1,
	
	IDX_HASH,
		IDC_MINPAGES,           TRUE,    16,   16,
		IDC_MAXPAGES,           TRUE,    -1,   -1,
		IDC_FILLFACTOR,         TRUE,    50,   75,
		IDC_LEAFFILL,           FALSE,   -1,   -1,
		IDC_NONLEAFFILL,        FALSE,   -1,   -1,
		IDC_TXT_MINPAGES,       TRUE,    -1,   -1,
		IDC_TXT_MAXPAGES,       TRUE,    -1,   -1,
		IDC_TXT_FILLFACTOR,     TRUE,    -1,   -1,
		IDC_TXT_LEAFFILL,       FALSE,   -1,   -1,
		IDC_TXT_NONLEAFFILL,    FALSE,   -1,   -1,
		IDC_SPIN_MINPAGES,      TRUE,    -1,   -1,
		IDC_SPIN_MAXPAGES,      TRUE,    -1,   -1,
		IDC_SPIN_FILLFACTOR,    TRUE,    -1,   -1,
		IDC_SPIN_LEAFFILL,      FALSE,   -1,   -1,
		IDC_SPIN_NONLEAFFILL,   FALSE,   -1,   -1,
		IDC_KEY_COMPRESSION,    FALSE,   -1,   -1,
		IDC_DATA_COMPRESSION,   TRUE,    -1,   -1,
		IDC_CHANGEST_HIDATA,    TRUE,    -1,   -1,
		IDC_UNIQUE_NO,          TRUE,    -1,   -1,
		IDC_UNIQUE_ROW,         TRUE,    -1,   -1,
		IDC_UNIQUE_STATEMENT,   TRUE,    -1,   -1,
	IDX_ISAM,
		IDC_MINPAGES,           FALSE,   -1,   -1,
		IDC_MAXPAGES,           FALSE,   -1,   -1,
		IDC_FILLFACTOR,         TRUE,    80,  100,
		IDC_LEAFFILL,           FALSE,   -1,   -1,
		IDC_NONLEAFFILL,        FALSE,   -1,   -1,
		IDC_TXT_MINPAGES,       FALSE,   -1,   -1,
		IDC_TXT_MAXPAGES,       FALSE,   -1,   -1,
		IDC_TXT_FILLFACTOR,     TRUE,    -1,   -1,
		IDC_TXT_LEAFFILL,       FALSE,   -1,   -1,
		IDC_TXT_NONLEAFFILL,    FALSE,   -1,   -1,
		IDC_SPIN_MINPAGES,      FALSE,   -1,   -1,
		IDC_SPIN_MAXPAGES,      FALSE,   -1,   -1,
		IDC_SPIN_FILLFACTOR,    TRUE,    -1,   -1,
		IDC_SPIN_LEAFFILL,      FALSE,   -1,   -1,
		IDC_SPIN_NONLEAFFILL,   FALSE,   -1,   -1,
		IDC_KEY_COMPRESSION,    FALSE,   -1,   -1,
		IDC_DATA_COMPRESSION,   TRUE,    -1,   -1,
		IDC_CHANGEST_HIDATA,    TRUE,    -1,   -1,
		IDC_UNIQUE_NO,          TRUE,    -1,   -1,
		IDC_UNIQUE_ROW,         TRUE,    -1,   -1,
		IDC_UNIQUE_STATEMENT,   TRUE,    -1,   -1,
	IDX_HEAP,
		IDC_MINPAGES,           FALSE,   -1,   -1,
		IDC_MAXPAGES,           FALSE,   -1,   -1,
		IDC_FILLFACTOR,         FALSE,   -1,   -1,
		IDC_LEAFFILL,           FALSE,   -1,   -1,
		IDC_NONLEAFFILL,        FALSE,   -1,   -1,
		IDC_TXT_MINPAGES,       FALSE,   -1,   -1,
		IDC_TXT_MAXPAGES,       FALSE,   -1,   -1,
		IDC_TXT_FILLFACTOR,     FALSE,   -1,   -1,
		IDC_TXT_LEAFFILL,       FALSE,   -1,   -1,
		IDC_TXT_NONLEAFFILL,    FALSE,   -1,   -1,
		IDC_SPIN_MINPAGES,      FALSE,   -1,   -1,
		IDC_SPIN_MAXPAGES,      FALSE,   -1,   -1,
		IDC_SPIN_FILLFACTOR,    FALSE,   -1,   -1,
		IDC_SPIN_LEAFFILL,      FALSE,   -1,   -1,
		IDC_SPIN_NONLEAFFILL,   FALSE,   -1,   -1,
		IDC_KEY_COMPRESSION,    FALSE,   -1,   -1,
		IDC_DATA_COMPRESSION,   TRUE,    -1,   -1,
		IDC_CHANGEST_HIDATA,    TRUE,    -1,   -1,
		IDC_UNIQUE_NO,          TRUE,    -1,   -1,
		IDC_UNIQUE_ROW,         FALSE,   -1,   -1,
		IDC_UNIQUE_STATEMENT,   FALSE,   -1,   -1,
	IDX_HEAPSORT,
		IDC_MINPAGES,           FALSE,   -1,   -1,
		IDC_MAXPAGES,           FALSE,   -1,   -1,
		IDC_FILLFACTOR,         FALSE,   -1,   -1,
		IDC_LEAFFILL,           FALSE,   -1,   -1,
		IDC_NONLEAFFILL,        FALSE,   -1,   -1,
		IDC_TXT_MINPAGES,       FALSE,   -1,   -1,
		IDC_TXT_MAXPAGES,       FALSE,   -1,   -1,
		IDC_TXT_FILLFACTOR,     FALSE,   -1,   -1,
		IDC_TXT_LEAFFILL,       FALSE,   -1,   -1,
		IDC_TXT_NONLEAFFILL,    FALSE,   -1,   -1,
		IDC_SPIN_MINPAGES,      FALSE,   -1,   -1,
		IDC_SPIN_MAXPAGES,      FALSE,   -1,   -1,
		IDC_SPIN_FILLFACTOR,    FALSE,   -1,   -1,
		IDC_SPIN_LEAFFILL,      FALSE,   -1,   -1,
		IDC_SPIN_NONLEAFFILL,   FALSE,   -1,   -1,
		IDC_KEY_COMPRESSION,    FALSE,   -1,   -1,
		IDC_DATA_COMPRESSION,   TRUE,    -1,   -1,
		IDC_CHANGEST_HIDATA,    TRUE,    -1,   -1,
		IDC_UNIQUE_NO,          TRUE,    -1,   -1,
		IDC_UNIQUE_ROW,         FALSE,   -1,   -1,
		IDC_UNIQUE_STATEMENT,   FALSE,   -1,   -1,
	-1,   // terminator
};

// FOR INDEX:
// Structure describing the controls enabled for each structure type
// and the default for the control if enabled
static STRUCTCNTLS cntlEnable2[] =
{
	IDX_BTREE,
		IDC_MINPAGES,           FALSE,   -1,   -1,
		IDC_MAXPAGES,           FALSE,   -1,   -1,
		IDC_FILLFACTOR,         TRUE,    80,  100,
		IDC_LEAFFILL,           TRUE,    80,   80,
		IDC_NONLEAFFILL,        TRUE,    70,   70,
		IDC_TXT_MINPAGES,       FALSE,   -1,   -1,
		IDC_TXT_MAXPAGES,       FALSE,   -1,   -1,
		IDC_TXT_FILLFACTOR,     TRUE,    -1,   -1,
		IDC_TXT_LEAFFILL,       TRUE,    -1,   -1,
		IDC_TXT_NONLEAFFILL,    TRUE,    -1,   -1,
		IDC_SPIN_MINPAGES,      FALSE,   -1,   -1,
		IDC_SPIN_MAXPAGES,      FALSE,   -1,   -1,
		IDC_SPIN_FILLFACTOR,    TRUE,    -1,   -1,
		IDC_SPIN_LEAFFILL,      TRUE,    -1,   -1,
		IDC_SPIN_NONLEAFFILL,   TRUE,    -1,   -1,
		IDC_KEY_COMPRESSION,    TRUE,    -1,   -1,
		IDC_DATA_COMPRESSION,   FALSE,   -1,   -1,
		IDC_CHANGEST_HIDATA,    FALSE,   -1,   -1,
		IDC_UNIQUE_NO,          TRUE,    -1,   -1,
		IDC_UNIQUE_ROW,         TRUE,    -1,   -1,
		IDC_UNIQUE_STATEMENT,   TRUE,    -1,   -1,
	
	IDX_HASH,
		IDC_MINPAGES,           TRUE,    16,   16,
		IDC_MAXPAGES,           TRUE,    -1,   -1,
		IDC_FILLFACTOR,         TRUE,    50,   75,
		IDC_LEAFFILL,           FALSE,   -1,   -1,
		IDC_NONLEAFFILL,        FALSE,   -1,   -1,
		IDC_TXT_MINPAGES,       TRUE,    -1,   -1,
		IDC_TXT_MAXPAGES,       TRUE,    -1,   -1,
		IDC_TXT_FILLFACTOR,     TRUE,    -1,   -1,
		IDC_TXT_LEAFFILL,       FALSE,   -1,   -1,
		IDC_TXT_NONLEAFFILL,    FALSE,   -1,   -1,
		IDC_SPIN_MINPAGES,      TRUE,    -1,   -1,
		IDC_SPIN_MAXPAGES,      TRUE,    -1,   -1,
		IDC_SPIN_FILLFACTOR,    TRUE,    -1,   -1,
		IDC_SPIN_LEAFFILL,      FALSE,   -1,   -1,
		IDC_SPIN_NONLEAFFILL,   FALSE,   -1,   -1,
		IDC_KEY_COMPRESSION,    FALSE,   -1,   -1,
		IDC_DATA_COMPRESSION,   TRUE,    -1,   -1,
		IDC_CHANGEST_HIDATA,    FALSE,   -1,   -1,
		IDC_UNIQUE_NO,          TRUE,    -1,   -1,
		IDC_UNIQUE_ROW,         TRUE,    -1,   -1,
		IDC_UNIQUE_STATEMENT,   TRUE,    -1,   -1,
	IDX_ISAM,
		IDC_MINPAGES,           FALSE,   -1,   -1,
		IDC_MAXPAGES,           FALSE,   -1,   -1,
		IDC_FILLFACTOR,         TRUE,    80,  100,
		IDC_LEAFFILL,           FALSE,   -1,   -1,
		IDC_NONLEAFFILL,        FALSE,   -1,   -1,
		IDC_TXT_MINPAGES,       FALSE,   -1,   -1,
		IDC_TXT_MAXPAGES,       FALSE,   -1,   -1,
		IDC_TXT_FILLFACTOR,     TRUE,    -1,   -1,
		IDC_TXT_LEAFFILL,       FALSE,   -1,   -1,
		IDC_TXT_NONLEAFFILL,    FALSE,   -1,   -1,
		IDC_SPIN_MINPAGES,      FALSE,   -1,   -1,
		IDC_SPIN_MAXPAGES,      FALSE,   -1,   -1,
		IDC_SPIN_FILLFACTOR,    TRUE,    -1,   -1,
		IDC_SPIN_LEAFFILL,      FALSE,   -1,   -1,
		IDC_SPIN_NONLEAFFILL,   FALSE,   -1,   -1,
		IDC_KEY_COMPRESSION,    FALSE,   -1,   -1,
		IDC_DATA_COMPRESSION,   TRUE,    -1,   -1,
		IDC_CHANGEST_HIDATA,    FALSE,   -1,   -1,
		IDC_UNIQUE_NO,          TRUE,    -1,   -1,
		IDC_UNIQUE_ROW,         TRUE,    -1,   -1,
		IDC_UNIQUE_STATEMENT,   TRUE,    -1,   -1,
	IDX_HEAP,
		IDC_MINPAGES,           FALSE,   -1,   -1,
		IDC_MAXPAGES,           FALSE,   -1,   -1,
		IDC_FILLFACTOR,         FALSE,   -1,   -1,
		IDC_LEAFFILL,           FALSE,   -1,   -1,
		IDC_NONLEAFFILL,        FALSE,   -1,   -1,
		IDC_TXT_MINPAGES,       FALSE,   -1,   -1,
		IDC_TXT_MAXPAGES,       FALSE,   -1,   -1,
		IDC_TXT_FILLFACTOR,     FALSE,   -1,   -1,
		IDC_TXT_LEAFFILL,       FALSE,   -1,   -1,
		IDC_TXT_NONLEAFFILL,    FALSE,   -1,   -1,
		IDC_SPIN_MINPAGES,      FALSE,   -1,   -1,
		IDC_SPIN_MAXPAGES,      FALSE,   -1,   -1,
		IDC_SPIN_FILLFACTOR,    FALSE,   -1,   -1,
		IDC_SPIN_LEAFFILL,      FALSE,   -1,   -1,
		IDC_SPIN_NONLEAFFILL,   FALSE,   -1,   -1,
		IDC_KEY_COMPRESSION,    FALSE,   -1,   -1,
		IDC_DATA_COMPRESSION,   FALSE,   -1,   -1,
		IDC_CHANGEST_HIDATA,    FALSE,   -1,   -1,
		IDC_UNIQUE_NO,          TRUE,    -1,   -1,
		IDC_UNIQUE_ROW,         FALSE,   -1,   -1,
		IDC_UNIQUE_STATEMENT,   FALSE,   -1,   -1,
	IDX_HEAPSORT,
		IDC_MINPAGES,           FALSE,   -1,   -1,
		IDC_MAXPAGES,           FALSE,   -1,   -1,
		IDC_FILLFACTOR,         FALSE,   -1,   -1,
		IDC_LEAFFILL,           FALSE,   -1,   -1,
		IDC_NONLEAFFILL,        FALSE,   -1,   -1,
		IDC_TXT_MINPAGES,       FALSE,   -1,   -1,
		IDC_TXT_MAXPAGES,       FALSE,   -1,   -1,
		IDC_TXT_FILLFACTOR,     FALSE,   -1,   -1,
		IDC_TXT_LEAFFILL,       FALSE,   -1,   -1,
		IDC_TXT_NONLEAFFILL,    FALSE,   -1,   -1,
		IDC_SPIN_MINPAGES,      FALSE,   -1,   -1,
		IDC_SPIN_MAXPAGES,      FALSE,   -1,   -1,
		IDC_SPIN_FILLFACTOR,    FALSE,   -1,   -1,
		IDC_SPIN_LEAFFILL,      FALSE,   -1,   -1,
		IDC_SPIN_NONLEAFFILL,   FALSE,   -1,   -1,
		IDC_KEY_COMPRESSION,    FALSE,   -1,   -1,
		IDC_DATA_COMPRESSION,   FALSE,   -1,   -1,
		IDC_CHANGEST_HIDATA,    FALSE,   -1,   -1,
		IDC_UNIQUE_NO,          TRUE,    -1,   -1,
		IDC_UNIQUE_ROW,         FALSE,   -1,   -1,
		IDC_UNIQUE_STATEMENT,   FALSE,   -1,   -1,
	-1,   // terminator
};


// Pointer to the field drawing procedure for the container
static DRAWPROC lpfnDrawProc = NULL;

// Pointer to the record data for the container
static LPIDXATTR lpidxAttr = NULL;

// Size of the lpidxAttr structure
static UINT uAttrSize;

// Total number of fields that have been created in the container
static UINT uFieldCounter = 0;

// Pointer to the original container window procedure
static WNDPROC wndprocCnt = NULL;

// Pointer to the original container child window procedure
static WNDPROC wndprocCntChild = NULL;

// Pointer to the original edit window procedure
static WNDPROC wndprocEdit = NULL;

// Handle to the container child.
static HWND hwndCntChild = NULL;

// Handle to the edit control used in the container for editing.
static HWND hwndCntEdit = NULL;

// Length of the key string.
static DWORD dwKeyLen = 0;

// Height of the check box.  Since its square the width is the same.
static int nCBHeight = 0;

// The maximum value of the column key
static int nMaxKey = 0;

// Store the structures that represent the cell that contains the edit
// window
LPFIELDINFO lpEditFI;
LPRECORDCORE lpEditRC;

// Margins to use for the check box in the container
#define BOX_LEFT_MARGIN		2
#define BOX_VERT_MARGIN		2

// Margin to use for the text for the check box in the container
#define TEXT_LEFT_MARGIN	3

// Window identifier for the container edit window
#define ID_CNTEDIT         20000

// Width of the edit control border.  Assume 3D controls.
#define EDIT_BORDER_WIDTH  2

// Width of the cursor in the edit control
#define EDIT_CURSOR_WIDTH  1

// Some default values
#define DEFAULT_STRUCT     IDX_ISAM
#define DEFAULT_ALLOCATION 4
#define DEFAULT_EXTEND     16
#define DEFAULT_UNIQUE     IDX_UNIQUE_NO


BOOL CALLBACK  __export DlgChangeStorageStructureDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI __export CntSubclassProc2 (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI __export CntChildSubclassProc2 (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK  __export EnumCntChildProc2 (HWND hwnd, LPARAM lParam);
int CALLBACK   __export DrawFldData2  (HWND   hwndCnt,
                                   LPFIELDINFO  lpFld,
                                   LPRECORDCORE lpRec,
                                   LPVOID lpData,
                                   HDC    hDC,
                                   int    nXstart,
                                   int    nYstart,
                                   UINT   fuOptions,
                                   LPRECT lpRect,
                                   LPSTR  lpszOut,
                                   UINT   uLenOut );
int GetSelectedStructure2(HWND hwnd);
LPSTRUCTCNTLS GetSelectedStructure2Ptr(HWND hwnd);



static void OnDestroy(HWND hwnd);
static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
//static void OnSysColorChange(HWND hwnd);

static void InitialiseSpinControls (HWND hwnd);
static void InitialiseEditControls (HWND hwnd, BOOL bUseDefaults);
static void InitContainer(HWND hwndCtnr);
static BOOL OccupyStructureControl (HWND hwnd);
static BOOL OccupyColumnsControl   (HWND hwnd);

static void DrawCheckBox(HDC hdc, LPRECT lprect, BOOL bSelected, BOOL bGrey);
static void CntOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
static void GetCheckBoxRectFromPt(HWND hwnd, LPFIELDINFO lpFI, LPRECT lprect, POINT pt);
static void CntOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static void EnableControls(HWND hwnd);
static long GetControlDefault(LPSTRUCTCNTLS lpstruct, int nCntl, BOOL bCompressed);
static void Edit_SetDefaultText(HWND hwnd, BOOL bCompressed);

static void CheckColumns        (HWND hwnd, LPOBJECTLIST lpColumn);
static void EnableColumnControl (HWND hwnd, BOOL bEnable);
static void OnDataCompession (HWND hwnd, int id);
static void OnOK (HWND hwnd);
static void OnMultiEditChange (HWND hwnd, HWND hwndCtl, UINT codeNotify);
static void OnColumns (HWND hwnd, HWND hwndCtl, UINT codeNotify);
static void OnStructure (HWND hwnd, HWND hwndCntr, UINT codeNotify);

int WINAPI __export DlgChangeStorageStructure (HWND hwndOwner, LPSTORAGEPARAMS lpcreate)
/*
//	Function:
//		Shows the Create Index dialog
//
//	Paramters:
//		hwndOwner	- Handle of the parent window for the dialog.
//		lpcreate 	- Points to structure containing information used to 
//						- initialise the dialog. The dialog uses the same
//						- structure to return the result of the dialog.
//
//	Returns:
//		TRUE if successful.
//
//	Note:
//		If invalid parameters are passed in they are reset to a default
//		so the dialog can be safely displayed.
*/
{
	int iRetVal;
	FARPROC lpfnDlgProc;

	if (!IsWindow(hwndOwner) || !lpcreate)
	{
		ASSERT(NULL);
		return -1;
	}

	if (!(*lpcreate->DBName) || !(*lpcreate->objectname))
	{
		ASSERT(NULL);
		return -1;
	}

	lpfnDlgProc = MakeProcInstance((FARPROC)DlgChangeStorageStructureDlgProc, hInst);
	iRetVal = VdbaDialogBoxParam (hResource, MAKEINTRESOURCE(IDD_CHANGEST), hwndOwner, (DLGPROC)lpfnDlgProc, (LPARAM)lpcreate);
	FreeProcInstance (lpfnDlgProc);
	return iRetVal;
}


BOOL CALLBACK __export DlgChangeStorageStructureDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);

		case WM_INITDIALOG:
			return HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, OnInitDialog);

		case LM_GETEDITMINMAX:
		{
			LPEDITMINMAX lpdesc = (LPEDITMINMAX) lParam;
			HWND hwndCtl = (HWND) wParam;

			ASSERT(IsWindow(hwndCtl));
			ASSERT(lpdesc);

			return GetEditCtrlMinMaxValue(hwnd, hwndCtl, limits, &lpdesc->dwMin, &lpdesc->dwMax);
		}

		case LM_GETEDITSPINCTRL:
		{
			HWND hwndEdit = (HWND) wParam;
			HWND FAR * lphwnd = (HWND FAR *)lParam;
			*lphwnd = GetEditSpinControl(hwndEdit, limits);
			break;
		}

		default:
			return HandleUserMessages(hwnd, message, wParam, lParam);
	}
	return TRUE;
}


static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	LPSTORAGEPARAMS lpidx = (LPSTORAGEPARAMS)lParam;
	HWND hwndStruct       = GetDlgItem (hwnd, IDC_STRUCT);
	LPSTRUCTCNTLS lpstruct;
	BOOL bKeyCompression = FALSE, bDataCompression = FALSE, bHiDataCompression = FALSE;
	int  nUniqueId, nIdx;
	char szFormat [100];
	char szTitle  [MAX_TITLEBAR_LEN];
	char buffer   [MAXOBJECTNAME];
	LPUCHAR parentstrings [MAXPLEVEL];

	if (!AllocDlgProp(hwnd, lpidx))
		return FALSE;
	//
	// OpenIngres Version 2.0 only
	// Initialize the Combobox of page_size.
	ComboBox_InitPageSize (hwnd, lpidx->uPage_size);
	if (GetOIVers() == OIVERS_12)
	{
		ShowWindow (GetDlgItem (hwnd, IDC_STATIC_PS), SW_HIDE);
		ShowWindow (GetDlgItem (hwnd, IDC_COMBOPAGESIZE), SW_HIDE);
	}
	else
	{
		ShowWindow (GetDlgItem (hwnd, IDC_STATIC_PS), SW_SHOW);
		ShowWindow (GetDlgItem (hwnd, IDC_COMBOPAGESIZE), SW_SHOW);
	}
	parentstrings [0] = lpidx->DBName;
	if (lpidx->nObjectType == OT_TABLE)
	{
		LoadString (hResource, (UINT)IDS_T_TABLE_STRUCT, szFormat, sizeof (szFormat));
		EnableWindow(GetDlgItem (hwnd, IDC_PERSISTENCE), FALSE);
		GetExactDisplayString (
			lpidx->objectname,
			lpidx->objectowner,
			OT_TABLE,
			parentstrings,
			buffer);
		if (GetOIVers() != OIVERS_12)
			lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_OIV2_CHANGEST));
		else
			lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_CHANGEST));
	}
	else
	{
		HWND hwndCheckHiData = GetDlgItem(hwnd, IDC_CHANGEST_HIDATA);
		LoadString	(hResource, (UINT)IDS_T_INDEX_STRUCT, szFormat, sizeof (szFormat));
		GetExactDisplayString (
			lpidx->objectname,
			lpidx->objectowner,
			OT_INDEX,
			parentstrings,
			buffer);
		if (GetOIVers() != OIVERS_12)
			lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_OIV2_CHANGEST_IDX));
		else
			lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_CHANGEST_IDX));
		EnableWindow (hwndCheckHiData, FALSE);
	}
	
	wsprintf (
		szTitle,
		szFormat,
		GetVirtNodeName (GetCurMdiNodeHandle ()),
		lpidx->DBName,
		buffer);
	SetWindowText (hwnd, szTitle);
	XX_MODIFY_TYPE = lpidx->nObjectType;
	
	//
	// Initialise our globals.
	//
	lpfnDrawProc    = NULL;
	lpidxAttr       = NULL;
	uAttrSize;
	uFieldCounter   = 0;
	wndprocCnt      = NULL;
	wndprocCntChild = NULL;
	hwndCntChild    = NULL;
	hwndCntEdit     = NULL;
	dwKeyLen        = 0;
	nCBHeight       = 0;
	nMaxKey         = 0;
	lpEditFI        = NULL;
	lpEditRC        = NULL;

	if (!OccupyStructureControl (hwnd) || !OccupyColumnsControl(hwnd))
	{
		ASSERT(NULL);
		return FALSE;
	}
	//
	// Select the default structure type if none passed in.
	//
	if (lpidx->nStructure > IDX_HEAPSORT || lpidx->nStructure < 1)
		lpidx->nStructure = DEFAULT_STRUCT;
	SelectComboBoxItem(GetDlgItem(hwnd, IDC_STRUCT), typeStructs, lpidx->nStructure);

	EnableControls(hwnd);
	lpstruct = GetSelectedStructure2Ptr(hwnd);

	if (lpidx->nUnique > IDX_UNIQUE_STATEMENT || lpidx->nUnique < 1)
		lpidx->nUnique = DEFAULT_UNIQUE;
	switch (lpidx->nUnique)
	{
	case IDX_UNIQUE_NO:
		nUniqueId = IDC_UNIQUE_NO;
		break;
	case IDX_UNIQUE_ROW:
		nUniqueId = IDC_UNIQUE_ROW;
		break;
	case IDX_UNIQUE_STATEMENT:
		nUniqueId = IDC_UNIQUE_STATEMENT;
		break;
	}
	
	CheckRadioButton(hwnd, IDC_UNIQUE_NO, IDC_UNIQUE_STATEMENT, nUniqueId);
	//
	// Initialise the compression controls
	//
	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_KEY_COMPRESSION)))
	{
		CheckDlgButton(hwnd, IDC_KEY_COMPRESSION, lpidx->bKeyCompression);
		bKeyCompression = lpidx->bKeyCompression;
	}
	
	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_DATA_COMPRESSION)))
	{
		CheckDlgButton(hwnd, IDC_DATA_COMPRESSION, lpidx->bDataCompression);
		bDataCompression = lpidx->bDataCompression;
	}
	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_CHANGEST_HIDATA)))
	{
		CheckDlgButton(hwnd, IDC_CHANGEST_HIDATA, lpidx->bHiDataCompression);
		bDataCompression = bHiDataCompression = lpidx->bHiDataCompression;
	}
	//
	// Ensure the defaults passed in are valid
	//
	if (lpidx->nFillFactor > IDX_MAX_FILLFACTOR || lpidx->nFillFactor < IDX_MIN_FILLFACTOR)
		lpidx->nFillFactor = (int)GetControlDefault(lpstruct, IDC_FILLFACTOR, bDataCompression);

	if (lpidx->nMinPages > IDX_MAX_MINPAGES || lpidx->nMinPages < IDX_MIN_MINPAGES)
		lpidx->nMinPages = (int)GetControlDefault(lpstruct, IDC_MINPAGES, bDataCompression);

	if (lpidx->nMaxPages > IDX_MAX_MAXPAGES || lpidx->nMaxPages < IDX_MIN_MAXPAGES)
		lpidx->nMaxPages = (int)GetControlDefault(lpstruct, IDC_MAXPAGES, bDataCompression);

	if (lpidx->nLeafFill > IDX_MAX_LEAFFILL || lpidx->nLeafFill < IDX_MIN_LEAFFILL)
		lpidx->nLeafFill = (int)GetControlDefault(lpstruct, IDC_LEAFFILL, bDataCompression);

	if (lpidx->nNonLeafFill > IDX_MAX_NONLEAFFILL || lpidx->nNonLeafFill < IDX_MIN_NONLEAFFILL)
		lpidx->nNonLeafFill = (int)GetControlDefault(lpstruct, IDC_NONLEAFFILL, bDataCompression);

	if (lpidx->lAllocation > IDX_MAX_ALLOCATION || lpidx->lAllocation < IDX_MIN_ALLOCATION)
		lpidx->lAllocation = GetControlDefault(lpstruct, IDC_ALLOCATION, bDataCompression);

	if (lpidx->lExtend > IDX_MAX_EXTEND || lpidx->lExtend < IDX_MIN_EXTEND)
		lpidx->lExtend = GetControlDefault(lpstruct, IDC_EXTEND, bDataCompression);
	
	//
	// Subclass the edit controls
	//
	SubclassAllNumericEditControls(hwnd, EC_SUBCLASS);

	InitialiseSpinControls  (hwnd);
	InitialiseEditControls  (hwnd, FALSE);
	InitContainer(GetDlgItem(hwnd, IDC_CONTAINER));

	nIdx = ComboBox_GetCurSel (hwndStruct);
	XX_STRUCT_TYPE = (int)ComboBox_GetItemData (hwndStruct, nIdx);

	if (lpidx->nObjectType == OT_TABLE)
	{
		if (XX_STRUCT_TYPE == IDX_HEAP)
			EnableColumnControl (hwnd, FALSE);
		else
		{
			CheckColumns        (hwnd, lpidx->lpMaintainCols);
		}
	}
	else
	{
		CheckColumns        (hwnd, lpidx->lpidxCols);
		EnableWindow (GetDlgItem (hwnd, IDC_COLUMNS),   FALSE);
		EnableWindow (GetDlgItem (hwnd, IDC_CONTAINER), FALSE);
	}

	// control available only for a Table
	if (GetOIVers() >= OIVERS_30 && lpidx->nObjectType == OT_TABLE)
		ShowWindow (GetDlgItem (hwnd, IDC_CHECK_CONCURRENT_UPDATE), SW_SHOW);
	else
		ShowWindow (GetDlgItem (hwnd, IDC_CHECK_CONCURRENT_UPDATE), SW_HIDE);

	richCenterDialog(hwnd);

	return TRUE;
}


static void OnDestroy(HWND hwnd)
{
	HWND hCntContainer  = GetDlgItem(hwnd,IDC_CONTAINER);
	CntKillRecList(hCntContainer);

	SubclassAllNumericEditControls(hwnd, EC_RESETSUBCLASS);
	ResetSubClassEditCntl(hwnd, IDC_INDEX);
	SubclassWindow(hwndCntChild, wndprocCntChild);
	SubclassWindow(hCntContainer, wndprocCnt);
	if (lpfnDrawProc)
		FreeProcInstance(lpfnDrawProc);
	if (lpidxAttr)
		ESL_FreeMem(lpidxAttr);
	DeallocDlgProp(hwnd);
	lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
	case IDOK:
		OnOK(hwnd);
		break;
	case IDCANCEL:
		EndDialog (hwnd, FALSE);
		break;
	
	case IDC_MINPAGES:
	case IDC_MAXPAGES:
	case IDC_LEAFFILL:
	case IDC_NONLEAFFILL:
	case IDC_FILLFACTOR:
	case IDC_ALLOCATION:
	case IDC_EXTEND:
		OnMultiEditChange (hwnd, hwndCtl, codeNotify);
		break;
	case IDC_COLUMNS:
		OnColumns (hwnd, hwndCtl, codeNotify);
		break;
	case IDC_SPIN_MINPAGES:
	case IDC_SPIN_MAXPAGES:
	case IDC_SPIN_LEAFFILL:
	case IDC_SPIN_NONLEAFFILL:
	case IDC_SPIN_FILLFACTOR:
	case IDC_SPIN_ALLOCATION:
	case IDC_SPIN_EXTEND:
		ProcessSpinControl (hwndCtl, codeNotify, limits);
		break;
	case IDC_STRUCT:
		OnStructure (hwnd, hwndCtl, codeNotify);
		break;
	case IDC_CONTAINER:
		break;
	case IDC_DATA_COMPRESSION:
	case IDC_CHANGEST_HIDATA:
		OnDataCompession (hwnd, id);
		break;
	}
}


/*
static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
	Ctl3dColorChange();
#endif
}*/


static void InitialiseEditControls (HWND hwnd, BOOL bUseDefaults)
{
	char szNumber[20];
	LPSTORAGEPARAMS lpidx = GetDlgProp(hwnd);

	HWND hwndIndex = GetDlgItem(hwnd, IDC_INDEX);
	HWND hwndFillFactor = GetDlgItem(hwnd, IDC_FILLFACTOR);
	HWND hwndMinPages = GetDlgItem(hwnd, IDC_MINPAGES);
	HWND hwndMaxPages = GetDlgItem(hwnd, IDC_MAXPAGES);
	HWND hwndLeafFill = GetDlgItem(hwnd, IDC_LEAFFILL);
	HWND hwndNonLeafFill = GetDlgItem(hwnd, IDC_NONLEAFFILL);
	HWND hwndAllocation = GetDlgItem(hwnd, IDC_ALLOCATION);
	HWND hwndExtend = GetDlgItem(hwnd, IDC_EXTEND);
	LPSTRUCTCNTLS lpstruct = GetSelectedStructure2Ptr(hwnd);

	// Limit the text in the edit controls

	Edit_LimitText(hwndIndex, MAXOBJECTNAME);
	LimitNumericEditControls(hwnd);

	if (!bUseDefaults)
	{
		// Initialise edit controls with strings
	
		Edit_SetText(hwndIndex, lpidx->objectname);
	
		if (IsWindowEnabled(hwndFillFactor))
			my_itoa(lpidx->nFillFactor, szNumber, 10);
		else
			*szNumber = (char)NULL;
		
		Edit_SetText(hwndFillFactor, szNumber);
		
		if (IsWindowEnabled(hwndMinPages))
			my_itoa(lpidx->nMinPages, szNumber, 10);
		else
			*szNumber = (char)NULL;
		
		Edit_SetText(hwndMinPages, szNumber);
		
		if (IsWindowEnabled(hwndMaxPages))
			my_itoa(lpidx->nMaxPages, szNumber, 10);
		else
			*szNumber = (char)NULL;
		
		Edit_SetText(hwndMaxPages, szNumber);
		
		if (IsWindowEnabled(hwndLeafFill))
			my_itoa(lpidx->nLeafFill, szNumber, 10);
		else
			*szNumber = (char)NULL;
		
		Edit_SetText(hwndLeafFill, szNumber);
		
		if (IsWindowEnabled(hwndNonLeafFill))
			my_itoa(lpidx->nNonLeafFill, szNumber, 10);
		else
			*szNumber = (char)NULL;
		
		Edit_SetText(hwndNonLeafFill, szNumber);
		
		if (IsWindowEnabled(hwndAllocation))
			my_dwtoa(lpidx->lAllocation, szNumber, 10);
		else
			*szNumber = (char)NULL;
		
		Edit_SetText(hwndAllocation, szNumber);
		
		if (IsWindowEnabled(hwndExtend))
			my_dwtoa(lpidx->lExtend, szNumber, 10);
		else
			*szNumber = (char)NULL;
		Edit_SetText(hwndExtend, szNumber);
	}
	else
		Edit_SetDefaultText(hwnd, IsDlgButtonChecked(hwnd, IDC_DATA_COMPRESSION));
}



static void InitialiseSpinControls (HWND hwnd)
{
	DWORD dwMin, dwMax;
	LPSTORAGEPARAMS lpidx = GetDlgProp(hwnd);

	GetEditCtrlMinMaxValue(hwnd, GetDlgItem(hwnd, IDC_FILLFACTOR), limits, &dwMin, &dwMax);
	SpinRangeSet(GetDlgItem(hwnd, IDC_SPIN_FILLFACTOR), dwMin, dwMax);

	GetEditCtrlMinMaxValue(hwnd, GetDlgItem(hwnd, IDC_MINPAGES), limits, &dwMin, &dwMax);
	SpinRangeSet(GetDlgItem(hwnd, IDC_SPIN_MINPAGES), dwMin, dwMax);

	GetEditCtrlMinMaxValue(hwnd, GetDlgItem(hwnd, IDC_MAXPAGES), limits, &dwMin, &dwMax);
	SpinRangeSet(GetDlgItem(hwnd, IDC_SPIN_MAXPAGES), dwMin, dwMax);

	GetEditCtrlMinMaxValue(hwnd, GetDlgItem(hwnd, IDC_LEAFFILL), limits, &dwMin, &dwMax);
	SpinRangeSet(GetDlgItem(hwnd, IDC_SPIN_LEAFFILL), dwMin, dwMax);

	GetEditCtrlMinMaxValue(hwnd, GetDlgItem(hwnd, IDC_NONLEAFFILL), limits, &dwMin, &dwMax);
	SpinRangeSet(GetDlgItem(hwnd, IDC_SPIN_NONLEAFFILL), dwMin, dwMax);

	GetEditCtrlMinMaxValue(hwnd, GetDlgItem(hwnd, IDC_ALLOCATION), limits, &dwMin, &dwMax);
	SpinRangeSet(GetDlgItem(hwnd, IDC_SPIN_ALLOCATION), dwMin, dwMax);

	GetEditCtrlMinMaxValue(hwnd, GetDlgItem(hwnd, IDC_EXTEND), limits, &dwMin, &dwMax);
	SpinRangeSet(GetDlgItem(hwnd, IDC_SPIN_EXTEND), dwMin, dwMax);
}



static BOOL OccupyStructureControl (HWND hwnd)
/*
// Function:
// Fills the structure drop down box with the structure names.
//
// Parameters:
//     hwnd - Handle to the dialog window.
//
// Returns:
//     TRUE if successful.
*/
{
	HWND hwndCtl          = GetDlgItem (hwnd, IDC_STRUCT);
	LPSTORAGEPARAMS lpidx = GetDlgProp (hwnd);


	if (lpidx->nObjectType == OT_TABLE)
		return OccupyComboBoxControl (hwndCtl, typeStructs );
	else
		return OccupyComboBoxControl (hwndCtl, typeStructs2);
}


static BOOL OccupyColumnsControl (HWND hwnd)
/*
// Function:
//     Fills the columns CA list box with the column names.
// Parameters:
//     hwnd  -Handle to the dialog window.
// Returns:
//     TRUE if successful.
*/
{
	LPOBJECTLIST    ls;
	HWND            hwndCtl = GetDlgItem (hwnd, IDC_COLUMNS);
	LPSTORAGEPARAMS lpidx   = GetDlgProp (hwnd);
	
	ls = lpidx->lpidxCols;
	while (ls)
	{
		LPIDXCOLS   lpObj = ls->lpObject;
		char*       item  = lpObj->szColName;
		CAListBox_AddString (hwndCtl, item);
		
		ls = ls->lpNext;
	}
	return TRUE;
}


static void InitContainer(HWND hwnd)
/*
	Function:
		Performs the necessary initialisation for the container window.

	Parameters:
		hwnd	- Handle to the container window.

	Returns:
		None.
*/
{
	HFONT hFont, hOldFont;
	HDC hdc = GetDC(hwnd);
	TEXTMETRIC tm;
	int nRowHeight;
	RECT rect;

	// Defer painting until done changing container attributes.
	CntDeferPaint( hwnd );

	hFont = GetWindowFont(GetParent(hwnd));
	hOldFont = SelectObject(hdc, hFont);
	GetTextMetrics(hdc, &tm);

	// Init the vertical scrolling range to 0 since we have no records.
	CntRangeExSet( hwnd, 0, 0 );

	// Set the container view.
	CntViewSet( hwnd, CV_DETAIL );

	// Set the container colors.
	CntColorSet( hwnd, CNTCOLOR_CNTBKGD,    RGB(255, 255, 255) );
	CntColorSet( hwnd, CNTCOLOR_FLDTITLES,  RGB(  0,   0,   0) );
	CntColorSet( hwnd, CNTCOLOR_TEXT,       RGB(  0,   0,   0) );
	CntColorSet( hwnd, CNTCOLOR_GRID,       RGB(  0,   0,   0) );

	// Set some general container attributes.
	CntAttribSet(hwnd, CA_MOVEABLEFLDS | /*CA_TITLE3D |*/ CA_FLDTTL3D);

	// Calculate row height
	// Height of check box plus margins
	nCBHeight = tm.tmHeight - 1;
	nRowHeight = tm.tmHeight + (2 * BOX_VERT_MARGIN);

	// Add in the height of an edit box
	GetClientRect(GetDlgItem(GetParent(hwnd), IDC_NONLEAFFILL), &rect);
	nRowHeight += (rect.bottom - rect.top);

	CntRowHtSet(hwnd, -nRowHeight, 0);

	// Define space for container field titles.
	CntFldTtlHtSet(hwnd, 1);
	CntFldTtlSepSet(hwnd);

	// Set up some cursors for the different areas.
	CntCursorSet( hwnd, LoadCursor(NULL, IDC_UPARROW ), CC_FLDTITLE );

	// Set some fonts for the different container areas.
	CntFontSet( hwnd, hFont, CF_GENERAL );

	// Initialise the drawing procedure for our owner draw fields.
	lpfnDrawProc = (DRAWPROC)MakeProcInstance((FARPROC)DrawFldData2, hInst);
	
	uAttrSize = sizeof(IDXATTR);
	lpidxAttr = ESL_AllocMem(uAttrSize);

	// Now paint the container.
	CntEndDeferPaint( hwnd, TRUE );

	// Subclass the container window so we can receive messages from the
	// edit control
	wndprocCnt = SubclassWindow(hwnd, CntSubclassProc2);
	
	// Subclass the container child window so we can operate the check boxes.
	EnumChildWindows(hwnd, EnumCntChildProc2, (LPARAM)(HWND FAR *)&hwndCntChild);
	wndprocCntChild = SubclassWindow(hwndCntChild, CntChildSubclassProc2);
	
	SelectObject(hdc, hOldFont);
	ReleaseDC(hwnd, hdc);
}


BOOL CALLBACK __export EnumCntChildProc2(HWND hwnd, LPARAM lParam)
{
	HWND FAR * lphwnd = (HWND FAR *)lParam;
	char szClassName[100];
	
	GetClassName(hwnd, szClassName, sizeof(szClassName));
	
	if (lstrcmpi(szClassName, CONTAINER_CHILD) == 0)
	{
		*lphwnd = hwnd;
		return FALSE;
	}
	
	return TRUE;
}


LRESULT WINAPI __export CntSubclassProc2(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		HANDLE_MSG(hwnd, WM_COMMAND, CntOnCommand);
	}
	return (LRESULT)CallWindowProc(wndprocCnt, hwnd, message, wParam, lParam);
}


LRESULT WINAPI __export CntChildSubclassProc2(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lret;

	lret = CallWindowProc(wndprocCntChild, hwnd, message, wParam, lParam);

	switch (message)
	{
		HANDLE_MSG(hwnd, WM_LBUTTONDOWN, CntOnLButtonDown);
		HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK, CntOnLButtonDown);
	}
	
	return lret;
}


int CALLBACK __export DrawFldData2( HWND   hwndCnt,     // container window handle
                                   LPFIELDINFO  lpFld, // field being drawn
                                   LPRECORDCORE lpRec, // record being drawn
                                   LPVOID lpData,      // data in internal format
                                   HDC    hDC,         // container window DC
                                   int    nXstart,     // starting position Xcoord
                                   int    nYstart,     // starting position Ycoord
                                   UINT   fuOptions,   // rectangle type
                                   LPRECT lpRect,      // rectangle of data cell
                                   LPSTR  lpszOut,     // data in string format
                                   UINT   uLenOut )    // len of lpszOut buffer
{
	LPIDXATTR lpattr = (LPIDXATTR)lpData;
	TEXTMETRIC tm;
	RECT rLine1;
	RECT rLine2;
	BOOL bGrey;
	
	GetTextMetrics(hDC, &tm);
	
	rLine1.left   = lpRect->left;
	rLine1.top    = lpRect->top;
	rLine1.right  = lpRect->right;
	rLine1.bottom = lpRect->top + nCBHeight + (2 * BOX_VERT_MARGIN);
	
	rLine2.left   = lpRect->left;
	rLine2.top    = rLine1.bottom + 1;
	rLine2.right  = lpRect->right;
	rLine2.bottom = lpRect->bottom;
	bGrey = FALSE;
	
	if ((XX_MODIFY_TYPE == OT_TABLE) && (XX_STRUCT_TYPE == IDX_HEAPSORT))
		DrawCheckBox(hDC, lpRect, lpattr->bKey, bGrey);
	return 1;
}


static void DrawCheckBox(HDC hdc, LPRECT lprect, BOOL bSelected, BOOL bGrey)
/*
   Function:
      Draws the check box in the container field. The check box
      is drawn in the first line of the record.

   Parameters:
      hdc         - Device context for container field.
      lprect      - Drawing rectangle for container field.
      bSelected   - If TRUE then check box is to be selected.
      bGrey       - Draw text greyed out.

   Returns:
      None.
*/
{
	RECT rect;
	char szText[50];
	COLORREF crText;

	rect.left   = lprect->left + BOX_LEFT_MARGIN;
	rect.top    = lprect->top  + BOX_VERT_MARGIN;
	rect.right  = rect.left + nCBHeight + 1;
	rect.bottom = rect.top + nCBHeight + 1;

	Rectangle3D(hdc, rect.left, rect.top, rect.right, rect.bottom);

	if (bSelected)
	DrawCheck3D(hdc, rect.left, rect.top, rect.right, rect.bottom, bGrey);

	// Draw the text associated with the check box.
	
	rect.left = rect.right + TEXT_LEFT_MARGIN;
	rect.right = lprect->right;
	
	if (XX_MODIFY_TYPE == OT_INDEX)
		LoadString(hResource, IDS_KEY, szText, sizeof(szText));
	else
		LoadString(hResource, (UINT)IDS_I_DESCENDING,  szText, sizeof(szText));
	
	rect.top--;
	
	if (bGrey)
		crText = SetTextColor(hdc, DARK_GREY);
	
	DrawText(hdc,
		szText,
		lstrlen(szText),
		&rect,
		DT_LEFT | DT_SINGLELINE | DT_BOTTOM);
	
	if (bGrey)
		SetTextColor(hdc, crText);
}


static void CntOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
	}
}


static void CntOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	POINT ptMouse;
	LPFIELDINFO lpFI;
	LPRECORDCORE lpRC;
	HWND hwndCnt = GetParent(hwnd);
	
	ptMouse.x = x;
	ptMouse.y = y;
	
	if (lpFI = CntFldAtPtGet(hwndCnt, &ptMouse))
	{
		if (lpRC = CntRecAtPtGet(hwndCnt, &ptMouse))
		{
			RECT rect;
			IDXATTR attr;
			LPCNTINFO lpcntInfo = CntInfoGet(hwndCnt);
			
			// Get the data for this cell
			CntFldDataGet(lpRC, lpFI, sizeof(attr), &attr);
			GetCheckBoxRectFromPt(hwndCnt, lpFI, &rect, ptMouse);
			
			if (PtInRect(&rect, ptMouse))
			{
				// Toggle the selection flag.
				attr.bKey = !attr.bKey;
				CntFldDataSet(lpRC, lpFI, sizeof(attr), &attr);
				// Force this cell to repaint
				CntUpdateRecArea(hwndCnt, lpRC, lpFI);
			}
		}
	}
}

static void GetCheckBoxRectFromPt(HWND hwnd, LPFIELDINFO lpFI, LPRECT lprect, POINT pt)
{
	LPCNTINFO lpcntInfo = CntInfoGet(hwnd);
	
	// Calculate area of first line in entire record
	lprect->top    = lpcntInfo->cyTitleArea + lpcntInfo->cyColTitleArea + BOX_VERT_MARGIN;
	lprect->left   = BOX_LEFT_MARGIN;
	lprect->right  = lpFI->cxPxlWidth - 1;
	lprect->bottom = lprect->top + nCBHeight - 1;
	
	while (lprect->right < pt.x)
	{
		lprect->left  += lpFI->cxPxlWidth;
		lprect->right += lpFI->cxPxlWidth;
	}
}




static void EnableControls(HWND hwnd)
/*
   Function:
      Enables/disables the controls needed for the current structure type.

   Parameters:
      hwnd  - Handle to the dialog window

   Returns:
      None.
*/
{
	LPSTRUCTCNTLS lpstruct = GetSelectedStructure2Ptr(hwnd);
	int i;
	
	if (!lpstruct)
		return;
	
	for (i = 0; i < CTL_ENABLE; i++)
	{
		HWND hwndCtl = GetDlgItem(hwnd, lpstruct->cntl[i].nCntlId);
		EnableWindow(hwndCtl, lpstruct->cntl[i].bEnabled);
	}
}


int GetSelectedStructure2(HWND hwnd)
{
	HWND hwndCtl = GetDlgItem (hwnd, IDC_STRUCT);
	int  nIdx       = ComboBox_GetCurSel (hwndCtl);
	int  k          = (int) ComboBox_GetItemData (hwndCtl, nIdx);

	return k;
}



LPSTRUCTCNTLS GetSelectedStructure2Ptr(HWND hwnd)
{
	LPSTORAGEPARAMS lpidx = GetDlgProp(hwnd);
	int nStruct = GetSelectedStructure2 (hwnd);
	LPSTRUCTCNTLS lpstruct = (lpidx->nObjectType == OT_TABLE)?cntlEnable: cntlEnable2;
	
	// Find the current structure
	while (lpstruct->nStructType != -1)
	{
		if (lpstruct->nStructType == nStruct)
			return lpstruct;
		lpstruct++;
	}
	
	ASSERT(NULL);
	return NULL;
}


static long GetControlDefault(LPSTRUCTCNTLS lpstruct, int nCntl, BOOL bCompressed)
{
	int i;
	
	switch (nCntl)
	{
	case IDC_STRUCT:
		return DEFAULT_STRUCT;
	case IDC_ALLOCATION:
		return DEFAULT_ALLOCATION;
	case IDC_EXTEND:
		return DEFAULT_EXTEND;
	}
	
	for (i = 0; i < CTL_ENABLE; i++)
	{
		if (lpstruct->cntl[i].nCntlId == nCntl)
			return (bCompressed ? lpstruct->cntl[i].lCDefault : lpstruct->cntl[i].lDefault);
	}
	
	return -1;
}


static void Edit_SetDefaultText(HWND hwnd, BOOL bCompressed)
{
	int nOffset = 0;
	char szNumber[20];
	LPSTRUCTCNTLS lpstruct = GetSelectedStructure2Ptr(hwnd);
	
	while (limits[nOffset].nEditId != -1)
	{
		HWND hwndCtl = GetDlgItem(hwnd, limits[nOffset].nEditId);
		
		if (IsWindowEnabled(hwndCtl))
			my_dwtoa(GetControlDefault(lpstruct, limits[nOffset].nEditId, bCompressed), szNumber, 10);
		else
			*szNumber = (char)NULL;
		
		Edit_SetText(hwndCtl, szNumber);
		nOffset++;
	}
}

static void CheckColumns (HWND hwnd, LPOBJECTLIST lpColumn)
{
	LPSTORAGEPARAMS lpidx = GetDlgProp(hwnd);
	HWND  hwndCtl   = GetDlgItem (hwnd, IDC_COLUMNS);
	HWND  hwndCntr  = GetDlgItem (hwnd, IDC_CONTAINER);
	LPIDXCOLS Order [MAXIMUM_COLUMNS];
	int   i;
	LPIDXCOLS    lpCol;
	LPOBJECTLIST ls;
	
	LPSTORAGEPARAMS lpstorage = GetDlgProp (hwnd);
	BOOL         bKey;
	char*        item;
	LPFIELDINFO  lpFI;
	UINT         uNewAttrSize;
	LPIDXATTR    lpThisIdxAttr;
	LPRECORDCORE lpRC;
	
	
	for (i=0; i<MAXIMUM_COLUMNS; i++)
		Order[i] = NULL;
	
	ls = lpColumn;
	while (ls)
	{
		lpCol = (LPIDXCOLS)ls->lpObject;
		if (lpCol->attr.bKey)
		{
			int nIndex = CAListBox_FindStringExact (hwndCtl, -1, (char *)lpCol->szColName);
			CAListBox_SetSel (hwndCtl, TRUE, nIndex);
			Order [lpCol->attr.nPrimary] = lpCol;
		}
		
		ls = ls->lpNext;
	}
	
	
	i=1;
	while (Order[i])
	{
		lpCol = (LPIDXCOLS)Order[i]; //->lpObject;
		item  = (char*) lpCol->szColName;
		bKey  = lpCol->attr.bKey;
		
		//
		// Add the column to the container
		//
		
		uFieldCounter++;
		
		// Allocate enough memory for this fields data. Since each
		// field is given a specific offset into the allocated
		// memory (see wOffStruct), the memory for the new field
		// is always tagged onto the end of the current block,
		// regardless of any deleted fields.
		
		uNewAttrSize = uFieldCounter * sizeof(IDXATTR);
		if (uNewAttrSize > uAttrSize)
		{
		    lpidxAttr = ESL_ReAllocMem(lpidxAttr, uNewAttrSize, uAttrSize);
		    uAttrSize = uNewAttrSize;
		}
		//
		// Copy the record data into our buffer
		//
		if (lpRC = CntRecHeadGet(hwndCntr))
		    CntRecDataGet(hwndCntr, lpRC, (LPVOID)lpidxAttr);
		//
		// Initialise the data for this field.
		//
		lpThisIdxAttr = lpidxAttr + (uFieldCounter - 1);
		
		// modified ps+emb 29/11/96
		//if (lpstorage->nObjectType == OT_TABLE)
		//    lpThisIdxAttr->bKey = FALSE;
		//else
		//    lpThisIdxAttr->bKey = bKey;
		if (lpstorage->nObjectType == OT_TABLE && lpstorage->nStructure == IDX_HEAPSORT)
		  lpThisIdxAttr->bKey = lpCol->attr.bDescending;
		else
		  lpThisIdxAttr->bKey = FALSE;
		
		//
		// Initialise the field descriptor.
		//
		lpFI = CntNewFldInfo();
		lpFI->wIndex       = uFieldCounter;
		lpFI->cxWidth      = 16;
		lpFI->flFDataAlign = CA_TA_LEFT | CA_TA_VCENTER;
		lpFI->wColType     = CFT_CUSTOM;
		lpFI->flColAttr    = CFA_OWNERDRAW;
		lpFI->wDataBytes   = sizeof(IDXATTR);
		lpFI->wOffStruct   = (uFieldCounter - 1) * sizeof(IDXATTR);
		
		CntFldTtlSet(hwndCntr, lpFI, item, lstrlen (item) + 1);
		CntFldColorSet(hwndCntr, lpFI, CNTCOLOR_FLDBKGD, CntFldColorGet (hwndCntr, lpFI, CNTCOLOR_FLDTTLBKGD));
		lpFI->flFTitleAlign = CA_TA_HCENTER | CA_TA_VCENTER;
		CntFldDrwProcSet (lpFI, lpfnDrawProc);
		//
		// Add the field
		//
		CntAddFldTail(hwndCntr, lpFI);
		
		// Add the new record to the container
		
		CntKillRecList      (hwndCntr);
		lpRC = CntNewRecCore(uAttrSize);
		CntRecDataSet       (lpRC, (LPVOID)lpidxAttr);
		CntAddRecHead       (hwndCntr, lpRC);
		i++;
	}
}


static void EnableColumnControl (HWND hwnd, BOOL bEnable)
{
	HWND  hwndCtl   = GetDlgItem (hwnd, IDC_COLUMNS);
	HWND  hwndCntr  = GetDlgItem (hwnd, IDC_CONTAINER);
	int	 n, i;
	BOOL  bSelected;
	char  szColumn [MAXOBJECTNAME];
	
	CntDeferPaint(hwndCntr);
	n = CAListBox_GetCount (hwndCtl);
	for (i=0; i<n; i++)
	{
		bSelected = CAListBox_GetSel (hwndCtl, i);
		if (bSelected)
		{
			LPFIELDINFO lpField = CntFldHeadGet (hwndCntr);
			
			ZEROINIT (szColumn);
			CAListBox_GetText (hwndCtl, i, szColumn);
			CAListBox_SetSel  (hwndCtl, FALSE, i);
			
			//
			// Remove the field from the container
			//
			do
			{
				if (!lstrcmpi (lpField->lpFTitle, szColumn))
				{
					CntRemoveFld(hwndCntr, lpField);
					break;
				}
			}
			while (lpField = CntNextFld(lpField));
		}
	}
	EnableWindow (hwndCtl , bEnable);
	EnableWindow (hwndCntr, bEnable);
	CntEndDeferPaint (hwndCntr, TRUE);
}


static void ComboBox_InitPageSize (HWND hwnd, LONG Page_size)
{
	int i, index;
	typedef struct tagCxU
	{
		char* cx;
		LONG  ux;
	} CxU;
	CxU pageSize [] =
	{
		{" 2K", 2048L},
		{" 4K", 4096L},
		{" 8K", 8192L},
		{"16K",16384L},
		{"32K",32768L},
		{"64K",65536L}
	};
	HWND hwndCombo = GetDlgItem (hwnd, IDC_COMBOPAGESIZE);
	for (i=0; i<6; i++)
	{
		index = ComboBox_AddString (hwndCombo, pageSize [i].cx);
		if (index != -1)
			ComboBox_SetItemData (hwndCombo, index, pageSize [i].ux);
	}
	ComboBox_SetCurSel(hwndCombo, 0);
	for (i=0; i<6; i++)
	{
		if (pageSize [i].ux == Page_size)
		{
			ComboBox_SetCurSel(hwndCombo, i);
			break;
		}
	}
}

static void OnDataCompession (HWND hwnd, int id)
{
	HWND hwndCompressData	= GetDlgItem (hwnd, IDC_DATA_COMPRESSION);
	HWND hwndCompressDataHi = GetDlgItem (hwnd, IDC_CHANGEST_HIDATA);
	switch (id)
	{
	case IDC_DATA_COMPRESSION:
		if (IsDlgButtonChecked(hwnd, IDC_DATA_COMPRESSION))
			Button_SetCheck (hwndCompressDataHi,  FALSE);
		break;
	case IDC_CHANGEST_HIDATA:
		if (IsDlgButtonChecked(hwnd, IDC_CHANGEST_HIDATA))
			Button_SetCheck (hwndCompressData,	FALSE);
		break;
	}
}

static void OnOK (HWND hwnd)
{
	char szNumber[20];
	int  nIdx;
	LPOBJECTLIST lpList = NULL;
	LPRECORDCORE lpRC;
	LPFIELDINFO lpFld;
	IDXATTR attr;
	int primKeyNumber = 0;
	LPIDXCOLS    lpObj;
	LPOBJECTLIST lpTemp;

	HWND hwndCombo = GetDlgItem (hwnd, IDC_COMBOPAGESIZE);
	HWND hwndStruct     = GetDlgItem(hwnd, IDC_STRUCT);
	HWND hwndIndex      = GetDlgItem(hwnd, IDC_INDEX);
	HWND hwndFillFactor = GetDlgItem(hwnd, IDC_FILLFACTOR);
	HWND hwndMinPages   = GetDlgItem(hwnd, IDC_MINPAGES);
	HWND hwndMaxPages   = GetDlgItem(hwnd, IDC_MAXPAGES);
	HWND hwndLeafFill   = GetDlgItem(hwnd, IDC_LEAFFILL);
	HWND hwndNonLeafFill= GetDlgItem(hwnd, IDC_NONLEAFFILL);
	HWND hwndAllocation = GetDlgItem(hwnd, IDC_ALLOCATION);
	HWND hwndExtend     = GetDlgItem(hwnd, IDC_EXTEND);
	HWND hwndLocations  = GetDlgItem(hwnd, IDC_LOCATION);
	HWND hwndCnt        = GetDlgItem(hwnd, IDC_CONTAINER);
	LPSTORAGEPARAMS lpidx = GetDlgProp(hwnd);
	
	if (!VerifyAllNumericEditControls (hwnd, TRUE, TRUE))
		return;
	//
	// Open Ingres Version 2.0 or Later
	// Get the page size.
	nIdx = ComboBox_GetCurSel (hwndCombo);
	if (nIdx != -1)
	{
		lpidx->uPage_size = (LONG)ComboBox_GetItemData (hwndCombo, nIdx);
		if (lpidx->uPage_size == 2048L)
			lpidx->uPage_size = 0L;
	}
	
	nIdx = ComboBox_GetCurSel(hwndStruct);
	lpidx->nStructure = (int)ComboBox_GetItemData(hwndStruct, nIdx);
	
	Edit_GetText(hwndFillFactor, szNumber, sizeof(szNumber));
	lpidx->nFillFactor = my_atoi(szNumber);
	
	Edit_GetText(hwndMinPages, szNumber, sizeof(szNumber));
	lpidx->nMinPages = my_atoi(szNumber);
	
	Edit_GetText(hwndMaxPages, szNumber, sizeof(szNumber));
	lpidx->nMaxPages = my_atoi(szNumber);
	
	Edit_GetText(hwndLeafFill, szNumber, sizeof(szNumber));
	lpidx->nLeafFill = my_atoi(szNumber);
	
	Edit_GetText(hwndNonLeafFill, szNumber, sizeof(szNumber));
	lpidx->nNonLeafFill = my_atoi(szNumber);
	
	Edit_GetText(hwndAllocation, szNumber, sizeof(szNumber));
	lpidx->lAllocation = my_atoi(szNumber);
	
	Edit_GetText(hwndExtend, szNumber, sizeof(szNumber));
	lpidx->lExtend = my_atoi(szNumber);
	
	lpidx->bKeyCompression = IsDlgButtonChecked(hwnd, IDC_KEY_COMPRESSION);
	lpidx->bDataCompression = IsDlgButtonChecked(hwnd, IDC_DATA_COMPRESSION);
	lpidx->bHiDataCompression = IsDlgButtonChecked(hwnd, IDC_CHANGEST_HIDATA);
	if (IsDlgButtonChecked(hwnd, IDC_UNIQUE_NO))
		lpidx->nUnique = IDX_UNIQUE_NO;
	else if (IsDlgButtonChecked(hwnd, IDC_UNIQUE_ROW))
		lpidx->nUnique = IDX_UNIQUE_ROW;
	else if (IsDlgButtonChecked(hwnd, IDC_UNIQUE_STATEMENT))
		lpidx->nUnique = IDX_UNIQUE_STATEMENT;
	
	lpidx->bPersistence = IsDlgButtonChecked(hwnd, IDC_PERSISTENCE);
	
	//
	// Work through the container control and create a linked list
	// of the selected columns and their attributes.
	//
	
	lpRC   = CntRecHeadGet (hwndCnt);
	
	// Get field count, for primary key
	lpFld  = CntFldTailGet (hwndCnt);
	while (lpFld) {
		primKeyNumber++;
		lpFld = CntPrevFld(lpFld);
	}
	
	lpFld  = CntFldTailGet (hwndCnt);
	while (lpFld)
	{
		lpTemp = AddListObject(lpList, sizeof(IDXCOLS));
		if (!lpTemp)
		{
			// Need to free previously allocated objects.
			FreeObjectList (lpList);
			return;
		}
		else
			lpList = lpTemp;
		
		lpObj = (LPIDXCOLS)lpList->lpObject;
		
		CntFldDataGet(lpRC, lpFld, sizeof(attr), &attr);
		lpObj->attr.bKey = TRUE;
		if ((lpidx->nObjectType == OT_TABLE) && (lpidx->nStructure == IDX_HEAPSORT))
			lpObj->attr.bDescending = attr.bKey; // bKey is used for desc
		
		lstrcpy(lpObj->szColName, lpFld->lpFTitle);
		lpObj->attr.nPrimary = primKeyNumber--;
		lpFld = CntPrevFld(lpFld);
	}
	FreeObjectList (lpidx->lpMaintainCols);
	lpidx->lpMaintainCols = lpList;

	if (GetOIVers() >= OIVERS_30 && lpidx->nObjectType == OT_TABLE)
		lpidx->bConcurrentUpdates = IsDlgButtonChecked(hwnd, IDC_CHECK_CONCURRENT_UPDATE);
	else
		lpidx->bConcurrentUpdates = FALSE;
	EndDialog (hwnd, TRUE);
}

static void OnMultiEditChange (HWND hwnd, HWND hwndCtl, UINT codeNotify)
{
	int nCount;
	HWND hwndNew;
	int nNewCtl;
	switch (codeNotify)
	{
	case EN_CHANGE:
		if (!IsWindowVisible(hwnd))
			break;
		// Test to see if the control is empty. If so then
		// break out.  It becomes tiresome to edit
		// if you delete all characters and an error box pops up.
		nCount = Edit_GetTextLength(hwndCtl);
		if (nCount == 0)
			break;
		VerifyAllNumericEditControls(hwnd, TRUE, TRUE);
		break;
	case EN_KILLFOCUS:
		hwndNew = GetFocus();
		nNewCtl = GetDlgCtrlID(hwndNew);
		if (nNewCtl == IDCANCEL|| nNewCtl == IDOK || !IsChild(hwnd, hwndNew))
			// Dont verify on any button hits
			break;
		if (!VerifyAllNumericEditControls(hwnd, TRUE, TRUE))
			break;
		UpdateSpinButtons(hwnd);
		break;
	default:
		break;
	}
}


static void OnColumns (HWND hwnd, HWND hwndCtl, UINT codeNotify)
{
	int nIdx;
	BOOL bSelected;
	char szColumn[MAXOBJECTNAME];
	HWND hwndCntr;

	switch (codeNotify)
	{
	case CALBN_CHECKCHANGE:
		hwndCntr = GetDlgItem(hwnd, IDC_CONTAINER);
		ASSERT(lpidxAttr);
		if (!lpidxAttr)
			break;
		nIdx = CAListBox_GetCurSel(hwndCtl);
		bSelected = CAListBox_GetSel(hwndCtl, nIdx);
		CAListBox_GetText(hwndCtl, nIdx, szColumn);
		CntDeferPaint(hwndCntr);
		if (bSelected)
		{
			//
			// Add the column to the container
			LPFIELDINFO lpFI;
			UINT uNewAttrSize;
			LPIDXATTR lpThisIdxAttr;
			LPRECORDCORE lpRC;
	
			uFieldCounter++;
			// Allocate enough memory for this fields data. Since each
			// field is given a specific offset into the allocated
			// memory (see wOffStruct), the memory for the new field
			// is always tagged onto the end of the current block,
			// regardless of any deleted fields.
			uNewAttrSize = uFieldCounter * sizeof(IDXATTR);
			if (uNewAttrSize > uAttrSize)
			{
				lpidxAttr = ESL_ReAllocMem(lpidxAttr, uNewAttrSize, uAttrSize);
				uAttrSize = uNewAttrSize;
			}
			//
			// Copy the record data into our buffer
			if (lpRC = CntRecHeadGet(hwndCntr))
				CntRecDataGet(hwndCntr, lpRC, (LPVOID)lpidxAttr);
			//
			// Initialise the data for this field.
			lpThisIdxAttr = lpidxAttr + (uFieldCounter - 1);
			lpThisIdxAttr->bKey = FALSE;
			//
			// Initialise the field descriptor.
			lpFI = CntNewFldInfo();
			lpFI->wIndex	   = uFieldCounter;
			lpFI->cxWidth	   = 16;
			lpFI->flFDataAlign = CA_TA_LEFT | CA_TA_VCENTER;
			lpFI->wColType	   = CFT_CUSTOM;
			lpFI->flColAttr    = CFA_OWNERDRAW;
			lpFI->wDataBytes   = sizeof(IDXATTR);
			lpFI->wOffStruct   = (uFieldCounter - 1) * sizeof(IDXATTR);
	
			CntFldTtlSet   (hwndCntr, lpFI, szColumn, lstrlen(szColumn) + 1);
			CntFldColorSet (hwndCntr, lpFI, CNTCOLOR_FLDBKGD, CntFldColorGet (hwndCntr, lpFI, CNTCOLOR_FLDTTLBKGD));
			lpFI->flFTitleAlign = CA_TA_HCENTER | CA_TA_VCENTER;
			CntFldDrwProcSet(lpFI, lpfnDrawProc);
	
			// Add the field
			CntAddFldTail(hwndCntr, lpFI);
			// Add the new record to the container
			CntKillRecList(hwndCntr);
			lpRC = CntNewRecCore(uAttrSize);
			CntRecDataSet(lpRC, (LPVOID)lpidxAttr);
			CntAddRecHead(hwndCntr, lpRC);
		}
		else
		{
			//
			// Remove the field from the container
			//
			LPFIELDINFO lpField = CntFldHeadGet(hwndCntr);
			do
			{
				if (!lstrcmpi(lpField->lpFTitle, szColumn))
				{
					CntRemoveFld(hwndCntr, lpField);
					break;
				}
			}
			while (lpField = CntNextFld(lpField));
		}
		CntEndDeferPaint(hwndCntr, TRUE);
		break;
	}
}

static void OnStructure (HWND hwnd, HWND hwndCtl, UINT codeNotify)
{
	int  nIdx;
	HWND hwndStruct = GetDlgItem (hwnd, IDC_STRUCT);
	HWND hwndCntr   = GetDlgItem (hwnd, IDC_CONTAINER);
	HWND hwndCompressData	= GetDlgItem (hwnd, IDC_DATA_COMPRESSION);
	HWND hwndCompressDataHi = GetDlgItem (hwnd, IDC_CHANGEST_HIDATA);
	HWND hwndCompressKey    = GetDlgItem (hwnd, IDC_KEY_COMPRESSION);

	LPSTORAGEPARAMS lpidx = GetDlgProp(hwnd);
	switch (codeNotify)
	{
	case CBN_SELCHANGE:
		EnableControls (hwnd);
		InitialiseSpinControls (hwnd);
		InitialiseEditControls (hwnd, TRUE);
		CntDeferPaint(hwndCntr);
		nIdx = ComboBox_GetCurSel (hwndStruct);
		XX_STRUCT_TYPE = (int)ComboBox_GetItemData(hwndStruct, nIdx);
		if ((XX_STRUCT_TYPE == IDX_HEAP) || (XX_STRUCT_TYPE == IDX_HEAPSORT))
		{
			Button_SetCheck (GetDlgItem (hwnd, IDC_UNIQUE_ROW), FALSE);
			Button_SetCheck (GetDlgItem (hwnd, IDC_UNIQUE_STATEMENT), FALSE);
			Button_SetCheck (GetDlgItem (hwnd, IDC_UNIQUE_NO), TRUE);
		}
	
		if (lpidx->nObjectType == OT_TABLE)
		{
			if (XX_STRUCT_TYPE == IDX_HEAP)
				EnableColumnControl (hwnd, FALSE);
			else
			{
				EnableColumnControl (hwnd, TRUE);
				CheckColumns (hwnd, lpidx->lpMaintainCols);
			}
		}
		CntEndDeferPaint (hwndCntr, TRUE);
		SetFocus (hwndCtl);
		Button_SetCheck (hwndCompressDataHi,  FALSE);
		Button_SetCheck (hwndCompressData, FALSE);
		Button_SetCheck (hwndCompressKey, FALSE);
		break;
	
	case CBN_KILLFOCUS:
		// We must verify the edit controls on exiting the
		// structure field since we dont do any checking when
		// it gets the focus,  therefore an edit control could
		// be invalid.
	
		VerifyAllNumericEditControls(hwnd, TRUE, TRUE);
		break;
	default:
		break;
	}
}
