/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : dlgrmans.cpp, Implementation File                                     //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II/Vdba.                                                       //
//    Author   : UK Sotheavut                       .                                  //
//                                                                                     //
//    Purpose  : Popup Dialog Box for the RMCMD Send Answer                            //
****************************************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "dlgrmans.h"
extern "C"
{
#include "resource.h"
}
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CxDlgRemoteCommandInterface::CxDlgRemoteCommandInterface(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgRemoteCommandInterface::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgRemoteCommandInterface)
	m_strEdit1 = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgRemoteCommandInterface::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgRemoteCommandInterface)
	DDX_Control(pDX, IDOK, m_cButtonOK);
	DDX_Control(pDX, IDC_MFC_EDIT1, m_cEdit1);
	DDX_Text(pDX, IDC_MFC_EDIT1, m_strEdit1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgRemoteCommandInterface, CDialog)
	//{{AFX_MSG_MAP(CxDlgRemoteCommandInterface)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgRemoteCommandInterface message handlers

BOOL CxDlgRemoteCommandInterface::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	PushHelpId (REMOTEPILOTDIALOG);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgRemoteCommandInterface::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}
