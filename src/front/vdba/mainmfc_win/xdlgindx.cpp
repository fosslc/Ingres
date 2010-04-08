/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : xdlgindx.cpp, Implementation File 
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut, Detail implementation.
**    Purpose  : Popup Dialog Box for Creating indices.
**
**  History after 04-May-1999:
**    06-Jan-2000 (schph01)
**      bug #99898
**    25-Jan-2000 (schph01)
**      bug #97680 Additional change
**      Add a quote on key column name when the syntax is generated
**    24-Feb-2000 (uk$so01)
**      bug #100560. Disable the Parallel index creation and index enhancement
**      if not running against the DBMS 2.5 or later.
**    27-Mar-2000 (schph01)
**      bug #101032 Remove the parameter added to the method
**      OnSelchangeComboIndexStructure() in the message map ON_CBN_SELCHANGE
**      for the bug #99898.
**    30-Mar-2000 (schph01)
**      bug #101104 Change the column width to force an refresh
**      display.
**    26-Mar-2001 (noifr01)
**      (sir 104270) removal of code for managing Ingres/Desktop
**    30-Mar-2001 (noifr01)
**      (sir 104378) differentiation of II 2.6.
** 02-May-2001 (uk$so01)
**    SIR #102678
**    Support the composite histogram of base table and secondary index.
**/

#include "stdafx.h"
#include "mainmfc.h"
#include "xdlgindx.h"
#include "sqlkeywd.h"
#include "dlgres.h"
//
// use the function Table_QueryStatisticColumns to get base table columns:
#include "parse.h"

#if !defined (BUFSIZE)
#define BUFSIZE 256
#endif
extern "C"
{
#include "dbaset.h"
#include "tools.h"
#include "getdata.h"
#include "domdata.h"
#include "msghandl.h"
#include "dbaginfo.h"
}


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CxDlgCreateIndex dialog

void CxDlgCreateIndex::SetCreateParam(DMLCREATESTRUCT* pParam)
{
	ASSERT (pParam);
	if (!pParam)
		return;
	m_pParam = pParam;
	m_strDatabase  = m_pParam->tchszDatabase;
	m_strTable     = m_pParam->tchszObject;
	m_strTableOwner= m_pParam->tchszObjectOwner;
}


CxDlgCreateIndex::CxDlgCreateIndex(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgCreateIndex::IDD, pParent)
{
	if (!m_cButtonUp.LoadBitmaps  (BTN_UP_UP,   BTN_UP_DOWN,   BTN_UP_FOCUS,   BTN_UP_GREY)   ||
		!m_cButtonDown.LoadBitmaps(BTN_DOWN_UP, BTN_DOWN_DOWN, BTN_DOWN_FOCUS, BTN_DOWN_GREY))
	{
		TRACE0("Failed to load bitmaps for buttons\n");
		AfxThrowResourceException();
	}
	
	m_pParam       = NULL;
	m_nNameSeq     = 1;
	m_strDatabase  = _T("");
	m_strTable     = _T("");
	m_strTableOwner= _T("");
	m_strIIDatabase= _T("ii_database");
	//{{AFX_DATA_INIT(CxDlgCreateIndex)
	m_bCompressData = FALSE;
	m_bCompressKey = FALSE;
	m_bPersistence = TRUE;
	m_nPageSize = 0;
	m_nStructure = 1;
	m_strIndexName = _T("");
	m_strMinPage = _T("");
	m_strMaxPage = _T("");
	m_strLeaffill = _T("");
	m_strNonleaffill = _T("");
	m_strAllocation = _T("");
	m_strExtend = _T("");
	m_strFillfactor = _T("");
	m_strMaxX = _T("");
	m_strMaxY = _T("");
	m_strMinX = _T("");
	m_strMinY = _T("");
	//}}AFX_DATA_INIT

	CaIndexStructure* pStruct = NULL;
	pStruct = new CaIndexStructure (IDX_ISAM, _T("Isam"));
	pStruct->AddComponent (_T("Allocation"),    4, 8388607, CaComponentValueConstraint::CONSTRAINT_EDITNUMBER, _T("4"));  // [4, 8388607]
	pStruct->AddComponent (_T("Extend"),        1, 8388607, CaComponentValueConstraint::CONSTRAINT_EDITNUMBER, _T("16")); // [1, 8388607]
	pStruct->AddComponent (_T("Fillfactor"),    1, 100,     _T("80"));
//	pStruct->AddComponent (m_strUnique,            CaComponentValueConstraint::CONSTRAINT_COMBO, _T("row"));
//	pStruct->AddComponent (_T("Data Compression"), CaComponentValueConstraint::CONSTRAINT_CHECK, _T("FALSE"));
	m_listStructure.AddTail (pStruct);

	pStruct = new CaIndexStructure (IDX_BTREE, _T("Btree"));
	pStruct->AddComponent (_T("Leaffill"),      1, 100,     _T("80"));
	pStruct->AddComponent (_T("Non Leaffill"),  1, 100,     _T("70"));
	pStruct->AddComponent (_T("Allocation"),    4, 8388607, CaComponentValueConstraint::CONSTRAINT_EDITNUMBER, _T("4"));  // [4, 8388607]
	pStruct->AddComponent (_T("Extend"),        1, 8388607, CaComponentValueConstraint::CONSTRAINT_EDITNUMBER, _T("16")); // [1, 8388607]
	pStruct->AddComponent (_T("Fillfactor"),    1, 100,     _T("80"));
//	pStruct->AddComponent (m_strUnique,           CaComponentValueConstraint::CONSTRAINT_COMBO, _T("row"));
//	pStruct->AddComponent (_T("Key Compression"), CaComponentValueConstraint::CONSTRAINT_CHECK, _T("FALSE"));
	m_listStructure.AddTail (pStruct);

	pStruct = new CaIndexStructure (IDX_HASH, _T("Hash"));
	pStruct->AddComponent (_T("Min Page"),      1, 8388607, _T("16"));
	pStruct->AddComponent (_T("Max Page"),      1, 8388607, _T("8388607"));
	pStruct->AddComponent (_T("Allocation"),    4, 8388607, CaComponentValueConstraint::CONSTRAINT_EDITNUMBER, _T("4"));  // [4, 8388607]
	pStruct->AddComponent (_T("Extend"),        1, 8388607, CaComponentValueConstraint::CONSTRAINT_EDITNUMBER, _T("16")); // [1, 8388607]
	pStruct->AddComponent (_T("Fillfactor"),    1, 100,     _T("50"));
//	pStruct->AddComponent (m_strUnique,            CaComponentValueConstraint::CONSTRAINT_COMBO, _T("row"));
//	pStruct->AddComponent (_T("Data Compression"), CaComponentValueConstraint::CONSTRAINT_CHECK, _T("FALSE"));
	m_listStructure.AddTail (pStruct);

	pStruct = new CaIndexStructure (IDX_RTREE, _T("Rtree"));
	pStruct->AddComponent (_T("Min X"), CaComponentValueConstraint::CONSTRAINT_EDIT);
	pStruct->AddComponent (_T("Max X"), CaComponentValueConstraint::CONSTRAINT_EDIT);
	pStruct->AddComponent (_T("Min Y"), CaComponentValueConstraint::CONSTRAINT_EDIT);
	pStruct->AddComponent (_T("Max Y"), CaComponentValueConstraint::CONSTRAINT_EDIT);

	pStruct->AddComponent (_T("Allocation"), 4, 8388607 ,  CaComponentValueConstraint::CONSTRAINT_EDITNUMBER, _T("4"));  // [4, 8388607]
	pStruct->AddComponent (_T("Extend"),     1, 8388607 ,  CaComponentValueConstraint::CONSTRAINT_EDITNUMBER, _T("16")); // [1, 8388607]
	pStruct->AddComponent (_T("Fillfactor"), 1, 100,     _T("80"));
//	pStruct->AddComponent (_T("Data Compression"),  CaComponentValueConstraint::CONSTRAINT_CHECK, _T("FALSE"));
	m_listStructure.AddTail (pStruct);

	m_nMinPageMin    = 1;
	m_nMinPageMax    = 8388607;
	m_nMaxPageMin    = 1;
	m_nMaxPageMax    = 8388607;
	m_nLeaffillMin   = 1;
	m_nLeaffillMax   = 100;
	m_nNonleaffillMin= 1;
	m_nNonleaffillMax= 100;
	m_nAllocationMin = 4;
	m_nAllocationMax = 8388607;
	m_nExtendMin     = 1;
	m_nExtendMax     = 8388607;
	m_nFillfactorMin = 1;
	m_nFillfactorMax = 100;
}


