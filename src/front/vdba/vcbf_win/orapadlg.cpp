/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : orapadlg.cpp, Implementation File 
**    Project  : OpenIngres Configuration Manager 
**    Author   : Joseph Cuccia
**    Purpose  : Modeless Dialog, Page (Parameter) of Oracle Gateway 
**
** History:
**
** xx-Dec-1998 (cucjo01)
**    created
** 06-Jun-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
**/

#include "stdafx.h"
#include "vcbf.h"
#include "orapadlg.h"
#include "wmusrmsg.h"
#include "crightdg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgGW_OracleParameter::CuDlgGW_OracleParameter(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgGW_OracleParameter::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgGW_OracleParameter)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgGW_OracleParameter::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgGW_OracleParameter)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgGW_OracleParameter, CDialog)
	//{{AFX_MSG_MAP(CuDlgGW_OracleParameter)
	ON_WM_SIZE()
	ON_NOTIFY(HDN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON1, OnButton1)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON2, OnButton2)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON3, OnButton3)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON4, OnButton4)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON5, OnButton5)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_VALIDATE, OnValidateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgGW_OracleParameter message handlers

void CuDlgGW_OracleParameter::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgGW_OracleParameter::OnDestroy() 
{
	VCBF_GenericDestroyItemData (&m_ListCtrl);
	CDialog::OnDestroy();
}

BOOL CuDlgGW_OracleParameter::OnInitDialog() 
{
	VCBF_CreateControlGeneric (m_ListCtrl, this, &m_ImageList, &m_ImageListCheck);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgGW_OracleParameter::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_ListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_ListCtrl.MoveWindow (r);
}

void CuDlgGW_OracleParameter::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	HD_NOTIFY *phdn = (HD_NOTIFY *) pNMHDR;
	//
	// Enable/Disable Button (Edit Value, Restore)
	BOOL bEnable = (m_ListCtrl.GetSelectedCount() == 1)? TRUE: FALSE;
	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);
	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);
	if (pButton1 && pButton2)
	{
		pButton1->EnableWindow (bEnable);
		pButton2->EnableWindow (bEnable);
	}
	
	*pResult = 0;
}

LONG CuDlgGW_OracleParameter::OnButton1 (UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgGW_OracleParameter::OnButton1 (Edit Value)...\n");
	CRect r, rCell;
	UINT nState = 0;
	int iNameCol = 1;
	int iIndex = -1;
	int i, nCount = m_ListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		nState = m_ListCtrl.GetItemState (i, LVIS_SELECTED);	
		if (nState & LVIS_SELECTED)
		{
			iIndex = i;
			break;
		}
	}
	if (iIndex == -1)
		return 0;
	m_ListCtrl.GetItemRect (iIndex, r, LVIR_BOUNDS);
	BOOL bOk = m_ListCtrl.GetCellRect (r, iNameCol, rCell);
	if (bOk)
	{
		rCell.top    -= 2;
		rCell.bottom += 2;
		m_ListCtrl.EditValue (iIndex, iNameCol, rCell);
	}
	return 0;
}


LONG CuDlgGW_OracleParameter::OnButton2(UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgGW_OracleParameter::OnButton2...\n");
	UINT nState = 0;
	int iIndex = -1;
	int i, nCount = m_ListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		nState = m_ListCtrl.GetItemState (i, LVIS_SELECTED);	
		if (nState & LVIS_SELECTED)
		{
			iIndex = i;
			break;
		}
	}
	if (iIndex == -1)
		return 0;
	m_ListCtrl.HideProperty();
	VCBF_GenericResetValue (m_ListCtrl,iIndex);
	return 0;
}

LONG CuDlgGW_OracleParameter::OnButton3(UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgGW_OracleParameter::OnButton3...\n");
	return 0;
}

LONG CuDlgGW_OracleParameter::OnButton4(UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgGW_OracleParameter::OnButton4...\n");
	return 0;
}

LONG CuDlgGW_OracleParameter::OnButton5(UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgGW_OracleParameter::OnButton5...\n");
	return 0;
}


LONG CuDlgGW_OracleParameter::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);

	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);

	CString csButtonTitle;
	csButtonTitle.LoadString(IDS_BUTTON_EDIT_VALUE);
	pButton1->SetWindowText (csButtonTitle);

	csButtonTitle.LoadString(IDS_BUTTON_RESTORE);
	pButton2->SetWindowText (csButtonTitle);

	pButton1->ShowWindow (SW_SHOW);
	pButton2->ShowWindow (SW_SHOW);

	VCBF_GenericAddItems (&m_ListCtrl);
	return 0L;
}


LONG CuDlgGW_OracleParameter::OnValidateData (WPARAM wParam, LPARAM lParam)
{
	m_ListCtrl.HideProperty();
	return 0L;
}