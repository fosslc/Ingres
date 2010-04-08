/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlpgtab.cpp : implementation file
**    Project  : SqlTest ActiveX.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Property page for Tab Layout
**
** History:
**
** 06-Mar-2002 (uk$so01)
**    Created
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 02-Feb-2004 (uk$so01)
**    SIR #106648, Vdba-Split, ocx property page should return FALSE
**    otherwise the apply button is enabled
** 14-Apr-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
**    Preference Options with different help context ID.
*/


#include "stdafx.h"
#include "sqlquery.h"
#include "rcdepend.h"
#include "qredoc.h"
#include "sqlpgtab.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define TRACE_MIN  1
#define TRACE_MAX  UD_MAXVAL
#define TAB_MIN     2
#define TAB_MAX    16


IMPLEMENT_DYNCREATE(CSqlqueryPropPageTabLayout, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CSqlqueryPropPageTabLayout, COlePropertyPage)
	//{{AFX_MSG_MAP(CSqlqueryPropPageTabLayout)
	ON_BN_CLICKED(IDC_CHECK_ACTIVATETRACE, OnCheckActivatetrace)
	ON_EN_KILLFOCUS(IDC_EDIT_MAXTAB, OnKillfocusEditMaxtab)
	ON_EN_KILLFOCUS(IDC_EDIT_MAXTRACESIZE, OnKillfocusEditMaxtracesize)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid
#if defined (EDBC)
// {68803776-99A8-4c71-9AFE-74FA935A670F}
IMPLEMENT_OLECREATE_EX(CSqlqueryPropPageTabLayout, "EDBQUERY.CSqlqueryPropPageTabLayout",
	0x68803776, 0x99A8, 0x4c71, 0x9A, 0xFE, 0x74, 0xfa, 0x93, 0x5a, 0x67, 0x07)
#else
// {FC40B058-2CEE-11D6-87A5-00C04F1F754A}
IMPLEMENT_OLECREATE_EX(CSqlqueryPropPageTabLayout, "SQLQUERY.CSqlqueryPropPageTabLayout",
	0xfc40b058, 0x2cee, 0x11d6, 0x87, 0xa5, 0x0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a)
#endif

/////////////////////////////////////////////////////////////////////////////
// CSqlqueryPropPageTabLayout::CSqlqueryPropPageTabLayoutFactory::UpdateRegistry -
// Adds or removes system registry entries for CSqlqueryPropPageTabLayout

BOOL CSqlqueryPropPageTabLayout::CSqlqueryPropPageTabLayoutFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_xSQLQUERY_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CSqlqueryPropPageTabLayout::CSqlqueryPropPageTabLayout - Constructor

// TODO: Define string resource for page caption; replace '0' below with ID.

CSqlqueryPropPageTabLayout::CSqlqueryPropPageTabLayout() :
	COlePropertyPage(IDD, IDS_SQLQUERY_PPGTABLAYOUT_CAPTION)
{
	//{{AFX_DATA_INIT(CSqlqueryPropPageTabLayout)
	m_bTraceTabActivated = FALSE;
	m_bShowNonSelectTab = FALSE;
	m_bTraceTabToTop = FALSE;
	m_nMaxTab = 0;
	m_nMaxTraceSize = 0;
	//}}AFX_DATA_INIT
	m_bTraceTabToTop = DEFAULT_TRACETOTOP;
	m_bTraceTabActivated = DEFAULT_TRACEACTIVATED;
	m_bShowNonSelectTab = DEFAULT_SHOWNONSELECTTAB;
	m_nMaxTab = DEFAULT_MAXTAB;
	m_nMaxTraceSize = DEFAULT_MAXTRACE;
	SetHelpInfo(_T(""), theApp.m_strHelpFile, 0);
}


/////////////////////////////////////////////////////////////////////////////
// CSqlqueryPropPageTabLayout::DoDataExchange - Moves data between page and properties

