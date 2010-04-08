/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : genericastartup.cpp, Implementation File
**
**
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**
**    Purpose  : Modeless Dialog, Page (Parameter) of NET Server
**               See the CONCEPT.DOC for more detail
** 09-dec-98 (cucjo01)
**      Removed buttons that are not needed on Startup tab
** 21-may-99 (cucjo01)
**      Set Limit Text for Startup Count to 10 character maximum.
**      Update startup count on Validate messages
** 13-aug-99 (cucjo01)
**      Set "Startup Count Tab" entry field to disabled if it is not changeable.
** 06-Jun-2000: (uk$so01) 
**     (BUG #99242)
**     Cleanup code for DBCS compliant
** 17-Dec-2003 (schph01)
**      SIR #111462, Put string into resource files
**
****************************************************************************************/

#include "stdafx.h"
#include "vcbf.h"
#include "wmusrmsg.h"
#include "crightdg.h"
#include "genericstartup.h"
#include "cbfitem.h"
#include "conffrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgGenericStartup dialog

CuDlgGenericStartup::CuDlgGenericStartup(CWnd* pParent)
	: CDialog(CuDlgGenericStartup::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgGenericStartup)
	//}}AFX_DATA_INIT
}


void CuDlgGenericStartup::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgGenericStartup)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgGenericStartup, CDialog)
	//{{AFX_MSG_MAP(CuDlgGenericStartup)
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
// CuDlgGenericStartup message handlers


void CuDlgGenericStartup::OnDestroy() 
{
	VCBF_GenericDestroyItemData (&m_ListCtrl);
	CDialog::OnDestroy();
}

BOOL CuDlgGenericStartup::OnInitDialog() 
{
	VCBF_CreateControlGeneric (m_ListCtrl, this, &m_ImageList, &m_ImageListCheck);
    
    { CEdit* edtStartupCount = (CEdit *)GetDlgItem (IDC_STARTUP_COUNT);
      if (edtStartupCount)
         edtStartupCount->SetLimitText(10);
    }
    
	return TRUE;
}


void CuDlgGenericStartup::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}


void CuDlgGenericStartup::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_ListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_ListCtrl.MoveWindow (r);
}


LONG CuDlgGenericStartup::OnUpdateData (WPARAM wParam, LPARAM lParam)
{

	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);

	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);
    CButton* pButton3 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON3);
	CButton* pButton4 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON4);
    CButton* pButton5 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON5);
	
	CString csButtonTitle;
	csButtonTitle.LoadString(IDS_BUTTON_EDIT_VALUE);
	pButton1->SetWindowText (csButtonTitle);

	csButtonTitle.LoadString(IDS_BUTTON_RESTORE);
	pButton2->SetWindowText (csButtonTitle);

    pButton1->ShowWindow (SW_HIDE);
	pButton2->ShowWindow (SW_HIDE);
    pButton3->ShowWindow (SW_HIDE);
	pButton4->ShowWindow (SW_HIDE);
    pButton5->ShowWindow (SW_HIDE);

	CuCbfListViewItem* pItem = (CuCbfListViewItem*)lParam;
    if (pItem)
    { CEdit* edtStartupCount = (CEdit *)GetDlgItem (IDC_STARTUP_COUNT);
      if (edtStartupCount)
      { edtStartupCount->SetWindowText((LPCTSTR)pItem->m_componentInfo.szcount);
        edtStartupCount->SetSel(0,-1);

        // Disable Edit Control if count is not changeable
        if (!VCBFllCanCountBeEdited (&pItem->m_componentInfo))
           edtStartupCount->EnableWindow(FALSE);
        else
           edtStartupCount->EnableWindow(TRUE);
      }
    }
   
	return 0L;
}

LONG CuDlgGenericStartup::OnValidateData (WPARAM wParam, LPARAM lParam)
{ 
    CWnd* pParent1x = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1x);
	CWnd* pParent2x = pParent1x->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2x);

	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent2x)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2x)->GetDlgItem (IDC_BUTTON2);
	CButton* pButton3 = (CButton*)((CConfRightDlg*)pParent2x)->GetDlgItem (IDC_BUTTON3);
	CButton* pButton4 = (CButton*)((CConfRightDlg*)pParent2x)->GetDlgItem (IDC_BUTTON4);
	CButton* pButton5 = (CButton*)((CConfRightDlg*)pParent2x)->GetDlgItem (IDC_BUTTON5);

	CString csButtonTitle;
	csButtonTitle.LoadString(IDS_BUTTON_EDIT_VALUE);
	pButton1->SetWindowText (csButtonTitle);

	csButtonTitle.LoadString(IDS_BUTTON_RESTORE);
	pButton2->SetWindowText (csButtonTitle);

    pButton1->ShowWindow (SW_HIDE);
	pButton2->ShowWindow (SW_HIDE);
	pButton3->ShowWindow (SW_HIDE);
	pButton4->ShowWindow (SW_HIDE);
	pButton5->ShowWindow (SW_HIDE);

    CuCbfListViewItem* pItem = (CuCbfListViewItem*)lParam;
    
    if (pItem)
    { CString strOldCount, strNewCount;  
      CEdit* edtStartupCount = (CEdit *)GetDlgItem (IDC_STARTUP_COUNT);
      if (edtStartupCount)
         edtStartupCount->GetWindowText(strNewCount);
     
      // Has Startup Count Changed?
      LPCOMPONENTINFO lpComponentInfo = &pItem->m_componentInfo;
      if (lpComponentInfo)
      { if (strNewCount.Compare((LPCTSTR)lpComponentInfo->szcount) != 0)
        { strOldCount=lpComponentInfo->szcount;
          BOOL bSuccess = VCBFllValidCount(lpComponentInfo, (LPTSTR)(LPCTSTR)strNewCount);
          if (bSuccess)
             VCBFll_OnStartupCountChange((LPTSTR)(LPCTSTR)strOldCount.GetBuffer(255));
        } 
      }
    }
   

    return 0L;
}

LONG CuDlgGenericStartup::OnButton1 (UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgGenericStartup::OnButton1 (Edit Value)...\n");
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


LONG CuDlgGenericStartup::OnButton2(UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgGenericStartup::OnButton2...\n");
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

LONG CuDlgGenericStartup::OnButton3(UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgGenericStartup::OnButton3...\n");
	return 0;
}

LONG CuDlgGenericStartup::OnButton4(UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgGenericStartup::OnButton4...\n");
	return 0;
}

LONG CuDlgGenericStartup::OnButton5(UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgGenericStartup::OnButton5...\n");
	return 0;
}

void CuDlgGenericStartup::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
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
