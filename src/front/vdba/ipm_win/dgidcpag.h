/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgidcpag.h, Header file. 
**    Project  : INGRES II/ Monitoring
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control. The Ice Cache Page Detail
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created
**
*/


#ifndef DGIDCPAG_HEADER
#define DGIDCPAG_HEADER

class CuDlgIpmIceDetailCachePage : public CFormView
{
protected:
	CuDlgIpmIceDetailCachePage();
	DECLARE_DYNCREATE(CuDlgIpmIceDetailCachePage)
public:

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmIceDetailCachePage)
	enum { IDD = IDD_IPMICEDETAIL_CACHEPAGE };
	CString m_strName;
	CString m_strLocation;
	CString m_strRequester;
	CString m_strCounter;
	CString m_strTimeOut;
	CString m_strStatus;
	BOOL m_bUsable;
	BOOL m_bExistingFile;
	//}}AFX_DATA
	ICE_CACHEINFODATAMIN m_Struct;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmIceDetailCachePage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmIceDetailCachePage();
	void ResetDisplay();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmIceDetailCachePage)
	virtual void OnInitialUpdate();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
