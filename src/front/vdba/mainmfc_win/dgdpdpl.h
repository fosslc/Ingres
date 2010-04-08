#ifndef DOMPROP_STARPROC_L_HEADER
#define DOMPROP_STARPROC_L_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DgDpPrc.h : header file
//

#include "domseri.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropProcLink form view

class CuDlgDomPropProcLink : public CFormView
{
protected:
	CuDlgDomPropProcLink();
  DECLARE_DYNCREATE(CuDlgDomPropProcLink)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropProcLink)
	enum { IDD = IDD_DOMPROP_STARPROC_L };
	CEdit	m_edText;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropProcLink)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataProcedure m_Data;

// Implementation
protected:
    virtual ~CuDlgDomPropProcLink();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropProcLink)
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

#endif  // DOMPROP_STARPROC_L_HEADER
