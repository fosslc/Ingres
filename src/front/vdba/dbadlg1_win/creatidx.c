/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project  : CA/OpenIngres Visual DBA
**
**    Source   : creatidx.c
**
**    Dialog box for creating Index.
**
** History: (after 29-may-2000)
**
** 29-May-2000 (uk$so01)
**    bug 99242 (DBCS), replace the functions
**    IsCharAlphaNumeric by _istalnum
**    IsCharAlpha by _istalpha
** 26-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
** 05-Jun-2002 (noifr01)
**   (bug 107773) adapted buffer sizes - avoiding an impact
**   of the fact that the VerifyObjectName() function doesn't
**   test any more the length of the string
** 11-May-2010 (drivi01)
**   Add vectorwise_ix storage structure to the list of 
**   possible structures for index.
** 02-Jun-2010 (drivi01)
**   Remove hard coded buffer sizes.
**/

#include "dll.h"
#include <stddef.h>
#include "dbadlg1.h"
#include "dlgres.h"
#include "subedit.h"
#include "getdata.h"
#include "msghandl.h"
#include "domdata.h"
#include "dbaset.h"
#include "dbaginfo.h"
#include "dom.h"
#include "catolist.h"
#include "catospin.h"
#include "catocntr.h"
#include "prodname.h"
#include "resource.h"
#include "tchar.h"

static void ComboBox_InitPageSize (HWND hwnd, LONG Page_size);

// Structure describing the available table structures
static CTLDATA typeStructs[] =
{
	IDX_BTREE,	IDS_STRUCT_BTREE,
	IDX_ISAM,	IDS_STRUCT_ISAM,
	IDX_HASH,	IDS_STRUCT_HASH,
	IDX_VWIX, 	IDS_STRUCT_VWIX,
	-1		// terminator
};

static CTLDATA typeStructsV2[] =
{
	IDX_BTREE,	IDS_STRUCT_BTREE,
	IDX_ISAM,	IDS_STRUCT_ISAM,
	IDX_HASH,	IDS_STRUCT_HASH,
    IDX_RTREE,  IDS_STRUCT_RTREE,
	IDX_VWIX, 	IDS_STRUCT_VWIX,
	-1		// terminator
};


// Structure describing the numeric edit controls
static EDITMINMAX limits[] =
{
   IDC_MINPAGES,    IDC_SPIN_MINPAGES,    IDX_MIN_MINPAGES,    IDX_MAX_MINPAGES,
   IDC_MAXPAGES,    IDC_SPIN_MAXPAGES,		IDX_MIN_MAXPAGES,    IDX_MAX_MAXPAGES,
   IDC_LEAFFILL,    IDC_SPIN_LEAFFILL,		IDX_MIN_LEAFFILL,    IDX_MAX_LEAFFILL,
   IDC_NONLEAFFILL, IDC_SPIN_NONLEAFFILL,	IDX_MIN_NONLEAFFILL, IDX_MAX_NONLEAFFILL,
   IDC_FILLFACTOR,  IDC_SPIN_FILLFACTOR,	IDX_MIN_FILLFACTOR,  IDX_MAX_FILLFACTOR,
   IDC_ALLOCATION,  IDC_SPIN_ALLOCATION,  IDX_MIN_ALLOCATION,  IDX_MAX_ALLOCATION,
   IDC_EXTEND,      IDC_SPIN_EXTEND,      IDX_MIN_EXTEND,      IDX_MAX_EXTEND,
	-1		// terminator
};

typedef struct tagCNTLENABLE
{
   int nCntlId;      // Control ID
   BOOL bEnabled;    // TRUE if control enabled for this structure type
   long lDefault;    // Default without compression
   long lCDefault;   // Default with compression
} CNTLENABLE, FAR * LPCNTLENABLE;

// Number of controls listed in the controls enable structure
#define CTL_ENABLE   17

typedef struct tagSTRUCTCNTLS
{
   int nStructType;     // Structure type
   CNTLENABLE cntl[CTL_ENABLE];  // Define enabled controls
} STRUCTCNTLS, FAR * LPSTRUCTCNTLS;

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
      IDC_DATA_COMPRESSION,   FALSE,   -1,   -1,
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
   IDX_VWIX,
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

BOOL CALLBACK  __export DlgCreateIndexDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI __export CntSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI __export CntChildSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK  __export EnumCntChildProc(HWND hwnd, LPARAM lParam);

//#if   0
//LRESULT WINAPI __export EditSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
//LRESULT WINAPI __export EditDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
//#endif

static void OnDestroy(HWND hwnd);
static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);