void CxDlgCreateIndex::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgCreateIndex)
	DDX_Control(pDX, IDOK, m_cButtonOK);
	DDX_Control(pDX, IDC_CHECK_INDEX_PERSISTENCE, m_cCheckPersistence);
	DDX_Control(pDX, IDC_EDIT_INDEX_FILLFACTOR, m_cEditFillfactor);
	DDX_Control(pDX, IDC_EDIT_INDEX_EXTEND, m_cEditExtend);
	DDX_Control(pDX, IDC_EDIT_INDEX_ALLOCATION, m_cEditAllocation);
	DDX_Control(pDX, IDC_CHECK_COMPRESS_KEY, m_cCheckCompressKey);
	DDX_Control(pDX, IDC_CHECK_COMPRESS_DATA, m_cCheckCompressData);
	DDX_Control(pDX, IDC_EDIT_INDEX_NAME, m_cEditIndexName);
	DDX_Control(pDX, IDC_MFC_SPIN_FILLFACTOR, m_cSpinFillfactor);
	DDX_Control(pDX, IDC_STATIC_MIN_Y, m_cStaticMinY);
	DDX_Control(pDX, IDC_STATIC_MIN_X, m_cStaticMinX);
	DDX_Control(pDX, IDC_STATIC_MAX_Y, m_cStaticMaxY);
	DDX_Control(pDX, IDC_STATIC_MAX_X, m_cStaticMaxX);
	DDX_Control(pDX, IDC_COMBO_INDEX_TRUCTURE, m_cComboStructure);
	DDX_Control(pDX, IDC_COMBO_INDEX_PAGESIZE, m_cComboPageSize);
	DDX_Control(pDX, IDC_EDIT_INDEX_MIN_Y, m_cEditMinY);
	DDX_Control(pDX, IDC_EDIT_INDEX_MIN_X, m_cEditMinX);
	DDX_Control(pDX, IDC_EDIT_INDEX_MAX_X, m_cEditMaxX);
	DDX_Control(pDX, IDC_EDIT_INDEX_MAX_Y, m_cEditMaxY);
	DDX_Control(pDX, IDC_EDIT_INDEX_NONLEAFFILL, m_cEditNonleaffill);
	DDX_Control(pDX, IDC_EDIT_INDEX_LEAFFILL, m_cEditLeaffill);
	DDX_Control(pDX, IDC_EDIT_INDEX_MAXPAGE, m_cEditMaxPage);
	DDX_Control(pDX, IDC_EDIT_INDEX_MINPAGE, m_cEditMinPage);
	DDX_Control(pDX, IDC_MFC_SPIN_NONLEAFFIL, m_cSpinNonleaffill);
	DDX_Control(pDX, IDC_MFC_SPIN_LEAFFIL, m_cSpinLeaffill);
	DDX_Control(pDX, IDC_STATIC_RANGE, m_cStaticRange);
	DDX_Control(pDX, IDC_STATIC_NONLEAFFILL, m_cStaticNonleaffill);
	DDX_Control(pDX, IDC_STATIC_LEAFFILL, m_cStaticLeaffill);
	DDX_Control(pDX, IDC_STATIC_MAXPAGE, m_cStaticMaxPage);
	DDX_Control(pDX, IDC_STATIC_MINPAGE, m_cStaticMinPage);
	DDX_Check(pDX, IDC_CHECK_COMPRESS_DATA, m_bCompressData);
	DDX_Check(pDX, IDC_CHECK_COMPRESS_KEY, m_bCompressKey);
	DDX_Check(pDX, IDC_CHECK_INDEX_PERSISTENCE, m_bPersistence);
	DDX_CBIndex(pDX, IDC_COMBO_INDEX_PAGESIZE, m_nPageSize);
	DDX_CBIndex(pDX, IDC_COMBO_INDEX_TRUCTURE, m_nStructure);
	DDX_Text(pDX, IDC_EDIT_INDEX_NAME, m_strIndexName);
	DDX_Text(pDX, IDC_EDIT_INDEX_MINPAGE, m_strMinPage);
	DDX_Text(pDX, IDC_EDIT_INDEX_MAXPAGE, m_strMaxPage);
	DDX_Text(pDX, IDC_EDIT_INDEX_LEAFFILL, m_strLeaffill);
	DDX_Text(pDX, IDC_EDIT_INDEX_NONLEAFFILL, m_strNonleaffill);
	DDX_Text(pDX, IDC_EDIT_INDEX_ALLOCATION, m_strAllocation);
	DDX_Text(pDX, IDC_EDIT_INDEX_EXTEND, m_strExtend);
	DDX_Text(pDX, IDC_EDIT_INDEX_FILLFACTOR, m_strFillfactor);
	DDX_Text(pDX, IDC_EDIT_INDEX_MAX_X, m_strMaxX);
	DDX_Text(pDX, IDC_EDIT_INDEX_MAX_Y, m_strMaxY);
	DDX_Text(pDX, IDC_EDIT_INDEX_MIN_X, m_strMinX);
	DDX_Text(pDX, IDC_EDIT_INDEX_MIN_Y, m_strMinY);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgCreateIndex, CDialog)
	//{{AFX_MSG_MAP(CxDlgCreateIndex)
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_EDIT_INDEX_NAME, OnChangeEditIndexName)
	ON_BN_CLICKED(IDC_BUTTON_NAME_NEW, OnButtonNameNew)
	ON_BN_CLICKED(IDC_BUTTON_NAME_DELETE, OnButtonNameDelete)
	ON_BN_CLICKED(IDC_BUTTON_COLUMN_ADD, OnButtonColumnAdd)
	ON_BN_CLICKED(IDC_BUTTON_COLUMN_REMOVE, OnButtonColumnRemove)
	ON_BN_CLICKED(IDC_BUTTON_COLUMN_UP, OnButtonColumnUp)
	ON_BN_CLICKED(IDC_BUTTON_COLUMN_DOWN, OnButtonColumnDown)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_BASETABLE_COLUMNS, OnDblclkListBasetableColumns)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_INDEX_COLUMNS, OnDblclkListIndexColumns)
	ON_CBN_SELCHANGE(IDC_COMBO_INDEX_TRUCTURE, OnSelchangeComboIndexStructure)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_INDEX_NAME, OnItemchangedListIndexName)
	ON_EN_KILLFOCUS(IDC_EDIT_INDEX_ALLOCATION, OnKillfocusEditIndexAllocation)
	ON_EN_KILLFOCUS(IDC_EDIT_INDEX_EXTEND, OnKillfocusEditIndexExtend)
	ON_EN_KILLFOCUS(IDC_EDIT_INDEX_FILLFACTOR, OnKillfocusEditIndexFillfactor)
	ON_EN_KILLFOCUS(IDC_EDIT_INDEX_LEAFFILL, OnKillfocusEditIndexLeaffill)
	ON_EN_KILLFOCUS(IDC_EDIT_INDEX_MAXPAGE, OnKillfocusEditIndexMaxpage)
	ON_EN_KILLFOCUS(IDC_EDIT_INDEX_MINPAGE, OnKillfocusEditIndexMinpage)
	ON_EN_KILLFOCUS(IDC_EDIT_INDEX_NONLEAFFILL, OnKillfocusEditIndexNonleaffill)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CxDlgCreateIndex message handlers

void CxDlgCreateIndex::OnDestroy() 
{
	CDialog::OnDestroy();
	while (!m_listStructure.IsEmpty())
		delete m_listStructure.RemoveHead();
	while (!m_listIndex.IsEmpty())
		delete m_listIndex.RemoveHead();
	PopHelpId();
}

