#ifndef DOMPROP_PROF_HEADER
#define DOMPROP_PROF_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DgDPusr.h : header file
//

#include "domseri.h"
#include "urpectrl.h"
#include "dmlurp.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropProfile form view

class CuDlgDomPropProfile : public CFormView
{
protected:
	CuDlgDomPropProfile();
  DECLARE_DYNCREATE(CuDlgDomPropProfile)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropProfile)
	enum { IDD = IDD_DOMPROP_PROF };
	CEdit	m_edLimSecureLabel;
	CEdit	m_edExpireDate;
	CEdit	m_edDefGroup;
	//}}AFX_DATA

	// Overrides
	virtual void OnInitialUpdate();
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropProfile)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataProfile m_Data;

	// Implementation
protected:
	CImageList m_ImageCheck;
	CuEditableListCtrlURPPrivileges m_cListPrivilege;
	BOOL m_bNeedSubclass;
	CaProfile m_profile;

	void FillPrivileges();
	void SubclassControls();
	virtual ~CuDlgDomPropProfile();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropProfile)
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

#endif  // DOMPROP_PROF_HEADER
