/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ipmppg.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Implementation of the CIpmPropPage property page class.
**
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 07-Apr-2004 (uk$so01)
**    BUG #112109 / ISSUE 13342826, The min of frequency is 10 when unit is Second(s)
**    The min of frequency should be 1 if unit is other than Second(s).
** 19-Nov-2004 (uk$so01)
**    BUG #113500 / ISSUE #13755297 (Vdba / monitor, references such as background 
**    refresh, or the number of sessions, are not taken into account in vdbamon nor 
**    in the VDBA Monitor functionality)
*/


#include "stdafx.h"
#include "ipm.h"
#include "ipmppg.h"
#include "rcdepend.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define FREQ_MIN 10
#define FREQ_MAX 216000
#define SESION_MIN 2
#define SESION_MAX 20


IMPLEMENT_DYNCREATE(CIpmPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CIpmPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CIpmPropPage)
	ON_EN_KILLFOCUS(IDC_EDIT_TIMEOUT, OnKillfocusEditTimeout)
	ON_EN_KILLFOCUS(IDC_EDIT_FREQUENCY, OnKillfocusEditFrequency)
	ON_EN_KILLFOCUS(IDC_EDIT_MAXSESSION, OnKillfocusEditMaxsession)
	ON_BN_CLICKED(IDC_CHECK_ACTIVATEREFRESH, OnCheckActivaterefresh)
	//}}AFX_MSG_MAP
	ON_CBN_SELCHANGE(IDC_COMBO1, OnCbnSelchangeCombo1)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CIpmPropPage, "IPM.IpmPropPage.1",
	0xab474687, 0xe577, 0x11d5, 0x87, 0x8c, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a)


/////////////////////////////////////////////////////////////////////////////
// CIpmPropPage::CIpmPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CIpmPropPage

BOOL CIpmPropPage::CIpmPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_IPM_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CIpmPropPage::CIpmPropPage - Constructor

CIpmPropPage::CIpmPropPage() :
	COlePropertyPage(IDD, IDS_IPM_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CIpmPropPage)
	m_nFrequency = 60;
	m_nTimeOut = 0;
	m_bActivateRefresh = FALSE;
	m_bShowGrid = FALSE;
	m_nUnit = 0;
	m_nMaxSession = 0;
	//}}AFX_DATA_INIT
	SetHelpInfo(_T(""), theApp.m_strHelpFile, 0);
}


/////////////////////////////////////////////////////////////////////////////
// CIpmPropPage::DoDataExchange - Moves data between page and properties

void CIpmPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CIpmPropPage)
	DDX_Control(pDX, IDC_COMBO1, m_cComboUnit);
	DDX_Control(pDX, IDC_SPIN3, m_cSpinMaxSession);
	DDX_Control(pDX, IDC_CHECK_ACTIVATEREFRESH, m_cCheckActivateBackroundRefresh);
	DDX_Control(pDX, IDC_EDIT_MAXSESSION, m_cEditMaxSession);
	DDX_Control(pDX, IDC_SPIN2, m_cSpinFrequency);
	DDX_Control(pDX, IDC_SPIN1, m_cSpinTimeOut);
	DDX_Control(pDX, IDC_EDIT_TIMEOUT, m_cEditTimeOut);
	DDX_Control(pDX, IDC_EDIT_FREQUENCY, m_cEditFrequency);
	DDP_Text(pDX, IDC_EDIT_FREQUENCY, m_nFrequency, _T("RefreshFrequency") );
	DDX_Text(pDX, IDC_EDIT_FREQUENCY, m_nFrequency);
	DDP_Text(pDX, IDC_EDIT_TIMEOUT, m_nTimeOut, _T("TimeOut") );
	DDX_Text(pDX, IDC_EDIT_TIMEOUT, m_nTimeOut);
	DDP_Check(pDX, IDC_CHECK_ACTIVATEREFRESH, m_bActivateRefresh, _T("ActivateRefresh") );
	DDX_Check(pDX, IDC_CHECK_ACTIVATEREFRESH, m_bActivateRefresh);
	DDP_Check(pDX, IDC_CHECK_SHOWGRID, m_bShowGrid, _T("ShowGrid") );
	DDX_Check(pDX, IDC_CHECK_SHOWGRID, m_bShowGrid);
	DDP_CBIndex(pDX, IDC_COMBO1, m_nUnit, _T("Unit") );
	DDX_CBIndex(pDX, IDC_COMBO1, m_nUnit);
	DDP_Text(pDX, IDC_EDIT_MAXSESSION, m_nMaxSession, _T("MaxSession") );
	DDX_Text(pDX, IDC_EDIT_MAXSESSION, m_nMaxSession);
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CIpmPropPage message handlers

void CIpmPropPage::OnKillfocusEditTimeout() 
{
	CString strText;
	m_cEditTimeOut.GetWindowText(strText);
	long lVal = _ttol (strText);
	if (lVal > UD_MAXVAL)
		lVal = UD_MAXVAL;
	strText.Format (_T("%d"), lVal);
	m_cEditTimeOut.SetWindowText(strText);
}

void CIpmPropPage::OnKillfocusEditFrequency() 
{
	CString strText;
	int nSel = m_cComboUnit.GetCurSel();
	int nMin = FREQ_MIN;
	m_cEditFrequency.GetWindowText(strText);
	if (nSel > 0)
		nMin = 1;
	long lVal = _ttol (strText);
	if (lVal > FREQ_MAX)
		lVal = FREQ_MAX;
	else
	if (lVal < nMin)
		lVal = nMin;

	strText.Format (_T("%d"), lVal);
	m_cEditFrequency.SetWindowText(strText);
}

BOOL CIpmPropPage::OnInitDialog() 
{
	COlePropertyPage::OnInitDialog();
	
	m_cSpinTimeOut.SetRange(0, UD_MAXVAL);
	m_cSpinFrequency.SetRange32(FREQ_MIN, FREQ_MAX);
	m_cSpinMaxSession.SetRange32(SESION_MIN, SESION_MAX);

	EnableBkRefreshParam();
	BOOL bEnable = LIBMON_CanUpdateMaxSessions();
	m_cEditMaxSession.EnableWindow(bEnable);
	m_cSpinMaxSession.EnableWindow(bEnable);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CIpmPropPage::OnKillfocusEditMaxsession() 
{
	CString strText;
	m_cEditMaxSession.GetWindowText(strText);
	long lVal = _ttol (strText);
	if (lVal > SESION_MAX)
		lVal = SESION_MAX;
	else
	if (lVal < SESION_MIN)
		lVal = SESION_MIN;

	strText.Format (_T("%d"), lVal);
	m_cEditMaxSession.SetWindowText(strText);
}

void CIpmPropPage::OnCheckActivaterefresh() 
{
	EnableBkRefreshParam();
}

void CIpmPropPage::EnableBkRefreshParam() 
{
	int nCheck = m_cCheckActivateBackroundRefresh.GetCheck();
	BOOL bEnable = (nCheck == 1);
	m_cSpinFrequency.EnableWindow(bEnable);
	m_cEditFrequency.EnableWindow(bEnable);
	m_cComboUnit.EnableWindow(bEnable);
}

BOOL CIpmPropPage::OnHelp(LPCTSTR lpszHelpDir)
{
	UINT nHelpID = 5536; // OLD monitor help. (only set font monitor)
	return APP_HelpInfo(m_hWnd, nHelpID);
}

void CIpmPropPage::OnCbnSelchangeCombo1()
{
	OnKillfocusEditFrequency();
}
