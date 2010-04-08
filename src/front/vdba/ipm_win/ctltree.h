/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ctltree.h : header file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : CTreeCtrl derived class to be used when creating the control
**               without the left pane (tree view)
**
** History:
**
** 15-Apr-2002 (uk$so01)
**    Created
*/

#if !defined(AFX_CTLTREE_H__D3805032_504A_11D6_87AF_00C04F1F754A__INCLUDED_)
#define AFX_CTLTREE_H__D3805032_504A_11D6_87AF_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
class CdIpmDoc;

class CuTreeCtrlInvisible : public CTreeCtrl
{
public:
	CuTreeCtrlInvisible();
	void SetIpmDoc(CdIpmDoc* pDoc){m_pDoc = pDoc;}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuTreeCtrlInvisible)
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CuTreeCtrlInvisible();

	// Generated message map functions
protected:
	CdIpmDoc* m_pDoc;
	//{{AFX_MSG(CuTreeCtrlInvisible)
	afx_msg void OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CTLTREE_H__D3805032_504A_11D6_87AF_00C04F1F754A__INCLUDED_)
