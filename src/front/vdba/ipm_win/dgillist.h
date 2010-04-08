/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgillist.h, Header file.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: Detail page of Lock Lists
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
*/

#ifndef DGILLIST_HEADER
#define DGILLIST_HEADER


class CuDlgIpmDetailLockList : public CFormView
{
protected:
	CuDlgIpmDetailLockList();
	DECLARE_DYNCREATE(CuDlgIpmDetailLockList)
public:
	// Dialog Data
	//{{AFX_DATA(CuDlgIpmDetailLockList)
	enum { IDD = IDD_IPMDETAIL_LOCKLIST };
	CString    m_strSession;
	CString    m_strStatus;
	CString    m_strTransID;
	CString    m_strTotal;
	CString    m_strLogical;
	CString    m_strMaxLogical;
	CString    m_strRelatedLockListID;
	CString    m_strCurrent;
	CString    m_strWaitingResourceListID;
	//}}AFX_DATA
	LOCKLISTDATAMAX m_llStruct;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmDetailLockList)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmDetailLockList();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmDetailLockList)
	virtual void OnInitialUpdate();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

#endif // DGILLIST_HEADER
