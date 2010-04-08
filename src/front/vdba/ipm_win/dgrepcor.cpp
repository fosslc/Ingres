/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgrepcor.cpp, Implementation file
**    Project  : INGRES II/ Replication Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Modeless Dialog to display the conflict rows
**
** History:
**
** xx-May-1998 (uk$so01)
**    Created
** 23-Jul-1999 (schph01)
**    bug #97682
**    When the CDDS target type is "Protected read-only" the radio button
**   "Target Prevails" is grayed.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgrepcor.h"
#include "vrepcort.h"
#include "vrepcolf.h"
#include "frepcoll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicationPageCollisionRight dialog


CuDlgReplicationPageCollisionRight::CuDlgReplicationPageCollisionRight(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgReplicationPageCollisionRight::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgReplicationPageCollisionRight)
	m_strCollisionType = _T("");
	m_strCDDS = _T("");
	m_strTable = _T("");
	m_strTransaction = _T("");
	m_strSourceTransTime = _T("");
	m_strTargetTransTime = _T("");
	m_strPrevTransID = _T("");
	m_strSourceDbName = _T("");
	//}}AFX_DATA_INIT
}


void CuDlgReplicationPageCollisionRight::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgReplicationPageCollisionRight)
	DDX_Text(pDX, IDC_EDIT1, m_strCollisionType);
	DDX_Text(pDX, IDC_EDIT2, m_strCDDS);
	DDX_Text(pDX, IDC_EDIT3, m_strTable);
	DDX_Text(pDX, IDC_EDIT4, m_strTransaction);
	DDX_Text(pDX, IDC_EDIT5, m_strSourceTransTime);
	DDX_Text(pDX, IDC_EDIT8, m_strTargetTransTime);
	DDX_Text(pDX, IDC_EDIT6, m_strPrevTransID);
	DDX_Text(pDX, IDC_EDIT7, m_strSourceDbName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgReplicationPageCollisionRight, CDialog)
	//{{AFX_MSG_MAP(CuDlgReplicationPageCollisionRight)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_RADIO1, OnRadioUndefined)
	ON_BN_CLICKED(IDC_RADIO2, OnRadioSourcePrevail)
	ON_BN_CLICKED(IDC_RADIO3, OnRadioTargetPreval)
	ON_BN_CLICKED(IDC_BUTTON1, OnApplyResolution)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicationPageCollisionRight message handlers

void CuDlgReplicationPageCollisionRight::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgReplicationPageCollisionRight::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	LONG style = GetWindowLong (m_cListCtrl.m_hWnd, GWL_STYLE);
	style |= LVS_SHOWSELALWAYS;
	SetWindowLong (m_cListCtrl.m_hWnd, GWL_STYLE, style);
	m_ImageList.Create(1, 20, TRUE, 1, 0);
	m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_cListCtrl.SetFullRowSelected (TRUE);
	m_cListCtrl.SetLineSeperator();

	const int LAYOUT_NUMBER = 4;
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_TC_COLUMN,   80, LVCFMT_LEFT, FALSE},
		{IDS_TC_KEY2,     60, LVCFMT_RIGHT,FALSE},
		{IDS_TC_LOCAL,   200, LVCFMT_LEFT, FALSE},
		{IDS_TC_TARGET,  200, LVCFMT_LEFT, FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, LAYOUT_NUMBER);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgReplicationPageCollisionRight::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	CListCtrl* pList = (CListCtrl*)GetDlgItem (IDC_LIST1);
	if (pList && IsWindow (pList->m_hWnd))
	{
		CRect r, rDlg;
		GetClientRect (rDlg);
		pList->GetWindowRect (r);
		ScreenToClient (r);
		r.left  = rDlg.left  +1;
		r.right = rDlg.right -1;
		r.bottom= rDlg.bottom-1;
		pList->MoveWindow (r);
	}
}


