/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vsdafrm.h, Header file (Manual Created Frame).
**    Project  : INGRES II/VSDA Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Frame for Visual Schema Diff control windows
**
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 17-Nov-2004 (uk$so01)
**    SIR #113475 (Add new functionality to allow user to save the results 
**    of a comparison into a CSV file.
*/

#if !defined(AFX_VSDAFRM_H__CC2DA2C7_B8F1_11D6_87D8_00C04F1F754A__INCLUDED_)
#define AFX_VSDAFRM_H__CC2DA2C7_B8F1_11D6_87D8_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
class CdSdaDoc;
class CaVsdItemData;
class CtrfItemData;
class CfSdaFrame : public CFrameWnd
{
	DECLARE_DYNCREATE(CfSdaFrame)
public:
	CfSdaFrame(); 

	CWnd* GetLeftPane();
	CWnd* GetRightPane ();
	BOOL IsAllViewCreated() {return m_bAllViewCreated;}
	CdSdaDoc* GetSdaDoc(){return m_pSdaDoc;}
	BOOL DoCompare (short nMode);
	BOOL DoWriteFile();
	BOOL ExistDifferences();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfSdaFrame)
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

	// Implementation
protected:
	virtual ~CfSdaFrame();
	void Init();
	void WriteFile (CFile& file, CtrfItemData* pItem, int nDepth);
	void WriteFileItem (CFile& file, CaVsdItemData* pVsdItemData, CtrfItemData* pItem);
	void HasDifference (CtrfItemData* pItem, int nDepth, BOOL& bHas);
	void HasDifferenceItem (CaVsdItemData* pVsdItemData, CtrfItemData* pItem, BOOL& bHas);

	BOOL m_bAllViewCreated;
	CSplitterWnd  m_WndSplitter;
	CdSdaDoc* m_pSdaDoc;

	// Generated message map functions
	//{{AFX_MSG(CfSdaFrame)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VSDAFRM_H__CC2DA2C7_B8F1_11D6_87D8_00C04F1F754A__INCLUDED_)
