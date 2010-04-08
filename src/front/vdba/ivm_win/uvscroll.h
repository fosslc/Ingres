/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : uvscroll.h, Header file
**    Project  : Ingres Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Custom vertical scroll bar
**
** History:
**
** 01-Sep-1999 (uk$so01)
**    created
** 21-Jul-2000 (uk$so01)
**    BUG #102152. Extend the Vertical scroll range limit to be greater than 32767 
**/


#if !defined(AFX_UVSCROLL_H__46C09FC2_6067_11D3_A305_00609725DDAF__INCLUDED_)
#define AFX_UVSCROLL_H__46C09FC2_6067_11D3_A305_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CuListCtrlVScrollEvent;
class CuVerticalScroll : public CScrollBar
{
public:
	CuVerticalScroll();
	void SetListCtrl(CuListCtrlVScrollEvent* pCtrl){m_pListCtrl = pCtrl;}


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuVerticalScroll)
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CuVerticalScroll();
	void SetScrollRange( int nMinPos, int nMaxPos, BOOL bRedraw = TRUE );
	int SetScrollPos( int nPos, BOOL bRedraw = TRUE, int nRealPos = -1);
	double GetFactor(){return m_dDiv;}
	long GetRealMaxScroll(){return m_nRealScrollMax;}
	int  GetScrollPosExtend(){return m_nScrollPosExted;}
	void SetScrollPosExtend(int nPos){m_nScrollPosExted = nPos;}

	// Generated message map functions
protected:
	CuListCtrlVScrollEvent* m_pListCtrl;
	long   m_nRealScrollMax;
	double m_dDiv;
	int    m_nScrollPosExted;

	//{{AFX_MSG(CuVerticalScroll)
	afx_msg void VScroll(UINT nSBCode, UINT nPos);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UVSCROLL_H__46C09FC2_6067_11D3_A305_00609725DDAF__INCLUDED_)
