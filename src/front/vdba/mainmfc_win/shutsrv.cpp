// ShutSrv.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "shutsrv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CShutdownServer dialog


CShutdownServer::CShutdownServer(CWnd* pParent /*=NULL*/)
	: CDialog(CShutdownServer::IDD, pParent)
{
	//{{AFX_DATA_INIT(CShutdownServer)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CShutdownServer::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CShutdownServer)
	DDX_Control(pDX, IDC_RADIO_SOFT, m_softRadio);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CShutdownServer, CDialog)
	//{{AFX_MSG_MAP(CShutdownServer)
	ON_BN_CLICKED(IDC_RADIO_HARD, OnRadioHard)
	ON_BN_CLICKED(IDC_RADIO_SOFT, OnRadioSoft)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CShutdownServer message handlers

BOOL CShutdownServer::OnInitDialog() 
{
	CDialog::OnInitDialog();

  m_softRadio.SetFocus();
  m_softRadio.SetCheck(1);
  m_bSoft = TRUE;

  SetWindowText(m_caption);

  return FALSE;         // FALSE since we have set the focus to a control
}

void CShutdownServer::OnRadioHard() 
{
  m_bSoft = FALSE;
}

void CShutdownServer::OnRadioSoft() 
{
  m_bSoft = TRUE;
}
