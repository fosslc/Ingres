/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vcdactl.cpp : Implementation of the CuVcdaCtrl ActiveX Control class
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Ocx control interfaces 
**
** History:
**
** 02-Oct-2002 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
** 06-Jan-2004 (schph01)
**    SIR #111462, Put string into resource files
** 30-Dec-2004 (komve01)
**    BUG #113714, Issue #13858925,
**    Set m_bComparedCurrInstall to TRUE when Current Installation is 
**    compared, else FALSE.
*/

#include "stdafx.h"
#include "vcda.h"
#include "vcdactl.h"
#include "vcdappg.h"
#include "udlgmain.h"
#include "vcdadml.h"
#include "colorind.h" // UxCreateFont
#include "vnodedml.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuVcdaCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CuVcdaCtrl, COleControl)
	//{{AFX_MSG_MAP(CuVcdaCtrl)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CuVcdaCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CuVcdaCtrl)
	DISP_FUNCTION(CuVcdaCtrl, "SaveInstallation", SaveInstallation, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CuVcdaCtrl, "SetCompareFile", SetCompareFile, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CuVcdaCtrl, "SetCompareFiles", SetCompareFiles, VT_EMPTY, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(CuVcdaCtrl, "Compare", Compare, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CuVcdaCtrl, "HideFrame", HideFrame, VT_EMPTY, VTS_I2)
	DISP_FUNCTION(CuVcdaCtrl, "SetInvitationText", SetInvitationText, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CuVcdaCtrl, "RestoreInstallation", RestoreInstallation, VT_EMPTY, VTS_NONE)
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CuVcdaCtrl, COleControl)
	//{{AFX_EVENT_MAP(CuVcdaCtrl)
	// NOTE - ClassWizard will add and remove event map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CuVcdaCtrl, 1)
	PROPPAGEID(CuVcdaPropPage::guid)
END_PROPPAGEIDS(CuVcdaCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CuVcdaCtrl, "VCDA.VcdaCtrl.1",
	0xeaf345ea, 0xd514, 0x11d6, 0x87, 0xea, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CuVcdaCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DVcda =      { 0xeaf345e8, 0xd514, 0x11d6, { 0x87, 0xea, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a } };
const IID BASED_CODE IID_DVcdaEvents ={ 0xeaf345e9, 0xd514, 0x11d6, { 0x87, 0xea, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwVcdaOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CuVcdaCtrl, IDS_VCDA, _dwVcdaOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CuVcdaCtrl::CuVcdaCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CuVcdaCtrl

BOOL CuVcdaCtrl::CuVcdaCtrlFactory::UpdateRegistry(BOOL bRegister)
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
			IDS_VCDA,
			IDB_VCDA,
			afxRegApartmentThreading,
			_dwVcdaOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CuVcdaCtrl::CuVcdaCtrl - Constructor

CuVcdaCtrl::CuVcdaCtrl()
{
	InitializeIIDs(&IID_DVcda, &IID_DVcdaEvents);
	m_pDlgMain = NULL;
	m_nMode = -1;
	m_strFile1 = _T("");
	m_strFile2 = _T("");
	m_strInvitation = _T("");
	m_bComparedCurrInstall = FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CuVcdaCtrl::~CuVcdaCtrl - Destructor

CuVcdaCtrl::~CuVcdaCtrl()
{
	// TODO: Cleanup your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CuVcdaCtrl::OnDraw - Drawing function

void CuVcdaCtrl::OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	if (m_pDlgMain && m_pDlgMain->IsWindowVisible())
	{
		m_pDlgMain->MoveWindow(rcBounds);
	}
	else
	{
		CRect r = rcBounds;
		pdc->FillRect(rcBounds, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
		CFont font;
		if (!UxCreateFont (font, _T("Arial"), 20))
			return;

		CString strMsg;
		if (m_strInvitation.IsEmpty())
			strMsg.LoadString(IDS_INVITATION);
		else
			strMsg = m_strInvitation;
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
// CuVcdaCtrl::DoPropExchange - Persistence support

void CuVcdaCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.

}


/////////////////////////////////////////////////////////////////////////////
// CuVcdaCtrl::OnResetState - Reset control to default state

void CuVcdaCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CuVcdaCtrl message handlers

int CuVcdaCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CWnd* pParent = this;
	CRect r;
	GetClientRect (r);
	m_pDlgMain = new CuDlgMain();
	m_pDlgMain->Create (IDD_MAIN, this);
	return 0;
}

void CuVcdaCtrl::SaveInstallation(LPCTSTR lpszFile) 
{
	try 
	{
		VCDA_SaveInstallation(lpszFile, m_pDlgMain->GetIngresVariable());
	}
	catch (...)
	{
		DeleteFile(lpszFile);
	}
}

void CuVcdaCtrl::SetCompareFile(LPCTSTR lpszFile) 
{
	m_nMode = 0;
	m_strFile1 = lpszFile;
}

void CuVcdaCtrl::SetCompareFiles(LPCTSTR lpszFile1, LPCTSTR lpszFile2) 
{
	m_nMode = 1;
	m_strFile1 = lpszFile1;
	m_strFile2 = lpszFile2;
}

void CuVcdaCtrl::Compare() 
{
	CWaitCursor doWaitCursor;
	BOOL bOK = FALSE;
	ASSERT (m_nMode == 0 || m_nMode == 1);
	if (m_nMode == 0 && !m_strFile1.IsEmpty())
	{
		//
		// Compare current installation with a given snap shot file:
		bOK = VCDA_Compare (m_pDlgMain, m_strFile1, NULL);
		m_bComparedCurrInstall = TRUE;
	}
	else
	if (m_nMode == 1 && !m_strFile1.IsEmpty() && !m_strFile2.IsEmpty())
	{
		//
		// Compare two snap shot files:
		bOK = VCDA_Compare (m_pDlgMain, m_strFile1, m_strFile2);
		m_bComparedCurrInstall = FALSE;
	}
	if (bOK)
		HideFrame(1);
}

void CuVcdaCtrl::HideFrame(short nShow) 
{
	int nMode = (nShow==0)? SW_HIDE: SW_SHOW;
	m_pDlgMain->HideFrame(nMode);
}

void CuVcdaCtrl::SetInvitationText(LPCTSTR lpszText) 
{
	m_strInvitation = lpszText;
}

void CuVcdaCtrl::RestoreInstallation() 
{
	try 
	{
		BOOL bRun = INGRESII_IsRunning();
		if (bRun)
		{
			//
			// The restore operation cannot be performed while Ingres is running.
			AfxMessageBox (IDS_MSG_CANNOTRESTORExINGRESRUNNING);
			return;
		}
		if (!m_pDlgMain->IsWindowVisible() || !m_bComparedCurrInstall)
		{
			AfxMessageBox (IDS_MSG_COMPARE_BEFORE);
			return;
		}

		m_pDlgMain->OnRestore();
	}
	catch (...)
	{
	}
}
