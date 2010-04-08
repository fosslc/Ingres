#ifndef DOMPROP_ROLE_HEADER
#define DOMPROP_ROLE_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DgDPusr.h : header file
//

#include "domseri.h"
#include "urpectrl.h"
#include "dmlurp.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropRole form view

class CuDlgDomPropRole : public CFormView
{
protected:
	CuDlgDomPropRole();
  DECLARE_DYNCREATE(CuDlgDomPropRole)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropRole)
	enum { IDD = IDD_DOMPROP_ROLE };
	CListBox	m_lbGranteesOnRole;
	CEdit	m_edLimSecureLabel;
	//}}AFX_DATA

	// Overrides
	virtual void OnInitialUpdate();
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropRole)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataRole m_Data;

	// Implementation
protected:
	CImageList m_ImageCheck;
	CuEditableListCtrlURPPrivileges m_cListPrivilege;
	BOOL m_bNeedSubclass;
	CaProfile m_profile;

	void FillPrivileges();
	void SubclassControls();

	virtual ~CuDlgDomPropRole();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropRole)
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

#endif  // DOMPROP_ROLE_HEADER
