/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dlgmain.h : header file
**    Project  : INGRES II/ Visual Schema Diff Control (vdda.exe).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : main dialog of vdda.exe (container of vdda.ocx)
**
** History:
**
** 23-Sep-2002 (uk$so01)
**    Created
** 12-Oct-2004 (uk$so01)
**    BUG #113215 / ISSUE 13720075,
**    F1-key should invoke always the topic Overview if no comparison's result.
**    Otherwise F1-key should invoke always the ocx.s help.
*/

//{{AFX_INCLUDES()
#include "vsdactl.h"
//}}AFX_INCLUDES
#if !defined(AFX_DLGMAIN_H__2F5E5366_CF0B_11D6_87E7_00C04F1F754A__INCLUDED_)
#define AFX_DLGMAIN_H__2F5E5366_CF0B_11D6_87E7_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "uchklbox.h"
class CaDBObject;
class CaNode;
class CaUser;

class CaIncluding: public CObject
{
public:
	CaIncluding(LPCTSTR lpszName, BOOL b4Installation, BOOL bIncluded = TRUE)
	{
		m_strName = lpszName;
		m_bInclude = bIncluded;
		m_b4Installation = b4Installation;
	}
	virtual ~CaIncluding(){}
	void SetInclude(BOOL bSet){m_bInclude = bSet;}
	BOOL GetInclude(){return m_bInclude;}
	BOOL GetInstallation(){return m_b4Installation;}
	CString GetName(){return m_strName;}

protected:
	CString m_strName;
	BOOL    m_bInclude;
	BOOL    m_b4Installation;
};



class CuVddaControl: public CuVsda
{
public:
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	void HideFrame(short nShow);

	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CuDlgMain dialog

class CuDlgMain : public CDialog
{
public:
	CuDlgMain(CWnd* pParent = NULL);   // standard constructor
	void OnOK(){return;}
	void OnCancel(){return;}
	void DoCompare();

	// Dialog Data
	//{{AFX_DATA(CuDlgMain)
	enum { IDD = IDD_MAIN };
	CButton	m_cButtonSc2Save;
	CButton	m_cButtonSc2Open;
	CButton	m_cButtonSc1Save;
	CButton	m_cButtonSc1Open;
	CButton	m_cCheckIgnoreOwner;
	CEdit	m_cEditFile2;
	CEdit	m_cEditFile1;
	CComboBoxEx	m_cComboUser2;
	CComboBoxEx	m_cComboDatabase2;
	CComboBoxEx	m_cComboNode2;
	CComboBoxEx	m_cComboUser1;
	CComboBoxEx	m_cComboDatabase1;
	CComboBoxEx	m_cComboNode1;
	CuVddaControl	m_vsdaCtrl;
	//}}AFX_DATA
	CuCheckListBox m_cListCheckBox;
	BOOL m_bCompared;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgMain)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CTypedPtrList< CObList, CaDBObject* > m_lNode1;
	CTypedPtrList< CObList, CaDBObject* > m_lUser1;
	CTypedPtrList< CObList, CaDBObject* > m_lDatabase1;
	CTypedPtrList< CObList, CaDBObject* > m_lNode2;
	CTypedPtrList< CObList, CaDBObject* > m_lUser2;
	CTypedPtrList< CObList, CaDBObject* > m_lDatabase2;
	CTypedPtrList< CObList, CaIncluding*> m_lInluding1;
	CTypedPtrList< CObList, CaIncluding*> m_lInluding2;
	BOOL       m_bInitIncluding;
	BOOL       m_bPreselectDBxUser;
	BOOL       m_bLoadedSchema1;
	BOOL       m_bLoadedSchema2;
	UINT       m_nAlreadyRefresh;
	CImageList m_ImageListNode;
	CImageList m_ImageListUser;
	CImageList m_ImageListDatabase;
	int m_nSelInstallation1;
	int m_nSelInstallation2;

	HICON m_hIconOpen;
	HICON m_hIconSave;
	HICON m_hIconNode;

	BOOL GetSchemaParam (int nSchema, CString& strNode, CString& strDatabase, CString& strUser);
	void ShowShemaFile(int nSchema, BOOL bShow, LPCTSTR lpszFile);
	void QueryNode(CComboBoxEx* pComboNode, CString& strSelectedNode);
	void QueryUser(CComboBoxEx* pComboUser, CaNode* pNode, CString& strSelectedUser);
	void QueryDatabase(CComboBoxEx* pComboDatabase, CaNode* pNode, CString& strSelectedDatabase);


	void CheckValidParameters();
	void CheckInputParam(int nParam);
	BOOL CheckInstallation();
	void GetFilter(CString& strFilter);
	void InitializeInluding();
	int GetInstallationParam(int nSchema); // -1: error, 0: -> no, 1:->yes
	void InitializeCheckIOW();

	// Generated message map functions
	//{{AFX_MSG(CuDlgMain)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnDropdownComboNode1();
	afx_msg void OnDropdownComboNode2();
	afx_msg void OnDropdownComboDatabase1();
	afx_msg void OnDropdownComboDatabase2();
	afx_msg void OnDropdownComboUser1();
	afx_msg void OnDropdownComboUser2();
	afx_msg void OnButtonCompare();
	afx_msg void OnSelchangeComboNode1();
	afx_msg void OnSelchangeComboNode2();
	afx_msg void OnButtonSc1Open();
	afx_msg void OnButtonSc2Open();
	afx_msg void OnButtonSc1Save();
	afx_msg void OnButtonSc2Save();
	afx_msg void OnSelchangeComboDatabase1();
	afx_msg void OnSelchangeComboDatabase2();
	afx_msg void OnSelchangeComboboxexUser1();
	afx_msg void OnSelchangeComboboxexUser2();
	afx_msg void OnCheckGroup();
	//}}AFX_MSG
	afx_msg void OnCheckChangeIncluding();
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGMAIN_H__2F5E5366_CF0B_11D6_87E7_00C04F1F754A__INCLUDED_)
