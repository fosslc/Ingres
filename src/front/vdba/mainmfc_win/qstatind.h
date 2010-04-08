/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : qstatind.h, Header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres Visual DBA.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Draw the static window as an indicator bar of percentage.             //
//                                                                                     //
****************************************************************************************/
#if !defined(AFX_UWINDOW_H__D2075462_7533_11D1_A230_00609725DDAF__INCLUDED_)
#define AFX_UWINDOW_H__D2075462_7533_11D1_A230_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Uwindow.h : header file
//

class CaStaticIndicatorData: public CObject
{
	DECLARE_SERIAL (CaStaticIndicatorData)
public:
	CaStaticIndicatorData(): m_dPercent(100.0), m_crPercent(RGB(0, 0, 255)) {}
	CaStaticIndicatorData(double dPercent, COLORREF crColor): m_dPercent(dPercent), m_crPercent(crColor){}
	virtual ~CaStaticIndicatorData(){}
	virtual void Serialize (CArchive& ar);

public:
	double   m_dPercent;
	COLORREF m_crPercent;

};


/////////////////////////////////////////////////////////////////////////////
// CuStaticIndicatorWnd window


class CuStaticIndicatorWnd : public CStatic
{
public:
	CuStaticIndicatorWnd();
	void SetColor     (COLORREF crColor) 
	{
		m_crColor = crColor; 
		if (IsWindow (m_hWnd)) 	Invalidate();
	}
	void AddIndicator (COLORREF crColor, double dPercent)
	{
		ASSERT ((m_dTotal + dPercent) <= 100.0);
		m_listPercent.AddTail (new CaStaticIndicatorData (dPercent, crColor));
		m_dTotal += dPercent;
		if (IsWindow (m_hWnd)) 	Invalidate();
	}
	void SetMode (BOOL bVertical){m_bVertical=bVertical;};
	void DeleteIndicator(); 

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuStaticIndicatorWnd)
	//}}AFX_VIRTUAL

	//
	// Implementation
public:
	CString m_strText;

	virtual ~CuStaticIndicatorWnd();
protected:
	BOOL   m_bVertical;
	double m_dTotal;
	COLORREF m_crColor;
	CTypedPtrList < CObList, CaStaticIndicatorData* > m_listPercent;

	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CuStaticIndicatorWnd)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UWINDOW_H__D2075462_7533_11D1_A230_00609725DDAF__INCLUDED_)
