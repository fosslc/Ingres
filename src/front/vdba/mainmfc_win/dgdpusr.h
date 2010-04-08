/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**
**    Project : CA/OpenIngres Visual DBA
**    Source : dgdpusr.h
**
**  History:
**  15-Jun-2001(schph01)
**     SIR 104991 add new controls to display the rmcmd grants.
**
*/
#ifndef DOMPROP_USER_HEADER
#define DOMPROP_USER_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "domseri.h"
#include "urpectrl.h"
#include "dmlurp.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropUser form view

class CuDlgDomPropUser : public CFormView
{
protected:
	CuDlgDomPropUser();
  DECLARE_DYNCREATE(CuDlgDomPropUser)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropUser)
	enum { IDD = IDD_DOMPROP_USER };
	CButton	m_RmcmdTitle;
	CButton	m_ctrlCheckRmcmd;
	CStatic	m_ctrlIconPartial;
	CStatic	m_ctrlUserGranted;
	CStatic	m_ctrlPartialGrant;
	CListBox	m_lbGroupsContainingUser;
	CEdit	m_edLimSecureLabel;
	CEdit	m_edExpireDate;
	CEdit	m_edDefProfile;
	CEdit	m_edDefGroup;
	//}}AFX_DATA

	// Overrides
	virtual void OnInitialUpdate();
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropUser)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataUser m_Data;

	// Implementation
protected:
	CImageList m_ImageCheck;
	CuEditableListCtrlURPPrivileges m_cListPrivilege;
	BOOL m_bNeedSubclass;
	CaProfile m_profile;

	void FillPrivileges();
	void SubclassControls();
	virtual ~CuDlgDomPropUser();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropUser)
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

#endif  // DOMPROP_USER_HEADER
