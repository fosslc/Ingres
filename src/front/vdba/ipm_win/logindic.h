/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : logindic.h : header file
**    Project  : INGRES II/ Monitoring.
**    Author   : Emmanuel Blattes
**    Purpose  : 
**
** History:
**
** xx-Xxx-1998 (Emmanuel Blattes)
**    Created
*/

#if !defined (LOGINDIC_HEADER)
#define LOGINDIC_HEADER

class CLogFileIndicator : public CStatic
{
public:
	CLogFileIndicator();

private:
	double m_dblBofPercent;
	double m_dblEofPercent;
	double m_dblRsvPercent;
	BOOL   m_bNotAvailable;

	// Operations
public:
	void SetBofEofPercent(double dblBofPercent, double dblEofPercent, double dblReservedPercent, BOOL bNotAvailable = FALSE);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLogFileIndicator)
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CLogFileIndicator();

private:
	void DrawPortion(CPaintDC &dc, int width, int height, double dblBegPercent, double dblEndPercent, COLORREF clr);

	// Generated message map functions
protected:
	//{{AFX_MSG(CLogFileIndicator)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif
