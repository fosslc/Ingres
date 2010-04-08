#ifndef DOMPROP_ICE_SEC_DBUSER_HEADER
#define DOMPROP_ICE_SEC_DBUSER_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
//

#include "domseri3.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceSecDbuser Form View

class CuDlgDomPropIceSecDbuser : public CFormView
{
protected:
  CuDlgDomPropIceSecDbuser();
  DECLARE_DYNCREATE(CuDlgDomPropIceSecDbuser)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropIceSecDbuser)
	enum { IDD = IDD_DOMPROP_ICE_SEC_DBUSER };
	CButton	m_chkUnitRead;
	CButton	m_chkReadDoc;
	CButton	m_chkCreateDoc;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropIceSecDbuser)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataIceSecDbuser m_Data;

// Implementation
protected:
  virtual ~CuDlgDomPropIceSecDbuser();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropIceSecDbuser)
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

#endif  // DOMPROP_ICE_SEC_DBUSER_HEADER
