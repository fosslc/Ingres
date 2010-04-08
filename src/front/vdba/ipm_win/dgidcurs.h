/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgidcurs.h, Header file
**    Project  : INGRES II/ Ice Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control. The Ice Cursor Detail
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created
**
*/

#ifndef DGIDCURS_HEADER
#define DGIDCURS_HEADER

class CuDlgIpmIceDetailCursor : public CFormView
{
protected:
	CuDlgIpmIceDetailCursor();
	DECLARE_DYNCREATE(CuDlgIpmIceDetailCursor)
public:

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmIceDetailCursor)
	enum { IDD = IDD_IPMICEDETAIL_CURSOR };
	CString m_strKey;
	CString m_strName;
	CString m_strOwner;
	CString m_strQuery;
	//}}AFX_DATA
	ICE_CURSORDATAMIN m_Struct;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmIceDetailCursor)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmIceDetailCursor();
	void ResetDisplay();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmIceDetailCursor)
	virtual void OnInitialUpdate();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
