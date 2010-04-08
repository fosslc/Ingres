/*
** Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Project  : Visual DBA
**
**    Source   : spcalc.c
**
**    Space calculation dialog box
**
**  30-Mar-2001 (noifr01)
**    (sir 104378) differentiation of II 2.6. (2.6 is temporary managed
**    like 2.5, until it gets rewritten specifically)
**  12-Apr-2001 (noifr01)
**    (sir 102955) take into account the page type (V1,V2,V3 or V4) on
**    2.6 installations (which lead to some cleanup of the code)
**  19-Feb-2002 (schph01)
**    (bug 107122) Initialize the array pagetypespertabletypes with the
**    default value in case where no DOM windows are open.
**  03-Apr-2002 (schph01)
**    (BUG 107249) remove initialization for "rows" and "row width"
**    controls when the "Structure" control change.
**    Additional change :
**     - Add structure to save the value changed by the user
**       in FillFactors Data.
**     - Add ResetWithDefaultEditCtrl() function to reset controls and display
**       message
**  24-Nov-2003 (schph01)
**    (SIR 107122) change the test and message, mentioning the calculation
**    will assume a 2.65 server instead 2.6 when no DOM opened.
**  23-Jan-2004 (schph01)
**   (sir 104378) detect version 3 of Ingres, for managing
**    new features provided in this version. replaced references
**    to 2.65 with refereces to 3  in #define definitions for
**    better readability in the future
**  05-Oct-2004 (schph01)
**    (sir 113187) - Add define MAX_ROW_WIDTH_R3 and initialize nRowWidth in
**    STRUCTINFO structure with this define.
**    - Add function RetrieveTupLen4PageSize()
**    When the rows span the page size:
**    - BTREE and ISAM calculation: get the calcul with CalculateRowsSpan()
**      function and continue the calcul.
**    - Heap and Hash calculation:  return the calcul with CalculateRowsSpan()
**      function.
**  02-Feb-2006 (drivi01)
**    BUG 115691
**    Keep VDBA tools at the same version as DBMS server.  Update VDBA
**    to 9.0 level versioning. Make sure that 9.0 version string is
**    correctly interpreted by visual tools and VDBA internal version
**    is set properly upon that interpretation. 
**    
********************************************************************/

#include "dll.h"
#include "subedit.h"
#include <limits.h>
#include "dbadlg1.h"
#include "dlgres.h"
#include "catospin.h"
#include "error.h"
#include "domdata.h"
#include "getdata.h"
#include "dgerrh.h"
#include "msghandl.h"
#include "dbaginfo.h"

extern BOOL MSGContinueBox (LPCTSTR lpszMessage);

#define LEAF_OVERHEAD_V1         84
#define LEAF_OVERHEAD_V_OTHERS  112

#define TIDP_SIZE                4

typedef struct tagFILLFACTORS
{
	short nData;
	short nIndex;
	short nLeaf;
} FILLFACTORS, FAR * LPFILLFACTORS;


#define PAGE_TYPE_V1 1    /* V1 MUST be 1 because it is the value gotten from the dm34 tracepoint */
#define PAGE_TYPE_V2 2    /* V2 MUST be 2 because it is the value gotten from the dm34 tracepoint */
#define PAGE_TYPE_V3 3    /* V3 MUST be 3 because it is the value gotten from the dm34 tracepoint */
#define PAGE_TYPE_V4 4    /* V4 MUST be 4 because it is the value gotten from the dm34 tracepoint */

static int pagetypespertabletypes[12];
static int iTabRowsSpanPages[]={ 2008, 3998, 8084, 16276, 32660, 65428 };

typedef struct tagSTRUCTINFO
{
	short nStructName;
	BOOL  bCompressed;
	DWORD dwRows;
	DWORD dwUniqueKeys;
	long  nRowWidth;
	short nKeyWidth;
	DWORD dwPageSize;
	short nIngresVersion;
	int   iPageType;
	FILLFACTORS fillFactors;
} STRUCTINFO, FAR * LPSTRUCTINFO;

// Provide offsets into the info defaults
typedef enum tagINFOOFFSET
{
	STRUCT_BTREE,
	STRUCT_HASH,
	STRUCT_HEAP,
	STRUCT_ISAM
} INFOOFFSET;

#define MAX_ROWS        999999999

#define MIN_ROWS        1
#define MIN_ROW_WIDTH   1
#define MIN_PERCENT     1
#define MAX_ROW_WIDTH   32767
#define MAX_ROW_WIDTH_R3 262144

// Provide defaults for the various fillfactors and
// maximums for various other fields.
// -1 indicates the field is not relevant for that structure type.
// -1 in struct name field indicates end of the table.
STRUCTINFO infoDefaults[] =
{
	IDS_STRUCT_BTREE, FALSE, MAX_ROWS, 0xFFFF,   2008,  440, 2048, OIVERS_12, PAGE_TYPE_V1,  80,  80,  70,
	IDS_STRUCT_HASH,  FALSE, MAX_ROWS, 0xFFFF,   2008,   -1, 2048, OIVERS_12, PAGE_TYPE_V1,  50,  -1,  -1,
	IDS_STRUCT_HEAP,  FALSE, MAX_ROWS, 0xFFFF,   2008,   -1, 2048, OIVERS_12, PAGE_TYPE_V1,  -1,  -1,  -1,
	IDS_STRUCT_ISAM,  FALSE, MAX_ROWS, MAX_ROWS, 2008, 1002, 2048, OIVERS_12, PAGE_TYPE_V1,  80,  -1,  -1,
	-1  // terminator
};

// Properties used for this dialog
char SZINFODEF[] = "INFODEF";
char szErrorProp[] = "ERROR";

#define ERROR_SHOW      1
#define ERROR_NOSHOW    2

STRUCTINFO infoMin[] =
{
	IDS_STRUCT_BTREE, FALSE, MIN_ROWS,  0, MIN_ROW_WIDTH, 0, 2048, OIVERS_12, PAGE_TYPE_V1, MIN_PERCENT,  MIN_PERCENT, MIN_PERCENT,
	IDS_STRUCT_HASH,  FALSE, MIN_ROWS,  0, MIN_ROW_WIDTH, 0, 2048, OIVERS_12, PAGE_TYPE_V1, MIN_PERCENT,            0,           0, 
	IDS_STRUCT_HEAP,  FALSE, MIN_ROWS,  0, MIN_ROW_WIDTH, 0, 2048, OIVERS_12, PAGE_TYPE_V1, 0,                      0,           0,
	IDS_STRUCT_ISAM,  FALSE, MIN_ROWS,  0, MIN_ROW_WIDTH, 0, 2048, OIVERS_12, PAGE_TYPE_V1, MIN_PERCENT,            0,           0,
	-1		// terminator
};

// Set this structure up just for the spin control associations
static EDITMINMAX limits[] =
{
	IDC_ROWS,           IDC_SPINROWS,       0, 0,
	IDC_ROWWIDTH,       IDC_SPINROWWIDTH,   0, 0,
	IDC_UNIQUEKEYS,     IDC_SPINKEYS,       0, 0,
	IDC_KEYWIDTH,       IDC_SPINKEYWIDTH,   0, 0,
	IDC_DATA,           IDC_SPINDATA,       0, 0,
	IDC_INDEX,          IDC_SPINIDX,        0, 0,
	IDC_LEAF,           IDC_SPINLEAF,       0, 0,
	-1       // terminator
};

BOOL CALLBACK __export DlgSpaceCalcDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK __export SpaceCalcEnumEditCntls (HWND hwnd, LPARAM lParam);

static void OnDestroy(HWND hwnd);
static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);

static BOOL OccupyStructureControl (HWND hwnd);
static BOOL OccupyPageSizeControl (HWND hwnd);
static void UpdateControls (HWND hwnd, BOOL bResetPageSize);
static void InitialiseSpinControls (HWND hwnd);
static void InitialiseEditControls (HWND hwnd);
static void ResetWithDefaultEditCtrl(hwnd);
static INFOOFFSET GetSelectedStructure (HWND hwnd);
static void GetDialogInfo (HWND hwnd, LPSTRUCTINFO lpinfo);
static BOOL LGetEditCtrlMinMaxValue (HWND hwnd, HWND hwndCtl, DWORD FAR * lpdwMin, DWORD FAR * lpdwMax);
static void Adjust4FillFactors(HWND hwnd);
static void SetCalcError(HWND hwnd);

