/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlqryct.h : Declaration of the CSqlqueryCtrl ActiveX Control class.
**    Project  : sqlquery.ocx 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Control Class for drawing the ocx control
**
** History:
**
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 13-Jun-2003 (uk$so01)
**    SIR #106648, Take into account the STAR database for connection.
** 03-Oct-2003 (uk$so01)
**    SIR #106648, Vdba-Split, Additional fix for GATEWAY Enhancement 
** 17-Oct-2003 (uk$so01)
**    SIR #106648, Additional change to change #464605 (role/password)
** 01-Sept-2004 (schph01)
**    SIR #106648 add Dispatch maps and event IDs for sql error
** 22-Sep-2004 (uk$so01)
**    BUG #113104 / ISSUE 13690527, 
**    Add method: short GetConnected(LPCTSTR lpszNode, LPCTSTR lpszDatabase)
**    that return 1 if there is an SQL Session.
**/

#if !defined(AFX_SQLQRYCT_H__634C384B_A069_11D5_8769_00C04F1F754A__INCLUDED_)
#define AFX_SQLQRYCT_H__634C384B_A069_11D5_8769_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "qframe.h"
class CuDlgSqlQueryPageResult;

class CSqlqueryCtrl : public COleControl
{
	DECLARE_DYNCREATE(CSqlqueryCtrl)

public:
	CSqlqueryCtrl();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSqlqueryCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual void OnFontChanged();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	~CSqlqueryCtrl();

	void ConstructPropertySet(CaSqlQueryProperty& property);
	void NotifySettingChange(UINT nMask = 0);
	CfSqlQueryFrame* m_pSqlQueryFrame;
	CuDlgSqlQueryPageResult* m_pSelectResultPage;

	DECLARE_OLECREATE_EX(CSqlqueryCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CSqlqueryCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CSqlqueryCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CSqlqueryCtrl)      // Type name and misc status

	// Message maps
	//{{AFX_MSG(CSqlqueryCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Dispatch maps
	//{{AFX_DISPATCH(CSqlqueryCtrl)
	BOOL m_showGrid;
	afx_msg void OnShowGridChanged();
	BOOL m_autoCommit;
	afx_msg void OnAutoCommitChanged();
	BOOL m_readLock;
	afx_msg void OnReadLockChanged();
	long m_timeOut;
	afx_msg void OnTimeOutChanged();
	long m_selectLimit;
	afx_msg void OnSelectLimitChanged();
	long m_selectMode;
	afx_msg void OnSelectModeChanged();
	long m_maxTab;
	afx_msg void OnMaxTabChanged();
	long m_maxTraceSize;
	afx_msg void OnMaxTraceSizeChanged();
	BOOL m_showNonSelectTab;
	afx_msg void OnShowNonSelectTabChanged();
	BOOL m_traceTabActivated;
	afx_msg void OnTraceTabActivatedChanged();
	BOOL m_traceTabToTop;
	afx_msg void OnTraceTabToTopChanged();
	long m_f4Width;
	afx_msg void OnF4WidthChanged();
	long m_f4Scale;
	afx_msg void OnF4ScaleChanged();
	BOOL m_f4Exponential;
	afx_msg void OnF4ExponentialChanged();
	long m_f8Width;
	afx_msg void OnF8WidthChanged();
	long m_f8Scale;
	afx_msg void OnF8ScaleChanged();
	BOOL m_f8Exponential;
	afx_msg void OnF8ExponentialChanged();
	long m_qepDisplayMode;
	afx_msg void OnQepDisplayModeChanged();
	long m_xmlDisplayMode;
	afx_msg void OnXmlDisplayModeChanged();
	afx_msg BOOL Initiate(LPCTSTR lpszNode, LPCTSTR lpszServer, LPCTSTR lpszUser);
	afx_msg void SetDatabase(LPCTSTR lpszDatabase);
	afx_msg void Clear();
	afx_msg void Open();
	afx_msg void Save();
	afx_msg void SqlAssistant();
	afx_msg void Run();
	afx_msg void Qep();
	afx_msg void Xml();
	afx_msg BOOL IsRunEnable();
	afx_msg BOOL IsQepEnable();
	afx_msg BOOL IsXmlEnable();
	afx_msg void Print();
	afx_msg BOOL IsPrintEnable();
	afx_msg void EnableTrace();
	afx_msg BOOL IsClearEnable();
	afx_msg BOOL IsTraceEnable();
	afx_msg BOOL IsSaveAsEnable();
	afx_msg BOOL IsOpenEnable();
	afx_msg void PrintPreview();
	afx_msg BOOL IsPrintPreviewEnable();
	afx_msg SCODE Storing(LPUNKNOWN FAR* ppStream);
	afx_msg SCODE Loading(LPUNKNOWN pStream);
	afx_msg BOOL IsUsedTracePage();
	afx_msg void SetIniFleName(LPCTSTR lpszFileIni);
	afx_msg void SetSessionDescription(LPCTSTR lpszSessionDescription);
	afx_msg void SetSessionStart(long nStart);
	afx_msg void InvalidateCursor();
	afx_msg void Commit(short nCommit);
	afx_msg BOOL IsCommitEnable();
	afx_msg void CreateSelectResultPage(LPCTSTR lpszNode, LPCTSTR lpszServer, LPCTSTR lpszUser, LPCTSTR lpszDatabase, LPCTSTR lpszStatement);
	afx_msg void SetDatabaseStar(LPCTSTR lpszDatabase, long nFlag);
	afx_msg void CreateSelectResultPage4Star(LPCTSTR lpszNode, LPCTSTR lpszServer, LPCTSTR lpszUser, LPCTSTR lpszDatabase, long nDbFlag, LPCTSTR lpszStatement);
	afx_msg BOOL Initiate2(LPCTSTR lpszNode, LPCTSTR lpszServer, LPCTSTR lpszUser, LPCTSTR lpszOption);
	afx_msg void SetConnectParamInfo(long nSessionVersion);
	afx_msg long GetConnectParamInfo();
	afx_msg long ConnectIfNeeded(short nDisplayError);
	afx_msg BOOL GetSessionAutocommitState();
	afx_msg BOOL GetSessionCommitState();
	afx_msg BOOL GetSessionReadLockState();
	afx_msg void SetHelpFile(LPCTSTR lpszFileWithoutPath);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();
	afx_msg void SetErrorFileName(LPCTSTR lpszFileWithoutPath);

