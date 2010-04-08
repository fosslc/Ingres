#if !defined(AFX_STARLOTB_H__2941CF81_5B79_11D1_A31F_00609725DBFA__INCLUDED_)
#define AFX_STARLOTB_H__2941CF81_5B79_11D1_A31F_00609725DBFA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// StarLoTb.h : header file
//

#include "vtreeglb.h"
#include "vtreectl.h"
#include "vtree3.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgStarLocalTable dialog

class CuDlgStarLocalTable : public CDialog
{
// Construction
public:
	BOOL m_bDefaultName;
	BOOL m_bCoordinator;
  BOOL m_bLocalNodeIsLocal;
	CuDlgStarLocalTable(CWnd* pParent = NULL);   // standard constructor
	~CuDlgStarLocalTable();

// Dialog Data
	//{{AFX_DATA(CuDlgStarLocalTable)
	enum { IDD = IDD_STAR_TABLECREATE };
	CButton	m_CheckDefaultName;
	CButton	m_RadCoordinator;
	CTreeCtrl	m_Tree;
	CEdit	m_EditName;
	//}}AFX_DATA

  CString m_CurrentNodeName;
  CString m_CurrentDbName;
  CString m_CurrentCDBName;
  CString m_CurrentTblName;
  BOOL  bLocalNodeIsLocal;

  CString m_LocalNodeName;
  CString m_LocalDbName;
  CString m_LocalTableName;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgStarLocalTable)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
	void UpdateCustomInfo(BOOL bMustFit);
	void PreselectTreeItem();
	BOOL MustPreventTreeOperation();
  BOOL m_bIgnorePreventTreeOperation;
  void IgnorePreventTreeOperation()     { m_bIgnorePreventTreeOperation = TRUE;  }
  void AuthorizePreventTreeOperation()  { m_bIgnorePreventTreeOperation = FALSE; }

	void ManageControlsState();
  CTreeGlobalData* m_pTreeGlobalData;

  BOOL m_bStateDisabled;
  BOOL IsControlsStateManagementDisabled() { return m_bStateDisabled; }
  void DisableControlsState() { ASSERT (!m_bStateDisabled); m_bStateDisabled = TRUE; }
  void EnableControlsState()  { ASSERT (m_bStateDisabled); m_bStateDisabled = FALSE; }

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgStarLocalTable)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDestroy();
	afx_msg void OnItemexpandingTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRadioCoordinator();
	afx_msg void OnRadioCustom();
	afx_msg void OnCheckUsedefaultname();
	afx_msg void OnChangeLocalname();
	afx_msg void OnSelchangingTree(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STARLOTB_H__2941CF81_5B79_11D1_A31F_00609725DBFA__INCLUDED_)
