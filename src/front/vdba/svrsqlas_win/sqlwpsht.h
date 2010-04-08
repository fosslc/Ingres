/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlwpsht.h, Header File 
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Select, Insert, Update, Delete)
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 14-Apr-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (sqlassis.chm file)
**    Activate Help Button.
*/

#if !defined (SQLWPSHT_PROPERTY_SHEET_HEADER)
#define SQLWPSHT_PROPERTY_SHEET_HEADER
#include "sqlkeywd.h" // SQL Key word
#include "uchklbox.h" // Check List Box
#include "sqlwmain.h" // Main Page
#include "sqlwsel1.h" // Select page 1
#include "sqlwsel2.h" // Select page 2
#include "sqlwsel3.h" // Select page 3
#include "sqlwins1.h" // Insert page 1
#include "sqlwins2.h" // Insert page 2
#include "sqlwupd1.h" // Update page 1
#include "sqlwupd2.h" // Update page 2
#include "sqlwupd3.h" // Update page 3
#include "sqlwdel1.h" // Delete page 1

//
// This is a control ID of help button (I use the SPY++ to detect it)
#define ID_W_HELP 9

BOOL SQLW_ComboBoxTablesClean (CComboBox* pCombo);
BOOL SQLW_ComboBoxFillTables  (CComboBox* pCombo,CaLLQueryInfo* pInfo);
BOOL SQLW_CuCheckListBoxTablesClean (CuListCtrlCheckBox* pListBox);
BOOL SQLW_CuCheckListBoxFillTables  (CuListCtrlCheckBox* pListBox, CaLLQueryInfo* pInfo);
BOOL SQLW_CuCheckListBoxColumnsClean (CuCheckListBox* pListBox);
BOOL SQLW_CuCheckListBoxFillColumns (CuCheckListBox* pListBox, CaLLQueryInfo* pInfo);
BOOL SQLW_ListBoxColumnsClean (CListBox* pListBox);
BOOL SQLW_ListBoxFillColumns (CListBox* pListBox, CaLLQueryInfo* pInfo);
void SQLW_SelectObject (CuListCtrlCheckBox& listCtrl, CaDBObject* pObj);


class CxDlgPropertySheetSqlWizard : public CPropertySheet
{
	DECLARE_DYNAMIC(CxDlgPropertySheetSqlWizard)

public:
	enum {SQLW_SELECT, SQLW_INSERT, SQLW_UPDATE, SQLW_DELETE};
	enum {SQLW_INSERT_MANUAL, SQLW_INSERT_SUBSELECT};

	CxDlgPropertySheetSqlWizard(CWnd* pWndParent = NULL);
	CxDlgPropertySheetSqlWizard(BOOL bSelectOnly, CWnd* pWndParent = NULL);

	void SetCategorySelect();
	void SetCategoryInsert();
	void SetCategoryUpdate();
	void SetCategoryDelete();
	void SetCategoryInsert2(BOOL bManual = TRUE);

	//
	// Operations
public:
	void GetStatement(CString& strStatement);
	void SetStatement(LPCTSTR  lpszStatement);

	int GetCategory(){return m_nWizardType;}
	int GetInsertType(){return m_nInsertType;}

	BOOL IsSelectOnly(){return m_bSelectOnly;}
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgPropertySheetSqlWizard)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	//
	// Implementation
public:
	CuDlgPropertyPageSqlWizardMain m_PageMain;

	CuDlgPropertyPageSqlWizardSelect1 m_PageSelect1;
	CuDlgPropertyPageSqlWizardSelect2 m_PageSelect2;
	CuDlgPropertyPageSqlWizardSelect3 m_PageSelect3;
	CuDlgPropertyPageSqlWizardInsert1 m_PageInsert1;
	CuDlgPropertyPageSqlWizardInsert2 m_PageInsert2;
	CuDlgPropertyPageSqlWizardUpdate1 m_PageUpdate1;
	CuDlgPropertyPageSqlWizardUpdate2 m_PageUpdate2;
	CuDlgPropertyPageSqlWizardUpdate3 m_PageUpdate3;
	CuDlgPropertyPageSqlWizardDelete1 m_PageDelete1;
	CaLLQueryInfo m_queryInfo;
	LPPOINT m_pPointTopLeft;

	virtual ~CxDlgPropertySheetSqlWizard();

	//
	// Generated message map functions
protected:
	void SetSelectOnlyMode();
	void SetCategory(int nMode){m_nWizardType = nMode;}

	int m_nWizardType;
	int m_nInsertType;
	int m_nHelpLevel;
	BOOL m_bSelectOnly;     // Wizard is available only for generating the select statement

	CString m_strStatement;
	CString m_strInsertStatement;

	//{{AFX_MSG(CxDlgPropertySheetSqlWizard)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()
};


#endif // __MYPROPERTYSHEET_H__
