/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qredoc.h, Header file
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut
**    Purpose  : Document for (CRichEditView/CRichEditDoc) architecture.
**
** History:
**
** xx-Oct-1997 (uk$o01)
**    Created
** 31-Jan-2000 (uk$so01)
**     (Bug #100235)
**     Special SQL Test Setting when running on Context.
** 26-Sep-2000 (uk$so01)
**    SIR #102711: Callable in context command line improvement (for Manage-IT)
**    Select the input database if specified.
** 26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 13-Jun-2003 (uk$so01)
**    SIR #106648, Take into account the STAR database for connection.
** 03-Oct-2003 (uk$so01)
**    SIR #106648, Vdba-Split, Additional fix for GATEWAY Enhancement 
** 01-Sept-2004 (schph01)
**    SIR #106648 add variable member m_strErrorFileName in 
**    CdSqlQueryRichEditDoc class
** 21-Aug-2008 (whiro01)
**    Moved <afx...> include to "stdafx.h"
**/

#if !defined(AFX_QREDOC_H__3053D92B_4EEA_11D1_A214_00609725DDAF__INCLUDED_)
#define AFX_QREDOC_H__3053D92B_4EEA_11D1_A214_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "qresurow.h"
#include "sqlprop.h"

class CaSqlQueryMeasureData
{
public:
	CaSqlQueryMeasureData();
	~CaSqlQueryMeasureData(){}

	void Initialize();
	void SetAvailable (BOOL bAvailable = TRUE);
	BOOL IsAvailable() {return m_bAvailable;}
	BOOL IsStarted(){return m_bStarted;}
	BOOL IsEllapseStarted(){return (m_bStarted && m_bStarted);}
	void SetCompileTime (long lCompileTime);
	//
	// Start the mesure, and if bStartElap is TRUE then start stop watch from 
	// the current time (local machine) in order to calculate the ellapsed time.
	void StartMeasure(BOOL bStartElap = TRUE);
	//
	// Stop the stop watch, and cummulate the ellapsed time:
	void StopEllapse();
	//
	// Start stop watch from  the current time (local machine) 
	// in order to calculate the ellapsed time.
	void StartEllapse();

	//
	// End the mesure, and all functions Sart.., Stop... have no effect.
	void StopMeasure (BOOL bElap = TRUE);

	void SetGateway(BOOL bGateway){m_bGateway = bGateway;}
	BOOL IsGateway(){return m_bGateway;}



	CString m_strCom;
	CString m_strExec;
	CString m_strCost;
	CString m_strElap;

private:
	BOOL m_bGateway;
	BOOL m_bStarted;
	BOOL m_bStartedEllapse;
	BOOL m_bAvailable;
	long m_lCom;
	long m_lExec;
	long m_lCost;
	long m_lElap;

	CTime m_tStartTime;
	CTime m_tEndTime;
	long  m_lBiocntStart;
	long  m_lCpuMsStart;
	long  m_lBiocntEnd;
	long  m_lCpuMsEnd;
};



/////////////////////////////////////////////////////////////////////////////
// CdSqlQueryRichEditDoc document

class CaSession;
class CdSqlQueryRichEditDoc : public CRichEditDoc
{
	DECLARE_DYNCREATE(CdSqlQueryRichEditDoc)
public: 
	CdSqlQueryRichEditDoc();

	void SetDatabase(LPCTSTR lpszDatabase, long nFlag){m_strDatabase = lpszDatabase; m_nDatabaseFlag = (UINT)nFlag;}
	void SetServerClass(LPCTSTR lpszServer){m_strServerClass = lpszServer;}
	void SetUser(LPCTSTR lpszUser){m_strUser = lpszUser;}
	void SetErrorFileName(LPCTSTR lpszErrFile){m_strErrorFileName = lpszErrFile;}
	void SetConnectionOption(LPCTSTR lpszFlag){m_strConnectionFlag = lpszFlag;}

	CString GetConnectionOption(){return m_strConnectionFlag;}
	CString GetUser(){return m_strUser;}
	CString GetDatabase(){return m_strDatabase;}
	CString GetErrorFileName(){return m_strErrorFileName;}
	CString GetServerClass(){return m_strServerClass;}
	UINT GetDatabaseFlag(){return m_nDatabaseFlag;}


	CaSqlQueryMeasureData& GetMeasureData() {return m_measureData;}
	CaSession* GetCurrentSession(){return m_pCurrentSession;}
	void SetCurrentSession(CaSession* pCurrentSession){m_pCurrentSession = pCurrentSession;}

public:
	BOOL SaveModified(){return TRUE;}
	BOOL IsMultipleSelectAllowed(); // Can multiple statements be executed against this gateway ?

	BOOL IsSetQep() {return m_bSetQep;}
	BOOL IsSetOptimizeOnly(){return  m_bSetOptimizeOnly;}

	BOOL IsUsedTracePage(){return m_bUseTracePage;}
	void SetUsedTracePage(BOOL bSet){m_bUseTracePage = bSet;}
	void SetConnectParamInfo(int nVersion){m_nSessionVersion = nVersion;}
	int  GetConnectParamInfo(){return m_nSessionVersion;}
	//
	// Preference Settings:
	CaSqlQueryProperty& GetProperty(){return m_property;}

	void SetIniFleName(LPCTSTR lpszFileIni){m_strIniFileName = lpszFileIni;}
	CString GetIniFleName(){return m_strIniFileName;}
	BOOL IsAutoCommit(){return m_bAutoCommit;}
	void UpdateAutocommit(BOOL bNew);
	//
	// Data to be stored.
	// Current Selected Database
	// Number of Pages (List of IDDs) 
	// For each page, store its data.
	// Current selected page (Current Selected Tab)
	BOOL m_bLoaded;
	int  m_cyCurMainSplit;  // Main splitter
	int  m_cyMinMainSplit;  // Main splitter
	int  m_cyCurNestSplit;  // Nest splitter
	int  m_cyMinNestSplit;  // Nest splitter

	int  m_nTabLastSel;          // The last selected Tab before Deactivating the Raw Page.
	int  m_nResultCounter;       // Number of Statemement executed. (TAB)
	CString  m_strTitle;         // Document Title.
	CString  m_strDatabase;      // Current connected database.
	CString  m_strStatement;     // Global statement text.
	CString  m_strErrorFileName; // Current file name for storing the SQL errors.
	COLORREF m_crColorSelect;    // Color of the active statement
	COLORREF m_crColorText;      // Color of the statement

	BOOL     m_bHighlight;       // There is a current highlight statement.
	LONG     m_nStart;           // Start position of highlight
	LONG     m_nEnd;             // End position of highlight
	int      m_nTabCurSel;       // Current selected page (Tab)

	CString  m_strCom;           // Compile Time
	CString  m_strCost;          // Cost
	CString  m_strElap;          // Elape Time
	CString  m_strExec;          // Execute Time
	BOOL     m_bQEPxTRACE;       // Enable or Disable (the button of QEP and TRACE)
	BOOL     m_bHasBug;          // DBMS Server bug.
	//
	// List of Page Results
	CTypedPtrList< CObList, CaQueryPageData* > m_listPageResult;

	BOOL  IsLoadedDoc()            { return m_bLoaded; }
	int GetSplitterCyMinMainSplit(){ return m_cyMinMainSplit; }
	int GetSplitterCyCurMainSplit(){ return m_cyCurMainSplit; }
	int GetSplitterCyMinNestSplit(){ return m_cyMinNestSplit; }
	int GetSplitterCyCurNestSplit(){ return m_cyCurNestSplit; }
	BOOL IsStatementExecuted(){return m_bQueryExecuted;}
	void SetStatementExecuted(BOOL bSet){m_bQueryExecuted = bSet;}
	CString GetNode(){return m_strNode;}
	void SetNode(LPCTSTR lpszNode){m_strNode = lpszNode;}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CdSqlQueryRichEditDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL
	virtual CRichEditCntrItem* CreateClientItem(REOBJECT* preo) const;

	//
	// Implementation
public:
	virtual ~CdSqlQueryRichEditDoc();

	BOOL m_bSetQep;
	BOOL m_bSetOptimizeOnly;
	BOOL m_bAutoCommit;

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void CleanUp();

	BOOL m_bUseTracePage;        // TRUE if the Trace page exists
	BOOL m_bQueryExecuted;
	CString m_strNode;
	CString m_strServerClass;
	CString m_strConnectionFlag;
	CString m_strUser;
	UINT    m_nDatabaseFlag;
	int     m_nSessionVersion;

	CString m_strIniFileName;
	CaSqlQueryMeasureData m_measureData;
	CaSession* m_pCurrentSession;

private:
	CaSqlQueryProperty m_property;

	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CdSqlQueryRichEditDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QREDOC_H__3053D92B_4EEA_11D1_A214_00609725DDAF__INCLUDED_)