static void InitialiseSpinControls (HWND hwnd);
static void InitialiseEditControls (HWND hwnd, BOOL bUseDefaults);
static void InitContainer(HWND hwndCtnr);
static BOOL OccupyStructureControl (HWND hwnd);
static BOOL OccupyColumnsControl (HWND hwnd);
static BOOL OccupyLocationControl (HWND hwnd);
int CALLBACK __export DrawFldData( HWND   hwndCnt,
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
static void DrawCheckBox(HDC hdc, LPRECT lprect, BOOL bSelected, BOOL bGrey);
static void CntOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
static void GetCheckBoxRectFromPt(HWND hwnd, LPFIELDINFO lpFI, LPRECT lprect, POINT pt);
//#if   0
//static void Edit_OnChar(HWND hWnd, UINT ch, int cRepeat);
//static void Edit_OnKey(HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
//static UINT Edit_OnGetDlgCode(HWND hWnd, MSG FAR* lpmsg);
//#endif
static void CntOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static void EnableControls(HWND hwnd);
int GetSelectedStructure(HWND hwnd);
LPSTRUCTCNTLS GetSelectedStructurePtr(HWND hwnd);
static long GetControlDefault(LPSTRUCTCNTLS lpstruct, int nCntl, BOOL bCompressed);
static void Edit_SetDefaultText(HWND hwnd, BOOL bCompressed);
static void EnableOKButton(HWND hwnd);
static void VerifyKeyColumns(HWND hwndCnt);
static BOOL IsKeyFieldValid(LPRECORDCORE lpRC, LPFIELDINFO lpFI);
static void CreateDlgTitle(HWND hwnd);

int WINAPI __export DlgCreateIndex (HWND hwndOwner, LPINDEXPARAMS lpcreate)
/*
	Function:
		Shows the Create Index dialog

	Paramters:
		hwndOwner	- Handle of the parent window for the dialog.
		lpcreate 	- Points to structure containing information used to 
						- initialise the dialog. The dialog uses the same
						- structure to return the result of the dialog.

	Returns:
		TRUE if successful.

	Note:
		If invalid parameters are passed in they are reset to a default
		so the dialog can be safely displayed.
*/
{
    int iRetVal;
    FARPROC lpfnDlgProc;
	if (GetOIVers() != OIVERS_NOTOI)
		return -1;

	if (!IsWindow(hwndOwner) || !lpcreate)
	{
		ASSERT(NULL);
		return -1;
	}
	if (!(*lpcreate->DBName) || !(*lpcreate->TableName))
	{
		ASSERT(NULL);
		return -1;
	}
    lpfnDlgProc = MakeProcInstance((FARPROC)DlgCreateIndexDlgProc, hInst);
		iRetVal = VdbaDialogBoxParam (hResource, MAKEINTRESOURCE(IDD_CREATEINDEX_GW), hwndOwner, (DLGPROC)lpfnDlgProc, (LPARAM)lpcreate);
    FreeProcInstance (lpfnDlgProc);
	return iRetVal;
}


BOOL CALLBACK __export DlgCreateIndexDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
    LPINDEXPARAMS lpidx = (LPINDEXPARAMS)lParam;
    LPSTRUCTCNTLS lpstruct;
    BOOL bKeyCompression = FALSE, bDataCompression = FALSE;
    int nUniqueId, nCount;

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
    // Initialise our globals.
    lpfnDrawProc    = NULL;
    lpidxAttr       = NULL;
    uAttrSize       = 0;
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
	if (!OccupyStructureControl(hwnd) || !OccupyColumnsControl(hwnd) || !OccupyLocationControl(hwnd))
	{
		ASSERT(NULL);
		return FALSE;
	}
    // Check to see if there are any valid columns to create an index on.
    // If not then quit the dialog.
    nCount = CAListBox_GetCount(GetDlgItem (hwnd, IDC_COLUMNS));
    if (nCount <= 0)
    {
        char szTitle[BUFSIZE];
        char szMessage[100];
        HWND currentFocus = GetFocus();

      //LoadString(hResource, IDS_PRODUCT, szTitle, sizeof(szTitle));
        VDBA_GetProductCaption(szTitle, sizeof(szTitle));
        LoadString(hResource, IDS_NOVALIDIDXCOLS, szMessage, sizeof(szMessage));

        MessageBox(NULL, szMessage, szTitle, MB_ICONQUESTION | MB_OK | MB_TASKMODAL);
        SetFocus (currentFocus);
        EndDialog (hwnd, TRUE);
        return FALSE;
    }
    // Select the default structure type if none passed in.
	if (lpidx->nStructure > IDX_HASH || lpidx->nStructure < 1)
		lpidx->nStructure = DEFAULT_STRUCT;
	SelectComboBoxItem(GetDlgItem(hwnd, IDC_STRUCT), typeStructs, lpidx->nStructure);
    EnableControls(hwnd);
    lpstruct = GetSelectedStructurePtr(hwnd);
    // Initialise the compression controls
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
    //
    // Default index persistance to TRUE:
    CheckDlgButton (hwnd, IDC_PERSISTENCE, lpidx->bPersistence? BST_CHECKED: BST_UNCHECKED);
    CheckRadioButton(hwnd, IDC_UNIQUE_NO, IDC_UNIQUE_STATEMENT, nUniqueId);
	// Ensure the defaults passed in are valid
	if (lpidx->nFillFactor > IDX_MAX_FILLFACTOR || lpidx->nFillFactor < IDX_MIN_FILLFACTOR)
		lpidx->nFillFactor = (int)GetControlDefault(lpstruct, IDC_FILLFACTOR, bDataCompression);
	if (lpidx->nMinPages > IDX_MAX_MINPAGES || lpidx->nMinPages < IDX_MIN_MINPAGES)
		lpidx->nMinPages = (LONG)GetControlDefault(lpstruct, IDC_MINPAGES, bDataCompression);
	if (lpidx->nMaxPages > IDX_MAX_MAXPAGES || lpidx->nMaxPages < IDX_MIN_MAXPAGES)
		lpidx->nMaxPages = (LONG)GetControlDefault(lpstruct, IDC_MAXPAGES, bDataCompression);
	if (lpidx->nLeafFill > IDX_MAX_LEAFFILL || lpidx->nLeafFill < IDX_MIN_LEAFFILL)
		lpidx->nLeafFill = (int)GetControlDefault(lpstruct, IDC_LEAFFILL, bDataCompression);
	if (lpidx->nNonLeafFill > IDX_MAX_NONLEAFFILL || lpidx->nNonLeafFill < IDX_MIN_NONLEAFFILL)
		lpidx->nNonLeafFill = (int)GetControlDefault(lpstruct, IDC_NONLEAFFILL, bDataCompression);
	if (lpidx->lAllocation > IDX_MAX_ALLOCATION || lpidx->lAllocation < IDX_MIN_ALLOCATION)
		lpidx->lAllocation = GetControlDefault(lpstruct, IDC_ALLOCATION, bDataCompression);
	if (lpidx->lExtend > IDX_MAX_EXTEND || lpidx->lExtend < IDX_MIN_EXTEND)
		lpidx->lExtend = GetControlDefault(lpstruct, IDC_EXTEND, bDataCompression);
	// Subclass the edit controls
	SubclassAllNumericEditControls(hwnd, EC_SUBCLASS);
	InitialiseSpinControls(hwnd);
	InitialiseEditControls(hwnd, FALSE);
	InitContainer(GetDlgItem(hwnd, IDC_CONTAINER));
    EnableOKButton(hwnd);
    CreateDlgTitle(hwnd);
    lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_CREATEINDEX));
	richCenterDialog(hwnd);
	return TRUE;
}


