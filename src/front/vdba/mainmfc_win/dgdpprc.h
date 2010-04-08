#ifndef DOMPROP_PROC_HEADER
#define DOMPROP_PROC_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DgDpPrc.h : header file
//

#include "domseri.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropProcedure form view

class CuDlgDomPropProcedure : public CFormView
{
protected:
	CuDlgDomPropProcedure();
  DECLARE_DYNCREATE(CuDlgDomPropProcedure)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropProcedure)
	enum { IDD = IDD_DOMPROP_PROC };
	CEdit	m_edText;
	CListBox	m_lbRulesExecutingProcedure;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropProcedure)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataProcedure m_Data;

// Implementation
protected:
    virtual ~CuDlgDomPropProcedure();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropProcedure)
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

#endif  // DOMPROP_PROC_HEADER
