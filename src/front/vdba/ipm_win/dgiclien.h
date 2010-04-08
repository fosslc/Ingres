/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgiclien.h, Header file
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : UK Sotheavut
**    Purpose  : Page of Table control: Client page of Session.(host, user, terminal,.)
**
** History:
**
** xx-Jun-1997 (uk$so01)
**    Created.
**
*/

#ifndef DGICLIEN_HEADER
#define DGICLIEN_HEADER
class CuDlgIpmPageClient : public CFormView
{
protected:
	CuDlgIpmPageClient();
	DECLARE_DYNCREATE(CuDlgIpmPageClient)

public:
	// Dialog Data
	//{{AFX_DATA(CuDlgIpmPageClient)
	enum { IDD = IDD_IPMPAGE_CLIENT };
	CString    m_strHost;
	CString    m_strUser;
	CString    m_strConnectedString;
	CString    m_strTerminal;
	CString    m_strPID;
	//}}AFX_DATA
	SESSIONDATAMAX m_ssStruct;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmPageClient)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmPageClient();
	void ResetDisplay();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmPageClient)
	virtual void OnInitialUpdate();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad  (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData  (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
