/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgidausr.h, Header file.
**    Project  : INGRES II/ Ice Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control. The Ice Active User Detail
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created.
*/

#ifndef DGIDAUSR_HEADER
#define DGIDAUSR_HEADER

class CuDlgIpmIceDetailActiveUser : public CFormView
{
protected:
	CuDlgIpmIceDetailActiveUser();
	DECLARE_DYNCREATE(CuDlgIpmIceDetailActiveUser)
public:

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmIceDetailActiveUser)
	enum { IDD = IDD_IPMICEDETAIL_ACTIVEUSER };
	CString m_strUser;
	CString m_strSession;
	CString m_strQuery;
	CString m_strHttpHostServer;
	CString m_strError;
	//}}AFX_DATA
	ICE_ACTIVE_USERDATAMIN m_Struct;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmIceDetailActiveUser)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmIceDetailActiveUser();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	void ResetDisplay();

	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmIceDetailActiveUser)
	virtual void OnInitialUpdate();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
