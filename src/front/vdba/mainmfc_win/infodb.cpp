/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : infodb.cpp, implementation file
**
**
**    Project  : Ingres II / Visual DBA.
**    Author   : Schalk Philippe
**
**    Purpose  : Manage dialog box for launch infodb command.
**
**
**  21-Nov-2001 (schph01)
**    bug 102285 this dialog manage the command line for infodb.
**               - infodb #cN DBNAME information on a specific checkpoint
**               - infodb #c DBNAME  information on the last checkpoint
**               - infodb DBNAME  information on all checkpoints
** 20-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
**  26-Nov-2003 (schph01)
**    bug 102285 Remove the member variable m_uiCheckPointNumber associates
**    with the IDC_EDIT_NUMBER because this control is now with the style
**    number in ressource files.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "infodb.h"
#include "ckplst.h"

extern "C"
{
#include "domdata.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CxDlgInfoDB dialog


CxDlgInfoDB::CxDlgInfoDB(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgInfoDB::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgInfoDB)
	//}}AFX_DATA_INIT
}


void CxDlgInfoDB::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgInfoDB)
	DDX_Control(pDX, IDC_BUTTON_CHECKPOINT, m_ctrlBtCheckList);
	DDX_Control(pDX, IDC_RADIO_LAST_CKP, m_ctrlBtLastCkp);
	DDX_Control(pDX, IDOK, m_ctrlBtOK);
	DDX_Control(pDX, IDC_EDIT_NUMBER, m_ctrlCheckNumber);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgInfoDB, CDialog)
	//{{AFX_MSG_MAP(CxDlgInfoDB)
	ON_BN_CLICKED(IDC_BUTTON_CHECKPOINT, OnButtonCheckpoint)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_RADIO_LAST_CKP, OnRadioLastCkp)
	ON_BN_CLICKED(IDC_CHECK_SPECIFY, OnCheckSpecify)
	ON_BN_CLICKED(IDC_RADIO_SPECIFY_CKP, OnRadioSpecifyCkp)
	ON_EN_CHANGE(IDC_EDIT_NUMBER, OnChangeEditNumber)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgInfoDB message handlers

BOOL CxDlgInfoDB::OnInitDialog() 
{
	CString csTitle,csNewTitle;
	CDialog::OnInitDialog();

	GetWindowText(csTitle);
	csNewTitle.Format(csTitle, m_csVnodeName,m_csDBname);
	SetWindowText(csNewTitle);
	
	CheckRadioButton(IDC_RADIO_LAST_CKP, IDC_RADIO_SPECIFY_CKP,IDC_RADIO_LAST_CKP);

	PushHelpId(IDD_INFODB);

	m_ctrlCheckNumber.SetLimitText(10);
	m_ctrlCheckNumber.SetWindowText(_T("0"));

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgInfoDB::OnOK() 
{
	CString strMsg;

	int hnode = OpenNodeStruct((LPUCHAR)(LPCTSTR)m_csVnodeName);
	if (hnode < 0)
	{
		strMsg = VDBA_MfcResourceString (IDS_MAX_NB_CONNECT);//_T("Maximum number of connections has been reached"
		strMsg += VDBA_MfcResourceString (IDS_E_RUN_INFODB);         // " - Cannot run Infodb.");
		AfxMessageBox (strMsg);
		return;
	}

	// activates a session - said temporary
	UCHAR buf[MAXOBJECTNAME];
	DOMGetFirstObject (hnode, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);

	// execute the remote command - no need to take an exclusive lock, so last parameter set to FALSE
	CString csTitle = _T("Infodb");
	CString csCommandLine;

	if (IsDlgButtonChecked(IDC_CHECK_SPECIFY))
	{
		int nID = GetCheckedRadioButton(IDC_RADIO_LAST_CKP, IDC_RADIO_SPECIFY_CKP);
		if (nID == IDC_RADIO_SPECIFY_CKP)
		{
			CString csTemp;
			m_ctrlCheckNumber.GetWindowText(csTemp);

			csCommandLine.Format(_T("infodb #c%s %s -u%s"),
			                     csTemp,
			                     m_csDBname,
			                     m_csOwnerName);
		}
		else
		{
			csCommandLine.Format(_T("infodb #c %s -u%s"),
			                     m_csDBname,
			                     m_csOwnerName);
		}
	}
	else
	{
		csCommandLine.Format(_T("infodb %s -u%s"),
		                     m_csDBname,
		                     m_csOwnerName);
	}

	execrmcmd1( (char *)(LPCTSTR)m_csVnodeName,
	            (char *)(LPCTSTR)csCommandLine,
	            (char *)(LPCTSTR)csTitle,
	            FALSE);

	CloseNodeStruct(hnode,FALSE);

	CDialog::OnOK();
}

void CxDlgInfoDB::OnButtonCheckpoint() 
{
	CString csTemp;

	CxDlgCheckPointLst Dlg;
	Dlg.m_csCurDBName    = m_csDBname;
	Dlg.m_csCurDBOwner   = m_csOwnerName;
	Dlg.m_csCurVnodeName = m_csVnodeName;
	m_ctrlCheckNumber.GetWindowText(csTemp);
	Dlg.SetSelectedCheckPoint(csTemp);
	Dlg.DoModal();
	csTemp = Dlg.GetSelectedCheckPoint();
	m_ctrlCheckNumber.SetWindowText(csTemp);

}

void CxDlgInfoDB::OnDestroy() 
{
	CDialog::OnDestroy();

	PopHelpId();
}

void CxDlgInfoDB::OnRadioLastCkp() 
{
	m_ctrlCheckNumber.EnableWindow(FALSE);
	m_ctrlBtCheckList.EnableWindow(FALSE);
	m_ctrlBtOK.EnableWindow(TRUE);

}

void CxDlgInfoDB::OnRadioSpecifyCkp() 
{
	m_ctrlCheckNumber.EnableWindow(TRUE);
	m_ctrlBtCheckList.EnableWindow(TRUE);
	if (m_ctrlCheckNumber.GetWindowTextLength() > 0 )
		m_ctrlBtOK.EnableWindow(TRUE);
	else
		m_ctrlBtOK.EnableWindow(FALSE);
}

void CxDlgInfoDB::OnCheckSpecify() 
{
	if (IsDlgButtonChecked(IDC_CHECK_SPECIFY))
	{
		m_ctrlCheckNumber.EnableWindow(FALSE);
		m_ctrlBtCheckList.EnableWindow(FALSE);
		m_ctrlBtLastCkp.EnableWindow(TRUE);
		GetDlgItem(IDC_RADIO_SPECIFY_CKP)->EnableWindow(TRUE);
	}
	else
	{
		m_ctrlCheckNumber.EnableWindow(FALSE);
		m_ctrlBtCheckList.EnableWindow(FALSE);
		m_ctrlBtLastCkp.EnableWindow(FALSE);
		GetDlgItem(IDC_RADIO_SPECIFY_CKP)->EnableWindow(FALSE);
		m_ctrlBtOK.EnableWindow(TRUE);
	}
}


void CxDlgInfoDB::OnChangeEditNumber() 
{
	if (m_ctrlCheckNumber.GetWindowTextLength() > 0 )
		m_ctrlBtOK.EnableWindow(TRUE);
	else
		m_ctrlBtOK.EnableWindow(FALSE);
}
