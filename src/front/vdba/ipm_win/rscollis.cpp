/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : rscollis.cpp, Implementation file
**    Project  : CA-OpenIngres/Replicator.
**    Author   : UK Sotheavut
**    Purpose  : Page of a static item Replication.  (Collision) 
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created
** 23-Jul-1999 (schph01)
**    bug #97682
**    Copy the new information about the replicator server type. (full peer etc..)
** 25-Oct-2000 (noifr01)
**    fixed a warning when building in release mode
**
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "ipmframe.h" // CfIpmFrame
#include "ipmdoc.h"   // CdIpmDoc
#include "frepcoll.h" // CfReplicationPageCollision New design (frame layout) 
#include "repcodoc.h" // CdReplicationPageCollisionDoc
#include "vrepcolf.h" // CvReplicationPageCollisionViewLeft
#include "vrepcort.h" // CvReplicationPageCollisionViewRight
#include "rscollis.h"
#include "ipmprdta.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgReplicationStaticPageCollision::CuDlgReplicationStaticPageCollision(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgReplicationStaticPageCollision::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgReplicationStaticPageCollision)
	m_strEdit1 = _T("");
	//}}AFX_DATA_INIT
	m_pFrameLayout = NULL;
	m_pCollisionData = NULL;
}


void CuDlgReplicationStaticPageCollision::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgReplicationStaticPageCollision)
	DDX_Control(pDX, IDC_EDIT1, m_cEdit1);
	DDX_Text(pDX, IDC_EDIT1, m_strEdit1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgReplicationStaticPageCollision, CDialog)
	//{{AFX_MSG_MAP(CuDlgReplicationStaticPageCollision)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
	ON_MESSAGE (WMUSRMSG_CHANGE_SETTING,   OnPropertiesChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicationStaticPageCollision message handlers

void CuDlgReplicationStaticPageCollision::PostNcDestroy() 
{
	if (m_pCollisionData)
	{
		m_pCollisionData->Cleanup();
		delete m_pCollisionData;
	}

	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgReplicationStaticPageCollision::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!m_pFrameLayout)
		return;
	if (!IsWindow (m_pFrameLayout->m_hWnd))
		return;
	CRect rDlg;
	GetClientRect (rDlg);

	m_pFrameLayout->MoveWindow (rDlg);
	m_pFrameLayout->ShowWindow (SW_SHOW);
}


LONG CuDlgReplicationStaticPageCollision::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	CWaitCursor doWaitCursor;
	int nNodeHandle = (int)wParam;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	CdIpmDoc* pIpmDoc = (CdIpmDoc*)wParam;

	try
	{
		if (!m_pCollisionData)
		{
			m_pCollisionData = new CaCollision();
			ASSERT (m_pFrameLayout);
			if (m_pFrameLayout && IsWindow (m_pFrameLayout->m_hWnd))
			{
				CvReplicationPageCollisionViewLeft* pView = (CvReplicationPageCollisionViewLeft*)m_pFrameLayout->GetLeftPane();
				ASSERT (pView);
				if (pView)
				{
					CdReplicationPageCollisionDoc* pDoc = (CdReplicationPageCollisionDoc*)pView->GetDocument();
					ASSERT (pDoc);
					if (pDoc)
						pDoc->SetCollisionData (m_pCollisionData);
				}
			}
		}

		switch (pUps->nIpmHint)
		{
		case 0:
		case FILTER_IPM_EXPRESS_REFRESH:
			break;
		default:
			return 0L;
		}
		
		ASSERT (pUps);
		if (pUps->nIpmHint != FILTER_IPM_EXPRESS_REFRESH)
		{
			m_SvrDta = *(LPRESOURCEDATAMIN)pUps->pStruct;
		}

		if (m_pFrameLayout && IsWindow (m_pFrameLayout->m_hWnd))
		{
			CvReplicationPageCollisionViewLeft* pViewL = (CvReplicationPageCollisionViewLeft*)m_pFrameLayout->GetLeftPane();
			ASSERT (pViewL);
			if (pViewL)
				pViewL->GetTreeCtrl().DeleteAllItems();
		}
		m_pCollisionData->Cleanup();
		//
		// Pass the FALSE to TestMode to disable the test data:
		BOOL bTest = FALSE;
		TestMode(FALSE);
		if (!bTest)
			IPM_RetrieveCollision(pIpmDoc, &m_SvrDta, m_pCollisionData);

		if (m_pFrameLayout && IsWindow (m_pFrameLayout->m_hWnd))
		{
			CWnd* pWnd = m_pFrameLayout->GetLeftPane();
			if (pWnd)
				pWnd->SendMessage (WM_USER_IPMPAGE_UPDATEING, wParam, lParam);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}

	return 0L;
}

