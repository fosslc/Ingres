/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : mainfrm.cpp : implementation of the CfMainFrame class
**    Project  : Ingres II / IIA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Parse the arguments
**
** History:
**
** 07-Feb-2001 (uk$so01)
**    Created
** 26-Sep-2001 (somsa01)
**    For MAINWIN, minimize the main window as it is not done intrinsically
**    as it is done on Windows.
** 18-Mar-2002 (uk$so01)
**    Use the correct character mapping.
** 26-Feb-2003 (uk$so01)
**    SIR  #106952, enhance library + cleanup.
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 21-Aug-2008 (whiro01)
**    Moved <afx...> include to "stdafx.h"
*/


#include "stdafx.h"
#include "iia.h"
#include "mainfrm.h"

#include "libguids.h" // guids
#include "impexpas.h" // import assistant interface
#include "usrexcep.h"
#include "ingobdml.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define WMUSR_CALLFUNCTION (WM_USER + 500)


IMPLEMENT_DYNAMIC(CfMainFrame, CFrameWnd)
BEGIN_MESSAGE_MAP(CfMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CfMainFrame)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSR_CALLFUNCTION, OnImportAssistant)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CfMainFrame construction/destruction

CfMainFrame::CfMainFrame()
{
}

CfMainFrame::~CfMainFrame()
{
}

int CfMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

#ifdef MAINWIN
	AfxGetMainWnd()->ShowWindow(SW_SHOWMINIMIZED);
#endif  /* MAINWIN */

	//
	// create a view to occupy the client area of the frame
	if (!m_wndView.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW,
		CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL))
	{
		TRACE0("Failed to create view window\n");
		return -1;
	}

	SetWindowPos (&wndBottom, -100, -100, 0, 0, SWP_SHOWWINDOW);
	PostMessage(WMUSR_CALLFUNCTION, 0, 0);

	return 0;
}

BOOL CfMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CfMainFrame diagnostics

#ifdef _DEBUG
void CfMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CfMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG






void CfMainFrame::OnSetFocus(CWnd* pOldWnd)
{
	//
	// forward focus to the view window
	m_wndView.SetFocus();
}

BOOL CfMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// let the view have first crack at the command
	if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// otherwise, do default handling
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}


static BOOL GetInterfacePointer(IUnknown* pObj, REFIID riid, PVOID* ppv)
{
	BOOL bResult = FALSE;
	HRESULT hError;

	*ppv=NULL;

	if (NULL != pObj)
	{
		hError = pObj->QueryInterface(riid, ppv);
	}
	return FAILED(hError)? FALSE: TRUE;
}

