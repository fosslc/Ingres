/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgitrans.h, Header file.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control: Detail page of Transaction
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
**/


#ifndef DGITRANS_HEADER
#define DGITRANS_HEADER

class CuDlgIpmDetailTransaction : public CFormView
{
protected:
	CuDlgIpmDetailTransaction();
	DECLARE_DYNCREATE(CuDlgIpmDetailTransaction)

public:
	// Dialog Data
	//{{AFX_DATA(CuDlgIpmDetailTransaction)
	enum { IDD = IDD_IPMDETAIL_TRANSACTION };
	CString    m_strDatabase;
	CString    m_strStatus;
	CString    m_strWrite;
	CString    m_strSession;
	CString    m_strSplit;
	CString    m_strForce;
	CString    m_strWait;
	CString    m_strInternalPID;
	CString    m_strExternalPID;
	CString    m_strFirst;
	CString    m_strLast;
	CString    m_strCP;
	CString m_strInternalTXID;
	//}}AFX_DATA
	LOGTRANSACTDATAMAX m_trStruct;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmDetailTransaction)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmDetailTransaction();
	void ResetDisplay();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmDetailTransaction)
	virtual void OnInitialUpdate();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
