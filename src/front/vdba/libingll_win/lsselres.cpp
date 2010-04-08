/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		        
*/

/*
**    Source   : lsselres.cpp, implementation file
**    Project  : Visual DBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Owner draw LISTCTRL to have a custom vircroll (for the select result)
**
** History:
**
** xx-Feb-1998 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 30-Jan-2002 (uk$so01)
**    SIR #106648, Enhance the library.
** 14-Fev-2002 (uk$so01)
**    SIR #106648, Enhance library (select loop)
** 22-Aug-2003 (uk$so01)
**    SIR #106648, Add silent mode in IEA. 
** 19-Dec-2003 (uk$so01)
**    SIR #111475, Coorperative shutdown between the DBMS Client & Server.
** 18-Nov-2004 (uk$so01)
**    BUG #113491 / ISSUE #13755178: Manage to Release & disconnect the DBMS session.
** 22-Nov-2004 (uk$so01)
**    BUG #113491 / ISSUE #13755178: Addition fix to the release session.
** 31-Jan-2005 (komve01)
**    BUG #113766,Issue# 13902259. 
**    Added additional validation for the existence of destination
**    folder for IEA. CFileException was giving "No Error occured" when
**	  there was no destination directory, as a errMessage. However,
**    upon receiving this error code, if we look at GetLastError, it gives
**    a more meaningful message called "The system cannot find the path specified".
**	  Hence, used GetLastError instead of relying on the exception's message.
** 28-Sep-2005 (gupsh01)
**    Fixed CloseCursor to release the session only if it is not an
**    independent session.
*/

#include "stdafx.h"
#include "lsselres.h"
#include "sessimgr.h" // Session Manager (shared and global)
#include "wmusrmsg.h"
#include "sqltrace.h"
#include "dmlcolum.h"
#include "sqlselec.h"
#include "usrexcep.h"
#include "tlsfunct.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//
// FETCH ROW: (using select loop instead of cursor)
// CLASS: CaExecParamSelectLoop
// ************************************************************************************************
CaExecParamSelectLoop::CaExecParamSelectLoop():CaExecParam(INTERRUPT_USERPRECIFY)
{
	Initialize();
}

CaExecParamSelectLoop::CaExecParamSelectLoop(UINT nInteruptType):CaExecParam(nInteruptType)
{
	Initialize();
}

void CaExecParamSelectLoop::Initialize()
{
	m_hWnd    = NULL;
	m_nEndFetchStatus = CaExecParamSelectLoop::FETCH_ERROR;
	m_bUseTrace = FALSE;
	m_nAccumulation = 0;
	m_bExecuteImmediately = TRUE;
	m_pSession = NULL;

	m_strMsgBoxTitle = _T("");
	m_strMsgFailed = _T("");
	m_strInterruptFetching =_T("");
	m_strFetchInfo = _T("");
	m_nRowLimit = 0;

	m_pUserData = NULL;
	m_pfnUserHandlerResult = NULL;
}

CaExecParamSelectLoop::~CaExecParamSelectLoop()
{
	while (!m_listHeader.IsEmpty())
		delete m_listHeader.RemoveHead();
	while (!m_listBufferColumn.IsEmpty())
		delete m_listBufferColumn.RemoveHead();
}

int CaExecParamSelectLoop::Run(HWND hWndTaskDlg)
{
	CaRowTransfer* pRecordTransfer = NULL;
	ASSERT(m_pSession);
	if (!m_pSession)
		return 1;
	class CaSessionScope
	{
	public:
		CaSessionScope(CaSession* pSess){m_pSession=pSess;}
		~CaSessionScope()
		{
			// Work around: If the session is independent, it should not be released
			// It is the caller that takes care of the release session!!!
			if (m_pSession && !m_pSession->GetIndependent())
				m_pSession->Release(SESSION_COMMIT);
		}

	private:
		CaSession* m_pSession;
	};

	try
	{
		m_pSession->Activate();
		CaSessionScope sessionScope(m_pSession);

		BOOL bInterrupted = FALSE;
		if (!SelectLoop(hWndTaskDlg))
			return 1;

		//
		// At this step, the process of fetching rows is stopped.
		// But we must know if the process is realy ended (ie fetching past the last row in the cursor):
		switch (m_nEndFetchStatus)
		{
		case CaExecParamSelectLoop::FETCH_NORMAL_ENDING:
			//
			// Notify window that handle the messages of TasAnimate that we have reached the end of cursor:
			// WPARAM = (status)
			// LPARAM = (Total number of rows fetched)
			if (m_hWnd)
				::PostMessage (m_hWnd, WMUSRMSG_SQL_FETCH, (WPARAM)m_nEndFetchStatus, (LPARAM)m_nAccumulation);
			break;
		case CaExecParamSelectLoop::FETCH_REACH_LIMIT:
			//
			// Notify window that handle the messages of TasAnimate that we have reached the end of cursor:
			// WPARAM = (status)
			// LPARAM = (Total number of rows fetched)
			if (m_hWnd)
				::PostMessage (m_hWnd, WMUSRMSG_SQL_FETCH, (WPARAM)m_nEndFetchStatus, (LPARAM)m_nAccumulation);
			break;
		}
		
		m_pSession->SetSessionNone();

		//
		// Run has terminated successfully.
		return 0;
	}
	catch (CeSqlException e)
	{
		TRACE1 ("Raise exception, CaExecParamQueryRows::Run(): %s\n", (LPCTSTR)e.GetReason());
		m_nEndFetchStatus = CaExecParamQueryRows::FETCH_ERROR;
		m_strMsgFailed = e.GetReason();
		if (m_bUseTrace)
			::SendMessage (m_hWnd, WMUSRMSG_SQL_FETCH, (WPARAM)CaExecParamSelectLoop::FETCH_TRACEINFO, (LPARAM)(LPCTSTR)m_strMsgFailed);
	}
	catch (CFileException* e)
	{
		CString strErr;
		LPVOID lpMsgBuf;
		switch(((CFileException*)e)->m_cause)
		{
		case CFileException::none:   
			if (!FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
								NULL,GetLastError(),
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								(LPTSTR) &lpMsgBuf,	0,NULL ))
			{
				// Handle the error.
				e->GetErrorMessage(strErr.GetBuffer(_MAX_PATH), _MAX_PATH);
				strErr.ReleaseBuffer();
				break;
			}
			strErr = (LPCTSTR)lpMsgBuf;
			// Free the buffer.
			LocalFree( lpMsgBuf );
			break;
		default:
			// Handle other errors.
			e->GetErrorMessage(strErr.GetBuffer(_MAX_PATH), _MAX_PATH);
			strErr.ReleaseBuffer();
			break;
		}
		m_strMsgFailed = strErr;
		e->Delete();
	}
	catch (int nExcep)
	{
		//
		// Caller of TkAnimat will handle error:
		if (nExcep == 1)
		{
			m_strMsgFailed =_T("");
			m_nEndFetchStatus = CaExecParamSelectLoop::FETCH_ERROR;
		}
	}
	catch (...)
	{
		m_strMsgFailed =_T("Raise exception, CaExecParamSelectLoop::Run() unknown error\n");
		m_nEndFetchStatus = CaExecParamSelectLoop::FETCH_ERROR;
	}

	//
	// Exception in Run has been raised:
	if (pRecordTransfer)
		delete pRecordTransfer;
	m_pSession->SetSessionNone();
	return 1;
}

