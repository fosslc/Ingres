/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : dlgepref.cpp , Implementation File 
**    Project  : Ingres II / Visual Manager 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : The Preferences setting of Visual Manager
**
** History:
**
** 05-Feb-1999 (uk$so01)
**    Created
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
**
**/

#include "stdafx.h"
#include "resource.h"
#include "ivm.h"
#include "dlgepref.h"
#include "xdlgpref.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgPropertyPageEventNotify, CPropertyPage)




/////////////////////////////////////////////////////////////////////////////
// CuDlgPropertyPageEventNotify property page
CuDlgPropertyPageEventNotify::CuDlgPropertyPageEventNotify() : CPropertyPage(CuDlgPropertyPageEventNotify::IDD)
{
	m_nEventMaxType = IDC_RADIO2;
	m_bInitDialog = FALSE;
	//{{AFX_DATA_INIT(CuDlgPropertyPageEventNotify)
	m_bSendingMail = FALSE;
	m_bMessageBox = FALSE;
	m_bSoundBeep = TRUE;
	m_strMaxMemory = _T("");
	m_strMaxEvent = _T("");
	m_strSinceDays = _T("");
	//}}AFX_DATA_INIT
	m_bSendingMail = theApp.m_setting.m_bSendingMail;
	m_bMessageBox = theApp.m_setting.m_bMessageBox;
	m_bSoundBeep = theApp.m_setting.m_bSoundBeep;
	m_strMaxMemory.Format (_T("%d"), theApp.m_setting.m_nMaxMemUnit);
	m_strMaxEvent.Format (_T("%d"), theApp.m_setting.m_nMaxEvent);
}

CuDlgPropertyPageEventNotify::~CuDlgPropertyPageEventNotify()
{
}

void CuDlgPropertyPageEventNotify::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPropertyPageEventNotify)
	DDX_Check(pDX, IDC_CHECK1, m_bSendingMail);
	DDX_Check(pDX, IDC_CHECK2, m_bMessageBox);
	DDX_Check(pDX, IDC_CHECK8, m_bSoundBeep);
	DDX_Control(pDX, IDC_EDIT1, m_cMaxMemory);
	DDX_Control(pDX, IDC_EDIT2, m_cMaxEvent);
	DDX_Control(pDX, IDC_EDIT3, m_cSinceDays);
	DDX_Text(pDX, IDC_EDIT1, m_strMaxMemory);
	DDV_MaxChars(pDX, m_strMaxMemory, 8);
	DDX_Text(pDX, IDC_EDIT2, m_strMaxEvent);
	DDV_MaxChars(pDX, m_strMaxEvent, 8);
	DDX_Text(pDX, IDC_EDIT3, m_strSinceDays);
	DDV_MaxChars(pDX, m_strSinceDays, 4);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPropertyPageEventNotify, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgPropertyPageEventNotify)
	ON_BN_CLICKED(IDC_RADIO1, OnRadioMaxMemory)
	ON_BN_CLICKED(IDC_RADIO2, OnRadioMaxEvent)
	ON_BN_CLICKED(IDC_RADIO3, OnRadioSinceDays)
	ON_BN_CLICKED(IDC_RADIO4, OnRadioSinceLastNameStarted)
	ON_BN_CLICKED(IDC_RADIO5, OnRadioNoLimit)
	ON_EN_KILLFOCUS(IDC_EDIT1, OnKillfocusEditMaxMemory)
	ON_EN_KILLFOCUS(IDC_EDIT2, OnKillfocusEditMaxEvent)
	ON_EN_KILLFOCUS(IDC_EDIT3, OnKillfocusEditSinceDays)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()





BOOL CuDlgPropertyPageEventNotify::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	m_bInitDialog = TRUE;
	CheckRadioButton (IDC_RADIO1, IDC_RADIO5, m_nEventMaxType);
	EnableDisableControl();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CuDlgPropertyPageEventNotify::EnableDisableControl()
{
	int nID = GetCheckedRadioButton (IDC_RADIO1, IDC_RADIO5);
	if (nID == IDC_RADIO1)
	{
		m_cMaxMemory.EnableWindow (TRUE);
		m_cMaxEvent.EnableWindow (FALSE);
		m_cSinceDays.EnableWindow (FALSE);
	}
	else
	if (nID == IDC_RADIO2)
	{
		m_cMaxMemory.EnableWindow (FALSE);
		m_cMaxEvent.EnableWindow (TRUE);
		m_cSinceDays.EnableWindow (FALSE);
	}
	else
	if (nID == IDC_RADIO3)
	{
		m_cMaxMemory.EnableWindow (FALSE);
		m_cMaxEvent.EnableWindow (FALSE);
		m_cSinceDays.EnableWindow (TRUE);
	}
	else
	{
		m_cMaxMemory.EnableWindow (FALSE);
		m_cMaxEvent.EnableWindow (FALSE);
		m_cSinceDays.EnableWindow (FALSE);
	}
}