static void OnDestroy(HWND hwnd)
{
	SubclassAllNumericEditControls(hwnd, EC_RESETSUBCLASS);
	ResetSubClassEditCntl(hwnd, IDC_INDEX);
    SubclassWindow(hwndCntChild, wndprocCntChild);
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
		{
            HWND hwndCols = GetDlgItem(hwnd, IDC_COLUMNS);
            HWND hwndCombo = GetDlgItem (hwnd, IDC_COMBOPAGESIZE);
			char szIndex[MAXOBJECTNAME]; /* MAXOBJECTNAME already includes the trailing \0 */
			char szNumber[64];
			int  nIdx;
			HWND hwndStruct = GetDlgItem(hwnd, IDC_STRUCT);
			HWND hwndIndex = GetDlgItem(hwnd, IDC_INDEX);
			HWND hwndFillFactor = GetDlgItem(hwnd, IDC_FILLFACTOR);
			HWND hwndMinPages = GetDlgItem(hwnd, IDC_MINPAGES);
			HWND hwndMaxPages = GetDlgItem(hwnd, IDC_MAXPAGES);
			HWND hwndLeafFill = GetDlgItem(hwnd, IDC_LEAFFILL);
			HWND hwndNonLeafFill = GetDlgItem(hwnd, IDC_NONLEAFFILL);
			HWND hwndAllocation = GetDlgItem(hwnd, IDC_ALLOCATION);
			HWND hwndExtend = GetDlgItem(hwnd, IDC_EXTEND);
            HWND hwndLocations = GetDlgItem(hwnd, IDC_LOCATION);
            HWND hwndCnt = GetDlgItem(hwnd, IDC_CONTAINER);
			LPINDEXPARAMS lpidx = GetDlgProp(hwnd);
            
			nIdx = ComboBox_GetCurSel(hwndStruct);
            if (nIdx != CB_ERR)
			    lpidx->nStructure = (int)ComboBox_GetItemData(hwndStruct, nIdx);
            else
                break;
            //
            // UK.S 25-07-1997: RTree Index can be specified with only one column !!
            if (lpidx->nStructure == IDX_RTREE && (int)CAListBox_GetSelCount(hwndCols) > 1)
            {
                MessageBox (hwnd, ResourceString ((UINT)IDS_ERR_ONECOLMAX_RTREE), NULL, MB_ICONEXCLAMATION|MB_OK);
                break;
            }
            //
            // OpenIngres version 2.0 ->>
            nIdx = ComboBox_GetCurSel (hwndCombo);
            if (nIdx != -1)
            {
                lpidx->uPage_size = (LONG)ComboBox_GetItemData (hwndCombo, nIdx);
                if (lpidx->uPage_size == 2048L)
                    lpidx->uPage_size = 0L;
            }
            //
            // How to validate the floating point data ?
            // TODO: Data validation for the floating point.
            if (lpidx->nStructure == IDX_RTREE)
            {
			    Edit_GetText (GetDlgItem (hwnd, IDC_EDIT1), lpidx->minX, sizeof(lpidx->minX));
			    Edit_GetText (GetDlgItem (hwnd, IDC_EDIT2), lpidx->maxX, sizeof(lpidx->maxX));
			    Edit_GetText (GetDlgItem (hwnd, IDC_EDIT3), lpidx->minY, sizeof(lpidx->minY));
			    Edit_GetText (GetDlgItem (hwnd, IDC_EDIT4), lpidx->maxY, sizeof(lpidx->maxY));
            }
            // <<-
			Edit_GetText(hwndIndex, szIndex, sizeof(szIndex));
			if (!VerifyObjectName(hwnd, szIndex, TRUE, TRUE, TRUE))
			{
				Edit_SetSel(hwndIndex, 0, -1);
				SetFocus(hwndIndex);
				break;
			}

			if (!VerifyAllNumericEditControls (hwnd, TRUE, TRUE))
				break;

			lstrcpy(lpidx->objectname, szIndex);
			lstrcpy(lpidx->szSchema, "");

			Edit_GetText(hwndFillFactor, szNumber, sizeof(szNumber));
			lpidx->nFillFactor = my_atoi(szNumber);

			Edit_GetText(hwndMinPages, szNumber, sizeof(szNumber));
			lpidx->nMinPages = /*my_atoi(szNumber);*/  atol (szNumber);

			Edit_GetText(hwndMaxPages, szNumber, sizeof(szNumber));
			lpidx->nMaxPages = /*my_atoi(szNumber);*/  atol (szNumber);

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

			if (IsDlgButtonChecked(hwnd, IDC_UNIQUE_NO))
                lpidx->nUnique = IDX_UNIQUE_NO;
            else if (IsDlgButtonChecked(hwnd, IDC_UNIQUE_ROW))
                lpidx->nUnique = IDX_UNIQUE_ROW;
            else if (IsDlgButtonChecked(hwnd, IDC_UNIQUE_STATEMENT))
                lpidx->nUnique = IDX_UNIQUE_STATEMENT;
		
			if (GetOIVers() == OIVERS_NOTOI) {
				if (IsDlgButtonChecked(hwnd, IDC_IDX_UNIQUE_GW))
	                lpidx->nUnique = IDX_UNIQUE_ROW; // mapped to unique_row, but low level only generates the UNIQUE keyword for gateways
				else
	                lpidx->nUnique = IDX_UNIQUE_NO;
			}

            lpidx->bPersistence = IsDlgButtonChecked(hwnd, IDC_PERSISTENCE);
    
            lpidx->lpLocations = CreateList4CheckedObjects(hwndLocations);

         // Work through the container control and create a linked list
         // of the selected columns and their attributes.

         {
            LPOBJECTLIST lpList = NULL;
            LPRECORDCORE lpRC;
            LPFIELDINFO lpFld;
            IDXATTR attr;

            lpRC = CntRecHeadGet(hwndCnt);
            lpFld = CntFldTailGet(hwndCnt);

            while (lpFld)
            {
               LPIDXCOLS lpObj;
               LPOBJECTLIST lpTemp;

               lpTemp = AddListObject(lpList, sizeof(IDXCOLS));
               if (!lpTemp)
               {
                  // Need to free previously allocated objects.
                  FreeObjectList(lpList);
                  break;
               }
               else
                  lpList = lpTemp;

               lpObj = (LPIDXCOLS)lpList->lpObject;

               CntFldDataGet(lpRC, lpFld, sizeof(attr), &attr);
               lpObj->attr.bKey = attr.bKey;
               lstrcpy(lpObj->szColName, lpFld->lpFTitle);

               lpFld = CntPrevFld(lpFld);
            }

            lpidx->lpidxCols = lpList;
         }

         // Call the low level function to write/update data on disk
         //
         if (lpidx->bCreate)
         {
             //
             // Add object
             //
             int ires;
             ires = DBAAddObject
                     ( GetVirtNodeName ( GetCurMdiNodeHandle ()),
                       OT_INDEX,
                       (void *) lpidx );

             if (ires != RES_SUCCESS)
             {
                   ErrorMessage ((UINT) IDS_E_CREATE_INDEX_FAILED, ires);
                   SetFocus (hwnd);
             }
             else
             {
                 EndDialog (hwnd, TRUE);
             }
             if (lpidx) {  // lionel : added the object free.
               FreeObjectList(lpidx->lpidxCols);   // free colomns list.        
               FreeObjectList(lpidx->lpLocations); // free locations list.        
               lpidx=NULL;
            }   
         }
  
		  break;
		}

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			break;

		case IDC_MINPAGES:
		case IDC_MAXPAGES:
		case IDC_LEAFFILL:
		case IDC_NONLEAFFILL:
		case IDC_FILLFACTOR:
		case IDC_ALLOCATION:
		case IDC_EXTEND:
		{
			switch (codeNotify)
			{
				case EN_CHANGE:
				{
					int nCount;

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
				}

				case EN_KILLFOCUS:
				{
					HWND hwndNew = GetFocus();
					int nNewCtl = GetDlgCtrlID(hwndNew);

					if (nNewCtl == IDCANCEL
					 || nNewCtl == IDOK
					 || !IsChild(hwnd, hwndNew))
						// Dont verify on any button hits
						break;

					if (!VerifyAllNumericEditControls(hwnd, TRUE, TRUE))
						break;

					UpdateSpinButtons(hwnd);
					break;
				}
			}
			break;
		}

      case IDC_COLUMNS:
      {
         switch (codeNotify)
         {
            case CALBN_CHECKCHANGE:
            {
               int nIdx;
               BOOL bSelected;
               char szColumn[MAXOBJECTNAME];
               HWND hwndCntr = GetDlgItem(hwnd, IDC_CONTAINER);

               ASSERT(lpidxAttr);
               if (!lpidxAttr)
                  break;

               nIdx = CAListBox_GetCurSel(hwndCtl);
               bSelected = CAListBox_GetSel(hwndCtl, nIdx);
               CAListBox_GetText(hwndCtl, nIdx, szColumn);

               CntDeferPaint(hwndCntr);

               if (bSelected)
               {
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

                  // Copy the record data into our buffer

                  if (lpRC = CntRecHeadGet(hwndCntr))
                     CntRecDataGet(hwndCntr, lpRC, (LPVOID)lpidxAttr);

                  // Initialise the data for this field.

                  lpThisIdxAttr = lpidxAttr + (uFieldCounter - 1);
                  lpThisIdxAttr->bKey = FALSE;

                  // Initialise the field descriptor.

                  lpFI = CntNewFldInfo();
                  lpFI->wIndex       = uFieldCounter;
                  lpFI->cxWidth      = 16;
                  lpFI->flFDataAlign = CA_TA_LEFT | CA_TA_VCENTER;
                  lpFI->wColType     = CFT_CUSTOM;
                  lpFI->flColAttr    = CFA_OWNERDRAW;
                  lpFI->wDataBytes   = sizeof(IDXATTR);
                  lpFI->wOffStruct   = (uFieldCounter - 1) * sizeof(IDXATTR);

                  CntFldTtlSet(hwndCntr, lpFI, szColumn, lstrlen(szColumn) + 1);
               	  CntFldColorSet(hwndCntr, lpFI, CNTCOLOR_FLDBKGD, CntFldColorGet(hwndCntr, lpFI, CNTCOLOR_FLDTTLBKGD));
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
                  // Remove the field from the container

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

               EnableOKButton(hwnd);

               CntEndDeferPaint(hwndCntr, TRUE);
               break;
            }
         }
         break;
      }

		case IDC_SPIN_MINPAGES:
		case IDC_SPIN_MAXPAGES:
		case IDC_SPIN_LEAFFILL:
		case IDC_SPIN_NONLEAFFILL:
		case IDC_SPIN_FILLFACTOR:
		case IDC_SPIN_ALLOCATION:
		case IDC_SPIN_EXTEND:
		{
			ProcessSpinControl(hwndCtl, codeNotify, limits);
			break;
		}

		case IDC_INDEX:
		{
			switch (codeNotify)
			{
				case EN_CHANGE:
				{
               EnableOKButton(hwnd);
					break;
				}
			}
			break;
		}

      case IDC_STRUCT:
      {
         switch (codeNotify)
         {
            case CBN_SELCHANGE:
            {
				EnableControls(hwnd);
				InitialiseSpinControls(hwnd);
				InitialiseEditControls(hwnd, TRUE);
                EnableOKButton(hwnd);
                SetFocus(hwndCtl);
				break;
            }

			case CBN_KILLFOCUS:
			{
				// We must verify the edit controls on exiting the
				// structure field since we dont do any checking when
				// it gets the focus,  therefore an edit control could
				// be invalid.

				VerifyAllNumericEditControls(hwnd, TRUE, TRUE);
				break;
			}
         }

         break;
      }

      case IDC_CONTAINER:
      {
         switch (codeNotify)
         {
            case CN_FLDMOVED:
            {
               VerifyKeyColumns(hwndCtl);
               break;
            }
         }

         break;
      }
      case IDC_EDIT1:
      case IDC_EDIT2:
      case IDC_EDIT3:
      case IDC_EDIT4:
          if (codeNotify == EN_CHANGE)
               EnableOKButton(hwnd);
          break;
	}
}


