/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**    Source   : taskanim.h, Header file 
**    Project  : Ingres Import Assistant
**    Author   : Sotheavut UK (uk$so01)
**
**    Purpose  : Dialog that run thread with animation, allow interupt task 
**               (Parameters and execute task)
**
**  History:
**  15-Mar-2001 (uk$so01)
**     created
**
*/

#if !defined (TASKANIM_HEADER)
#define TASKANIM_HEADER
#include "tkwait.h"
class CaLLAddAlterDrop;
class CaIIAInfo;
class CaSeparatorSet;

//
// RUN THE COPY STATEMENT EXEC SQL COPY ....
class CaExecParamSQLCopy: public CaExecParam
{
public:
	CaExecParamSQLCopy();
	virtual ~CaExecParamSQLCopy(){}
	virtual int  Run(HWND hWndTaskDlg = NULL);
	virtual void OnTerminate(HWND hWndTaskDlg = NULL);
	virtual BOOL OnCancelled(HWND hWndTaskDlg = NULL);

	BOOL IsSuccess(){return m_bSuccess;}
	void SetIIDecimalFormat (LPCTSTR lpszDecimalFormat){m_strIIDecimalFormat = lpszDecimalFormat;}
	void SetIIDateFormat (LPCTSTR lpszDateFormat){m_strIIDateFormat = lpszDateFormat;}
	void SetNewTable (LPCTSTR lpszStatement2CreateNewTable){m_strStatementCreateTable = lpszStatement2CreateNewTable;}
	void SetDeleteRow(LPCTSTR lpszStatementDeleteRows){m_strStatementDeleteRows = lpszStatementDeleteRows;}
	void SetAadParam (CaLLAddAlterDrop* pAadParam){m_pAadParam = pAadParam;}

private:
	CaLLAddAlterDrop* m_pAadParam;
	CString m_strMsgBoxTitle;

	CString m_strMsgOperationSuccess1;
	CString m_strMsgOperationSuccess2;
	CString m_strMsgOperationSuccess3;
	CString m_strMsgOperationSuccess4;
	CString m_strMsgConfirmInterrupt1;
	CString m_strMsgConfirmInterrupt2;
	CString m_strMsgConfirmInterrupt3;
	CString m_strMsgConfirmInterrupt4;
	CString m_strMsgFail; // Will be appened to the message from the DBMS
	CString m_strMsgStop;
	CString m_strMsgKillTask;
	CString m_strSQLCOPYReturnSmallLen;
	CString m_strMsgTaskNotResponds1;
	CString m_strMsgTaskNotResponds2;
	CString m_strErrorWithPartialCommit;

	CString m_strIIDecimalFormat;
	CString m_strIIDateFormat;
	CString m_strStatementDeleteRows;
	CString m_strStatementCreateTable;
	CString m_strConfirmInterrupt;
	BOOL m_bSuccess;
};



//
// RUN THE AUTOMATIC DETECTION OF SOLUTION ....
class CaExecParamDetectSolution: public CaExecParam
{
public:
	CaExecParamDetectSolution(CaIIAInfo* pIIAInfo);
	virtual ~CaExecParamDetectSolution(){}
	virtual int  Run(HWND hWndTaskDlg = NULL);
	virtual void OnTerminate(HWND hWndTaskDlg = NULL);
	virtual BOOL OnCancelled(HWND hWndTaskDlg = NULL);
	BOOL IsInterrupted(){return m_bInterrupted;}

private:
	CaIIAInfo* m_pIIAInfo;
	CString m_strMsgBoxTitle;
	CString m_strException;
	CString m_strMsgFailed;
	CString m_strInterruptDetection;
	BOOL m_bOutOfMemory;
	BOOL m_bInterrupted;
};


//
// RUN THE THREAD OF QUERYING RECORDS FOR DISPLAYING ....
class CaExecParamQueryRecords: public CaExecParam
{
public:
	CaExecParamQueryRecords(CaIIAInfo* pIIAInfo, CaSeparatorSet* pSet, int* pColumnCount);
	virtual ~CaExecParamQueryRecords(){}
	virtual int  Run(HWND hWndTaskDlg = NULL);
	virtual void OnTerminate(HWND hWndTaskDlg = NULL);
	virtual BOOL OnCancelled(HWND hWndTaskDlg = NULL);
	BOOL IsInterrupted(){return m_bInterrupted;}

private:

	int* m_pnColumnCount;
	CaSeparatorSet* m_pSet;
	CaIIAInfo* m_pIIAInfo;
	CString m_strMsgBoxTitle;
	CString m_strQuerying;
	CString m_strStopQuerying1;
	CString m_strStopQuerying2;
	BOOL m_bInterrupted;
};


#endif // TASKANIM_HEADER