/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : statleg2.h, Header file
**    Project  : CA-OpenIngres/Monitoring
**    Author   : Emmanuel Blattes
**    Purpose  : Owner draw listbox derived from CuStatisticBarLegendList2
**               having a different background color compatible with dialog box
**
** History:
**
** xx-Apr-1997 (Emmanuel Blattes)
**    Created
*/


#ifndef STATLEGE2_HEADER
#define STATLEGE2_HEADER
#include "statfrm.h"

class CuStatisticBarLegendList2 : public CuStatisticBarLegendList
{
public:
	CuStatisticBarLegendList2();
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuStatisticBarLegendList2)
	public:
	protected:
	//}}AFX_VIRTUAL

	afx_msg void OnSysColorChange();

	// Implementation
public:
	virtual ~CuStatisticBarLegendList2();

	// Generated message map functions
protected:

	//{{AFX_MSG(CuStatisticBarLegendList2)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif
