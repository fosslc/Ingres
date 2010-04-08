/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : lsselres.h, header file
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
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 22-Aug-2003 (uk$so01)
**    SIR #106648, Add silent mode in IEA. 
*/

#if !defined(AFX_LSSELRES_H__23DD3B71_A12A_11D1_A24A_00609725DDAF__INCLUDED_)
#define AFX_LSSELRES_H__23DD3B71_A12A_11D1_A24A_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "calsctrl.h"
#include "synchron.h" // CaSychronizeInterrupt
#include "tkwait.h"
#include "sqlselec.h"


class CaRowTransfer;
typedef void (CALLBACK* PfnUserManageQueryResult) (LPVOID pUserData, CaRowTransfer* pResultRow);

//
// FETCH ROW:
// CLASS: CaExecParamQueryRows
// ************************************************************************************************
class CaCursor;
class CaSession;

//
// FETCH ROW: (using select loop instead of cursor)
// CLASS: CaExecParamSelectLoop
// ************************************************************************************************
class CaExecParamSelectLoop: public CaExecParam
{
public:
	enum 
	{
		FETCH_ERROR = 0, 
		FETCH_TRACEBEGIN,       // Associated with WPARAM to nofify parent window of begining trace
		FETCH_TRACEINFO,        // Associated with WPARAM to nofify parent window of info trace
		FETCH_TRACEEND,         // Associated with WPARAM to nofify parent window of ending trace
		FETCH_NORMAL_ENDING, 
		FETCH_REACH_LIMIT, 
		FETCH_INTERRUPTED
	};
	CaExecParamSelectLoop();
	CaExecParamSelectLoop(UINT nInteruptType);
	virtual ~CaExecParamSelectLoop();
	BOOL IsInterrupted(){return m_bInterrupted;}

	void Initialize();
	void SetStatement(LPCTSTR lpszStatement){m_strStatement = lpszStatement;}
	void SetSession (CaSession* pSession){m_pSession = pSession;}
	void ConstructListHeader(PVOID* ppSqlDa = NULL);

	virtual int Run(HWND hWndTaskDlg = NULL);
	virtual void OnTerminate(HWND hWndTaskDlg = NULL);
	virtual BOOL OnCancelled(HWND hWndTaskDlg = NULL);

	CTypedPtrList< CObList, CaColumn* >& GetListHeader(){return m_listHeader;}
	CaSession* GetSession(){return m_pSession;}
	//
	// The window that handles the user message sent/posted by the Thread that
	// called the member Run().
	// Ex: the message 'WMUSRMSG_UPDATEDATA' is associated with the row in LPARAM.
	void SetMessageHandler(HWND hWnd){m_hWnd = hWnd;}

	void SetShowRawPage (BOOL bSet){m_bUseTrace = bSet;}
	void SetRowLimit (int nLimit){m_nRowLimit = nLimit;}
	BOOL IsShowRawPage(){return m_bUseTrace;}
	int  GetRowLimit (){return m_nRowLimit;}

	CStringList& GetTrace(){return m_strlTrace;}
	int  GetFetchStatus(){return m_nEndFetchStatus;}
	CaFloatFormat& GetFloatFormat(){return m_fetchInfo;}
	void SetStrings (LPCTSTR lpszMsgBoxTitle, LPCTSTR lpszInterruptFetching, LPCTSTR lpszstrFetchInfo)
	{
		m_strMsgBoxTitle = lpszMsgBoxTitle;
		m_strInterruptFetching = lpszInterruptFetching;
		m_strFetchInfo = lpszstrFetchInfo;
	}
	void SetUserHandlerResult(LPVOID pUserData, PfnUserManageQueryResult pUserFunction)
	{
		m_pUserData = pUserData;
		m_pfnUserHandlerResult = pUserFunction;
	}
	CString GetErrorMessageText(){return m_strMsgFailed;}

protected:
	CString m_strStatement;
	//
	// String used by the Taskanimat.dll:
	CString m_strMsgBoxTitle;
	CString m_strMsgFailed;;
	CString m_strInterruptFetching;
	CString m_strFetchInfo;
	HWND    m_hWnd;
	CaSychronizeInterrupt m_synchronizeInterrupt;
	CaSession* m_pSession;    // Copy pointer only (do not delete)
	//
	// m_nEndFetchStatus = FETCH_ERROR  : (An error has occurred)
	//                     FETCH_NORMAL_ENDING: normal ending (attempt to fectch past the last row)
	//                     FETCH_REACH_LIMIT  : Reach the limit
	//                     FETCH_INTERRUPTED  : Interrupted.
	int  m_nEndFetchStatus; 
	BOOL m_bUseTrace;
	int  m_nRowLimit;     // The limit of rows to be displayed in the control. (0=Nolimit)
	int  m_nAccumulation; // Accumulate the fetched rows.

	CStringList  m_strlTrace;

