/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qpageraw.h, Header file    (Modeless Dialog)
**    Project  : CA-OpenIngres Visual DBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : It contains the result from execution of raw statement.
**
** History:
**
** xx-Feb-1998 (uk$so01)
**    Created
** 31-Jan-2000 (uk$so01)
**    (Bug #100235)
**    Special SQL Test Setting when running on Context.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 22-Fev-2002 (uk$so01)
**    SIR #107133. Use the select loop instead of cursor when we get
**    all rows at one time.
*/

#if !defined(AFX_QPAGERAW_H__15FBAEB1_9E0A_11D1_A249_00609725DDAF__INCLUDED_)
#define AFX_QPAGERAW_H__15FBAEB1_9E0A_11D1_A249_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "qresurow.h"


class CuDlgSqlQueryPageRaw : public CDialog
{
public:
	CuDlgSqlQueryPageRaw(CWnd* pParent = NULL);   // standard constructor
	void OnOK (){return;}
	void OnCancel(){return;}
	void NotifyLoad (CaQueryRawPageData* pData);
	void DisplayTraceLine (LPCTSTR strStatement, LPCTSTR lpszTrace, LPCTSTR lpszCursorName);
	CString GetLastCallerID(){return m_strCallerID;}

	// Dialog Data
	//{{AFX_DATA(CuDlgSqlQueryPageRaw)
	enum { IDD = IDD_SQLQUERY_PAGERAW };
	CEdit	m_cEdit1;
	CString	m_strEdit1;
	//}}AFX_DATA

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgSqlQueryPageRaw)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL
	
	//
	// Implementation
protected:
	int  m_nLine;         // Number of Lines in the Edit control
	int  m_nLength;       // Leng of the text in the Edit control
	BOOL m_bLimitReached; // The limit of buffer has been reached.
	CString m_strCallerID;// Usually the cursor name.

	// Generated message map functions
	//{{AFX_MSG(CuDlgSqlQueryPageRaw)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnGetData       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnDisplayResult (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetTabBitmap  (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnHighlightStatement (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnChangeSetting (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetSqlFont    (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QPAGERAW_H__15FBAEB1_9E0A_11D1_A249_00609725DDAF__INCLUDED_)
