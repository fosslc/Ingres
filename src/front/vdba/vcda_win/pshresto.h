/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : pshresto.h : header file
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Restore: PropertySheet Dialog box
**
** History:
**
** 18-Mar-2003 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
*/

#ifndef __PSHRESTO_H__
#define __PSHRESTO_H__

#include "ppagpat1.h"
#include "ppagpat2.h"
#include "ppgropti.h"
#include "ppgrfina.h"
#include "vcdadml.h"

class CxPSheetRestore : public CPropertySheet
{
	DECLARE_DYNAMIC(CxPSheetRestore)
public:
	CxPSheetRestore(CTypedPtrList< CObList, CaCdaDifference* >& listDifference, CWnd* pWndParent = NULL);
	virtual BOOL OnInitDialog();
	CaRestoreParam& GetData(){return m_data;}

	// Attributes
public:
	CuPropertyPageRestorePatch1 m_PagePatch1;
	CuPropertyPageRestorePatch2 m_PagePatch2;
	CuPropertyPageRestoreOption m_Page2;
	CuPropertyPageRestoreFinal  m_Page3;

	// Operations
public:
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxPSheetRestore)
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CxPSheetRestore();
	CaRestoreParam m_data;

	// Generated message map functions
protected:
	//{{AFX_MSG(CxPSheetRestore)
	//}}AFX_MSG
	afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()
};

#endif // __PSHRESTO_H__