void CuDlgPropertyPageEventNotify::OnOK() 
{
	CPropertyPage::OnOK();
	m_nEventMaxType = GetCheckedRadioButton (IDC_RADIO1, IDC_RADIO5);
	UpdateSetting(TRUE);
}



BOOL CuDlgPropertyPageEventNotify::OnSetActive() 
{
	CxDlgPropertySheetPreference* pParent = (CxDlgPropertySheetPreference*)GetParent();
	return CPropertyPage::OnSetActive();
}

void CuDlgPropertyPageEventNotify::OnRadioMaxMemory() 
{
	EnableDisableControl();
}

void CuDlgPropertyPageEventNotify::OnRadioMaxEvent() 
{
	EnableDisableControl();
}

void CuDlgPropertyPageEventNotify::OnRadioSinceDays()
{
	EnableDisableControl();
}

void CuDlgPropertyPageEventNotify::OnRadioSinceLastNameStarted()
{
	EnableDisableControl();
}

void CuDlgPropertyPageEventNotify::OnRadioNoLimit()
{
	EnableDisableControl();
}



void CuDlgPropertyPageEventNotify::UpdateSetting(BOOL bOut)
{
	if (bOut)
	{
		if (!m_bInitDialog)
			return;
		CxDlgPropertySheetPreference* pParent = (CxDlgPropertySheetPreference*)GetParent();

		theApp.m_setting.SetEventMaxType(m_nEventMaxType);
		theApp.m_setting.m_nMaxEvent   = _ttol (m_strMaxEvent);
		theApp.m_setting.m_nMaxMemUnit = _ttol (m_strMaxMemory);
		theApp.m_setting.m_nMaxDay     = _ttol(m_strSinceDays);
		theApp.m_setting.m_bSendingMail= m_bSendingMail;
		theApp.m_setting.m_bMessageBox = m_bMessageBox;
		theApp.m_setting.m_bSoundBeep  = m_bSoundBeep;
	}
	else
	{
		switch (theApp.m_setting.GetEventMaxType())
		{
		case EVMAX_MEMORY:
			m_nEventMaxType = IDC_RADIO1;
			break;
		case EVMAX_COUNT:
			m_nEventMaxType = IDC_RADIO2;
			break;
		case EVMAX_SINCEDAY:
			m_nEventMaxType = IDC_RADIO3;
			break;
		case EVMAX_SINCENAME:
			m_nEventMaxType = IDC_RADIO4;
			break;
		case EVMAX_NOLIMIT:
			m_nEventMaxType = IDC_RADIO5;
			break;
		default:
			m_nEventMaxType = IDC_RADIO5;
			break;
		}

		m_bSendingMail = theApp.m_setting.m_bSendingMail;
		m_bMessageBox  = theApp.m_setting.m_bMessageBox;
		m_bSoundBeep   = theApp.m_setting.m_bSoundBeep;
		m_strMaxEvent.Format  (_T("%d"), theApp.m_setting.m_nMaxEvent);
		m_strMaxMemory.Format (_T("%d"), theApp.m_setting.m_nMaxMemUnit);
		m_strSinceDays.Format (_T("%d"), theApp.m_setting.m_nMaxDay);
	}
}


void CuDlgPropertyPageEventNotify::OnKillfocusEditMaxMemory() 
{
	CString strText = _T("");
	m_cMaxMemory.GetWindowText (strText);
	if (_ttol (strText) > 0)
		return;
	// _T("Invalid Value for the \"Max Used Memory\" parameter");
	CString strMsg;
	strMsg.LoadString(IDS_MSG_ERROR_SETTING_MAXMEMORY);
	AfxMessageBox(strMsg);
	UpdateData (FALSE);
}

void CuDlgPropertyPageEventNotify::OnKillfocusEditMaxEvent() 
{
	CString strText = _T("");
	m_cMaxEvent.GetWindowText (strText);
	long lMin = 100;
	long lMax = _ttol (strText);
	if (lMax >= lMin)
		return;
	// _T("The minimum value for the \"Max Number of Events\" is 100");
	CString strMsg;
	strMsg.LoadString(IDS_MSG_ERROR_SETTING_MAXEVENT);
	AfxMessageBox(strMsg);
	UpdateData (FALSE);
}

void CuDlgPropertyPageEventNotify::OnKillfocusEditSinceDays() 
{
	CString strText = _T("");
	m_cSinceDays.GetWindowText (strText);
	if (_ttol (strText) > 0)
		return;
	//_T("Invalid Value for the \"Since ... days\" parameter");
	CString strMsg;
	strMsg.LoadString(IDS_MSG_ERROR_SETTING_SINCEDAY);
	AfxMessageBox(strMsg);
	UpdateData (FALSE);
}
