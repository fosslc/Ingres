/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : xdgidxop.cpp, Implementation File                                     //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II/Vdba.                                                       //
//    Author   : UK Sotheavut                       .                                  //
//                                                                                     //
//    Purpose  : Popup Dialog Box for the Constraint Enhancement (Index Option)        //
//    History:                                                                         //
//      02-Jun-2010 (drivi01)                                                          //
//        Removed BUFSIZE redefinition.  It's not needed here anymore.                 //
//        the constant is defined in main.h now. and main.h is included                //
//        in dom.h.                                                                    //
****************************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "xdgidxop.h"

//
// use the function Table_QueryStatisticColumns to get base table columns:
#include "parse.h"

extern "C"
{
#include "dbaset.h"
#include "tools.h"
#include "getdata.h"
#include "domdata.h"
#include "msghandl.h"
#include "dbadlg1.h"
LPSTRINGLIST VDBA2xGetIndexConstraints (LPCTSTR lpszTable, LPCTSTR lpszTableOwner, int* pError);
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

enum 
{
	INDEX_MINPAGE,
	INDEX_MAXPAGE,
	INDEX_ALLOCATION,
	INDEX_EXTEND,
	INDEX_FILLFACTOR,
	INDEX_LEAFFILL,
	INDEX_NONLEAFFILL,
	INDEX_STRUCTURE,
	INDEX_LOCATION
};

/////////////////////////////////////////////////////////////////////////////
// CxDlgIndexOption dialog


CxDlgIndexOption::CxDlgIndexOption(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgIndexOption::IDD, pParent)
{
	m_bDefineProperty = FALSE;
	m_nGenerateMode   = 0;
	m_pTable    = NULL;

	m_bFromCreateTable = FALSE;
	m_pParam       = NULL;
	m_strTable     = _T("");
	m_strTableOwner= _T("");
	m_nCallFor = 0;
	//{{AFX_DATA_INIT(CxDlgIndexOption)
	m_nStructure = 0;
	m_strMinPage = _T("");
	m_strMaxPage = _T("");
	m_strLeaffill = _T("");
	m_strNonleaffill = _T("");
	m_strAllocation = _T("");
	m_strExtend = _T("");
	m_strFillfactor = _T("");
	m_nComboIndex = 0;
	//}}AFX_DATA_INIT
	m_strDatabase = _T("iidbdb");
	m_strTitle    = _T("Index Enforcement");
	CaIndexStructure* pStruct = NULL;

	pStruct = new CaIndexStructure (IDX_BTREE, _T("Btree"));
	pStruct->AddComponent (_T("Leaffill"),      1, 100,     _T("80"));
	pStruct->AddComponent (_T("Non Leaffill"),  1, 100,     _T("70"));
	pStruct->AddComponent (_T("Allocation"),    4, 8388607, CaComponentValueConstraint::CONSTRAINT_EDITNUMBER, _T("4"));  // [4, 8388607]
	pStruct->AddComponent (_T("Extend"),        1, 8388607, CaComponentValueConstraint::CONSTRAINT_EDITNUMBER, _T("16")); // [1, 8388607]
	pStruct->AddComponent (_T("Fillfactor"),    1, 100,     _T("80"));
	m_listStructure.AddTail (pStruct);

	pStruct = new CaIndexStructure (IDX_ISAM, _T("Isam"));
	pStruct->AddComponent (_T("Allocation"),    4, 8388607, CaComponentValueConstraint::CONSTRAINT_EDITNUMBER, _T("4"));  // [4, 8388607]
	pStruct->AddComponent (_T("Extend"),        1, 8388607, CaComponentValueConstraint::CONSTRAINT_EDITNUMBER, _T("16")); // [1, 8388607]
	pStruct->AddComponent (_T("Fillfactor"),    1, 100,     _T("80"));
	m_listStructure.AddTail (pStruct);

	pStruct = new CaIndexStructure (IDX_HASH, _T("Hash"));
	pStruct->AddComponent (_T("Min Page"),      1, 8388607, _T("16"));
	pStruct->AddComponent (_T("Max Page"),      1, 8388607, _T("8388607"));
	pStruct->AddComponent (_T("Allocation"),    4, 8388607, CaComponentValueConstraint::CONSTRAINT_EDITNUMBER, _T("4"));  // [4, 8388607]
	pStruct->AddComponent (_T("Extend"),        1, 8388607, CaComponentValueConstraint::CONSTRAINT_EDITNUMBER, _T("16")); // [1, 8388607]
	pStruct->AddComponent (_T("Fillfactor"),    1, 100,     _T("50"));
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
	m_bAlter = FALSE;
}


void CxDlgIndexOption::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgIndexOption)
	DDX_Control(pDX, IDC_STATIC_INDEX, m_cStaticIndex);
	DDX_Control(pDX, IDC_COMBO_INDEXNAME2, m_cComboIndexNameDDList);
	DDX_Control(pDX, IDC_STATIC_INDEXNAME, m_cStaticIndexName);
	DDX_Control(pDX, IDC_MFC_CHECK1, m_cCheckProperty);
	DDX_Control(pDX, IDC_COMBO_INDEX, m_cComboIndex);
	DDX_Control(pDX, IDC_STATIC_LOCATION, m_cStaticLocation);
	DDX_Control(pDX, IDC_STATIC_EXTEND, m_cStaticExtend);
	DDX_Control(pDX, IDC_STATIC_ALLOCATION, m_cStaticAllocation);
	DDX_Control(pDX, IDC_STATIC_STRUCTURE, m_cStaticStructure);
	DDX_Control(pDX, IDC_STATIC_FILLFACTOR, m_cStaticFillfactor);
	DDX_Control(pDX, IDOK, m_cButtonOK);
	DDX_Control(pDX, IDC_COMBO_INDEXNAME, m_cComboIndexName);
	DDX_Control(pDX, IDC_EDIT_INDEX_FILLFACTOR, m_cEditFillfactor);
	DDX_Control(pDX, IDC_EDIT_INDEX_EXTEND, m_cEditExtend);
	DDX_Control(pDX, IDC_EDIT_INDEX_ALLOCATION, m_cEditAllocation);
	DDX_Control(pDX, IDC_EDIT_INDEX_NONLEAFFILL, m_cEditNonleaffill);
	DDX_Control(pDX, IDC_EDIT_INDEX_LEAFFILL, m_cEditLeaffill);
	DDX_Control(pDX, IDC_EDIT_INDEX_MAXPAGE, m_cEditMaxPage);
	DDX_Control(pDX, IDC_EDIT_INDEX_MINPAGE, m_cEditMinPage);
	DDX_Control(pDX, IDC_MFC_SPIN_NONLEAFFIL, m_cSpinNonleaffill);
	DDX_Control(pDX, IDC_MFC_SPIN_LEAFFIL, m_cSpinLeaffill);
	DDX_Control(pDX, IDC_MFC_SPIN_FILLFACTOR, m_cSpinFillfactor);
	DDX_Control(pDX, IDC_COMBO_INDEX_TRUCTURE, m_cComboStructure);
	DDX_CBIndex(pDX, IDC_COMBO_INDEX_TRUCTURE, m_nStructure);
	DDX_Text(pDX, IDC_EDIT_INDEX_MINPAGE, m_strMinPage);
	DDX_Text(pDX, IDC_EDIT_INDEX_MAXPAGE, m_strMaxPage);
	DDX_Text(pDX, IDC_EDIT_INDEX_LEAFFILL, m_strLeaffill);
	DDX_Text(pDX, IDC_EDIT_INDEX_NONLEAFFILL, m_strNonleaffill);
	DDX_Text(pDX, IDC_EDIT_INDEX_ALLOCATION, m_strAllocation);
	DDX_Text(pDX, IDC_EDIT_INDEX_EXTEND, m_strExtend);
	DDX_Text(pDX, IDC_EDIT_INDEX_FILLFACTOR, m_strFillfactor);
	DDX_Control(pDX, IDC_STATIC_NONLEAFFILL, m_cStaticNonleaffill);
	DDX_Control(pDX, IDC_STATIC_LEAFFILL, m_cStaticLeaffill);
	DDX_Control(pDX, IDC_STATIC_MAXPAGE, m_cStaticMaxPage);
	DDX_Control(pDX, IDC_STATIC_MINPAGE, m_cStaticMinPage);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgIndexOption, CDialog)
	//{{AFX_MSG_MAP(CxDlgIndexOption)
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_COMBO_INDEX_TRUCTURE, OnSelchangeComboIndexTructure)
	ON_EN_KILLFOCUS(IDC_EDIT_INDEX_ALLOCATION, OnKillfocusEditIndexAllocation)
	ON_EN_KILLFOCUS(IDC_EDIT_INDEX_EXTEND, OnKillfocusEditIndexExtend)
	ON_EN_KILLFOCUS(IDC_EDIT_INDEX_FILLFACTOR, OnKillfocusEditIndexFillfactor)
	ON_EN_KILLFOCUS(IDC_EDIT_INDEX_LEAFFILL, OnKillfocusEditIndexLeaffill)
	ON_EN_KILLFOCUS(IDC_EDIT_INDEX_MAXPAGE, OnKillfocusEditIndexMaxpage)
	ON_EN_KILLFOCUS(IDC_EDIT_INDEX_MINPAGE, OnKillfocusEditIndexMinpage)
	ON_EN_KILLFOCUS(IDC_EDIT_INDEX_NONLEAFFILL, OnKillfocusEditIndexNonleaffill)
	ON_CBN_EDITCHANGE(IDC_COMBO_INDEXNAME, OnEditchangeComboIndexname)
	ON_CBN_SELCHANGE(IDC_COMBO_INDEXNAME, OnSelchangeComboIndexname)
	ON_CBN_SELCHANGE(IDC_COMBO_INDEX, OnSelchangeComboIndex)
	ON_BN_CLICKED(IDC_MFC_CHECK1, OnCheckDefineProperty)
	ON_CBN_SELCHANGE(IDC_COMBO_INDEXNAME2, OnSelchangeComboIndexNameDDList)
	//}}AFX_MSG_MAP
	ON_CLBN_CHKCHANGE (IDC_LIST_INDEX_LOCATION, OnCheckChangeLocation)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgIndexOption message handlers

