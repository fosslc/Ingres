#ifndef DOMPROP_ICE_BUNIT_HEADER
#define DOMPROP_ICE_BUNIT_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
//

#include "domseri3.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceBunit Form View

class CuDlgDomPropIceBunit : public CFormView
{
protected:
  CuDlgDomPropIceBunit();
  DECLARE_DYNCREATE(CuDlgDomPropIceBunit)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropIceBunit)
	enum { IDD = IDD_DOMPROP_ICE_BUNIT };
	CEdit	m_edOwner;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropIceBunit)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataIceBunit m_Data;

// Implementation
protected:
  virtual ~CuDlgDomPropIceBunit();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropIceBunit)
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

#endif  // DOMPROP_ICE_BUNIT_HEADER
