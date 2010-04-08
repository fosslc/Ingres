/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : lsctlhug.h: interface for the CuListCtrlHuge class
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : The special list control that enable scrolling with 
**               the huge amount of item..
**
** History:
**
**	12-Feb-2001 (uk$so01)
**	    Created.
**	15-aug-2001 (somsa01)
**	    Changed prototype of OnKeydownListCtrl().
*/


/*
****  HOW TO USE THIS CONTROL
**
** 1) You must Construct and create the control
**    Ex: CuListCtrlHuge m_wndMyList;
**        GetClientRect (r);
**        m_wndMyList.Create (WS_EX_CLIENTEDGE, NULL, NULL, WS_CHILD|WS_VISIBLE, r, this, 1000);
**
** 2) You must initialize the header and set the callback function for displaying item.
**    Ex:
**        LSCTRLHEADERPARAMS lsp[5] =
**        {
**            {_T("col0"),    100,  LVCFMT_LEFT,  FALSE},
**            {_T("col1"),     80,  LVCFMT_LEFT,  FALSE},
**            {_T("col2"),    100,  LVCFMT_LEFT,  FALSE},
**            {_T("col3"),    100,  LVCFMT_LEFT,  FALSE},
**            {_T("col4"),    150,  LVCFMT_LEFT,  FALSE}
**        };
**        m_wndMyList.InitializeHeader(lsp, 5);
**        m_wndMyList.SetDisplayFunction(myDisplayItem);
**        where myDisplayItem is defined as:
**        void CALLBACK myDisplayItem(void* pItemData, CuListCtrl* pListCtrl, int nItem);
**
** 3) Add/Remove item by using the class CPtrArray. After that you must call
**    the member ResizeToFit() to manage the scrollbar.
**    Ex:
**        CPtrArray& arrayData = m_wndMyList.GetArrayData();
**        arrayData.SetSize (100000); // 100000 items
**        for (int i = 0; i<100000; i++)
**        {
**            CMyObject* myObject= Alocate my object;
**            arrayData.SetAt(i, (void*)myObject);
**        }
**        m_wndMyList.ResizeToFit();
**
** 4) The caller is responsible to destroy the allocated data:
**    Ex:
**        CPtrArray& arrayData = m_wndMyList.GetArrayData();
**        for (int i = 0; i<100000; i++)
**        {
**            CMyObject* myObject = (CMyObject*) arrayData.GetAt(i);
**            delete myObject;
**        }
**        arrayData.RemoveAll();
**        m_wndMyList.ResizeToFit();
**
*/




#if !defined(AFX_LSCTLHUG_H__5487D497_FD06_11D4_873E_00C04F1F754A__INCLUDED_)
#define AFX_LSCTLHUG_H__5487D497_FD06_11D4_873E_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// lsctlhug.h : header file
//

#include "calsctrl.h"
//
// pItemData is a void* pointer. In the callback function, this pointer must be casted to
// the known type.
// pCtrl is a CListCtrl derived class.
// nItem is the list control item index at which to display the item.
// If bUpdate = TRUE then the 'pItemData' will replace the current 'nItem'. Do not insert new item. !!
// pInfo is a user information of how to display the items. This information is the one that you
// set by calling CuListCtrlHuge::SetDisplayFunction()
typedef void (CALLBACK* PfnListCtrlDisplayItem) (void* pItemData, CuListCtrl* pCtrl, int nItem, BOOL bUpdate, void* pInfo);

class CuListCtrlHugeChildList: public CuListCtrl
{
public:
	CuListCtrlHugeChildList():CuListCtrl(){}
	~CuListCtrlHugeChildList(){}

protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
};


class CuListCtrlHuge : public CWnd
{
	DECLARE_DYNCREATE(CuListCtrlHuge)
public:
	CuListCtrlHuge();
	CScrollBar& GetVirtScrollBar(){return m_vcroll;}
	CPtrArray* GetArrayData(){return m_pData;}
	int GetCurPos(){return m_nCurPos;}
	void SetCurPos(int nPos = 0){m_nCurPos = nPos;}
	PfnListCtrlDisplayItem GetPfnDisplayItem(){return m_pfnDisplayItem;}
	void* GetUserInfo(){return m_pUserInfo;}
	CuListCtrl* GetListCtrl(){return m_pListCtrl;}

	void SetCountPerPage (int nCount){m_nItemCountPerPage = nCount;}
	virtual void InitializeHeader(LSCTRLHEADERPARAMS* pArrayHeader, int nSize);
	virtual void Cleanup();
	//
	// When you add/remove the elements into/from the array 'm_arrayPtr' then you
	// should call this function manage the scrolling (hide/show scrollbar).
	virtual void ResizeToFit();

	void SetDisplayFunction (PfnListCtrlDisplayItem pfn, void* pUserInfo)
	{
		m_pfnDisplayItem = pfn;
		m_pUserInfo = pUserInfo;
	}
	void SetSharedData(CPtrArray* pExternalArray){m_pData = pExternalArray; m_bSharedData = TRUE;}
	void UnsetSharedData(){m_pData = &m_arrayPtr; m_bSharedData = FALSE;}
	void SetFullRowSelected(BOOL bFullRow = TRUE)
	{
		ASSERT (m_pListCtrl);
		if (m_pListCtrl)
			m_pListCtrl->SetFullRowSelected(bFullRow);
	}
	void SetLineSeperator   (BOOL bLine = TRUE, BOOL bVertLine = TRUE, BOOL bHorzLine = TRUE)
	{
		ASSERT (m_pListCtrl);
		if (m_pListCtrl)
			m_pListCtrl->SetLineSeperator(bLine, bVertLine, bHorzLine);
	}
	void SetAllowSelect (BOOL bAllow)
	{
		ASSERT (m_pListCtrl);
		if (m_pListCtrl)
			m_pListCtrl->SetAllowSelect(bAllow);
	}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuListCtrlHuge)
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CuListCtrlHuge();
protected:
	int  m_nCurPos;
	int  m_nItemCountPerPage;
	int  m_nListCtrlStyle;
	BOOL m_bSharedData;
	BOOL m_bDrawInfo;
	BOOL m_bTrackPos;
	CPoint m_pointInfo;
	SCROLLINFO m_scrollInfo;
	CPtrArray* m_pData;
	PfnListCtrlDisplayItem m_pfnDisplayItem;
	CScrollBar  m_vcroll;
	CPtrArray   m_arrayPtr;
	CuListCtrl* m_pListCtrl;
	//
	// User provided the information of how to display the items:
	void* m_pUserInfo;   

	void DrawThumbTrackInfo (int nPos, BOOL bErase = FALSE);
	void Display (int nCurrentScrollPos = 0, BOOL bUpdate = FALSE);
	void ScrollLineUp();
	void ScrollLineDown();
	void ScrollPageUp();
	void ScrollPageDown();
	void OnKeydownListCtrl (NMHDR* pNMHDR, LRESULT* pResult);

	//
	// The derived class can overwrite these members and are called
	// by the On_xx below:
	virtual void OnItemchangedListCtrl(NMHDR* pNMHDR, LRESULT* pResult){}
	virtual void OnColumnclickListCtrl(NMHDR* pNMHDR, LRESULT* pResult){}

	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CuListCtrlHuge)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg void OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	afx_msg void On_KeydownListCtrl(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void On_ItemchangedListCtrl(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void On_ColumnclickListCtrl(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LSCTLHUG_H__5487D497_FD06_11D4_873E_00C04F1F754A__INCLUDED_)