static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
	Ctl3dColorChange();
#endif
}


static void InitialiseEditControls (HWND hwnd, BOOL bUseDefaults)
{
	char szNumber[20];
	LPINDEXPARAMS lpidx = GetDlgProp(hwnd);

	HWND hwndIndex = GetDlgItem(hwnd, IDC_INDEX);
	HWND hwndFillFactor = GetDlgItem(hwnd, IDC_FILLFACTOR);
	HWND hwndMinPages = GetDlgItem(hwnd, IDC_MINPAGES);
	HWND hwndMaxPages = GetDlgItem(hwnd, IDC_MAXPAGES);
	HWND hwndLeafFill = GetDlgItem(hwnd, IDC_LEAFFILL);
	HWND hwndNonLeafFill = GetDlgItem(hwnd, IDC_NONLEAFFILL);
	HWND hwndAllocation = GetDlgItem(hwnd, IDC_ALLOCATION);
	HWND hwndExtend = GetDlgItem(hwnd, IDC_EXTEND);
    LPSTRUCTCNTLS lpstruct = GetSelectedStructurePtr(hwnd);
    //
	// Limit the text in the edit controls
    //
    Edit_LimitText (GetDlgItem (hwnd, IDC_EDIT1), 30);
    Edit_LimitText (GetDlgItem (hwnd, IDC_EDIT2), 30);
    Edit_LimitText (GetDlgItem (hwnd, IDC_EDIT3), 30);
    Edit_LimitText (GetDlgItem (hwnd, IDC_EDIT4), 30);
    if (!lpstruct)
        return; // Rtree Structure.

	Edit_LimitText(hwndIndex, MAXOBJECTNAME-1);
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
      	//my_itoa(lpidx->nMinPages, szNumber, 10);
         _ltoa(lpidx->nMinPages, szNumber, 10);
      else
         *szNumber = (char)NULL;

      Edit_SetText(hwndMinPages, szNumber);

      if (IsWindowEnabled(hwndMaxPages))
      	//my_itoa(lpidx->nMaxPages, szNumber, 10);
      	 _ltoa(lpidx->nMaxPages, szNumber, 10);
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
	LPINDEXPARAMS lpidx = GetDlgProp(hwnd);

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
	Function:
		Fills the structure drop down box with the structure names.

	Parameters:
		hwnd	- Handle to the dialog window.

	Returns:
		TRUE if successful.
*/
{
	HWND hwndCtl = GetDlgItem (hwnd, IDC_STRUCT);
    if (GetOIVers () ==OIVERS_12)
	    return OccupyComboBoxControl(hwndCtl, typeStructs);
    else
	    return OccupyComboBoxControl(hwndCtl, typeStructsV2);
}


static BOOL OccupyColumnsControl (HWND hwnd)
/*
	Function:
		Fills the columns CA list box with the column names.

	Parameters:
		hwnd	- Handle to the dialog window.

	Returns:
		TRUE if successful.
*/
{
	HWND hwndCtl = GetDlgItem (hwnd, IDC_COLUMNS);
	int hNode;
	int err;
	BOOL bSystem;
	char szObject[MAXOBJECTNAME];
   char szFilter[MAXOBJECTNAME];
   char szAll   [MAXOBJECTNAME];

   LPUCHAR aparents[MAXPLEVEL];
	LPINDEXPARAMS lpidx = GetDlgProp(hwnd);
   UCHAR extradata[EXTRADATASIZE];

	ZEROINIT(aparents);
	ZEROINIT(szObject);
	ZEROINIT(szFilter);

   StringWithOwner (lpidx->TableName, lpidx->TableOwner, szAll);
	aparents[0] = lpidx->DBName;
	aparents[1] = szAll;

	hNode = GetCurMdiNodeHandle();
	bSystem = GetSystemFlag ();

	err = DOMGetFirstObject(hNode,
									OT_COLUMN,
									2,
									aparents,
									bSystem,
									NULL,
									szObject,
									NULL,
									extradata);

	while (err == RES_SUCCESS)
	{
      int nType = getint(extradata);

      // You cannot create an index on a longvarchar column

      if (nType != INGTYPE_LONGVARCHAR)
   		CAListBox_AddString(hwndCtl, szObject);

		err = DOMGetNextObject (szObject, szFilter, extradata);
	}

	return TRUE;
}


static BOOL OccupyLocationControl (HWND hwnd)
/*
	Function:
		Fills the location drop down box with the location names.

	Parameters:
		hwnd	- Handle to the dialog window.

	Returns:
		TRUE if successful.
*/
{
	HWND hwndCtl = GetDlgItem (hwnd, IDC_LOCATION);
	int hNode;
	int err;
	BOOL bSystem;
	char szObject[MAXOBJECTNAME];
   char szFilter[MAXOBJECTNAME];
   LPUCHAR aparents[MAXPLEVEL];
	LPINDEXPARAMS lpidx = GetDlgProp(hwnd);
   char szDB[MAXOBJECTNAME];
   int nIdx;

	ZEROINIT(aparents);
	ZEROINIT(szObject);
	ZEROINIT(szFilter);

	aparents[0] = lpidx->DBName;
	aparents[1] = lpidx->TableName;

	hNode = GetCurMdiNodeHandle();
#if   0
	bSystem = GetSystemFlag ();
#else
   // Force to TRUE so as II_DATABASE is picked up.
   bSystem = TRUE;
#endif
	if ( !(GetOIVers() == OIVERS_NOTOI)
	   ) {

		err = DOMGetFirstObject(hNode,
										OT_LOCATION,
										0,
										aparents,
										bSystem,
										NULL,
										szObject,
										NULL,
										NULL);

		while (err == RES_SUCCESS)
		{
		  BOOL bOK;
		  if (DOMLocationUsageAccepted(hNode,szObject,LOCATIONDATABASE,&bOK)==
			  RES_SUCCESS && bOK) {
			   CAListBox_AddString(hwndCtl, szObject);
		  }
			err = DOMGetNextObject (szObject, szFilter, NULL);
		}
	}

   // Select II_DATABASE by default
   LoadString(hResource, IDS_IIDATABASE, szDB, sizeof(szDB));
   nIdx = CAListBox_FindString(hwndCtl, -1, szDB);
   CAListBox_SetSel(hwndCtl, TRUE, nIdx);

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
//	char szTitle[128];
	HDC hdc = GetDC(hwnd);
   TEXTMETRIC tm;
   int nRowHeight;
   RECT rect;
//   char szKey[20];

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
#if   0
	CntColorSet( hwnd, CNTCOLOR_TITLE,      RGB(  0,   0,   0) );
#endif
	CntColorSet( hwnd, CNTCOLOR_FLDTITLES,  RGB(  0,   0,   0) );
	CntColorSet( hwnd, CNTCOLOR_TEXT,       RGB(  0,   0,   0) );
	CntColorSet( hwnd, CNTCOLOR_GRID,       RGB(  0,   0,   0) );

	// Set some general container attributes.
	CntAttribSet(hwnd, CA_MOVEABLEFLDS | /*CA_TITLE3D |*/ CA_FLDTTL3D);

#if   0
   // Dont use a title with the new layout

	LoadString(hResource, IDS_IDXCNTR_TITLE, szTitle, sizeof(szTitle));
	CntTtlSet(hwnd, szTitle);
	CntTtlAlignSet(hwnd, CA_TA_HCENTER | CA_TA_VCENTER);
	CntTtlHtSet(hwnd, 1);
	// Turn on the horz separator line.
	CntTtlSepSet(hwnd);
#endif

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
   lpfnDrawProc = (DRAWPROC)MakeProcInstance((FARPROC)DrawFldData, hInst);

   uAttrSize = sizeof(IDXATTR);
   lpidxAttr = ESL_AllocMem(uAttrSize);

	// Now paint the container.
	CntEndDeferPaint( hwnd, TRUE );

   // Subclass the container window so we can receive messages from the
   // edit control
   wndprocCnt = SubclassWindow(hwnd, CntSubclassProc);

   // Subclass the container child window so we can operate the check boxes.
   EnumChildWindows(hwnd, EnumCntChildProc, (LPARAM)(HWND FAR *)&hwndCntChild);
   wndprocCntChild = SubclassWindow(hwndCntChild, CntChildSubclassProc);

#if   0
   // You cannot edit the key value.  The key is determined by the order
   // of the column name in the list

   // Create an edit window so we can edit the index keys.
   hwndCntEdit = CreateWindow("edit",
                              "",
                              WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL | WS_CLIPSIBLINGS,
                              0, 0, 0, 0,
                              hwnd,
                              (HMENU)ID_CNTEDIT, 
                              GetWindowInstance(hwnd),
                              NULL);
   wndprocEdit = SubclassWindow(hwndCntEdit, EditSubclassProc);

   // Determine the maximum key value and set up the edit control limit

   nMaxKey = CAListBox_GetCount(GetDlgItem(GetParent(hwnd), IDC_COLUMNS));
   my_itoa(nMaxKey, szKey, 10);
   Edit_LimitText(hwndCntEdit, lstrlen(szKey));

   SetWindowFont(hwndCntEdit, hFont, FALSE);
#endif

   VerifyKeyColumns(hwnd);

	SelectObject(hdc, hOldFont);
	ReleaseDC(hwnd, hdc);
}


BOOL CALLBACK __export EnumCntChildProc(HWND hwnd, LPARAM lParam)
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


LRESULT WINAPI __export CntSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
      HANDLE_MSG(hwnd, WM_COMMAND, CntOnCommand);
   }

	return CallWindowProc(wndprocCnt, hwnd, message, wParam, lParam);
}


