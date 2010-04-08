#ifndef DOMPROP_ICE_DBUSER_HEADER
#define DOMPROP_ICE_DBUSER_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DgDPInt.h : header file
//

#include "domseri3.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceDbuser Form View

class CuDlgDomPropIceDbuser : public CFormView
{
protected:
  CuDlgDomPropIceDbuser();
  DECLARE_DYNCREATE(CuDlgDomPropIceDbuser)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropIceDbuser)
	enum { IDD = IDD_DOMPROP_ICE_DBUSER };
	CEdit	m_edComment;
	CEdit	m_edAlias;
	CEdit	m_edName;

	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropIceDbuser)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataIceDbuser m_Data;

// Implementation
protected:
  virtual ~CuDlgDomPropIceDbuser();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropIceDbuser)
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

#endif  // DOMPROP_ICE_DBUSER_HEADER
