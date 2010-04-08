/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
** 
**    Source   : LayEdDlg.cpp, Implementation File
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**    Purpose  : Edit Box to be used with Editable Owner Draw ListCtrl
**
** History:
**
** xx-Sep-1997: (uk$so01) 
**    created
** 21-Sept-1999 (uk$so01)
**    Add function ResizeControls(), used by OnSize() and OnShowWindow()
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
**
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "layeddlg.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuLayoutEditDlg dialog


CuLayoutEditDlg::CuLayoutEditDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CuLayoutEditDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuLayoutEditDlg)
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

CuLayoutEditDlg::CuLayoutEditDlg(CWnd* pParent, CString strText, int iItem, int iSubItem)
	:CDialog(CuLayoutEditDlg::IDD, pParent)
{
	m_strItemText = strText;
	m_iItem   = iItem;
	m_iSubItem= iSubItem;
	m_bUseExpressDialog = TRUE;
}

void CuLayoutEditDlg::SetData (LPCTSTR lpszText, int iItem, int iSubItem)
{
	m_strItemText = lpszText;
	m_iItem   = iItem;
	m_iSubItem= iSubItem;
	CEdit* edit = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
	if (edit && IsWindow (edit->m_hWnd))
		edit->SetWindowText (m_strItemText);
}

void CuLayoutEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuLayoutEditDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuLayoutEditDlg, CDialog)
	//{{AFX_MSG_MAP(CuLayoutEditDlg)
	ON_BN_CLICKED(IDC_BUTTON_ASSIST, OnButtonAssistant)
	ON_WM_SIZE()
	ON_WM_KILLFOCUS()
	ON_WM_CLOSE()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuLayoutEditDlg message handlers

void CuLayoutEditDlg::OnButtonAssistant() 
{
	CEdit*  edit1 = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
	CString s;
	edit1->GetWindowText (s);
	GetParent()->SendMessage (WM_LAYOUTEDITDLG_EXPR_ASSISTANT, 0, (WPARAM)(LPCTSTR)s);
}

BOOL CuLayoutEditDlg::OnInitDialog() 
{
	if (m_bUseSpecialParse)
	{
		ASSERT (m_wParseStyle != 0);
		if (m_wParseStyle == 0)
			return FALSE;
		m_cEdit1.SubclassEdit(IDC_RCT_EDIT1, this, m_wParseStyle);
	}
	else
	{
		CDialog::OnInitDialog();
	}
	CEdit* edit = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
	CButton* button= (CButton*)GetDlgItem (IDC_BUTTON_ASSIST);
	if (edit && IsWindow (edit->m_hWnd))
		edit->SetWindowText (m_strItemText);
	if (!m_bUseExpressDialog)
	{
		button->ShowWindow (SW_HIDE);
	}
	else
	{
		button->ShowWindow (SW_SHOW);
	}
	edit->SetReadOnly (m_bReadOnly);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

CString CuLayoutEditDlg::GetText ()
{
	CString s;
	CEdit* edit = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
	if (edit && IsWindow (edit->m_hWnd))
		edit->GetWindowText (s);
	return (s);
}


void CuLayoutEditDlg::GetText (CString& strText)
{
	CEdit* edit = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
	edit->GetWindowText (strText);
}

void CuLayoutEditDlg::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuLayoutEditDlg::OnCancel ()
{
	ShowWindow (SW_HIDE);
	CWnd* pParent = GetParent ();
	ASSERT_VALID (pParent);
	pParent->SendMessage (WM_LAYOUTEDITDLG_CANCEL);
}

void CuLayoutEditDlg::OnOK()
{
	ShowWindow (SW_HIDE);
	CWnd* pParent = GetParent ();
	ASSERT_VALID (pParent);
	CEdit*  edit1 = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
	edit1->GetWindowText (m_strItemText);
	pParent->SendMessage (WM_LAYOUTEDITDLG_OK);
}

void CuLayoutEditDlg::ResizeControls()
{
	CEdit*   edit  = (CEdit*)  GetDlgItem (IDC_RCT_EDIT1);
	CButton* button= (CButton*)GetDlgItem (IDC_BUTTON_ASSIST);
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
	{
		button->MoveWindow (R);
		button->ShowWindow (SW_SHOW);
	}
}


void CuLayoutEditDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	ResizeControls();
}

void CuLayoutEditDlg::OnKillFocus(CWnd* pNewWnd) 
{
	CDialog::OnKillFocus(pNewWnd);
}

void CuLayoutEditDlg::OnClose() 
{
	ShowWindow (SW_HIDE);
}


void CuLayoutEditDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);
	if (bShow)
	{
		CEdit*  edit1 = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
		if (edit1 && IsWindow (edit1->m_hWnd))
		{
			ResizeControls();
			edit1->SetFocus();
			edit1->SetSel(0, -1);
		}
	}
}

void CuLayoutEditDlg::SetFocus()
{
	CEdit*  edit1 = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
	if (edit1 && IsWindow (edit1->m_hWnd))
	{
		edit1->SetFocus();
		edit1->SetSel(0, -1);
	}
}

void CuLayoutEditDlg::SetReadOnly (BOOL bReadOnly)
{
	m_bReadOnly = bReadOnly;
	CEdit*  edit1 = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
	if (edit1 && IsWindow (edit1->m_hWnd))
	{
		edit1->SetReadOnly(m_bReadOnly);
	}
}
