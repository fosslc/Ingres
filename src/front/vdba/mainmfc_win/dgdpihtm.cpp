/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : dgdpihtm.cpp, Implementation File 
**
**
**  Project  : INGRES II / VDBA.                
**  Author   : UK Sotheavut (uk$so01)          
**
**  Purpose  : The Right pane (for B-Unit page and facet)          
**             It contains the HTML source or and image .GIF      
**
**  History :
**	04-Jun-2002 (hanje04)
**	    Define tchszReturn to be 0x0a 0x00 for MAINWIN builds to stop
**	    unwanted ^M's in generated text files.
**	27-aug-2002 (somsa01)
**	    Corrected previous cross-integration.
**	02-feb-2004 (somsa01)
**	    Updated to build with .NET 2003 compiler.
**      19-Mar-2009 (drivi01)
**          In effort to port to Visual Studio 2008, move
**          fstream includes to stdafx.h.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgdpihtm.h"
#include "vtree.h"      // FILTER_DOM_XYZ


extern "C" 
{
#include "dba.h"
#include "domdata.h"
#include "main.h"
#include "dom.h"
#include "tree.h"
#include "dbaset.h"
#include "ice.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
BOOL VDBA_IsOpen (ifstream& f);


CuDlgDomPropIceHtml::CuDlgDomPropIceHtml(int nMode, CWnd* pParent /*=NULL*/)
	: CDialog((nMode == 0)? IDD_DOMPROP_ICE_HTML_SOURCE: IDD_DOMPROP_ICE_HTML_IMAGE, pParent)
{
	m_nMode = nMode;
	m_nIDD = (nMode == 0)? IDD_DOMPROP_ICE_HTML_SOURCE: IDD_DOMPROP_ICE_HTML_IMAGE;
	#if !defined (DOMPROP_HTML_SIMULATE)
	m_Data.m_refreshParams.InitializeRefreshParamsType(OT_ICE_GENERIC);
	#endif
	//{{AFX_DATA_INIT(CuDlgDomPropIceHtml)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgDomPropIceHtml::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropIceHtml)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropIceHtml, CDialog)
	//{{AFX_MSG_MAP(CuDlgDomPropIceHtml)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
	ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceHtml message handlers

void CuDlgDomPropIceHtml::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropIceHtml::OnInitDialog() 
{
	CDialog::OnInitDialog();
	if (m_nMode == 0)
	{
		CEdit* pEdit = (CEdit*)GetDlgItem (IDC_MFC_EDIT1);
		if (pEdit && IsWindow (pEdit->m_hWnd))
			pEdit->LimitText();
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropIceHtml::OnDestroy() 
{
	CDialog::OnDestroy();
}

void CuDlgDomPropIceHtml::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	CRect r;
	GetClientRect (r);
	if (m_nMode == 0)
	{
		CEdit* pEdit = (CEdit*)GetDlgItem (IDC_MFC_EDIT1);
		if (pEdit && IsWindow (pEdit->m_hWnd))
		{
			pEdit->MoveWindow (r);
		}
	}
	else
	if (m_nMode == 1)
	{
		CuWebBrowser2* pIe = (CuWebBrowser2*)GetDlgItem (IDC_EXPLORER1);
		if (pIe && IsWindow (pIe->m_hWnd))
		{
			pIe->MoveWindow (r);
		}
	}
	else
	{
		ASSERT (FALSE);
	}
}

LONG CuDlgDomPropIceHtml::OnQueryType(WPARAM wParam, LPARAM lParam)
{
	ASSERT (wParam == 0);
	ASSERT (lParam == 0);
	return DOMPAGE_DETAIL; // ? because it is not a list 
}

LONG CuDlgDomPropIceHtml::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
#if !defined (DOMPROP_HTML_SIMULATE)
	int nNodeHandle = (int)wParam;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	ASSERT (nNodeHandle != -1);
	ASSERT (pUps);

	switch (pUps->nIpmHint)
	{
	case 0:
		break;

	case FILTER_DOM_BKREFRESH_DETAIL:
		if (m_Data.m_refreshParams.MustRefresh(pUps->pSFilter->bOnLoad, pUps->pSFilter->refreshtime))
			break;    // need to update
		else
			return 0; // no need to update
		break;
	default:
		return 0L;    // nothing to change on the display
	}
#endif
	ResetDisplay();

#if !defined (DOMPROP_HTML_SIMULATE)
	//
	// Get info on the object
	ICEBUSUNITDOCDATA IceFacetAndPageParams;
	memset (&IceFacetAndPageParams, 0, sizeof (IceFacetAndPageParams));

	LPTREERECORD lpRecord = (LPTREERECORD)pUps->pStruct;
	ASSERT (lpRecord);

	//
	// Get ICE Detail Info
	//
	int objType = lpRecord->recType;
	ASSERT (objType == OT_ICE_BUNIT_FACET || objType == OT_ICE_BUNIT_PAGE);
	m_Data.m_objType = objType;   // MANDATORY!

	//
	// prepare parenthood: name of business unit
	lstrcpy ((LPTSTR)IceFacetAndPageParams.icebusunit.Name, (LPCTSTR)lpRecord->extra);

	CString csName = (LPCTSTR)lpRecord->objName;
	int separator = csName.Find(_T('.'));
	ASSERT (separator != -1);     // FNN VA ENVOYER NAME.SUFFIX - PAS ENCORE PRÊT!
	ASSERT (separator > 0);
	if (separator > 0) {
		lstrcpy ((LPTSTR)IceFacetAndPageParams.name, (LPCTSTR)csName.Left(separator));
		lstrcpy ((LPTSTR)IceFacetAndPageParams.suffix, (LPCTSTR)csName.Mid(separator+1));
	}
	else {
		lstrcpy ((LPTSTR)IceFacetAndPageParams.name, (LPCTSTR)lpRecord->objName);
		IceFacetAndPageParams.suffix[0] = '\0';
	}

	m_Data.ClearTempFileIfAny(); // removes previous temp file if any

	char bufFileTmp[_MAX_PATH];
	int iResult = ICE_FillTempFileFromDoc((LPUCHAR)GetVirtNodeName(nNodeHandle),
											&IceFacetAndPageParams,
											(LPUCHAR)bufFileTmp);
#if 0
	x_strcpy (bufFileTmp,_T("C:\\MISC\\UNIX\\REPORT.HTML"));
	int iResult = RES_SUCCESS;
#endif
	if (iResult != RES_SUCCESS) {
		ASSERT (FALSE);

		// Reset m_Data
		CuDomPropDataIceFacetAndPage tempData;
		tempData.m_refreshParams = m_Data.m_refreshParams;
		m_Data = tempData;

		// Refresh display
		RefreshDisplay();

		return 0L;
	}
	m_Data.m_csTempFile = (LPCTSTR)bufFileTmp;
	// Update refresh info
	m_Data.m_refreshParams.UpdateRefreshParams();

	//
	// update member variables, for display/load/save purpose
	//

	// Common info
//	m_Data.m_csNamePlusExt = IceFacetAndPageParams.name;
//	m_Data.m_csNamePlusExt += _T('.');
//	m_Data.m_csNamePlusExt += IceFacetAndPageParams.suffix;
//	m_Data.m_bPublic = IceFacetAndPageParams.bPublic;
//
//	// Storage type - criterion has changed!
//	if (IceFacetAndPageParams.bpre_cache ||
////////		  IceFacetAndPageParams.bperm_cache ||
//		  IceFacetAndPageParams.bsession_cache)
//		m_Data.m_bStorageInsideRepository = TRUE;
//	else
//		m_Data.m_bStorageInsideRepository = FALSE;
//
//	// specific for Inside repository
//	if (m_Data.m_bStorageInsideRepository) {
//		m_Data.m_bPreCache = m_Data.m_bPermCache = m_Data.m_bSessionCache = FALSE;
//
//		if (IceFacetAndPageParams.bpre_cache)
//		  m_Data.m_bPreCache = TRUE;
//		else if (IceFacetAndPageParams.bperm_cache)
//		  m_Data.m_bPermCache = TRUE;
////		else if (IceFacetAndPageParams.bsession_cache)
//		  m_Data.m_bSessionCache = TRUE;
//
//		m_Data.m_csPath = IceFacetAndPageParams.doc_file;
//	}
//
//	// specific for File system
//	if (!m_Data.m_bStorageInsideRepository) {
//		m_Data.m_csLocation = IceFacetAndPageParams.ext_loc.LocationName;
//
//		m_Data.m_csFilename = IceFacetAndPageParams.ext_file;
//		m_Data.m_csFilename += _T('.');
//		m_Data.m_csFilename += IceFacetAndPageParams.ext_suffix;
//	}
#endif
	//
	// Refresh display
	RefreshDisplay();

	return 0L;
}

LONG CuDlgDomPropIceHtml::OnLoad (WPARAM wParam, LPARAM lParam)
{
#if !defined (DOMPROP_HTML_SIMULATE)
	// Precheck consistency of class name
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, "CuDomPropDataIceFacetAndPage") == 0);
	ResetDisplay();

	// Get class pointer, and check for it's class runtime type
	CuDomPropDataIceFacetAndPage* pData = (CuDomPropDataIceFacetAndPage*)lParam;
	ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataIceFacetAndPage)));

	// Update current m_Data using operator = overloading
	m_Data = *pData;

	// Refresh display
	RefreshDisplay();
