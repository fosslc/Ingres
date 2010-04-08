/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : fileerr.h : header file.
**    Project  : INGRES II/ SQL/Test.
**    Author   : Schalk Philippe (schph01)
**    Purpose  : interface for the CaFileError class and CaSqlErrorInfo class.
**
** History:
**
** 01-Sept-2004 (schph01)
**    Added
*/

#if !defined(AFX_FILEERR_H__DD2429F8_6D94_11D6_B6A5_00C04F1790C3__INCLUDED_)
#define AFX_FILEERR_H__DD2429F8_6D94_11D6_B6A5_00C04F1790C3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CaSqlErrorInfo class

class CaSqlErrorInfo : public CObList
{
// Construction
public:
	CaSqlErrorInfo():m_csSqlErrorText(_T("")),
	                 m_csSqlStatement(_T("")),
	                 m_csSqlCode(_T("")){}
	CaSqlErrorInfo( LPCTSTR lpszErrorText,
	                LPCTSTR lpszSqlStatement,
	                LPCTSTR lpszSqlCode):m_csSqlErrorText(lpszErrorText),
	                                     m_csSqlStatement(lpszSqlStatement),
	                                     m_csSqlCode(lpszSqlCode){}

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CaSqlErrorInfo)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CaSqlErrorInfo();
	LPCTSTR GetSqlCode()      {return m_csSqlCode;}
	LPCTSTR GetErrorText()    {return m_csSqlErrorText;}
	LPCTSTR GetSqlStatement() {return m_csSqlStatement;}
	LPCTSTR SetSqlCode(LPCTSTR lpCode)           {m_csSqlCode      = lpCode;}
	LPCTSTR SetErrorText(LPCTSTR lpErrorText)    {m_csSqlErrorText =lpErrorText;}
	LPCTSTR SetSqlStatement(LPCTSTR lpStatement) {m_csSqlStatement =lpStatement;}

protected:
	//{{AFX_MSG(CaSqlErrorInfo)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

private:
	CString m_csSqlCode;
	CString m_csSqlErrorText;
	CString m_csSqlStatement;
};

/////////////////////////////////////////////////////////////////////////////
// CaFileError class

class CaFileError
{
public:
	CaFileError();
	virtual ~CaFileError();

	void InitializeFilesNames();
	void WriteSqlErrorInFiles(LPCTSTR lpErrorText,LPCTSTR lpStatementText, LPCTSTR lpErrorNum);
	int ReadSqlErrorFromFile( CTypedPtrList<CPtrList, CaSqlErrorInfo*>* lstSqlError);
	BOOL GetStoreInFilesEnable(){return bStoreInFileActif;}

	LPCTSTR GetLocalErrorFile() {return m_strLocalSqlErrorFileName;}
	LPCTSTR GetGlobalErrorFile(){return m_strGlobalSqlErrorFileName;}
protected:
	BOOL SetLocalFileName();
	BOOL SetGlobalFileName();

	// File name to store the SQL errors
	CString m_strLocalSqlErrorFileName;  // SQL Error produce by this program
	CString m_strGlobalSqlErrorFileName; // SQL Error in files\errvdba.log
	BOOL bStoreInFileActif;

};


#endif // !defined(AFX_FILEERR_H__DD2429F8_6D94_11D6_B6A5_00C04F1790C3__INCLUDED_)


