/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : stadrdlg.cpp, Implementation File
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**    Purpose  : Modeless Dialog, Page (Derived) of Star 
**               See the CONCEPT.DOC for more detail
**
**  History:
**	xx-Sep-1997 (uk$so01)
**	    created
**	06-Jun-2000 (uk$so01)
**	    bug 99242 Handle DBCS
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings / errors.
**	17-Dec-2003 (schph01)
**	    SIR #111462, Put string into resource files
*/

#include "stdafx.h"
#include "vcbf.h"
#include "crightdg.h"
#include "stadrdlg.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CuDlgStarDerived dialog


CuDlgStarDerived::CuDlgStarDerived(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgStarDerived::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgStarDerived)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgStarDerived::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgStarDerived)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgStarDerived, CDialog)
	//{{AFX_MSG_MAP(CuDlgStarDerived)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
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
// CuDlgStarDerived message handlers

void CuDlgStarDerived::OnDestroy() 
{
	VCBF_GenericDerivedDestroyItemData (&m_ListCtrl);
	CDialog::OnDestroy();
}

BOOL CuDlgStarDerived::OnInitDialog() 
{
	VCBF_CreateControlGenericDerived (m_ListCtrl, this, &m_ImageList, &m_ImageListCheck);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgStarDerived::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_ListCtrl.m_hWnd))	
		return;
	CRect r;
	GetClientRect (r);
	m_ListCtrl.MoveWindow (r);		
}

void CuDlgStarDerived::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}


LONG CuDlgStarDerived::OnUpdateData (WPARAM wParam, LPARAM lParam)
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

	csButtonTitle.LoadString(IDS_BUTTON_RECALCULATE);
	pButton2->SetWindowText (csButtonTitle);

	pButton1->ShowWindow (SW_SHOW);
	pButton2->ShowWindow (SW_SHOW);

	if (!IsWindow (m_ListCtrl.m_hWnd))
		return 0;
	VCBF_GenericDerivedAddItem (&m_ListCtrl);
	return 0L;
}

LONG CuDlgStarDerived::OnValidateData (WPARAM wParam, LPARAM lParam)
{
	if (!IsWindow (m_ListCtrl.m_hWnd))
		return 0;
	m_ListCtrl.HideProperty();
	return 0L;
}


LONG CuDlgStarDerived::OnButton1 (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgStarDerived::OnButton1...\n");
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


LONG CuDlgStarDerived::OnButton2(WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgStarDerived::OnButton2...\n");

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
	VCBF_GenericDerivedRecalculate (m_ListCtrl, iIndex);
	return 0;
}


LONG CuDlgStarDerived::OnButton3(WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgStarDerived::OnButton3...\n");
	return 0;
}

LONG CuDlgStarDerived::OnButton4(WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgStarDerived::OnButton4...\n");
	return 0;
}

LONG CuDlgStarDerived::OnButton5(WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgStarDerived::OnButton5...\n");
	return 0;
}

void CuDlgStarDerived::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	//
	// Enable/Disable Button (Edit Value, Recallculate)
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
