/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qpageqin.h, Header file    (Modeless Dialog) 
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : It contains a a scroll view for drawing the tree of qep.
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#if !defined(AFX_QPAGEQIN_H__73FD5E71_7785_11D1_A233_00609725DDAF__INCLUDED_)
#define AFX_QPAGEQIN_H__73FD5E71_7785_11D1_A233_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "qepdoc.h"
#include "qepframe.h"


class CuDlgSqlQueryPageQepIndividual : public CDialog
{
public:
	CuDlgSqlQueryPageQepIndividual(CWnd* pParent = NULL);
	void OnOK (){return;}
	void OnCancel(){return;}
	void ResizeControls();

	// Dialog Data
	//{{AFX_DATA(CuDlgSqlQueryPageQepIndividual)
	enum { IDD = IDD_SQLQUERY_PAGEQEP_INDIVIDUAL };
	CStatic	m_cStaticFrame;
	CStatic	m_cStaticExpression;
	CStatic	m_cStaticAggregate;
	CListBox	m_cList2;
	CListBox	m_cList1;
	BOOL	m_bTimeOut;
	BOOL	m_bLargeTemporaries;
	BOOL	m_bFloatIntegerException;
	CString	m_strTable;
	//}}AFX_DATA

	CStringList* m_pListAggregate;
	CStringList* m_pListExpression;

	CdQueryExecutionPlanDoc* m_pDoc;
	POSITION m_nQepSequence;
	BOOL m_bPreviewMode;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgSqlQueryPageQepIndividual)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CfQueryExecutionPlanFrame* m_pQepFrame;
	CString m_strNormal;
	CString m_strPreview;

	// Generated message map functions
	//{{AFX_MSG(CuDlgSqlQueryPageQepIndividual)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPreview();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QPAGEQIN_H__73FD5E71_7785_11D1_A233_00609725DDAF__INCLUDED_)