void CxDlgIndexOption::OnOK() 
{
	int nData = m_cComboIndex.GetCurSel();
	if (nData != CB_ERR)
		m_nGenerateMode = (int)m_cComboIndex.GetItemData (nData);
	else
		m_nGenerateMode = INDEXOPTION_USEDEFAULT;
	m_bDefineProperty = (m_cCheckProperty.GetCheck() == 1)? TRUE: FALSE;
	switch (m_nGenerateMode)
	{
	case INDEXOPTION_USEDEFAULT:
	case INDEXOPTION_NOINDEX:
	case INDEXOPTION_BASETABLE_STRUCTURE:
		//
		// The name will be generate at the Syntax Generation Level:
		m_strIndexName = _T("");
		break;
	case INDEXOPTION_USEEXISTING_INDEX:
		m_cComboIndexNameDDList.GetWindowText (m_strIndexName);
		m_strIndexName.TrimLeft();
		m_strIndexName.TrimRight();
		break;
	case INDEXOPTION_USER_DEFINED:
	case INDEXOPTION_USER_DEFINED_PROPERTY:
		m_cComboIndexName.GetWindowText (m_strIndexName);
		m_strIndexName.TrimLeft();
		m_strIndexName.TrimRight();
		if (m_strIndexName.CompareNoCase (VDBA_MfcResourceString(IDS_SYSTEM_GENERATED)) == 0)//_T("<system generated>")
			m_strIndexName = _T("");
		break;
	default:
		ASSERT (FALSE);
		break;
	}

	CDialog::OnOK();
	CaIndexStructure* pStructure = (CaIndexStructure*)m_cComboStructure.GetItemData (m_nStructure);
	m_strStructure = pStructure->m_strStructure;
	int i, nCount = m_cListLocation.GetCount();
	for (i=0; i<nCount; i++)
	{
		if (m_cListLocation.GetCheck(i))
		{
			CString strItem;
			m_cListLocation.GetText (i, strItem);
			m_listLocation.AddTail (strItem);
		}
	}
	if (m_strIndexName.CompareNoCase (VDBA_MfcResourceString(IDS_SYSTEM_GENERATED)) == 0 )//_T("<system generated>")
	{
		m_strIndexName = _T("");
	}

	if (m_strStructure.CompareNoCase (_T("Isam")) == 0)
	{
		m_strMinPage = _T("");
		m_strMaxPage = _T("");
		m_strLeaffill = _T("");
		m_strNonleaffill = _T("");
	}
	else
	if (m_strStructure.CompareNoCase (_T("Btree")) == 0)
	{
		m_strMinPage = _T("");
		m_strMaxPage = _T("");
	}
	else
	if (m_strStructure.CompareNoCase (_T("Hash")) == 0)
	{
		m_strLeaffill = _T("");
		m_strNonleaffill = _T("");
	}

	m_bDefineProperty = (m_cCheckProperty.GetCheck() == 1)? TRUE: FALSE;
}

