/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qframe.h, Header file    (MDI Child Frame) 
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Frame window for the SQL Test
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created
** 05-Jul-2001 (uk$so01)
**    SIR #105199. Integrate & Generate XML.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 22-Fev-2002 (uk$so01)
**    SIR #107133. Use the select loop instead of cursor when we get
**    all rows at one time.
** 21-Oct-2002 (uk$so01)
**    BUG/SIR #106156 Manually integrate the change #453850 into ingres30
** 13-Jun-2003 (uk$so01)
**    SIR #106648, Take into account the STAR database for connection.
** 17-Oct-2003 (uk$so01)
**    SIR #106648, Additional change to change #464605 (role/password)
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 01-Sept-2004 (schph01)
**    SIR #106648 add method SetErrorFileName in CfSqlQueryFrame class
** 09-Nov-2004 (uk$so01)
**    BUG #113416 / ISSUE #13771051 On Simplified Chinese WinXP.
**    In VDBA, if the SQL window is closed without a commit after inserting some 
**    queries, the query seem to be pending (uncommitted) until the VDBA 
**    application is closed.
*/

#if !defined(AFX_QFRAME_H__21BF266B_4A04_11D1_A20C_00609725DDAF__INCLUDED_)
#define AFX_QFRAME_H__21BF266B_4A04_11D1_A20C_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "qsplittr.h"    // Main Splitter.
#include "qepdoc.h"      // Document for the Qep.
#include "qredoc.h"      // Document Global.
#include "qreview.h"     // RichEdit view.
#include "qupview.h"     // Upper view.
#include "qlowview.h"    // Lower view.
#include "sqlselec.h"
#include "usrexcep.h"


//
// Exception used by the SQL Test.
class CeSqlQueryException: public CeSqlException
{
public:
	CeSqlQueryException():CeSqlException(_T("Unknown error occured.")){};
	CeSqlQueryException(LPCTSTR lpszReason): CeSqlException(lpszReason){}
	~CeSqlQueryException(){};
};



//
// class contains information about the statement.
class CaSqlQueryStatement
{
public:
	CaSqlQueryStatement(): m_strStatement(_T("")), m_strDatabase (_T("")), m_bSelect(FALSE){}
	CaSqlQueryStatement(LPCTSTR lpszStatement, LPCTSTR lpszDatabase, BOOL bSelect)
		: m_strStatement(lpszStatement),  m_strDatabase (lpszDatabase), m_bSelect(bSelect){};
	~CaSqlQueryStatement(){}
	void SetStatementPosition (LONG nStart, LONG nEnd){m_nStart=nStart; m_nEnd=nEnd;}
	
	LONG    m_nStart;
	LONG    m_nEnd;
	BOOL    m_bSelect;      // TRUE if the statement m_strStatement is a Select statement
	CString m_strStatement; // The statement string
	CString m_strDatabase;  // The database name
};


class CaSqlQueryData: public CObject
{
public:
	CaSqlQueryData(): m_strStatement (""), m_pPage(NULL), m_pQueryStatement(NULL), m_nAffectedRows(-1){};
	virtual ~CaSqlQueryData()
	{
		if (m_pQueryStatement)
			delete m_pQueryStatement;
	}

	CaSqlQueryStatement* m_pQueryStatement;
	CString      m_strStatement;
	CWnd*        m_pPage;
	long m_nAffectedRows; // For insert, delete, update statement (-1: not avalable)
};

class CaSqlQueryNonSelectData: public CaSqlQueryData
{
public:
	CaSqlQueryNonSelectData(): CaSqlQueryData() {}
	virtual ~CaSqlQueryNonSelectData(){}
	//
	// Data member ??
};

class CaSqlQuerySelectData: public CaSqlQueryData
{
public:
	CaSqlQuerySelectData(): CaSqlQueryData(), m_pCursor(NULL){};
	virtual ~CaSqlQuerySelectData()
	{
		if (m_pCursor)
			delete m_pCursor;
	}

	CaCursor* m_pCursor;
};


class CaSqlQueryQepData: public CaSqlQueryData
{
public:
	CaSqlQueryQepData(): CaSqlQueryData(), m_pQepDoc(NULL), m_bDeleteQepDoc(FALSE){};
	virtual ~CaSqlQueryQepData() 
	{
		if (m_bDeleteQepDoc && m_pQepDoc)
			delete m_pQepDoc;
	}
	void SetQepDoc(CdQueryExecutionPlanDoc* pDoc){m_pQepDoc = pDoc;}
	CdQueryExecutionPlanDoc* GetQepDoc(){return m_pQepDoc;}

