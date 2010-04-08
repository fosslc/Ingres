/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdtdiff.cpp : implementation file
**    Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Property page to show the detail of differences
**               for some kinds of objects.
**
** History:
**
** 10-Dec-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 07-May-2004 (schph01)
**    SIR #112281, retrieve the current selected item in m_pListCtrl to
**    retrieve the detail information.
** 17-May-2004 (uk$so01)
**    SIR #109220, ISSUE 13407277: additional change to put the header
**    in the pop-up for an item's detail of difference.
*/

#include "stdafx.h"
#include "vsda.h"
#include "dgdtdiff.h"
#include "frdtdiff.h"
#include "vsdtree.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgDifferenceDetail, CPropertyPage)

CuDlgDifferenceDetail::CuDlgDifferenceDetail() : CPropertyPage(CuDlgDifferenceDetail::IDD)
{
	//{{AFX_DATA_INIT(CuDlgDifferenceDetail)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pDetailFrame = NULL;
}

CuDlgDifferenceDetail::~CuDlgDifferenceDetail()
{
}

void CuDlgDifferenceDetail::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDifferenceDetail)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDifferenceDetail, CPropertyPage)
	//{{AFX_MSG_MAP(CuDlgDifferenceDetail)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_UPDATEDATA, OnUpdateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDifferenceDetail message handlers

BOOL CuDlgDifferenceDetail::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	CRect r;
	GetClientRect (r);
	m_pDetailFrame = new CfDifferenceDetail();
	m_pDetailFrame->Create (
		NULL,
		NULL,
		WS_CHILD,
		r,
		this);
	if (m_pDetailFrame)
	{
		m_pDetailFrame->InitialUpdateFrame(NULL, TRUE);
		m_pDetailFrame->ShowWindow(SW_SHOW);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDifferenceDetail::OnSize(UINT nType, int cx, int cy) 
{
	CPropertyPage::OnSize(nType, cx, cy);
	if (m_pDetailFrame && IsWindow (m_pDetailFrame->m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_pDetailFrame->MoveWindow(r);
	}
}

long CuDlgDifferenceDetail::OnUpdateData(WPARAM w, LPARAM l)
{
	TCHAR tchszBuff[512];
	CString strType1,strType2;
	CString strTitle1 = _T("");
	CString strTitle2 = _T("");
	if (!l)
		return 0;

	strType1.Empty();
	strType2.Empty();
	LVCOLUMN vc;
	memset (&vc, 0, sizeof (vc));
	vc.mask = LVCF_TEXT;
	vc.cchTextMax = 510;
	vc.pszText = tchszBuff;
	if (m_pListCtrl->GetColumn(3, &vc))
		strTitle1 = vc.pszText;
	if (m_pListCtrl->GetColumn(4, &vc))
		strTitle2 = vc.pszText;
	
	int iCur,iCount = m_pListCtrl->GetItemCount();
	for (iCur = 0;iCur<iCount;iCur++)
	{
		if (m_pListCtrl->GetItemState (iCur, LVIS_SELECTED)&LVIS_SELECTED)
		{
			strType1 = m_pListCtrl->GetItemText (iCur, 3);
			strType2 = m_pListCtrl->GetItemText (iCur, 4);
			break;
		}
	}

	if (m_pDetailFrame && IsWindow (m_pDetailFrame->m_hWnd))
	{

		CvDifferenceDetail* v1 = m_pDetailFrame->GetLeftPane();
		CvDifferenceDetail* v2 = m_pDetailFrame->GetRightPane();

		CtrfItemData* pItem = (CtrfItemData*)l;
		CaVsdItemData* pVsdItemData = (CaVsdItemData*)pItem->GetUserData();
		if (v1)
		{
			CuDlgDifferenceDetailPage* pPage = v1->GetDetailPage();
			pPage->m_cStatic1.SetWindowText(strTitle1);
			pPage->m_cEdit1.SetSel(0, -1);
			pPage->m_cEdit1.ReplaceSel(_T(""));
			if (!strType1.IsEmpty())
				pPage->m_cEdit1.ReplaceSel(strType1);
		}
		if (v2)
		{
			CuDlgDifferenceDetailPage* pPage = v2->GetDetailPage();
			pPage->m_cStatic1.SetWindowText(strTitle2);
			pPage->m_cEdit1.SetSel(0, -1);
			pPage->m_cEdit1.ReplaceSel(_T(""));
			if (!strType2.IsEmpty())
				pPage->m_cEdit1.ReplaceSel(strType2);
		}
	}
	return 0;
}