LRESULT WINAPI __export CntChildSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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


int CALLBACK __export DrawFldData( HWND   hwndCnt,     // container window handle
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
//   char szData[10];
//   char szKey[20];
   LPIDXATTR lpattr = (LPIDXATTR)lpData;
   TEXTMETRIC tm;
   RECT rLine1;
   RECT rLine2;
   BOOL bGrey;

#if   0
   HPEN hpen, hpenOld;

   hpen = CreatePen(hDC, PS_SOLID, RGB(0, 0, 0));
   hpenOld = SelectObject(hDC, hpen);

   // Draw the right border

	MoveToEx(hDC, lpRect->right - 1, lpRect->top, NULL);
	LineTo(hDC, lpRect->right - 1, lpRect->bottom);

   // Draw the bottom border

	MoveToEx(hDC, lpRect->left, lpRect->bottom - 1, NULL);
	LineTo(hDC, lpRect->right, lpRect->bottom - 1);

   // Free the pen resource
   SelectObject(hDC, hpenOld);
   DeleteObject(hpen);
#endif

   GetTextMetrics(hDC, &tm);

   rLine1.left   = lpRect->left;
   rLine1.top    = lpRect->top;
   rLine1.right  = lpRect->right;
   rLine1.bottom = lpRect->top + nCBHeight + (2 * BOX_VERT_MARGIN);

   rLine2.left   = lpRect->left;
   rLine2.top    = rLine1.bottom + 1;
   rLine2.right  = lpRect->right;
   rLine2.bottom = lpRect->bottom;

   bGrey = !IsKeyFieldValid(lpRec, lpFld);

	if (!(GetOIVers() == OIVERS_NOTOI))
		DrawCheckBox(hDC, lpRect, lpattr->bKey, bGrey);


#if   0
   LoadString(hResource, IDS_KEY, szKey, sizeof(szKey));

   // Line the key header up with the check box
   rLine2.left += BOX_LEFT_MARGIN;

   rLine2.top += EDIT_BORDER_WIDTH;
   rLine2.bottom -= EDIT_BORDER_WIDTH;

   // Show the key header

   DrawText(hDC,
            szKey,
            lstrlen(szKey),
            &rLine2,
            DT_LEFT | DT_VCENTER);

    if (dwKeyLen == 0)
    {    
        dwKeyLen = GetTextExtent(hDC, szKey, lstrlen(szKey));
    }

   rLine2.left += (int)dwKeyLen + EDIT_BORDER_WIDTH + EDIT_CURSOR_WIDTH;

   // Show the key

   DrawText(hDC,
            szData,
            lstrlen(szData),
            &rLine2,
            DT_LEFT);
#endif

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

	Rectangle3D(hdc,
				   rect.left,
   				rect.top,
	   			rect.right,
		   		rect.bottom);

	if (bSelected)
   	DrawCheck3D(hdc, rect.left, rect.top, rect.right, rect.bottom, bGrey);

   // Draw the text associated with the check box.

   rect.left = rect.right + TEXT_LEFT_MARGIN;
   rect.right = lprect->right;

   LoadString(hResource, IDS_KEY, szText, sizeof(szText));

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
#if   0
      case ID_CNTEDIT:
      {
         switch (codeNotify)
         {
            case EN_CHANGE:
            {
               char szKey[20];
               int nKey;

               Edit_GetText(hwndCtl, szKey, sizeof(szKey));
               nKey = my_atoi(szKey);
               if (nKey > nMaxKey)
               {
                  // If we exceed the maximum then reset the text
                  // to the maximum

                  my_itoa(nMaxKey, szKey, 10);
                  Edit_SetText(hwndCtl, szKey);
         			Edit_SetSel(hwndCtl, 0, -1);
               }

               break;
            }

            case EN_KILLFOCUS:
            {
               LPSTR lpattr;
               char szKey[20];
               int nKey;

               MoveWindow(hwndCtl, 0, 0, 0, 0, TRUE);

               // Point at the field data in the container record list.
               lpattr = (LPSTR)lpEditRC->lpRecData;
               lpattr += lpEditFI->wOffStruct;

               Edit_GetText(hwndCtl, szKey, sizeof(szKey));
               nKey = my_atoi(szKey);
               ((LPIDXATTR)lpattr)->nKey = nKey;

               CntUpdateRecArea(hwnd, lpEditRC, lpEditFI);
               break;
            }
         }

         break;
      }
#endif
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
//         char szKey[20];
         LPCNTINFO lpcntInfo = CntInfoGet(hwndCnt);

         // Get the data for this cell
         CntFldDataGet(lpRC, lpFI, sizeof(attr), &attr);

         if (!IsKeyFieldValid(lpRC, lpFI))
            return;

         GetCheckBoxRectFromPt(hwndCnt, lpFI, &rect, ptMouse);

         if (PtInRect(&rect, ptMouse))
         {
            // Toggle the selection flag.

            attr.bKey = !attr.bKey;
            CntFldDataSet(lpRC, lpFI, sizeof(attr), &attr);

            // Force this cell to repaint

            CntUpdateRecArea(hwndCnt, lpRC, lpFI);

            VerifyKeyColumns(hwndCnt);
            return;
         }

#if   0
         // If not in the check box area we must be in the key area.

         my_itoa(attr.nKey, szKey, 10);
         Edit_SetText(hwndCntEdit, szKey);

         // Adjust the checkbox rect to point to the key rectangle

         rect.left += (int)dwKeyLen;
         rect.top = rect.bottom + BOX_VERT_MARGIN + 1;
         rect.bottom = rect.top + (lpcntInfo->cyRow - nCBHeight - (2 * BOX_VERT_MARGIN));

         // Set up the pointers so the edit window can update our data
         // structure and repaint the control
         lpEditFI = lpFI;
         lpEditRC = lpRC;

         SetWindowPos(hwndCntEdit,
                      HWND_TOP,
                      rect.left,
                      rect.top,
                      rect.right - rect.left - 2,
                      rect.bottom - rect.top - 2,
                      SWP_SHOWWINDOW);

         SetFocus(hwndCntEdit);
         return;
#endif
      }
   }
   return;
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


