/*
** Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : dlgmsbox.cpp, Implementation File
**    Project  : Ingres II / Visual VDBA
**    Author   : Sotheavut UK (uk$so01)
**
**    Purpose  : Dialog box that acts like a message box.
**
** History:
**    26-Jan-2000 (uk$so01)
**       (SIR #100175) Created.
**    15-Dec-2003 (schph01)
**       SIR #111462, remove the text initializing the m_strStaticText variable,
**       the CxDlgMessageBox class not used at the present time in ijactrl
**       project.
**
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "dlgmsbox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CxDlgMessageBox::CxDlgMessageBox(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgMessageBox::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgMessageBox)
	m_strStaticText = _T("");
	m_bNotShow = TRUE;
	//}}AFX_DATA_INIT
}


void CxDlgMessageBox::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgMessageBox)
	DDX_Control(pDX, IDC_STATIC_IMAGE, m_cStaticImage);
	DDX_Text(pDX, IDC_STATIC_01, m_strStaticText);
	DDX_Check(pDX, IDC_MFC_CHECK1, m_bNotShow);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgMessageBox, CDialog)
	//{{AFX_MSG_MAP(CxDlgMessageBox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgMessageBox message handlers

BOOL CxDlgMessageBox::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	HICON hIcon = theApp.LoadStandardIcon(IDI_EXCLAMATION);
	m_cStaticImage.SetIcon(hIcon);
	m_cStaticImage.Invalidate();
	DestroyIcon(hIcon);
	SetWindowText (theApp.m_pszAppName);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


//
// RETURN IDOK or IDCANCEL
// bContinue will be set to TRUE or FALSE;
int MSGContinueBox (LPCTSTR lpszMessage, BOOL& bContinue)
{
	CxDlgMessageBox dlg;
	dlg.m_strStaticText = lpszMessage;
	int idret = dlg.DoModal();
	bContinue = dlg.m_bNotShow;
	return idret;
}