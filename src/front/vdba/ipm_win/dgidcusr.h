/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgidcusr.h, Header file
**    Project  : INGRES II/ Ice Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control. The Ice Connected User Detail
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created
**
*/

#ifndef DGIDCUSR_HEADER
#define DGIDACSR_HEADER

class CuDlgIpmIceDetailConnectedUser : public CFormView
{
protected:
	CuDlgIpmIceDetailConnectedUser();
	DECLARE_DYNCREATE(CuDlgIpmIceDetailConnectedUser)
public:

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmIceDetailConnectedUser)
	enum { IDD = IDD_IPMICEDETAIL_CONNECTEDUSER };
	CString    m_strName;
	CString    m_strIceUser;
	CString    m_strActiveSession;
	CString    m_strTimeOut;
	//}}AFX_DATA
	ICE_CONNECTED_USERDATAMIN m_Struct;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmIceDetailConnectedUser)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmIceDetailConnectedUser();
	void ResetDisplay();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmIceDetailConnectedUser)
	virtual void OnInitialUpdate();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
