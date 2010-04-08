/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : eappage2.cpp : implementation file
**    Project  : EXPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : STEP 2 of Export assistant
**
** History:
**
** 16-Oct-2001 (uk$so01)
**    Created
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 22-Aug-2003 (uk$so01)
**    SIR #106648, Add silent mode in IEA. 
** 08-May-2008 (drivi01)
**    Apply wizard97 style to this wizard.
**/


#include "stdafx.h"
#include "svriia.h"
#include "eappage2.h"

#include "constdef.h"
#include "rcdepend.h"
#include "eapsheet.h"
#include "rctools.h"  // Resource symbols of rctools.rc

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif





IMPLEMENT_DYNCREATE(CuIeaPPage2, CPropertyPage)
CuIeaPPage2::CuIeaPPage2() : CPropertyPage(CuIeaPPage2::IDD), CuIeaPPage2CommonData()
{
	//{{AFX_DATA_INIT(CuIeaPPage2)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bitmap.SetBitmpapId (IDB_IMPORTAS_1);
	m_bitmap.SetCenterVertical(TRUE);

	CString strStep;
	strStep.LoadString (IDS_IEA_STEP_2);
	m_strPageTitle.LoadString(IDS_TITLE_EXPORT);
	m_strPageTitle += theApp.m_strInstallationID;
	m_strPageTitle += strStep;
	m_psp.dwFlags |= PSP_USETITLE;
	m_psp.pszTitle = (LPCTSTR)m_strPageTitle;

	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

CuIeaPPage2::~CuIeaPPage2()
{
}

void CuIeaPPage2::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuIeaPPage2)
	DDX_Control(pDX, IDC_CHECK4, m_cCheckReadLock);
	DDX_Control(pDX, IDC_CHECK1, m_cCheckExportHeader);
	DDX_Control(pDX, IDC_CHECK3, m_cCheckQuoteIfNeeded);
	DDX_Control(pDX, IDC_COMBO3, m_cComboQuote);
	DDX_Control(pDX, IDC_COMBO2, m_cComboDelimiter);
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonSave);
	DDX_Control(pDX, IDC_STATIC1, m_cStatic1);
	DDX_Control(pDX, IDC_STATIC_CHECKMARK, m_cStaticCheckMark);
	DDX_Control(pDX, IDC_STATIC_IMAGE1, m_bitmap);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuIeaPPage2, CPropertyPage)
	//{{AFX_MSG_MAP(CuIeaPPage2)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonSaveParameters)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuIeaPPage2 message handlers

