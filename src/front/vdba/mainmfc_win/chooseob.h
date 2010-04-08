/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : chooseob.h : header file
**    Project  : VDBA / Generic window control 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Modal dialog that displays items in the list control and
**               allows user to select the objects 
**               (The list control behaves like CCheckListBox)
**
** History:
**
** 27-Apr-2001 (uk$so01)
**    Created for SIR #102678
**    Support the composite histogram of base table and secondary index.
**/

#if !defined(AFX_STATEIM_H__04B6E085_3988_11D5_874F_00C04F1F754A__INCLUDED_)
#define AFX_STATEIM_H__04B6E085_3988_11D5_874F_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "calscbox.h"
typedef void (CALLBACK* PfnDisplayChooseObject) (void* pItemData, CuListCtrlCheckBox* pCtrl, int nItem);


class CxChooseObjects : public CDialog
{
public:
	CxChooseObjects(CWnd* pParent = NULL); 
	void InitializeHeader(LSCTRLHEADERPARAMS* pArrayHeader, int nSize)
	{
		m_pArrayHeader = pArrayHeader;
		m_nHeaderSize = nSize;
	}
	CPtrArray& GetArrayObjects(){return m_arrayObjects;}
	CPtrArray& GetArraySelectedObjects(){return m_arraySelectedObjects;}
	void SetDisplayFunction(PfnDisplayChooseObject pfnDisplay){m_pfnDisplay = pfnDisplay;}
	void SetSmallImageList(CImageList* pImageList){m_pSmallImageList = pImageList;}
	void SetTitle(LPCTSTR lpszTitle){m_strTitle = lpszTitle;}
	void SetHelpID(UINT nHelpID){m_nIDHelp = nHelpID;}

	// Dialog Data
	//{{AFX_DATA(CxChooseObjects)
	enum { IDD = IDD_XCHOOSEOBJECTS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxChooseObjects)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CPtrArray m_arrayObjects;
	CPtrArray m_arraySelectedObjects;
	LSCTRLHEADERPARAMS* m_pArrayHeader;
	int m_nHeaderSize;
	PfnDisplayChooseObject m_pfnDisplay;

	CuListCtrlCheckBox m_cList1;
	CImageList m_StateImageList;
	CImageList* m_pSmallImageList;
	CString m_strTitle;
	UINT m_nIDHelp;

	void Display();
	void* GetSelectObject(void* pObject);
	// Generated message map functions
	//{{AFX_MSG(CxChooseObjects)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#define _USE_PUSHHELPID
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STATEIM_H__04B6E085_3988_11D5_874F_00C04F1F754A__INCLUDED_)
