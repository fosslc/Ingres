/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : eappagc2.cpp : Implementation file
**    Project  : EXPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : STEP 2 of Export assistant (DBF, XML, FIXED WIDTH, COPY STATEMENT)
**
** History:
**
** 16-Jan-2002 (uk$so01)
**    Created
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 22-Aug-2003 (uk$so01)
**    SIR #106648, Add silent mode in IEA. 
** 08-May-2008 (drivi01)
**    Apply wizard97 style to this wizard.
**/

#include "stdafx.h"
#include "svriia.h"
#include "eappagc2.h"
#include "constdef.h"
#include "rcdepend.h"
#include "eapsheet.h"
#include "rctools.h"  // Resource symbols of rctools.rc

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuIeaPPage2Common, CPropertyPage)
CuIeaPPage2Common::CuIeaPPage2Common() : CPropertyPage(CuIeaPPage2Common::IDD), CuIeaPPage2CommonData()
{
	//{{AFX_DATA_INIT(CuIeaPPage2Common)
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

CuIeaPPage2Common::~CuIeaPPage2Common()
{
}


void CuIeaPPage2Common::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuIeaPPage2Common)
	DDX_Control(pDX, IDC_CHECK1, m_cCheckExportHeader);
	DDX_Control(pDX, IDC_CHECK4, m_cCheckReadLock);
	DDX_Control(pDX, IDC_STATIC_TEXT1, m_cExportInfo);
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonSave);
	DDX_Control(pDX, IDC_STATIC1, m_cStatic1);
	DDX_Control(pDX, IDC_STATIC_CHECKMARK, m_cStaticCheckMark);
	DDX_Control(pDX, IDC_STATIC_IMAGE1, m_bitmap);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuIeaPPage2Common, CPropertyPage)
	//{{AFX_MSG_MAP(CuIeaPPage2Common)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonSaveParameters)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuIeaPPage2Common message handlers
BOOL CuIeaPPage2Common::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	CuIEAPropertySheet* pParent = (CuIEAPropertySheet*)GetParent();
	CaIEAInfo& data = pParent->GetData();
	CaIeaDataPage1& dataPage1 = data.m_dataPage1;
	m_hIcon = theApp.LoadIcon (IDI_ICON_SAVE);
	
	try
	{
		CRect r;
		m_cStatic1.GetWindowRect (r);
		ScreenToClient (r);
		
		CreateDoubleListCtrl(this, r);
		m_cButtonSave.SetIcon(m_hIcon);
	}
	catch (...)
	{
		TRACE0("Raise exception at CuIeaPPage2Common::OnInitDialog()\n");
		return FALSE;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuIeaPPage2Common::OnDestroy() 
{
	if (m_hIcon)
		DestroyIcon(m_hIcon);
	m_hIcon = NULL;
	CPropertyPage::OnDestroy();
}

BOOL CuIeaPPage2Common::OnSetActive() 
{
	CuIEAPropertySheet* pParent = (CuIEAPropertySheet*)GetParent();
	CaIEAInfo& data = pParent->GetData();
	CaIeaDataPage1& dataPage1 = data.m_dataPage1;
	CaIeaDataPage2& dataPage2 = data.m_dataPage2;
	pParent->SetWizardButtons(PSWIZB_FINISH|PSWIZB_BACK);
	m_cCheckExportHeader.ShowWindow(SW_HIDE);

	UINT nIDsInfo = 0;
	switch (dataPage1.GetExportedFileType())
	{
	case EXPORT_DBF:
		nIDsInfo = IDS_EXPORT_2_DBF;
		break;
	case EXPORT_XML:
		nIDsInfo = IDS_EXPORT_2_XML;
		break;
	case EXPORT_FIXED_WIDTH:
		nIDsInfo = IDS_EXPORT_2_FIXED;
		m_cCheckExportHeader.ShowWindow(SW_SHOW);
		m_cCheckExportHeader.SetCheck(dataPage2.IsExportHeader()? 1: 0);
		break;
	case EXPORT_COPY_STATEMENT:
		nIDsInfo = IDS_EXPORT_2_COPY;
		break;
	default:
		//
		// You must define new type.
		ASSERT(FALSE);
		break;
	}
	if (nIDsInfo > 0)
	{
		CString strText;
		strText.LoadString(nIDsInfo);
		m_cExportInfo.SetWindowText(strText);
	}

	m_cCheckReadLock.SetCheck(dataPage2.IsReadLock()? 1: 0);
	DisplayResults(this, dataPage1.GetExportedFileType());
	return CPropertyPage::OnSetActive();
}

BOOL CuIeaPPage2Common::OnKillActive() 
{
	CuIEAPropertySheet* pParent = (CuIEAPropertySheet*)GetParent();
	CaIEAInfo& data = pParent->GetData();

	pParent->SetPreviousPage (2);
	return CPropertyPage::OnKillActive();
}

BOOL CuIeaPPage2Common::OnWizardFinish() 
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

void CuIeaPPage2Common::OnButtonSaveParameters() 
{
	CuIEAPropertySheet* pParent = (CuIEAPropertySheet*)GetParent();
	CaIEAInfo& data = pParent->GetData();
	CollectData(&data);
	SaveExportParameter(this);
}

void CuIeaPPage2Common::CollectData(CaIEAInfo* pData)
{
	CuIEAPropertySheet* pParent = (CuIEAPropertySheet*)GetParent();
	CaIEAInfo& data = pParent->GetData();
	CaIeaDataPage2& dataPage2 = data.m_dataPage2;

	//
	// Read Lock:
	dataPage2.SetReadLock((m_cCheckReadLock.GetCheck () == 1)? TRUE: FALSE);
	//
	// Export Header:
	dataPage2.SetExportHeader((m_cCheckExportHeader.GetCheck() == 1)? TRUE: FALSE);
}