#if   0
LRESULT WINAPI __export EditSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
      HANDLE_MSG(hwnd, WM_KEYDOWN, Edit_OnKey);
      HANDLE_MSG(hwnd, WM_CHAR, Edit_OnChar);
      HANDLE_MSG(hwnd, WM_GETDLGCODE, Edit_OnGetDlgCode);

      default:
      	return EditDefWndProc(hwnd, message, wParam, lParam);
   }

   return 0L;
}


LRESULT WINAPI __export EditDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   return CallWindowProc(wndprocEdit, hwnd, message, wParam, lParam);
}


static void Edit_OnChar(HWND hwnd, UINT ch, int cRepeat)
{
   if (_istalnum(ch) && !_istalpha(ch) || ch == VK_BACK)
      FORWARD_WM_CHAR(hwnd, ch, cRepeat, EditDefWndProc);
}


static void Edit_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
   HWND hwndCnt = GetParent(hwnd);
   LPSTR lpattr;
   char szKey[20];

   // We don't process key-up messages.
   if( !fDown )
   {
      // Forward this to the default proc for processing.
      FORWARD_WM_KEYUP(hwnd, vk, cRepeat, flags, EditDefWndProc);
      return;
   }

   switch(vk)
   {
      case VK_ESCAPE:
         // If user hit the escape key just retrieve the original data
         // and set it into the edit control. UpdateCntFld currently
         // just updates blindly but it could be modified to only update
         // the record list if the data has changed.

         // Point at the field data in the container record list.
         lpattr = (LPSTR)lpEditRC->lpRecData;
         lpattr += lpEditFI->wOffStruct;

         my_itoa(((LPIDXATTR)lpattr)->nKey, szKey, 10);

         Edit_SetText(hwnd, szKey);
         SetFocus(hwndCnt);
         break;

      case VK_RETURN:
      case VK_TAB:
      {
         int nKey;
         char szKey[20];

         // End the edit session by returning focus to the container.
         // The edit control will get an EN_KILLFOCUS first so that
         // the record list is updated and the edit control destroyed.
         SetFocus(hwndCnt);

         Edit_GetText(hwnd, szKey, sizeof(szKey));
         nKey = my_atoi(szKey);

         // Send the RETURN or TAB to the container so it will notify
         // the app. The app will then move the focus appropriately.
         FORWARD_WM_KEYDOWN(hwndCnt, vk, cRepeat, flags, SendMessage);
         break;
      }
   }

   // Forward this to the default proc for processing.
   FORWARD_WM_KEYDOWN(hwnd, vk, cRepeat, flags, EditDefWndProc);
}