	//
	// User handler of result set:
	// Each fetched row, 'm_pfnUserHandlerResult' will be called 
	// m_pfnUserHandlerResult (m_pUserData, pNewTransferRow). 'pNewTransferRow'
	// is a newly fetched row.
	LPVOID m_pUserData;
	PfnUserManageQueryResult m_pfnUserHandlerResult;

	CaDecimalFormat m_decimalFormat;
	CaMoneyFormat   m_moneyFormat;
	CaFloatFormat   m_fetchInfo;
	CTypedPtrList< CObList, CaColumn* > m_listHeader;
	CTypedPtrList< CObList, CaBufferColumn* > m_listBufferColumn;

protected:
	BOOL SelectLoop(HWND hWndTaskDlg = NULL);

};


//
// FETCH ROW: (using select with cursor)
// CLASS: CaExecParamSelectCursor
//        The inherited m_fetchInfo will not be used and m_pCursor must have its own m_cursorInfo
// ************************************************************************************************
class CaExecParamSelectCursor: public CaExecParamSelectLoop
{
public:
	CaExecParamSelectCursor();
	CaExecParamSelectCursor(UINT nInteruptType);
	virtual ~CaExecParamSelectCursor();
	void Initialize();

	virtual int Run(HWND hWndTaskDlg = NULL);
	void OpenCursor();
	void CloseCursor();
	CaCursor* GetCursor(){return m_pCursor;}
	CaSession* GetSession(){return m_pSession;}
	void SetCursor (CaCursor* pCursor);
	BOOL IsFetchMore(){return m_bFetchMore;}
	void SetFetchMore(){m_bFetchMore = TRUE;}
	int  GetFirstV2Index() {ASSERT (m_bFetchMore); return m_nFirstV2;}
	void SetFirstV2Index(int nFirstV2) {ASSERT (m_bFetchMore); m_nFirstV2 = nFirstV2;}


protected:
	CaCursor*  m_pCursor; // Must delete this pointer
	BOOL m_bFetchMore;    // Refetch rows after the limit is reached
	int  m_nFirstV2;      // Available only if SetFetchMore();
	BOOL m_bClose;        // TRUE if the cursor has been close;
};


class CaExecParamQueryRows: public CaExecParamSelectCursor
{
public:
	CaExecParamQueryRows(BOOL bCursor);
	CaExecParamQueryRows(BOOL bCursor, UINT nInteruptType);
	virtual ~CaExecParamQueryRows();
	void Initialize();

	virtual int Run(HWND hWndTaskDlg = NULL);
	BOOL IsUsedCursor(){return m_bUseCursor;}
	void DoInterrupt();
	BOOL IsMoreResult();
	CString GetCursorName();
	void ConstructListHeader();
	CTypedPtrList< CObList, CaColumn* >& GetListHeader();

protected:
	BOOL m_bUseCursor;
	BOOL m_bConstructedHeader;
};


//
// CLASS: CuListCtrlSelectResult
// ************************************************************************************************
class CuListCtrlSelectResult : public CuListCtrl
{
public:
	CuListCtrlSelectResult();
	void SetQueryRowParam(CaExecParamQueryRows* pParam){m_pQueryRowParam = pParam;}
	CaExecParamQueryRows* GetQueryRowParam() {return m_pQueryRowParam;}

	void SortColumn(int nCol);
	//
	// DisplayResult:
	// This function returns 0 if the cursor is closed (fetched till the end), otherwise the cursor
	long DisplayResult(); 
	void SetShowRowNumber(BOOL bShow = TRUE){m_bShowRowNumber = bShow;};

	CString& GetNoMoreAvailableString(){return m_strNoMoreAvailable;}
	CString& GetNotAvailableString(){return m_strNotAvailable;}
	CString& GetTitle(){return m_strTitle;}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuListCtrlSelectResult)
	//}}AFX_VIRTUAL

	//
	// Implementation
public:
	virtual ~CuListCtrlSelectResult();
protected:
	CaExecParamQueryRows* m_pQueryRowParam;
	CArray <int, int> m_nHeaderType;
	BOOL m_bShowRowNumber;
	BOOL m_bAsc;
	BOOL m_bAllRows; // When all rows are fetched at one time;
	BOOL m_bFirstRefetch;
	int  m_nArraySize;
	int  m_nSortColumn;
	int* m_pArrayColumnWidth;
	int  m_nColumn;
	long m_nRow;
	long m_nRowDeleted;
	int  m_nFirstItemV2;

	CString m_strNoMoreAvailable;
	CString m_strNotAvailable;
	CString m_strTitle;
	CString m_strOutOfMemory;
	// Generated message map functions
protected:
	void AddLineItem (int iSubItem, LPCTSTR lpValue);
	void MoreTupleIndicator();
	void OldTupleIndicator();


	//{{AFX_MSG(CuListCtrlSelectResult)
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	afx_msg LONG OnDisplayRow  (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnEndFetchRows(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnFetchNewRows(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnChangeSetting(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LSSELRES_H__23DD3B71_A12A_11D1_A24A_00609725DDAF__INCLUDED_)
