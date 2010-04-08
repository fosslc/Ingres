/*
**  Copyright (C) 2005-2006 Ingres Corporation.
*/

/*
**    Source   : combodlg.cpp Implementation File
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**    Purpose  : Special ComboBox for Owner draw List Control(Editable)
**
** History:
**
** xx-Sep-1997: (uk$so01) 
**    created
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
** 17-Nov-2000: (uk$so01) 
**    Add combobox (dropdown style, editable)
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
**
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "combodlg.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CComboBox* CuLayoutComboDlg::GetComboBox()
{
	return m_bEditable? (CComboBox*)GetDlgItem(IDC_RCT_COMBO2) :(CComboBox*)GetDlgItem(IDC_RCT_COMBO1);
}


CString CuLayoutComboDlg::GetText()
{
	CString s = _T("");
	CComboBox* pCombo = GetComboBox();
	if (pCombo && IsWindow (pCombo->m_hWnd))
	{
		if (!m_bEditable)
		{
			int iSelect = pCombo->GetCurSel();
			if (iSelect != CB_ERR)
			{
				pCombo->GetLBText (iSelect, s);
				return s;
			}
		}
		else
		{
			pCombo->GetWindowText (s);
		}
	}
	return s;
}

int CuLayoutComboDlg::AddItem (LPCTSTR lpszItem)
{
	CComboBox* pCombo = GetComboBox();
	if (pCombo && IsWindow (pCombo->m_hWnd) && lpszItem)
		return pCombo->AddString (lpszItem);
	return CB_ERR;
}


int CuLayoutComboDlg::SetItemData(int nIndex, DWORD dwItemData)
{
	CComboBox* pCombo = GetComboBox();
	if (nIndex != -1 && pCombo && IsWindow (pCombo->m_hWnd))
		return pCombo->SetItemData (nIndex, dwItemData);
	return CB_ERR;

}

DWORD CuLayoutComboDlg::GetItemData(int nIndex)
{
	CComboBox* pCombo = GetComboBox();
	if (nIndex != -1 && pCombo && IsWindow (pCombo->m_hWnd))
		return pCombo->GetItemData (nIndex);
	return 0;
}

CuLayoutComboDlg::CuLayoutComboDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CuLayoutComboDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuLayoutComboDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_StrText  = _T("");
	m_iItem    = -1;
	m_iSubItem = -1;
	m_bEditable= FALSE;
}

CuLayoutComboDlg::CuLayoutComboDlg(CWnd* pParent, CString strText, int iItem, int iSubItem)
	:CDialog(CuLayoutComboDlg::IDD, pParent)
{
	m_StrText  = strText;
	m_iItem    = iItem;
	m_iSubItem = iSubItem;
	m_bEditable= FALSE;
}

void CuLayoutComboDlg::SetData (LPCTSTR lpszText, int iItem, int iSubItem)
{
	m_StrText = lpszText;
	m_iItem   = iItem;
	m_iSubItem= iSubItem;
	CComboBox* pCombo = GetComboBox();
	if (pCombo && IsWindow (pCombo->m_hWnd))
	{
		int nIndex = pCombo->FindStringExact (-1, (LPCTSTR)m_StrText);
		if (nIndex != CB_ERR)
			pCombo->SetCurSel (nIndex);
		else
		{
			if (!m_bEditable)
				pCombo->SetCurSel(-1);
			else
				pCombo->SetWindowText(m_StrText);
		}
	}
}

void CuLayoutComboDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuLayoutComboDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuLayoutComboDlg, CDialog)
	//{{AFX_MSG_MAP(CuLayoutComboDlg)
	ON_WM_SIZE()
	ON_WM_KILLFOCUS()
	ON_WM_CLOSE()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
	ON_CBN_EDITCHANGE(IDC_RCT_COMBO2, OnEditchangeCombo2)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuLayoutComboDlg message handlers



BOOL CuLayoutComboDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CComboBox* pCombo1 = (CComboBox*)GetDlgItem (IDC_RCT_COMBO1);
	CComboBox* pCombo2 = (CComboBox*)GetDlgItem (IDC_RCT_COMBO2);
	if (pCombo1 && IsWindow (pCombo1->m_hWnd) && pCombo2 && IsWindow (pCombo2->m_hWnd))
	{
		int nIndex = pCombo1->FindStringExact (-1, (LPCTSTR)m_StrText);
		if (nIndex != CB_ERR)
			pCombo1->SetCurSel (nIndex);

		nIndex = pCombo2->FindStringExact (-1, (LPCTSTR)m_StrText);
		if (nIndex != CB_ERR)
			pCombo2->SetCurSel (nIndex);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuLayoutComboDlg::GetText (CString& strText)
{
	CComboBox* pCombo = GetComboBox();;
	int nIndex = pCombo->GetCurSel();
	if (nIndex != CB_ERR)
		pCombo->GetLBText (nIndex, strText);
	else
		strText = _T("");
}

void CuLayoutComboDlg::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuLayoutComboDlg::OnCancel ()
{
	ShowWindow (SW_HIDE);
	CWnd* pParent = GetParent ();
	ASSERT_VALID (pParent);
	pParent->SendMessage (WM_LAYOUTCOMBODLG_CANCEL);
}

void CuLayoutComboDlg::OnOK()
{
	ShowWindow (SW_HIDE);
	CWnd* pParent = GetParent ();
	ASSERT_VALID (pParent);
	pParent->SendMessage (WM_LAYOUTCOMBODLG_OK);
}

void CuLayoutComboDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	CComboBox* pCombo1 = (CComboBox*)GetDlgItem (IDC_RCT_COMBO1);
	CComboBox* pCombo2 = (CComboBox*)GetDlgItem (IDC_RCT_COMBO2);
	if (!(pCombo1 && pCombo2))
		return;
	if (!(IsWindow(pCombo1->m_hWnd) && IsWindow(pCombo2->m_hWnd)))
		return;
	CRect r;
	CRect rCombo;
	GetClientRect (r);
	pCombo1->GetClientRect (rCombo);
	r.bottom = r.top + rCombo.Height();

	pCombo1->MoveWindow (r);
	pCombo2->MoveWindow (r);
}

void CuLayoutComboDlg::OnKillFocus(CWnd* pNewWnd) 
{
	CDialog::OnKillFocus(pNewWnd);
}

void CuLayoutComboDlg::OnClose() 
{
	ShowWindow (SW_HIDE);
}

void CuLayoutComboDlg::SetFocus()
{
	CComboBox* pCombo = GetComboBox();;
	if (pCombo && IsWindow (pCombo->m_hWnd))
	{
		pCombo->SetFocus();
	}
}

void CuLayoutComboDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);
	if (bShow)
	{
		CComboBox* pCombo = GetComboBox();
		ASSERT(pCombo);
		if (!pCombo)
			return;
		pCombo->SetFocus();
		pCombo->ShowDropDown();
	}
}

void CuLayoutComboDlg::ChangeMode(BOOL bEditable)
{
	CComboBox* pCombo = NULL;
	if (!IsWindow (m_hWnd))
		return;
	if (bEditable)
	{
		pCombo = (CComboBox*)GetDlgItem (IDC_RCT_COMBO1);
		if (pCombo)
			pCombo->ShowWindow(SW_HIDE);
		pCombo = (CComboBox*)GetDlgItem (IDC_RCT_COMBO2);
		if (pCombo)
			pCombo->ShowWindow(SW_SHOW);
	}
	else
	{
		pCombo = (CComboBox*)GetDlgItem (IDC_RCT_COMBO2);
		if (pCombo)
			pCombo->ShowWindow(SW_HIDE);
		pCombo = (CComboBox*)GetDlgItem (IDC_RCT_COMBO1);
		if (pCombo)
			pCombo->ShowWindow(SW_SHOW);
	}
}

void CuLayoutComboDlg::OnEditchangeCombo2() 
{
	CComboBox* pCombo = pCombo = (CComboBox*)GetDlgItem (IDC_RCT_COMBO2);
	pCombo->ShowDropDown(FALSE);
}