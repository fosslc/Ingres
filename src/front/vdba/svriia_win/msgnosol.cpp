/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : msgnosol.cpp : implementation file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Modal dialog box that acts like MessageBox 
**
** History:
**
** 14-Mar-2001 (uk$so01)
**    Created
**/


#include "stdafx.h"
#include "svriia.h"
#include "msgnosol.h"
#include "xdlgdete.h" // Modal dialog detection info (failure reason)

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CxDlgMessageNoSolution::CxDlgMessageNoSolution(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgMessageNoSolution::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgMessageNoSolution)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_hIcon = NULL;
	m_pData = NULL;
}


void CxDlgMessageNoSolution::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgMessageNoSolution)
	DDX_Control(pDX, IDC_STATIC_IMAGE1, m_cStaticImage1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgMessageNoSolution, CDialog)
	//{{AFX_MSG_MAP(CxDlgMessageNoSolution)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonSeeRejectedCombinaition)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CxDlgMessageNoSolution::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_hIcon = theApp.LoadStandardIcon(IDI_QUESTION);
	m_cStaticImage1.SetIcon(m_hIcon);
	m_cStaticImage1.Invalidate();
	DestroyIcon(m_hIcon);
	m_hIcon = NULL;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgMessageNoSolution::OnDestroy() 
{
	if (m_hIcon)
		DestroyIcon(m_hIcon);
	CDialog::OnDestroy();
}

void CxDlgMessageNoSolution::OnButtonSeeRejectedCombinaition() 
{
	ASSERT(m_pData);
	if (!m_pData)
		return;
	CxDlgDetectionInfo dlg;
	dlg.SetData(m_pData);
	dlg.DoModal();
}