void CaExecParamSelectLoop::OnTerminate(HWND hWndTaskDlg)
{
	if (m_nEndFetchStatus == CaExecParamQueryRows::FETCH_ERROR)
	{
		if (!m_strMsgFailed.IsEmpty())
		{
			::SendMessage(m_hWnd, WMUSRMSG_SQL_FETCH, (LPARAM)m_nEndFetchStatus, 0);
			::MessageBox(hWndTaskDlg, m_strMsgFailed, m_strMsgBoxTitle, MB_ICONEXCLAMATION | MB_OK);
		}
	}
}

BOOL CaExecParamSelectLoop::OnCancelled(HWND hWndTaskDlg)
{
	m_synchronizeInterrupt.BlockUserInterface(TRUE);    // Block itself;
	m_synchronizeInterrupt.SetRequestCancel(TRUE);      // Request the cancel.
	m_synchronizeInterrupt.BlockWorkerThread(TRUE);     // Block worker thread (Run() method);
	m_synchronizeInterrupt.WaitUserInterface();         // Wait itself until worker thread allows to do something;

	int nRes = ::MessageBox(hWndTaskDlg, m_strInterruptFetching, m_strMsgBoxTitle, MB_ICONQUESTION | MB_YESNO);
	m_synchronizeInterrupt.BlockWorkerThread(FALSE);    // Release worker thread (Run() method);
	m_synchronizeInterrupt.SetRequestCancel(FALSE);     // Un-Request the cancel.
	if (nRes == IDYES)
	{
		SetInterrupted (TRUE);
		return TRUE;
	}
	else
		return FALSE;
}


//
// FETCH ROW: (using select with cursor)
// CLASS: CaExecParamSelectCursor
// ************************************************************************************************
CaExecParamSelectCursor::CaExecParamSelectCursor():CaExecParamSelectLoop()
{
	Initialize();

}

CaExecParamSelectCursor::CaExecParamSelectCursor(UINT nInteruptType):CaExecParamSelectLoop(nInteruptType)
{
	Initialize();
}


CaExecParamSelectCursor::~CaExecParamSelectCursor()
{
	CloseCursor();
}

void CaExecParamSelectCursor::Initialize()
{
	m_bClose  = FALSE;
	m_pCursor = NULL;
	m_bFetchMore= FALSE;
}

void CaExecParamSelectCursor::SetCursor (CaCursor* pCursor)
{
	m_pCursor = pCursor;
	if (m_pCursor)
	{
		m_bClose = FALSE;
		/*
		CaCursorInfo& cursorInfo = m_pCursor->GetCursorInfo();
		cursorInfo.SetFloat4Format(m_fetchInfo.GetFloat4Width(), m_fetchInfo.GetFloat4Decimal(), m_fetchInfo.GetFloat4DisplayMode());
		cursorInfo.SetFloat8Format(m_fetchInfo.GetFloat8Width(), m_fetchInfo.GetFloat8Decimal(), m_fetchInfo.GetFloat8DisplayMode());
		*/
	}
}