static UINT Edit_OnGetDlgCode(HWND hwnd, MSG FAR* lpmsg)
{
   UINT uRet;

   // Processing of this message is only necessary for controls
   // within dialogs. The return value of DLGC_WANTMESSAGE tells the
   // dialog manager that we want ALL keystroke msgs to come to this
   // edit control and no further processing of the keystroke msg is
   // to take place. If this message is not processed in this manner
   // the control will not receive the keystroke messages that you
   // really need (like TAB, ENTER, and ESCAPE) while in a dialog.
   // This message is being processed the same way in the container.

   // Forward this to the default proc for processing.
   uRet = (UINT)FORWARD_WM_GETDLGCODE(hwnd, lpmsg, EditDefWndProc);

   // Now specify that we want all keyboard input.
   uRet |= DLGC_WANTMESSAGE;

   return uRet;
}
#endif


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
    LPSTRUCTCNTLS lpstruct = GetSelectedStructurePtr(hwnd);
    int i, strid;

    strid = GetSelectedStructure (hwnd);
    if (strid == IDX_RTREE)
    {
        ShowWindow (GetDlgItem (hwnd, IDC_TXT_MINPAGES),   SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_TXT_MAXPAGES),   SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_TXT_LEAFFILL),   SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_TXT_NONLEAFFILL),SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_MINPAGES),  SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_MAXPAGES),  SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_LEAFFILL),  SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_NONLEAFFILL),SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_SPIN_MINPAGES),   SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_SPIN_MAXPAGES),   SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_SPIN_LEAFFILL),   SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_SPIN_NONLEAFFILL),SW_HIDE);

        ShowWindow (GetDlgItem (hwnd, IDC_GROUP),  SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_STATIC1),SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_STATIC2),SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_STATIC3),SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_STATIC4),SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_EDIT1),SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_EDIT2),SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_EDIT3),SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_EDIT4),SW_SHOW);

        CheckRadioButton(hwnd, IDC_UNIQUE_NO, IDC_UNIQUE_STATEMENT, IDC_UNIQUE_NO);
        EnableWindow (GetDlgItem (hwnd, IDC_UNIQUE_ROW), FALSE);
        EnableWindow (GetDlgItem (hwnd, IDC_UNIQUE_STATEMENT), FALSE);
        CheckDlgButton(hwnd, IDC_DATA_COMPRESSION, TRUE);
        return;
    }

    ShowWindow (GetDlgItem (hwnd, IDC_TXT_MINPAGES),   SW_SHOW);
    ShowWindow (GetDlgItem (hwnd, IDC_TXT_MAXPAGES),   SW_SHOW);
    ShowWindow (GetDlgItem (hwnd, IDC_TXT_LEAFFILL),   SW_SHOW);
    ShowWindow (GetDlgItem (hwnd, IDC_TXT_NONLEAFFILL),SW_SHOW);
    ShowWindow (GetDlgItem (hwnd, IDC_MINPAGES),  SW_SHOW);
    ShowWindow (GetDlgItem (hwnd, IDC_MAXPAGES),  SW_SHOW);
    ShowWindow (GetDlgItem (hwnd, IDC_LEAFFILL),  SW_SHOW);
    ShowWindow (GetDlgItem (hwnd, IDC_NONLEAFFILL),SW_SHOW);
    ShowWindow (GetDlgItem (hwnd, IDC_SPIN_MINPAGES),   SW_SHOW);
    ShowWindow (GetDlgItem (hwnd, IDC_SPIN_MAXPAGES),   SW_SHOW);
    ShowWindow (GetDlgItem (hwnd, IDC_SPIN_LEAFFILL),   SW_SHOW);
    ShowWindow (GetDlgItem (hwnd, IDC_SPIN_NONLEAFFILL),SW_SHOW);

    ShowWindow (GetDlgItem (hwnd, IDC_GROUP),  SW_HIDE);
    ShowWindow (GetDlgItem (hwnd, IDC_STATIC1),SW_HIDE);
    ShowWindow (GetDlgItem (hwnd, IDC_STATIC2),SW_HIDE);
    ShowWindow (GetDlgItem (hwnd, IDC_STATIC3),SW_HIDE);
    ShowWindow (GetDlgItem (hwnd, IDC_STATIC4),SW_HIDE);
    ShowWindow (GetDlgItem (hwnd, IDC_EDIT1),SW_HIDE);
    ShowWindow (GetDlgItem (hwnd, IDC_EDIT2),SW_HIDE);
    ShowWindow (GetDlgItem (hwnd, IDC_EDIT3),SW_HIDE);
    ShowWindow (GetDlgItem (hwnd, IDC_EDIT4),SW_HIDE);
    EnableWindow (GetDlgItem (hwnd, IDC_UNIQUE_ROW), TRUE);
    EnableWindow (GetDlgItem (hwnd, IDC_UNIQUE_STATEMENT), TRUE);
    CheckDlgButton(hwnd, IDC_DATA_COMPRESSION, FALSE);

    if (!lpstruct)
       return;
    
    for (i = 0; i < CTL_ENABLE; i++)
    {
       HWND hwndCtl = GetDlgItem(hwnd, lpstruct->cntl[i].nCntlId);
       EnableWindow(hwndCtl, lpstruct->cntl[i].bEnabled);
    }
}