long CfMainFrame::OnImportAssistant(WPARAM w, LPARAM l)
{
	USES_CONVERSION;
	CString strMsg;
	HRESULT hError = NOERROR;

	try
	{
		//
		// Check host name and iisystem:
		CString strFile = theApp.m_cmdLine.m_strIIImportFile;
		if (!strFile.IsEmpty())
		{
			//
			// Check file extension:
			BOOL bFileOK = TRUE;
			int nPos = strFile.ReverseFind(_T('.'));
			if (nPos != -1)
			{
				CString strExt = strFile.Mid(nPos+1);
				if (strExt.CompareNoCase(_T("ii_imp")) != 0)
					bFileOK = FALSE;
			}
			else
				bFileOK = FALSE;
			if (!bFileOK)
			{
				AfxMessageBox (IDS_MSG_WRONG_PARAMETER_FILE);
				theApp.m_cmdLine.SetNoParam();
			}

			CFile f((LPCTSTR)strFile, CFile::shareDenyNone|CFile::modeRead);
			CArchive ar(&f, CArchive::load);
			CString strHostName;
			CString strIISystem;
			ar >> strHostName;
			ar >> strIISystem;
			TRACE1("Check host name in import parameter: %s\n", (LPCTSTR)strHostName);
			TRACE1("Check II_SYSTEM in import parameter: %s\n", (LPCTSTR)strIISystem);

			TCHAR* pEnv;
			pEnv = _tgetenv(_T("II_SYSTEM"));
			CString strThisHost = GetLocalHostName();
			if (!strHostName.IsEmpty() && strThisHost.CompareNoCase(strHostName) == 0 && pEnv)
			{
				if (strIISystem.CompareNoCase(pEnv) != 0)
				{
					//
					// Set the II_SYSTEM from import parameter file:
					SetEnvironmentVariable(_T("II_SYSTEM"), strIISystem);
				}
			}

			ar.Close();
			f.Close();
		}

		hError = CoInitialize(NULL);
		if (FAILED(hError))
		{
			strMsg = _T("Initialize COM Library failed");
			AfxMessageBox (strMsg);
		}
		else
		{
			IUnknown*   pAptImportAssistant = NULL;
			IImportAssistant* pIia;

			hError = CoCreateInstance(
				CLSID_IMPxEXPxASSISTANCT,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_IUnknown,
				(PVOID*)&pAptImportAssistant);

			if (SUCCEEDED(hError))
			{
				BOOL bOk = GetInterfacePointer(pAptImportAssistant, IID_IImportAssistant, (LPVOID*)&pIia);
				if (bOk)
				{
					if (theApp.m_cmdLine.GetStatus() && !theApp.m_cmdLine.HasCmdLine())
					{
						pIia->ImportAssistant (m_hWnd);
						pIia->Release();
					}
					else
					{
						const int nBuffLen = 64;
						BOOL bLenOK = TRUE;
						IIASTRUCT iiparam;
						memset (&iiparam, 0, sizeof(iiparam));
						iiparam.bLoop    = theApp.m_cmdLine.m_bLoop;
						iiparam.bBatch   = theApp.m_cmdLine.m_bBatch;
						iiparam.bNewTable= theApp.m_cmdLine.m_bNewTable;
						wcscpy (iiparam.wczSessionDescription, L"Ingres Import Assistant");

						// NODE NAME:
						if (bLenOK && theApp.m_cmdLine.m_strNode.GetLength() < nBuffLen)
							wcscpy (iiparam.wczNode, T2W((LPTSTR)(LPCTSTR)theApp.m_cmdLine.m_strNode));
						else
							bLenOK = FALSE;
						// SERVER CLASS:
						if (bLenOK && theApp.m_cmdLine.m_strServerClass.GetLength() < nBuffLen)
							wcscpy (iiparam.wczServerClass, T2W((LPTSTR)(LPCTSTR)theApp.m_cmdLine.m_strServerClass));
						else
							bLenOK = FALSE;

						// DATABASE NAME:
						if (bLenOK && theApp.m_cmdLine.m_strDatabase.GetLength() < nBuffLen)
							wcscpy (iiparam.wczDatabase, T2W((LPTSTR)(LPCTSTR)theApp.m_cmdLine.m_strDatabase));
						else
							bLenOK = FALSE;
						// TABLE NAME:
						if (bLenOK && theApp.m_cmdLine.m_strTable.GetLength() < nBuffLen)
							wcscpy (iiparam.wczTable, T2W((LPTSTR)(LPCTSTR)theApp.m_cmdLine.m_strTable));
						else
							bLenOK = FALSE;
						// TABLE OWNER:
						if (bLenOK && theApp.m_cmdLine.m_strTableOwner.GetLength() < nBuffLen)
							wcscpy (iiparam.wczTableOwner, T2W((LPTSTR)(LPCTSTR)theApp.m_cmdLine.m_strTableOwner));
						else
							bLenOK = FALSE;
						// NEW TABLE:
						if (bLenOK && theApp.m_cmdLine.m_strNewTable.GetLength() < nBuffLen)
							wcscpy (iiparam.wczNewTable, T2W((LPTSTR)(LPCTSTR)theApp.m_cmdLine.m_strNewTable));
						else
							bLenOK = FALSE;

						// IMPORT PARAMETER FILE:
						if (bLenOK && theApp.m_cmdLine.m_strIIImportFile.GetLength() < _MAX_PATH)
							wcscpy (iiparam.wczParamFile, T2W((LPTSTR)(LPCTSTR)theApp.m_cmdLine.m_strIIImportFile));
						else
							bLenOK = FALSE;
						// LOG FILE:
						if (bLenOK && theApp.m_cmdLine.m_strLogFile.GetLength() < _MAX_PATH)
							wcscpy (iiparam.wczLogFile, T2W((LPTSTR)(LPCTSTR)theApp.m_cmdLine.m_strLogFile));
						else
							bLenOK = FALSE;
						
						if (bLenOK)
							pIia->ImportAssistant2 (m_hWnd, &iiparam);
						else
						{
							strMsg = _T("Internal Error: Object length exceeds the buffer length.\nPlease see the structure  IIASTRUCT.");
							AfxMessageBox (strMsg);
						}

						pIia->Release();
					}
				}
				else
				{
					strMsg = _T("Fail to query interface of Import Assistance");
					AfxMessageBox (strMsg);
				}
				
				pAptImportAssistant->Release();
				CoFreeUnusedLibraries();
			}
			else
			{
				strMsg = _T("Fail to create an instance Import Assistance COM Object");
				AfxMessageBox (strMsg);
			}
			CoUninitialize ();
		}
	}
	catch (CFileException* e)
	{
		MSGBOX_FileExceptionMessage (e);
		e->Delete();
	}
	catch (CArchiveException* e)
	{
		MSGBOX_ArchiveExceptionMessage (e);
		e->Delete();
	}
	catch (...)
	{

	}

	PostMessage (WM_CLOSE, 0, 0);
	return 0;
}
