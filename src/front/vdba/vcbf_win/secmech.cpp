/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : secmech.cpp : implementation file
**    Project  : Configuration Manager 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Modeless Dialog, Page (Mechanism) of Security
**
** History:
**
** 20-jun-2003 (uk$so01)
**    created.
**    SIR #110389 / INGDBA210
**    Add functionalities for configuring the GCF Security and
**    the security mechanisms.
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
** 16-Mar-2005 (shaha03)
**	  BUG #114089, Display the confirmation check for any tab switch in security, 
**	  if it is edited.Removed security type check.
*/

#include "stdafx.h"
#include "vcbf.h"
#include "secmech.h"
#include "crightdg.h"
#include "wmusrmsg.h"
#include "cbfitem.h"
#include "ll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CuDlgSecurityMechanism::CuDlgSecurityMechanism(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgSecurityMechanism::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgSecurityMechanism)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgSecurityMechanism::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgSecurityMechanism)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgSecurityMechanism, CDialog)
	//{{AFX_MSG_MAP(CuDlgSecurityMechanism)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgSecurityMechanism message handlers

void CuDlgSecurityMechanism::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgSecurityMechanism::OnDestroy() 
{
	Cleanup();
	CDialog::OnDestroy();
}

void CuDlgSecurityMechanism::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_ListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_ListCtrl.MoveWindow (r);
}

BOOL CuDlgSecurityMechanism::OnInitDialog() 
{
	VERIFY (m_ListCtrl.SubclassDlgItem (IDC_LIST1, this));
	LONG style = GetWindowLong (m_ListCtrl.m_hWnd, GWL_STYLE);
	style |= WS_CLIPCHILDREN;
	SetWindowLong (m_ListCtrl.m_hWnd, GWL_STYLE, style);
	m_ImageList.Create(1, 20, TRUE, 1, 0);
	m_ImageListCheck.Create (IDB_CHECK, 16, 1, RGB (255, 0, 0));
	m_ListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_ListCtrl.SetCheckImageList(&m_ImageListCheck);

	const int nColumn = 2;
	LVCOLUMN lvcolumn;
	UINT strHeaderID[nColumn] = { IDS_COL_MECHANISM, IDS_COL_ENABLED};
	int i, hWidth   [nColumn] = {150 , 100};
	int fmt [nColumn] = {LVCFMT_LEFT , LVCFMT_CENTER};
	//
	// Set the number of columns: 2
	CString csCol;
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (i=0; i<nColumn; i++)
	{
		lvcolumn.fmt = fmt[i];
		csCol.LoadString(strHeaderID[i]);
		lvcolumn.pszText = (LPTSTR)(LPCTSTR)csCol;
		lvcolumn.iSubItem = i;
		lvcolumn.cx = hWidth[i];
		if (i==1)
			m_ListCtrl.InsertColumn (i, &lvcolumn, TRUE); // TRUE---> uses checkbox
		else
			m_ListCtrl.InsertColumn (i, &lvcolumn); 
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


LONG CuDlgSecurityMechanism::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	if (lParam) 
	{
			CuSECUREItemData *pSecureData = (CuSECUREItemData*)lParam;
			// terminate old page family
			pSecureData->LowLevelDeactivationWithCheck();
			// Initiate new page family
			BOOL bOk = pSecureData->LowLevelActivationWithCheck();
			ASSERT (bOk);
	}

	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);

	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);
	pButton1->ShowWindow(SW_HIDE);
	pButton2->ShowWindow(SW_HIDE);

	Cleanup();
	m_ListCtrl.DeleteAllItems();
	VCBFll_Security_machanism_get(m_listMechanism);
	Display();
	return 0L;
}

void CuDlgSecurityMechanism::Cleanup()
{
	while (!m_listMechanism.IsEmpty())
		delete (GENERICLINEINFO*)m_listMechanism.RemoveHead();
}


void CuDlgSecurityMechanism::Display()
{
	int index, nCount = 0;
	POSITION pos = m_listMechanism.GetHeadPosition();
	while(pos != NULL)
	{
		GENERICLINEINFO* pObj = (GENERICLINEINFO*)m_listMechanism.GetNext(pos);
		nCount = m_ListCtrl.GetItemCount();
		//
		// Main Item: col 0
		index = m_ListCtrl.InsertItem (nCount, (LPTSTR)pObj->szname);
		if (index == -1)
			return;
		m_ListCtrl.SetItemData (index, (DWORD_PTR)pObj);
		//
		// Value: col 1
		if (&pObj->szvalue[0] && _tcsicmp (_T("TRUE"), pObj->szvalue) == 0)
			m_ListCtrl.SetCheck(nCount, 1, TRUE);
		else 
			m_ListCtrl.SetCheck(nCount, 1, FALSE);
	}
}



//
// ************************************************************************************************
// CuEditableListCtrlMechanism

CuEditableListCtrlMechanism::CuEditableListCtrlMechanism()
	:CuEditableListCtrl()
{

}

CuEditableListCtrlMechanism::~CuEditableListCtrlMechanism()
{
}



BEGIN_MESSAGE_MAP(CuEditableListCtrlMechanism, CuEditableListCtrl)
	//{{AFX_MSG_MAP(CuEditableListCtrlNet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlMechanism message handlers

void CuEditableListCtrlMechanism::OnCheckChange (int iItem, int iSubItem, BOOL bCheck)
{
	ASSERT (iSubItem == 1);
	TRACE1 ("CuDlgSecurityMechanism::OnCheckChange (%d)\n", (int)bCheck);
	CString strOnOff = bCheck? _T("true"): _T("false");
	GENERICLINEINFO* pObj = (GENERICLINEINFO*)GetItemData(iItem);
	try
	{
		if (pObj)
		{
			if (VCBFll_Security_machanism_edit(pObj->szname, (LPTSTR)(LPCTSTR)strOnOff))
				_tcscpy(pObj->szvalue, strOnOff);
			else
			{
				if (_tcsicmp (_T("TRUE"), pObj->szvalue) == 0)
					SetCheck(iItem, iSubItem, TRUE);
				else
					SetCheck(iItem, iSubItem, FALSE);
			}
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuDlgSecurityMechanism::OnCheckChange has caught exception: %s\n", e.m_strReason);
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
