// replikey.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "replikey.h"
extern "C" {
#include "dba.h"
}
#include "replutil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
extern "C" int MfcReplicationSelectKeyOption(char *pTableName)
{
	CuDlgReplicationSelectKeyOption dlg;
	dlg.m_strTableName = pTableName;
	dlg.DoModal();
	return dlg.m_nKeyType;
}

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicationSelectKeyOption dialog


CuDlgReplicationSelectKeyOption::CuDlgReplicationSelectKeyOption(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgReplicationSelectKeyOption::IDD, pParent)
{
	m_nKeyType = NO_KEYSCHOOSE;
	//{{AFX_DATA_INIT(CuDlgReplicationSelectKeyOption)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgReplicationSelectKeyOption::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgReplicationSelectKeyOption)
	DDX_Control(pDX, IDOK, m_ctrlOK);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgReplicationSelectKeyOption, CDialog)
	//{{AFX_MSG_MAP(CuDlgReplicationSelectKeyOption)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicationSelectKeyOption message handlers

void CuDlgReplicationSelectKeyOption::OnOK() 
{

	if (((CButton*)GetDlgItem(IDC_RADIOSHADOWTBL))->GetCheck() == 1)
		m_nKeyType = KEYS_SHADOWTBLONLY;
	else if (((CButton*)GetDlgItem(IDC_RADIOBOTHQUEUE))->GetCheck() == 1)
		m_nKeyType = KEYS_BOTHQUEUEANDSHADOW;
	else 
		m_nKeyType = NO_KEYSCHOOSE;

	CDialog::OnOK();
}

BOOL CuDlgReplicationSelectKeyOption::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CString csTitle,csNewTitle;

	GetWindowText(csTitle);
	csNewTitle.Format(csTitle,m_strTableName);
	SetWindowText(csNewTitle);
	CheckRadioButton (IDC_RADIOSHADOWTBL, IDC_RADIOBOTHQUEUE, IDC_RADIOSHADOWTBL);
	PushHelpId (IDD_REPLICATION_KEY_OPTION);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgReplicationSelectKeyOption::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}
