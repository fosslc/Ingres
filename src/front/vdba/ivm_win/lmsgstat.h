/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : lmsgstat.h , Header File 
**    Project  : Ingres II / Visual Manager 
**    Author   : Sotheavut UK (uk$so01) 
**    Purpose  : Owner draw LISTCTRL that can draw 3D horizontal bar in the column 
**
** History:
**
** 07-Jul-1999 (uk$so01)
**    Created
** 12-Fev-2001 (noifr01)
**    bug 102974 (manage multiline errlog.log messages)
** 14-Aug-2001 (uk$so01)
**    SIR #105384 (Adjust the header width)
** 06-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
**/

#if !defined(LIST_MSG_STATISTIC)
#define LIST_MSG_STATISTIC

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "calsctrl.h"

class CaMessageStatisticItemData: public CObject
{
public:
	CaMessageStatisticItemData():
		m_strTitle(""), m_lCode(-1), m_nCount(0),m_nCountExtraLines(0), m_lCatType(0) {};
	CaMessageStatisticItemData(LPCTSTR lpszTitle, long lCode, int nCount, int nCountExtra):
		m_strTitle(lpszTitle), m_lCode(lCode), m_nCount(nCount), m_nCountExtraLines(nCountExtra), m_lCatType(0) {};
	virtual ~CaMessageStatisticItemData(){}

	void SetCatType(long lCat){m_lCatType = lCat;}
	void SetTitle(LPCTSTR lpszTitle){m_strTitle = lpszTitle;}
	CString GetTitle(){return m_strTitle;}
	void SetCount (int nCount){m_nCount = nCount;}
	int GetCount(){return m_nCount;}
	void SetCountExtra (int nCount){m_nCountExtraLines = nCount;}
	int GetCountExtra(){return m_nCountExtraLines;}
	void SetCode (long lCode){m_lCode = lCode;}
	long GetCode (){return m_lCode;}
	long GetCatType (){return m_lCatType;}
private:
	CString m_strTitle;
	long m_lCatType;
	long m_lCode;
	int  m_nCount;
	int  m_nCountExtraLines;

};


class CuListCtrlMessageStatistic : public CuListCtrl
{
public:
	enum 
	{
		STATITEM_MESSAGE,
		STATITEM_COUNT,
		STATITEM_GRAPH
	};
	CuListCtrlMessageStatistic();
	void SetMaxCountGroup(int nMaxCountGroup){m_nMaxCountGroup = nMaxCountGroup;}
	void SetAdjustInfo(LPCTSTR lpszLongMessage, LPCTSTR lpszLongCount)
	{
		m_strLongMessage = lpszLongMessage;
		m_strLongCount = lpszLongCount;
		m_nMaxMessageWidth = 0;
		m_nMaxCountWidth = 0;
	}
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuListCtrlMessageStatistic)
	//}}AFX_VIRTUAL
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult);

	//
	// Implementation
public:
	virtual ~CuListCtrlMessageStatistic();

protected:
	//
	// The OnDraw will perform the calculation of max string 
	// in pixel (m_nMaxMessageWidth and m_nMaxCountWidth) so that the column header size will fit the text
	// (when user double clicks on the column divider)
	int  m_nMaxMessageWidth;
	int  m_nMaxCountWidth;
	CString m_strLongMessage;
	CString m_strLongCount;
	//
	// Color for 3D bars:
	COLORREF m_crFaceFront; // Front face
	COLORREF m_crFaceUpper; // Upper face
	COLORREF m_crFaceRight; // Right face
	COLORREF m_colorFaceSelected;
	int m_nMaxCountGroup;

	UINT GetColumnJustify(int nCol);
	virtual void DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct);
	double GetPercentUnit();
	double GetRepFactorUnit();
	void   DrawBar(CDC* pDC, CRect r, CaMessageStatisticItemData* pData);

	// Generated message map functions
protected:
	//{{AFX_MSG(CuListCtrlMessageStatistic)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	afx_msg void OnDividerdblclick(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(LIST_MSG_STATISTIC)
