/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgidtran.h, Header file.
**    Project  : INGRES II/ Ice Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control. The Ice Transaction Detail
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created
**
*/

#ifndef DGIDTRAN_HEADER
#define DGIDTRAN_HEADER

class CuDlgIpmIceDetailTransaction : public CFormView
{
protected:
	CuDlgIpmIceDetailTransaction();
	DECLARE_DYNCREATE(CuDlgIpmIceDetailTransaction)
public:

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmIceDetailTransaction)
	enum { IDD = IDD_IPMICEDETAIL_TRANSACTION };
	CString m_strKey;
	CString m_strName;
	CString m_strUserSession;
	CString m_strConnectionKey;
	//}}AFX_DATA
	ICE_TRANSACTIONDATAMIN m_Struct;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmIceDetailTransaction)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmIceDetailTransaction();
	void ResetDisplay();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmIceDetailTransaction)
	virtual void OnInitialUpdate();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
