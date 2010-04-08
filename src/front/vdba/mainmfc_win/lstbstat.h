/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : lstbstat.h, header file
**    Project  : CA-OpenIngres/Vdba
**    Author   : UK Sotheavut
**    Purpose  : Owner draw LISTCTRL that can draw 3D horizontal bar in the column 
**
** History 
**
** xx-Mar-1998 (uk$so01)
**    created.
** 04-Jun-2002 (uk$so01)
**    SIR #105384 Adjust the header witdth when double-ckicking on the divider
*/

#if !defined(AFX_LSTBSTAT_H__8B2ABD41_C577_11D1_A273_00609725DDAF__INCLUDED_)
#define AFX_LSTBSTAT_H__8B2ABD41_C577_11D1_A273_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// lstbstat.h : header file
//
#include "calsctrl.h"
#include "tbstatis.h"

class CuListCtrlTableStatistic : public CuListCtrl
{
public:
	enum 
	{
		STATITEM_NUM,
		STATITEM_VALUE,
		STATITEM_PERCENTAGE,
		STATITEM_GRAPH1,
		STATITEM_REPFACTOR,
		STATITEM_GRAPH2
	};
	CuListCtrlTableStatistic();
	//
	// You must set these values before adding the items:
	void SetMaxValues (double maxPercent, double maxRepFactor)
	{
		m_dMaxPercentage = maxPercent;
		m_dMaxRepetitionFactor =maxRepFactor;
	}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuListCtrlTableStatistic)
	//}}AFX_VIRTUAL
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult);

	//
	// Implementation
public:
	virtual ~CuListCtrlTableStatistic();

protected:
	double m_dMaxPercentage;
	double m_dMaxRepetitionFactor;
	//
	// Color for 3D bars:
	COLORREF m_crFaceFront; // Front face
	COLORREF m_crFaceUpper; // Upper face
	COLORREF m_crFaceRight; // Right face

	UINT GetColumnJustify(int nCol);
	virtual void DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct);
	double GetPercentUnit();
	double GetRepFactorUnit();
	void   DrawBar(CDC* pDC, CRect r, CaTableStatisticItem* pData, BOOL bRep = FALSE);

	// Generated message map functions
protected:
	//{{AFX_MSG(CuListCtrlTableStatistic)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	afx_msg void OnDividerdblclick(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LSTBSTAT_H__8B2ABD41_C577_11D1_A273_00609725DDAF__INCLUDED_)
