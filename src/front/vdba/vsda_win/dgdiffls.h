/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdiffls.h : header file
**    Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of right pane
**
** History:
**
** 03-Sep-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 17-Nov-2004 (uk$so01)
**    SIR #113475 (Add new functionality to allow user to save the results 
**    of a comparison into a CSV file.
*/


#if !defined(AFX_DGDIFFLS_H__418B42A1_BE49_11D6_87DB_00C04F1F754A__INCLUDED_)
#define AFX_DGDIFFLS_H__418B42A1_BE49_11D6_87DB_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "vsdtree.h"
#include "titlebmp.h"
#include "calsctrl.h"
#include "dgdtdiff.h"

class CfMiniFrame;
class CuDlgPageDifferentList : public CDialog
{
public:
	CuDlgPageDifferentList(CWnd* pParent = NULL);
	void OnCancel() {return;}
	void OnOK()     {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgPageDifferentList)
	enum { IDD = IDD_DIFFERENT_LIST };
	CButton	m_cCheckGroup;
	CStatic	m_cStaticDepth2;
	CStatic	m_cStaticDepth1;
	CStatic	m_cStaticNoDifference;
	CStatic	m_cStaticDifferenceCount;
	CEdit	m_cEditDepth;
	CSpinButtonCtrl	m_cSpinDepth;
	BOOL	m_bActionGroupByObject;
	CString	m_strDepth;
	//}}AFX_DATA
	CuListCtrl	m_cListCtrl;


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPageDifferentList)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

private:
	BOOL m_InitGroupBy;
	BOOL m_bDisplaySummarySubBranches;
	CtrfItemData* m_pCurrentTreeItem;
	COLORREF m_rgbBkColor;
	COLORREF m_rgbFgColor;
	CBrush*  m_pEditBkBrush;
	CuTitleBitmap m_cStaticSelectedItem;
	SORTPARAMS m_sortparam;
	CFont m_fontNoDifference;
	BOOL  m_bCreateDiffFont;
	CfMiniFrame* m_pPropFrame;

	// Implementation
protected:
	void DisplayItem (CaVsdItemData* pVsdItemData,  CtrfItemData* pItem, BOOL bShowSummary = FALSE);
	void Display (CtrfItemData* pItem, int nDepth=3);
	void InitColumns(CtrfItemData* pItem = NULL);
	void ConstructList4Sorting(CObArray& T);
	void SortDifferenceItems(int nHeader);
	void ShowNoDifference();
	void PopupDetail(int nSelectedDiffItem);

	CImageList m_ImageList;
	CuDlgDifferenceDetail* m_pDetailDifference;

	// Generated message map functions
	//{{AFX_MSG(CuDlgPageDifferentList)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckGroupByObject();
	afx_msg void OnDeltaposSpinDepth(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeEditDepth();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnColumnclickList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};
void QueryImageID(CtrfItemData* pItem, int& nImage, CString& strObjectType);
BOOL IgnoreItem(CtrfItemData* pItem);
CString GetFullObjectName(CtrfItemData* pItem, BOOL bKey4Sort = FALSE);
BOOL QueryOwner(int nObjectType);

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGDIFFLS_H__418B42A1_BE49_11D6_87DB_00C04F1F754A__INCLUDED_)