BOOL CxDlgCreateIndex::OnInitDialog() 
{
	CDialog::OnInitDialog();
	ASSERT (m_pParam);
	if (!m_pParam)
		return FALSE;

	m_cButtonOK.EnableWindow (FALSE);
	VERIFY(m_cButtonUp.SubclassDlgItem(IDC_BUTTON_COLUMN_UP, this));
	VERIFY(m_cButtonDown.SubclassDlgItem(IDC_BUTTON_COLUMN_DOWN, this));
	VERIFY (m_cListCtrlBaseTableColumn.SubclassDlgItem (IDC_LIST_BASETABLE_COLUMNS, this));
	VERIFY (m_cListCtrlIndexColumn.SubclassDlgItem (IDC_LIST_INDEX_COLUMNS, this));
	VERIFY (m_cListIndexName.SubclassDlgItem (IDC_LIST_INDEX_NAME, this));
	m_ImageCheck.Create (IDB_CHECK, 16, 1, RGB (255, 0, 0));
	m_ImageHeight.Create(1, 20, TRUE, 1, 0);
	m_cListCtrlIndexColumn.SetCheckImageList(&m_ImageCheck);
	VERIFY (m_cListLocation.SubclassDlgItem (IDC_LIST_INDEX_LOCATION, this));
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	if (hFont == NULL)
		hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
	// m_cListLocation.SendMessage(WM_SETFONT, (WPARAM)hFont);

	m_cListIndexName.SetImageList (&m_ImageHeight, LVSIL_SMALL);

	m_cButtonUp.SizeToContent();
	m_cButtonDown.SizeToContent();

	LV_COLUMN lvcolumn;
	TCHAR     szColHeader [2][16];
	lstrcpy(szColHeader [0],VDBA_MfcResourceString(IDS_TC_COLUMN));// _T("Column")
	lstrcpy(szColHeader [1],VDBA_MfcResourceString(IDS_TC_TYPE));  // _T("Type")
	int       i;
	int       hWidth [2] = {130, 300};
	CRect r;
	//
	// Set the number of columns: 2
	// List of columns of Base Table:
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	m_cListCtrlBaseTableColumn.GetClientRect (r);
	// Set the column width more large than the dimension of control
	// for refresh display for all the column
	hWidth [1] = (r.Width()-hWidth [0]+20);
	for (i=0; i<2; i++)
	{
		lvcolumn.fmt = LVCFMT_LEFT;
		lvcolumn.pszText = szColHeader[i];
		lvcolumn.iSubItem = i;
		lvcolumn.cx = hWidth[i];
		m_cListCtrlBaseTableColumn.InsertColumn (i, &lvcolumn);
	}
	// Set the last column width with the good dimension.
	m_cListCtrlBaseTableColumn.SetColumnWidth(1,r.Width()-hWidth [0]);
	//
	// Set the number of columns: 2
	// List of columns of Index:
	TCHAR  szIdxColHeader [2][16];
	lstrcpy( szIdxColHeader[0],VDBA_MfcResourceString(IDS_TC_COLUMN));// _T("Column")
	lstrcpy( szIdxColHeader[1],VDBA_MfcResourceString(IDS_TC_KEY));   // _T("Key")
	hWidth [0] = 160;
	hWidth [1] = 60;

	for (i=0; i<2; i++)
	{
		lvcolumn.fmt = LVCFMT_LEFT;
		lvcolumn.pszText = szIdxColHeader[i];
		lvcolumn.iSubItem = i;
		lvcolumn.cx = hWidth[i];
		if (i == 0)
			m_cListCtrlIndexColumn.InsertColumn (i, &lvcolumn);
		else
		{
			lvcolumn.fmt = LVCFMT_CENTER;
			m_cListCtrlIndexColumn.InsertColumn (i, &lvcolumn, TRUE);
		}
	}

	m_cListIndexName.GetClientRect (r);
	lvcolumn.fmt = LVCFMT_LEFT;
	lvcolumn.pszText  = _T("Index Name");
	lvcolumn.cx = r.Width()-20;
	m_cListIndexName.InsertColumn (0, &lvcolumn);

	//
	// Query the list of columns of Base Table:
	if (!InitializeBaseTableColumns())
		return FALSE;
	//
	// Query the list of locations:
	InitializeLocations();
	//
	// Initialize the Combobox of Page Size:
	InitPageSize (2048L);
	//
	// Initialize the Combobox of Structure:
	int idx;
	POSITION pos = m_listStructure.GetHeadPosition();
	while (pos != NULL)
	{
		CaIndexStructure* pStruct = m_listStructure.GetNext (pos);
		idx = m_cComboStructure.AddString (pStruct->m_strStructure);
		if (idx != CB_ERR)
			m_cComboStructure.SetItemData (idx, (DWORD)pStruct);
	}
	m_cComboStructure.SetCurSel(m_nStructure);
	//
	// Reposition the Min Max X, Y of RTree Edit Controls:
	m_cEditMinPage.GetWindowRect (r);
	ScreenToClient (r);
	m_cEditMinX.SetWindowPos (NULL, r.left, r.top, 0, 0, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOZORDER);

	m_cEditMaxPage.GetWindowRect (r);
	ScreenToClient (r);
	m_cEditMaxX.SetWindowPos (NULL, r.left, r.top, 0, 0, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOZORDER);

	m_cEditLeaffill.GetWindowRect (r);
	ScreenToClient (r);
	m_cEditMinY.SetWindowPos (NULL, r.left, r.top, 0, 0, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOZORDER);

	m_cEditNonleaffill.GetWindowRect (r);
	ScreenToClient (r);
	m_cEditMaxY.SetWindowPos (NULL, r.left, r.top, 0, 0, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOZORDER);

	//
	// Reposition the Min Max X, Y of RTree Static Label Controls:
	m_cStaticMinPage.GetWindowRect (r);
	ScreenToClient (r);
	m_cStaticMinX.SetWindowPos (NULL, r.left, r.top, 0, 0, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOZORDER);

	m_cStaticMaxPage.GetWindowRect (r);
	ScreenToClient (r);
	m_cStaticMaxX.SetWindowPos (NULL, r.left, r.top, 0, 0, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOZORDER);

	m_cStaticLeaffill.GetWindowRect (r);
	ScreenToClient (r);
	m_cStaticMinY.SetWindowPos (NULL, r.left, r.top, 0, 0, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOZORDER);

	m_cStaticNonleaffill.GetWindowRect (r);
	ScreenToClient (r);
	m_cStaticMaxY.SetWindowPos (NULL, r.left, r.top, 0, 0, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOZORDER);

	// Limit text for edit
	m_cEditAllocation.LimitText(8);
	m_cEditExtend.LimitText(8);
	m_cEditMaxPage.LimitText(8);
	m_cEditMinPage.LimitText(8);

	//
	// Initially, the dialog creates an index named <Undamed 1> and adds it
	// in the list of index names:
	OnButtonNameNew();
	//
	// Set the Title:
	CString strTitle;
	CString strTemplate;
	GetWindowText (strTemplate);
	strTitle.Format (
		strTemplate, 
		(LPCTSTR)(LPUCHAR)GetVirtNodeName (GetCurMdiNodeHandle()),
		(LPCTSTR)m_strDatabase,
		(LPCTSTR)m_strTable);
	SetWindowText (strTitle);
	PushHelpId(IDD_CREATEINDEX);

	//
	// If not Ingres 2.5 (Create only single index):
	if ( GetOIVers() < OIVERS_25) 
	{
		CWnd* pWnd = GetDlgItem(IDC_BUTTON_NAME_NEW);
		if (pWnd && IsWindow (pWnd->m_hWnd))
			pWnd->ShowWindow (SW_HIDE);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgCreateIndex::OnCancel() 
{
	CaIndex* pIndex = NULL;
	int i, nCount = m_cListIndexName.GetItemCount();
	m_listIndex.RemoveAll();
	for (i=0; i<nCount; i++)
	{
		pIndex = (CaIndex*)m_cListIndexName.GetItemData (i);
		m_listIndex.AddTail(pIndex);
	}
	
	CDialog::OnCancel();
}

void CxDlgCreateIndex::OnOK() 
{
	CaIndex* pIndex = NULL;
	int ires,nSel = -1;
	int i, nCount = m_cListIndexName.GetItemCount();

	pIndex = GetCurrentIndex(nSel); //retrieve the last change.
	ASSERT (pIndex);
	if (!pIndex)
		return;
	GetIndexData (pIndex);

	m_listIndex.RemoveAll();
	for (i=0; i<nCount; i++)
	{
		pIndex = (CaIndex*)m_cListIndexName.GetItemData (i);
		m_listIndex.AddTail(pIndex);
	}

	CString strStatement;
	GenerateSyntax(strStatement);
	m_pParam->lpszStatement = (LPCTSTR)strStatement;
	
	ires = DBAAddObject((LPUCHAR)GetVirtNodeName(GetCurMdiNodeHandle ()), OT_INDEX, (void *)m_pParam);
	if (ires != RES_SUCCESS)
	{
		ErrorMessage ((UINT) IDS_E_CREATE_INDEX_FAILED, ires);
		return;
	}

	TRACE1 ("STATEMENT: %s\n", (LPCTSTR)strStatement);
	CDialog::OnOK();
}



void CxDlgCreateIndex::OnItemchangedListIndexName(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;

	try
	{
		CaIndex* pIndex = NULL;
		//
		// New Selected Item:
		if (pNMListView->iItem != -1 && pNMListView->uNewState != 0)
		{
			pIndex = (CaIndex*)m_cListIndexName.GetItemData (pNMListView->iItem);
			//
			// Data from pIndex to Dlg:
			SetIndexData (pIndex);
		}
		else
		if (pNMListView->iItem != -1 && pNMListView->uOldState != 0)
		{
			pIndex = (CaIndex*)m_cListIndexName.GetItemData (pNMListView->iItem);
			//
			// Data from Dlg to pIndex:
			GetIndexData (pIndex);
		}
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	catch (...)
	{
		CString strMsg = _T("Exepcetion raised: CxDlgCreateIndex::OnSelchangeListIndexName()\n");
		TRACE0 (strMsg);
	}
}



void CxDlgCreateIndex::OnButtonNameNew() 
{
	try
	{
		CaIndex* pIndex = NULL;
		int nCount = 0;
		CString strName = GetIndexName();
		pIndex = new CaIndex (strName, _T(""));
		//
		// Default Index location:
		if (!m_strIIDatabase.IsEmpty())
			pIndex->m_listLocation.AddTail (m_strIIDatabase);

		int idx = -1;
		nCount = m_cListIndexName.GetItemCount();
		LV_ITEM lvitem;
		memset (&lvitem, 0, sizeof(lvitem));
		lvitem.mask       = LVIF_TEXT|LVIF_PARAM|LVIF_STATE;
		lvitem.iItem      = nCount;
		lvitem.state      = LVIS_SELECTED;
		lvitem.pszText    = (LPTSTR)(LPCTSTR)strName;
		lvitem.cchTextMax = 64;
		lvitem.lParam     = (LPARAM)pIndex;
		idx = m_cListIndexName.InsertItem (&lvitem);
		ASSERT (idx != LB_ERR);
		if (idx == LB_ERR && pIndex)
		{
			delete pIndex;
			return;
		}
		EnableDisableButtonOK();
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	catch (...)
	{
		CString strMsg = _T("Exepcetion raised: CxDlgCreateIndex::OnButtonNameNew()\n");
		TRACE0 (strMsg);
	}
}

void CxDlgCreateIndex::OnButtonNameDelete() 
{
	CaIndex* pIndex = NULL;
	int nDel = -1, nCount = m_cListIndexName.GetItemCount();
	//
	// There is always an index:
	if (nCount == 1)
		return;
	
	int nSel = m_cListIndexName.GetNextItem (-1, LVNI_SELECTED);
	if (nSel == -1)
		return;

	pIndex = (CaIndex*)m_cListIndexName.GetItemData (nSel);
	nDel   = nSel;
	if (nSel == 0)
		nSel = 1;
	else
	if (nSel == (nCount-1))
		nSel = nCount-2;
	else
		nSel = nSel-1;
	m_cListIndexName.SetItemState (nSel, LVIS_SELECTED, LVIS_SELECTED);
	if (nDel != -1 && pIndex)
	{
		m_cListIndexName.DeleteItem (nDel);
		delete pIndex;
	}
	EnableDisableButtonOK();
}

void CxDlgCreateIndex::OnButtonColumnAdd() 
{
	LV_FINDINFO lvfindinfo;
	CString strItem;
	int iIndex = -1;
	int i, idx, nCount = m_cListCtrlBaseTableColumn.GetItemCount();

	memset (&lvfindinfo, 0, sizeof (LV_FINDINFO));
	lvfindinfo.flags    = LVFI_STRING;
	for (i=0; i<nCount; i++)
	{
		if (m_cListCtrlBaseTableColumn.GetItemState(i, LVIS_SELECTED) & LVIS_SELECTED)
		{
			strItem = m_cListCtrlBaseTableColumn.GetItemText (i, 0);
			lvfindinfo.psz      = (LPCSTR)strItem;
			iIndex = m_cListCtrlIndexColumn.FindItem (&lvfindinfo);
			if (iIndex == -1)
			{
				idx = m_cListCtrlIndexColumn.GetItemCount();
				m_cListCtrlIndexColumn.InsertItem (idx, strItem);
				//
				// By default, index is keyed on the columns in the column list.
				m_cListCtrlIndexColumn.SetCheck (idx, 1, FALSE);
			}
		}
	}

	int nSel = -1;
	CaIndex* pIndex = GetCurrentIndex(nSel);
	ASSERT (pIndex);
	if (!pIndex)
		return;
	//
	// List of index columns:
	CaColumnIndex* pIndexColumn = NULL;
	while (!pIndex->m_listColumn.IsEmpty())
		delete pIndex->m_listColumn.RemoveHead();
	nCount = m_cListCtrlIndexColumn.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		strItem = m_cListCtrlIndexColumn.GetItemText (i, 0);
		pIndexColumn = new CaColumnIndex(strItem, m_cListCtrlIndexColumn.GetCheck(i, 1));
		pIndex->m_listColumn.AddTail(pIndexColumn);
	}
	m_cListCtrlIndexColumn.Invalidate();
	EnableDisableButtonOK();
}

void CxDlgCreateIndex::OnButtonColumnRemove() 
{
	int iIndex = -1;
	int i = 0, nCount = m_cListCtrlIndexColumn.GetItemCount();
	
	while (i<nCount)
	{
		if (m_cListCtrlIndexColumn.GetItemState(i, LVIS_SELECTED) & LVIS_SELECTED)
		{
			m_cListCtrlIndexColumn.DeleteItem (i);
			nCount = m_cListCtrlIndexColumn.GetItemCount();
			continue;
		}
		i++;
	}
	int nSel = -1;
	CString strItem;
	CaIndex* pIndex = GetCurrentIndex(nSel);
	ASSERT (pIndex);
	if (!pIndex)
		return;
	//
	// List of index columns:
	CaColumnIndex* pIndexColumn = NULL;
	while (!pIndex->m_listColumn.IsEmpty())
		delete pIndex->m_listColumn.RemoveHead();
	nCount = m_cListCtrlIndexColumn.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		strItem = m_cListCtrlIndexColumn.GetItemText (i, 0);
		pIndexColumn = new CaColumnIndex(strItem, m_cListCtrlIndexColumn.GetCheck(i, 1));
		pIndex->m_listColumn.AddTail(pIndexColumn);
	}
	m_cListCtrlIndexColumn.Invalidate();
	EnableDisableButtonOK();
}


void CxDlgCreateIndex::OnButtonColumnUp() 
{
	int i, nSelected = -1, nCount = 0;
	CString strItem;
	CaIndex* pIndex = GetCurrentIndex(nSelected);
	ASSERT (pIndex);
	if (!pIndex)
		return;
	nCount = m_cListCtrlIndexColumn.GetItemCount();
	nSelected = m_cListCtrlIndexColumn.GetNextItem (-1, LVNI_SELECTED);
	if (nSelected == -1 || nSelected == 0 || nCount == 1)
		return;
	//
	// Current Item
	CString strCurrentColumn = m_cListCtrlIndexColumn.GetItemText (nSelected, 0);
	BOOL    bCurrentKey      = m_cListCtrlIndexColumn.GetCheck    (nSelected, 1);
	UINT    nCurrentState    = m_cListCtrlIndexColumn.GetItemState(nSelected, LVIS_SELECTED|LVIS_FOCUSED);

	//
	// Up Item
	CString strUpColumn      = m_cListCtrlIndexColumn.GetItemText (nSelected-1, 0);
	BOOL    bUpKey           = m_cListCtrlIndexColumn.GetCheck    (nSelected-1, 1);
	UINT    nUpState         = m_cListCtrlIndexColumn.GetItemState(nSelected-1, LVIS_SELECTED|LVIS_FOCUSED);

	m_cListCtrlIndexColumn.SetItemText (nSelected,   0, strUpColumn);
	m_cListCtrlIndexColumn.SetCheck    (nSelected,   1, bUpKey);
	m_cListCtrlIndexColumn.SetItemState(nSelected  , nUpState, LVIS_SELECTED|LVIS_FOCUSED);

	m_cListCtrlIndexColumn.SetItemText (nSelected-1, 0, strCurrentColumn);
	m_cListCtrlIndexColumn.SetCheck    (nSelected-1, 1, bCurrentKey);
	m_cListCtrlIndexColumn.SetItemState(nSelected-1, nCurrentState, LVIS_SELECTED|LVIS_FOCUSED);

	//
	// List of index columns:
	CaColumnIndex* pIndexColumn = NULL;
	while (!pIndex->m_listColumn.IsEmpty())
		delete pIndex->m_listColumn.RemoveHead();
	nCount = m_cListCtrlIndexColumn.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		strItem = m_cListCtrlIndexColumn.GetItemText (i, 0);
		pIndexColumn = new CaColumnIndex(strItem, m_cListCtrlIndexColumn.GetCheck(i, 1));
		pIndex->m_listColumn.AddTail(pIndexColumn);
	}
}

void CxDlgCreateIndex::OnButtonColumnDown() 
{
	int i, nSelected = -1, nCount = 0;
	CString strItem;
	CaIndex* pIndex = GetCurrentIndex(nSelected);
	ASSERT (pIndex);
	if (!pIndex)
		return;
	nCount = m_cListCtrlIndexColumn.GetItemCount();
	nSelected = m_cListCtrlIndexColumn.GetNextItem (-1, LVNI_SELECTED);
	if (nSelected != -1 && nSelected < (nCount-1))
	{
		//
		// Current Item
		CString strCurrentColumn = m_cListCtrlIndexColumn.GetItemText (nSelected, 0);
		BOOL    bCurrentKey      = m_cListCtrlIndexColumn.GetCheck    (nSelected, 1);
		UINT    nCurrentState    = m_cListCtrlIndexColumn.GetItemState(nSelected, LVIS_SELECTED|LVIS_FOCUSED);
		//
		// Down Item
		CString strDownColumn    = m_cListCtrlIndexColumn.GetItemText (nSelected+1, 0);
		BOOL    bDownKey         = m_cListCtrlIndexColumn.GetCheck    (nSelected+1, 1);
		UINT    nDownState       = m_cListCtrlIndexColumn.GetItemState(nSelected+1, LVIS_SELECTED|LVIS_FOCUSED);

		m_cListCtrlIndexColumn.SetItemText (nSelected,   0, strDownColumn);
		m_cListCtrlIndexColumn.SetCheck    (nSelected,   1, bDownKey);
		m_cListCtrlIndexColumn.SetItemState(nSelected, nDownState, LVIS_SELECTED|LVIS_FOCUSED);

		m_cListCtrlIndexColumn.SetItemText (nSelected+1, 0, strCurrentColumn);
		m_cListCtrlIndexColumn.SetCheck    (nSelected+1, 1, bCurrentKey);
		m_cListCtrlIndexColumn.SetItemState(nSelected+1, nCurrentState, LVIS_SELECTED|LVIS_FOCUSED);

	}
	//
	// List of index columns:
	CaColumnIndex* pIndexColumn = NULL;
	while (!pIndex->m_listColumn.IsEmpty())
		delete pIndex->m_listColumn.RemoveHead();
	nCount = m_cListCtrlIndexColumn.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		strItem = m_cListCtrlIndexColumn.GetItemText (i, 0);
		pIndexColumn = new CaColumnIndex(strItem, m_cListCtrlIndexColumn.GetCheck(i, 1));
		pIndex->m_listColumn.AddTail(pIndexColumn);
	}
}

