/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : staautco.cpp , Implementation File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Status Page for Component that starts automatically by others
**
** History:
**
** 04-Mar-1999 (uk$so01)
**    created
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 30-Oct-2003 (noifr01)
**    upon cleanup after massive ingres30->main propagation, added CR 
**    after traling }, which was causing problems in propagations
**
**/

#include "stdafx.h"
#include "ivm.h"
#include "mgrfrm.h"
#include "staautco.h"
#include "ivmdisp.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgStatusAutoStartComponent dialog


CuDlgStatusAutoStartComponent::CuDlgStatusAutoStartComponent(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgStatusAutoStartComponent::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgStatusAutoStartComponent)
	m_bUnreadCriticalEvent = FALSE;
	m_bStarted = FALSE;
	//}}AFX_DATA_INIT
}


void CuDlgStatusAutoStartComponent::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgStatusAutoStartComponent)
	DDX_Check(pDX, IDC_CHECK1, m_bUnreadCriticalEvent);
	DDX_Check(pDX, IDC_CHECK5, m_bStarted);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgStatusAutoStartComponent, CDialog)
	//{{AFX_MSG_MAP(CuDlgStatusAutoStartComponent)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_IVM_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_IVM_COMPONENT_CHANGED, OnComponentChanged)
	ON_MESSAGE (WMUSRMSG_FIND_ACCEPTED, OnFindAccepted)
	ON_MESSAGE (WMUSRMSG_FIND, OnFind)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgStatusAutoStartComponent message handlers

void CuDlgStatusAutoStartComponent::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}


LONG CuDlgStatusAutoStartComponent::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	try
	{
		CaTreeComponentItemData* pItem = NULL;
		CaPageInformation* pPageInfo = (CaPageInformation*)lParam;
		if (pPageInfo)
			pItem = pPageInfo->GetIvmItem();
		//
		// Result from the Refresh by the Worker thread of Main application <CappIvmApplication>
		if (wParam == 0 && lParam == 0)
		{
			CfManagerFrame* pManagerFrame = (CfManagerFrame*)GetParentFrame();
			ASSERT (pManagerFrame);
			if (!pManagerFrame)
				return 0L;
			CvManagerViewLeft* pTreeView = pManagerFrame->GetLeftView();
			if (!pTreeView)
				return 0L;
			CTreeCtrl& cTree = pTreeView->GetTreeCtrl();
			HTREEITEM hSelected = cTree.GetSelectedItem();
			if (!hSelected)
				return 0L;
			pItem = (CaTreeComponentItemData*)cTree.GetItemData (hSelected);
		}
		if (!pItem)
			return 0L;

		m_bUnreadCriticalEvent = (pItem->m_treeCtrlData.AlertGetCount() > 0)? TRUE: FALSE;
		m_bStarted = FALSE;
		int nStatus = pItem->GetItemStatus();
		switch (nStatus)
		{
		case COMP_NORMAL:
			break;
		case COMP_START:
		case COMP_STARTMORE:
			m_bStarted = TRUE;
			break;
		default:
			break;
		}
		UpdateData (FALSE);
		return 0L;
	}
	catch (...)
	{
		AfxMessageBox (_T("System error(CuDlgStatusAutoStartComponent::OnUpdateData): \nFailed to refresh the Data"));
	}

	return 0L;
}


LONG CuDlgStatusAutoStartComponent::OnComponentChanged (WPARAM wParam, LPARAM lParam)
{
	OnUpdateData (0, 0);
	return 0;
}

LONG CuDlgStatusAutoStartComponent::OnFindAccepted (WPARAM wParam, LPARAM lParam)
{
	//
	// This pane is not currently used !
	ASSERT (FALSE);
	return (LONG)TRUE;
}


LONG CuDlgStatusAutoStartComponent::OnFind (WPARAM wParam, LPARAM lParam)
{
	//
	// This pane is not currently used !
	ASSERT (FALSE);
	TRACE0 ("CuDlgStatusAutoStartComponent::OnFind \n");

	return 0L;
}
