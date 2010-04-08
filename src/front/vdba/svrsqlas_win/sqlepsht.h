/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlepsht.h, Header File 
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generate the sql expression
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 14-Apr-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (sqlassis.chm file)
**    Activate Help Button.
*/

#if !defined(AFX_SQLEPSHT_H__25100EC1_FF90_11D1_A281_00609725DDAF__INCLUDED_)
#define AFX_SQLEPSHT_H__25100EC1_FF90_11D1_A281_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//
// When you choose the finish button, the property sheet returns 12325
// instead of IDOK. I don't know why ...
#define BUG_PROPPERTY_SHEET_FINISH 12325
//
// This is a control ID of help button (I use the SPY++ to detect it)
#define ID_W_HELP 9

#include "sqlemain.h" // Main page (function choice)
#include "sqlefpar.h" // Function Parameters
#include "sqlecpar.h" // Comparaison
#include "sqleapar.h" // Aggregate Parameters
#include "sqleepar.h" // Any/All/Exist Parameters
#include "sqleipar.h" // In Parameters
#include "sqleopar.h" // Database Object

void FillFunctionParameters (LPGENFUNCPARAMS lpFparam, CaSQLComponent* lpComp);
CString GetConstantAllDistinct(BOOL bAll);


/////////////////////////////////////////////////////////////////////////////
// CxDlgPropertySheetSqlExpressionWizard

class CxDlgPropertySheetSqlExpressionWizard : public CPropertySheet
{
	DECLARE_DYNAMIC(CxDlgPropertySheetSqlExpressionWizard)

public:
	CxDlgPropertySheetSqlExpressionWizard(CWnd* pWndParent = NULL);
	CxDlgPropertySheetSqlExpressionWizard(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CxDlgPropertySheetSqlExpressionWizard(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

	CuDlgPropertyPageSqlExpressionMain              m_PageMain;
	CuDlgPropertyPageSqlExpressionFunctionParams    m_PageFuntionParam;
	CuDlgPropertyPageSqlExpressionCompareParams     m_PageComparaison;
	CuDlgPropertyPageSqlExpressionAggregateParams   m_PageAggregateParam;
	CuDlgPropertyPageSqlExpressionAnyAllExistParams m_PageAnyAllExistParam;
	CuDlgPropertyPageSqlExpressionInParams          m_PageInParam;
	CuDlgPropertyPageSqlExpressionDBObjectParams    m_PageDBObject;

	//
	// Operations
public:
	void SetStepAggregate();
	void SetStepFunctionParam();
	void SetStepComparaison();
	void SetStepAnyAllExist();
	void SetStepIn();
	void SetStepDBObject();

	void UpdateCaption(){SetTitle(m_strCaption);}
	LPCTSTR GetCaption() const {return (LPCTSTR)m_strCaption;}
	void GetStatement(CString& strStatement);
	void SetStatement(LPCTSTR  lpszStatement);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgPropertySheetSqlExpressionWizard)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	//
	// Implementation
public:
	CStringList m_listStrColumn;
	CTypedPtrList<CObList, CaDBObject*> m_listObject; // List of Objects (Table or View)

	int  m_nCategory;  // Choose the category (default 0) the first one
	int  m_nNodeHandle;
	int  m_nFamilyID;
	int  m_nAggType;   // IDC_RADIO1 (All), IDC_MFC_RADIO2 (distinct)
	int  m_nCompare;   // IDC_RADIO1 (Greater Than), .., IDC_RADIO6(equal)
	int  m_nIn_notIn;  // IDC_RADIO1 (In), IDC_MFC_RADIO2(Not In)
	int  m_nSub_List;  // IDC_RADIO1 (Sub Query), IDC_MFC_RADIO2(List)
	CaLLQueryInfo m_queryInfo;
	LPPOINT m_pPointTopLeft;
	virtual ~CxDlgPropertySheetSqlExpressionWizard();

protected:
	void ResetCategory();

	CString m_strCaption;
	CString m_strStatement;
	int m_nHelpLevel;

	// Generated message map functions
protected:
	//{{AFX_MSG(CxDlgPropertySheetSqlExpressionWizard)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLEPSHT_H__25100EC1_FF90_11D1_A281_00609725DDAF__INCLUDED_)
