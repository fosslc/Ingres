/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : xdlgdrop.cpp, Implementation file                                     //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II/VDBA                                                        //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Dialog Box to allow to drop object with cascade | restrict            //
****************************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "xdlgdrop.h"
#include "extccall.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CxDlgDropObjectCascadeRestrict dialog


CxDlgDropObjectCascadeRestrict::CxDlgDropObjectCascadeRestrict(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgDropObjectCascadeRestrict::IDD, pParent)
{
	m_pParam = NULL;
	m_strTitle = _T("Drop Object");
	//{{AFX_DATA_INIT(CxDlgDropObjectCascadeRestrict)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CxDlgDropObjectCascadeRestrict::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgDropObjectCascadeRestrict)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgDropObjectCascadeRestrict, CDialog)
	//{{AFX_MSG_MAP(CxDlgDropObjectCascadeRestrict)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgDropObjectCascadeRestrict message handlers

void CxDlgDropObjectCascadeRestrict::OnOK() 
{
	int nRadio = GetCheckedRadioButton (IDC_MFC_RADIO1, IDC_MFC_RADIO2);
	DROPOBJECTSTRUCT* pDrop = (DROPOBJECTSTRUCT*)m_pParam;
	ASSERT (pDrop);
	if (!pDrop)
		return;

	switch (nRadio)
	{
	case IDC_MFC_RADIO1:
		pDrop->bRestrict = TRUE;
		break;
	case IDC_MFC_RADIO2:
		pDrop->bRestrict = FALSE;
		break;
	default:
		pDrop->bRestrict = TRUE;
		break;
	}
	CDialog::OnOK();
}

BOOL CxDlgDropObjectCascadeRestrict::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	ASSERT (m_pParam);
	if (!m_pParam)
		return FALSE;
	SetWindowText (m_strTitle);
	CheckRadioButton (IDC_MFC_RADIO1, IDC_MFC_RADIO2, IDC_MFC_RADIO1);
	PushHelpId (IDD);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgDropObjectCascadeRestrict::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}