LONG CuDlgReplicationStaticPageCollision::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CaReplicationStaticDataPageCollision")) == 0);
	CaReplicationStaticDataPageCollision* pData = (CaReplicationStaticDataPageCollision*)lParam;
	ASSERT (pData);
	if (!pData)
		return 0;
	
	if (m_pCollisionData)
	{
		if (m_pFrameLayout && IsWindow (m_pFrameLayout->m_hWnd))
		{
			CvReplicationPageCollisionViewLeft* pViewL = (CvReplicationPageCollisionViewLeft*)m_pFrameLayout->GetLeftPane();
			ASSERT (pViewL);
			if (pViewL)
				pViewL->GetTreeCtrl().DeleteAllItems();
		}
		m_pCollisionData->Cleanup();
		delete m_pCollisionData;
	}
	m_pCollisionData = pData->m_pData;
	ASSERT (m_pFrameLayout);
	if (m_pFrameLayout && IsWindow (m_pFrameLayout->m_hWnd))
	{
		CvReplicationPageCollisionViewLeft* pView = (CvReplicationPageCollisionViewLeft*)m_pFrameLayout->GetLeftPane();
		ASSERT (pView);
		if (pView)
		{
			CdReplicationPageCollisionDoc* pDoc = (CdReplicationPageCollisionDoc*)pView->GetDocument();
			ASSERT (pDoc);
			if (pDoc)
				pDoc->SetCollisionData (m_pCollisionData);
		}
		CWnd* pWnd = m_pFrameLayout->GetLeftPane();
		if (pWnd)
			pWnd->SendMessage (WM_USER_IPMPAGE_UPDATEING, 0, 0);
	}
	return 0L;
}

LONG CuDlgReplicationStaticPageCollision::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CaReplicationStaticDataPageCollision* pData = NULL;
	try
	{
		pData = new CaReplicationStaticDataPageCollision ();
		pData->m_pData = m_pCollisionData;
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}