void CxDlgCreateIndex::OnDblclkListBasetableColumns(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnButtonColumnAdd();
	*pResult = 0;
}

void CxDlgCreateIndex::OnDblclkListIndexColumns(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnButtonColumnRemove();
	*pResult = 0;
}

void CxDlgCreateIndex::OnSelchangeComboIndexStructure()
{
	SelchangeComboIndexStructure (TRUE);
}

void CxDlgCreateIndex::SelchangeComboIndexStructure(BOOL bSetDefaultValue)
{
	try
	{
		int nSel;
		CaIndex* pIndex = GetCurrentIndex(nSel);
		m_nStructure = m_cComboStructure.GetCurSel();
		if (m_nStructure == CB_ERR)
			return;
		CaIndexStructure* pStruct = (CaIndexStructure*)m_cComboStructure.GetItemData(m_nStructure);
		//
		// Specific for RTree Structure:
		if (pStruct->m_nStructType != IDX_RTREE)
		{
			m_cStaticRange.ShowWindow (SW_HIDE);
			m_cStaticMinX.ShowWindow (SW_HIDE);
			m_cStaticMaxX.ShowWindow (SW_HIDE);
			m_cStaticMinY.ShowWindow (SW_HIDE);
			m_cStaticMaxY.ShowWindow (SW_HIDE);
			m_cEditMinX.ShowWindow (SW_HIDE);
			m_cEditMaxX.ShowWindow (SW_HIDE);
			m_cEditMinY.ShowWindow (SW_HIDE);
			m_cEditMaxY.ShowWindow (SW_HIDE);

			m_cStaticMinPage.ShowWindow (SW_SHOW);
			m_cStaticMaxPage.ShowWindow (SW_SHOW);
			m_cStaticLeaffill.ShowWindow (SW_SHOW);
			m_cStaticNonleaffill.ShowWindow (SW_SHOW);

			m_cEditMinPage.ShowWindow (SW_SHOW);
			m_cEditMaxPage.ShowWindow (SW_SHOW);
			m_cEditLeaffill.ShowWindow (SW_SHOW);
			m_cEditNonleaffill.ShowWindow (SW_SHOW);
			m_cSpinLeaffill.ShowWindow (SW_SHOW);
			m_cSpinNonleaffill.ShowWindow (SW_SHOW);

			m_cCheckCompressData.EnableWindow (TRUE);
			m_cCheckCompressKey.EnableWindow (TRUE);
			GetDlgItem(IDC_RADIO_UNIQUE_ROW)->EnableWindow (TRUE);
			GetDlgItem(IDC_RADIO_UNIQUE_STATEMENT)->EnableWindow (TRUE);
		}
		else
		{
			m_cStaticRange.ShowWindow (SW_SHOW);
			m_cStaticMinX.ShowWindow (SW_SHOW);
			m_cStaticMaxX.ShowWindow (SW_SHOW);
			m_cStaticMinY.ShowWindow (SW_SHOW);
			m_cStaticMaxY.ShowWindow (SW_SHOW);
			m_cEditMinX.ShowWindow (SW_SHOW);
			m_cEditMaxX.ShowWindow (SW_SHOW);
			m_cEditMinY.ShowWindow (SW_SHOW);
			m_cEditMaxY.ShowWindow (SW_SHOW);

			m_cStaticMinPage.ShowWindow (SW_HIDE);
			m_cStaticMaxPage.ShowWindow (SW_HIDE);
			m_cStaticLeaffill.ShowWindow (SW_HIDE);
			m_cStaticNonleaffill.ShowWindow (SW_HIDE);

			m_cEditMinPage.ShowWindow (SW_HIDE);
			m_cEditMaxPage.ShowWindow (SW_HIDE);
			m_cEditLeaffill.ShowWindow (SW_HIDE);
			m_cEditNonleaffill.ShowWindow (SW_HIDE);
			m_cSpinLeaffill.ShowWindow (SW_HIDE);
			m_cSpinNonleaffill.ShowWindow (SW_HIDE);

			m_cCheckCompressData.EnableWindow (FALSE);
			m_cCheckCompressKey.EnableWindow (FALSE);

			CheckRadioButton (IDC_RADIO_UNIQUE_NO, IDC_RADIO_UNIQUE_STATEMENT, IDC_RADIO_UNIQUE_NO);
			GetDlgItem(IDC_RADIO_UNIQUE_ROW)->EnableWindow (FALSE);
			GetDlgItem(IDC_RADIO_UNIQUE_STATEMENT)->EnableWindow (FALSE);
			if (pIndex)
			{
				pIndex->m_bUnique = FALSE;
				pIndex->m_nUniqueScope = -1;
			}
			m_strMinPage.Empty();
			m_strMaxPage.Empty();
		}

		if (pStruct->m_nStructType == IDX_BTREE)
		{
			m_cStaticLeaffill.EnableWindow (TRUE);
			m_cStaticNonleaffill.EnableWindow (TRUE);
			m_cEditLeaffill.EnableWindow (TRUE);
			m_cEditNonleaffill.EnableWindow (TRUE);
			m_cSpinLeaffill.EnableWindow (TRUE);
			m_cSpinNonleaffill.EnableWindow (TRUE);
			m_strMinPage.Empty();
			m_strMaxPage.Empty();

			m_cCheckCompressData.EnableWindow (FALSE);
		}
		else
		{
			m_cStaticLeaffill.EnableWindow (FALSE);
			m_cStaticNonleaffill.EnableWindow (FALSE);
			m_cEditLeaffill.EnableWindow (FALSE);
			m_cEditNonleaffill.EnableWindow (FALSE);
			m_cSpinLeaffill.EnableWindow (FALSE);
			m_cSpinNonleaffill.EnableWindow (FALSE);
			m_strNonleaffill.Empty();
			m_strLeaffill.Empty();

			m_cCheckCompressKey.EnableWindow (FALSE);
		}

		if (pStruct->m_nStructType == IDX_HASH)
		{
			m_cStaticMinPage.EnableWindow (TRUE);
			m_cStaticMaxPage.EnableWindow (TRUE);
			m_cEditMinPage.EnableWindow (TRUE);
			m_cEditMaxPage.EnableWindow (TRUE);
		}
		else
		{
			m_cStaticMinPage.EnableWindow (FALSE);
			m_cStaticMaxPage.EnableWindow (FALSE);
			m_cEditMinPage.EnableWindow (FALSE);
			m_cEditMaxPage.EnableWindow (FALSE);
		}

		CaComponentValueConstraint* pComp = NULL;
		POSITION pos = pStruct->m_listComponent.GetHeadPosition();
		switch (pStruct->m_nStructType)
		{
		case IDX_BTREE:
			while (pos != NULL)
			{
				pComp = pStruct->m_listComponent.GetNext (pos);
				if (pComp->m_strComponentName == _T("Leaffill"))
				{
					m_cSpinLeaffill.SetRange (pComp->m_nLower, pComp->m_nUpper);
					m_strLeaffill = pComp->m_strValue;
					if (pIndex && !bSetDefaultValue)
							m_strLeaffill = pIndex->m_strLeaffill ;

					m_nLeaffillMin = pComp->m_nLower;
					m_nLeaffillMax = pComp->m_nUpper;
				}
				else
				if (pComp->m_strComponentName == _T("Non Leaffill"))
				{
					m_cSpinNonleaffill.SetRange (pComp->m_nLower, pComp->m_nUpper);
					m_strNonleaffill = pComp->m_strValue;
					if (pIndex && !bSetDefaultValue)
						m_strNonleaffill = pIndex->m_strNonleaffill;

					m_nNonleaffillMin = pComp->m_nLower;
					m_nNonleaffillMax = pComp->m_nUpper;
				}
			}
			break;
		case IDX_ISAM:
			break;
		case IDX_HASH:
			while (pos != NULL)
			{
				pComp = pStruct->m_listComponent.GetNext (pos);
				if (pComp->m_strComponentName == _T("Min Page"))
				{
					m_strMinPage = pComp->m_strValue;
					if (pIndex && !bSetDefaultValue)
						m_strMinPage = pIndex->m_strMinPage;

					m_nMinPageMin = pComp->m_nLower;
					m_nMinPageMax = pComp->m_nUpper;
				}
				else
				if (pComp->m_strComponentName == _T("Max Page"))
				{
					m_strMaxPage = pComp->m_strValue;
					if (pIndex && !bSetDefaultValue)
						m_strMaxPage = pIndex->m_strMaxPage;

					m_nMaxPageMin = pComp->m_nLower;
					m_nMaxPageMax = pComp->m_nUpper;
				}
			}
			break;
		case IDX_RTREE:
			break;
		default:
			break;
		}
		pos = pStruct->m_listComponent.GetHeadPosition();
		while (pos != NULL)
		{
			pComp = pStruct->m_listComponent.GetNext (pos);
			if (pComp->m_strComponentName == _T("Allocation"))
			{
				m_strAllocation = pComp->m_strValue;
				if (pIndex && !bSetDefaultValue)
					m_strAllocation = pIndex->m_strAllocation;

				m_nAllocationMin = pComp->m_nLower;
				m_nAllocationMax = pComp->m_nUpper;
			}
			else
			if (pComp->m_strComponentName == _T("Extend"))
			{	
				m_strExtend = pComp->m_strValue;
				if (pIndex && !bSetDefaultValue)
					m_strExtend = pIndex->m_strExtend;

				m_nExtendMin = pComp->m_nLower;
				m_nExtendMax = pComp->m_nUpper;
			}
			else
			if (pComp->m_strComponentName == _T("Fillfactor"))
			{
				m_cSpinFillfactor.SetRange (pComp->m_nLower, pComp->m_nUpper);
				m_strFillfactor = pComp->m_strValue;
				if (pIndex && !bSetDefaultValue)
					m_strFillfactor = pIndex->m_strFillfactor;

				m_nFillfactorMin = pComp->m_nLower;
				m_nFillfactorMax = pComp->m_nUpper;
			}
		}

		UpdateData (FALSE);
		if (!pIndex)
			return;
		pIndex->m_nStructure   = m_nStructure;
		pIndex->m_strStructure = pStruct->m_strStructure;
	}
	catch (...)
	{
		CString strMsg = _T("Exepcetion raised: CxDlgCreateIndex::SelchangeComboIndexStructure()\n");
		TRACE0 (strMsg);
	}
}


