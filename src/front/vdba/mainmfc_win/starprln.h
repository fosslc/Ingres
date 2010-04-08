/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

#ifndef STARPROCEDURELINK_HEADER
#define STARPROCEDURELINK_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/*
**
**  Name: StarTbLn.h
**
**  Description:
**	This header file defines the CuDlgStarProcRegister class.
**
**  History:
**	16-feb-2000 (somsa01)
**	    Properly commented STARPROCEDURELINK_HEADER.
*/

#include "vtreeglb.h"
#include "vtreectl.h"
#include "vtree3.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgStarProcRegister dialog

class CuDlgStarProcRegister : public CDialog
{
// Construction
public:
	CuDlgStarProcRegister(CWnd* pParent = NULL);   // standard constructor
  ~CuDlgStarProcRegister();

// Dialog Data
	//{{AFX_DATA(CuDlgStarProcRegister)
	enum { IDD = IDD_STAR_PROCREGISTER };
	CButton	m_GroupboxSource;
	CTreeCtrl	m_Tree;
	CStatic	m_StaticName;
	CEdit	m_EditName;
	//}}AFX_DATA

  CString m_destNodeName;
  CString m_destDbName;
  BOOL    m_bDragDrop;
  CString m_srcNodeName;
  CString m_srcDbName;
  CString m_srcObjName;
  CString m_srcOwnerName;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgStarProcRegister)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  BOOL  m_bPreventTreeOperation;
  CTreeGlobalData* m_pTreeGlobalData;

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgStarProcRegister)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnItemexpandingTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangingTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeDistname();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif  /* STARPROCEDURELINK_HEADER */