int GetSelectedStructure(HWND hwnd)
{
	HWND hwndCtl = GetDlgItem (hwnd, IDC_STRUCT);
	return (int)ComboBox_GetItemData(hwndCtl, ComboBox_GetCurSel(hwndCtl));
}



LPSTRUCTCNTLS GetSelectedStructurePtr(HWND hwnd)
{
   int nStruct = GetSelectedStructure(hwnd);
   LPSTRUCTCNTLS lpstruct = cntlEnable;

   // Find the current structure

   while (lpstruct->nStructType != -1)
   {
      if (lpstruct->nStructType == nStruct)
         return lpstruct;

      lpstruct++;
   }

//   ASSERT(NULL);
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
   LPSTRUCTCNTLS lpstruct = GetSelectedStructurePtr(hwnd);

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


static void EnableOKButton(HWND hwnd)
{
    BOOL bEnabled = FALSE;
    HWND hwndCols = GetDlgItem(hwnd, IDC_COLUMNS);
    int  nStruct, cItems = CAListBox_GetSelCount(hwndCols);

    if (cItems != 0 && Edit_GetTextLength(GetDlgItem(hwnd, IDC_INDEX)) > 0)
        bEnabled = TRUE;
    nStruct = GetSelectedStructure (hwnd);
    if (nStruct == IDX_RTREE)
    {
        if (Edit_GetTextLength (GetDlgItem (hwnd, IDC_EDIT1)) == 0)
            bEnabled = FALSE;
        if (Edit_GetTextLength (GetDlgItem (hwnd, IDC_EDIT2)) == 0)
            bEnabled = FALSE;
        if (Edit_GetTextLength (GetDlgItem (hwnd, IDC_EDIT3)) == 0)
            bEnabled = FALSE;
        if (Edit_GetTextLength (GetDlgItem (hwnd, IDC_EDIT4)) == 0)
            bEnabled = FALSE;
    }
    EnableWindow(GetDlgItem(hwnd, IDOK), bEnabled);
}


static void VerifyKeyColumns(HWND hwndCnt)
/*
   Function:
      Looks at all the columns in the container and ensures that
      the selected key columns are the leading columns in the definition.
      The key option is greyed if it is not valid,  and if columns are
      re-ordered invalid keys will be cleared.

   Parameters:
      hwndCnt     - Handle to the container window

   Returns:
      None.
*/
{
   LPRECORDCORE lpRC;
   LPFIELDINFO lpFI;
   BOOL bKeysValid = TRUE;

   lpRC = CntRecHeadGet(hwndCnt);
   lpFI = CntFldHeadGet(hwndCnt);

   while (lpFI)
   {
      IDXATTR attr;

      CntFldDataGet(lpRC, lpFI, sizeof(attr), &attr);

      // Keys valid from the first column to the first non-key column
      if (!attr.bKey)
         bKeysValid = FALSE;

      if (!bKeysValid)
         attr.bKey = FALSE;

      CntFldDataSet(lpRC, lpFI, sizeof(attr), &attr);

      CntUpdateRecArea(hwndCnt, lpRC, lpFI);

      lpFI = CntNextFld(lpFI);
   }
}


static BOOL IsKeyFieldValid(LPRECORDCORE lpRC, LPFIELDINFO lpFI)
{
   LPFIELDINFO lpFIPrev = CntPrevFld(lpFI);
   IDXATTR attr;

   // First field is always a valid key field.

   if (!lpFIPrev)
      return TRUE;

   // Get the data for the previous field. If a key column
   // then this column can be a key column.

   CntFldDataGet(lpRC, lpFIPrev, sizeof(attr), &attr);

   if (attr.bKey)
      return TRUE;

   return FALSE;
}


static void CreateDlgTitle(HWND hwnd)
{
   char szTitle[MAXOBJECTNAME*4+256];
   char szBuf[100];
   char buffer   [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];

   LPINDEXPARAMS lpidx = GetDlgProp(hwnd);
   LPSTR lpszNode = GetVirtNodeName(GetCurMdiNodeHandle());

   GetWindowText(hwnd, szTitle, sizeof(szTitle));
   LoadString(hResource, IDS_ON, szBuf, sizeof(szBuf));
   parentstrings [0] = lpidx->DBName;
   GetExactDisplayString (
       lpidx->TableName,
       lpidx->TableOwner,
       OT_TABLE,
       parentstrings,
       buffer);

   //lstrcat(szTitle, " on ");
   lstrcat(szTitle, szBuf);
   lstrcat(szTitle, "[");
   lstrcat(szTitle, lpszNode);
   lstrcat(szTitle, "::");
   lstrcat(szTitle, lpidx->DBName);
   lstrcat(szTitle, "] ");
   lstrcat(szTitle, buffer);

   SetWindowText(hwnd, szTitle);
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
