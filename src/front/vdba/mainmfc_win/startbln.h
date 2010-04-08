#if !defined(AFX_STARTBLN_H__F235B2F1_463C_11D1_A309_00609725DBFA__INCLUDED_)
#define AFX_STARTBLN_H__F235B2F1_463C_11D1_A309_00609725DBFA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// StarTbLn.h : header file
//

#include "vtreeglb.h"
#include "vtreectl.h"
#include "vtree3.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgStarTblRegister dialog

class CuDlgStarTblRegister : public CDialog
{
// Construction
public:
	CuDlgStarTblRegister(CWnd* pParent = NULL);   // standard constructor
  ~CuDlgStarTblRegister();

// Dialog Data
	//{{AFX_DATA(CuDlgStarTblRegister)
	enum { IDD = IDD_STAR_TABLEREGISTER };
	CButton	m_BtnReset;
	CButton	m_GroupboxSource;
	CTreeCtrl	m_Tree;
	CListCtrl	m_Columns;
	CStatic	m_StaticName;
	CEdit	m_EditName;
	//}}AFX_DATA

  CString m_destNodeName;
  CString m_destDbName;
  BOOL    m_bDragDrop;
  int     m_linkType;
  CString m_srcNodeName;
  CString m_srcDbName;
  CString m_srcObjName;
  CString m_srcOwnerName;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgStarTblRegister)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
	BOOL m_bPreventTreeOperation;
	void FillColumnsList(CTreeItem *pItem);
  CTreeGlobalData* m_pTreeGlobalData;

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgStarTblRegister)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnItemexpandingTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeDistname();
	afx_msg void OnBtnReset();
	afx_msg void OnBeginEditColumnName(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndEditColumnName(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangingTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBtnName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // !defined(AFX_STARTBLN_H__F235B2F1_463C_11D1_A309_00609725DBFA__INCLUDED_)