void CSqlqueryPropPageTabLayout::DoDataExchange(CDataExchange* pDX)
{
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//{{AFX_DATA_MAP(CSqlqueryPropPageTabLayout)
	DDX_Control(pDX, IDC_CHECK_SHOWDONSELECT, m_cCheckNonSelect);
	DDX_Control(pDX, IDC_EDIT_MAXTRACESIZE, m_cEditMaxTraceSize);
	DDX_Control(pDX, IDC_EDIT_MAXTAB, m_cEditMaxTab);
	DDX_Control(pDX, IDC_SPIN2, m_cSpinMaxTraceSize);
	DDX_Control(pDX, IDC_SPIN1, m_cSpinMaxTab);
	DDX_Control(pDX, IDC_CHECK_TRACETOP, m_cCheckTraceTabToTop);
	DDX_Control(pDX, IDC_CHECK_ACTIVATETRACE, m_cCheckActivateTrace);
	DDP_Check(pDX, IDC_CHECK_ACTIVATETRACE, m_bTraceTabActivated, _T("TraceTabActivated") );
	DDX_Check(pDX, IDC_CHECK_ACTIVATETRACE, m_bTraceTabActivated);
	DDP_Check(pDX, IDC_CHECK_SHOWDONSELECT, m_bShowNonSelectTab, _T("ShowNonSelectTab") );
	DDX_Check(pDX, IDC_CHECK_SHOWDONSELECT, m_bShowNonSelectTab);
	DDP_Check(pDX, IDC_CHECK_TRACETOP, m_bTraceTabToTop, _T("TraceTabToTop") );
	DDX_Check(pDX, IDC_CHECK_TRACETOP, m_bTraceTabToTop);
	DDP_Text(pDX, IDC_EDIT_MAXTAB, m_nMaxTab, _T("MaxTab") );
	DDX_Text(pDX, IDC_EDIT_MAXTAB, m_nMaxTab);
	DDP_Text(pDX, IDC_EDIT_MAXTRACESIZE, m_nMaxTraceSize, _T("MaxTraceSize") );
	DDX_Text(pDX, IDC_EDIT_MAXTRACESIZE, m_nMaxTraceSize);
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CSqlqueryPropPageTabLayout message handlers

BOOL CSqlqueryPropPageTabLayout::OnInitDialog() 
{
	COlePropertyPage::OnInitDialog();
	m_cSpinMaxTraceSize.SetRange32(TRACE_MIN, TRACE_MAX);
	m_cSpinMaxTab.SetRange32 (TAB_MIN, TAB_MAX);
	OnCheckActivatetrace();

	BOOL bEnable = TRUE;
	CTypedPtrList < CObList, CaDocTracker* >& ldoc = theApp.GetDocTracker();
	POSITION pos = ldoc.GetHeadPosition();
	while (pos != NULL)
	{
		CaDocTracker* pObj = ldoc.GetNext(pos);
		CdSqlQueryRichEditDoc* pDoc = pObj->GetDoc();
		if (pDoc)
		{
			int nCheck = m_cCheckNonSelect.GetCheck();
			if (nCheck == 0 && pDoc->IsStatementExecuted())
			{
				bEnable = FALSE; 
				break;
			}
		}
	}
	m_cCheckNonSelect.EnableWindow(bEnable);
	SetModifiedFlag(FALSE);
	return FALSE; // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSqlqueryPropPageTabLayout::OnCheckActivatetrace() 
{
}

void CSqlqueryPropPageTabLayout::OnKillfocusEditMaxtab() 
{
	CString strText;
	m_cEditMaxTab.GetWindowText(strText);
	long lVal = _ttol (strText);
	if (lVal > TAB_MAX)
		lVal = TAB_MAX;
	else
	if (lVal < TAB_MIN)
		lVal = TAB_MIN;
	strText.Format (_T("%d"), lVal);
	m_cEditMaxTab.SetWindowText(strText);
}

void CSqlqueryPropPageTabLayout::OnKillfocusEditMaxtracesize() 
{
	CString strText;
	m_cEditMaxTraceSize.GetWindowText(strText);
	long lVal = _ttol (strText);
	if (lVal > TRACE_MAX)
		lVal = TRACE_MAX;
	else
	if (lVal < TRACE_MIN)
		lVal = TRACE_MIN;
	strText.Format (_T("%d"), lVal);
	m_cEditMaxTraceSize.SetWindowText(strText);
}

BOOL CSqlqueryPropPageTabLayout::OnHelp(LPCTSTR lpszHelpDir)
{
	UINT nHelpID = 5508; // OLD vdba: SQLACTPREFDIALOG = 5507
	return APP_HelpInfo(m_hWnd, nHelpID);
}
