/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qpagexml.cpp : implementation file 
**    Project  : INGRES II / VDBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : The result tab of SQL/Test that show XML/XML Resultset.
**
** History:
**
** 05-Jul-2001 (uk$so01)
**    Created.
**    SIR #105199. Integrate & Generate XML.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
**
*/

#if !defined(AFX_QPAGEXML_H__F9607AA1_6B0B_11D5_8759_00C04F1F754A__INCLUDED_)
#define AFX_QPAGEXML_H__F9607AA1_6B0B_11D5_8759_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "taskxprm.h"

class CaCursor;
class CaQuerySelectPageData;
class CaExecParamGenerateXMLfromSelect;
class CaQueryXMLPageData;
class CuDlgSqlQueryPageXML : public CDialog
{
public:
	CuDlgSqlQueryPageXML(CWnd* pParent = NULL);
	void OnOK (){return;}
	void OnCancel(){return;}
	void NotifyLoad (CaQueryXMLPageData* pData);

	CaExecParamGenerateXMLfromSelect* GetQueryRowParam(){return m_pQueryRowParam;}

	// Dialog Data
	//{{AFX_DATA(CuDlgSqlQueryPageXML)
	enum { IDD = IDD_SQLQUERY_PAGEXML };
	CEdit	m_cEditStatement;
	CEdit	m_cEditDatabase;
	CStatic	m_cStaticDatabase;
	CButton	m_cButtonXML;
	CString m_strStatement;
	CString m_strDatabase;
	//}}AFX_DATA

	LONG m_nStart;
	LONG m_nEnd;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgSqlQueryPageXML)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	BOOL m_bFirstRun;           // Run the cursor for the first time.
	BOOL m_bXMLSource;          // Default = TRUE (Show xml source)
	BOOL m_bShowStatement;      // Default = FALSE(not show the statement)
	CString m_strXmlSource;
	CString m_strPreview;
	CaExecParamGenerateXMLfromSelect* m_pQueryRowParam;

	void ResizeControls();
	// Generated message map functions
	//{{AFX_MSG(CuDlgSqlQueryPageXML)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnButtonXML();
	afx_msg void OnButtonSave();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg LONG OnDisplayHeader      (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnHighlightStatement (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnDisplayResult      (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData            (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnFetchRows          (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetTabBitmap       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnChangeSetting      (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QPAGEXML_H__F9607AA1_6B0B_11D5_8759_00C04F1F754A__INCLUDED_)
