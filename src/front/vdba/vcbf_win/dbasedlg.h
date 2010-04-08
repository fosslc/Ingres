/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dbasedlg.h, Header File 
**    Project  : OpenIngres Configuration Manager 
**    Author   : UK Sotheavut
**               Blatte Emannuel 
**    Purpose  : Special Custom List Box to provide on line editing.
**               DBMS Database page 
**
** History:
** xx-Sep-1997 (uk$so01)
**    Created.
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
*/

#if !defined(AFX_DBASEDLG_H__05328423_2516_11D1_A1EE_00609725DDAF__INCLUDED_)
#define AFX_DBASEDLG_H__05328423_2516_11D1_A1EE_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// dbasedlg.h : header file
//
#include "layeddlg.h"

/////////////////////////////////////////////////////////////////////////////
// CuDbmsDatabaseListBox window
class CuDbmsDatabaseListBox : public CListBox
{
public:
	CuDbmsDatabaseListBox();
	void SetEditText (int iIndex = -1, LPCTSTR lpszText = NULL);
	void DeleteItem  (int iIndex);
	void HideProperty(BOOL bValidate = FALSE);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDbmsDatabaseListBox)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

	//
	// Implementation
public:
	CuLayoutEditDlg2* m_pEditDlg;
	virtual ~CuDbmsDatabaseListBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CuDbmsDatabaseListBox)
	afx_msg void OnSelchange();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg LONG OnEditOK     (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnEditCancel (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};



/////////////////////////////////////////////////////////////////////////////
// CuDlgDbmsDatabase dialog

class CuDBMSItemData;
class CuDlgDbmsDatabase : public CFormView
{
private:
  CuDBMSItemData* m_pDbmsItemData;
  CString BuildResultList();

// Construction
protected:
	CuDlgDbmsDatabase();   // standard constructor
	DECLARE_DYNCREATE(CuDlgDbmsDatabase)

public:

	// Dialog Data
	//{{AFX_DATA(CuDlgDbmsDatabase)
	enum { IDD = IDD_DBMS_PAGE_DATABASE };
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDbmsDatabase)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CuDbmsDatabaseListBox m_DbList;

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CuDlgDbmsDatabase)
	virtual void OnInitialUpdate();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData  (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnValidateData(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnAdd         (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnDelete      (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnEditOK      (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnEditCancel  (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};



/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DBASEDLG_H__05328423_2516_11D1_A1EE_00609725DDAF__INCLUDED_)