int CaExecParamSelectCursor::Run(HWND hWndTaskDlg)
{
	CaRowTransfer* pRecordTransfer = NULL;
	ASSERT(m_pSession);
	if (!m_pSession)
		return 1;

	try
	{
		CString strInfo;
		int nTuple = 0;
		int nColCount, nRes=0;
		m_pSession->Activate();

		int nLimit = m_bFetchMore? m_nRowLimit/2: m_nRowLimit;
		BOOL bGoOnFetching = TRUE;
		BOOL bHasTrace = FALSE;
		BOOL bInterrupted = FALSE;
		CaSqlTrace aTrace;
		if (m_bUseTrace)
		{
			bHasTrace = FALSE;
			aTrace.Start();
			m_strlTrace.RemoveAll();
		}

		while (bGoOnFetching)
		{
			if (m_synchronizeInterrupt.IsRequestCancel())
			{
				m_synchronizeInterrupt.BlockUserInterface (FALSE); // Release user interface
				m_synchronizeInterrupt.WaitWorkerThread ();        // Wait itself until user interface releases it.
			}

			bInterrupted = IsInterrupted();
			if (bInterrupted)
			{
				m_nEndFetchStatus = CaExecParamSelectCursor::FETCH_INTERRUPTED;
				break;
			}
			pRecordTransfer = new CaRowTransfer();

			bGoOnFetching = m_pCursor->Fetch(pRecordTransfer, nColCount);
			if (!bGoOnFetching)
			{
				if (m_nRowLimit > 0 )
				{
					if (nTuple < nLimit)
						m_nEndFetchStatus = CaExecParamSelectCursor::FETCH_NORMAL_ENDING;
					else
						m_nEndFetchStatus = CaExecParamSelectCursor::FETCH_REACH_LIMIT;
				}
				else
					m_nEndFetchStatus = CaExecParamSelectCursor::FETCH_NORMAL_ENDING;
				delete pRecordTransfer;
			}
			else
			{
				m_nAccumulation++;
				nTuple++;
				
				if (hWndTaskDlg)
				{
					//
					// MSG = "Rows fetched : %d"
					strInfo.Format (m_strFetchInfo, m_nAccumulation);//_T("Rows fetched : %d")
					LPTSTR lpszMsg = new TCHAR[strInfo.GetLength()+1];
					lstrcpy (lpszMsg, strInfo);
					::PostMessage (hWndTaskDlg, WM_EXECUTE_TASK, W_EXTRA_TEXTINFO, (LPARAM)lpszMsg);
				}
				if (m_hWnd)
				{
					//
					// List of records to be displayed
					::SendMessage (m_hWnd, WMUSRMSG_UPDATEDATA, (BOOL)m_bFetchMore, (LPARAM)pRecordTransfer);
				}
				else
				if (m_pfnUserHandlerResult && m_pUserData)
				{
					m_pfnUserHandlerResult (m_pUserData, pRecordTransfer);
				}
				else
				{
					delete pRecordTransfer;
					pRecordTransfer = NULL;
				}

				if (m_nRowLimit > 0 && nTuple >= nLimit)
				{
					m_nEndFetchStatus = CaExecParamSelectCursor::FETCH_REACH_LIMIT;
					break;
				}
			}
		}
		
		if (m_bUseTrace)
		{
			aTrace.Stop();
			CString& strTraceBuffer = aTrace.GetTraceBuffer();
			RCTOOL_CR20x0D0x0A(strTraceBuffer);
			if (!strTraceBuffer.IsEmpty())
			{
				bHasTrace = TRUE;
				m_strlTrace.AddTail (strTraceBuffer);
				// Notify window that handle the messages of TasAnimate that we have reached the end of cursor:
				// WPARAM = (status)
				// LPARAM = (Total number of rows fetched)
				if (m_hWnd)
					::SendMessage (m_hWnd, WMUSRMSG_SQL_FETCH, (WPARAM)CaExecParamSelectCursor::FETCH_TRACEINFO, (LPARAM)(LPCTSTR)strTraceBuffer);
			}
		}

		//
		// At this step, the process of fetching rows is stopped.
		// But we must know if the process is realy ended (ie fetching past the last row in the cursor):
		switch (m_nEndFetchStatus)
		{
		case CaExecParamSelectCursor::FETCH_NORMAL_ENDING:
			//
			// Notify window that handle the messages of TasAnimate that we have reached the end of cursor:
			// WPARAM = (status)
			// LPARAM = (Total number of rows fetched)
			CloseCursor();
			if (m_hWnd)
				::SendMessage (m_hWnd, WMUSRMSG_SQL_FETCH, (WPARAM)m_nEndFetchStatus, (LPARAM)m_nAccumulation);
			break;
		case CaExecParamSelectCursor::FETCH_REACH_LIMIT:
			//
			// Notify window that handle the messages of TasAnimate that we have reached the end of cursor:
			// WPARAM = (status)
			// LPARAM = (Total number of rows fetched)
			if (m_hWnd)
				::SendMessage (m_hWnd, WMUSRMSG_SQL_FETCH, (WPARAM)m_nEndFetchStatus, (LPARAM)m_nAccumulation);
			break;
		}
		
		m_pSession->SetSessionNone();

		//
		// Run has terminated successfully.
		return 1;
	}
	catch (CeSqlException e)
	{
		TRACE1 ("Raise exception, CaExecParamSelectCursor::Run(): %s\n", (LPCTSTR)e.GetReason());
		m_nEndFetchStatus = CaExecParamSelectCursor::FETCH_ERROR;
		m_strMsgFailed = e.GetReason();
		if (m_bUseTrace && m_hWnd)
			::SendMessage (m_hWnd, WMUSRMSG_SQL_FETCH, (WPARAM)CaExecParamSelectCursor::FETCH_TRACEINFO, (LPARAM)(LPCTSTR)m_strMsgFailed);
	}
	catch (CFileException* e)
	{
		CString strErr;
		LPVOID lpMsgBuf;
		switch(((CFileException*)e)->m_cause)
		{
		case CFileException::none:   
			if (!FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
								NULL,GetLastError(),
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								(LPTSTR) &lpMsgBuf,	0,NULL ))
			{
				// Handle the error.
				e->GetErrorMessage(strErr.GetBuffer(_MAX_PATH), _MAX_PATH);
				strErr.ReleaseBuffer();
				break;
			}
			strErr = (LPCTSTR)lpMsgBuf;
			// Free the buffer.
			LocalFree( lpMsgBuf );
			break;
		default:
			// Handle other errors.
			e->GetErrorMessage(strErr.GetBuffer(_MAX_PATH), _MAX_PATH);
			strErr.ReleaseBuffer();
			break;
		}
		m_strMsgFailed = strErr;
		e->Delete();
		if (m_bUseTrace && m_hWnd)
			::SendMessage (m_hWnd, WMUSRMSG_SQL_FETCH, (WPARAM)CaExecParamSelectCursor::FETCH_TRACEINFO, (LPARAM)(LPCTSTR)m_strMsgFailed);
	}
	catch (int nExcep)
	{
		//
		// Caller of TkAnimat will handle error:
		if (nExcep == 1)
		{
			m_strMsgFailed =_T("");
			m_nEndFetchStatus = CaExecParamSelectCursor::FETCH_ERROR;
		}
	}
	catch (...)
	{
		m_strMsgFailed =_T("Raise exception, CaExecParamSelectCursor::Run() unknown error\n");
		m_nEndFetchStatus = CaExecParamSelectCursor::FETCH_ERROR;
		if (m_bUseTrace)
			::SendMessage (m_hWnd, WMUSRMSG_SQL_FETCH, (WPARAM)CaExecParamSelectCursor::FETCH_TRACEINFO, (LPARAM)(LPCTSTR)m_strMsgFailed);
	}

	CloseCursor();
	//
	// Exception in Run has been raised:
	if (pRecordTransfer)
		delete pRecordTransfer;
	m_pSession->SetSessionNone();
	return 0;
}


