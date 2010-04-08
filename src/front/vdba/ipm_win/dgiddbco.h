/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgiddbco.h, Header file
**    Project  : INGRES II/ Ice Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control. The Ice Datbase Connection Detail.
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created
**
*/

#ifndef DGIDDBCO_HEADER
#define DGIDDBCO_HEADER

class CuDlgIpmIceDetailDatabaseConnection : public CFormView
{
protected:
	CuDlgIpmIceDetailDatabaseConnection();
	DECLARE_DYNCREATE(CuDlgIpmIceDetailDatabaseConnection)
public:

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmIceDetailDatabaseConnection)
	enum { IDD = IDD_IPMICEDETAIL_DATABASECONNECTION };
	CString m_strKey;
	CString m_strDriver;
	CString m_strDatabaseName;
	CString m_strTimeOut;
	BOOL m_bCurrentlyUsed;
	//}}AFX_DATA
	ICE_DB_CONNECTIONDATAMIN m_Struct;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmIceDetailDatabaseConnection)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmIceDetailDatabaseConnection();
	void ResetDisplay();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmIceDetailDatabaseConnection)
	virtual void OnInitialUpdate();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