static void DoCalculation (HWND hwnd);
static DWORD CalculateHash(HWND hwnd, LPSTRUCTINFO lpinfo);
static DWORD CalculateIsam(HWND hwnd, LPSTRUCTINFO lpinfo);
static DWORD CalculateHeap(HWND hwnd, LPSTRUCTINFO lpinfo);
static DWORD CalculateBtree(HWND hwnd, LPSTRUCTINFO lpinfo);

static DWORD mycast(float f)
{
	DWORD dw1,dw2;
	dw1=(DWORD)(f);
	dw2=(DWORD)(f*1.000001);
	if (dw2==dw1+1)
		return dw2;
	else
		return dw1;
}
// This function force the compilator to do push and pop with the good 
static DWORD CastFloatToDW(float f)
{
	return mycast(f);
}

static int GetPageOverHead(int iPageType)
{
	switch (iPageType) {
		case PAGE_TYPE_V1:
			return 38;
		case PAGE_TYPE_V2:
		case PAGE_TYPE_V3:
		case PAGE_TYPE_V4:
			return 80;
		default:
			myerror(ERR_INGRESPAGES);
			return 0;
	}
}

static int GetRowOverHead(int iPageType)
{
	switch (iPageType) {
		case PAGE_TYPE_V1:
			return 2;
		case PAGE_TYPE_V2:
			return 28;
		case PAGE_TYPE_V3:
			return 10;
		case PAGE_TYPE_V4:
			return 8;
		default:
			myerror(ERR_INGRESPAGES);
			return 0;
	}
}

extern LPTSTR INGRESII_llDBMSInfo(LPTSTR lpszConstName, LPTSTR lpVer);

/*
**
**  Function:
**      Retrieve the maximum row size for the table page size.
**      Rows span pages if the row size is greater than the maximum row size
**      for the table page size.
**      The dbmsinfo request 'tup_len_Nk' returns zero if the specified page
**      size is not configured in the current installation and the default value in
**      iTabRowsSpanPages array is preserved.
**
**    Parameters:
**      none
**
**    Returns:
**      RES_SUCCESS
**      RES_ERR connection failed.
**
*/
#define MAX_ENUMTUPLEN 6

static int RetrieveTupLen4PageSize( void )
{
	int iret, SessType, ilocsession,dwRange;
	char connectname[MAXOBJECTNAME];
	char szValue[40];
	char szConst[MAX_ENUMTUPLEN][14] = {"tup_len_2k", "tup_len_4k",
	                                    "tup_len_8k", "tup_len_16k",
	                                    "tup_len_32k", "tup_len_64k" };

	if (GetOIVers() >= OIVERS_30)
	{
		SessType=SESSION_TYPE_CACHENOREADLOCK;

		wsprintf(connectname,"%s::iidbdb",GetVirtNodeName(GetCurMdiNodeHandle ()));
		iret = Getsession(connectname, SessType, &ilocsession);
		if (iret !=RES_SUCCESS)
			return RES_ERR;

		for (dwRange=0;dwRange<MAX_ENUMTUPLEN;dwRange++)  {
			INGRESII_llDBMSInfo(szConst[dwRange],szValue);
			if (szValue[0] != '0')
				iTabRowsSpanPages[dwRange] = my_atoi(szValue);
		}

		if (iret==RES_SUCCESS)
			iret=ReleaseSession(ilocsession, RELEASE_COMMIT);
		else
			ReleaseSession(ilocsession, RELEASE_ROLLBACK);
	}
	return RES_SUCCESS;
}

BOOL WINAPI __export DlgSpaceCalc (HWND hwndOwner)
/*
	Function:
		Given the relevant information, calculates the number of
		INGRES pages required for a database table.

	Parameters:
		hwndOwner	- Handle of the parent window for the dialog.

	Returns:
		TRUE if successful.
*/
{
	if (!IsWindow(hwndOwner))
	{
		ASSERT(NULL);
		return FALSE;
	}

	if (VdbaDialogBox (hResource, MAKEINTRESOURCE(IDD_SPACECALC), hwndOwner, DlgSpaceCalcDlgProc) == -1)
		return FALSE;
	return TRUE;
}


BOOL CALLBACK __export DlgSpaceCalcDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);

		case LM_GETEDITMINMAX:
		{
			LPEDITMINMAX lplimit = (LPEDITMINMAX) lParam;
			HWND hwndCtl = (HWND)wParam;

			ASSERT(IsWindow(hwndCtl));
			ASSERT(lplimit);

			return LGetEditCtrlMinMaxValue(hwnd, hwndCtl, &lplimit->dwMin, &lplimit->dwMax);
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
	return FALSE;
}


typedef struct tagSAVEVALUE
{
	FILLFACTORS FillFactors;
	short DefaultData;
} SAVEVALUES, FAR * LPSAVEVALUES;



static SAVEVALUES SaveFillFactors;
static BOOL bUserChangeEdit = FALSE;
static BOOL bInitEdit = FALSE;
static int oldIngresVer;
static BOOL bContinueShowMessage = TRUE;
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	HWND hwndTempDoc;
	HLOCAL hmem = LocalAlloc(LHND, sizeof(infoDefaults));
	LPSTRUCTINFO lpinfo = hmem ? LocalLock(hmem) : NULL;

	if (!lpinfo)
	{
		if (hmem)
			LocalFree(hmem);
		EndDialog(hwnd, -1);
		return TRUE;
	}

	// Since we modify infoDefaults, make a copy of it for each instance
	// of the dialog
	oldIngresVer = GetOIVers();
	SetProp(hwnd, SZINFODEF, hmem);
	_fmemcpy(lpinfo, infoDefaults, sizeof(infoDefaults));
	if (oldIngresVer >= OIVERS_30)
	{
		int i =0;
		for (i=0; i<4; i++)
		{
			lpinfo[i].nRowWidth = MAX_ROW_WIDTH_R3;
		}
	}
	LocalUnlock(hmem);

	hwndTempDoc = GetWindow(hwndMDIClient, GW_CHILD);
	if (hwndTempDoc)
	{
		if (GetOIVers()>=OIVERS_26) {
			BOOL bResult = SQLGetPageTypesPerTableTypes(GetVirtNodeName(GetCurMdiNodeHandle ()), pagetypespertabletypes);
			if (!bResult) {
				MessageWithHistoryButton(GetFocus(), ResourceString(IDS_E_CONNECTION_FAILED));
				EndDialog(hwnd, -1);
				return TRUE;
			}
			if (PAGE_TYPE_V1 != 1 || PAGE_TYPE_V2 != 2 || PAGE_TYPE_V3 != 3 || PAGE_TYPE_V4 !=4) {
				myerror(ERR_INGRESPAGES); /* the parsing of dm34 trace output (in parse.cpp) assumes V1 == 1 V2 == 2 etc... */
				EndDialog(hwnd, -1);
				return TRUE;
			}
			Button_SetCheck (GetDlgItem (hwnd, IDC_TYPE_TABLE), TRUE);
		}
		if (GetOIVers()>=OIVERS_30)
			RetrieveTupLen4PageSize();
}
	else
	{
		oldIngresVer = SetOIVers(OIVERS_90); // Simulate the current Ingres version 9.0
		Button_SetCheck (GetDlgItem (hwnd, IDC_TYPE_TABLE), TRUE);
		pagetypespertabletypes [0]  = PAGE_TYPE_V1;
		pagetypespertabletypes [1]  = PAGE_TYPE_V3;
		pagetypespertabletypes [2]  = PAGE_TYPE_V3;
		pagetypespertabletypes [3]  = PAGE_TYPE_V3;
		pagetypespertabletypes [4]  = PAGE_TYPE_V3;
		pagetypespertabletypes [5]  = PAGE_TYPE_V3;
		pagetypespertabletypes [6]  = PAGE_TYPE_V1;
		pagetypespertabletypes [7]  = PAGE_TYPE_V4;
		pagetypespertabletypes [8]  = PAGE_TYPE_V4;
		pagetypespertabletypes [9]  = PAGE_TYPE_V4;
		pagetypespertabletypes [10] = PAGE_TYPE_V4;
		pagetypespertabletypes [11] = PAGE_TYPE_V4;
		if (bContinueShowMessage)
		{
			if (MSGContinueBox (ResourceString(IDS_W_CALCULATION_WITH_DEFAULT)))
				bContinueShowMessage = FALSE;
		}
	}

	// Force the catospin.dll to load
	SpinGetVersion();

	if (!OccupyStructureControl(hwnd) || !OccupyPageSizeControl(hwnd))
	{
		EndDialog(hwnd, -1);
		return TRUE;
	}
	if (GetOIVers() >= OIVERS_20)
		lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_OIV2_SPACECALC));
	else
		lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_SPACECALC));

	SubclassAllNumericEditControls(hwnd, EC_SUBCLASS);

	InitialiseSpinControls(hwnd);
	InitialiseEditControls(hwnd);
	UpdateControls(hwnd,TRUE);
	DoCalculation(hwnd);
	richCenterDialog(hwnd);
	return TRUE;
}