void CxDlgIndexOption::OnDestroy() 
{
	CDialog::OnDestroy();
	while (!m_listStructure.IsEmpty())
		delete m_listStructure.RemoveHead();
	PopHelpId();
}

BOOL CxDlgIndexOption::OnInitDialog() 
{
	CDialog::OnInitDialog();
	LPTABLEPARAMS pTable = (LPTABLEPARAMS)m_pTable;
	INDEXOPTION* pIndex = (INDEXOPTION*)m_pParam;
	ASSERT (pIndex);
	if (!pIndex)
		return FALSE;
	if (pIndex->bAlter)
		SetAlter();
	VERIFY (m_cListLocation.SubclassDlgItem (IDC_LIST_INDEX_LOCATION, this));
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	if (hFont == NULL)
		hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
	// m_cListLocation.SendMessage(WM_SETFONT, (WPARAM)hFont);
	m_cComboIndexNameDDList.ResetContent();
	m_cComboIndexName.AddString (VDBA_MfcResourceString(IDS_SYSTEM_GENERATED));//_T("<system generated>")
	SetDisplayMode();
	
	InitializeComboIndex();
	if (!pTable->bCreate)
		QueryExistingIndexes();

	InitializeSharedIndexes();
	//
	// Query the list of locations:
	InitializeLocations();
	SetWindowText (m_strTitle);
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
	if (!pIndex->bAlter)
	{
		int nItemData = 0;
		int i, nItemPos = 0, nCount = m_cComboIndex.GetCount();
		for (i=0; i<nCount; i++)
		{
			nItemData = (int)m_cComboIndex.GetItemData(i);
			if (nItemData == pIndex->nGenerateMode)
			{
				nItemPos = i;
				break;
			}
			else
			if (nItemData == INDEXOPTION_USER_DEFINED && pIndex->nGenerateMode == INDEXOPTION_USER_DEFINED_PROPERTY)
			{
				if (pIndex->bDefineProperty)
				{
					nItemPos = i;
					break;
				}
			}
		}
		//
		// Newly created indexe.
		switch (pIndex->nGenerateMode)
		{
		case INDEXOPTION_USEDEFAULT:
		case INDEXOPTION_NOINDEX:
		case INDEXOPTION_BASETABLE_STRUCTURE:
		case INDEXOPTION_USEEXISTING_INDEX:
			m_cComboIndex.SetCurSel (nItemPos);
			OnSelchangeComboIndex();
			break;
		case INDEXOPTION_USER_DEFINED:
		case INDEXOPTION_USER_DEFINED_PROPERTY:
			m_cComboIndex.SetCurSel (nItemPos);
			OnSelchangeComboIndex();
			DisplayIndexInfo ((LPVOID)pIndex);
			OnEditchangeComboIndexname();
			break;
		default:
			//
			// Specify new index:
			m_cComboIndex.SetCurSel (0);
			OnSelchangeComboIndex();
			DisplayIndexInfo ((LPVOID)pIndex);
			OnEditchangeComboIndexname();
		}
	}
	else
	{
		DisplayIndexInfo ((LPVOID)pIndex);
		OnEditchangeComboIndexname();
	}

	CRect r;
	m_cComboIndexName.GetWindowRect (r);
	ScreenToClient (r);
	m_cComboIndexNameDDList.MoveWindow (r);
	EnableOKButton();
	switch (m_nCallFor)
	{
	case 0:
		PushHelpId (HELPID_IDD_INDEX_OPTION_PRIMARYKEY);
		break;
	case 1:
		PushHelpId (HELPID_IDD_INDEX_OPTION_UNIQUEKEY);
		break;
	case 2:
		PushHelpId (HELPID_IDD_INDEX_OPTION_FOREIGNKEY);
		break;
	default:
		PushHelpId (0);
		break;
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CxDlgIndexOption::InitializeLocations()
{
	int     hNode   = -1, err = RES_ERR;
	BOOL    bSystem = TRUE; // Force to true to get II_DATABASE ...
	TCHAR   tchszObject[MAXOBJECTNAME];
	TCHAR   tchszFilter[MAXOBJECTNAME];
	LPUCHAR aparents[MAXPLEVEL];
	if (m_bAlter)
		return TRUE;
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
	#if 0
	//
	// Select II_DATABASE by default
	m_strIIDatabase.LoadString (IDS_IIDATABASE);
	if (!m_strIIDatabase.IsEmpty())
	{
		int nIdx = m_cListLocation.FindStringExact (-1, m_strIIDatabase);
		if (nIdx != LB_ERR)
			m_cListLocation.SetCheck (nIdx, TRUE);
	}
	#endif

	return TRUE;
}

void CxDlgIndexOption::IndexTructurePrepareControls(int nStructure)
{
	m_cStaticLeaffill.EnableWindow (FALSE);
	m_cStaticNonleaffill.EnableWindow (FALSE);
	m_cEditLeaffill.EnableWindow (FALSE);
	m_cEditNonleaffill.EnableWindow (FALSE);
	m_cSpinLeaffill.EnableWindow (FALSE);
	m_cSpinNonleaffill.EnableWindow (FALSE);
	m_cStaticMinPage.EnableWindow (FALSE);
	m_cStaticMaxPage.EnableWindow (FALSE);
	m_cEditMinPage.EnableWindow (FALSE);
	m_cEditMaxPage.EnableWindow (FALSE);

	switch (nStructure)
	{
	case IDX_BTREE:
		m_cStaticLeaffill.EnableWindow (TRUE);
		m_cStaticNonleaffill.EnableWindow (TRUE);
		m_cEditLeaffill.EnableWindow (TRUE);
		m_cEditNonleaffill.EnableWindow (TRUE);
		m_cSpinLeaffill.EnableWindow (TRUE);
		m_cSpinNonleaffill.EnableWindow (TRUE);
		break;
	case IDX_HASH:
		m_cStaticMinPage.EnableWindow (TRUE);
		m_cStaticMaxPage.EnableWindow (TRUE);
		m_cEditMinPage.EnableWindow (TRUE);
		m_cEditMaxPage.EnableWindow (TRUE);
	case IDX_ISAM:
		break;
	}
}

void CxDlgIndexOption::OnSelchangeComboIndexTructure() 
{
	m_nStructure = m_cComboStructure.GetCurSel();
	if (m_nStructure == CB_ERR)
		return;
	CaIndexStructure* pStruct = (CaIndexStructure*)m_cComboStructure.GetItemData(m_nStructure);
	IndexTructurePrepareControls(pStruct->m_nStructType);
	
	/*

	if (pStruct->m_nStructType == IDX_BTREE)
	{
		m_cStaticLeaffill.EnableWindow (TRUE);
		m_cStaticNonleaffill.EnableWindow (TRUE);
		m_cEditLeaffill.EnableWindow (TRUE);
		m_cEditNonleaffill.EnableWindow (TRUE);
		m_cSpinLeaffill.EnableWindow (TRUE);
		m_cSpinNonleaffill.EnableWindow (TRUE);
	}
	else
	{
		m_cStaticLeaffill.EnableWindow (FALSE);
		m_cStaticNonleaffill.EnableWindow (FALSE);
		m_cEditLeaffill.EnableWindow (FALSE);
		m_cEditNonleaffill.EnableWindow (FALSE);
		m_cSpinLeaffill.EnableWindow (FALSE);
		m_cSpinNonleaffill.EnableWindow (FALSE);
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
	*/
	m_strMinPage = _T("");
	m_strMaxPage = _T("");
	m_strLeaffill = _T("");
	m_strNonleaffill = _T("");
	m_strAllocation = _T("");
	m_strExtend = _T("");
	m_strFillfactor = _T("");

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
				m_nLeaffillMin = pComp->m_nLower;
				m_nLeaffillMax = pComp->m_nUpper;
			}
			else
			if (pComp->m_strComponentName == _T("Non Leaffill"))
			{
				m_cSpinNonleaffill.SetRange (pComp->m_nLower, pComp->m_nUpper);
				m_strNonleaffill = pComp->m_strValue;
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
				m_nMinPageMin = pComp->m_nLower;
				m_nMinPageMax = pComp->m_nUpper;
			}
			else
			if (pComp->m_strComponentName == _T("Max Page"))
			{
				m_strMaxPage = pComp->m_strValue;
				m_nMaxPageMin = pComp->m_nLower;
				m_nMaxPageMax = pComp->m_nUpper;
			}
		}
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
			m_nAllocationMin = pComp->m_nLower;
			m_nAllocationMax = pComp->m_nUpper;
		}
		else
		if (pComp->m_strComponentName == _T("Extend"))
		{	
			m_strExtend = pComp->m_strValue;
			m_nExtendMin = pComp->m_nLower;
			m_nExtendMax = pComp->m_nUpper;
		}
		else
		if (pComp->m_strComponentName == _T("Fillfactor"))
		{
			m_cSpinFillfactor.SetRange (pComp->m_nLower, pComp->m_nUpper);
			m_strFillfactor = pComp->m_strValue;
			m_nFillfactorMin = pComp->m_nLower;
			m_nFillfactorMax = pComp->m_nUpper;
		}
	}
	m_strStructure = pStruct->m_strStructure;
	UpdateData (FALSE);
	SharedIndexChange (INDEX_STRUCTURE, _T(""));
}

