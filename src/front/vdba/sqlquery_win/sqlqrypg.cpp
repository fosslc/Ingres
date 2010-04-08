/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlqrypg.cpp : Implementation of the CSqlqueryPropPage property page class..
**    Project  : sqlquery.ocx 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Declaration of the CSqlqueryPropPage property page class
**
** History:
**
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 23-Jan-2004 (schph01)
**    (sir 104378) detect version 3 of Ingres, for managing
**    new features provided in this version. replaced references
**    to 2.65 with refereces to 3  in #define definitions for
**    better readability in the future
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
*/


#include "stdafx.h"
#include "sqlquery.h"
#include "rcdepend.h"
#include "sqlqrypg.h"
#include "qredoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define SELECT_MIN    200
#define SELECT_MAX 100000


IMPLEMENT_DYNCREATE(CSqlqueryPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CSqlqueryPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CSqlqueryPropPage)
	ON_EN_KILLFOCUS(IDC_EDIT_TIMEOUT, OnKillfocusEditTimeout)
	ON_EN_KILLFOCUS(IDC_EDIT_MAXTUPLE, OnKillfocusEditMaxtuple)
	ON_BN_CLICKED(IDC_RADIO1, OnSelectMode)
	ON_BN_CLICKED(IDC_RADIO2, OnSelectMode)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid
#if defined (EDBC)
// {1793CCBE-9858-4803-B01C-7B089B872BE3}
IMPLEMENT_OLECREATE_EX(CSqlqueryPropPage, "EDBQUERY.CSqlqueryPropPage",
	0x1793ccbe, 0x9858, 0x4803, 0xb0, 0x1c, 0x7b, 0x08, 0x9b, 0x87, 0x2b, 0xe3)
#else
// {634C383E-A069-11D5-8769-00C04F1F754A}
IMPLEMENT_OLECREATE_EX(CSqlqueryPropPage, "SQLQUERY.CSqlqueryPropPage",
	0x634c383e, 0xa069, 0x11d5, 0x87, 0x69, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a)
#endif

/////////////////////////////////////////////////////////////////////////////
// CSqlqueryPropPage::CSqlqueryPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CSqlqueryPropPage

BOOL CSqlqueryPropPage::CSqlqueryPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_xSQLQUERY_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CSqlqueryPropPage::CSqlqueryPropPage - Constructor

CSqlqueryPropPage::CSqlqueryPropPage() :
	COlePropertyPage(IDD, IDS_SQLQUERY_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CSqlqueryPropPage)
	m_bAutoCommit = FALSE;
	m_bReadLock = FALSE;
	m_nTimeOut = 0;
	m_nSelectLimit = 0;
	//}}AFX_DATA_INIT
	m_nSelectMode = 0;
	SetHelpInfo(_T(""), theApp.m_strHelpFile, 0);
}


/////////////////////////////////////////////////////////////////////////////
// CSqlqueryPropPage::DoDataExchange - Moves data between page and properties

void CSqlqueryPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CSqlqueryPropPage)
	DDX_Control(pDX, IDC_CHECK_AUTOCOMMIT, m_cAutocommit);
	DDX_Control(pDX, IDC_EDIT_TIMEOUT, m_cEditTimeOut);
	DDX_Control(pDX, IDC_SPIN2, m_cSpinSelectLimit);
	DDX_Control(pDX, IDC_SPIN1, m_cSpinTimeOut);
	DDX_Control(pDX, IDC_EDIT_MAXTUPLE, m_cEditSelectLimit);
	DDP_Check(pDX, IDC_CHECK_AUTOCOMMIT, m_bAutoCommit, _T("AutoCommit") );
	DDX_Check(pDX, IDC_CHECK_AUTOCOMMIT, m_bAutoCommit);
	DDP_Check(pDX, IDC_CHECK_READLOCK, m_bReadLock, _T("ReadLock") );
	DDX_Check(pDX, IDC_CHECK_READLOCK, m_bReadLock);
	DDP_Text(pDX, IDC_EDIT_TIMEOUT, m_nTimeOut, _T("TimeOut") );
	DDX_Text(pDX, IDC_EDIT_TIMEOUT, m_nTimeOut);
	DDP_Text(pDX, IDC_EDIT_MAXTUPLE, m_nSelectLimit, _T("SelectLimit") );
	DDX_Text(pDX, IDC_EDIT_MAXTUPLE, m_nSelectLimit);
	//}}AFX_DATA_MAP
	DDX_Radio (pDX, IDC_RADIO1, m_nSelectMode);
	DDP_Radio (pDX, IDC_RADIO1, m_nSelectMode, _T("SelectMode") );

	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CSqlqueryPropPage message handlers
void CSqlqueryPropPage::OnSelectMode()
{
	int nRado = GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO2);
	if (nRado == IDC_RADIO1)
	{
		m_cEditSelectLimit.EnableWindow(FALSE);
		m_cSpinSelectLimit.EnableWindow(FALSE);
	}
	else
	{
		m_cEditSelectLimit.EnableWindow(TRUE);
		m_cSpinSelectLimit.EnableWindow(TRUE);
	}
}


BOOL CSqlqueryPropPage::OnInitDialog() 
{
	COlePropertyPage::OnInitDialog();

	m_cSpinTimeOut.SetRange(0, UD_MAXVAL);
	m_cSpinSelectLimit.SetRange32(SELECT_MIN, SELECT_MAX);
	OnSelectMode();
	EnableAutocommit();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CSqlqueryPropPage::OnKillfocusEditTimeout() 
{
	CString strText;
	m_cEditTimeOut.GetWindowText(strText);
	long lVal = _ttol (strText);
	if (lVal > UD_MAXVAL)
		lVal = UD_MAXVAL;
	strText.Format (_T("%d"), lVal);
	m_cEditTimeOut.SetWindowText(strText);
}

void CSqlqueryPropPage::OnKillfocusEditMaxtuple() 
{
	CString strText;
	m_cEditSelectLimit.GetWindowText(strText);
	long lVal = _ttol (strText);
	if (lVal > SELECT_MAX)
		lVal = SELECT_MAX;
	else
	if (lVal < SELECT_MIN)
		lVal = SELECT_MIN;

	strText.Format (_T("%d"), lVal);
	m_cEditSelectLimit.SetWindowText(strText);
}

void CSqlqueryPropPage::EnableAutocommit() 
{
	BOOL bLt2_65 = FALSE;
	BOOL bHasTransaction = FALSE;
	CTypedPtrList < CObList, CaDocTracker* >& ldoc = theApp.GetDocTracker();
	POSITION pos = ldoc.GetHeadPosition();
	while (pos != NULL)
	{
		CaDocTracker* pObj = ldoc.GetNext(pos);
		CdSqlQueryRichEditDoc* pDoc = pObj->GetDoc();
		if (pDoc)
		{
			CaSession* pCurrentSession = pDoc->GetCurrentSession();
			if (pCurrentSession)
			{
				int nVersion = pCurrentSession->GetVersion();
				if (nVersion < INGRESVERS_30)
				{
					bLt2_65 = TRUE;
					break;
				}

				bHasTransaction = !pCurrentSession->IsCommitted();
				if (bHasTransaction)
					break;
			}
		}
	}

	if (bHasTransaction || bLt2_65)
	{
		m_cAutocommit.EnableWindow (FALSE);
	}
}

BOOL CSqlqueryPropPage::OnHelp(LPCTSTR lpszHelpDir)
{
	UINT nHelpID = 5507; // OLD vdba: SQLACTPREFDIALOG = 5507
	return APP_HelpInfo(m_hWnd, nHelpID);
}