	// Event maps
	//{{AFX_EVENT(CSqlqueryCtrl)
	void FirePropertyChange()
		{FireEvent(eventidPropertyChange,EVENT_PARAM(VTS_NONE));}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

	// Dispatch and event IDs
public:
	enum {
		dispidGetConnected = 62L,		dispidSetErrorFileName = 61L,	//{{AFX_DISP_ID(CSqlqueryCtrl)
	//{{AFX_DISP_ID(CSqlqueryCtrl)
	dispidShowGrid = 1L,
	dispidAutoCommit = 2L,
	dispidReadLock = 3L,
	dispidTimeOut = 4L,
	dispidSelectLimit = 5L,
	dispidSelectMode = 6L,
	dispidMaxTab = 7L,
	dispidMaxTraceSize = 8L,
	dispidShowNonSelectTab = 9L,
	dispidTraceTabActivated = 10L,
	dispidTraceTabToTop = 11L,
	dispidF4Width = 12L,
	dispidF4Scale = 13L,
	dispidF4Exponential = 14L,
	dispidF8Width = 15L,
	dispidF8Scale = 16L,
	dispidF8Exponential = 17L,
	dispidQepDisplayMode = 18L,
	dispidXmlDisplayMode = 19L,
	dispidInitiate = 20L,
	dispidSetDatabase = 21L,
	dispidClear = 22L,
	dispidOpen = 23L,
	dispidSave = 24L,
	dispidSqlAssistant = 25L,
	dispidRun = 26L,
	dispidQep = 27L,
	dispidXml = 28L,
	dispidIsRunEnable = 29L,
	dispidIsQepEnable = 30L,
	dispidIsXmlEnable = 31L,
	dispidPrint = 32L,
	dispidIsPrintEnable = 33L,
	dispidEnableTrace = 34L,
	dispidIsClearEnable = 35L,
	dispidIsTraceEnable = 36L,
	dispidIsSaveAsEnable = 37L,
	dispidIsOpenEnable = 38L,
	dispidPrintPreview = 39L,
	dispidIsPrintPreviewEnable = 40L,
	dispidStoring = 41L,
	dispidLoading = 42L,
	dispidIsUsedTracePage = 43L,
	dispidSetIniFleName = 44L,
	dispidSetSessionDescription = 45L,
	dispidSetSessionStart = 46L,
	dispidInvalidateCursor = 47L,
	dispidCommit = 48L,
	dispidIsCommitEnable = 49L,
	dispidCreateSelectResultPage = 50L,
	dispidSetDatabaseStar = 51L,
	dispidCreateSelectResultPage4Star = 52L,
	dispidInitiate2 = 53L,
	dispidSetConnectParamInfo = 54L,
	dispidGetConnectParamInfo = 55L,
	dispidConnectIfNeeded = 56L,
	dispidGetSessionAutocommitState = 57L,
	dispidGetSessionCommitState = 58L,
	dispidGetSessionReadLockState = 59L,
	dispidSetHelpFile = 60L,
	eventidPropertyChange = 1L,
	//}}AFX_DISP_ID
	};
protected:
	SHORT GetConnected(LPCTSTR lpszNode, LPCTSTR lpszDatabase);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLQRYCT_H__634C384B_A069_11D5_8769_00C04F1F754A__INCLUDED)