void CxDlgIndexOption::OnKillfocusEditIndexAllocation() 
{
	m_cEditAllocation.GetWindowText (m_strAllocation);
	m_strAllocation.TrimLeft();
	m_strAllocation.TrimRight();

	int nVal = atoi (m_strAllocation);
	if (nVal < m_nAllocationMin)
	{
		m_strAllocation.Format (_T("%d"), m_nAllocationMin);
		m_cEditAllocation.SetWindowText (m_strAllocation);
		MessageBeep (-1);
	}
	else
	if (nVal > m_nAllocationMax)
	{
		m_strAllocation.Format (_T("%d"), m_nAllocationMax);
		m_cEditAllocation.SetWindowText (m_strAllocation);
		MessageBeep (-1);
	}
	SharedIndexChange (INDEX_ALLOCATION, m_strAllocation);
}

void CxDlgIndexOption::OnKillfocusEditIndexExtend() 
{
	m_cEditExtend.GetWindowText (m_strExtend);
	m_strExtend.TrimLeft();
	m_strExtend.TrimRight();

	int nVal = atoi (m_strExtend);
	if (nVal < m_nExtendMin)
	{
		m_strExtend.Format (_T("%d"), m_nExtendMin);
		m_cEditExtend.SetWindowText (m_strExtend);
		MessageBeep (-1);
	}
	else
	if (nVal > m_nExtendMax)
	{
		m_strExtend.Format (_T("%d"), m_nExtendMax);
		m_cEditExtend.SetWindowText (m_strExtend);
		MessageBeep (-1);
	}
	SharedIndexChange (INDEX_EXTEND, m_strExtend);
}

void CxDlgIndexOption::OnKillfocusEditIndexFillfactor() 
{
	m_cEditFillfactor.GetWindowText (m_strFillfactor);
	m_strFillfactor.TrimLeft();
	m_strFillfactor.TrimRight();

	int nVal = atoi (m_strFillfactor);
	if (nVal < m_nFillfactorMin)
	{
		m_strFillfactor.Format (_T("%d"), m_nFillfactorMin);
		m_cEditFillfactor.SetWindowText (m_strFillfactor);
		MessageBeep (-1);
	}
	else
	if (nVal > m_nFillfactorMax)
	{
		m_strFillfactor.Format (_T("%d"), m_nFillfactorMax);
		m_cEditFillfactor.SetWindowText (m_strFillfactor);
		MessageBeep (-1);
	}
	SharedIndexChange (INDEX_FILLFACTOR, m_strFillfactor);
}

void CxDlgIndexOption::OnKillfocusEditIndexLeaffill() 
{
	m_cEditLeaffill.GetWindowText (m_strLeaffill);
	m_strLeaffill.TrimLeft();
	m_strLeaffill.TrimRight();

	int nVal = atoi (m_strLeaffill);
	if (nVal < m_nLeaffillMin)
	{
		m_strLeaffill.Format (_T("%d"), m_nLeaffillMin);
		m_cEditLeaffill.SetWindowText (m_strLeaffill);
		MessageBeep (-1);
	}
	else
	if (nVal > m_nLeaffillMax)
	{
		m_strLeaffill.Format (_T("%d"), m_nLeaffillMax);
		m_cEditLeaffill.SetWindowText (m_strLeaffill);
		MessageBeep (-1);
	}
	SharedIndexChange (INDEX_LEAFFILL, m_strLeaffill);
}

