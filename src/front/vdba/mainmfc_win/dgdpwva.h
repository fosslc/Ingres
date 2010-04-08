#ifndef DOMPROP_ICE_VARIABLE_HEADER
#define DOMPROP_ICE_VARIABLE_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DgDPInt.h : header file
//

#include "domseri3.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceVariable Form View

class CuDlgDomPropIceVariable : public CFormView
{
protected:
  CuDlgDomPropIceVariable();
  DECLARE_DYNCREATE(CuDlgDomPropIceVariable)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropIceVariable)
	enum { IDD = IDD_DOMPROP_ICE_SERVER_VARIABLE };
	CEdit	m_edVariableValue;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropIceVariable)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataIceVariable m_Data;

// Implementation
protected:
  virtual ~CuDlgDomPropIceVariable();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropIceVariable)
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

#endif  // DOMPROP_ICE_VARIABLE_HEADER
