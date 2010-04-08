/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgrepco2.h, header file
**    Project  : INGRES II/ Replication Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Modeless Dialog to display the special page for Transaction Collision
**
** History:
**
** xx-Sep-1998 (uk$so01)
**    Created
**/


#if !defined(AFX_DGREPCO2_H__DC3A0862_4973_11D2_A2A1_00609725DDAF__INCLUDED_)
#define AFX_DGREPCO2_H__DC3A0862_4973_11D2_A2A1_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "collidta.h"

class CuDlgReplicationPageCollisionRight2 : public CDialog
{

public:
	CuDlgReplicationPageCollisionRight2(CWnd* pParent = NULL);
	void OnOK() {return;}
	void OnCancel() {return;}

	void DisplayData (CaCollisionTransaction* pData);

	// Dialog Data
	//{{AFX_DATA(CuDlgReplicationPageCollisionRight2)
	enum { IDD = IDD_REPCOLLISION_PAGE_RIGHT2 };
	CButton	m_cButton1;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgReplicationPageCollisionRight2)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgReplicationPageCollisionRight2)
	afx_msg void OnApplyResolution();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGREPCO2_H__DC3A0862_4973_11D2_A2A1_00609725DDAF__INCLUDED_)
