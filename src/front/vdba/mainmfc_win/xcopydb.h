/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : xcopydb.h, header file
**
**  Project  : Ingres II/ VDBA.
**
**  Author   : Schalk Philippe
**
**  Purpose  : Dialog and Class used for manage the new version of utility
**             "copydb"(INGRES 2.6 only) on databases and tables and views.
**
**
**  25-Apr-2001 (schph01)
**      SIR 103299 Created
**  02-May-2003 (uk$so01)
**     BUG #110167, Disable the OK button if no statement is selected.
*/


#if !defined(AFX_XCOPYDB_H__5C5A4AA3_37C3_11D5_B601_00C04F1790C3__INCLUDED_)
#define AFX_XCOPYDB_H__5C5A4AA3_37C3_11D5_B601_00C04F1790C3__INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// xcopydb.h : header file
//
#include "uchklbox.h"

extern "C"
{
#include "dba.h"
#include "dll.h"
#include "dbadlg1.h"
#include "domdata.h"
#include "dlgres.h"
#include "dbaginfo.h"
#include "getdata.h"
#include "msghandl.h"
}

class CaCopyDBOptionList: public CObject
{
public:
	CaCopyDBOptionList():m_strOption(""){};
	CaCopyDBOptionList(int iStringID,LPCTSTR lpszOption):m_iStringID(iStringID),m_strOption(lpszOption){};
	virtual ~CaCopyDBOptionList(){};

	CString     m_strOption; // option used in command line
	int         m_iStringID; // string ID define in resource to explain this option
};


/////////////////////////////////////////////////////////////////////////////
// CxDlgCopyDb dialog

class CxDlgCopyDb : public CDialog
{
// Construction
public:
	int m_iObjectType;
	CString m_csTableOwner;
	CString m_csNodeName;
	CString m_csOwnerTbl;
	CString m_csTableName;
	CString m_csDBName;

	AUDITDBTPARAMS m_lpTblParam;

	void FillListOptions();
	
	CTypedPtrArray<CObArray, CaCopyDBOptionList*> m_listOptions; 
	void InitializeLstOptions();

	CxDlgCopyDb(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CxDlgCopyDb)
	enum { IDD = IDD_XCOPYDB };
	CEdit	m_ctrlEditOutputName;
	CEdit	m_ctrlEditInputName;
	CEdit	m_ctrlEditDirSource;
	CEdit	m_ctrlEditDirName;
	CEdit	m_ctrlEditDirDest;
	CButton	m_ctrlCheckTables;
	CEdit	m_ctrlSelTbl;
	CStatic	m_staticTbl;
	CButton	m_ctrlSpecifyTable;
	//}}AFX_DATA

	CuCheckListBox	m_lstCheckOptions;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgCopyDb)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void GenerateCopyDBSyntax(CString& csCommandSyntax);
	void ExecuteRemoteCommand(LPCTSTR csCommandLine);

	// Generated message map functions
	//{{AFX_MSG(CxDlgCopyDb)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnCheckTables();
	afx_msg void OnButtonSpecifyTables();
	virtual void OnOK();
	//}}AFX_MSG
	afx_msg void OnCheckChangeOption();
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XCOPYDB_H__5C5A4AA3_37C3_11D5_B601_00C04F1790C3__INCLUDED_)
