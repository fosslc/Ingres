/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdlgdete.cpp : implementation file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Modal dialog box to show the process of detection
**
** History:
**
** 13-Mar-2001 (uk$so01)
**    Created
**/


#include "stdafx.h"
#include "svriia.h"
#include "xdlgdete.h"
#include "rctools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//#define _GROUP_COMBINATION // Display the rejected combination by group.

static int CALLBACK CompareSubItem (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int nC, n;

	SORTPARAMS* sp = (SORTPARAMS*)lParamSort;
	int nSubItem = sp->m_nItem;
	BOOL bAsc = sp->m_bAsc;

	CaFailureInfo* lpItem1 = (CaFailureInfo*)((CaListCtrlColorItemData*)lParam1)->m_lUserData;
	CaFailureInfo* lpItem2 = (CaFailureInfo*)((CaListCtrlColorItemData*)lParam2)->m_lUserData;
	ASSERT(lpItem1 && lpItem2);
	if (lpItem1 && lpItem2)
	{
		nC = bAsc? 1 : -1;
		switch (nSubItem)
		{
		case 3:
			n = (lpItem1->m_nLine > lpItem2->m_nLine)? 1: (lpItem1->m_nLine < lpItem2->m_nLine)? -1: 0;
			return nC * n;
		default:
			return 0;
		}
	}
	return 0;
}

CxDlgDetectionInfo::CxDlgDetectionInfo(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgDetectionInfo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgDetectionInfo)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pData = NULL;
}


void CxDlgDetectionInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgDetectionInfo)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgDetectionInfo, CDialog)
	//{{AFX_MSG_MAP(CxDlgDetectionInfo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CxDlgDetectionInfo::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	m_ImageCheck.Create (IDB_CHECK4LISTCTRL, 16, 1, RGB (255, 0, 0));
	m_cListCtrl.SetCheckImageList(&m_ImageCheck);
	m_cListCtrl.ActivateColorItem();
	m_cListCtrl.SetFullRowSelected(TRUE);
	m_cListCtrl.SetLineSeperator(TRUE);
	m_cListCtrl.SetAllowSelect(FALSE);
	
	enum {COL_SEP=0, COL_TQ, COL_CS, COL_TS, COL_LINENO, COL_LINETEXT};
	const int nHeader = 6;
	LVCOLUMN lvcolumn;
	LSCTRLHEADERPARAMS2 lsp[nHeader] =
	{
		{IDS_HEADER_SEPARATORS,    80,  LVCFMT_LEFT,  FALSE},
		{IDS_HEADER_TEXTQUALIFIER, 80,  LVCFMT_LEFT,  FALSE},
		{IDS_HEADER_CS_ASONE,      40,  LVCFMT_LEFT,  TRUE },
		{IDS_HEADER_IGNORE_TS,     40,  LVCFMT_LEFT,  TRUE },
		{IDS_HEADER_LINE_NUMBER,   50,  LVCFMT_RIGHT, FALSE},
		{IDS_HEADER_LINE_TEXT,    100,  LVCFMT_LEFT,  FALSE}
	};

	CString strHeader;
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
	for (int i=0; i<nHeader; i++)
	{
		strHeader.LoadString (lsp[i].m_nIDS);
		lvcolumn.fmt      = lsp[i].m_fmt;
		lvcolumn.pszText  = (LPTSTR)(LPCTSTR)strHeader;
		lvcolumn.iSubItem = i;
		lvcolumn.cx       = lsp[i].m_cxWidth;
		m_cListCtrl.InsertColumn (i, &lvcolumn, lsp[i].m_bUseCheckMark);
	}

	if (m_pData != NULL)
	{
		int idx, nCount = m_cListCtrl.GetItemCount();
		CaDataPage1& dataPage1 = m_pData->m_dataPage1;
		CTypedPtrList< CObList, CaFailureInfo* >& listFailureInfo = dataPage1.GetListFailureInfo();
	
		CClientDC dc(&m_cListCtrl);

		BOOL bBlue = TRUE;
		CaFailureInfo* pPrev = NULL;
		int nMaxLength = 100; /* have a minimum width for the case where the only problematic line[s] would be empty or too small)*/
		POSITION pos = listFailureInfo.GetHeadPosition();
		while (pos != NULL)
		{
			CaFailureInfo* pFailureInfo = listFailureInfo.GetNext (pos);

			CSize size = dc.GetTextExtent (pFailureInfo->m_strLine, pFailureInfo->m_strLine.GetLength());
			if (nMaxLength < size.cx)
				nMaxLength = size.cx;

#if defined (_GROUP_COMBINATION)
			if (pPrev)
			{
				if (pPrev->m_bCS != pFailureInfo->m_bCS || 
				    pPrev->m_tchTQ != pFailureInfo->m_tchTQ ||
				    pPrev->m_bTS != pFailureInfo->m_bTS)
				{
					bBlue = !bBlue;
				}
			}
#endif
			idx = m_cListCtrl.InsertItem (nCount, pFailureInfo->m_strSeparator);
			if (idx != -1)
			{
				CString strItem;
				if (pFailureInfo->m_tchTQ == _T('\0'))
					strItem = _T("{none}");
				else
					strItem.Format(_T("{%c}"), pFailureInfo->m_tchTQ);

				m_cListCtrl.SetItemText (idx, COL_TQ, strItem);
				m_cListCtrl.SetCheck    (idx, COL_CS, pFailureInfo->m_bCS);
				m_cListCtrl.SetCheck    (idx, COL_TS, pFailureInfo->m_bTS);
				strItem.Format (_T("%d"), pFailureInfo->m_nLine);
				m_cListCtrl.SetItemText (idx, COL_LINENO, strItem);
				m_cListCtrl.SetItemText (idx, COL_LINETEXT, pFailureInfo->m_strLine);
				m_cListCtrl.SetItemData (idx, (DWORD)pFailureInfo);
			}

#if defined (_GROUP_COMBINATION)
			if (bBlue)
				m_cListCtrl.SetBgColor  (idx, RGB(128, 255, 255));
			else
				m_cListCtrl.SetBgColor  (idx, RGB(255, 255, 255));

			pPrev = pFailureInfo;
#endif

			nCount = m_cListCtrl.GetItemCount();
		}

#if !defined (_GROUP_COMBINATION)
		SORTPARAMS sp;
		sp.m_nItem = 3;
		sp.m_bAsc  = FALSE;
		m_cListCtrl.SortItems(CompareSubItem, (LPARAM)&sp);
#endif

		m_cListCtrl.SetColumnWidth (COL_LINETEXT, nMaxLength);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
