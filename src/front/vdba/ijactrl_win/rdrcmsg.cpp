/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : rdrcmsg.cpp : implementation file
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Implementation of a Big Text message
**
** History:
**
** 26-Jun-2000 (uk$so01)
**    created
**
**/

#include "stdafx.h"
#include "ijactrl.h"
#include "rdrcmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CxDlgMessage::CxDlgMessage(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgMessage::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgMessage)
	m_strText = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgMessage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgMessage)
	DDX_Text(pDX, IDC_EDIT1, m_strText);
	//}}AFX_DATA_MAP
	m_hIcon = AfxGetApp()->LoadStandardIcon(IDI_EXCLAMATION);
}


BEGIN_MESSAGE_MAP(CxDlgMessage, CDialog)
	//{{AFX_MSG_MAP(CxDlgMessage)
	ON_WM_DESTROY()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgMessage message handlers

void CxDlgMessage::OnDestroy() 
{
	if (m_hIcon)
		DestroyIcon (m_hIcon);
	CDialog::OnDestroy();
}

BOOL CxDlgMessage::OnInitDialog() 
{
	CDialog::OnInitDialog();
	SetIcon(m_hIcon, TRUE); // Set big icon
	SetIcon(m_hIcon, FALSE);// Set small icon
	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CxDlgMessage::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void CxDlgMessage::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}
