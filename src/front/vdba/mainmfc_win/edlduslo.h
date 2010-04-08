// edlduslo.h: interface for the CuEditableListCtrlDuplicateDbSelectLocation class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EDLDUSLO_H__25BE3F03_6CB4_11D2_9734_00609725DBF9__INCLUDED_)
#define AFX_EDLDUSLO_H__25BE3F03_6CB4_11D2_9734_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "editlsct.h"

class CuEditableListCtrlDuplicateDbSelectLocation : public CuEditableListCtrl
{
public:
	enum {INITIAL_LOC, NEW_LOC};
	CuEditableListCtrlDuplicateDbSelectLocation();
	virtual ~CuEditableListCtrlDuplicateDbSelectLocation();

	LONG ManageComboDlgOK      (UINT wParam, LONG lParam) {return OnComboDlgOK(wParam, lParam);}

	void EditValue (int iItem, int iSubItem, CRect rcCell);
	//void UpdateDisplay (int iItem);

protected:
	void InitSortComboBox();
	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CuEditableListCtrlDuplicateDbSelectLocation)
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	//afx_msg LONG OnEditDlgOK (UINT wParam, LONG lParam);
	afx_msg LONG OnComboDlgOK (UINT wParam, LONG lParam);
	//afx_msg void OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()

};

#endif // !defined(AFX_EDLDUSLO_H__25BE3F03_6CB4_11D2_9734_00609725DBF9__INCLUDED_)
