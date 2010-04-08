/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : LaySpin.cpp, Implementation File 
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Edit Box with Spin to be used with Editable Owner Draw ListCtrl
**
** History:
**
** xx-Sep-1997 (uk$so01)
**    created
** 28-Feb-2000 (uk$so01)
**    Position the member variable 'm_bReadOnly' before using the control
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "layspin.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuLayoutSpinDlg dialog
void CuLayoutSpinDlg::Init()
{
	m_strValue= _T("");
	m_iItem   = 0;
	m_iSubItem= 0;
	m_Min = 0;
	m_Max = 100;
	m_strValue = _T("0");
	m_bUseSpin = TRUE;
	m_bReadOnly= FALSE;
	m_bUseSpecialParse = FALSE;
	m_wParseStyle  = 0;
}


CuLayoutSpinDlg::CuLayoutSpinDlg(CWnd* pParent, CString strText, int iItem, int iSubItem)
	:CDialog(CuLayoutSpinDlg::IDD, pParent)
{
	Init();
	m_strValue= strText;
	m_iItem   = iItem;
	m_iSubItem= iSubItem;
}



CuLayoutSpinDlg::CuLayoutSpinDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CuLayoutSpinDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuLayoutSpinDlg)
	m_strValue = _T("0");
	//}}AFX_DATA_INIT
	Init();
}


void CuLayoutSpinDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuLayoutSpinDlg)
	DDX_Text(pDX, IDC_RCT_EDIT1, m_strValue);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuLayoutSpinDlg, CDialog)
	//{{AFX_MSG_MAP(CuLayoutSpinDlg)
	ON_WM_CLOSE()
	ON_WM_KILLFOCUS()
	ON_WM_SIZE()
	ON_WM_SHOWWINDOW()
	ON_EN_CHANGE(IDC_RCT_EDIT1, OnChangeEdit1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CuLayoutSpinDlg::SetData (LPCTSTR lpszText, int iItem, int iSubItem)
{
	m_strValue= lpszText;
	m_iItem   = iItem;
	m_iSubItem= iSubItem;
	CEdit* edit = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
	if (edit && IsWindow (edit->m_hWnd))
		edit->SetWindowText (m_strValue);
}

void CuLayoutSpinDlg::SetRange (int Min, int Max)
{
	m_Min = Min;
	m_Max = Max;
	CSpinButtonCtrl* pSpin1 = (CSpinButtonCtrl*)GetDlgItem (IDC_RCT_SPIN1);
	pSpin1->SetRange (m_Min, m_Max);
}


CString CuLayoutSpinDlg::GetText ()
{
	CString s;
	CEdit* edit = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
	if (edit && IsWindow (edit->m_hWnd))
		edit->GetWindowText (s);
	return (s);
}


void CuLayoutSpinDlg::GetText (CString& strText)
{
	CEdit* edit = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
	edit->GetWindowText (strText);
}


void CuLayoutSpinDlg::OnCancel ()
{
	ShowWindow (SW_HIDE);
	CWnd* pParent = GetParent ();
	ASSERT_VALID (pParent);
	if (m_bUseSpin)
		pParent->SendMessage (WM_LAYOUTEDITSPINDLG_CANCEL);
	else
		pParent->SendMessage (WM_LAYOUTEDITNUMBERDLG_CANCEL);
}

void CuLayoutSpinDlg::OnOK()
{
	CString strText;
	CEdit* edit = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
	edit->GetWindowText (strText);
	if (strText.IsEmpty())
	{
		MessageBeep (0xFFFFFFFF);
		return;
	}

	ShowWindow (SW_HIDE);
	CWnd* pParent = GetParent ();
	ASSERT_VALID (pParent);
	if (m_bUseSpin)
		pParent->SendMessage (WM_LAYOUTEDITSPINDLG_OK);
	else
		pParent->SendMessage (WM_LAYOUTEDITNUMBERDLG_OK);
}

void CuLayoutSpinDlg::SetSpecialParse (BOOL bSpecialParse)
{
	m_bUseSpecialParse = bSpecialParse;
	if (!IsWindow (m_hWnd))
		return;
	if (!m_bUseSpecialParse)
	{
		CEdit* edit = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
		if (edit && IsWindow (edit->m_hWnd))
		{
			LONG lStyle = ::GetWindowLong (edit->m_hWnd, GWL_STYLE);
			if (!(lStyle & ES_NUMBER))
			{
				lStyle |= ES_NUMBER;
				::SetWindowLong (edit->m_hWnd, GWL_STYLE, lStyle);
			}
		}
		m_cEdit1.SetParsing (FALSE);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CuLayoutSpinDlg message handlers

BOOL CuLayoutSpinDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	if (m_bUseSpecialParse)
	{
		ASSERT (m_wParseStyle != 0);
		if (m_wParseStyle == 0)
			return FALSE;
		m_cEdit1.SubclassEdit(IDC_RCT_EDIT1, this, m_wParseStyle);
	}
	else
	{
		CEdit* edit = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
		if (edit && IsWindow (edit->m_hWnd))
		{
			LONG lStyle = ::GetWindowLong (edit->m_hWnd, GWL_STYLE);
			lStyle |= ES_NUMBER;
			::SetWindowLong (edit->m_hWnd, GWL_STYLE, lStyle);
		}
		m_cEdit1.SetParsing (FALSE);
		m_cEdit1.SubclassEdit(IDC_RCT_EDIT1, this, PES_NATURAL);
	}
	SetRange (m_Max, m_Min);
	CEdit* edit = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
	if (edit && IsWindow (edit->m_hWnd))
		edit->SetReadOnly (m_bReadOnly);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuLayoutSpinDlg::OnClose() 
{
	ShowWindow (SW_HIDE);
}

void CuLayoutSpinDlg::OnKillFocus(CWnd* pNewWnd) 
{
	CDialog::OnKillFocus(pNewWnd);
}

void CuLayoutSpinDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	CEdit*   edit  = (CEdit*)  GetDlgItem (IDC_RCT_EDIT1);
	CSpinButtonCtrl*    button= (CSpinButtonCtrl*)GetDlgItem (IDC_RCT_SPIN1);
	if (!(edit && button))
		return;
	if (!IsWindow(edit->m_hWnd) || !IsWindow(button->m_hWnd))
		 return;
	CRect r;
	GetClientRect (r);
	edit->MoveWindow (r);
	if (m_bUseSpin)
	{
		button->SetBuddy (edit);
		button->ShowWindow (SW_SHOW);
	}
	else
	{
		button->ShowWindow (SW_HIDE);
	}
}

void CuLayoutSpinDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);
	if (bShow)
	{
		CEdit*  edit1 = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
		if (edit1 && IsWindow (edit1->m_hWnd))
		{
			edit1->SetFocus();
			edit1->SetSel(0, -1);
		}
	}
}

