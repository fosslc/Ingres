/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
** 
**    Source   : layeddlg.cpp, Implementation File
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**    Purpose  : Edit Box to be used with Editable Owner Draw ListCtrl
**
** History:
**
** xx-Sep-1997: (uk$so01) 
**    created
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
**/

#include "stdafx.h"
#include "vcbf.h"
#include "layeddlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuLayoutEditDlg2 dialog


CuLayoutEditDlg2::CuLayoutEditDlg2(CWnd* pParent /*=NULL*/)
	: CDialog(CuLayoutEditDlg2::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuLayoutEditDlg2)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_strItemText = _T("");
	m_iItem   = -1;
	m_iSubItem= -1;
	m_bUseExpressDialog = TRUE;
	m_bReadOnly = FALSE;
	m_bUseSpecialParse = FALSE;
	m_wParseStyle  = 0;
}

CuLayoutEditDlg2::CuLayoutEditDlg2(CWnd* pParent, CString strText, int iItem, int iSubItem)
	:CDialog(CuLayoutEditDlg2::IDD, pParent)
{
	m_strItemText = strText;
	m_iItem   = iItem;
	m_iSubItem= iSubItem;
	m_bUseExpressDialog = TRUE;
}

void CuLayoutEditDlg2::SetData (LPCTSTR lpszText, int iItem, int iSubItem)
{
	m_strItemText = lpszText;
	m_iItem   = iItem;
	m_iSubItem= iSubItem;
	CEdit* edit = (CEdit*)GetDlgItem (IDC_EDIT1);
	if (edit && IsWindow (edit->m_hWnd))
		edit->SetWindowText (m_strItemText);
}

void CuLayoutEditDlg2::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuLayoutEditDlg2)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuLayoutEditDlg2, CDialog)
	//{{AFX_MSG_MAP(CuLayoutEditDlg2)
	ON_BN_CLICKED(IDC_BUTTON_ASSIST2, OnButtonAssistant)
	ON_WM_SIZE()
	ON_WM_KILLFOCUS()
	ON_WM_CLOSE()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuLayoutEditDlg2 message handlers

void CuLayoutEditDlg2::OnButtonAssistant() 
{
}

BOOL CuLayoutEditDlg2::OnInitDialog() 
{
	if (m_bUseSpecialParse)
	{
		ASSERT (m_wParseStyle != 0);
		if (m_wParseStyle == 0)
			return FALSE;
		m_cEdit1.SubclassEdit(IDC_EDIT1, this, m_wParseStyle);
	}
	else
	{
		CDialog::OnInitDialog();
	}
	CEdit* edit = (CEdit*)GetDlgItem (IDC_EDIT1);
	CButton* button= (CButton*)GetDlgItem (IDC_BUTTON_ASSIST2);
	if (edit && IsWindow (edit->m_hWnd))
		edit->SetWindowText (m_strItemText);
	if (!m_bUseExpressDialog)
	{
		button->ShowWindow (SW_HIDE);
	}
	edit->SetReadOnly (m_bReadOnly);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

CString CuLayoutEditDlg2::GetText ()
{
	CString s;
	CEdit* edit = (CEdit*)GetDlgItem (IDC_EDIT1);
	if (edit && IsWindow (edit->m_hWnd))
		edit->GetWindowText (s);
	return (s);
}


void CuLayoutEditDlg2::GetText (CString& strText)
{
	CEdit* edit = (CEdit*)GetDlgItem (IDC_EDIT1);
	edit->GetWindowText (strText);
}

void CuLayoutEditDlg2::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuLayoutEditDlg2::OnCancel ()
{
	ShowWindow (SW_HIDE);
	CWnd* pParent = GetParent ();
	ASSERT_VALID (pParent);
	pParent->SendMessage (WM_LAYOUTEDITDLG_CANCEL);
}

void CuLayoutEditDlg2::OnOK()
{
	ShowWindow (SW_HIDE);
	CWnd* pParent = GetParent ();
	ASSERT_VALID (pParent);
	CEdit*  edit1 = (CEdit*)GetDlgItem (IDC_EDIT1);
	edit1->GetWindowText (m_strItemText);
	pParent->SendMessage (WM_LAYOUTEDITDLG_OK);
}

void CuLayoutEditDlg2::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	CEdit*   edit  = (CEdit*)  GetDlgItem (IDC_EDIT1);
	CButton* button= (CButton*)GetDlgItem (IDC_BUTTON_ASSIST2);
	if (!(edit&&button))
		return;
	if (!IsWindow(edit->m_hWnd) || !IsWindow(button->m_hWnd))
		return;
	CRect R, rB (0,0,0,0), rE;
	GetClientRect (R);
	if (!m_bUseExpressDialog)
		button->ShowWindow (SW_HIDE);

	if (m_bUseExpressDialog)
	{
		button->GetWindowRect (rB);
		ScreenToClient (rB);
	}
	rE.CopyRect (R);
	rE.right -= rB.right - rB.left;
	edit->MoveWindow (rE);
	R.left = rE.right;
	R.top    +=1;
	R.bottom -=1;
	if (m_bUseExpressDialog)
		button->MoveWindow (R);	
}

void CuLayoutEditDlg2::OnKillFocus(CWnd* pNewWnd) 
{
	CDialog::OnKillFocus(pNewWnd);
}

void CuLayoutEditDlg2::OnClose() 
{
	ShowWindow (SW_HIDE);
}


void CuLayoutEditDlg2::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);
	if (bShow)
	{
		CEdit*  edit1 = (CEdit*)GetDlgItem (IDC_EDIT1);
		if (edit1 && IsWindow (edit1->m_hWnd))
		{
			edit1->SetFocus();
			edit1->SetSel(0, -1);
		}
	}
}

void CuLayoutEditDlg2::SetFocus()
{
	CEdit*  edit1 = (CEdit*)GetDlgItem (IDC_EDIT1);
	if (edit1 && IsWindow (edit1->m_hWnd))
	{
		edit1->SetFocus();
		edit1->SetSel(0, -1);
	}
}

void CuLayoutEditDlg2::SetReadOnly (BOOL bReadOnly)
{
	m_bReadOnly = bReadOnly;
	CEdit*  edit1 = (CEdit*)GetDlgItem (IDC_EDIT1);
	if (edit1 && IsWindow (edit1->m_hWnd))
	{
		edit1->SetReadOnly(m_bReadOnly);
	}
}