BOOL CxDlgCreateIndex::InitializeBaseTableColumns()
{
	int nCount = 0;
	CaTableStatistic ts;
	CaTableStatisticColumn* pColumn = NULL;
	int nIndex = 0;

	ts.m_nOT           = OT_TABLE;
	ts.m_strVNode      = (LPCTSTR)(LPUCHAR)GetVirtNodeName (GetCurMdiNodeHandle());
	ts.m_strDBName     = m_strDatabase;
	ts.m_strTable      = m_strTable;
	ts.m_strTableOwner = m_strTableOwner;
	ts.m_AddCompositeForHistogram = FALSE;
	if (!Table_QueryStatisticColumns (ts))
		return FALSE;

	while (!ts.m_listColumn.IsEmpty())
	{
		nCount = m_cListCtrlBaseTableColumn.GetItemCount();
		pColumn = ts.m_listColumn.RemoveHead();
		//
		// You cannot create an index on a longvarchar column
		if (pColumn->m_nDataType != INGTYPE_LONGVARCHAR)
		{
			m_cListCtrlBaseTableColumn.InsertItem  (nCount, pColumn->m_strColumn);
			m_cListCtrlBaseTableColumn.SetItemText (nCount, 1, pColumn->m_strType);
		}
		delete pColumn;
	}

	return TRUE;
}