void CuLayoutSpinDlg::PostNcDestroy() 
{
	delete this;	
	CDialog::PostNcDestroy();
}

void CuLayoutSpinDlg::OnChangeEdit1() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	CEdit*  edit1 = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
	CString strText;
	GetText (strText);
	if (!strText.IsEmpty() && m_bUseSpin)
	{
		int iNumber = _ttoi ((LPCTSTR)strText);
		if (iNumber > m_Max)
		{
			strText.Format (_T("%d"), m_Max);
			if (edit1 && IsWindow (edit1->m_hWnd))
				edit1->SetWindowText (strText);
			MessageBeep (0xFFFFFFFF);
		}
		if (iNumber < m_Min)
		{
			strText.Format (_T("%d"), m_Min);
			if (edit1 && IsWindow (edit1->m_hWnd))
				edit1->SetWindowText (strText);
			MessageBeep (0xFFFFFFFF);
		}
	}
}

void CuLayoutSpinDlg::SetFocus()
{
	CEdit*  edit1 = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
	if (edit1 && IsWindow (edit1->m_hWnd))
	{
		edit1->SetFocus();
		edit1->SetSel(0, -1);
	}
}

void CuLayoutSpinDlg::SetReadOnly (BOOL bReadOnly)
{
	m_bReadOnly = bReadOnly;
	CEdit*  edit1 = (CEdit*)GetDlgItem (IDC_RCT_EDIT1);
	if (edit1 && IsWindow (edit1->m_hWnd))
	{
		edit1->SetReadOnly(m_bReadOnly);
	}
}
