/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: DgDpTb.h
**
**  Description:
**
**  History:
**   24-Oct-2000 (schph01)
**   sir 102821 Comment on table and columns.
*/
#ifndef DOMPROP_TABLE_HEADER
#define DOMPROP_TABLE_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DgDPTb.h : header file
//

#include "domseri.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropTable dialog

class CuDlgDomPropTable : public CFormView
{
protected:
  CuDlgDomPropTable();
  DECLARE_DYNCREATE(CuDlgDomPropTable)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropTable)
	enum { IDD = IDD_DOMPROP_TABLE };
	CEdit	m_edComment;
	CStatic	m_stUniqStmt;
	CStatic	m_stUniqRow;
	CStatic	m_stUniqNo;
	CStatic	m_stDupRows;
	CStatic	m_stStructUniq;
	CStatic	m_sticon_expire;
	CEdit	m_edExpireDate;
	CEdit	m_edTotalSize;
	CStatic	m_sticon_bigtable;
	CStatic	m_stStruct2;
	CStatic	m_stStruct1;
	CStatic	m_stPgSize;
	CStatic	m_stFillFact;
	CEdit	m_edStruct2;
	CEdit	m_edStruct1;
	CEdit	m_edStructure;
	CEdit	m_edSchema;
	CEdit	m_edPgSize;
	CEdit	m_edPgAlloc;
	CEdit	m_edEstimNbRows;
	CEdit	m_edFillFact;
	CEdit	m_edExtendPg;
  CEdit m_edAllocatedPages;
	CButton	m_chkDupRows;
	CButton	m_ReadOnly;
	//}}AFX_DATA

  // constraints list control
  CuListCtrlCheckmarks  m_cListConstraints;
  CImageList            m_ImageList2;
  CImageList            m_ImageCheck2;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropTable)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void  RefreshDisplay();
  void  AddTblConstraint(CuTblConstraint* pTblConstraint);

  // Mandatory for controls subclass good management whatever the pane is created or loaded
  BOOL  m_bNeedSubclass;
  void SubclassControls();

private:
  CuDomPropDataTable m_Data;

// Implementation
protected:
  virtual ~CuDlgDomPropTable();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropTable)
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

#endif  // DOMPROP_TABLE_HEADER
