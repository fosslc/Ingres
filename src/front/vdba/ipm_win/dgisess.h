/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgisess.h, Header file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control: Detail page of Session
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
** 25-Jan-2000 (uk$so01)
**    SIR #100118: Add a description of the session.
**/


#ifndef DGISSESS_HEADER
#define DGISSESS_HEADER

class CuDlgIpmDetailSession : public CFormView
{
protected:
	CuDlgIpmDetailSession();
	DECLARE_DYNCREATE(CuDlgIpmDetailSession)

public:
	// Dialog Data
	//{{AFX_DATA(CuDlgIpmDetailSession)
	enum { IDD = IDD_IPMDETAIL_SESSION };
	CStatic	m_cStaticDescription;
	CEdit	m_cEditDescription;
	CString    m_strName;
	CString    m_strState;
	CString    m_strMask;
	CString    m_strTerminal;
	CString    m_strID;
	CString    m_strUserReal;
	CString    m_strUserApparent;
	CString    m_strDBName;
	CString    m_strDBA;
	CString    m_strGroupID;
	CString    m_strRoleID;
	CString    m_strServerType;
	CString    m_strServerFacility;
	CString    m_strServerListenAddress;
	CString    m_strActivitySessionState;
	CString    m_strActivityDetail;
	CString    m_strQuery;
	CString	m_strDescription;
	//}}AFX_DATA
	SESSIONDATAMAX m_ssStruct;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmDetailSession)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmDetailSession();
	void ResetDisplay();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmDetailSession)
	virtual void OnInitialUpdate();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad  (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

#endif // DGISSESS_HEADER