void CxDlgIndexOption::OnKillfocusEditIndexMaxpage() 
{
	m_cEditMaxPage.GetWindowText (m_strMaxPage);
	m_strMaxPage.TrimLeft();
	m_strMaxPage.TrimRight();

	int nVal = atoi (m_strMaxPage);
	if (nVal < m_nMaxPageMin)
	{
		m_strMaxPage.Format (_T("%d"), m_nMaxPageMin);
		m_cEditMaxPage.SetWindowText (m_strMaxPage);
		MessageBeep (-1);
	}
	else
	if (nVal > m_nMaxPageMax)
	{
		m_strMaxPage.Format (_T("%d"), m_nMaxPageMax);
		m_cEditMaxPage.SetWindowText (m_strMaxPage);
		MessageBeep (-1);
	}
	SharedIndexChange (INDEX_MAXPAGE, m_strMaxPage);
}

void CxDlgIndexOption::OnKillfocusEditIndexMinpage() 
{
	m_cEditMinPage.GetWindowText (m_strMinPage);
	m_strMinPage.TrimLeft();
	m_strMinPage.TrimRight();

	int nVal = atoi (m_strMinPage);
	if (nVal < m_nMinPageMin)
	{
		m_strMinPage.Format (_T("%d"), m_nMinPageMin);
		m_cEditMinPage.SetWindowText (m_strMinPage);
		MessageBeep (-1);
	}
	else
	if (nVal > m_nMinPageMax)
	{
		m_strMinPage.Format (_T("%d"), m_nMinPageMax);
		m_cEditMinPage.SetWindowText (m_strMinPage);
		MessageBeep (-1);
	}
	SharedIndexChange (INDEX_MINPAGE, m_strMinPage);
}

void CxDlgIndexOption::OnKillfocusEditIndexNonleaffill() 
{
	m_cEditNonleaffill.GetWindowText (m_strNonleaffill);
	m_strNonleaffill.TrimLeft();
	m_strNonleaffill.TrimRight();
	int nVal = atoi (m_strNonleaffill);
	if (nVal < m_nNonleaffillMin)
	{
		m_strNonleaffill.Format (_T("%d"), m_nNonleaffillMin);
		m_cEditNonleaffill.SetWindowText (m_strNonleaffill);
		MessageBeep (-1);
	}
	else
	if (nVal > m_nNonleaffillMax)
	{
		m_strNonleaffill.Format (_T("%d"), m_nNonleaffillMax);
		m_cEditNonleaffill.SetWindowText (m_strNonleaffill);
		MessageBeep (-1);
	}
	SharedIndexChange (INDEX_NONLEAFFILL, m_strNonleaffill);
}

void CxDlgIndexOption::OnEditchangeComboIndexname() 
{
	m_cComboIndexName.GetWindowText (m_strIndexName);
	m_strIndexName.TrimLeft();
	m_strIndexName.TrimRight();
	int nFound = m_cComboIndexName.FindStringExact (-1, m_strIndexName);
	if (nFound != CB_ERR)
	{
		m_cComboIndexName.SetCurSel (nFound);
		OnSelchangeComboIndexname();
	}
	EnableOKButton();
}

void CxDlgIndexOption::OnSelchangeComboIndexname() 
{
	int nCurSel = m_cComboIndexName.GetCurSel();
	if (nCurSel == CB_ERR)
		return;
	INDEXOPTION* pIndex = (INDEXOPTION*)m_cComboIndexName.GetItemData (nCurSel);
	if (pIndex)
	{
		if (pIndex->bDefineProperty)
		{
			ShowControls(SW_SHOW);
			m_cCheckProperty.SetCheck (TRUE);
			DisplayIndexInfo ((LPVOID)pIndex, TRUE);
		}
		else
		{
			m_cCheckProperty.SetCheck (FALSE);
			ShowControls(SW_HIDE);
		}
	}
	else
	{
		//
		// Hide the controls:
		ShowControls ((m_cCheckProperty.GetCheck() == 1)? SW_SHOW: SW_HIDE);
	}
	EnableOKButton();
}

void CxDlgIndexOption::ShowControls (int nShow)
{
	m_cListLocation.ShowWindow (nShow);
	m_cComboStructure.ShowWindow (nShow);
	m_cStaticLeaffill.ShowWindow (nShow);
	m_cStaticNonleaffill.ShowWindow (nShow);
	m_cEditLeaffill.ShowWindow (nShow);
	m_cEditNonleaffill.ShowWindow (nShow);
	m_cSpinLeaffill.ShowWindow (nShow);
	m_cSpinNonleaffill.ShowWindow (nShow);

	m_cStaticMinPage.ShowWindow (nShow);
	m_cStaticMaxPage.ShowWindow (nShow);
	m_cEditMinPage.ShowWindow (nShow);
	m_cEditMaxPage.ShowWindow (nShow);

	m_cSpinFillfactor.ShowWindow (nShow);
	m_cEditFillfactor.ShowWindow (nShow);
	m_cEditAllocation.ShowWindow (nShow);
	m_cEditExtend.ShowWindow (nShow);

	m_cStaticAllocation.ShowWindow (nShow);
	m_cStaticExtend.ShowWindow (nShow);
	m_cStaticFillfactor.ShowWindow (nShow);
	m_cStaticLocation.ShowWindow (nShow);
	m_cStaticStructure.ShowWindow (nShow);
}

//
// If m_bAlter is TRUE, then everything is only for information:
void CxDlgIndexOption::SetDisplayMode(BOOL bShared)
{
	if (!m_bAlter)
		return;
	int i = 0;
	CWnd*  pWndSpin [] ={&m_cSpinFillfactor, &m_cSpinLeaffill, &m_cSpinNonleaffill, NULL};
	CWnd*  pWndCtrl [] ={&m_cButtonOK, &m_cComboIndexName, &m_cComboStructure, &m_cListLocation, NULL};
	CEdit* pWndEdit [] =
	{
		&m_cEditFillfactor,
		&m_cEditAllocation,
		&m_cEditExtend,
		&m_cEditMinPage,
		&m_cEditMaxPage,
		&m_cEditLeaffill,
		&m_cEditNonleaffill,
		NULL
	};

	for (i=0; pWndEdit[i]; i++)
		pWndEdit[i]->SetReadOnly (TRUE);
	for (i=0; pWndSpin[i]; i++)
		pWndSpin[i]->EnableWindow(FALSE);
	for (i=0; pWndCtrl[i]; i++)
		pWndCtrl[i]->EnableWindow(FALSE);

	m_cStaticIndex.ShowWindow (SW_HIDE);
	m_cComboIndex.ShowWindow (SW_HIDE);
	m_cCheckProperty.ShowWindow (SW_HIDE);
}


