/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : prefdlg.cpp, Implementation file                                      //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//               SCHALK P (Custom Implementation)                                      //
//                                                                                     //
//    Purpose  : Modeless Dialog Page (Preference)                                     //
****************************************************************************************/

#include "stdafx.h"
#include "vcbf.h"
#include "prefdlg.h"
#include "wmusrmsg.h"
#include "ll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPrefDlg dialog


CPrefDlg::CPrefDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPrefDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPrefDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CPrefDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPrefDlg)
	DDX_Control(pDX, IDC_CHECK1, m_CButtonPreference);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPrefDlg, CDialog)
	//{{AFX_MSG_MAP(CPrefDlg)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_CHECK1, OnCheckPreference)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrefDlg message handlers

void CPrefDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	// TODO: Add your message handler code here
	
}

BOOL CPrefDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPrefDlg::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

LONG CPrefDlg::OnUpdateData (WPARAM wParam, LPARAM lParam)
{	
	BOOL bSyscheck = FALSE;
	CWaitCursor CWaitCur;
	try
	{
		if (VCBFll_settings_Init( &bSyscheck ))
			m_CButtonPreference.SetCheck( bSyscheck ); 
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CPrefDlg::OnUpdateData has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
	return 0L;
}

void CPrefDlg::OnCheckPreference() 
{
	BOOL bCheckedResult;
	// TODO: Add your control notification handler code here
	try
	{
		if ( VCBFll_On_settings_CheckChanged(m_CButtonPreference.GetCheck(),
											&bCheckedResult))
			m_CButtonPreference.SetCheck( bCheckedResult ); 
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CPrefDlg::OnCheckPreference has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
}

void CPrefDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);
	try
	{
		if (!bShow)
			VCBFll_settings_Terminate();
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
}
