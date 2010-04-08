/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : edlssele.h, Header File 
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Insert Page 2)
**               Editable List control to provide the Insert Values
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created.
** 01-Mars-2000 (schph01)
**    bug 97680 : management of objects names including special characters
**    in Sql Assistant.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
*/

#if !defined(AFX_EDLSQSE1_H__D141DEE1_B27F_11D1_A25C_00609725DDAF__INCLUDED_)
#define AFX_EDLSQSE1_H__D141DEE1_B27F_11D1_A25C_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "editlsct.h"


class CaSqlWizardDataField: public CObject
{
public:
	enum {COLUMN_NORMAL, COLUMN_AGGREGATE};
	CaSqlWizardDataField():m_iType(COLUMN_NORMAL), m_strColumn(_T("")), m_strObjectOwner(_T("")), m_strObjectName(_T("")){}
	CaSqlWizardDataField(int iType, LPCTSTR lpszField)
		:m_iType(iType), m_strColumn(lpszField), m_strObjectOwner(_T("")),m_strObjectName(_T("")){}

	// copy constructor and '=' operator overloading
	CaSqlWizardDataField(const CaSqlWizardDataField& a);
	CaSqlWizardDataField operator = (const CaSqlWizardDataField & b);

	void duplicate(const CaSqlWizardDataField& p);
	virtual ~CaSqlWizardDataField(){}
	void SetTable(LPCTSTR lpszTable, LPCTSTR lpszTableOwner, int nObjectType); // nObjectType = OBT_TABLE or OBT_VIEW
	CString GetTable(){return m_strObjectName;}
	CString GetTableOwner(){return m_strObjectOwner;}
	CString GetColumn(){return m_strColumn;}
	int GetObjectType(){return m_nObjType;}


	CString FormatString4SQL();
	CString FormatString4Display();

	int m_nObjType;
	int m_iType;
	CString m_strColumn;
	CString m_strObjectOwner;
	CString m_strObjectName;
};


class CaSqlWizardDataFieldOrder: public CaSqlWizardDataField
{
public:
	CaSqlWizardDataFieldOrder():CaSqlWizardDataField(), m_strSort(_T("")){}
	CaSqlWizardDataFieldOrder(int iType, LPCTSTR lpszField, LPCTSTR lpszSort)
		:CaSqlWizardDataField(iType, lpszField), m_strSort(lpszSort){}

	virtual ~CaSqlWizardDataFieldOrder(){}

	CString m_strSort;
	BOOL m_bMultiTablesSelected;
};


/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlSqlWizardSelectOrderColumn window

class CuEditableListCtrlSqlWizardSelectOrderColumn : public CuEditableListCtrl
{
public:
	enum {COLUMN, SORT};
	CuEditableListCtrlSqlWizardSelectOrderColumn();
	void GetSortOrder (CStringList& listColumn);
	void UpdateDisplay (int iItem = -1);
	void FormatString4display (CaSqlWizardDataFieldOrder* pItemTemp, CString& csTempo);

	LPCTSTR GetDefaultSort (){return m_strArraySort[1];};



	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlSqlWizardSelectOrderColumn)
	//}}AFX_VIRTUAL

	void EditValue (int iItem, int iSubItem, CRect rcCell);
	virtual LONG ManageEditDlgOK       (UINT wParam, LONG lParam) {return OnEditDlgOK(wParam, lParam);}      
	virtual LONG ManageComboDlgOK      (UINT wParam, LONG lParam) {return OnComboDlgOK(wParam, lParam);}  

	//
	// Implementation
public:
	virtual ~CuEditableListCtrlSqlWizardSelectOrderColumn();



protected:
	void InitSortComboBox();

	enum {m_nSortItem = 3};
	TCHAR m_strArraySort[m_nSortItem][16];
	int   m_nArraySort[m_nSortItem];

	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CuEditableListCtrlSqlWizardSelectOrderColumn)
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	afx_msg LONG OnEditDlgOK (UINT wParam, LONG lParam);
	afx_msg LONG OnComboDlgOK (UINT wParam, LONG lParam);
	afx_msg void OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDLSQSE1_H__D141DEE1_B27F_11D1_A25C_00609725DDAF__INCLUDED_)
