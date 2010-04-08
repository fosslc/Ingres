#ifndef DOMPROP_DDB_HEADER
#define DOMPROP_DDB_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DgDPInt.h : header file
//

#include "domseri.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropDDb Form View

class CuDlgDomPropDDb : public CFormView
{
protected:
  CuDlgDomPropDDb();
  DECLARE_DYNCREATE(CuDlgDomPropDDb)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropDDb)
	enum { IDD = IDD_DOMPROP_DDB };
	CEdit	m_edCoordinator;
	CEdit	m_edLocChk;
	CEdit	m_edOwner;
	CEdit	m_edLocSrt;
	CEdit	m_edLocJnl;
	CEdit	m_edLocDmp;
	CEdit	m_edLocDb;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropDDb)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataDDb m_Data;

// Implementation
protected:
  virtual ~CuDlgDomPropDDb();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropDDb)
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

#endif  // DOMPROP_DDB_HEADER