static void OnDestroy(HWND hwnd)
{
	HLOCAL hmem = GetProp(hwnd, SZINFODEF);
	SetOIVers(oldIngresVer); // restore the Ingres Version stored on InitDialog

	SubclassAllNumericEditControls(hwnd, EC_RESETSUBCLASS);
	if (hmem)
	{
		LocalFree(hmem);
		RemoveProp(hwnd, SZINFODEF);
	}

	if (GetProp(hwnd, szErrorProp) != 0)
		RemoveProp(hwnd, szErrorProp);
	lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
		case IDOK:
			EndDialog(hwnd, 1);
			break;

		case IDCANCEL:
			EndDialog(hwnd, 0);
			break;

		case IDC_TYPE_TABLE:
		case IDC_TYPE_INDEX:
			if (codeNotify == BN_CLICKED) {

				SetProp(hwnd, szErrorProp, (HANDLE)ERROR_NOSHOW);

				Adjust4FillFactors(hwnd);
				InitialiseSpinControls (hwnd);
				LimitNumericEditControls(hwnd);


				DoCalculation(hwnd);

				SetProp(hwnd, szErrorProp, (HANDLE)ERROR_SHOW);
			}
			break;

		case IDC_CLEAR:
			InitialiseSpinControls(hwnd);
			InitialiseEditControls(hwnd);
			UpdateControls(hwnd,TRUE);
			Adjust4FillFactors(hwnd);
			bUserChangeEdit = FALSE;
			break;

		case IDC_PAGESIZE:
			switch (codeNotify)
			{
				case CBN_SELCHANGE:

				// Inhibit error messages while we are updating since
				// the EN_CHANGE message causes all the edit controls to be
				// verified for this structure type,  even the controls that
				// have not been reinitialised yet.

				SetProp(hwnd, szErrorProp, (HANDLE)ERROR_NOSHOW);

				Adjust4FillFactors(hwnd);
				InitialiseSpinControls (hwnd);
				LimitNumericEditControls(hwnd);


				DoCalculation(hwnd);

				SetProp(hwnd, szErrorProp, (HANDLE)ERROR_SHOW);
				break;
			}
		break;

		case IDC_STRUCTURE:
			switch (codeNotify)
			{
				case CBN_SELCHANGE:

					// Inhibit error messages while we are updating since
					// the EN_CHANGE message causes all the edit controls to be
					// verified for this structure type,  even the controls that
					// have not been reinitialised yet.

					SetProp(hwnd, szErrorProp, (HANDLE)ERROR_NOSHOW);

					InitialiseSpinControls(hwnd);
					ResetWithDefaultEditCtrl(hwnd);
					//InitialiseEditControls(hwnd);
					UpdateControls(hwnd,FALSE);
					DoCalculation(hwnd);

					SetProp(hwnd, szErrorProp, (HANDLE)ERROR_SHOW);
					break;

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

		case IDC_ROWS:
		case IDC_ROWWIDTH:
		case IDC_UNIQUEKEYS:
		case IDC_KEYWIDTH:
		case IDC_DATA:
		case IDC_INDEX:
		case IDC_LEAF:
		{
			switch (codeNotify)
			{
				case EN_CHANGE:
				{
					int nCount;
					BOOL bShowError = (GetProp(hwnd, szErrorProp) == (HANDLE)ERROR_NOSHOW) ? FALSE : TRUE;

					if (id == IDC_DATA
					 || id == IDC_INDEX
					 || id == IDC_LEAF
					 || id == IDC_ROWWIDTH)
					{
						// Do this even on INITDIALOG
						Adjust4FillFactors(hwnd);
					}

					if (!IsWindowVisible(hwnd))
						break;

					// Test to see if the control is empty. If so then
					// set ERROR and break out.  It becomes tiresome to edit
					// if you delete all characters and an error box pops up.

					nCount = Edit_GetTextLength(hwndCtl);
					if (nCount == 0)
					{
						SetCalcError(hwnd);
						break;
					}
					if (!bInitEdit && (   id == IDC_DATA
					                   || id == IDC_INDEX
					                   || id == IDC_LEAF ))
					{
						char szBuf[20];
						INFOOFFSET infoIdx = GetSelectedStructure(hwnd);
						int nData,nIndex,nLeaf;
						GetDlgItemText (hwnd, IDC_DATA, szBuf, sizeof (szBuf));
						nData = my_atoi(szBuf);

						GetDlgItemText (hwnd, IDC_INDEX, szBuf, sizeof (szBuf));
						nIndex = my_atoi(szBuf);

						GetDlgItemText (hwnd, IDC_LEAF, szBuf, sizeof (szBuf));
						nLeaf = my_atoi(szBuf);

						if ( SaveFillFactors.FillFactors.nData != nData)
						{
							SaveFillFactors.FillFactors.nData = nData;
							SaveFillFactors.DefaultData = infoDefaults[infoIdx].fillFactors.nData;
							bUserChangeEdit = TRUE;
						}
						if ( SaveFillFactors.FillFactors.nIndex != nIndex)
							SaveFillFactors.FillFactors.nIndex = nIndex;
						if ( SaveFillFactors.FillFactors.nLeaf != nLeaf)
							SaveFillFactors.FillFactors.nLeaf = nLeaf;
					}

					if (!VerifyAllNumericEditControls(hwnd, bShowError, TRUE))
						break;

					DoCalculation(hwnd);
					break;
				}

				case EN_KILLFOCUS:
				{
					HWND hwndNew = GetFocus();
					int nNewCtl = GetDlgCtrlID(hwndNew);

					if (!IsEditControl(hwnd, hwndNew)
					 || !IsChild(hwnd, hwndNew))
						// Dont recalc on any button hits or change of structure
						// or error message displays
						break;

					if (!VerifyAllNumericEditControls(hwnd, TRUE, TRUE))
						break;

					UpdateSpinButtons(hwnd);
					DoCalculation(hwnd);
					break;
				}
			}
			break;
		}

		case IDC_SPINROWWIDTH:
		case IDC_SPINKEYWIDTH:
		case IDC_SPINROWS:
		case IDC_SPINKEYS:
		case IDC_SPINDATA:
		case IDC_SPINIDX:
		case IDC_SPINLEAF:
		{
			// Handle the spin button notifications

			switch (codeNotify)
			{
				case SN_INCBUTTONUP:
				case SN_DECBUTTONUP:
				{
					switch (id)
					{
						case IDC_SPINDATA:
						case IDC_SPINIDX:
						case IDC_SPINLEAF:
						{
							// Set for no error display so we do not get
							// errors in the row width while decreasing the
							// fillfactors and therefore lowering the row
							// width maximum.  The edit box will just adjust
							// to the new maximum without showing an error.

							SetProp(hwnd, szErrorProp, (HANDLE)ERROR_NOSHOW);
							if (id == IDC_SPINDATA)
								bUserChangeEdit = TRUE;
						}
					}

					if (ProcessSpinControl(hwndCtl, codeNotify, limits))
						DoCalculation(hwnd);

					SetProp(hwnd, szErrorProp, (HANDLE)ERROR_SHOW);
					break;
				}

				default:
					ProcessSpinControl(hwndCtl, codeNotify, limits);
			}
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


static void UpdateControls (HWND hwnd, BOOL bResetPageSize)
/*
	Function:
		Enables/disables controls depending on the selected table structure

	Parameters:
		hwnd           - Handle to the dialog window
		bResetPageSize - TRUE  Select the first item in PageSize Combobox
		               - FALSE Keep the current item selected
	Returns:
		None.
*/
{
	INFOOFFSET infoIdx = GetSelectedStructure(hwnd);
	HLOCAL hmem = GetProp(hwnd, SZINFODEF);
	LPSTRUCTINFO lpinfo = LocalLock(hmem);

	EnableWindow (GetDlgItem(hwnd, IDC_SPINROWWIDTH),
	              lpinfo[infoIdx].nRowWidth == -1 ? FALSE : TRUE);
	EnableWindow (GetDlgItem(hwnd, IDC_ROWWIDTH),
	              lpinfo[infoIdx].nRowWidth == -1 ? FALSE : TRUE);
	EnableWindow (GetDlgItem(hwnd, IDC_TXT_ROWWIDTH),
	              lpinfo[infoIdx].nRowWidth == -1 ? FALSE : TRUE);

	EnableWindow (GetDlgItem(hwnd, IDC_SPINKEYWIDTH),
	              lpinfo[infoIdx].nKeyWidth == -1 ? FALSE : TRUE);
	EnableWindow (GetDlgItem(hwnd, IDC_KEYWIDTH),
	              lpinfo[infoIdx].nKeyWidth == -1 ? FALSE : TRUE);
	EnableWindow (GetDlgItem(hwnd, IDC_TXT_KEYWIDTH),
	              lpinfo[infoIdx].nKeyWidth == -1 ? FALSE : TRUE);

	EnableWindow (GetDlgItem(hwnd, IDC_SPINROWS),
	              lpinfo[infoIdx].dwRows == 0xFFFF ? FALSE : TRUE);
	EnableWindow (GetDlgItem(hwnd, IDC_ROWS),
	              lpinfo[infoIdx].dwRows == 0xFFFF ? FALSE : TRUE);
	EnableWindow (GetDlgItem(hwnd, IDC_TXT_ROWS),
	              lpinfo[infoIdx].dwRows == 0xFFFF ? FALSE : TRUE);

	EnableWindow (GetDlgItem(hwnd, IDC_SPINKEYS),
	              lpinfo[infoIdx].dwUniqueKeys == 0xFFFF ? FALSE : TRUE);
	EnableWindow (GetDlgItem(hwnd, IDC_UNIQUEKEYS),
	              lpinfo[infoIdx].dwUniqueKeys == 0xFFFF ? FALSE : TRUE);
	EnableWindow (GetDlgItem(hwnd, IDC_TXT_UNIQUEKEYS),
	              lpinfo[infoIdx].dwUniqueKeys == 0xFFFF ? FALSE : TRUE);

	EnableWindow (GetDlgItem(hwnd, IDC_SPINDATA),
	              lpinfo[infoIdx].fillFactors.nData == -1 ? FALSE : TRUE);
	EnableWindow (GetDlgItem(hwnd, IDC_DATA),
	              lpinfo[infoIdx].fillFactors.nData == -1 ? FALSE : TRUE);
	EnableWindow (GetDlgItem(hwnd, IDC_TXT_DATA),
	              lpinfo[infoIdx].fillFactors.nData == -1 ? FALSE : TRUE);

	EnableWindow (GetDlgItem(hwnd, IDC_SPINIDX),
	              lpinfo[infoIdx].fillFactors.nIndex == -1 ? FALSE : TRUE);
	EnableWindow (GetDlgItem(hwnd, IDC_INDEX),
	              lpinfo[infoIdx].fillFactors.nIndex == -1 ? FALSE : TRUE);
	EnableWindow (GetDlgItem(hwnd, IDC_TXT_INDEX),
	              lpinfo[infoIdx].fillFactors.nIndex == -1 ? FALSE : TRUE);

	EnableWindow (GetDlgItem(hwnd, IDC_SPINLEAF),
	              lpinfo[infoIdx].fillFactors.nLeaf == -1 ? FALSE : TRUE);
	EnableWindow (GetDlgItem(hwnd, IDC_LEAF),
	              lpinfo[infoIdx].fillFactors.nLeaf == -1 ? FALSE : TRUE);
	EnableWindow (GetDlgItem(hwnd, IDC_TXT_LEAF),
	              lpinfo[infoIdx].fillFactors.nLeaf == -1 ? FALSE : TRUE);

	if (bResetPageSize)
		ComboBox_SetCurSel(GetDlgItem (hwnd, IDC_PAGESIZE), 0);

	LocalUnlock(hmem);
}

static BOOL OccupyPageSizeControl (HWND hwnd)
/*
	Function:
   Ingres version == 1.x :
      Hide combobox and static name.
   Ingres version == 2.x :
      Fills the Page_size drop down box with the page size.
      

	Parameters:
		hwnd	- Handle to the dialog window.

	Returns:
		TRUE if successful.
*/
{
	BOOL bRetVal = TRUE;
	DWORD dwi;
	int nIdx;

	char szpagesize[20];
	HWND hwndCtl = GetDlgItem (hwnd, IDC_PAGESIZE);

	int ishowstate = GetOIVers() >= OIVERS_26 ? SW_SHOW : SW_HIDE;
	ShowWindow(GetDlgItem(hwnd,IDC_FRAME_PAGEORINDEX),ishowstate);
	ShowWindow(GetDlgItem(hwnd,IDC_TYPE_TABLE),ishowstate);
	ShowWindow(GetDlgItem(hwnd,IDC_TYPE_INDEX),ishowstate);

	if (GetOIVers() >= OIVERS_20) {
		ShowWindow(GetDlgItem(hwnd,IDC_STATIC_PAGESIZE),SW_SHOW);
		ShowWindow(GetDlgItem(hwnd,IDC_PAGESIZE),SW_SHOW);

		for (dwi=2048;dwi<65537;dwi=dwi*2)  {
			wsprintf(szpagesize,"%5ld",dwi);
			nIdx = ComboBox_AddString(hwndCtl,szpagesize );

			if (nIdx == CB_ERR)	{
				bRetVal = FALSE;
			break;
			}
			else
				ComboBox_SetItemData(hwndCtl, nIdx, dwi);
		}
		// Set the first string in the list as the default
		if (bRetVal)
			ComboBox_SetCurSel(hwndCtl, 0);
	}
	else {
		ShowWindow(GetDlgItem(hwnd,IDC_STATIC_PAGESIZE),SW_HIDE);
		ShowWindow(GetDlgItem(hwnd,IDC_PAGESIZE),SW_HIDE);
	}
	return bRetVal;
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
	int i = 0;
	BOOL bRetVal = TRUE;
	HWND hwndCtl = GetDlgItem (hwnd, IDC_STRUCTURE);
	HLOCAL hmem;
	LPSTRUCTINFO lpinfo;

	if (!IsWindow(hwndCtl))
	{
		ASSERT(FALSE);
		return FALSE;
	}

	hmem = GetProp(hwnd, SZINFODEF);
	lpinfo = LocalLock(hmem);	

	while (lpinfo[i].nStructName != -1)
	{
		char szStructName[50];
		int nIdx;

		if (LoadString(hResource, lpinfo[i].nStructName, szStructName, sizeof(szStructName)) == 0)
			bRetVal = FALSE;

		nIdx = ComboBox_AddString(hwndCtl, szStructName);

		if (nIdx == CB_ERR)
			bRetVal = FALSE;
		else
			ComboBox_SetItemData(hwndCtl, nIdx, (LPARAM)i);

		if (GetOIVers() >= OIVERS_20)
			lpinfo[i].nIngresVersion = GetOIVers();

		i++;
	}

	// Set the first string in the list as the default
	if (bRetVal)
		ComboBox_SetCurSel(hwndCtl, 0);

	LocalUnlock(hmem);

	return bRetVal;
}


static void InitialiseSpinControls (HWND hwnd)
{
	INFOOFFSET infoIdx = GetSelectedStructure(hwnd);
	HLOCAL hmem = GetProp(hwnd, SZINFODEF);
	LPSTRUCTINFO lpinfo = LocalLock(hmem);

	if (lpinfo[infoIdx].nRowWidth != -1)
		SpinRangeSet(GetDlgItem(hwnd, IDC_SPINROWWIDTH), infoMin[infoIdx].nRowWidth, lpinfo[infoIdx].nRowWidth);

	if (lpinfo[infoIdx].nKeyWidth != -1)
		SpinRangeSet(GetDlgItem(hwnd, IDC_SPINKEYWIDTH), infoMin[infoIdx].nKeyWidth, lpinfo[infoIdx].nKeyWidth);

	if (lpinfo[infoIdx].dwRows != 0xFFFF)
		SpinRangeSet(GetDlgItem(hwnd, IDC_SPINROWS), infoMin[infoIdx].dwRows, lpinfo[infoIdx].dwRows);

	if (lpinfo[infoIdx].dwUniqueKeys != 0xFFFF)
		SpinRangeSet(GetDlgItem(hwnd, IDC_SPINKEYS), infoMin[infoIdx].dwUniqueKeys, lpinfo[infoIdx].dwUniqueKeys);

	if (lpinfo[infoIdx].fillFactors.nData != -1)
		SpinRangeSet(GetDlgItem(hwnd, IDC_SPINDATA), infoMin[infoIdx].fillFactors.nData, 100);

	if (lpinfo[infoIdx].fillFactors.nIndex != -1)
		SpinRangeSet(GetDlgItem(hwnd, IDC_SPINIDX), infoMin[infoIdx].fillFactors.nIndex, 100);

	if (lpinfo[infoIdx].fillFactors.nLeaf != -1)
		SpinRangeSet(GetDlgItem(hwnd, IDC_SPINLEAF), infoMin[infoIdx].fillFactors.nLeaf, 100);

	SpinPositionSet(GetDlgItem(hwnd, IDC_SPINROWWIDTH), infoMin[infoIdx].nRowWidth);
	SpinPositionSet(GetDlgItem(hwnd, IDC_SPINKEYWIDTH), infoMin[infoIdx].nKeyWidth);
	SpinPositionSet(GetDlgItem(hwnd, IDC_SPINROWS), infoMin[infoIdx].dwRows);
	SpinPositionSet(GetDlgItem(hwnd, IDC_SPINKEYS), infoMin[infoIdx].dwUniqueKeys);

	SpinPositionSet(GetDlgItem(hwnd, IDC_SPINDATA), lpinfo[infoIdx].fillFactors.nData == -1 ? 0 : lpinfo[infoIdx].fillFactors.nData);
	SpinPositionSet(GetDlgItem(hwnd, IDC_SPINIDX), lpinfo[infoIdx].fillFactors.nIndex == -1 ? 0 : lpinfo[infoIdx].fillFactors.nIndex);
	SpinPositionSet(GetDlgItem(hwnd, IDC_SPINLEAF), lpinfo[infoIdx].fillFactors.nLeaf == -1 ? 0 : lpinfo[infoIdx].fillFactors.nLeaf);

	LocalUnlock(hmem);
}

static void ResetWithDefaultEditCtrl (HWND hwnd)
{
	BOOL bKeepCurrentValues = FALSE;
	INFOOFFSET infoIdx = GetSelectedStructure(hwnd);
	char szNumber[20];
	HLOCAL hmem = GetProp(hwnd, SZINFODEF);
	LPSTRUCTINFO lpinfo = LocalLock(hmem);

	if (bUserChangeEdit == TRUE)
	{
		if (infoDefaults[infoIdx].fillFactors.nData != -1 &&
			SaveFillFactors.DefaultData != infoDefaults[infoIdx].fillFactors.nData )
		{
			int iret;
			// "You have changed the data pages fillfactor.\n"
			// "The new structure you have chosen has a different default value for the fillfactor.\n"
			// "Apply the default of the new structure ?"
			iret = MessageBox(GetFocus(), ResourceString(IDS_APPLY_DEFAULT_FILLFACTOR_DATA), NULL,
			                              MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL);

			if (iret == IDNO)
				bKeepCurrentValues = TRUE;
			else
				bUserChangeEdit = FALSE;
		}
		else
			bKeepCurrentValues = TRUE;
	}
	else
	{
		if ( infoDefaults[infoIdx].fillFactors.nIndex != -1 &&
			 SaveFillFactors.FillFactors.nIndex != -1 &&
			 SaveFillFactors.FillFactors.nIndex != infoDefaults[infoIdx].fillFactors.nIndex)
				bKeepCurrentValues = TRUE;
		if ( infoDefaults[infoIdx].fillFactors.nLeaf != -1 &&
			 SaveFillFactors.FillFactors.nLeaf != -1 &&
			 SaveFillFactors.FillFactors.nLeaf != infoDefaults[infoIdx].fillFactors.nLeaf)
				bKeepCurrentValues = TRUE;

	}
	bInitEdit = TRUE;

	if (bKeepCurrentValues)
	{
		my_itoa(SaveFillFactors.FillFactors.nData, szNumber, 10);
		Edit_SetText (GetDlgItem(hwnd, IDC_DATA), szNumber);
		my_itoa(SaveFillFactors.FillFactors.nIndex, szNumber, 10);
		Edit_SetText (GetDlgItem(hwnd, IDC_INDEX), szNumber);
		my_itoa(SaveFillFactors.FillFactors.nLeaf, szNumber, 10);
		Edit_SetText (GetDlgItem(hwnd, IDC_LEAF), szNumber);
	}
	else
	{
		if (lpinfo[infoIdx].fillFactors.nData != -1)
		{
			my_itoa(lpinfo[infoIdx].fillFactors.nData, szNumber, 10);
			Edit_SetText (GetDlgItem(hwnd, IDC_DATA), szNumber);
		}
		if (lpinfo[infoIdx].fillFactors.nIndex != -1)
		{
			my_itoa(lpinfo[infoIdx].fillFactors.nIndex, szNumber, 10);
			Edit_SetText (GetDlgItem(hwnd, IDC_INDEX), szNumber);
		}
		if (lpinfo[infoIdx].fillFactors.nLeaf != -1)
		{
			my_itoa(lpinfo[infoIdx].fillFactors.nLeaf, szNumber, 10);
			Edit_SetText (GetDlgItem(hwnd, IDC_LEAF), szNumber);
		}
	}
	bInitEdit = FALSE;
	LimitNumericEditControls(hwnd);

	LocalUnlock(hmem);
}


static void InitialiseEditControls (HWND hwnd)
{
	INFOOFFSET infoIdx = GetSelectedStructure(hwnd);
	char szNumber[20];
	HLOCAL hmem = GetProp(hwnd, SZINFODEF);
	LPSTRUCTINFO lpinfo = LocalLock(hmem);

	bUserChangeEdit = FALSE;
	bInitEdit = TRUE;
	my_dwtoa(infoMin[infoIdx].dwRows, szNumber, 10);
	Edit_Enable(GetDlgItem(hwnd, IDC_ROWS), FALSE);
	Edit_SetText (GetDlgItem(hwnd, IDC_ROWS), szNumber);
	Edit_Enable(GetDlgItem(hwnd, IDC_ROWS), TRUE);

	my_itoa(infoMin[infoIdx].nRowWidth, szNumber, 10);
	Edit_SetText (GetDlgItem(hwnd, IDC_ROWWIDTH), szNumber);

	my_dwtoa(infoMin[infoIdx].dwUniqueKeys, szNumber, 10);
	Edit_SetText (GetDlgItem(hwnd, IDC_UNIQUEKEYS), szNumber);

	my_itoa(infoMin[infoIdx].nKeyWidth, szNumber, 10);
	Edit_SetText (GetDlgItem(hwnd, IDC_KEYWIDTH), szNumber);

	if (lpinfo[infoIdx].fillFactors.nData != -1)
		my_itoa(lpinfo[infoIdx].fillFactors.nData, szNumber, 10);
	else
		my_itoa(infoMin[infoIdx].fillFactors.nData, szNumber, 10);
	Edit_SetText (GetDlgItem(hwnd, IDC_DATA), szNumber);

	if (lpinfo[infoIdx].fillFactors.nIndex != -1)
		my_itoa(lpinfo[infoIdx].fillFactors.nIndex, szNumber, 10);
	else
		my_itoa(infoMin[infoIdx].fillFactors.nIndex, szNumber, 10);
	Edit_SetText (GetDlgItem(hwnd, IDC_INDEX), szNumber);

	if (lpinfo[infoIdx].fillFactors.nLeaf != -1)
		my_itoa(lpinfo[infoIdx].fillFactors.nLeaf, szNumber, 10);
	else
		my_itoa(infoMin[infoIdx].fillFactors.nLeaf, szNumber, 10);
	Edit_SetText (GetDlgItem(hwnd, IDC_LEAF), szNumber);

	if (lpinfo[infoIdx].fillFactors.nData != -1)
		SaveFillFactors.FillFactors.nData = lpinfo[infoIdx].fillFactors.nData;
	if (lpinfo[infoIdx].fillFactors.nIndex != -1)
		SaveFillFactors.FillFactors.nIndex = lpinfo[infoIdx].fillFactors.nIndex;
	if (lpinfo[infoIdx].fillFactors.nLeaf != -1)
		SaveFillFactors.FillFactors.nLeaf = lpinfo[infoIdx].fillFactors.nLeaf;

	bInitEdit = FALSE;

	LimitNumericEditControls(hwnd);

	LocalUnlock(hmem);
}


static INFOOFFSET GetSelectedStructure (HWND hwnd)
{
	HWND hwndCtl = GetDlgItem (hwnd, IDC_STRUCTURE);
	return (INFOOFFSET)ComboBox_GetItemData(hwndCtl, ComboBox_GetCurSel(hwndCtl));
}


static void DoCalculation (HWND hwnd)
{
	INFOOFFSET infoIdx = GetSelectedStructure(hwnd);
	STRUCTINFO info;
	DWORD dwTotalPages = 0;
	char szNumber[50];

	// Verify all controls are within their limits.

	if (!VerifyAllNumericEditControls(hwnd, TRUE, TRUE))
		return;

	info.nIngresVersion= GetOIVers();

	GetDialogInfo (hwnd, &info);

	switch (infoIdx)
	{
		case STRUCT_HASH:
			dwTotalPages = CalculateHash(hwnd, &info);
			break;

		case STRUCT_ISAM:
			dwTotalPages = CalculateIsam(hwnd, &info);
			break;

		case STRUCT_HEAP:
			dwTotalPages = CalculateHeap(hwnd, &info);
			break;

		case STRUCT_BTREE:
			dwTotalPages = CalculateBtree(hwnd, &info);
			break;

		default:
			ASSERT(NULL);
	}

	if (dwTotalPages != (DWORD)-1)
	{
		// Update the results display

		double dBytes;

		my_dwtoa(dwTotalPages, szNumber, 10);
		Static_SetText(GetDlgItem(hwnd, IDC_INGRESPAGES), szNumber);

		dBytes = (double)dwTotalPages * (double)info.dwPageSize; // PS 
		sprintf (szNumber, "%.0f", dBytes);
		Static_SetText(GetDlgItem(hwnd, IDC_BYTES), szNumber);
	}
	else
	{
		SetCalcError(hwnd);
	}
}
/*
**
**  Function:
**      The following function determines the amount of space needed to
**      store data in a table when rows span pages.
**
**    Parameters:
**      LPSTRUCTINFO - struture
**
**    Returns:
**      -1 when the Page Size is unknown or data page value.
**
*/
static DWORD CalculateRowsSpan (LPSTRUCTINFO lpinfo)
{
	long iPagesPerRow ,itemp1;
		switch (lpinfo->dwPageSize)
		{
			case 2048:
				iPagesPerRow = lpinfo->nRowWidth / iTabRowsSpanPages[0];
				itemp1 = lpinfo->nRowWidth % iTabRowsSpanPages[0];
				break;
			case 4096:
				iPagesPerRow = lpinfo->nRowWidth / iTabRowsSpanPages[1];
				itemp1 = lpinfo->nRowWidth % iTabRowsSpanPages[1];
				break;
			case 8192:
				iPagesPerRow = lpinfo->nRowWidth / iTabRowsSpanPages[2];
				itemp1 = lpinfo->nRowWidth % iTabRowsSpanPages[2];
				break;
			case 16384:
				iPagesPerRow = lpinfo->nRowWidth / iTabRowsSpanPages[3];
				itemp1 = lpinfo->nRowWidth % iTabRowsSpanPages[3];
			case 32768:
				iPagesPerRow = lpinfo->nRowWidth / iTabRowsSpanPages[4];
				itemp1 = lpinfo->nRowWidth % iTabRowsSpanPages[4];
				break;
			case 65536:
				iPagesPerRow = lpinfo->nRowWidth / iTabRowsSpanPages[5];
				itemp1 = lpinfo->nRowWidth % iTabRowsSpanPages[5];
				break;
			default:
				return (DWORD)-1;
		}
		if (itemp1 != 0)
			iPagesPerRow++;
		return iPagesPerRow*lpinfo->dwRows;
}
static DWORD CalculateHash(HWND hwnd, LPSTRUCTINFO lpinfo)
{
	DWORD dwRowsPerPage;
	DWORD dwTotalHashPages;
	DWORD dwPageFreeSpace;
	DWORD dwTupleOverhead;
	float fTotalHashPages;

	// Vut Nov 2' 1995 
	float fillFactor = (float)lpinfo->fillFactors.nData;
	// PS + Emb Dec. 9, 96: test asap
	if (fillFactor == 0)
		return (DWORD)-1;

	dwTupleOverhead = GetRowOverHead(lpinfo->iPageType);
	dwPageFreeSpace = lpinfo->dwPageSize - GetPageOverHead(lpinfo->iPageType);

	// Determine the number of rows that will fit on an INGRES page and
	// round down to the nearest integer.

	//dwRowsPerPage = (DWORD)((fillFactor * (float)INGRES_DATA_PAGE) / ((float)lpinfo->nRowWidth + (float)ROW_WIDTH_ADJ));

	// Vut Nov 2' 1995
	//dwRowsPerPage = (DWORD)((float)2008 / ((float)lpinfo->nRowWidth + (float)ROW_WIDTH_ADJ));
	dwRowsPerPage = dwPageFreeSpace / (lpinfo->nRowWidth + dwTupleOverhead ) ;
	if (dwRowsPerPage == 0)
	{
		if (lpinfo->nIngresVersion >= OIVERS_30)
			return CalculateRowsSpan(lpinfo);
		else
			return (DWORD)-1;
	}

	// Calculate the number of pages needed and round up to the
	// nearest integer

	//fTotalHashPages = ((float)lpinfo->dwRows / (float)dwRowsPerPage) * ((float)100 / fillFactor);
	fTotalHashPages = ((float)lpinfo->dwRows / dwRowsPerPage) * (100 / fillFactor);
	dwTotalHashPages = (DWORD)fTotalHashPages;
	if (fTotalHashPages - (float)dwTotalHashPages != 0)
		dwTotalHashPages++;

	return dwTotalHashPages;
}


static DWORD CalculateIsam(HWND hwnd, LPSTRUCTINFO lpinfo)
{
	DWORD dwFree;
	DWORD dwDataPages;
	DWORD dwKeysPerPage;
	DWORD dwRowsPerPage;
	DWORD dwIndexPages;
	DWORD dwTotalIsamPages;
	DWORD dwPageFreeSpace;
	DWORD dwTupleOverhead;
	float fillFactor = (float)lpinfo->fillFactors.nData / 100;
	HLOCAL hmem = GetProp(hwnd, SZINFODEF);
	LPSTRUCTINFO lpprop = LocalLock(hmem);

	ASSERT (lpinfo->nKeyWidth <= lpprop[STRUCT_ISAM].nKeyWidth);

	LocalUnlock(hmem);

	// Determine the number of rows that will fit on an INGRES page and
	// round down to the nearest integer.
	dwTupleOverhead = GetRowOverHead(lpinfo->iPageType);
	dwPageFreeSpace = lpinfo->dwPageSize - GetPageOverHead(lpinfo->iPageType);

	//	dwRowsPerPage = (DWORD)((fillFactor * (float)2008) / ((float)lpinfo->nRowWidth + (float)ROW_WIDTH_ADJ));
	//dwRowsPerPage = (DWORD)((fillFactor * (float)dwPageFreeSpace) / ((float)lpinfo->nRowWidth + (float)dwTupleOverhead));
	dwRowsPerPage = CastFloatToDW((fillFactor * dwPageFreeSpace) / (lpinfo->nRowWidth + dwTupleOverhead));
	if (dwRowsPerPage == 0) {
		if (lpinfo->nIngresVersion >= OIVERS_30)
			dwDataPages =  CalculateRowsSpan(lpinfo);
		else
			return (DWORD)-1;
	}
	else {
		// Calculate the amount of free space left on a data page to
		// determine if dwRowsPerPage can be incremented by one.

		//	dwFree = 2048 - (dwRowsPerPage * (lpinfo->nRowWidth + 2));
		//	dwFree = lpinfo->dwPageSize - (dwRowsPerPage * (lpinfo->nRowWidth + dwTupleOverhead));
		dwFree = dwPageFreeSpace - (dwRowsPerPage * (lpinfo->nRowWidth + dwTupleOverhead));

		//	if (((float)dwFree > ((float)2048 - (fillFactor * (float)2048)))
		if (((float)dwFree > ((float)lpinfo->dwPageSize - (fillFactor * lpinfo->dwPageSize)))
		&& ((DWORD)lpinfo->nRowWidth <= dwFree))
			dwRowsPerPage++;

		if (dwRowsPerPage > 512)
			dwRowsPerPage = 512;

		// Determine the number of data pages needed and round up to the
		// nearest integer.
		dwDataPages = lpinfo->dwRows / dwRowsPerPage;
		//	dwDataPages = (DWORD)((float)lpinfo->dwRows / (float)dwRowsPerPage);
		if (lpinfo->dwRows % dwRowsPerPage != 0)
			dwDataPages++;

		// Determine the number of keys per index page and round down to
		// the nearest integer.
	}

	//dwKeysPerPage = (DWORD)((float)dwPageFreeSpace / ((float)lpinfo->nKeyWidth + (float)dwTupleOverhead));
	dwKeysPerPage = dwPageFreeSpace / (lpinfo->nKeyWidth + dwTupleOverhead);

	// Calculate the number of index pages needed and round up to the
	// nearest integer

	if (dwRowsPerPage == 0)
	{
		dwIndexPages = lpinfo->dwRows / dwKeysPerPage;
		if (lpinfo->dwRows % dwKeysPerPage != 0)
			dwIndexPages++;
	}
	else
	{
		//dwIndexPages = (DWORD)((float)dwDataPages / (float)dwKeysPerPage) + 1;
		dwIndexPages = dwDataPages / dwKeysPerPage;
		if (dwDataPages % dwKeysPerPage != 0)
			dwIndexPages++;
	}
	// Calculate the total pages needed.

	dwTotalIsamPages = dwIndexPages + dwDataPages;

	return dwTotalIsamPages;
}


static DWORD CalculateHeap(HWND hwnd, LPSTRUCTINFO lpinfo)
{
	DWORD dwRowsPerPage;
	DWORD dwTotalHeapPages;
	DWORD dwPageFreeSpace;
	DWORD dwTupleOverhead;

	dwTupleOverhead = GetRowOverHead(lpinfo->iPageType);
	dwPageFreeSpace = lpinfo->dwPageSize - GetPageOverHead(lpinfo->iPageType);

	// Determine the number of rows that will fit on an INGRES page and
	// round down to the nearest integer.
	// dwRowsPerPage = (DWORD)((float)INGRES_DATA_PAGE / ((float)lpinfo->nRowWidth + (float)ROW_WIDTH_ADJ));
	// IngresDataPage = lpinfo->dwPageSize-INGRES_RESERVED_BYTES_V2-INGRES_ROW_BYTES;

	dwRowsPerPage  = (dwPageFreeSpace / (lpinfo->nRowWidth + dwTupleOverhead));

	if (dwRowsPerPage == 0)
	{
		if (lpinfo->nIngresVersion >= OIVERS_30)
			return CalculateRowsSpan( lpinfo);
		else
			return (DWORD)-1;
	}
	// Calculate the total pages needed and round up if necessary.

	dwTotalHeapPages = (lpinfo->dwRows / dwRowsPerPage);
	if ((lpinfo->dwRows % dwRowsPerPage) != 0)
		dwTotalHeapPages++;

	return dwTotalHeapPages;
}


static DWORD CalculateBtree(HWND hwnd, LPSTRUCTINFO lpinfo)
{
	DWORD dwRowsPerPage;
	DWORD dwFree;
	DWORD dwKeysPerLPage;
	DWORD dwKeysPerIPage;
	DWORD dwMaxKeys;
	DWORD dwLeafPages;
	DWORD dwDataPages;
	DWORD dwSprigPages;
	DWORD dwIndexPages;
	DWORD dwTotalBtreePages;
	DWORD dwTupleOverhead;
	DWORD dwPageFreeSpace;
	DWORD dwLeafFreeSpace;
	int nLeafPageRem;

	float dataFill = (float)lpinfo->fillFactors.nData / 100;
	float leafFill = (float)lpinfo->fillFactors.nLeaf / 100;
	float indexFill = (float)lpinfo->fillFactors.nIndex / 100;
	HLOCAL hmem = GetProp(hwnd, SZINFODEF);
	LPSTRUCTINFO lpprop = LocalLock(hmem);

	ASSERT (lpinfo->nKeyWidth <= lpprop[STRUCT_BTREE].nKeyWidth);

	LocalUnlock(hmem);

	// Determine the number of rows that will fit on an INGRES page and
	// round down to the nearest integer.

	dwTupleOverhead = GetRowOverHead(lpinfo->iPageType);
	dwPageFreeSpace = lpinfo->dwPageSize - GetPageOverHead(lpinfo->iPageType);

	dwRowsPerPage = CastFloatToDW( (dataFill * dwPageFreeSpace) / (lpinfo->nRowWidth + dwTupleOverhead) );

	if (dwRowsPerPage == 0L)
	{
		if (lpinfo->nIngresVersion >= OIVERS_30)
			dwDataPages = CalculateRowsSpan( lpinfo);
		else
			return (DWORD)-1;
	}
	else
	{
		// Determine the amount of free space left on a data page to determine
		// if dwRowsPerPage can be incremented by 1.
		//   dwFree = 2008 - (dwRowsPerPage * (lpinfo->nRowWidth + 2));
		dwFree = dwPageFreeSpace - (dwRowsPerPage * (lpinfo->nRowWidth + dwTupleOverhead));

		//if (((float)dwFree > ((float)lpinfo->dwPageSize - (dataFill * (float)lpinfo->dwPageSize)))
		//&& ((DWORD)lpinfo->nRowWidth <= dwFree))
		if (((float)dwFree > ((float)lpinfo->dwPageSize - (dataFill * lpinfo->dwPageSize)))
		&& ((DWORD)lpinfo->nRowWidth <= dwFree))
			dwRowsPerPage++;

		if (dwRowsPerPage > 512)
			dwRowsPerPage = 512;
	}

	if (lpinfo->iPageType == PAGE_TYPE_V1)
		dwLeafFreeSpace = lpinfo->dwPageSize - LEAF_OVERHEAD_V1;
	else
		dwLeafFreeSpace = lpinfo->dwPageSize - LEAF_OVERHEAD_V_OTHERS;

	dwMaxKeys = (dwLeafFreeSpace / (lpinfo->nKeyWidth + TIDP_SIZE + dwTupleOverhead))-2 ;

	// Calculate the number of keys per leaf page by adjusting for the
	// leaf fill factor and round down to the nearest integer.  This value
	// cannot be less than two.

	dwKeysPerLPage = CastFloatToDW((float)dwMaxKeys * leafFill);

	if (dwKeysPerLPage < 2)
		dwKeysPerLPage = 2;

	// Calculate the number of keys per index page by adjusting for the
	// index fill factor and round down to the nearest integer.  This value
	// cannot be less than two.
	dwKeysPerIPage = CastFloatToDW((float)dwMaxKeys * indexFill);
	if (dwKeysPerIPage < 2)
		dwKeysPerIPage = 2;

	// Determine the number of leaf pages needed for the btree table and
	// round down to the nearest integer.  Save the remainder for future
	// use.

	dwLeafPages = (lpinfo->dwRows / dwKeysPerLPage);
	nLeafPageRem = (int)(lpinfo->dwRows % dwKeysPerLPage);

	// Determine the number of data pages needed and round up to the
	// nearest integer.

	if ( dwRowsPerPage == 0L )
	{
		if (nLeafPageRem > 0)
			dwLeafPages++;
	}
	else
	{
		DWORD dwTemp;
		dwTemp = dwKeysPerLPage / dwRowsPerPage;
		// Round the division up.
		if (dwKeysPerLPage % dwRowsPerPage != 0)
			dwTemp++;
		dwDataPages = dwLeafPages * dwTemp;
		
		// If there is a remainder from the leaf page calculation, adjust the
		// number of leaf and data pages.

		if (nLeafPageRem > 0)
		{
			DWORD dwTemp;
			dwLeafPages++;
			dwTemp = (DWORD)((float)nLeafPageRem / (float)dwRowsPerPage);
			// Round the division up.
			if ((DWORD)nLeafPageRem % dwRowsPerPage != 0)
				dwTemp++;
			dwDataPages += dwTemp;
		}
	}

	// Determine the number of sprig pages, ie. the number of index pages
	// that have leaf pages as their lower level.

	if (dwLeafPages <= dwKeysPerIPage)
		dwSprigPages = 0;
	else
	{
		// Round division up.....

		dwSprigPages = dwLeafPages / dwKeysPerIPage;
		if (dwLeafPages % dwKeysPerIPage != 0)
			dwSprigPages++;
	}

	// Determine the number of index pages.

	dwIndexPages = 0;

	if (dwSprigPages > dwKeysPerIPage)
	{
		DWORD dwTemp = dwSprigPages;

		do
		{
			// ???? Does this division get rounded up or down ????
			// ???? Go with up for now                        ????
			dwTemp = dwTemp / dwKeysPerIPage;
			if (dwTemp % dwKeysPerIPage != 0)
				dwTemp++;
			dwIndexPages += dwTemp;
		} while (dwTemp > dwKeysPerIPage);
	}

	// Calculate the total pages needed.

	dwTotalBtreePages = dwDataPages + dwLeafPages
	                  + dwSprigPages + dwIndexPages + 2;

	return dwTotalBtreePages;
}


static void GetDialogInfo (HWND hwnd, LPSTRUCTINFO lpinfo)
{
	char szBuf[100];

	GetDlgItemText (hwnd, IDC_ROWS, szBuf, sizeof (szBuf));
	lpinfo->dwRows = my_atodw(szBuf);

	GetDlgItemText (hwnd, IDC_ROWWIDTH, szBuf, sizeof (szBuf));
	lpinfo->nRowWidth = my_atoi(szBuf);

	GetDlgItemText (hwnd, IDC_UNIQUEKEYS, szBuf, sizeof (szBuf));
	lpinfo->dwUniqueKeys = my_atodw(szBuf);

	GetDlgItemText (hwnd, IDC_KEYWIDTH, szBuf, sizeof (szBuf));
	lpinfo->nKeyWidth = my_atoi(szBuf);

	GetDlgItemText (hwnd, IDC_DATA, szBuf, sizeof (szBuf));
	lpinfo->fillFactors.nData = my_atoi(szBuf);

	GetDlgItemText (hwnd, IDC_INDEX, szBuf, sizeof (szBuf));
	lpinfo->fillFactors.nIndex = my_atoi(szBuf);

	GetDlgItemText (hwnd, IDC_LEAF, szBuf, sizeof (szBuf));
	lpinfo->fillFactors.nLeaf = my_atoi(szBuf);



	if (lpinfo->nIngresVersion != OIVERS_12)   {
		HWND hwndCtl = GetDlgItem (hwnd, IDC_PAGESIZE);
		lpinfo->dwPageSize = ComboBox_GetItemData(hwndCtl, ComboBox_GetCurSel(hwndCtl));
	}
	else
		lpinfo->dwPageSize = 2048;

	if (lpinfo->nIngresVersion >= OIVERS_26)   {
		BOOL bIndex = Button_GetCheck (GetDlgItem (hwnd, IDC_TYPE_INDEX));
		int ipos = 0;
		long lpagesize = lpinfo->dwPageSize;
		while (lpagesize>2048) {
			ipos++;
			lpagesize/=2;
		}
		if (ipos>5) {
			myerror(ERR_INGRESPAGES);
			ipos=0;
		}
		if (bIndex)
			ipos+=6;
		lpinfo->iPageType = pagetypespertabletypes[ipos];
	}
	else if (lpinfo->nIngresVersion >= OIVERS_20)   {
		if (lpinfo->dwPageSize == 2048)
			lpinfo->iPageType = PAGE_TYPE_V1;
		else
			lpinfo->iPageType = PAGE_TYPE_V2;
	}
	else /* before 2.0 */
			lpinfo->iPageType = PAGE_TYPE_V1;

}


static BOOL LGetEditCtrlMinMaxValue (HWND hwnd, HWND hwndCtl, DWORD FAR * lpdwMin, DWORD FAR * lpdwMax)
{
	int id = GetDlgCtrlID(hwndCtl);
	INFOOFFSET infoIdx = GetSelectedStructure(hwnd);
	HLOCAL hmem = GetProp(hwnd, SZINFODEF);
	LPSTRUCTINFO lpinfo = LocalLock(hmem);

	switch (id)
	{
		case IDC_ROWS:
			*lpdwMin = infoMin[infoIdx].dwRows;
			*lpdwMax = lpinfo[infoIdx].dwRows;
			break;

		case IDC_ROWWIDTH:
			*lpdwMin = infoMin[infoIdx].nRowWidth;
			*lpdwMax = lpinfo[infoIdx].nRowWidth;
			break;

		case IDC_UNIQUEKEYS:
			*lpdwMin = infoMin[infoIdx].dwUniqueKeys;
			*lpdwMax = lpinfo[infoIdx].dwUniqueKeys;
			break;

		case IDC_KEYWIDTH:
			*lpdwMin = infoMin[infoIdx].nKeyWidth;
			*lpdwMax = lpinfo[infoIdx].nKeyWidth;
			break;

		case IDC_DATA:
			*lpdwMin = infoMin[infoIdx].fillFactors.nData;
			*lpdwMax = 100;
			break;

		case IDC_INDEX:
			*lpdwMin = infoMin[infoIdx].fillFactors.nIndex;
			*lpdwMax = 100;
			break;

		case IDC_LEAF:
			*lpdwMin = infoMin[infoIdx].fillFactors.nLeaf;
			*lpdwMax = 100;
			break;

		default:
			ASSERT(NULL);
			*lpdwMin = 0;
			*lpdwMax = 0;
			LocalUnlock(hmem);
			return FALSE;
	}
	LocalUnlock(hmem);
	return TRUE;
}



static void Adjust4FillFactors(HWND hwnd)
/*
	Function:
		Adjusts the maximum values of limits that are affected by the
		changes in fill factors.

	Parameters:
		hwnd		- Handle of dialog window

	Returns:
		None.
*/
{
	INFOOFFSET infoIdx = GetSelectedStructure(hwnd);
	STRUCTINFO info;
	HLOCAL hmem = GetProp(hwnd, SZINFODEF);
	LPSTRUCTINFO lpinfo = LocalLock(hmem);

	info.nIngresVersion= GetOIVers();

	GetDialogInfo(hwnd, &info);

	/* 
	** BUG #112863 : in r3 the row width can be 256K and row can span page!
	*/
	if (info.nIngresVersion < OIVERS_30)
	{
		if (info.dwPageSize == 65536L)
			lpinfo[infoIdx].nRowWidth = 32767L;
		else
			lpinfo[infoIdx].nRowWidth = info.dwPageSize - GetPageOverHead(info.iPageType) - GetRowOverHead(info.iPageType);
	}

	LocalUnlock(hmem);
}


static void SetCalcError(HWND hwnd)
{
	char szErrNotify[50];

	if (LoadString(hResource, IDS_CALCERRNOTIFY, szErrNotify, sizeof(szErrNotify)) > 0)
	{
		Static_SetText(GetDlgItem(hwnd, IDC_INGRESPAGES), szErrNotify);
		Static_SetText(GetDlgItem(hwnd, IDC_BYTES), szErrNotify);
	}

}

