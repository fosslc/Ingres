/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgidhttp.h, Header file
**    Project  : INGRES II/ Ice Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control. The Ice Http Server Connection Detail
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created
**
*/


#ifndef DGIDHTTP_HEADER
#define DGIDHTTP_HEADER

class CuDlgIpmIceDetailHttpServerConnection : public CFormView
{
protected:
	CuDlgIpmIceDetailHttpServerConnection();
	DECLARE_DYNCREATE(CuDlgIpmIceDetailHttpServerConnection)
public:

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmIceDetailHttpServerConnection)
	enum { IDD = IDD_IPMICEDETAIL_HTTPSERVERCONNECTION };
	CString m_strRealUser;
	CString m_strID;
	CString m_strState;
	CString m_strWaitReason;
	//}}AFX_DATA
	SESSIONDATAMIN m_Struct;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmIceDetailHttpServerConnection)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmIceDetailHttpServerConnection();
	void ResetDisplay();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmIceDetailHttpServerConnection)
	virtual void OnInitialUpdate();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