BOOL CuDlgReplicationStaticPageCollision::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_cEdit1.ShowWindow (SW_HIDE);
	try
	{
		CRect    r;
		GetClientRect (r);

		m_pFrameLayout = new CfReplicationPageCollision ();
		m_pFrameLayout->Create (
			NULL,
			NULL,
			WS_CHILD,
			r,
			this);
		m_pFrameLayout->InitialUpdateFrame(NULL, FALSE);
		if (m_pFrameLayout && IsWindow (m_pFrameLayout->m_hWnd))
		{
			CvReplicationPageCollisionViewLeft* pView = (CvReplicationPageCollisionViewLeft*)m_pFrameLayout->GetLeftPane();
			ASSERT (pView);
			if (pView)
			{
				CfIpmFrame* pIpmFrame = (CfIpmFrame*)GetParentFrame();
				ASSERT(pIpmFrame);
				if (pIpmFrame)
				{
					CdIpmDoc* pIpmDoc = (CdIpmDoc*)pIpmFrame->GetIpmDoc();
					if (pIpmDoc)
					{
						CTreeCtrl& treeCtrl = pView->GetTreeCtrl();
						CaIpmProperty& property = pIpmDoc->GetProperty();
						treeCtrl.SetFont (CFont::FromHandle(property.GetFont()));
					}
				}
			}
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (...)
	{
		if (m_pFrameLayout)
			m_pFrameLayout->DestroyWindow();
		m_pFrameLayout = NULL;
		TRACE0 ("Cannot allocate frame (collision page new design)\n");
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


long CuDlgReplicationStaticPageCollision::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	CaIpmProperty* pProperty = (CaIpmProperty*)lParam;
	if (!pProperty)
		return 0;
	if (m_pFrameLayout && IsWindow (m_pFrameLayout->m_hWnd))
	{
		CvReplicationPageCollisionViewLeft* pView = (CvReplicationPageCollisionViewLeft*)m_pFrameLayout->GetLeftPane();
		ASSERT (pView);
		if (pView)
		{
			CTreeCtrl& treeCtrl = pView->GetTreeCtrl();
			treeCtrl.SendMessage (WM_SETFONT, (WPARAM)pProperty->GetFont(), MAKELPARAM(TRUE, 0));
		}

		CvReplicationPageCollisionViewRight* pView2 = (CvReplicationPageCollisionViewRight*)m_pFrameLayout->GetRightPane();
		ASSERT (pView2);
		if (pView2)
		{
			pView2->SendMessage(WMUSRMSG_CHANGE_SETTING, wParam, lParam);
		}

	}
	return 0;
}


void CuDlgReplicationStaticPageCollision::TestMode(BOOL bTest)
{
	if (!bTest)
		return;
#ifdef _DEBUG
	CaCollisionSequence* pNewSequence;
	//
	// First Transaction:
	m_pCollisionData->AddTransaction (1000);
	pNewSequence = new CaCollisionSequence (100);
	pNewSequence->AddColumn (_T("C1"), _T("S1"), _T("T1"), 1);
	pNewSequence->AddColumn (_T("C2"), _T("S2"), _T("T2"), 2, TRUE); // Conflicted
	pNewSequence->AddColumn (_T("C3"), _T("S3"), _T("T3"));
	pNewSequence->AddColumn (_T("C4"), _T("S4"), _T("T4"));

	pNewSequence->m_strCollisionType = _T("Update");
	pNewSequence->m_strSourceVNode   = _T("aaa");
	pNewSequence->m_strTargetVNode   = _T("bbb");
	pNewSequence->m_strSourceDatabase= _T("ccc");
	pNewSequence->m_strTargetDatabase= _T("MXDB");
	pNewSequence->m_strTable         = _T("activity");
	pNewSequence->m_strTableOwner    = _T("xyz");
	pNewSequence->m_nCDDS            = 500;
	pNewSequence->m_nTransaction     = 7;
	pNewSequence->m_nPrevTransaction = 800;
	pNewSequence->m_strSourceTransTime     = _T("17-sept-1998:12:00:00");
	pNewSequence->m_strTargetTransTime     = _T("17-sept-1998:12:00:01");
	m_pCollisionData->AddCollisionSequence (1000, pNewSequence);

	pNewSequence = new CaCollisionSequence (101);
	pNewSequence->AddColumn (_T("C1"), _T("S1"), _T("T1"));
	pNewSequence->m_strCollisionType = _T("Update");
	pNewSequence->m_strSourceVNode   = _T("aaa");
	pNewSequence->m_strTargetVNode   = _T("bbb");
	pNewSequence->m_strSourceDatabase= _T("FXDB");
	pNewSequence->m_strTargetDatabase= _T("MXDB");
	pNewSequence->m_strTable         = _T("xxx");
	pNewSequence->m_strTableOwner    = _T("");
	pNewSequence->m_nCDDS            = 501;
	pNewSequence->m_nTransaction     = 8;
	pNewSequence->m_nPrevTransaction = 800;
	pNewSequence->m_strSourceTransTime     = _T("17-sept-1998:12:00:00");
	pNewSequence->m_strTargetTransTime     = _T("17-sept-1998:12:00:01");
	m_pCollisionData->AddCollisionSequence (1000, pNewSequence);

	//
	// Second Transaction:
	m_pCollisionData->AddTransaction (2000);
	pNewSequence = new CaCollisionSequence (102);
	pNewSequence->AddColumn (_T("C1"), _T("S1"), _T("T1"));
	pNewSequence->AddColumn (_T("C2"), _T("S2"), _T("T2"));
	pNewSequence->AddColumn (_T("C3"), _T("S3"), _T("T3"));
	pNewSequence->m_strCollisionType = _T("Update");
	pNewSequence->m_strSourceVNode   = _T("aaa");
	pNewSequence->m_strTargetVNode   = _T("bbb");
	pNewSequence->m_strSourceDatabase= _T("FXDB");
	pNewSequence->m_strTargetDatabase= _T("MXDB");
	pNewSequence->m_strTable         = _T("\"xxx\"");
	pNewSequence->m_strTableOwner    = _T("\"yyy\"");
	pNewSequence->m_nCDDS            = 502;
	pNewSequence->m_nTransaction     = 9;
	pNewSequence->m_nPrevTransaction = 800;
	pNewSequence->m_strSourceTransTime     = _T("17-sept-1998:12:00:00");
	pNewSequence->m_strTargetTransTime     = _T("17-sept-1998:12:00:01");

	m_pCollisionData->AddCollisionSequence (2000, pNewSequence);

	pNewSequence = new CaCollisionSequence (103);
	pNewSequence->AddColumn (_T("C1"), _T("S1"), _T("T1"));
	pNewSequence->AddColumn (_T("C2"), _T("S2"), _T("T2"));
	pNewSequence->m_strCollisionType = _T("Update");
	pNewSequence->m_strSourceVNode   = _T("FRNASU02");
	pNewSequence->m_strTargetVNode   = _T("FRNAGP10");
	pNewSequence->m_strSourceDatabase= _T("FXDB");
	pNewSequence->m_strTargetDatabase= _T("MXDBZZ");
	pNewSequence->m_strTable         = _T("\"xxx\"");
	pNewSequence->m_strTableOwner    = _T("\"yyy\"");
	pNewSequence->m_nCDDS            = 503;
	pNewSequence->m_nTransaction     = 10;
	pNewSequence->m_nPrevTransaction = 800;
	pNewSequence->m_strSourceTransTime     = _T("17-sept-1998:12:00:00");
	pNewSequence->m_strTargetTransTime     = _T("17-sept-1998:12:00:01");

	m_pCollisionData->AddCollisionSequence (2000, pNewSequence);
#endif
}

