
/*
**  Copyright (C) 2008 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : messagebox.cpp : implementation file
**    Project  : CA-OpenIngres/Replicator.
**    Author   : Viktoriya Driker
**    Purpose  : Message dialog to request information or notify of
**				 special action. One of the buttons requires elevation 
**				 icon painted.
**
** History:
**
** 20-Oct-2008 (drivi01)
**		Created.
*/


#include "stdafx.h"
#include "ipm.h"
#include "messagebox.h"
#include ".\messagebox.h"
#include "rcdepend.h"
#include <gvcl.h>


// CMessageBox dialog

IMPLEMENT_DYNAMIC(CMessageBox, CDialog)
CMessageBox::CMessageBox(CWnd* pParent /*=NULL*/, CString msg)
	: CDialog(CMessageBox::IDD, pParent)
{
	m_msg = msg;
}

CMessageBox::~CMessageBox()
{
}




void CMessageBox::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIpmMessageBox)
	DDX_Control(pDX, IDYES, m_yes);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMessageBox, CDialog)
	ON_BN_CLICKED(IDYES, OnBnClickedYes)
	ON_BN_CLICKED(IDNO, OnBnClickedNo)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL CMessageBox::OnInitDialog()
{


	CDialog::OnInitDialog();
	//Set Message in the dialog
	GetDlgItem(IDC_STATIC_MSG)->SetWindowText(m_msg);

	//Insert elevation shield on button "Yes"
	m_shield.LoadBitmap(IDB_SHIELD);
	CButton* pButton = (CButton* )GetDlgItem(IDYES);		
	pButton->SetBitmap(m_shield);

	//Insert build-int question mark icon in the dialog
	HICON ico = AfxGetApp()->LoadStandardIcon((LPCTSTR)IDI_QUESTION);
	CStatic *pic = (CStatic *)GetDlgItem(IDC_STATIC_PIC);
	pic->SetIcon(ico);

	return TRUE;
	
}

HBRUSH CMessageBox::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{

//lets return the color brush that we our background to be
return CreateSolidBrush(RGB(255,255,255));//red is a good color don't u think?
}


// CMessageBox message handlers

void CMessageBox::OnBnClickedYes()
{
	// TODO: Add your control notification handler code here
	EndDialog(IDYES);
}

void CMessageBox::OnBnClickedNo()
{
	// TODO: Add your control notification handler code here
	EndDialog(IDNO);
}
