/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : frdtdiff.h : header file
**    Project  : INGRES II/VSDA Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Frame for detail difference page
**
** History:
**
** 10-Dec-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
*/


#if !defined(AFX_FRDTDIFF_H__6F33C6A3_0B84_11D7_87F9_00C04F1F754A__INCLUDED_)
#define AFX_FRDTDIFF_H__6F33C6A3_0B84_11D7_87F9_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "vwdtdiff.h"

class CdDifferenceDetail;
class CfDifferenceDetail : public CFrameWnd
{
	DECLARE_DYNCREATE(CfDifferenceDetail)
public:
	CfDifferenceDetail();
	CvDifferenceDetail* GetLeftPane();
	CvDifferenceDetail* GetRightPane ();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfDifferenceDetail)
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

	// Implementation
protected:
	virtual ~CfDifferenceDetail();

	BOOL m_bAllViewCreated;
	CSplitterWnd  m_WndSplitter;
	CdDifferenceDetail* m_pDoc;

	// Generated message map functions
	//{{AFX_MSG(CfDifferenceDetail)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FRDTDIFF_H__6F33C6A3_0B84_11D7_87F9_00C04F1F754A__INCLUDED_)
