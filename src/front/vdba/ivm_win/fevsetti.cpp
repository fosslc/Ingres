/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
** 
**    Source   : fevsetti.cpp, Implementation file
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Frame support to provide the splitter window
**
** History:
**
** 21-May-1999 (uk$so01)
**    Created
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
**
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "fevsetti.h"
#include "ivmsgdml.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CfEventSetting::CfEventSetting(CdEventSetting* pDoc)
{
	ASSERT (pDoc);
	m_pEventSettingDoc = pDoc;
	m_bAllViewCreated = FALSE;

	//
	// Clone the message Entry from the global:
	m_pMessageEntry = theApp.m_setting.m_messageManager.Clone();

	BOOL bStoring = FALSE;
	IVM_SerializeMessageSetting(m_pMessageEntry, &m_userCategory, bStoring);


	//
	// Populate the tree of Category View with the Unclassified (others) messages:
	m_treeCategoryView.Populate (m_pMessageEntry);
	//
	// Populate the tree of Category View with the user specialized message categories:
	m_treeCategoryView.Populate (m_pMessageEntry, m_userCategory);

	//
	// Populate the tree of State View from the Tree of Category View:
	m_treeStateView.UpdateTree (NULL, &m_treeCategoryView);
}

CfEventSetting::~CfEventSetting()
{
	if (m_pMessageEntry)
		delete m_pMessageEntry;
}


BEGIN_MESSAGE_MAP(CfEventSetting, CFrameWnd)
	//{{AFX_MSG_MAP(CfEventSetting)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CfEventSetting message handlers

BOOL CfEventSetting::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	//
	//          Nest Splitter ->*
	// ------------------------ * -----------------------
	// |                       |*                        |
	// |                       |*                        |
	// | CvEventSettingLeft    |* CvEventSettingRight    |
	// |                       |*                        |
	// ------------------------ *------------------------ 
	// *************************************************** <-- Main Splitter
	//
	// List of Event category View (CvEventSettingBottom)
	//

	ASSERT (m_pEventSettingDoc);
	if (!m_pEventSettingDoc)
	{
		TRACE0 ("Main document is NULL while creating CfEventSetting\n");
		return FALSE;
	}
	CCreateContext context1, context2, context3;
	CRuntimeClass* pView1 = RUNTIME_CLASS(CvEventSettingLeft);
	CRuntimeClass* pView2 = RUNTIME_CLASS(CvEventSettingRight);
	CRuntimeClass* pView3 = RUNTIME_CLASS(CvEventSettingBottom);
	
	context1.m_pNewViewClass = pView1;
	context1.m_pCurrentDoc   = m_pEventSettingDoc;
	context2.m_pNewViewClass = pView2;
	context2.m_pCurrentDoc   = context1.m_pCurrentDoc;
	context3.m_pNewViewClass = pView3;
	context3.m_pCurrentDoc   = context1.m_pCurrentDoc;
	CRect r;
	GetClientRect (r);
	CSize v1Size ((int)(r.Width()/2), (int)(r.Height()*0.6));
	CSize v2Size ((int)(r.Width()/2), (int)(r.Height()*0.6));
	CSize v3Size (r.Width(), (int)(r.Height()*0.4));
	//
	// Create a main splitter of 2 rows and 1 column.
	//
	if (!m_MainSplitterWnd.CreateStatic (this, 2, 1))
	{
		TRACE0 ("CfEventSetting::OnCreateClient: Failed to create Splitter\n");
		return FALSE;
	}
	//
	// Create a nested splitter which has 1 row and 2 columns.
	// It is child of the second pane of the main splitter.
	BOOL b = m_NestSplitterWnd.CreateStatic (
		&m_MainSplitterWnd, // Parent is the main splitter.
		1,                  // 1 row.
		2,                  // 2 columns.
		WS_CHILD|WS_VISIBLE|WS_BORDER,  m_MainSplitterWnd.IdFromRowCol (0, 0));
	if (!b)
	{
		TRACE0 ("CfEventSetting::OnCreateClient: Failed to create Nested Splitter\n");
		return FALSE;
	}
	//
	// Add the first splitter pane of a nested splitter - the view (CvEventSettingLeft) in row 0 column 0
	// of the main splitter.
	//
	if (!m_NestSplitterWnd.CreateView (0, 0, pView1, v1Size, &context1))
	{
		TRACE0 ("CfEventSetting::OnCreateClient: Failed to create first pane\n");
		return FALSE;
	}
	//
	// Add the first splitter pane of the Nested Splitter -
	// the View (CvEventSettingLeft) in Row 0 col 1
	if (!m_NestSplitterWnd.CreateView (0, 1, pView2, v2Size, &context2))
	{
		TRACE0 ("CfEventSetting::OnCreateClient: Failed to create second pane\n");
		return FALSE;
	}
	//
	// Add the second splitter pane of the main Splitter - 
	// the View (CvEventSettingBottom) in  row 1 column 0
	if (!m_MainSplitterWnd.CreateView (1, 0, pView3, v3Size, &context3))
	{
		TRACE0 ("CfEventSetting::OnCreateClient: Failed to create first pane\n");
		return FALSE;
	}


	m_MainSplitterWnd.SetRowInfo    (0, v1Size.cy, 10);
	m_MainSplitterWnd.SetRowInfo    (1, v3Size.cy, 10);
	m_MainSplitterWnd.RecalcLayout();
	m_NestSplitterWnd.RecalcLayout();
	m_bAllViewCreated = TRUE;
	SetActiveView ((CView*)m_NestSplitterWnd.GetPane(0, 0));
	return TRUE;

	return TRUE;
}