BOOL CuIeaPPage2::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	CuIEAPropertySheet* pParent = (CuIEAPropertySheet*)GetParent();
	CaIEAInfo& data = pParent->GetData();
	CaIeaDataPage1& dataPage1 = data.m_dataPage1;
	CaIeaDataPage2& dataPage2 = data.m_dataPage2;
	m_hIcon = theApp.LoadIcon (IDI_ICON_SAVE);

	try
	{
		CRect r;


		m_cStatic1.GetWindowRect (r);
		ScreenToClient (r);
		CreateDoubleListCtrl(this, r);

		m_cButtonSave.SetIcon(m_hIcon);
		m_cCheckQuoteIfNeeded.SetCheck (dataPage2.IsQuoteIfDelimiter() ? 1: 0);

		int i, idx;
		CString strItem;
		UINT  nDelimiterID[2] = {IDS_DELIMITER_COMMA, IDS_DELIMITER_SEMICOLON};
		TCHAR nDelimiter[2] = {_T(','), _T(';')};
		for (i=0; i<2; i++)
		{
			strItem.LoadString(nDelimiterID[i]);
			idx = m_cComboDelimiter.AddString(strItem);
			if (idx != CB_ERR)
				m_cComboDelimiter.SetItemData(idx, (DWORD)nDelimiter[i]);
		}

		UINT  nQuoteID[2] = {IDS_QUOTE_DOUBLE, IDS_QUOTE_SINGLE};
		TCHAR nQuote[2] = {_T('\"'), _T('\'')};
		for (i=0; i<2; i++)
		{
			strItem.LoadString(nQuoteID[i]);
			idx = m_cComboQuote.AddString(strItem);
			if (idx != CB_ERR)
				m_cComboQuote.SetItemData(idx, (DWORD)nQuote[i]);
		}
	}
	catch (...)
	{
		TRACE0("Raise exception at CuIeaPPage2::OnInitDialog()\n");
		return FALSE;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuIeaPPage2::OnDestroy() 
{
	if (m_hIcon)
		DestroyIcon(m_hIcon);
	m_hIcon = NULL;
	CPropertyPage::OnDestroy();
}



BOOL CuIeaPPage2::OnSetActive() 
{
	CuIEAPropertySheet* pParent = (CuIEAPropertySheet*)GetParent();
	CaIEAInfo& data = pParent->GetData();
	CaIeaDataPage1& dataPage1 = data.m_dataPage1;
	CaIeaDataPage2& dataPage2 = data.m_dataPage2;
	pParent->SetWizardButtons(PSWIZB_FINISH|PSWIZB_BACK);
	ExportedFileType expft = dataPage1.GetExportedFileType();
	ASSERT(expft == EXPORT_CSV);

	m_cCheckReadLock.SetCheck (dataPage2.IsReadLock() ? 1: 0);
	m_cCheckExportHeader.SetCheck (dataPage2.IsExportHeader() ? 1: 0);
	m_cCheckQuoteIfNeeded.SetCheck (dataPage2.IsQuoteIfDelimiter() ? 1: 0);
	int i;
	for (i=0; i<2; i++)
	{
		if (dataPage2.GetDelimiter() == (TCHAR)m_cComboDelimiter.GetItemData(i))
		{
			m_cComboDelimiter.SetCurSel(i);
			break;
		}
	}

	for (i=0; i<2; i++)
	{
		if (dataPage2.GetQuote() == (TCHAR)m_cComboQuote.GetItemData(i))
		{
			m_cComboQuote.SetCurSel(i);
			break;
		}
	}

	DisplayResults(this, expft);

	return CPropertyPage::OnSetActive();
}

BOOL CuIeaPPage2::OnKillActive() 
{
	CuIEAPropertySheet* pParent = (CuIEAPropertySheet*)GetParent();
	CaIEAInfo& data = pParent->GetData();

	pParent->SetPreviousPage (2);
	return CPropertyPage::OnKillActive();
}

BOOL CuIeaPPage2::OnWizardFinish() 
{
	CString strFile;
	CString strMsg;
	CuIEAPropertySheet* pParent = (CuIEAPropertySheet*)GetParent();
	CaIEAInfo& data = pParent->GetData();
	CollectData(&data);
	BOOL bOK = Findish(data);
	if (!bOK)
		return FALSE;
	return CPropertyPage::OnWizardFinish();
}

void CuIeaPPage2::OnButtonSaveParameters() 
{
	CuIEAPropertySheet* pParent = (CuIEAPropertySheet*)GetParent();
	CaIEAInfo& data = pParent->GetData();
	CollectData(&data);
	SaveExportParameter(this);
}

void CuIeaPPage2::CollectData(CaIEAInfo* pData)
{
	CuIEAPropertySheet* pParent = (CuIEAPropertySheet*)GetParent();
	CaIEAInfo& data = pParent->GetData();
	CaIeaDataPage2& dataPage2 = data.m_dataPage2;

	//
	// Export Header:
	dataPage2.SetExportHeader((m_cCheckExportHeader.GetCheck() == 1)? TRUE: FALSE);
	//
	// Read Lock:
	dataPage2.SetReadLock((m_cCheckReadLock.GetCheck () == 1)? TRUE: FALSE);
	//
	// Quote only if String contains the delimiters:
	dataPage2.SetQuoteIfDelimiter((m_cCheckQuoteIfNeeded.GetCheck () == 1)? TRUE: FALSE);
	//
	// Delimiter:
	int nSel = m_cComboDelimiter.GetCurSel();
	if (nSel != CB_ERR)
	{
		TCHAR tchszDelimiter = (TCHAR)m_cComboDelimiter.GetItemData(nSel);
		dataPage2.SetDelimiter(tchszDelimiter);
	}
	//
	// Quote:
	nSel = m_cComboQuote.GetCurSel();
	if (nSel != CB_ERR)
	{
		TCHAR tchszQuote = (TCHAR)m_cComboQuote.GetItemData(nSel);
		dataPage2.SetQuote(tchszQuote);
	}
}