BOOL CxDlgCreateIndex::InitializeLocations()
{
	int     hNode   = -1, err = RES_ERR;
	BOOL    bSystem = TRUE; // Force to true to get II_DATABASE ...
	TCHAR   tchszObject[MAXOBJECTNAME];
	TCHAR   tchszFilter[MAXOBJECTNAME];
	LPUCHAR aparents[MAXPLEVEL];

	aparents[0] = (LPUCHAR)(LPCTSTR)m_strDatabase;
	aparents[1] = (LPUCHAR)NULL;
	hNode   = GetCurMdiNodeHandle();

	err = DOMGetFirstObject(
		hNode,
		OT_LOCATION,
		0,
		aparents,
		bSystem,
		NULL,
		(LPUCHAR)tchszObject,
		NULL,
		NULL);
	while (err == RES_SUCCESS)
	{
		BOOL bOK;
		if (DOMLocationUsageAccepted(hNode, (LPUCHAR)tchszObject, LOCATIONDATABASE, &bOK)== RES_SUCCESS && bOK) 
			m_cListLocation.AddString (tchszObject);
		err = DOMGetNextObject ((LPUCHAR)tchszObject, (LPUCHAR)tchszFilter, NULL);
	}
	//
	// Select II_DATABASE by default
	m_strIIDatabase.LoadString (IDS_IIDATABASE);
	if (!m_strIIDatabase.IsEmpty())
	{
		int nIdx = m_cListLocation.FindStringExact (-1, m_strIIDatabase);
		if (nIdx != LB_ERR)
			m_cListLocation.SetCheck (nIdx, TRUE);
	}
	return TRUE;
}

void CxDlgCreateIndex::InitPageSize (LONG lPage_size)
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

	for (i=0; i<6; i++)
	{
		index = m_cComboPageSize.AddString (pageSize [i].cx);
		if (index != -1)
			m_cComboPageSize.SetItemData (index, (DWORD)pageSize [i].ux);
	}
	m_cComboPageSize.SetCurSel(0);
	for (i=0; i<6; i++)
	{
		if (pageSize [i].ux == lPage_size)
		{
			m_cComboPageSize.SetCurSel(i);
			break;
		}
	}
}

//
// Data from Dialog controls to pIndex:
void CxDlgCreateIndex::GetIndexData (CaIndex* pIndex)
{
	CString strItem;
	long lPageSize = 0;
	CaIndexStructure* pStructure = NULL;

	UpdateData (TRUE);
	pIndex->m_bPersistence   = m_bPersistence;

	int nID = GetCheckedRadioButton (IDC_RADIO_UNIQUE_NO, IDC_RADIO_UNIQUE_STATEMENT);
	pIndex->m_bUnique        = (nID != IDC_RADIO_UNIQUE_NO)? TRUE: FALSE;
	if (pIndex->m_bUnique)
	{
		if (nID == IDC_RADIO_UNIQUE_ROW)
			pIndex->m_nUniqueScope = CaIndex::INDEX_UNIQUE_SCOPE_ROW;
		else
			pIndex->m_nUniqueScope = CaIndex::INDEX_UNIQUE_SCOPE_STATEMENT;
	}
	pIndex->m_bCompressData  = m_bCompressData;
	pIndex->m_bCompressKey   = m_bCompressKey;

	pIndex->m_nStructure     = m_nStructure;
	pIndex->m_nPageSize      = m_nPageSize;
	lPageSize = (long)m_cComboPageSize.GetItemData (m_nPageSize);
	pIndex->m_strPageSize.Format (_T("%ld"),lPageSize);
	pStructure = (CaIndexStructure*)m_cComboStructure.GetItemData (m_nStructure);
	pIndex->m_strStructure   = pStructure->m_strStructure;

	pIndex->m_strMinPage     = m_strMinPage;
	pIndex->m_strMaxPage     = m_strMaxPage;
	pIndex->m_strLeaffill    = m_strLeaffill;
	pIndex->m_strNonleaffill = m_strNonleaffill;
	pIndex->m_strAllocation  = m_strAllocation;
	pIndex->m_strExtend      = m_strExtend;
	pIndex->m_strFillfactor  = m_strFillfactor;
	pIndex->m_strMaxX = m_strMaxX;
	pIndex->m_strMaxY = m_strMaxY;
	pIndex->m_strMinX = m_strMinX;
	pIndex->m_strMinY = m_strMinY;

	//
	// List of index columns:
	CaColumnIndex* pIndexColumn = NULL;
	while (!pIndex->m_listColumn.IsEmpty())
		delete pIndex->m_listColumn.RemoveHead();
	int i, nCount = m_cListCtrlIndexColumn.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		strItem = m_cListCtrlIndexColumn.GetItemText (i, 0);
		pIndexColumn = new CaColumnIndex(strItem, m_cListCtrlIndexColumn.GetCheck(i, 1));
		pIndex->m_listColumn.AddTail(pIndexColumn);
	}
	//
	// List locations:
	pIndex->m_listLocation.RemoveAll();
	nCount = m_cListLocation.GetCount();
	for (i=0; i<nCount; i++)
	{
		if (m_cListLocation.GetCheck (i) == 1)
		{
			m_cListLocation.GetText (i, strItem);
			pIndex->m_listLocation.AddTail (strItem);
		}
	}
}

