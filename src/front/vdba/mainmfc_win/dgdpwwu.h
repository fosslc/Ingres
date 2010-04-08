#ifndef DOMPROP_ICE_WEB_USER_HEADER
#define DOMPROP_ICE_WEB_USER_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DgDPInt.h : header file
//

#include "domseri3.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceWebuser Form View

class CuDlgDomPropIceWebuser : public CFormView
{
protected:
  CuDlgDomPropIceWebuser();
  DECLARE_DYNCREATE(CuDlgDomPropIceWebuser)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropIceWebuser)
	enum { IDD = IDD_DOMPROP_ICE_WEBUSER };
	CButton	m_chkUnitMgrPriv;
	CEdit	m_edtimeoutms;
	CButton	m_chkSecurityPriv;
	CButton	m_chkMonitorPriv;
	CEdit	m_edComment;
	CEdit	m_edDefDBUser;
	CButton	m_chkAdminPriv;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropIceWebuser)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataIceWebuser m_Data;

// Implementation
protected:
  virtual ~CuDlgDomPropIceWebuser();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropIceWebuser)
  afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG

  afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif  // DOMPROP_ICE_WEB_USER_HEADER
