/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mainfrm.cpp : implementation of the CfMainFrame class
**    Project  : Ingres II / IEA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main frame
**
** History:
**
** 16-Oct-2001 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 26-Feb-2003 (uk$so01)
**    SIR  #106952, Enhance library + command line.
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 22-Aug-2003 (uk$so01)
**    SIR #106648, Add silent mode in IEA. 
** 24-Mar-2005 (komve01)
**    BUG #113962/ Issue#13992386
**	  IEA when supplied with a file name on the command line
**	  shows a message box and does not exit.
** 21-Aug-2008 (whiro01)
**    Moved <afx...> include to "stdafx.h"
** 02-Oct-2008 (drivi01)
**    Bug #120996/ Issue #131294
**    Replace multi-threaded apartment model with single-threaded
**    apartment model.  Multi-threaded apartment model is causing
**    problems with "Save As" dialog and is not really needed here.
**    Add forgotten uninitialize statement.
**
**/


#include "stdafx.h"
#include "iea.h"
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
	ON_MESSAGE (WMUSR_CALLFUNCTION, OnExportAssistant)
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
#if defined (MAINWIN)
	AfxGetMainWnd()->ShowWindow(SW_SHOWMINIMIZED);
#endif
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

long CfMainFrame::OnExportAssistant(WPARAM w, LPARAM l)
{
	USES_CONVERSION;
	HRESULT hError = NOERROR;

	try
	{
		//
		// Check host name and iisystem:
		CString strFile = theApp.m_cmdLine.m_strIIParamFile;
		if (!strFile.IsEmpty())
		{
			//
			// Check file extension:
			BOOL bFileOK = TRUE;
			int nPos = strFile.ReverseFind(_T('.'));
			if (nPos != -1)
			{
				CString strExt = strFile.Mid(nPos+1);
				if (strExt.CompareNoCase(_T("ii_exp")) != 0)
					bFileOK = FALSE;
			}
			else
				bFileOK = FALSE;
			if (!bFileOK)
			{
				AfxMessageBox (IDS_MSG_WRONG_PARAMETER_FILE);
				return FALSE;
			}

			CFile f((LPCTSTR)strFile, CFile::modeRead);
			f.Close();
		}


		hError = CoInitialize(NULL);
		if (FAILED(hError))
		{
			AfxMessageBox (IDS_MSG_FAIL_2_INITIALIZE_COM);
		}
		else
		{
			IUnknown*   pAptImportAssistant = NULL;
			IExportAssistant* pIea;

			hError = CoCreateInstance(
				CLSID_IMPxEXPxASSISTANCT,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_IUnknown,
				(PVOID*)&pAptImportAssistant);

			if (SUCCEEDED(hError))
			{
				BOOL bOk = GetInterfacePointer(pAptImportAssistant, IID_IExportAssistant, (LPVOID*)&pIea);
				if (bOk)
				{
					if (theApp.m_cmdLine.GetStatus() && !theApp.m_cmdLine.HasCmdLine())
					{
						pIea->ExportAssistant (m_hWnd);
						pIea->Release();
					}
					else
					{
						const int nBuffLen = 64;
						BOOL bLenOK = TRUE;
						IEASTRUCT iiparam;
						memset (&iiparam, 0, sizeof(iiparam));
						iiparam.bLoop    = theApp.m_cmdLine.m_bLoop;
						iiparam.bSilent  = theApp.m_cmdLine.m_bBatch;
						wcscpy (iiparam.wczSessionDescription, L"Ingres Export Assistant");
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
						// STATEMENT:
						if (bLenOK && !theApp.m_cmdLine.m_strStatement.IsEmpty())
						{
							iiparam.lpbstrStatement = theApp.m_cmdLine.m_strStatement.AllocSysString();
						}
						// EXPORT FILE:
						if (bLenOK && theApp.m_cmdLine.m_strExportFile.GetLength() < _MAX_PATH)
							wcscpy (iiparam.wczExportFile, T2W((LPTSTR)(LPCTSTR)theApp.m_cmdLine.m_strExportFile));
						else
							bLenOK = FALSE;
						// EXPORT MODE:
						if (bLenOK && theApp.m_cmdLine.m_strExportMode.GetLength() < _MAX_PATH)
							wcscpy (iiparam.wczExportFormat, T2W((LPTSTR)(LPCTSTR)theApp.m_cmdLine.m_strExportMode));
						else
							bLenOK = FALSE;

						// EXPORT PARAMETER FILE:
						if (bLenOK && theApp.m_cmdLine.m_strIIParamFile.GetLength() < _MAX_PATH)
							wcscpy (iiparam.wczParamFile, T2W((LPTSTR)(LPCTSTR)theApp.m_cmdLine.m_strIIParamFile));
						else
							bLenOK = FALSE;
						// LIST OF EXPORT PARAMETER FILES (silent mode only):
						if (bLenOK && theApp.m_cmdLine.m_strListFile.GetLength() < _MAX_PATH)
							wcscpy (iiparam.wczListFile, T2W((LPTSTR)(LPCTSTR)theApp.m_cmdLine.m_strListFile));
						else
							bLenOK = FALSE;
						// LOG FILE (silent mode only):
						if (bLenOK && theApp.m_cmdLine.m_strLogFile.GetLength() < nBuffLen)
							wcscpy (iiparam.wczLogFile, T2W((LPTSTR)(LPCTSTR)theApp.m_cmdLine.m_strLogFile));
						else
							bLenOK = FALSE;
						// OVERWRITE (silent mode only):
						iiparam.bOverWrite = theApp.m_cmdLine.m_bOverWrite;
						if (bLenOK)
							pIea->ExportAssistant2 (m_hWnd, &iiparam);
						else
						{
							AfxMessageBox (IDS_MSG_BUFFER_LENGTH);
						}
						pIea->Release();
						if (iiparam.lpbstrStatement)
						{
							SysFreeString(iiparam.lpbstrStatement);
							iiparam.lpbstrStatement = NULL;
						}
					}
				}
				else
				{
					AfxMessageBox (IDS_MSG_FAIL_2_QUERYINTERFACE_EXPORT_ASSISTANT);
				}
				
				pAptImportAssistant->Release();
				CoFreeUnusedLibraries();
			}
			else
			{
				AfxMessageBox (IDS_MSG_FAIL_2_CREATE_EXPORT_ASSISTANT);
			}
			CoUninitialize();
		}
	}
	catch (CFileException* e)
	{
		MSGBOX_FileExceptionMessage (e);
		e->Delete();
		//e->ReportError();
		//e->Delete();
	}
	catch (CArchiveException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch (...)
	{

	}

	PostMessage (WM_CLOSE, 0, 0);
	return 0;
}