	CdQueryExecutionPlanDoc* m_pQepDoc;
	BOOL m_bDeleteQepDoc;
};


class CfSqlQueryFrame : public CFrameWnd 
{
	DECLARE_DYNCREATE(CfSqlQueryFrame)
public:
	enum {RUN=0, QEP, XML};
	CfSqlQueryFrame();
	CfSqlQueryFrame(CdSqlQueryRichEditDoc* pDoc);
	BOOL IsAllViewsCreated(){return m_bAllViewCreated;}

	CuDlgSqlQueryResult*    GetDlgSqlQueryResult();
	CuDlgSqlQueryStatement* GetDlgSqlQueryStatement();
	CvSqlQueryRichEditView* GetRichEditView();
	CSplitterWnd*  GetMainSplitterWnd() {return &m_wndSplitter;}
	CSplitterWnd*  GetNestSplitterWnd() {return &m_wndNestSplitter;}
	CdSqlQueryRichEditDoc*  GetSqlDocument() {return m_pDoc;}
	BOOL IsCommittable();
	short GetReadLock();

	//
	// This function is called by 'OnButtonQueryQep' and 'OnButtonQueryGo'.
	// It parses to get the statement, and executes to display the result.
	BOOL Execute (int nExec = RUN);
	BOOL IsRunEnable();
	BOOL IsQepEnable();
	BOOL IsXmlEnable();
	BOOL IsClearEnable();
	BOOL IsTraceEnable();
	BOOL IsSaveAsEnable();
	BOOL IsPrintEnable();
	BOOL IsUsedTracePage();

	void Initiate(LPCTSTR lpszNode, LPCTSTR lpszServer, LPCTSTR lpszUser, LPCTSTR lpszOption = NULL);
	void SetDatabase(LPCTSTR lpszDatabase, long nFlag = 0);
	void SetErrorFileName(LPCTSTR lpszFileName);
	void SqlWizard();
	void Clear();
	void Open();
	void Saveas();
	void TracePage();
	void DoPrint();
	void DoPrintPreview();
	void GetData (IStream** ppStream);
	void SetData (IStream* pStream);
	void SetIniFleName(LPCTSTR lpszFileIni);
	void SetSessionStart(long nStart);
	void InvalidateCursor();
	void Commit();
	void Rollback();
	//
	// ConnectIfNeeded() return 0 if successful. In case of error the function
	// return a sqlca.sqlcode, and if bDisplayMessage = TRUE
	// the function also popups a message box.
	long ConnectIfNeeded(BOOL bDisplayMessage=TRUE);
	void SetSessionOption(UINT nMask, CaSqlQueryProperty* pProperty);
	//
	// nWhat = (1: autocommit state, 2: commit state, 3: readlock state)
	BOOL GetTransactionState(short nWhat);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfSqlQueryFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	//
	// Implementation
public:
	virtual ~CfSqlQueryFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

private:
	BOOL m_bUseCursor;
	BOOL m_bTraceIsUp;
	CdSqlQueryRichEditDoc*  m_pDoc;
	BOOL m_bEned;

protected:
	BOOL                   m_bAllViewCreated;
	CuQueryMainSplitterWnd m_wndSplitter;
	CSplitterWnd           m_wndNestSplitter;
	CTypedPtrList<CObList, CaSqlQueryData*> m_listResultPage;

	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	BOOL QEPExcuteSelectStatement    (CaSqlQueryQepData*       pData);
	BOOL QEPExcuteNonSelectStatement (CaSqlQueryQepData*       pData);
	BOOL ExcuteSelectStatement       (CaSqlQuerySelectData*    pData, BOOL& bStatInfo, int nIngresVersion);
	BOOL ExcuteGenerateXML           (CaSqlQuerySelectData*    pData, BOOL& bStatInfo);
	BOOL ExcuteNonSelectStatement    (CaSqlQueryNonSelectData* pData);
	long CompileSqlStatement (int nIngresVersion);
	BOOL SetPreviousMode();
	BOOL ParseStatement (BOOL bQep, CString& strRawStatement);
	
	BOOL ShouldButtonsBeEnable();
	void Cleanup();
	void Initialize();
public:
	// Generated message map functions
	//{{AFX_MSG(CfSqlQueryFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QFRAME_H__21BF266B_4A04_11D1_A20C_00609725DDAF__INCLUDED_)