void INDEXOPTION_STRUCT2IDX (CxDlgIndexOption* pDest,   LPVOID lpSource)
{
	INDEXOPTION* pIdx = (INDEXOPTION*)lpSource;

	pDest->m_strIndexName  = pIdx->tchszIndexName;
	pDest->m_strStructure  = pIdx->tchszStructure;
	pDest->m_strFillfactor = pIdx->tchszFillfactor;
	pDest->m_strMinPage    = pIdx->tchszMinPage;
	pDest->m_strMaxPage    = pIdx->tchszMaxPage;
	pDest->m_strLeaffill   = pIdx->tchszLeaffill;
	pDest->m_strNonleaffill= pIdx->tchszNonleaffill;
	pDest->m_strAllocation = pIdx->tchszAllocation;
	pDest->m_strExtend     = pIdx->tchszExtend;

	pDest->m_bDefineProperty = pIdx->bDefineProperty;
	pDest->m_nGenerateMode   = pIdx->nGenerateMode;

	pDest->m_listLocation.RemoveAll();
	LPSTRINGLIST ls = pIdx->pListLocation;
	while (ls != NULL)
	{
		pDest->m_listLocation.AddTail (ls->lpszString);
		ls = ls->next;
	}
}


void INDEXOPTION_IDX2STRUCT (CxDlgIndexOption* pSource, LPVOID lpDest)
{
	INDEXOPTION* pIdx = (INDEXOPTION*)lpDest;
	lstrcpy (pIdx->tchszIndexName,  pSource->m_strIndexName);
	lstrcpy (pIdx->tchszStructure,  pSource->m_strStructure);

	lstrcpy (pIdx->tchszFillfactor, pSource->m_strFillfactor);
	lstrcpy (pIdx->tchszMinPage,    pSource->m_strMinPage);
	lstrcpy (pIdx->tchszMaxPage,    pSource->m_strMaxPage);
	lstrcpy (pIdx->tchszLeaffill,   pSource->m_strLeaffill);
	lstrcpy (pIdx->tchszNonleaffill,pSource->m_strNonleaffill);
	lstrcpy (pIdx->tchszAllocation, pSource->m_strAllocation);
	lstrcpy (pIdx->tchszExtend,     pSource->m_strExtend);

	pIdx->bDefineProperty = pSource->m_bDefineProperty;
	pIdx->nGenerateMode   = pSource->m_nGenerateMode;
	pIdx->pListLocation = StringList_Done (pIdx->pListLocation);
	POSITION pos = pSource->m_listLocation.GetHeadPosition();
	while (pos != NULL)
	{
		CString strItem = pSource->m_listLocation.GetNext (pos);
		pIdx->pListLocation = StringList_AddObject (pIdx->pListLocation, strItem, NULL);
	}
}

void CxDlgIndexOption::OnSelchangeComboIndex() 
{
	long lStyle = 0;
	int nSel  = m_cComboIndex.GetCurSel();
	int nItem = (int)m_cComboIndex.GetItemData (nSel);
	switch (nItem)
	{
	case INDEXOPTION_USEDEFAULT:
	case INDEXOPTION_NOINDEX:
	case INDEXOPTION_BASETABLE_STRUCTURE:
		ShowControls (SW_HIDE);
		m_cCheckProperty.ShowWindow (SW_HIDE);
		m_cComboIndexName.ShowWindow (SW_HIDE);
		m_cComboIndexNameDDList.ShowWindow (SW_HIDE);
		m_cStaticIndexName.ShowWindow (SW_HIDE);
		break;
	case INDEXOPTION_USEEXISTING_INDEX:
		ShowControls (SW_HIDE);
		m_cCheckProperty.ShowWindow (SW_HIDE);
		m_cStaticIndexName.ShowWindow (SW_SHOW);
		m_cComboIndexName.ShowWindow (SW_HIDE);
		m_cComboIndexNameDDList.ShowWindow (SW_SHOW);
		break;
	case INDEXOPTION_USER_DEFINED:
		m_cCheckProperty.ShowWindow (SW_SHOW);
		ShowControls (SW_HIDE);
		m_cStaticIndexName.ShowWindow (SW_SHOW);
		m_cComboIndexName.ShowWindow (SW_SHOW);
		m_cComboIndexNameDDList.ShowWindow (SW_HIDE);
		if (m_cComboIndexName.GetWindowTextLength() == 0)
			m_cComboIndexName.SetCurSel (0);
		break;
	case INDEXOPTION_USER_DEFINED_PROPERTY:
		m_cCheckProperty.ShowWindow (SW_SHOW);
		ShowControls (SW_SHOW);
		m_cStaticIndexName.ShowWindow (SW_SHOW);
		m_cComboIndexName.ShowWindow (SW_SHOW);
		m_cComboIndexNameDDList.ShowWindow (SW_HIDE);
		if (m_cComboIndexName.GetWindowTextLength() == 0)
			m_cComboIndexName.SetCurSel (0);
		break;
	default:
		ASSERT (FALSE);
		break;
	}
	EnableOKButton();
}


void CxDlgIndexOption::InitializeComboIndex()
{
	LPTABLEPARAMS pTable = (LPTABLEPARAMS)m_pTable;
	int nIdx = 0;
	TCHAR tchszIndex[5][32];// =
	lstrcpy(tchszIndex[0],VDBA_MfcResourceString(IDS_USE_DEFAULT));   //_T("<use default>")
	lstrcpy(tchszIndex[1],VDBA_MfcResourceString(IDS_NO_INDEX));      //_T("<no index>"),
	lstrcpy(tchszIndex[2],VDBA_MfcResourceString(IDS_BASE_TABLE));    //_T("<base table structure>"),
	lstrcpy(tchszIndex[3],VDBA_MfcResourceString(IDS_EXISTING_INDEX));//_T("<use existing index>"),
	lstrcpy(tchszIndex[4],VDBA_MfcResourceString(IDS_USER_DEFINED));  //_T("<user defined>")
	int nData [6] = 
	{
		INDEXOPTION_USEDEFAULT,
		INDEXOPTION_NOINDEX,
		INDEXOPTION_BASETABLE_STRUCTURE,
		INDEXOPTION_USEEXISTING_INDEX,
		INDEXOPTION_USER_DEFINED,
		INDEXOPTION_USER_DEFINED_PROPERTY
	};
	for (int i=0; i<5; i++)
	{
		if (m_nCallFor != 2 && i == 1)
			continue;
		if (pTable->bCreate && (i ==  2 || i == 3))
			continue;
		nIdx = m_cComboIndex.AddString (tchszIndex[i]);
		if (nIdx != CB_ERR)
			m_cComboIndex.SetItemData (nIdx, (DWORD)nData[i]);
	}
}