#endif
	return 0L;
}

LONG CuDlgDomPropIceHtml::OnGetData (WPARAM wParam, LPARAM lParam)
{
#if !defined (DOMPROP_HTML_SIMULATE)
	// Allocate data class instance WITH NEW, and
	// DUPLICATE current m_Data in the allocated instance using copy constructor
	CuDomPropDataIceFacetAndPage* pData = new CuDomPropDataIceFacetAndPage(m_Data);

	// return it WITHOUT FREEING - will be freed by caller
	return (LRESULT)pData;
#endif
	return 0;
}

void CuDlgDomPropIceHtml::RefreshDisplay()
{
	try
	{
		if (m_nMode == 0) // HTML Source
		{
			//
			// Construct your CString to point to the appropriate .html file
			// according to the m_Data. The full path must be given !!
			CString strFile = m_Data.m_csTempFile;
 
			CEdit* pEdit = (CEdit*)GetDlgItem (IDC_MFC_EDIT1);
			if (pEdit && IsWindow (pEdit->m_hWnd))
			{
				TCHAR tchszBuff [1024];
#ifdef MAINWIN
				TCHAR tchszReturn [] = {0x0a, 0x00};
#else
				TCHAR tchszReturn [] = {0x0d, 0x0a, 0x00};
#endif
				CString strText = _T("");
				int nLen;

				pEdit->SetWindowText (_T(""));
				ifstream fx (strFile, ios::in);
				if (!VDBA_IsOpen(fx))
					return;

				while (fx)
				{
					fx.getline(tchszBuff, 1023);
					strText = tchszBuff;
					strText+= tchszReturn;
					nLen = pEdit->GetWindowTextLength();
					pEdit->SetSel (nLen, nLen);
					pEdit->ReplaceSel (strText);
				}
			}
		}
		else
		if (m_nMode == 1) // HTML Gif file
		{
			//
			// Construct your CString to point to the appropriate gif file
			// according to the m_Data. The full path must be given !!
			CString strFile = m_Data.m_csTempFile;
			CuWebBrowser2* pIe = (CuWebBrowser2*)GetDlgItem (IDC_EXPLORER1);
			if (pIe && IsWindow (pIe->m_hWnd))
			{
				pIe->ShowWindow(SW_SHOW);
				pIe->Navigate (strFile, NULL, NULL, NULL, NULL);
			}
		}
		else
		{
			ASSERT (FALSE);
		}
	}
	catch (...)
	{
		CString strMsg = VDBA_MfcResourceString(IDS_E_DISPLAY_DATA);//_T("Cannot display data");
		AfxMessageBox (strMsg);
	}
}

void CuDlgDomPropIceHtml::ResetDisplay()
{
	if (m_nMode == 0)
	{
		CEdit* pEdit = (CEdit*)GetDlgItem (IDC_MFC_EDIT1);
		if (pEdit && IsWindow (pEdit->m_hWnd))
		{
			pEdit->SetWindowText (_T(""));
		}
	}
	else
	if (m_nMode == 1)
	{
	}
	else
	{
		ASSERT (FALSE);
	}
}

BOOL VDBA_IsOpen (ifstream& f)
{
	#if defined (MAINWIN)
	return (f)? TRUE: FALSE;
	#else
	return f.is_open();
	#endif
}