void CaExecParamSelectCursor::OpenCursor()
{
	ASSERT(m_pCursor != NULL);
	ASSERT(m_pSession!= NULL);
	if (m_pSession && m_pCursor )
	{
		m_pSession->Activate();
		m_pCursor->Open();
	}
}

void CaExecParamSelectCursor::CloseCursor()
{
	BOOL bReleaseSessionOnClose = TRUE;
	if (!m_bClose && m_pSession && m_pCursor)
	{
		m_pSession->Activate();
		if (m_pCursor)
		{
		    bReleaseSessionOnClose = 
			m_pCursor->GetReleaseSessionOnClose();	
		    delete m_pCursor;
		}
		if (bReleaseSessionOnClose && !m_pSession->GetIndependent())
		    m_pSession->Release(SESSION_COMMIT);
	}
	m_pCursor = NULL;
	m_bClose = TRUE;
}


//
// FETCH ROW:
// CLASS: CaExecParamQueryRows
// ************************************************************************************************
CaExecParamQueryRows::CaExecParamQueryRows(BOOL bCursor):CaExecParamSelectCursor()
{
	Initialize();
	m_bUseCursor = bCursor;
}

CaExecParamQueryRows::CaExecParamQueryRows(BOOL bCursor, UINT nInteruptType):CaExecParamSelectCursor(nInteruptType)
{
	Initialize();
}

void CaExecParamQueryRows::Initialize()
{
	m_bConstructedHeader = FALSE;
}

CaExecParamQueryRows::~CaExecParamQueryRows()
{

}



int CaExecParamQueryRows::Run(HWND hWndTaskDlg)
{
	int nRet = 1;
	if (m_bUseCursor)
	{
		ASSERT(m_pCursor);
		if (m_pCursor)
			nRet = CaExecParamSelectCursor::Run(hWndTaskDlg);
	}
	else
	{
		nRet = CaExecParamSelectLoop::Run(hWndTaskDlg);
	}

	return nRet;
}

void CaExecParamQueryRows::DoInterrupt()
{
	if (m_bUseCursor)
	{
		CloseCursor();
	}
}

BOOL CaExecParamQueryRows::IsMoreResult()
{
	if (m_bUseCursor)
	{
		return (GetCursor() != NULL);
	}
	else
	{
		return FALSE;
	}
}

CString CaExecParamQueryRows::GetCursorName()
{
	if (!m_bUseCursor)
		return _T("");
	if (m_pCursor)
		return m_pCursor->GetCursorName();
	return _T("");
}

CTypedPtrList< CObList, CaColumn* >& CaExecParamQueryRows::GetListHeader()
{
	//
	// You must call ConstructListHeader first:
	ASSERT(m_bConstructedHeader);

	if (m_bUseCursor)
	{
		ASSERT(m_pCursor);
		return m_pCursor->GetListHeader();
	}
	else
	{
		return m_listHeader;
	}
}

void CaExecParamQueryRows::ConstructListHeader()
{
	ASSERT(m_pSession);
	if (m_pSession)
		m_pSession->Activate();
	if (m_bUseCursor)
	{
		ASSERT(m_pCursor);
		if (m_pCursor)
		{
			OpenCursor();
			m_bConstructedHeader = TRUE;
		}
	}
	else
	{
		CaExecParamSelectLoop::ConstructListHeader();
		m_bConstructedHeader = TRUE;
	}
}


//
// CLASS: CuListCtrlSelectResult
// ************************************************************************************************
CuListCtrlSelectResult::CuListCtrlSelectResult():CuListCtrl()
{
	m_pQueryRowParam = NULL;
	m_bShowRowNumber = TRUE;
	m_nColumn = 0;
	m_nRow = 0;
	m_nRowDeleted  = 0;
	m_nFirstItemV2 = -1;
	m_bFirstRefetch = TRUE;
	m_nArraySize = 0;
	m_nSortColumn = -1;
	m_bAsc = TRUE;
	m_bAllRows = TRUE;

	m_pArrayColumnWidth = NULL;
	m_strNoMoreAvailable = _T("The first %d tuples are no longer available"); 
	m_strNotAvailable    = _T("n/a");
	m_strOutOfMemory = _T("Low of Memory ...\nCannot allocate memory, please close some applications !");
	m_strTitle = _T("Fetching Rows In Progress");
}