void CxDlgIndexOption::OnCheckDefineProperty() 
{
	int idxFound = m_cComboIndex.FindStringExact (-1,VDBA_MfcResourceString(IDS_USER_DEFINED)); //_T("<user defined>")
	ASSERT (idxFound != CB_ERR);
	if (m_cCheckProperty.GetCheck() == 1 && idxFound != CB_ERR)
	{
		m_cComboIndex.SetItemData (idxFound, (DWORD)INDEXOPTION_USER_DEFINED_PROPERTY);
		ShowControls (SW_SHOW);
	}
	else
	{
		m_cComboIndex.SetItemData ((idxFound == CB_ERR)? 4: idxFound, (DWORD)INDEXOPTION_USER_DEFINED);
		ShowControls (SW_HIDE);
	}
	EnableOKButton();
}


BOOL CxDlgIndexOption::QueryExistingIndexes()
{
	LPUCHAR aparentsTemp[MAXPLEVEL];
	TCHAR   buf[MAXOBJECTNAME];
	TCHAR   bufOwner[MAXOBJECTNAME];
	TCHAR   bufComplim[MAXOBJECTNAME];
	TCHAR   bufFull[MAXOBJECTNAME*2];

	try
	{
		LPTABLEPARAMS pTable = (LPTABLEPARAMS)m_pTable;
		aparentsTemp[0] = (LPUCHAR)(LPCTSTR)m_strDatabase;
		aparentsTemp[1] = StringWithOwner ((LPUCHAR)Quote4DisplayIfNeeded((LPCTSTR)pTable->objectname), pTable->szSchema, (LPUCHAR)bufFull);
		aparentsTemp[2] = NULL;
		memset (&bufComplim, '\0', sizeof(bufComplim));
		memset (&bufOwner, '\0', sizeof(bufOwner));
		
		int ires =  DOMGetFirstObject(
			GetCurMdiNodeHandle (),
			OT_INDEX,
			2,
			aparentsTemp,
			TRUE,
			NULL,
			(LPUCHAR)buf,
			(LPUCHAR)bufOwner,
			(LPUCHAR)bufComplim);
		
		while (ires == RES_SUCCESS) 
		{
			if (buf [0] && buf [0] != _T('$'))
				m_cComboIndexNameDDList.AddString (buf);
			ires = DOMGetNextObject((LPUCHAR)buf, (LPUCHAR)bufOwner, (LPUCHAR)bufComplim);
		}
	}
	catch (...)
	{
		TRACE0 ("Internal error: CxDlgIndexOption::QueryExistingIndexes()\n");
		return FALSE;
	}
	return TRUE;
}

BOOL CxDlgIndexOption::InitializeSharedIndexes()
{
	//
	// Problem in design:
	// Do not use this function:
	return FALSE;

	int nIdx = CB_ERR;
	LPTABLEPARAMS pTable = (LPTABLEPARAMS)m_pTable;
	//
	// Index of Primary Key:
	if (!pTable->primaryKey.bAlter && StringList_Count (pTable->primaryKey.pListKey) > 0)
	{
		//
		// Only if index name is specified:
		if (pTable->primaryKey.constraintWithClause.tchszIndexName[0] && 
		    pTable->primaryKey.constraintWithClause.tchszIndexName[0] != _T('$'))
		{
			nIdx = m_cComboIndexName.FindStringExact (-1, pTable->primaryKey.constraintWithClause.tchszIndexName);
			if (nIdx == CB_ERR)
			{
				nIdx = m_cComboIndexName.AddString (pTable->primaryKey.constraintWithClause.tchszIndexName);
				if (nIdx != CB_ERR)
					m_cComboIndexName.SetItemData (nIdx, (DWORD)&(pTable->primaryKey.constraintWithClause));
			}
		}
	}
	//
	// Index of Unique Key:
	LPTLCUNIQUE ls = pTable->lpUnique;
	while (ls)
	{
		if (StringList_Count (ls->lpcolumnlist) > 0 && !ls->bAlter)
		{
			//
			// Only if index name is specified:
			if (ls->constraintWithClause.tchszIndexName[0] && ls->constraintWithClause.tchszIndexName[0] != _T('$'))
			{
				nIdx = m_cComboIndexName.FindStringExact (-1, ls->constraintWithClause.tchszIndexName);
				if (nIdx == CB_ERR)
				{
					nIdx = m_cComboIndexName.AddString (ls->constraintWithClause.tchszIndexName);
					if (nIdx != CB_ERR)
						m_cComboIndexName.SetItemData (nIdx, (DWORD)&(ls->constraintWithClause));
				}
			}
		}
		ls = ls->next;
	}
	//
	// Index of Foreign Key:
	LPOBJECTLIST lsr = pTable->lpReferences;
	LPREFERENCEPARAMS lpRef = NULL;
	while (lsr)
	{
		lpRef = (LPREFERENCEPARAMS)lsr->lpObject;
		if (!lpRef->bAlter)
		{
			if (lpRef->constraintWithClause.tchszIndexName[0] && lpRef->constraintWithClause.tchszIndexName[0] != _T('$'))
			{
				nIdx = m_cComboIndexName.FindStringExact (-1, lpRef->constraintWithClause.tchszIndexName);
				if (nIdx == CB_ERR)
				{
					nIdx = m_cComboIndexName.AddString (lpRef->constraintWithClause.tchszIndexName);
					if (nIdx != CB_ERR)
						m_cComboIndexName.SetItemData (nIdx, (DWORD)&(lpRef->constraintWithClause));
				}
			}
		}
		lsr = (LPOBJECTLIST)lsr->lpNext;
	}

	return TRUE;
}