void CuDlgReplicationPageCollisionRight::SetListCtrlColumnHeader (int iColumn, LPCTSTR lpszText)
{
	LV_COLUMN lvcolumn;
	memset (&lvcolumn, 0, sizeof (lvcolumn));
	lvcolumn.mask      = LVCF_TEXT;
	lvcolumn.pszText   = (LPTSTR)lpszText;
	lvcolumn.cchTextMax= lstrlen (lpszText) + 1;
	m_cListCtrl.SetColumn (iColumn, &lvcolumn);
}


void CuDlgReplicationPageCollisionRight::DisplayData (CaCollisionSequence* pData)
{
	CString strItem;
	m_cListCtrl.DeleteAllItems();
	ASSERT (pData);
	if (!pData)
		return;
	m_strCollisionType = pData->m_strCollisionType;
	m_strCDDS.Format (_T("%d"), pData->m_nCDDS);
	m_strTable.Format(_T("%s.%s"), (LPCTSTR)pData->m_strTableOwner, (LPCTSTR)pData->m_strTable);
	m_strTransaction.Format (_T("%ld"), pData->m_nTransaction);
	m_strPrevTransID.Format (_T("%ld"), pData->m_nPrevTransaction);
	m_strSourceTransTime = pData->m_strSourceTransTime;
	m_strTargetTransTime = pData->m_strTargetTransTime;
	m_strSourceDbName.Format(_T("%s::%s"), (LPCTSTR)pData->m_strSourceVNode, (LPCTSTR)pData->m_strSourceDatabase);
	UpdateData (FALSE);

	strItem.Format (_T("%s::%s"), (LPCTSTR)pData->m_strLocalVNode, (LPCTSTR)pData->m_strLocalDatabase);
	SetListCtrlColumnHeader (2, strItem);
	strItem.Format (_T("%s::%s"), (LPCTSTR)pData->m_strTargetVNode, (LPCTSTR)pData->m_strTargetDatabase);
	SetListCtrlColumnHeader (3, strItem);

	//
	// Radio:
	int nRadio = IDC_RADIO1;
	int nImage = pData->GetImageType();
	switch (nImage)
	{
	case CaCollisionItem::PREVAIL_UNRESOLVED:
		nRadio = IDC_RADIO1;
		break;
	case CaCollisionItem::PREVAIL_SOURCE:
		nRadio = IDC_RADIO2;
		break;
	case CaCollisionItem::PREVAIL_TARGET:
		nRadio = IDC_RADIO3;
		break;
	default:
		nRadio = IDC_RADIO1;
		break;
	}
	CheckRadioButton(IDC_RADIO1, IDC_RADIO3, nRadio);
	if (pData->IsResolved())
	{
		GetDlgItem(IDC_RADIO1) ->EnableWindow (FALSE);
		GetDlgItem(IDC_RADIO2) ->EnableWindow (FALSE);
		GetDlgItem(IDC_RADIO3) ->EnableWindow (FALSE);
		GetDlgItem(IDC_BUTTON1)->EnableWindow (FALSE);
	}
	else
	{
		GetDlgItem(IDC_RADIO1) ->EnableWindow (TRUE);
		GetDlgItem(IDC_RADIO2) ->EnableWindow (TRUE);

		if (pData->m_nSvrTargetType == REPLIC_PROT_RO)
			GetDlgItem(IDC_RADIO3) ->EnableWindow (FALSE);
		else
			GetDlgItem(IDC_RADIO3) ->EnableWindow (TRUE);
		GetDlgItem(IDC_BUTTON1)->EnableWindow (TRUE);
	}

	int nCount = m_cListCtrl.GetItemCount();
	m_cListCtrl.ActivateColorItem();
	POSITION pos = pData->m_listColumns.GetHeadPosition();
	while (pos != NULL)
	{
		CaCollisionColumn* pColumn = pData->m_listColumns.GetNext (pos);
		nCount = m_cListCtrl.GetItemCount();
		strItem.Format (_T("%d"), pColumn->m_nKey);
		m_cListCtrl.InsertItem  (nCount,    pColumn->m_strColumn);
		m_cListCtrl.SetItemText (nCount, 1, strItem);
		m_cListCtrl.SetItemText (nCount, 2, pColumn->m_strSource);
		m_cListCtrl.SetItemText (nCount, 3, pColumn->m_strTarget);
		if (pColumn->IsConflicted())
		{
			m_cListCtrl.SetFgColor  (nCount, RGB (255, 0,   0));
			m_cListCtrl.SetBgColor  (nCount, RGB (255, 255, 0));
		}
	}
}