CuListCtrlSelectResult::~CuListCtrlSelectResult()
{
	if (m_pQueryRowParam)
		delete m_pQueryRowParam;
	m_pQueryRowParam = NULL;
	if (m_pArrayColumnWidth)
		delete m_pArrayColumnWidth;
}


BEGIN_MESSAGE_MAP(CuListCtrlSelectResult, CuListCtrl)
	//{{AFX_MSG_MAP(CuListCtrlSelectResult)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_UPDATEDATA,     OnDisplayRow)
	ON_MESSAGE (WMUSRMSG_SQL_FETCH,      OnEndFetchRows)
	ON_MESSAGE (WMUSRMSG_CHANGE_SETTING, OnChangeSetting)
END_MESSAGE_MAP()

void CuListCtrlSelectResult::OldTupleIndicator ()
{
	ASSERT(m_pQueryRowParam);
	if (!m_pQueryRowParam)
		return;
	if (!m_pQueryRowParam->IsUsedCursor())
		return;
	int nHalf = m_pQueryRowParam->GetRowLimit()/2;
	int i, nCount = GetItemCount();
	if (nCount == 0)
		return;
	//
	// Delete the first nHalf items. 
	int nPosDelete = m_bFirstRefetch ? 0: 1;
	for (i=0; i<nHalf; i++)
	{
		if ((int)GetItemData(nPosDelete) == 0)
		{
			if (DeleteItem (nPosDelete))
				m_nRowDeleted++;
		}
	}

	//
	// Indicate that the nHalf items are no longer availables:
	CString strMsg;
	strMsg.Format (m_strNoMoreAvailable, m_nRowDeleted);
	if (m_bFirstRefetch)
		InsertItem  (0, strMsg);
	else
		SetItemText (0, 0, strMsg);
	SetItemData (0, (DWORD)1);

	if (!m_pQueryRowParam->IsMoreResult())
		return;
	CTypedPtrList< CObList, CaColumn* >& listHeader = m_pQueryRowParam->GetListHeader();
	for (i=0; i<listHeader.GetCount(); i++)
		SetItemText (0, i, strMsg);
}


