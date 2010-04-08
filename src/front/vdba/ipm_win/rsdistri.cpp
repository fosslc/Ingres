/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : rsdistri.cpp : implementation file
**    Project  : CA-OpenIngres/Replicator.
**    Author   : UK Sotheavut
**    Purpose  : Page of a static item Replication.  (Distrib)
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created
**
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "rsdistri.h"
#include "ipmprdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgReplicationStaticPageDistrib::CuDlgReplicationStaticPageDistrib(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgReplicationStaticPageDistrib::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgReplicationStaticPageDistrib)
	m_strEdit1 = _T("");
	//}}AFX_DATA_INIT
}


void CuDlgReplicationStaticPageDistrib::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgReplicationStaticPageDistrib)
	DDX_Control(pDX, IDC_EDIT1, m_cEdit1);
	DDX_Text(pDX, IDC_EDIT1, m_strEdit1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgReplicationStaticPageDistrib, CDialog)
	//{{AFX_MSG_MAP(CuDlgReplicationStaticPageDistrib)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()


void CuDlgReplicationStaticPageDistrib::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgReplicationStaticPageDistrib::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cEdit1.m_hWnd))
		return;
	CRect rDlg;
	GetClientRect (rDlg);
	m_cEdit1.MoveWindow (rDlg);
}


LONG CuDlgReplicationStaticPageDistrib::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;

	switch (pUps->nIpmHint)
	{
	case 0:
	//case FILTER_INTERNAL_SESSIONS:
		break;
	default:
		return 0L;
	}

	ASSERT (pUps);
	try
	{
		RESOURCEDATAMIN* pSvrDta = (RESOURCEDATAMIN*)pUps->pStruct;
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		IPM_CheckDistributionConfig(pDoc, pSvrDta, m_strEdit1);
		UpdateData (FALSE);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	return 0L;
}

LONG CuDlgReplicationStaticPageDistrib::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CaReplicationStaticDataPageDistrib")) == 0);
	m_strEdit1 = (LPCTSTR)lParam;
	UpdateData (FALSE);
	return 0L;
}

LONG CuDlgReplicationStaticPageDistrib::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CaReplicationStaticDataPageDistrib* pData = NULL;
	try
	{
		pData = new CaReplicationStaticDataPageDistrib ();
		pData->m_strText = m_strEdit1;
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}