void CxDlgIndexOption::DisplayIndexInfo(LPVOID lpvIndex, BOOL bShared)
{
	INDEXOPTION* pIndex = (INDEXOPTION*)lpvIndex;
	int nSel, nFoundIndexName = 0;
	if (pIndex)
	{
		nFoundIndexName = m_cComboIndexName.FindStringExact (-1, pIndex->tchszIndexName);
		m_nStructure = m_cComboStructure.FindStringExact (-1, pIndex->tchszStructure);
	}
	m_cComboStructure.SetCurSel ((m_nStructure == CB_ERR)? 0: m_nStructure);
	nSel = m_cComboStructure.GetCurSel();
	CaIndexStructure* pStruct = (CaIndexStructure*)m_cComboStructure.GetItemData(nSel);
	if (!bShared)
		OnSelchangeComboIndexTructure();
	else
		IndexTructurePrepareControls(pStruct->m_nStructType);

	if (pIndex->tchszIndexName[0] && nFoundIndexName == CB_ERR)
		m_cComboIndexName.SetWindowText (pIndex->tchszIndexName);
	else
		m_cComboIndexName.SetCurSel ((nFoundIndexName == CB_ERR)? 0: nFoundIndexName);

	if (pIndex)
	{
		if (pIndex->tchszFillfactor[0])
			m_cEditFillfactor.SetWindowText(pIndex->tchszFillfactor);
		if (pIndex->tchszMinPage[0])
			m_cEditMinPage.SetWindowText(pIndex->tchszMinPage);
		if (pIndex->tchszMaxPage[0])
			m_cEditMaxPage.SetWindowText(pIndex->tchszMaxPage);
		if (pIndex->tchszLeaffill[0])
			m_cEditLeaffill.SetWindowText(pIndex->tchszLeaffill);
		if (pIndex->tchszNonleaffill[0])
			m_cEditNonleaffill.SetWindowText(pIndex->tchszNonleaffill);
		if (pIndex->tchszAllocation[0])
			m_cEditAllocation.SetWindowText(pIndex->tchszAllocation);
		if (pIndex->tchszExtend[0])
			m_cEditExtend.SetWindowText(pIndex->tchszExtend);

		LPSTRINGLIST ls = pIndex->pListLocation;
		while (ls != NULL)
		{
			int nLoc = 0;
			if (m_bAlter)
			{
				nLoc = m_cListLocation.AddString (ls->lpszString);
				if (nLoc != CB_ERR)
					m_cListLocation.SetCheck (nLoc, TRUE);
			}
			else
			{
				nLoc = m_cListLocation.FindStringExact (-1, ls->lpszString);
				if (nLoc != LB_ERR)
					m_cListLocation.SetCheck (nLoc, TRUE);
			}
			ls = ls->next;
		}
		if (pIndex->bDefineProperty)
		{
			ShowControls (SW_SHOW);
			m_cCheckProperty.SetCheck (1);
		}
	}
}

void CxDlgIndexOption::EnableOKButton()
{
	BOOL bEnable = FALSE;
	
	CString strIndexName;
	int nSel = m_cComboIndex.GetCurSel();
	if (nSel != CB_ERR && !m_bAlter)
	{
		int nData = m_cComboIndex.GetItemData (nSel);
		switch (nData)
		{
		case INDEXOPTION_USEDEFAULT:
		case INDEXOPTION_NOINDEX:
		case INDEXOPTION_BASETABLE_STRUCTURE:
		case INDEXOPTION_USER_DEFINED_PROPERTY:
			bEnable = TRUE;
			break;
		case INDEXOPTION_USEEXISTING_INDEX:
			m_cComboIndexNameDDList.GetWindowText (strIndexName);
			bEnable = strIndexName.IsEmpty()? FALSE: TRUE;
			break;
		case INDEXOPTION_USER_DEFINED:
			m_cComboIndexName.GetWindowText (strIndexName);
			strIndexName.TrimLeft();
			strIndexName.TrimRight();
			bEnable = strIndexName.IsEmpty()? FALSE: TRUE;
			break;
		}
	}
	m_cButtonOK.EnableWindow (bEnable);
}

void CxDlgIndexOption::OnSelchangeComboIndexNameDDList() 
{
	EnableOKButton();
}

void CxDlgIndexOption::SharedIndexChange (int nMember, LPCTSTR lpszValue)
{
	//
	// Problem in design:
	// Do not  use this function:
	return;

	CString strIndexName;
	CString strItem;
	m_cComboIndexName.GetWindowText (strIndexName);
	strIndexName.TrimLeft();
	strIndexName.TrimRight();
	int i, nCount, nIdx = m_cComboIndexName.FindStringExact (-1, strIndexName);
	if (nIdx == CB_ERR)
		return;
	INDEXOPTION* pIdx = (INDEXOPTION*)m_cComboIndexName.GetItemData (nIdx);
	if (!pIdx)
		return;

	switch (nMember)
	{
	case INDEX_MINPAGE:
		lstrcpy (pIdx->tchszMinPage, lpszValue);
		break;
	case INDEX_MAXPAGE:
		lstrcpy (pIdx->tchszMaxPage, lpszValue);
		break;
	case INDEX_ALLOCATION:
		lstrcpy (pIdx->tchszAllocation, lpszValue);
		break;
	case INDEX_EXTEND:
		lstrcpy (pIdx->tchszExtend, lpszValue);
		break;
	case INDEX_FILLFACTOR:
		lstrcpy (pIdx->tchszFillfactor, lpszValue);
		break;
	case INDEX_LEAFFILL:
		lstrcpy (pIdx->tchszLeaffill, lpszValue);
		break;
	case INDEX_NONLEAFFILL:
		lstrcpy (pIdx->tchszNonleaffill, lpszValue);
		break;
	case INDEX_STRUCTURE:
		lstrcpy (pIdx->tchszStructure,  m_strStructure);
		lstrcpy (pIdx->tchszMinPage, m_strMinPage);
		lstrcpy (pIdx->tchszMaxPage, m_strMaxPage);
		lstrcpy (pIdx->tchszAllocation, m_strAllocation);
		lstrcpy (pIdx->tchszExtend, m_strExtend);
		lstrcpy (pIdx->tchszFillfactor, m_strFillfactor);
		lstrcpy (pIdx->tchszLeaffill, m_strLeaffill);
		lstrcpy (pIdx->tchszNonleaffill, m_strNonleaffill);
		break;
	case INDEX_LOCATION:
		pIdx->pListLocation = StringList_Done (pIdx->pListLocation);
		nCount = m_cListLocation.GetCount();
		for (i=0; i<nCount; i++)
		{
			if (m_cListLocation.GetCheck (i))
			{
				m_cListLocation.GetText (i, strItem);
				pIdx->pListLocation = StringList_Add (pIdx->pListLocation, strItem);
			}
		}
		break;
	default:
		ASSERT (FALSE);
		break;
	}
}


void CxDlgIndexOption::OnCheckChangeLocation()
{
	SharedIndexChange (INDEX_LOCATION, _T(""));
}