void CuListCtrlSelectResult::MoreTupleIndicator ()
{
	ASSERT(m_pQueryRowParam);
	if (!m_pQueryRowParam)
		return;
	int nHalf = m_pQueryRowParam->GetRowLimit()/2;
	int i, nCount = GetItemCount();
	for (i=0; i<nHalf; i++)
	{
		InsertItem  (nCount+i, _T(""));
		SetItemData (nCount+i, (DWORD)2);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CuListCtrlSelectResult message handlers

void CuListCtrlSelectResult::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	//
	// BUG: Scroll one line up that is nSBCode = 0 but nSBCode is not SB_LINEUP 
	if ((nSBCode & SB_LINEUP) || (nSBCode == 0) || (nSBCode & SB_PAGEUP) || nSBCode & (SB_THUMBTRACK|SB_PAGEUP)) 
	{
		CuListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
	}
	else
	if ((nSBCode & SB_LINEDOWN) || (nSBCode == 0) || (nSBCode & SB_PAGEDOWN) || nSBCode & (SB_THUMBTRACK|SB_PAGEDOWN)) 
	{
		CuListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
	}
	else
	if (nSBCode == SB_ENDSCROLL)
	{
		CuListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
		OnFetchNewRows (0, 0);
	}
}


void CuListCtrlSelectResult::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CuListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CuListCtrlSelectResult::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CuListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CuListCtrlSelectResult::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	int nSel= GetNextItem(-1, LVNI_SELECTED);
	CuListCtrl::OnKeyUp(nChar, nRepCnt, nFlags);
	if (!m_pQueryRowParam)
		return;
	int nHalf = m_pQueryRowParam->GetRowLimit()/2;
	if (nChar == VK_DOWN || nChar == VK_NEXT || nChar == VK_END)
	{
		BOOL bV2 = FALSE;
		int  nCount = 0, nCountPerPage = GetCountPerPage();
		int  i, nTopIndex = GetTopIndex();
		for (i=nTopIndex; nCount < nCountPerPage && i < GetItemCount(); i++)
		{
			nCount++;
			if ((int)GetItemData (i) == 2)
			{
				bV2 = TRUE;
				break;
			}
		}

		if (bV2)
		{
			OnFetchNewRows (0, 0);
			nSel = nSel - nHalf;
			if (nSel >= 1)
				SetItemState (nSel, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		}
	}
}


void CuListCtrlSelectResult::AddLineItem (int iSubItem, LPCTSTR lpValue)
{
	int nCount = GetItemCount();
	if (iSubItem < 0)
		return;
	if (iSubItem == 0)
	{
		int nIdx = InsertItem(nCount, lpValue);
		SetItemData (nIdx, (DWORD)0);
	}
	else
	{
		if (nCount == 0)
			return;
		SetItemText (nCount-1, iSubItem, lpValue);
	}
}

LONG CuListCtrlSelectResult::OnDisplayRow (WPARAM wParam, LPARAM lParam)
{
	int i=0;
	CString strItem;
	CaRowTransfer* pRecord = (CaRowTransfer*)lParam;
	BOOL bFetchedMore = (BOOL)wParam; // TRUE (indicate that the row is refetched after the limit has been reached)
	ASSERT(pRecord);
	if (!pRecord)
		return 0;

	CStringList& listFields = pRecord->GetRecord();
	if (!listFields.IsEmpty())
	{
		int nColumnCount = listFields.GetCount();
		if (m_bShowRowNumber)
			nColumnCount++;
		ASSERT (m_nColumn == nColumnCount);
		if (nColumnCount == m_nColumn)
		{
			m_nRow++;
			i = 0;
			if (!bFetchedMore)
			{
				if (m_bShowRowNumber)
				{
					strItem.Format (_T("%d "), m_nRow);
					AddLineItem (i, strItem);
					i++;
				}

				POSITION pos = listFields.GetHeadPosition();
				while (pos != NULL)
				{
					strItem = listFields.GetNext(pos);
					AddLineItem (i, strItem);
					m_pArrayColumnWidth[i] = max (m_pArrayColumnWidth[i], strItem.GetLength());
					i++;
				}
			}
			else
			{
				if (m_bShowRowNumber)
				{
					strItem.Format (_T("%d "), m_nRow);
					SetItemText (m_nFirstItemV2, i, strItem);
					i++;
				}

				POSITION pos = listFields.GetHeadPosition();
				while (pos != NULL)
				{
					strItem = listFields.GetNext(pos);
					SetItemText (m_nFirstItemV2, i, strItem);
					m_pArrayColumnWidth[i] = max (m_pArrayColumnWidth[i], strItem.GetLength());
					i++;
				}
				SetItemData (m_nFirstItemV2, (DWORD)0);
				m_nFirstItemV2++;
			}
		}
		delete pRecord;
	}

	return 0;
}


LONG CuListCtrlSelectResult::OnFetchNewRows (WPARAM wParam, LPARAM lParam)
{
	BOOL bStartedEllapse = FALSE;
	CWaitCursor doWaitCursor;
	if (!m_pQueryRowParam)
		return 0;
	if (!m_pQueryRowParam->IsUsedCursor())
		return 0;
	if (!m_pQueryRowParam->IsMoreResult())
		return 0;
	BOOL bTopIndexIsV2 = FALSE;
	int nTopIndex = GetTopIndex();
	int nHalf = m_pQueryRowParam->GetRowLimit()/2;
	int nCountPerPage = GetCountPerPage();

	try
	{
		m_nFirstItemV2 = -1;
		if (m_pQueryRowParam->GetRowLimit() == 0) // It means that we fetch all rows at one time !
			return 0L;
		//
		// Ensure that at least one item of type (2), ie the empty item indicating that
		// there are more tuples to be fetched, is visible:
		BOOL bV2Visible = FALSE;
		int nFirstV2 = -1;
		int i, nVisible = 0, nCount = GetItemCount();
		for (i = nTopIndex; i < nCount && nVisible < nCountPerPage; i++)
		{
			nVisible++;
			if ((int)GetItemData(i) == 2)
			{
				bV2Visible = TRUE;
				break;
			}
		}
		if (bV2Visible)
		{
			bTopIndexIsV2 = ((int)GetItemData (nTopIndex) == 2)? TRUE: FALSE;
			//
			// Delete some rows and indicate that some tuples are no longer available:
			OldTupleIndicator();
			nTopIndex = nTopIndex - nHalf -1;
			if (nTopIndex == -1)
				return 0L;

			nCount = GetItemCount();
			for (i= nHalf; i<nCount; i++)
			{
				if ((int)GetItemData(i) == 2)
				{
					nFirstV2 = i;
					break;
				}
			}
			ASSERT(nFirstV2 != -1);
			if (nFirstV2 == -1)
				return 0L;
			m_nFirstItemV2 = nFirstV2;

			CaSession* pCurrentSession = m_pQueryRowParam->GetSession();
			ASSERT(pCurrentSession);
			if (pCurrentSession)
				pCurrentSession->SetSessionNone();

			m_pQueryRowParam->SetFetchMore();
			m_pQueryRowParam->SetMessageHandler (m_hWnd);
#if defined (_ANIMATION_)
			CxDlgWait dlgWait(m_strTitle, this);
			dlgWait.SetDeleteParam(FALSE);
			dlgWait.SetExecParam (m_pQueryRowParam);
			dlgWait.SetUseAnimateAvi(AVI_FETCHR);
			dlgWait.SetUseExtraInfo();
			dlgWait.SetShowProgressBar(FALSE);
			dlgWait.SetHideCancelBottonAlways(TRUE);
			dlgWait.DoModal();
#else
			m_pQueryRowParam->Run();
#endif
			m_bFirstRefetch = FALSE;
			//
			// Restore the Item position after scrolling:
			CRect rs;
			GetItemRect (0, rs, LVIR_BOUNDS);
			if (!bTopIndexIsV2)
			{
				EnsureVisible (0, TRUE);
				CSize size (0, (nTopIndex +1)*rs.Height());
				Scroll (size);
			}
			else
			{
				EnsureVisible (0, TRUE);
				CSize size (0, (m_nFirstItemV2 - nHalf)*rs.Height());
				Scroll (size);
			}
			//
			// Update the position of Scroll Box:
			SetWindowPos (NULL, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);
		}
	}
	catch(CMemoryException* e)
	{
		AfxMessageBox (m_strOutOfMemory, MB_ICONHAND|MB_SYSTEMMODAL|MB_OK);
		e->Delete();
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		AfxMessageBox (_T("CuListCtrlSelectResult::OnFetchNewRows: unknown error"), MB_ICONEXCLAMATION|MB_OK); // IDS_E_UNKNOWN_ERROR
	}

	return 0L;
}


long CuListCtrlSelectResult::OnEndFetchRows(WPARAM wParam, LPARAM lParam)
{
	CWnd* pParent = GetParent();
	int nStatus = (int)wParam;
	switch (nStatus)
	{
	case CaExecParamQueryRows::FETCH_NORMAL_ENDING:
		if (pParent)
			return (long)pParent->SendMessage (WMUSRMSG_SQL_FETCH, wParam, lParam);
		break;
	case CaExecParamQueryRows::FETCH_REACH_LIMIT:
		MoreTupleIndicator();
		if (pParent)
			return (long)pParent->SendMessage (WMUSRMSG_SQL_FETCH, wParam, lParam);
		break;
	case CaExecParamQueryRows::FETCH_ERROR:
		if (pParent)
			return (long)pParent->SendMessage (WMUSRMSG_SQL_FETCH, wParam, lParam);
		break;
	case CaExecParamQueryRows::FETCH_TRACEINFO:
		if (pParent)
			return (long)pParent->SendMessage (WMUSRMSG_SQL_FETCH, wParam, lParam);
		break;
	default:
		break;
	}

	return 0;
}

void CuListCtrlSelectResult::SortColumn(int nCol)
{
	SORTROW sr;
	LPTSTR lpItemData = NULL;
	CWaitCursor doWaitCursor;
	if (!m_pQueryRowParam)
		return;
	//
	// If the cursor is still opened, we cannot sort because
	// there are some special item data (0,1,2) in the list control and the sort operation
	// need to modify the item data.
	if (!m_bAllRows)
		return;
	m_bAsc = (m_nSortColumn == nCol)? !m_bAsc: TRUE;
	m_nSortColumn = nCol;

	int i, nCount = GetItemCount();
	try
	{
		CString strItem;
		//
		// Sort on the column:
		for (i=0; i<nCount; i++)
		{
			strItem = GetItemText (i, m_nSortColumn);
			if (!strItem.IsEmpty())
			{
				lpItemData = new TCHAR [strItem.GetLength() +1];
				lstrcpy (lpItemData, strItem);
				SetItemData (i, (DWORD)lpItemData);
			}
		}
		sr.bAsc = m_bAsc;
		sr.nDisplayType = m_nHeaderType.GetAt (m_nSortColumn);
		SortItems(SelectResultCompareSubItem, (LPARAM)&sr);
	}
	catch (CMemoryException* e)
	{
		e->ReportError();
		e->Delete ();
	}
	catch (...)
	{
		TRACE0 ("CuListCtrlSelectResult::SortColumn(int nCol): Attempt to sort items failed\n");
	}

	for (i=0; i<nCount; i++)
	{
		lpItemData = (LPTSTR)GetItemData(i);
		if (lpItemData)
			delete lpItemData;
		SetItemData (i, (DWORD)0);
	}
}


//
// return 0 if the cursor is closed (fetched till the end), otherwise the 1
long CuListCtrlSelectResult::DisplayResult()
{
	int i;
	int iType = -1;
	int ilen;
	TCHAR tchszColumn [64];
	CClientDC dc(this);
	m_nRow = 0;
	ASSERT(m_pQueryRowParam);
	if (!m_pQueryRowParam)
		return 0;

	CaColumn* pColumn = NULL;
	CTypedPtrList< CObList, CaColumn* >& listHeader = m_pQueryRowParam->GetListHeader();
	m_nColumn = listHeader.GetCount();
	if (m_bShowRowNumber)
		m_nColumn++;
	//
	// Initialize the column headers
	LVCOLUMN lvcolumn;
	SIZE  size;
	BOOL  bTextExtend = FALSE;
	memset (&lvcolumn, 0, sizeof (LVCOLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	lvcolumn.fmt  = LVCFMT_LEFT;
	
	POSITION pos = listHeader.GetHeadPosition();
	i = 0;

	//
	// Display row number:
	if (m_bShowRowNumber)
	{
		lvcolumn.fmt = LVCFMT_RIGHT;
		lvcolumn.pszText = _T("#");
		lvcolumn.iSubItem = 0;
		lvcolumn.cx = 40;
		InsertColumn (i, &lvcolumn);

		m_nArraySize = listHeader.GetCount();
		m_nArraySize++;
		m_nHeaderType.SetSize (m_nArraySize, 1);
		m_nHeaderType.SetAtGrow (i, SQLACT_NUMBER);

		i++;
	}
	else
	{
		m_nArraySize = listHeader.GetCount();
		m_nHeaderType.SetSize (m_nArraySize, 1);
	}

	while (pos != NULL)
	{
		pColumn = listHeader.GetNext(pos);

		iType = pColumn->GetType();
		lvcolumn.fmt = INGRESII_GetSqlDisplayType(iType)!= SQLACT_NUMBER ? LVCFMT_LEFT: LVCFMT_RIGHT;
		lvcolumn.pszText = (LPTSTR)(LPCTSTR)pColumn->GetName();
		lvcolumn.iSubItem = i;
		bTextExtend = GetTextExtentPoint32 (dc.m_hDC, pColumn->GetName(), (pColumn->GetName()).GetLength(), &size);
		lvcolumn.cx = (bTextExtend && size.cx > 40)? size.cx: 40;
		InsertColumn (i, &lvcolumn);
		if (i==0 && INGRESII_GetSqlDisplayType(iType)== SQLACT_NUMBER) 
		{
			// the 'lvc.fmt' attribute doesn't work in CListCtrl::InsertColumn()
			// for the first column
			// need to do it afterwards through CListCtrl::SetColumn()
			LVCOLUMN lvc; 
			memset (&lvc, 0, sizeof (lvc));
			lvc.mask = LVCF_FMT;
			lvc.fmt  = LVCFMT_RIGHT;
			SetColumn(0, &lvc);
		}
		
		m_nHeaderType.SetAtGrow (i, INGRESII_GetSqlDisplayType(iType));
		i++;
	}

	m_pArrayColumnWidth = new int [m_nColumn];
	for (i = 0; i<m_nColumn; i++)
		m_pArrayColumnWidth [i] = 0;

	//
	// N0TIFY the parent window to handle the TRACE output if necessary:
	if (m_pQueryRowParam->IsShowRawPage())
	{
		CWnd* pWndParent = GetParent();
		if (pWndParent)
			pWndParent->SendMessage(WMUSRMSG_SQL_FETCH, (WPARAM)CaExecParamQueryRows::FETCH_TRACEBEGIN, (LPARAM)m_pQueryRowParam);
	}

	CaSession* pCurrentSession = m_pQueryRowParam->GetSession();
	ASSERT(pCurrentSession);
	if (pCurrentSession)
		pCurrentSession->SetSessionNone();

	m_pQueryRowParam->SetMessageHandler (m_hWnd);
#if defined (_ANIMATION_)
	CxDlgWait dlgWait(m_strTitle, this);
	dlgWait.SetDeleteParam(FALSE);
	dlgWait.SetExecParam (m_pQueryRowParam);
	dlgWait.SetUseAnimateAvi(AVI_FETCHR);
	dlgWait.SetUseExtraInfo();
	dlgWait.SetShowProgressBar(FALSE);
	dlgWait.SetHideCancelBottonAlways(TRUE);
	dlgWait.DoModal();
#else
	m_pQueryRowParam->Run();
#endif
	if (m_pArrayColumnWidth)
	{
		// Column width is sized to fit the column value, limit to 25 characters
		//                            012345678901234567890123
		CString strTemplateChar = _T("X8X8X8X8X8X8X8X8X8X8X8X8");
		LVCOLUMN lvcolumn; 
		int nColumLen = 0;
		memset (&lvcolumn, 0, sizeof (lvcolumn));
		lvcolumn.mask      = LVCF_TEXT;
		lvcolumn.pszText   = tchszColumn;
		lvcolumn.cchTextMax= sizeof (tchszColumn);
	
		for (i=0; i<m_nColumn; i++)
		{
			nColumLen = 0;
			if (GetColumn (i, &lvcolumn))
			{
				if (lvcolumn.pszText[0])
					nColumLen = lstrlen (lvcolumn.pszText);
			}
			ilen = (m_pArrayColumnWidth[i] < 26)? m_pArrayColumnWidth[i]: 25; 
			if (nColumLen > ilen)
				ilen = nColumLen;
			bTextExtend = GetTextExtentPoint32 (dc.m_hDC, (LPCTSTR)strTemplateChar, ilen, &size);
			m_pArrayColumnWidth[i] = (bTextExtend && size.cx > 50)? size.cx+10: 50;
			SetColumnWidth (i, m_pArrayColumnWidth[i]);
		}
	}

	BOOL bInterrupted = m_pQueryRowParam->IsInterrupted();
	if (bInterrupted)
	{
		m_pQueryRowParam->DoInterrupt();
	}

	m_bAllRows = !m_pQueryRowParam->IsMoreResult();
	if (!m_bAllRows)
		return 1;
	return 0;
}

//
// Just a convention:
// wParam = 1 (request to select all rows)
// wParam = 2 (request to update the float4 column, lParam = (bool)exponential)
// wParam = 3 (request to update the float8 column, lParam = (bool)exponential)
long CuListCtrlSelectResult::OnChangeSetting(WPARAM wParam, LPARAM lParam)
{
	int nWhat = (int)wParam;
	if (nWhat == 1)
	{
		if (!m_pQueryRowParam)
			return 0;
		if (!m_pQueryRowParam->IsMoreResult())
			return 0;

		//
		// Delete all the items of type 2 (empty items that indicate more tuples):
		int nCount = GetItemCount();
		for (int i = 0; i < nCount; i++)
		{
			if ((int)GetItemData(i) == 2)
			{
				while (GetItemCount() > i)
				{
					DeleteItem (i);
				}
				break;
			}
		}

		CaSession* pCurrentSession = m_pQueryRowParam->GetSession();
		ASSERT(pCurrentSession);
		if (pCurrentSession)
			pCurrentSession->SetSessionNone();

		m_pQueryRowParam->SetMessageHandler (m_hWnd);
		m_pQueryRowParam->SetRowLimit(0);
#if defined (_ANIMATION_)
		CxDlgWait dlgWait(m_strTitle, this);
		dlgWait.SetDeleteParam(FALSE);
		dlgWait.SetExecParam (m_pQueryRowParam);
		dlgWait.SetUseAnimateAvi(AVI_FETCHR);
		dlgWait.SetUseExtraInfo();
		dlgWait.SetShowProgressBar(FALSE);
		dlgWait.SetHideCancelBottonAlways(TRUE);
		dlgWait.DoModal();
#else
		m_pQueryRowParam->Run();
#endif
	}

	if (nWhat == 2)
	{
		BOOL bExponential = (BOOL)lParam;

	}
	
	if (nWhat == 3)
	{
		BOOL bExponential = (BOOL)lParam;


	}

	return 0;
}

