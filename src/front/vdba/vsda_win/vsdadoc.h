/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vsdadoc.h, Header File 
**    Project  : INGRESII/VDDA (vsda.ocx)
**    Author   : UK Sotheavut
**    Purpose  : The Document Data for the VSDA Window.
**
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
*/

#ifndef VSDACTRLDOC_HEADER
#define VSDACTRLDOC_HEADER
#include "qryinfo.h"
#include "vsda.h"
#define  COMPARE_UNDEFINED  (-1)
#define  COMPARE_DBMS         0
#define  COMPARE_FILE         1

class CdSdaDoc : public CDocument
{
	DECLARE_SERIAL(CdSdaDoc)
public:
	CdSdaDoc();
	void SetSchema1Param(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszUser)
	{
		m_strNode1 = lpszNode;
		m_strDatabase1 = lpszDatabase;
		m_strUser1 = lpszUser;
		m_nSchema1Type = 0;
	}
	void SetSchema2Param(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszUser)
	{
		m_strNode2 = lpszNode;
		m_strDatabase2 = lpszDatabase;
		m_strUser2 = lpszUser;
		m_nSchema2Type = 0;
	}
	void LoadSchema1Param(LPCTSTR lpszFile)
	{
		m_strFile1 = lpszFile;
		m_nSchema1Type = 1;
	}
	void LoadSchema2Param(LPCTSTR lpszFile)
	{
		m_strFile2 = lpszFile;
		m_nSchema2Type = 1;
	}

	CString GetNode1(){return m_strNode1;}
	CString GetNode2(){return m_strNode2;}
	CString GetDatabase1(){return m_strDatabase1;}
	CString GetDatabase2(){return m_strDatabase2;}
	CString GetUser1(){return m_strUser1;}
	CString GetUser2(){return m_strUser2;}
	CString GetFile1(){return m_strFile1;}
	CString GetFile2(){return m_strFile2;}
	int GetSchema1Type(){return m_nSchema1Type;}
	int GetSchema2Type(){return m_nSchema2Type;}

protected:
	CString m_strNode1;
	CString m_strDatabase1;
	CString m_strUser1;
	CString m_strNode2;
	CString m_strDatabase2;
	CString m_strUser2;
	CString m_strFile1;
	CString m_strFile2;

	int m_nSchema1Type; // -1: undefined. 0: DBMS: 1: from File
	int m_nSchema2Type; // -1: undefined. 0: DBMS: 1: from File

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CdSdaDoc)
	public:
	virtual void Serialize(CArchive& ar); // overridden for document i/o
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CdSdaDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CdSdaDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // VSDACTRLDOC_HEADER