//
// Data from pIndex to Dialog controls:
void CxDlgCreateIndex::SetIndexData (CaIndex* pIndex)
{
	CString strItem;

	m_bPersistence   = pIndex->m_bPersistence;
	int nID = IDC_RADIO_UNIQUE_NO;
	if (pIndex->m_bUnique)
	{
		if (pIndex->m_nUniqueScope == CaIndex::INDEX_UNIQUE_SCOPE_ROW)
			nID = IDC_RADIO_UNIQUE_ROW;
		else
			nID = IDC_RADIO_UNIQUE_STATEMENT;
	}
	CheckRadioButton (IDC_RADIO_UNIQUE_NO, IDC_RADIO_UNIQUE_STATEMENT, nID);

	m_strIndexName   = pIndex->m_strItem;
	m_bCompressData  = pIndex->m_bCompressData;
	m_bCompressKey   = pIndex->m_bCompressKey;

	m_nPageSize      = pIndex->m_nPageSize;
	m_strMinPage     = pIndex->m_strMinPage;
	m_strMaxPage     = pIndex->m_strMaxPage;
	m_strLeaffill    = pIndex->m_strLeaffill;
	m_strNonleaffill = pIndex->m_strNonleaffill;
	m_strAllocation  = pIndex->m_strAllocation;
	m_strExtend      = pIndex->m_strExtend;
	m_strFillfactor  = pIndex->m_strFillfactor;
	m_strMaxX = pIndex->m_strMaxX;
	m_strMaxY = pIndex->m_strMaxY;
	m_strMinX = pIndex->m_strMinX;
	m_strMinY = pIndex->m_strMinY;
	m_nStructure     = pIndex->m_nStructure;
	m_cComboStructure.SetCurSel (m_nStructure);
	SelchangeComboIndexStructure(FALSE);

	//
	// List of index columns:
	int i, nCount = 0;
	m_cListCtrlIndexColumn.DeleteAllItems();
	POSITION pos = pIndex->m_listColumn.GetHeadPosition();
	CaColumnIndex* pIndexColumn = NULL;
	while(pos != NULL)
	{
		pIndexColumn = pIndex->m_listColumn.GetNext (pos);
		nCount = m_cListCtrlIndexColumn.GetItemCount();
		if (m_cListCtrlIndexColumn.InsertItem (nCount, pIndexColumn->m_strItem) != -1)
			m_cListCtrlIndexColumn.SetCheck (nCount, 1, pIndexColumn->m_bKeyed);
	}
	
	//
	// List locations:
	nCount = m_cListLocation.GetCount();
	for (i=0; i<nCount; i++)
		m_cListLocation.SetCheck (i, FALSE);
	pos = pIndex->m_listLocation.GetHeadPosition();
	while(pos != NULL)
	{
		strItem = pIndex->m_listLocation.GetNext (pos);
		i = m_cListLocation.FindStringExact (-1, strItem);
		if (i != LB_ERR)
			m_cListLocation.SetCheck (i, TRUE);
	}
	UpdateData (FALSE);
}

CString  CxDlgCreateIndex::GetIndexName()
{
	CString strName;
	strName.Format (_T("<Unnamed %02d>"), m_nNameSeq);
	m_nNameSeq++;
	return strName;
}

CaIndex* CxDlgCreateIndex::GetCurrentIndex(int& nSel)
{
	if (!IsWindow (m_cListIndexName.m_hWnd))
		return NULL;
	nSel = m_cListIndexName.GetNextItem (-1, LVNI_SELECTED);
	if (nSel == -1)
		return NULL;
	return (CaIndex*)m_cListIndexName.GetItemData (nSel);
}

void CxDlgCreateIndex::EnableDisableButtonOK()
{
	BOOL bEnable = TRUE;
	CaIndex* pIndex = NULL;
	int i, nCount = m_cListIndexName.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		pIndex = (CaIndex*)m_cListIndexName.GetItemData (i);
		//
		// Test if exist an empty index name:
		if (pIndex->m_strItem.IsEmpty())
		{
			bEnable = FALSE;
			break;
		}
		//
		// Test if exist an index does not have columns:
		if (pIndex->m_listColumn.GetCount() == 0)
		{
			bEnable = FALSE;
			break;
		}
	}
	m_cButtonOK.EnableWindow (bEnable);
}


void CxDlgCreateIndex::OnChangeEditIndexName() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.

	int nSel = -1;
	CString strItem;
	CaIndex* pIndex = GetCurrentIndex(nSel);
	if (!pIndex)
	{
		m_cButtonOK.EnableWindow (FALSE);
		return;
	}

	m_cEditIndexName.GetWindowText (strItem);
	pIndex->m_strItem = strItem;
	m_strIndexName = pIndex->m_strItem;
	m_cListIndexName.SetItemText (nSel, 0, strItem);
	EnableDisableButtonOK();
}

BOOL CxDlgCreateIndex::CheckValidate()
{
	CString strMsg = _T("");
	if (m_listIndex.IsEmpty())
		return FALSE;
	POSITION pos = m_listIndex.GetHeadPosition();
	while (pos != NULL)
	{
		CaIndex* pIndex = m_listIndex.GetNext (pos);
		if (pIndex && !pIndex->IsValide(strMsg))
		{
			AfxMessageBox (strMsg);
			return FALSE;
		}
	}
	return TRUE;
}

BOOL CxDlgCreateIndex::GenerateSyntax(CString& strStatement)
{
	int i, nCount = m_cListIndexName.GetItemCount();
	strStatement = _T("CREATE INDEX ");

	BOOL bOne = TRUE;
	POSITION pos = NULL;
	CString strColumnList = _T("");
	CString strListKey    = _T("");
	CString strText       = _T("");
	CString strWithClause = _T("");
	CaIndex* pIndex = NULL;

	//
	// Use the old syntax for compatibility:
	if (nCount == 1)
	{
		pIndex = (CaIndex*)m_cListIndexName.GetItemData (0);
		if (pIndex && pIndex->m_bUnique)
			strStatement = _T("CREATE UNIQUE INDEX ");
	}
	for (i=0; i<nCount; i++)
	{
		strText       = _T("");
		strColumnList = _T("");
		strListKey    = _T("");
		strWithClause = _T("");

		pIndex = (CaIndex*)m_cListIndexName.GetItemData (i);
		//
		// Generate the list of column 'strColumnList' and list of
		// Keys 'strListKey':
		pos = pIndex->m_listColumn.GetHeadPosition();
		while (pos != NULL)
		{
			CaColumnIndex* pColumn = pIndex->m_listColumn.GetNext (pos);
			if (!strColumnList.IsEmpty())
				strColumnList += _T(", ");
			strColumnList += QuoteIfNeeded(pColumn->m_strItem);
			//
			// Generate ASC|DESC: (actually not generate)
			if (pColumn->m_bKeyed)
			{
				if (!strListKey.IsEmpty())
					strListKey += _T(", ");
				strListKey += QuoteIfNeeded(pColumn->m_strItem);
			}
		}
		//
		// Generate the INDEX_NAME ON TABLE_NAME (list columns):
		strText += QuoteIfNeeded(pIndex->m_strItem);
		strText += _T(" ON ");

		strText += QuoteIfNeeded((LPCTSTR)m_strTableOwner); 
		strText += _T(".");
		strText += QuoteIfNeeded((LPCTSTR)m_strTable); 

		strText += _T(" (");
		strText += strColumnList;
		strText += _T(") ");
		//
		// Generate the UNIQUE:
		if (nCount > 1 && pIndex->m_bUnique)
			strText += _T("UNIQUE ");
		//
		// Generate the WITH CLAUSE:
		pIndex->GenerateWithClause(strWithClause, strListKey);
		if (!strWithClause.IsEmpty())
		{
			strText += cstrWith;
			strText += _T(" ");
			strText += strWithClause;
		}
		//
		// Generate the global syntax of create index:
		if (nCount > 1)
		{
			//
			// Multiple Index:
			if (!bOne)
				strStatement += _T(", ");
			strStatement += _T("(");
			strStatement += strText;
			strStatement += _T(")");
			bOne = FALSE;
		}
		else
		{
			//
			// Single Index:
			strStatement += strText;
		}
	}
	return TRUE;
}

