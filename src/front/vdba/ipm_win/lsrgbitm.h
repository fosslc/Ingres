/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : lsrgbitm.h, Header file 
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : List control that allows the items to have foreground and background
**               colors
**
** History:
**
** xx-Sep-1998 (uk$so01)
**    Created
*/

#if !defined(LIST_CONTROL_RGBITEM__2BED42B6_6186_11D1_A225_00609725DDAF__INCLUDED_)
#define LIST_CONTROL_RGBITEM__2BED42B6_6186_11D1_A225_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "calsctrl.h"


class CuListCtrlColorItem : public CuListCtrl
{
public:
	CuListCtrlColorItem();

	//
	// Implementation
public:
	virtual ~CuListCtrlColorItem();
	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CuListCtrlColorItem)
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(LIST_CONTROL_RGBITEM__2BED42B6_6186_11D1_A225_00609725DDAF__INCLUDED_)
