/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : uchklbox.h, Header file
**
**  Project  : Prototype
**  Author   : UK Sotheavut
**
**  Purpose  : Owner draw LISTBOX to have a check list box.
**
**  History:
** ??-Jan-1996 (uk$so01)
**    Created.
** 04-feb-2000 (somsa01)
**    Properly declared OnSetFont().
** 10-Jul-2001 (uk$so01)
**    Get this file from IVM and put it in the library libwctrl.lib
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 28-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
**    And integrate sam modif on 01-nov-2001 (somsa01) Cleaned up 64-bit compiler warnings.
*/

#ifndef UCHKLBOX_HEADER
#define UCHKLBOX_HEADER

class CuCheckListBox : public CListBox
{
protected:
	HBITMAP   m_hBitmapCheck;     // Bitmap used to check the list box items
	int       m_iPlatform;        // Platform ID: W31, WNT, W95
	BOOL      m_bEnable;          // TRUE: Window is enabled. FALSE: Window is disabled, grey items.
	CSize     m_sizeCheck;
	
	int  GetBitmapCheck (int index);
	int  CalcMinimumItemHeight();
	int  CheckFromPoint(CPoint point, BOOL& bInCheck);
	void InvalidateCheck(int nIndex);

public:
	CuCheckListBox();
	BOOL EnableWindow  (BOOL bEnable = TRUE);
	BOOL Create        (DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
	BOOL ResetContent  ();
	BOOL GetCheck      (int iIndex);
	BOOL EnableItem    (int iIndex, BOOL bEnabled);
	BOOL IsItemEnabled (int iIndex);
	BOOL SetItemColor  (int iIndex, COLORREF colorref);
	
	void PreDrawItem   (LPDRAWITEMSTRUCT    lpDrawItemStruct);
	void PreMeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	
	int AddString        (LPCTSTR lpszText);
	int AddString        (LPCTSTR lpszText, COLORREF cColor);
	
	int InsertString     (int iIndex, LPCTSTR lpszText);
	int DeleteString     (int iIndex);
	int SetCheck         (int iIndex, BOOL bCheck = TRUE);
	int SetCheck         (LPCTSTR lpszItem, BOOL bCheck = TRUE);
	
	int SetItemData      (int iIndex, DWORD_PTR dwItemData);
	DWORD_PTR GetItemData    (int iIndex);
	int GetCheckCount();
	
	//
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuCheckListBox)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult);
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

	//
	// Implementation
public:
	virtual ~CuCheckListBox();
	BOOL m_bSelectDisableItem;
	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CuCheckListBox)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg LRESULT OnSetFont(WPARAM , LPARAM);

	DECLARE_MESSAGE_MAP()
};

#endif
/////////////////////////////////////////////////////////////////////////////