void CxDlgCreateIndex::OnKillfocusEditIndexAllocation() 
{
	int nSel = -1;
	CString strItem;
	CaIndex* pIndex = GetCurrentIndex(nSel);
	if (!pIndex)
		return;
	m_cEditAllocation.GetWindowText (pIndex->m_strAllocation);
	pIndex->m_strAllocation.TrimLeft();
	pIndex->m_strAllocation.TrimRight();

	int nVal = atoi (pIndex->m_strAllocation);
	if (nVal < m_nAllocationMin)
	{
		pIndex->m_strAllocation.Format (_T("%d"), m_nAllocationMin);
		m_strAllocation = pIndex->m_strAllocation;
		m_cEditAllocation.SetWindowText (m_strAllocation);
		MessageBeep (-1);
	}
	else
	if (nVal > m_nAllocationMax)
	{
		pIndex->m_strAllocation.Format (_T("%d"), m_nAllocationMax);
		m_strAllocation = pIndex->m_strAllocation;
		m_cEditAllocation.SetWindowText (m_strAllocation);
		MessageBeep (-1);
	}
}

void CxDlgCreateIndex::OnKillfocusEditIndexExtend() 
{
	int nSel = -1;
	CString strItem;
	CaIndex* pIndex = GetCurrentIndex(nSel);
	if (!pIndex)
		return;
	m_cEditExtend.GetWindowText (pIndex->m_strExtend);
	pIndex->m_strExtend.TrimLeft();
	pIndex->m_strExtend.TrimRight();

	int nVal = atoi (pIndex->m_strExtend);
	if (nVal < m_nExtendMin)
	{
		pIndex->m_strExtend.Format (_T("%d"), m_nExtendMin);
		m_strExtend = pIndex->m_strExtend;
		m_cEditExtend.SetWindowText (m_strExtend);
		MessageBeep (-1);
	}
	else
	if (nVal > m_nExtendMax)
	{
		pIndex->m_strExtend.Format (_T("%d"), m_nExtendMax);
		m_strExtend = pIndex->m_strExtend;
		m_cEditExtend.SetWindowText (m_strExtend);
		MessageBeep (-1);
	}
}

void CxDlgCreateIndex::OnKillfocusEditIndexFillfactor() 
{
	int nSel = -1;
	CString strItem;
	CaIndex* pIndex = GetCurrentIndex(nSel);
	if (!pIndex)
		return;
	m_cEditFillfactor.GetWindowText (pIndex->m_strFillfactor);
	pIndex->m_strFillfactor.TrimLeft();
	pIndex->m_strFillfactor.TrimRight();

	int nVal = atoi (pIndex->m_strFillfactor);
	if (nVal < m_nFillfactorMin)
	{
		pIndex->m_strFillfactor.Format (_T("%d"), m_nFillfactorMin);
		m_strFillfactor = pIndex->m_strFillfactor;
		m_cEditFillfactor.SetWindowText (m_strFillfactor);
		MessageBeep (-1);
	}
	else
	if (nVal > m_nFillfactorMax)
	{
		pIndex->m_strFillfactor.Format (_T("%d"), m_nFillfactorMax);
		m_strFillfactor = pIndex->m_strFillfactor;
		m_cEditFillfactor.SetWindowText (m_strFillfactor);
		MessageBeep (-1);
	}
}

void CxDlgCreateIndex::OnKillfocusEditIndexLeaffill() 
{
	int nSel = -1;
	CString strItem;
	CaIndex* pIndex = GetCurrentIndex(nSel);
	if (!pIndex)
		return;
	m_cEditLeaffill.GetWindowText (pIndex->m_strLeaffill);
	pIndex->m_strLeaffill.TrimLeft();
	pIndex->m_strLeaffill.TrimRight();

	int nVal = atoi (pIndex->m_strLeaffill);
	if (nVal < m_nLeaffillMin)
	{
		pIndex->m_strLeaffill.Format (_T("%d"), m_nLeaffillMin);
		m_strLeaffill = pIndex->m_strLeaffill;
		m_cEditLeaffill.SetWindowText (m_strLeaffill);
		MessageBeep (-1);
	}
	else
	if (nVal > m_nLeaffillMax)
	{
		pIndex->m_strLeaffill.Format (_T("%d"), m_nLeaffillMax);
		m_strLeaffill = pIndex->m_strLeaffill;
		m_cEditLeaffill.SetWindowText (m_strLeaffill);
		MessageBeep (-1);
	}
}

void CxDlgCreateIndex::OnKillfocusEditIndexMaxpage() 
{
	int nSel = -1;
	CString strItem;
	CaIndex* pIndex = GetCurrentIndex(nSel);
	if (!pIndex)
		return;
	m_cEditMaxPage.GetWindowText (pIndex->m_strMaxPage);
	pIndex->m_strMaxPage.TrimLeft();
	pIndex->m_strMaxPage.TrimRight();

	int nVal = atoi (pIndex->m_strMaxPage);
	if (nVal < m_nMaxPageMin)
	{
		pIndex->m_strMaxPage.Format (_T("%d"), m_nMaxPageMin);
		m_strMaxPage = pIndex->m_strMaxPage;
		m_cEditMaxPage.SetWindowText (m_strMaxPage);
		MessageBeep (-1);
	}
	else
	if (nVal > m_nMaxPageMax)
	{
		pIndex->m_strMaxPage.Format (_T("%d"), m_nMaxPageMax);
		m_strMinPage = pIndex->m_strMaxPage;
		m_cEditMaxPage.SetWindowText (m_strMaxPage);
		MessageBeep (-1);
	}
}

void CxDlgCreateIndex::OnKillfocusEditIndexMinpage() 
{
	int nSel = -1;
	CString strItem;
	CaIndex* pIndex = GetCurrentIndex(nSel);
	if (!pIndex)
		return;
	m_cEditMinPage.GetWindowText (pIndex->m_strMinPage);
	pIndex->m_strMinPage.TrimLeft();
	pIndex->m_strMinPage.TrimRight();

	int nVal = atoi (pIndex->m_strMinPage);
	if (nVal < m_nMinPageMin)
	{
		pIndex->m_strMinPage.Format (_T("%d"), m_nMinPageMin);
		m_strMinPage = pIndex->m_strMinPage;
		m_cEditMinPage.SetWindowText (m_strMinPage);
		MessageBeep (-1);
	}
	else
	if (nVal > m_nMinPageMax)
	{
		pIndex->m_strMinPage.Format (_T("%d"), m_nMinPageMax);
		m_strMinPage = pIndex->m_strMinPage;
		m_cEditMinPage.SetWindowText (m_strMinPage);
		MessageBeep (-1);
	}
}

void CxDlgCreateIndex::OnKillfocusEditIndexNonleaffill() 
{
	int nSel = -1;
	CString strItem;
	CaIndex* pIndex = GetCurrentIndex(nSel);
	if (!pIndex)
		return;
	m_cEditNonleaffill.GetWindowText (pIndex->m_strNonleaffill);
	
	pIndex->m_strNonleaffill.TrimLeft();
	pIndex->m_strNonleaffill.TrimRight();
	int nVal = atoi (pIndex->m_strNonleaffill);
	if (nVal < m_nNonleaffillMin)
	{
		pIndex->m_strNonleaffill.Format (_T("%d"), m_nNonleaffillMin);
		m_strNonleaffill = pIndex->m_strNonleaffill;
		m_cEditNonleaffill.SetWindowText (m_strNonleaffill);
		MessageBeep (-1);
	}
	else
	if (nVal > m_nNonleaffillMax)
	{
		pIndex->m_strNonleaffill.Format (_T("%d"), m_nNonleaffillMax);
		m_strNonleaffill = pIndex->m_strNonleaffill;
		m_cEditNonleaffill.SetWindowText (m_strNonleaffill);
		MessageBeep (-1);
	}
}