CaCollisionItem* CuDlgReplicationPageCollisionRight::ViewLeftGetCurrentSelectedItem()
{
	CfReplicationPageCollision* pFrame = (CfReplicationPageCollision*)GetParentFrame();
	ASSERT (pFrame);
	if (pFrame)
	{
		CvReplicationPageCollisionViewLeft* pViewL = (CvReplicationPageCollisionViewLeft*)pFrame->GetLeftPane();
		HTREEITEM hCurSel = (pViewL->GetTreeCtrl()).GetSelectedItem();
		return hCurSel? (CaCollisionItem*)(pViewL->GetTreeCtrl()).GetItemData (hCurSel): NULL;
	}
	return NULL;
}

void CuDlgReplicationPageCollisionRight::ViewLeftUpdateImageCurrentSelectedItem()
{
	CfReplicationPageCollision* pFrame = (CfReplicationPageCollision*)GetParentFrame();
	ASSERT (pFrame);
	if (pFrame)
	{
		CvReplicationPageCollisionViewLeft* pViewL = (CvReplicationPageCollisionViewLeft*)pFrame->GetLeftPane();
		pViewL->CurrentSelectedUpdateImage ();
	}
}


void CuDlgReplicationPageCollisionRight::OnRadioUndefined() 
{
	CaCollisionItem* pSelectItem = ViewLeftGetCurrentSelectedItem();
	ASSERT (pSelectItem->IsTransaction() == FALSE);
	if (!pSelectItem->IsTransaction())
	{
		CaCollisionSequence* pSequence = (CaCollisionSequence*)pSelectItem;
		pSequence->SetImageType (CaCollisionItem::PREVAIL_UNRESOLVED);
		ViewLeftUpdateImageCurrentSelectedItem();
	}
}

void CuDlgReplicationPageCollisionRight::OnRadioSourcePrevail() 
{
	CaCollisionItem* pSelectItem = ViewLeftGetCurrentSelectedItem();
	ASSERT (pSelectItem->IsTransaction() == FALSE);
	if (!pSelectItem->IsTransaction())
	{
		CaCollisionSequence* pSequence = (CaCollisionSequence*)pSelectItem;
		pSequence->SetImageType (CaCollisionItem::PREVAIL_SOURCE);
		ViewLeftUpdateImageCurrentSelectedItem();
	}
}

void CuDlgReplicationPageCollisionRight::OnRadioTargetPreval() 
{
	CaCollisionItem* pSelectItem = ViewLeftGetCurrentSelectedItem();
	ASSERT (pSelectItem->IsTransaction() == FALSE);
	if (!pSelectItem->IsTransaction())
	{
		CaCollisionSequence* pSequence = (CaCollisionSequence*)pSelectItem;
		pSequence->SetImageType (CaCollisionItem::PREVAIL_TARGET);
		ViewLeftUpdateImageCurrentSelectedItem();
	}
}


void CuDlgReplicationPageCollisionRight::OnApplyResolution() 
{
	CfReplicationPageCollision* pFrame = (CfReplicationPageCollision*)GetParentFrame();
	ASSERT (pFrame);
	if (pFrame)
	{
		CvReplicationPageCollisionViewLeft* pViewL = (CvReplicationPageCollisionViewLeft*)pFrame->GetLeftPane();
		if (!pViewL->CheckResolveCurrentTransaction())
		{
			//_T("Not all sequences are marked as \nSource prevails or Target prevails.");
			AfxMessageBox (IDS_E_ALL_SEQUENCES);
			return;
		}
		pViewL->ResolveCurrentTransaction();
	}
}