void CfEventSetting::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);
}

CvEventSettingLeft* CfEventSetting::GetLeftView()
{
	ASSERT (m_bAllViewCreated);
	if (!m_bAllViewCreated)
		return NULL;
	CvEventSettingLeft* pView =(CvEventSettingLeft*)m_NestSplitterWnd.GetPane (0, 0);
	ASSERT_VALID(pView);
	return pView;
}

CvEventSettingRight* CfEventSetting::GetRightView()
{
	ASSERT (m_bAllViewCreated);
	if (!m_bAllViewCreated)
		return NULL;
	CvEventSettingRight* pView =(CvEventSettingRight*)m_NestSplitterWnd.GetPane (0, 1);
	ASSERT_VALID(pView);
	return pView;
}

CvEventSettingBottom* CfEventSetting::GetBottomView()
{
	ASSERT (m_bAllViewCreated);
	if (!m_bAllViewCreated)
		return NULL;
	CvEventSettingBottom* pView =(CvEventSettingBottom*)m_MainSplitterWnd.GetPane (1, 0);
	ASSERT_VALID(pView);
	return pView;
}


UINT CfEventSetting::GetHelpID()
{
	TRACE0 ("TODO: CfEventSetting::GetHelpID\n");
	return 0;
}

BOOL CfEventSetting::SaveMessageState()
{
	//
	// TODO:
	if (m_pMessageEntry)
		m_pMessageEntry->Output();
	return TRUE;
}


BOOL CfEventSetting::SaveUserCategory()
{
	//
	// TODO:
	ASSERT (FALSE);
	m_treeCategoryView.Output2(&m_treeCategoryView);

	return TRUE;
}


BOOL CfEventSetting::SaveSettings()
{
	//
	// User specialized folders:
	try
	{
		if (!m_pMessageEntry)
			return FALSE;
		CaCategoryDataUserManager usrFolder;
		m_treeCategoryView.GenerateUserFolder (usrFolder);
		BOOL bStoring = TRUE;
		IVM_SerializeMessageSetting(m_pMessageEntry, &usrFolder, bStoring);
		theApp.m_setting.m_messageManager.Update (*m_pMessageEntry);
	}
	catch (CArchiveException* e)
	{
		IVM_ArchiveExceptionMessage (e);
		e->Delete();
		return FALSE;
	}
	catch (...)
	{
		AfxMessageBox (_T("System error(CfEventSetting::SaveSettings): Cannot load/save the settings."));
		return FALSE;
	}
	return TRUE;
}
