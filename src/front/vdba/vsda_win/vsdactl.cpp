/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vsdactl.cpp : Implementation of the CuSdaCtrl ActiveX Control class.
**    Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Declaration of the CuSdaCtrl ActiveX Control class
**
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 17-Nov-2004 (uk$so01)
**    SIR #113475 (Add new functionality to allow user to save the results 
**    of a comparison into a CSV file.
** 30-Mar-2005 (komve01)
**    BUG #114176/Issue# 13941970
**    Added additional checks to validate the path entered by the user
**    and an appropriate message is displayed indicating the cause of failure.

*/


#include "stdafx.h"
#include "vsda.h"
#include "vsdactl.h"
#include "vsdappg.h"
#include "vsdadoc.h"
#include "usrexcep.h"
#include "viewleft.h"
#include "taskanim.h"
#include "colorind.h" // UxCreateFont
#include <io.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuSdaCtrl, COleControl)
/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CuSdaCtrl, COleControl)
	//{{AFX_MSG_MAP(CuSdaCtrl)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CuSdaCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CuSdaCtrl)
	DISP_FUNCTION(CuSdaCtrl, "SetSchema1Param", SetSchema1Param, VT_EMPTY, VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(CuSdaCtrl, "SetSchema2Param", SetSchema2Param, VT_EMPTY, VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(CuSdaCtrl, "StoreSchema", StoreSchema, VT_EMPTY, VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(CuSdaCtrl, "LoadSchema1Param", LoadSchema1Param, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CuSdaCtrl, "LoadSchema2Param", LoadSchema2Param, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CuSdaCtrl, "DoCompare", DoCompare, VT_EMPTY, VTS_I2)
	DISP_FUNCTION(CuSdaCtrl, "UpdateDisplayFilter", UpdateDisplayFilter, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CuSdaCtrl, "IsEnableDiscard", IsEnableDiscard, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CuSdaCtrl, "DiscardItem", DiscardItem, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CuSdaCtrl, "IsEnableUndiscard", IsEnableUndiscard, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CuSdaCtrl, "UndiscardItem", UndiscardItem, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CuSdaCtrl, "HideFrame", HideFrame, VT_EMPTY, VTS_I2)
	DISP_FUNCTION(CuSdaCtrl, "IsEnableAccessVdba", IsEnableAccessVdba, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CuSdaCtrl, "AccessVdba", AccessVdba, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CuSdaCtrl, "SetSessionStart", SetSessionStart, VT_EMPTY, VTS_I4)
	DISP_FUNCTION(CuSdaCtrl, "SetSessionDescription", SetSessionDescription, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CuSdaCtrl, "IsResultFrameVisible", IsResultFrameVisible, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CuSdaCtrl, "DoWriteFile", DoWriteFile, VT_EMPTY, VTS_NONE)
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CuSdaCtrl, COleControl)
	//{{AFX_EVENT_MAP(CuSdaCtrl)
	// NOTE - ClassWizard will add and remove event map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CuSdaCtrl, 1)
	PROPPAGEID(CVsdaPropPage::guid)
END_PROPPAGEIDS(CuSdaCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CuSdaCtrl, "VSDA.VsdaCtrl.1",
	0xcc2da2b6, 0xb8f1, 0x11d6, 0x87, 0xd8, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CuSdaCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DVsda =
		{ 0xcc2da2b4, 0xb8f1, 0x11d6, { 0x87, 0xd8, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a } };
const IID BASED_CODE IID_DVsdaEvents =
		{ 0xcc2da2b5, 0xb8f1, 0x11d6, { 0x87, 0xd8, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwVsdaOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CuSdaCtrl, IDS_VSDA, _dwVsdaOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CuSdaCtrl::CuSdaCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CuSdaCtrl

BOOL CuSdaCtrl::CuSdaCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegApartmentThreading to 0.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_VSDA,
			IDB_VSDA,
			afxRegApartmentThreading,
			_dwVsdaOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CuSdaCtrl::CuSdaCtrl - Constructor

CuSdaCtrl::CuSdaCtrl()
{
	InitializeIIDs(&IID_DVsda, &IID_DVsdaEvents);

	// TODO: Initialize your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CuSdaCtrl::~CuSdaCtrl - Destructor

CuSdaCtrl::~CuSdaCtrl()
{
	// TODO: Cleanup your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CuSdaCtrl::OnDraw - Drawing function

void CuSdaCtrl::OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	if (m_pVsdFrame && m_pVsdFrame->IsWindowVisible())
	{
		m_pVsdFrame->MoveWindow(rcBounds);
	}
	else
	{
		CRect r = rcBounds;
		pdc->FillRect(rcBounds, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
		CFont font;
		if (!UxCreateFont (font, _T("Arial"), 20))
			return;

		CString strMsg;
		strMsg.LoadString(IDS_INVITATION);
		CFont* pOldFond = pdc->SelectObject (&font);
		COLORREF colorTextOld   = pdc->SetTextColor (RGB (0, 0, 255));
		COLORREF oldTextBkColor = pdc->SetBkColor (GetSysColor (COLOR_WINDOW));
		CSize txSize= pdc->GetTextExtent (strMsg, strMsg.GetLength());
		int iPrevMode = pdc->SetBkMode (TRANSPARENT);
		pdc->DrawText (strMsg, r, DT_CENTER|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
		pdc->SetBkMode (iPrevMode);
		pdc->SelectObject (pOldFond);
		pdc->SetBkColor (oldTextBkColor);
		pdc->SetTextColor (colorTextOld);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CuSdaCtrl::DoPropExchange - Persistence support

void CuSdaCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.

}


/////////////////////////////////////////////////////////////////////////////
// CuSdaCtrl::OnResetState - Reset control to default state

void CuSdaCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CuSdaCtrl message handlers

int CuSdaCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CWnd* pParent = this;
	CRect r;
	GetClientRect (r);
	m_pVsdFrame = new CfSdaFrame();
	m_pVsdFrame->Create (
		NULL,
		NULL,
		WS_CHILD,
		r,
		pParent);
	if (!m_pVsdFrame)
		return FALSE;

	m_pVsdFrame->InitialUpdateFrame(NULL, TRUE);
	m_pVsdFrame->ShowWindow(SW_HIDE);
	return 0;
}

void CuSdaCtrl::SetSchema1Param(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszUser) 
{
	ASSERT(m_pVsdFrame);
	if (!m_pVsdFrame)
		return;
	CdSdaDoc* pDoc = m_pVsdFrame->GetSdaDoc();
	ASSERT(pDoc);
	if (pDoc)
	{
		pDoc->SetSchema1Param(lpszNode, lpszDatabase, lpszUser);
	}
}

void CuSdaCtrl::SetSchema2Param(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszUser) 
{
	ASSERT(m_pVsdFrame);
	if (!m_pVsdFrame)
		return;
	CdSdaDoc* pDoc = m_pVsdFrame->GetSdaDoc();
	ASSERT(pDoc);
	if (pDoc)
	{
		pDoc->SetSchema2Param(lpszNode, lpszDatabase, lpszUser);
	}
}


void CuSdaCtrl::StoreSchema(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszUser, LPCTSTR lpszFile) 
{
	CaVsdStoreSchema* pStoreSchema = new CaVsdStoreSchema(&(theApp.m_sessionManager), lpszFile, lpszNode, lpszDatabase, lpszUser);
	pStoreSchema->SetFetchObjects(CaLLQueryInfo::FETCH_USER);
	CaExecParamInstallation* p = new CaExecParamInstallation(pStoreSchema);
	CString strDirException;
	strDirException.Empty();
	CString strDir = lpszFile;
	strDir = strDir.Left(strDir.ReverseFind('\\'));
	if(_access(strDir.GetBuffer(strDir.GetLength()),0) == -1)
	{
		//Invalid directory
		CFileException e;
		e.m_cause = CFileException::badPath;
		MSGBOX_FileExceptionMessage (&e, strDirException);
	}

	CString strMsgAnimateTitle = _T("Saving...");
	if (pStoreSchema->GetDatabase().IsEmpty())
		strMsgAnimateTitle.LoadString(IDS_TITLE_STORE_INSTALLATION);
	else
		strMsgAnimateTitle.LoadString(IDS_TITLE_STORE_DATABASE_SCHEMA);
#if defined (_ANIMATION_)
	CxDlgWait dlg (strMsgAnimateTitle);
	dlg.SetUseAnimateAvi(AVI_FETCHF);
	dlg.SetExecParam (p);
	dlg.SetDeleteParam(FALSE);
	dlg.SetShowProgressBar(FALSE);
	dlg.SetUseExtraInfo();
	dlg.SetHideCancelBottonAlways(TRUE);
	dlg.DoModal();
#else
	p->Run();
#endif
	CString strError = p->GetFailMessage();
	if(!strDirException.IsEmpty())
	{
		strError += consttchszReturn;
		strError += strDirException;
	}
	delete p;

	if (!strError.IsEmpty())
	{
		AfxMessageBox (strError);
	}
}

void CuSdaCtrl::LoadSchema1Param(LPCTSTR lpszFile) 
{
	ASSERT(m_pVsdFrame);
	if (!m_pVsdFrame)
		return;
	CdSdaDoc* pDoc = m_pVsdFrame->GetSdaDoc();
	ASSERT(pDoc);
	if (pDoc)
	{
		pDoc->LoadSchema1Param(lpszFile);
	}
}

void CuSdaCtrl::LoadSchema2Param(LPCTSTR lpszFile) 
{
	ASSERT(m_pVsdFrame);
	if (!m_pVsdFrame)
		return;
	CdSdaDoc* pDoc = m_pVsdFrame->GetSdaDoc();
	ASSERT(pDoc);
	if (pDoc)
	{
		pDoc->LoadSchema2Param(lpszFile);
	}
}

void CuSdaCtrl::DoCompare(short nMode) 
{
	CWaitCursor doWaitCursor;
	ASSERT(m_pVsdFrame);
	if (!m_pVsdFrame)
		return;
	m_pVsdFrame->DoCompare(nMode);
}

void CuSdaCtrl::UpdateDisplayFilter(LPCTSTR lpszFilter) 
{
	CWaitCursor doWaitCursor;
	ASSERT(m_pVsdFrame);
	if (!m_pVsdFrame)
		return;
	CWnd* pLeftPane = m_pVsdFrame->GetLeftPane();
	if (pLeftPane)
	{
		CvSdaLeft* pViewLeft = (CvSdaLeft*)pLeftPane;
		pViewLeft->UpdateDisplayFilter(lpszFilter);
	}
}

BOOL CuSdaCtrl::IsEnableDiscard() 
{
	BOOL bEnable = FALSE;
	ASSERT(m_pVsdFrame);
	if (!m_pVsdFrame)
		return bEnable;
	CWnd* pLeftPane = m_pVsdFrame->GetLeftPane();
	if (pLeftPane)
	{
		CvSdaLeft* pViewLeft = (CvSdaLeft*)pLeftPane;
		bEnable = pViewLeft->IsEnableDiscard();
	}

	return bEnable;
}

void CuSdaCtrl::DiscardItem() 
{
	ASSERT(m_pVsdFrame);
	if (!m_pVsdFrame)
		return;
	CWnd* pLeftPane = m_pVsdFrame->GetLeftPane();
	if (pLeftPane)
	{
		CvSdaLeft* pViewLeft = (CvSdaLeft*)pLeftPane;
		pViewLeft->DiscardItem();
	}
}

BOOL CuSdaCtrl::IsEnableUndiscard() 
{
	BOOL bEnable = FALSE;
	ASSERT(m_pVsdFrame);
	if (!m_pVsdFrame)
		return bEnable;
	CWnd* pLeftPane = m_pVsdFrame->GetLeftPane();
	if (pLeftPane)
	{
		CvSdaLeft* pViewLeft = (CvSdaLeft*)pLeftPane;
		bEnable = pViewLeft->IsEnableUndiscard();
	}

	return bEnable;
}

void CuSdaCtrl::UndiscardItem() 
{
	ASSERT(m_pVsdFrame);
	if (!m_pVsdFrame)
		return;
	CWnd* pLeftPane = m_pVsdFrame->GetLeftPane();
	if (pLeftPane)
	{
		CvSdaLeft* pViewLeft = (CvSdaLeft*)pLeftPane;
		pViewLeft->UndiscardItem();
	}
}

void CuSdaCtrl::HideFrame(short nShow) 
{
	if (!m_pVsdFrame)
		return;
	if (nShow == 1)
		m_pVsdFrame->ShowWindow(SW_HIDE);
	else
		m_pVsdFrame->ShowWindow(SW_SHOW);
}

BOOL CuSdaCtrl::IsEnableAccessVdba() 
{
	BOOL bEnable = FALSE;
	ASSERT(m_pVsdFrame);
	if (!m_pVsdFrame)
		return bEnable;
	CWnd* pLeftPane = m_pVsdFrame->GetLeftPane();
	if (pLeftPane)
	{
		CvSdaLeft* pViewLeft = (CvSdaLeft*)pLeftPane;
		bEnable = pViewLeft->IsEnableAccessVdba();
	}

	return bEnable;
}

void CuSdaCtrl::AccessVdba() 
{
	ASSERT(m_pVsdFrame);
	if (!m_pVsdFrame)
		return;
	CWnd* pLeftPane = m_pVsdFrame->GetLeftPane();
	if (pLeftPane)
	{
		CvSdaLeft* pViewLeft = (CvSdaLeft*)pLeftPane;
		pViewLeft->AccessVdba();
	}
}

void CuSdaCtrl::SetSessionStart(long nStart) 
{
	int nCurrStart = theApp.m_sessionManager.GetSessionStart();
	ASSERT(nCurrStart == 1); // you can set only one time
	if (nCurrStart == 1)
	{
		theApp.m_sessionManager.SetSessionStart(nStart);
	}
}

void CuSdaCtrl::SetSessionDescription(LPCTSTR lpszDescription) 
{
	theApp.m_sessionManager.SetDescription(lpszDescription);
}

BOOL CuSdaCtrl::IsResultFrameVisible() 
{
	if (m_pVsdFrame && m_pVsdFrame->IsWindowVisible())
		return TRUE;
	return FALSE;
}

void CuSdaCtrl::DoWriteFile() 
{
	if (m_pVsdFrame && m_pVsdFrame->IsWindowVisible())
		m_pVsdFrame->DoWriteFile();
}

