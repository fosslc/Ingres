/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgilocks.h, Header file.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: Detail page of Locks.
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
** 15-Apr-2004 (uk$so01)
**    BUG #112157 / ISSUE 13350172
**    Detail Page of Locked Table has an unexpected extra icons, this is
**    a side effect of change #462417.
*/


#ifndef DGILOCKS_HEADER
#define DGILOCKS_HEADER
#include "titlebmp.h"

class CuDlgIpmDetailLock : public CFormView
{
protected:
	CuDlgIpmDetailLock();
	DECLARE_DYNCREATE(CuDlgIpmDetailLock)
public:

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmDetailLock)
	enum { IDD = IDD_IPMDETAIL_LOCK };
	CuTitleBitmap m_transparentBmp;
	CString    m_strRQ;
	CString    m_strType;
	CString    m_strDatabase;
	CString    m_strPage;
	CString    m_strState;
	CString    m_strLockListID;
	CString    m_strTable;
	CString    m_strResourceID;
	//}}AFX_DATA
	LOCKDATAMAX m_lcStruct;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmDetailLock)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmDetailLock();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmDetailLock)
	virtual void OnInitialUpdate();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad   (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
