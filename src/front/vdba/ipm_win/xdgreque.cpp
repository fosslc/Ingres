/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dlgreque.cpp : implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : schph01
**    Purpose  : Send DBevents to Replication server. This Box allows to specify
**               Server(s) where the dbevents are to send to.
**
** History:
**
** xx-xxx-1997 (schph01)
**    Created
**
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "xdgreque.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CxDlgRaiseEventQuestion::CxDlgRaiseEventQuestion(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgRaiseEventQuestion::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgRaiseEventQuestion)
	m_NewValue = 1;
	m_strQuestion = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgRaiseEventQuestion::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CxDlgRaiseEventQuestion)
	DDX_Text(pDX, IDC_REPLICQUESTION, m_NewValue);
	DDX_Text(pDX, IDC_QUESTION, m_strQuestion);
	//}}AFX_DATA_MAP
	DDV_MinMaxInt(pDX, m_NewValue, m_iMin, m_iMax);
}


BEGIN_MESSAGE_MAP(CxDlgRaiseEventQuestion, CDialog)
	//{{AFX_MSG_MAP(CxDlgRaiseEventQuestion)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgRaiseEventQuestion message handlers

BOOL CxDlgRaiseEventQuestion::OnInitDialog() 
{
	CDialog::OnInitDialog();
	SetWindowText( m_strCaption );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgRaiseEventQuestion::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}
