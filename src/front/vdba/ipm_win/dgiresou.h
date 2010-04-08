/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgiresou.h, Header file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control: Detail page of Resource
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
** 15-Apr-2004 (uk$so01)
**    BUG #112157 / ISSUE 13350172
**    Detail Page of Locked Table has an unexpected extra icons, this is
**    a side effect of change #462417.
*/


#ifndef DGIRESOU_HEADER
#define DGIRESOU_HEADER
#include "titlebmp.h"

class CuDlgIpmDetailResource : public CFormView
{
protected:
	CuDlgIpmDetailResource();
	DECLARE_DYNCREATE(CuDlgIpmDetailResource)
public:
	// Dialog Data
	//{{AFX_DATA(CuDlgIpmDetailResource)
	enum { IDD = IDD_IPMDETAIL_RESOURCE };
	CuTitleBitmap    m_transparentBmp;
	CString    m_strGrantMode;
	CString    m_strType;
	CString    m_strDatabase;
	CString    m_strPage;
	CString    m_strConvertMode;
	CString    m_strTable;
	CString    m_strLockCount;
	//}}AFX_DATA
	RESOURCEDATAMAX m_rcStruct;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmDetailResource)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmDetailResource();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmDetailResource)
	virtual void OnInitialUpdate();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
